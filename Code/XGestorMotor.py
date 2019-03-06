import os

import LCEngine4 as LCEngine

from Code import VarGen
from Code import XMotor
from Code import XMotorRespuesta
from Code import EngineThread
from Code.Constantes import *


class ListaGestoresMotor:
    def __init__(self):
        self.lista = []

    def append(self, gestorMotor):
        self.lista.append(gestorMotor)

    def listaActivos(self):
        return [gestorMotor for gestorMotor in self.lista if gestorMotor.activo]

    def closeAll(self):
        for gestorMotor in self.lista:
            gestorMotor.terminar()


class GestorMotor:
    def __init__(self, procesador, confMotor, direct = False):
        self.procesador = procesador

        self.motor = None
        self.confMotor = confMotor
        self.nombre = confMotor.nombre
        self.clave = confMotor.clave
        self.nMultiPV = 0

        self.priority = EngineThread.priorities.normal

        self.dispatching = None

        self.activo = True  # No es suficiente con motor == None para saber si esta activo y se puede logear

        self.ficheroLog = None

        self.direct = direct
        VarGen.listaGestoresMotor.append(self)
        if VarGen.configuracion.siLogEngines:
            self.log_open()

    def set_direct(self):
        self.direct = True

    def opciones(self, tiempoJugada, profundidad, siMultiPV):
        self.motorTiempoJugada = tiempoJugada
        self.motorProfundidad = profundidad
        self.nMultiPV = self.confMotor.multiPV if siMultiPV else 0

        self.siInfinito = self.clave == "tarrasch"

        if self.clave in ("daydreamer", "cinnamon") and profundidad and profundidad == 1:
            self.motorProfundidad = 2

    def log_open(self):
        carpeta = os.path.join(VarGen.configuracion.carpeta, "EngineLogs")
        if not os.path.isdir(carpeta):
            os.mkdir(carpeta)
        plantlog = "%s_%%05d"% os.path.join(carpeta, self.nombre)
        pos = 1
        nomlog = plantlog % pos

        while os.path.isfile(nomlog):
            pos += 1
            nomlog = plantlog % pos
        self.ficheroLog = nomlog
        if self.motor:
            self.motor.log_open(nomlog)

    def log_close(self):
        self.ficheroLog = None
        if self.motor:
            self.motor.log_close()

    def cambiaOpciones(self, tiempoJugada, profundidad):
        self.motorTiempoJugada = tiempoJugada
        self.motorProfundidad = profundidad

    def setPriority(self, priority):
        self.priority = priority if priority else EngineThread.priorities.normal

    def maximizaMultiPV(self):
        self.nMultiPV = 9999

    def ponGuiDispatch(self, rutina, whoDispatch=None):
        if self.motor:
            self.motor.ponGuiDispatch(rutina, whoDispatch)
        else:
            self.dispatching = rutina, whoDispatch

    def actMultiPV(self, xMultiPV):
        self.confMotor.actMultiPV(xMultiPV)
        self.testEngine()
        self.motor.ponMultiPV(self.confMotor.multiPV)

    def anulaMultiPV(self):
        self.nMultiPV = 0

    def setMultiPV(self, nMultiPV):
        self.nMultiPV = nMultiPV

    def quitaGuiDispatch(self):
        if self.motor:
            self.motor.guiDispatch = None

    def testEngine(self, nMultiPV=0):
        if self.motor:
            return
        if self.nMultiPV:
            self.nMultiPV = min(self.nMultiPV, self.confMotor.maxMultiPV)

        exe = self.confMotor.ejecutable()
        args = self.confMotor.argumentos()
        liUCI = self.confMotor.liUCI
        if self.direct:
            self.motor = XMotor.FastEngine(self.nombre, exe, liUCI, self.nMultiPV, priority=self.priority, args = args)
        else:
            self.motor = XMotor.XMotor(self.nombre, exe, liUCI, self.nMultiPV, priority=self.priority, args = args)
        if self.confMotor.siDebug:
            self.motor.siDebug = True
            self.motor.nomDebug = self.confMotor.nomDebug
        if self.dispatching:
            rutina, whoDispatch = self.dispatching
            self.motor.ponGuiDispatch(rutina, whoDispatch)
        if self.ficheroLog:
            self.motor.log_open(self.ficheroLog)

    def juegaSegundos(self, segundos):
        self.testEngine()
        partida = self.procesador.gestor.partida
        if self.siInfinito:  # problema tarrasch
            mrm = self.motor.bestmove_infinite(partida, segundos * 1000)
        else:
            mrm = self.motor.bestmove_game(partida, segundos * 1000, None)
        return mrm.mejorMov() if mrm else None

    def juega(self, nAjustado=0):
        return self.juegaPartida(self.procesador.gestor.partida, nAjustado)

    def juegaPartida(self, partida, nAjustado=0):
        self.testEngine()

        if self.siInfinito:  # problema tarrasch
            if self.motorProfundidad:
                mrm = self.motor.bestmove_infinite_depth(partida, self.motorProfundidad)
            else:
                mrm = self.motor.bestmove_infinite(partida, self.motorTiempoJugada)

        else:
            mrm = self.motor.bestmove_game(partida, self.motorTiempoJugada, self.motorProfundidad)

        if nAjustado:
            mrm.partida = partida
            if nAjustado >= 1000:
                mrm.liPersonalidades = self.procesador.configuracion.liPersonalidades
                mrm.fenBase = partida.ultPosicion.fen()
            return mrm.mejorMovAjustado(nAjustado) if nAjustado != kAjustarPlayer else mrm
        else:
            return mrm.mejorMov()

    def juegaTiempo(self, tiempoBlancas, tiempoNegras, tiempoJugada, nAjustado=0):
        self.testEngine()
        if self.motorTiempoJugada or self.motorProfundidad:
            return self.juega(nAjustado)
        tiempoBlancas = int(tiempoBlancas * 1000)
        tiempoNegras = int(tiempoNegras * 1000)
        tiempoJugada = int(tiempoJugada * 1000)
        partida = self.procesador.gestor.partida
        mrm = self.motor.bestmove_time(partida, tiempoBlancas, tiempoNegras, tiempoJugada)
        if mrm is None:
            return None

        if nAjustado:
            mrm.partida = partida
            if nAjustado >= 1000:
                mrm.liPersonalidades = self.procesador.configuracion.liPersonalidades
                mrm.fenBase = partida.ultPosicion.fen()
            return mrm.mejorMovAjustado(nAjustado) if nAjustado != kAjustarPlayer else mrm
        else:
            return mrm.mejorMov()

    def juegaTiempoTorneo(self, tiempoBlancas, tiempoNegras, tiempoJugada):
        self.testEngine()
        if self.motor.pondering:
            self.motor.stop_ponder()
        partida = self.procesador.gestor.partida
        if self.motorTiempoJugada or self.motorProfundidad:
            mrm = self.motor.bestmove_game(partida, self.motorTiempoJugada, self.motorProfundidad)
        else:
            tiempoBlancas = int(tiempoBlancas * 1000)
            tiempoNegras = int(tiempoNegras * 1000)
            tiempoJugada = int(tiempoJugada * 1000)
            mrm = self.motor.bestmove_time(partida, tiempoBlancas, tiempoNegras, tiempoJugada)
        if self.motor and self.motor.ponder: # test si self.motor, ya que puede haber terminado en el ponder
            self.motor.run_ponder(partida, mrm)
        return mrm

    def analiza(self, fen):
        self.testEngine()
        return self.motor.bestmove_fen(fen, self.motorTiempoJugada, self.motorProfundidad)

    def valora(self, posicion, desde, hasta, coronacion):
        self.testEngine()

        posicionNueva = posicion.copia()
        posicionNueva.mover(desde, hasta, coronacion)

        fen = posicionNueva.fen()
        if LCEngine.fenTerminado(fen):
            # Par que llegue hasta aqui tiene que ser tablas
            rm = XMotorRespuesta.RespuestaMotor("", posicion.siBlancas)
            rm.sinInicializar = False
            self.sinMovimientos = True
            self.pv = desde+hasta+coronacion
            self.desde = desde
            self.hasta = hasta
            self.coronacion = coronacion
            return rm

        mrm = self.motor.bestmove_fen(fen, self.motorTiempoJugada, self.motorProfundidad)
        rm = mrm.mejorMov()
        rm.cambiaColor(posicion)
        mv = desde + hasta + (coronacion if coronacion else "")
        rm.pv = mv + " " + rm.pv
        rm.desde = desde
        rm.hasta = hasta
        rm.coronacion = coronacion if coronacion else ""
        rm.siBlancas = posicion.siBlancas
        return rm

    def control(self, fen, profundidad):
        self.testEngine()
        return self.motor.bestmove_fen(fen, 0, profundidad)

    def terminar(self):
        if self.motor:
            self.motor.close()
            self.motor = None
            self.activo = False

    def analizaJugada(self, jg, tiempo, depth=0, brDepth=5, brPuntos=50):
        self.testEngine()

        mrm = self.motor.bestmove_fen(jg.posicionBase.fen(), tiempo, depth, is_savelines=True)
        mv = jg.movimiento()
        if not mv:
            return mrm, 0
        rm, n = mrm.buscaRM(jg.movimiento())
        if rm:
            if n == 0:
                mrm.miraBrilliancies(brDepth, brPuntos)
            return mrm, n

        # No esta considerado, obliga a hacer el analisis de nuevo desde posicion
        if jg.siJaqueMate or jg.siTablas():
            rm = XMotorRespuesta.RespuestaMotor(self.nombre, jg.posicionBase.siBlancas)
            rm.desde = mv[:2]
            rm.hasta = mv[2:4]
            rm.coronacion = mv[4] if len(mv) == 5 else ""
            rm.pv = mv
        else:
            posicion = jg.posicion

            mrm1 = self.motor.bestmove_fen(posicion.fen(), tiempo, depth)
            if mrm1 and mrm1.liMultiPV:
                rm = mrm1.liMultiPV[0]
                rm.cambiaColor(posicion)
                rm.pv = mv + " " + rm.pv
            else:
                rm = XMotorRespuesta.RespuestaMotor(self.nombre, mrm1.siBlancas)
                rm.pv = mv
            rm.desde = mv[:2]
            rm.hasta = mv[2:4]
            rm.coronacion = mv[4] if len(mv) == 5 else ""
            rm.siBlancas = jg.posicionBase.siBlancas
        pos = mrm.agregaRM(rm)

        return mrm, pos

    def analizaJugadaPartida(self, partida, njg, tiempo, depth=0, brDepth=5, brPuntos=50,
                             stability=False, st_centipawns=0, st_depths=0, st_timelimit=0):
        self.testEngine()

        if stability:
            mrm = self.motor.analysis_stable(partida, njg, tiempo, depth, True, st_centipawns, st_depths, st_timelimit)
        else:
            mrm = self.motor.bestmove_game_jg(partida, njg, tiempo, depth, is_savelines=True)

        jg = partida.jugada(njg)
        mv = jg.movimiento()
        if not mv:
            return mrm, 0
        rm, n = mrm.buscaRM(mv)
        if rm:
            if n == 0:
                mrm.miraBrilliancies(brDepth, brPuntos)
            return mrm, n

        # No esta considerado, obliga a hacer el analisis de nuevo desde posicion
        if jg.siJaqueMate or jg.siTablas():
            rm = XMotorRespuesta.RespuestaMotor(self.nombre, jg.posicionBase.siBlancas)
            rm.desde = mv[:2]
            rm.hasta = mv[2:4]
            rm.coronacion = mv[4] if len(mv) == 5 else ""
            rm.pv = mv
        else:
            posicion = jg.posicion

            mrm1 = self.motor.bestmove_fen(posicion.fen(), tiempo, depth)
            if mrm1 and mrm1.liMultiPV:
                rm = mrm1.liMultiPV[0]
                rm.cambiaColor(posicion)
                rm.pv = mv + " " + rm.pv
            else:
                rm = XMotorRespuesta.RespuestaMotor(self.nombre, mrm1.siBlancas)
                rm.pv = mv
            rm.desde = mv[:2]
            rm.hasta = mv[2:4]
            rm.coronacion = mv[4] if len(mv) == 5 else ""
            rm.siBlancas = jg.posicionBase.siBlancas
        pos = mrm.agregaRM(rm)

        return mrm, pos

    def analizaVariante(self, jg, tiempo, siBlancas):
        self.testEngine()

        mrm = self.motor.bestmove_fen(jg.posicion.fen(), tiempo, None)
        if mrm.liMultiPV:
            rm = mrm.liMultiPV[0]
            # if siBlancas != jg.posicion.siBlancas:
            #     if rm.mate:
            #         rm.mate += +1 if rm.mate > 0 else -1
        else:
            rm = XMotorRespuesta.RespuestaMotor("", siBlancas)
        return rm

    def ac_inicio(self, partida):
        self.testEngine()
        self.motor.ac_inicio(partida)

    def ac_minimo(self, minTiempo, lockAC):
        self.testEngine()
        return self.motor.ac_minimo(minTiempo, lockAC)

    def ac_minimoTD(self, minTiempo, minDepth, lockAC):
        self.testEngine()
        return self.motor.ac_minimoTD(minTiempo, minDepth, lockAC)

    def ac_estado(self):
        self.testEngine()
        return self.motor.ac_estado()

    def ac_final(self, minTiempo):
        self.testEngine()
        return self.motor.ac_final(minTiempo)

    def set_option(self, name, value):
        self.testEngine()
        self.motor.set_option(name, value)

    def miraListaPV(self, fen, siUna):  #
        """Servicio para Opening lines-importar polyglot-generador de movimientos-emula un book polyglot"""
        mrm = self.analiza(fen)
        lipv = [rm.movimiento() for rm in mrm.liMultiPV]
        return lipv[0] if siUna else lipv

