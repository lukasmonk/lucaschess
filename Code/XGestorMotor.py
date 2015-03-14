# -*- coding:latin-1 -*-

from Code.Constantes import *

import Code.LCOS as LCOS
import Code.XMotor as XMotor
import Code.XMotorRespuesta as XMotorRespuesta

class GestorMotor:
    def __init__(self, procesador, confMotor):
        self.procesador = procesador

        self.siIniMotor = False
        self.confMotor = confMotor
        self.nombre = confMotor.nombre
        self.clave = confMotor.clave
        self.siMultiPV = False
        self.siMaximizaMultiPV = False

        self.priority = LCOS.NORMAL

        self.dispatch = None

    def opciones(self, tiempoJugada, profundidad, siMultiPV):
        self.motorTiempoJugada = tiempoJugada
        self.motorProfundidad = profundidad
        self.siMultiPV = siMultiPV

        self.siInfinito = self.clave == "tarrasch"

        if self.clave == "daydreamer" and profundidad and profundidad == 1:
            self.motorProfundidad = 2

    def cambiaOpciones(self, tiempoJugada, profundidad):
        self.motorTiempoJugada = tiempoJugada
        self.motorProfundidad = profundidad

    def setPriority(self, priority):
        self.priority = priority

    def maximizaMultiPV(self):
        self.siMaximizaMultiPV = True
        self.siMultiPV = True

    def ponGuiDispatch(self, rutina, whoDispatch=None):
        if not self.siIniMotor:
            self.iniciarMotor()
        self.motor.ponGuiDispatch(rutina, whoDispatch)

    def actMultiPV(self, xMultiPV):
        self.confMotor.actMultiPV(xMultiPV)
        if not self.siIniMotor:
            self.iniciarMotor()
        self.motor.ponMultiPV(self.confMotor.multiPV)

    def quitaGuiDispatch(self):
        if self.siIniMotor:
            self.motor.guiDispatch = None

    def iniciarMotor(self, nMultiPV=0):
        self.siIniMotor = True

        if self.siMultiPV:
            if self.siMaximizaMultiPV and self.confMotor.maxMultiPV:
                nMultiPV = self.confMotor.maxMultiPV
            else:
                if nMultiPV == 0:
                    nMultiPV = self.confMotor.multiPV
                else:
                    if self.confMotor.multiPV < nMultiPV:
                        nMultiPV = self.confMotor.multiPV

        exe = self.confMotor.ejecutable()
        liUCI = self.confMotor.liUCI
        self.motor = XMotor.XMotor(self.nombre, exe, liUCI, nMultiPV, priority=self.priority)
        if self.confMotor.siDebug:
            self.motor.siDebug = True
            self.motor.nomDebug = self.confMotor.nomDebug

    def juegaSegundos(self, segundos):
        if not self.siIniMotor:
            self.iniciarMotor()
        partida = self.procesador.gestor.partida
        if self.siInfinito:  # problema tarrasch
            mrm = self.motor.mejorMovInfinitoTiempoP(partida, segundos * 1000)
        else:
            mrm = self.motor.mejorMovP(partida, segundos * 1000, None)
        return mrm.mejorMov() if mrm else None

    def juega(self, nAjustado=0):
        return self.juegaPartida(self.procesador.gestor.partida, nAjustado)

    def juegaPartida(self, partida, nAjustado=0):
        if not self.siIniMotor:
            self.iniciarMotor()

        if self.siInfinito:  # problema tarrasch
            if self.motorProfundidad:
                mrm = self.motor.mejorMovInfinitoP(partida, self.motorProfundidad)
            else:
                mrm = self.motor.mejorMovInfinitoTiempoP(partida, self.motorTiempoJugada)

        else:
            mrm = self.motor.mejorMovP(partida, self.motorTiempoJugada, self.motorProfundidad)

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

    def juegaTiempo(self, tiempoBlancas, tiempoNegras, tiempoJugada, nAjustado=0):
        if self.motorTiempoJugada or self.motorProfundidad:
            return self.juega(nAjustado)
        if not self.siIniMotor:
            self.iniciarMotor()
        tiempoBlancas = int(tiempoBlancas * 1000)
        tiempoNegras = int(tiempoNegras * 1000)
        tiempoJugada = int(tiempoJugada * 1000)
        partida = self.procesador.gestor.partida
        mrm = self.motor.mejorMovTiempoP(partida, tiempoBlancas, tiempoNegras, tiempoJugada)
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
        if not self.siIniMotor:
            self.iniciarMotor()
        partida = self.procesador.gestor.partida
        if self.motorTiempoJugada or self.motorProfundidad:
            mrm = self.motor.mejorMovP(partida, self.motorTiempoJugada, self.motorProfundidad)
        else:
            tiempoBlancas = int(tiempoBlancas * 1000)
            tiempoNegras = int(tiempoNegras * 1000)
            tiempoJugada = int(tiempoJugada * 1000)
            mrm = self.motor.mejorMovTiempoP(partida, tiempoBlancas, tiempoNegras, tiempoJugada)
        return mrm

    def analiza(self, fen):
        if not self.siIniMotor:
            self.iniciarMotor()
        return self.motor.mejorMovF(fen, self.motorTiempoJugada, self.motorProfundidad)

    def valora(self, posicion, desde, hasta, coronacion):
        if not self.siIniMotor:
            self.iniciarMotor()
        if not hasattr(self, "ml"):
            self.ml = self.procesador.gestor.ml  # Motor interno en gestor.py

        posicionNueva = posicion.copia()
        posicionNueva.mover(desde, hasta, coronacion)

        fen = posicionNueva.fen()
        self.ml.ponFen(fen)

        if self.ml.siTerminada():
            return None

        mrm = self.motor.mejorMovF(fen, self.motorTiempoJugada, self.motorProfundidad)
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
        if not self.siIniMotor:
            self.iniciarMotor()

        return self.motor.mejorMovF(fen, 0, profundidad)

    def terminar(self):
        if self.siIniMotor:
            self.motor.apagar()
            self.siIniMotor = False  # necesario si se quiere reusar

    def analizaJugada(self, jg, tiempo, depth=0, brDepth=5, brPuntos=50):
        if not self.siIniMotor:
            self.iniciarMotor()

        resp = self.motor.mejorMovF(jg.posicionBase.fen(), tiempo, depth, siReturnTxt=True)
        if resp is None:
            return None
        mrm, txt = resp
        mv = jg.movimiento()
        if not mv:
            return mrm, 0
        rm, n = mrm.buscaRM(jg.movimiento())
        if rm:
            if n == 0:
                mrm.miraBrilliancies(txt, brDepth, brPuntos)
            return mrm, n

        # No está considerado, obliga a hacer el análisis de nuevo desde posicion
        if jg.siJaqueMate or jg.siTablas():
            rm = XMotorRespuesta.RespuestaMotor(self.nombre, jg.posicionBase.siBlancas)
            rm.desde = mv[:2]
            rm.hasta = mv[2:4]
            rm.coronacion = mv[4] if len(mv) == 5 else ""
            rm.pv = mv
        else:
            posicion = jg.posicion

            mrm1 = self.motor.mejorMovF(posicion.fen(), tiempo, depth)
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
        if not self.siIniMotor:
            self.iniciarMotor()

        mrm = self.motor.mejorMovF(jg.posicion.fen(), tiempo, None)
        if mrm.liMultiPV:
            rm = mrm.liMultiPV[0]
            if siBlancas != jg.posicion.siBlancas:
                if rm.mate:
                    rm.mate += +1 if rm.mate > 0 else -1
        else:
            rm = XMotorRespuesta.RespuestaMotor("", siBlancas)
        return rm

    def ac_inicio(self, partida):
        if not self.siIniMotor:
            self.iniciarMotor()

        self.motor.ac_inicio(partida)

    def ac_minimo(self, minTiempo, lockAC):
        return self.motor.ac_minimo(minTiempo, lockAC)

    def ac_minimoTD(self, minTiempo, minDepth, lockAC):
        return self.motor.ac_minimoTD(minTiempo, minDepth, lockAC)

    def ac_estado(self):
        return self.motor.ac_estado()

    def ac_final(self, minTiempo):
        return self.motor.ac_final(minTiempo)

