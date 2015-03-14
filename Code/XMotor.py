# -*- coding:latin-1 -*-
import os
import struct
import time
import atexit
import signal

from PyQt4 import QtCore

from Code.Constantes import *
import Code.VarGen as VarGen
import Code.LCOS as LCOS
import Code.XMotorRespuesta as XMotorRespuesta

class XMotor(QtCore.QProcess):
    def __init__(self, nombre, exe, liOpcionesUCI=None, nMultiPV=None, priority=LCOS.NORMAL):
        QtCore.QProcess.__init__(self)

        absexe = os.path.abspath(exe)
        direxe = os.path.abspath(os.path.dirname(exe))
        self.setWorkingDirectory(direxe)

        if exe.lower().endswith(".exe") and VarGen.isLinux:
            self.start("/usr/bin/wine", [absexe], mode=QtCore.QIODevice.ReadWrite)
        else:
            self.start(absexe, [], mode=QtCore.QIODevice.ReadWrite)

        self.waitForStarted()
        LCOS.setPriorityQProcess(self, priority)

        self.nMultiPV = nMultiPV

        self.lockAC = True

        self.pid = self.pid()
        if VarGen.isWindows:
            hp, ht, self.pid, dt = struct.unpack("PPII", self.pid.asstring(16))
        atexit.register(self.cerrar)

        # Control de lectura
        self._buffer = ""

        self.siDebug = False
        self.nomDebug = nombre

        self.connect(self, QtCore.SIGNAL("readyReadStandardOutput()"), self._lee)

        self.guiDispatch = None
        self.ultDispatch = 0
        self.minDispatch = 1.0 #segundos
        self.whoDispatch = nombre
        self.siBlancas = True

        # Configuramos
        self.nombre = nombre
        self.uci = self.orden_uci()

        if liOpcionesUCI:
            for opcion, valor in liOpcionesUCI:
                if valor is None:  # button en motores externos
                    self.orden_ok("setoption name %s" % opcion)
                else:
                    if type(valor) == bool:
                        valor = str(valor).lower()
                    self.orden_ok("setoption name %s value %s" % (opcion, valor))
        if nMultiPV:
            self.ponMultiPV(nMultiPV)

    def ponGuiDispatch(self, guiDispatch, whoDispatch=None):
        self.guiDispatch = guiDispatch
        if whoDispatch is not None:
            self.whoDispatch = whoDispatch

    def ponMultiPV(self, nMultiPV):
        self.orden_ok("setoption name MultiPV value %s" % nMultiPV)

    def flush(self):
        self.readAllStandardOutput()
        self._buffer = ""

    def buffer(self):
        self._lee()
        resp = self._buffer
        self._buffer = ""
        return resp

    def apagar(self):
        self.write("stop\n")
        self.waitForReadyRead(90)
        self.write("quit\n")
        self.waitForReadyRead(90)
        self.write(chr(26) + "\n")
        self.waitForReadyRead(90)
        self.cerrar()

    def cerrar(self):
        if self.pid:
            try:
                os.kill(self.pid, signal.SIGTERM)
            except:
                self.close()
            self.pid = None

    def _lee(self):
        x = str(self.readAllStandardOutput())
        if x:
            self._buffer += x
            if self.siDebug:
                prlk(x)
            return True
        return False

    def escribe(self, linea):
        self.write(str(linea) + "\n")
        if self.siDebug:
            prlkn(self.nomDebug, "W", linea)

    def dispatch(self):
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.ExcludeUserInputEvents)
        if self.guiDispatch:
            tm = time.clock()
            if tm-self.ultDispatch < self.minDispatch:
                return True
            self.ultDispatch = tm
            mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, self.siBlancas)
            if self._buffer.endswith("\n"):
                b = self._buffer
            else:
                n = self._buffer.rfind("\n")
                b = self._buffer[:-n + 1] if n > 1 else ""
            mrm.dispatch(b)
            mrm.ordena()
            rm = mrm.mejorMov()
            rm.whoDispatch = self.whoDispatch
            if not self.guiDispatch(rm):
                return False
        return True

    def espera(self, txt, msStop, siStop=True):
        iniTiempo = time.clock()
        stop = False
        tamBuffer = len(self._buffer)
        while True:
            if tamBuffer != len(self._buffer):
                tamBuffer = len(self._buffer)
                if txt in self._buffer:
                    if self._buffer.endswith("\n"):
                        self.dispatch()
                        return True

            if not self.dispatch():
                return False

            queda = msStop - int((time.clock() - iniTiempo) * 1000)
            if queda <= 0:
                if stop:
                    return True
                if siStop:
                    self.escribe("stop")
                msStop += 2000
                stop = True
            self.waitForReadyRead(90)

    def orden_ok(self, orden):
        self.escribe(orden)
        self.escribe("isready")
        self.espera("readyok", 1000)
        return self.buffer()

    def orden_uci(self):
        self.escribe("uci")
        self.espera("uciok", 5000)
        return self.buffer()

    def orden_bestmove(self, orden, msMaxTiempo):
        self.flush()
        self.escribe(orden)
        self.espera("bestmove", msMaxTiempo, siStop=True)
        return self.buffer()

    def orden_infinito(self, busca, msMaxTiempo):
        self.flush()
        self.escribe("go infinite")
        self.espera(busca, msMaxTiempo)
        return self.orden_ok("stop")

    def posicionPartida(self, partida):
        posInicial = "startpos" if partida.siFenInicial()  else "fen %s" % partida.iniPosicion.fen()
        li = [jg.movimiento().lower() for jg in partida.liJugadas]
        moves = " moves %s" % (" ".join(li)) if li else ""
        self.orden_ok("position %s%s" % (posInicial, moves))

    def mejorMovP(self, partida, maxTiempo, maxProfundidad, siReturnTxt=False):
        self.posicionPartida(partida)
        self.siBlancas = siBlancas = partida.ultPosicion.siBlancas
        return self._mejorMov(maxTiempo, maxProfundidad, siReturnTxt, siBlancas)

    def mejorMovF(self, fen, maxTiempo, maxProfundidad, siReturnTxt=False):
        self.orden_ok("position fen %s" % fen)
        self.siBlancas = siBlancas = "w" in fen
        return self._mejorMov(maxTiempo, maxProfundidad, siReturnTxt, siBlancas)

    def _mejorMov(self, maxTiempo, maxProfundidad, siReturnTxt, siBlancas):
        env = "go"
        if maxProfundidad:
            env += " depth %d" % maxProfundidad
        elif maxTiempo:
            env += " movetime %d" % maxTiempo

        msTiempo = 10000
        if maxTiempo:
            msTiempo = maxTiempo
        elif maxProfundidad:
            msTiempo = int(maxProfundidad * msTiempo / 3.0)

        resp = self.orden_bestmove(env, msTiempo)
        if not resp:
            return None

        mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, siBlancas)
        mrm.dispatch(resp, maxTiempo, maxProfundidad)
        mrm.maxTiempo = maxTiempo
        mrm.maxProfundidad = maxProfundidad
        if siReturnTxt:
            return mrm, resp
        else:
            return mrm

    def mejorMovInfinitoP(self, partida, maxProfundidad):
        self.posicionPartida(partida)
        self.siBlancas = siBlancas = partida.ultPosicion.siBlancas
        return self._mejorMovInfinito(maxProfundidad, siBlancas)

    def mejorMovInfinitoF(self, fen, maxProfundidad):
        self.orden_ok("position fen %s" % fen)
        self.siBlancas = siBlancas = "w" in fen
        return self._mejorMovInfinito(maxProfundidad, siBlancas)

    def _mejorMovInfinito(self, maxProfundidad, siBlancas):
        busca = " depth %d " % maxProfundidad

        tiempo = maxProfundidad * 2000
        if maxProfundidad > 9:
            tiempo += (maxProfundidad - 9) * 20000

        resp = self.orden_infinito(busca, tiempo)
        if not resp:
            return None

        mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, siBlancas)
        mrm.dispatch(resp, None, maxProfundidad)
        mrm.maxTiempo = None
        mrm.maxProfundidad = maxProfundidad
        return mrm

    def mejorMovInfinitoTiempoP(self, partida, maxTiempo):
        self.posicionPartida(partida)
        self.siBlancas = siBlancas = partida.ultPosicion.siBlancas
        return self._mejorMovInfinitoTiempo(maxTiempo, siBlancas)

    def mejorMovInfinitoTiempoF(self, fen, maxTiempo):
        self.orden_ok("position fen %s" % fen)
        self.siBlancas = siBlancas = "w" in fen
        return self._mejorMovInfinitoTiempo(maxTiempo, siBlancas)

    def _mejorMovInfinitoTiempo(self, maxTiempo, siBlancas):
        busca = " @@ "  # que no busque nada
        resp = self.orden_infinito(busca, maxTiempo)
        if not resp:
            return None

        mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, siBlancas)
        mrm.dispatch(resp, maxTiempo, None)
        mrm.maxTiempo = maxTiempo
        mrm.maxProfundidad = None
        return mrm

    def mejorMovTiempoP(self, partida, tiempoBlancas, tiempoNegras, tiempoJugada):
        self.posicionPartida(partida)
        self.siBlancas = siBlancas = partida.ultPosicion.siBlancas
        return self._mejorMovTiempo(partida.ultPosicion.fen(), tiempoBlancas, tiempoNegras, tiempoJugada, siBlancas)

    def mejorMovTiempoF(self, fen, tiempoBlancas, tiempoNegras, tiempoJugada):
        self.orden_ok("position fen %s" % fen)
        self.siBlancas = siBlancas = "w" in fen
        return self._mejorMovTiempo(fen, tiempoBlancas, tiempoNegras, tiempoJugada, siBlancas)

    def _mejorMovTiempo(self, fen, tiempoBlancas, tiempoNegras, tiempoJugada, siBlancas):
        env = "go wtime %d btime %d" % ( tiempoBlancas, tiempoNegras )
        if tiempoJugada:
            env += " winc %d" % tiempoJugada
        maxtiempo = tiempoBlancas if " w " in fen else tiempoNegras
        resp = self.orden_bestmove(env, maxtiempo)
        if not resp:
            return None

        mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, siBlancas)
        mrm.dispatch(resp, None, None)
        return mrm

    def ac_inicio(self, partida):
        self.lockAC = True
        self.posicionPartida(partida)
        self.siBlancas = partida.ultPosicion.siBlancas
        self.connect(self, QtCore.SIGNAL("readyReadStandardOutput()"), self.ac_lee)
        self.mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, self.siBlancas)
        self.flush()
        self.escribe("go infinite")
        self.waitForReadyRead(90)
        self.lockAC = False

    def ac_lee(self):
        if self.lockAC:
            return
        self._lee()
        li = self._buffer.split("\n")
        if self._buffer.endswith("\n"):
            self._buffer = ""
        else:
            self._buffer = li[-1]
            li = li[:-1]
        for linea in li:
            self.mrm.dispatchLinea(linea)

    def ac_estado(self):
        self.mrm.ordena()
        return self.mrm

    def ac_minimo(self, minimoTiempo, lockAC):
        self.mrm.ordena()
        rm = self.mrm.mejorMov()
        dt = rm.time
        while dt < minimoTiempo:
            self.mrm.ordena()
            rm = self.mrm.mejorMov()
            self.waitForReadyRead(100)
            dt += 100
        self.lockAC = lockAC
        return self.ac_estado()

    def ac_minimoTD(self, minTime, minDepth, lockAC):
        self.mrm.ordena()
        rm = self.mrm.mejorMov()
        dt = rm.time
        while rm.time < minTime or rm.depth < minDepth:
            self.mrm.ordena()
            rm = self.mrm.mejorMov()
            self.waitForReadyRead(100)
            dt += 100
        self.lockAC = lockAC
        return self.ac_estado()

    def ac_final(self, minimoTiempo):
        self.ac_minimo(minimoTiempo, True)
        self.escribe("stop")
        self.waitForReadyRead(90)
        return self.ac_estado()
