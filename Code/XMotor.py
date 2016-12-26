import os
import signal
import struct
import time

import psutil

from PyQt4 import QtCore

from Code import VarGen
from Code.Constantes import *
from Code import XMotorRespuesta

DEBUG = False
tdbg = [time.time()]

def xpr(line):
    if DEBUG:
        t = time.time()
        prlk("%0.04f %s" % (t - tdbg[0], line))
        tdbg[0] = t
    return True

def xprli(li):
    if DEBUG:
        t = time.time()
        dif = t - tdbg[0]
        for line in li:
            prlk("%0.04f %s\n" % (dif, line))
        tdbg[0] = t
    return True

if DEBUG:
    xpr("DEBUG XMOTOR")

if VarGen.isLinux:
    PRIORITY_NORMAL = 0
    PRIORITY_LOW, PRIORITY_VERYLOW = 10, 20
    PRIORITY_HIGH, PRIORITY_VERYHIGH = -10, -20
else:
    PRIORITY_NORMAL                  = psutil.NORMAL_PRIORITY_CLASS
    PRIORITY_LOW, PRIORITY_VERYLOW   = psutil.BELOW_NORMAL_PRIORITY_CLASS, psutil.IDLE_PRIORITY_CLASS
    PRIORITY_HIGH, PRIORITY_VERYHIGH = psutil.ABOVE_NORMAL_PRIORITY_CLASS, psutil.HIGH_PRIORITY_CLASS

class Engine(QtCore.QThread):
    def __init__(self, exe, priority, args):
        QtCore.QThread.__init__(self)
        self.pid = None
        self.exe = os.path.abspath(exe)
        self.direxe = os.path.dirname(exe)
        self.priority = priority
        self.working = True
        self.mutex = QtCore.QMutex()
        self.libuffer = []
        self.lastline = ""
        self.starting = True
        self.args = args if args else []

    def cerrar(self):
        self.working = False
        self.wait()

    def put_line(self, line):
        assert xpr("put>>> %s\n" % line)
        self.process.write(line +"\n")

    def get_lines(self):
        self.mutex.lock()
        li = self.libuffer
        self.libuffer = []
        self.mutex.unlock()
        assert xprli(li)
        return li

    def hay_datos(self):
        return len(self.libuffer) > 0

    def reset(self):
        self.mutex.lock()
        self.libuffer = []
        self.lastline = ""
        self.mutex.unlock()

    def close(self):
        self.working = False
        self.wait()

    def run(self):
        self.process = QtCore.QProcess()
        self.process.setWorkingDirectory(self.direxe)
        self.process.start(self.exe, self.args, mode=QtCore.QIODevice.ReadWrite)
        self.process.waitForStarted()
        self.pid = self.process.pid()
        if VarGen.isWindows:
            hp, ht, self.pid, dt = struct.unpack("PPII", self.pid.asstring(16))
        if self.priority != PRIORITY_NORMAL:
            p = psutil.Process(self.pid)
            p.nice(self.priority)

        self.starting = False
        while self.working:
            if self.process.waitForReadyRead(90):
                x = str(self.process.readAllStandardOutput())
                if x:
                    self.mutex.lock()
                    if self.lastline:
                        x = self.lastline + x
                    self.lastline = ""
                    sifdl = x.endswith("\n")
                    li = x.split("\n")
                    if not sifdl:
                        self.lastline = li[-1]
                        li = li[:-1]
                    self.libuffer.extend(li)
                    if len(self.libuffer) > 2000:
                        self.libuffer = self.libuffer[1000:]
                    self.mutex.unlock()
        self.put_line("quit")
        self.process.kill()
        self.process.close()

class XMotor:
    def __init__(self, nombre, exe, liOpcionesUCI=None, nMultiPV=0, priority=PRIORITY_NORMAL, args=[]):
        self.nombre = nombre

        self.ponder = False
        self.pondering = False

        self.is_white = True

        self.guiDispatch = None
        self.ultDispatch = 0
        self.minDispatch = 1.0  # segundos
        self.whoDispatch = nombre
        self.uci_ok = False
        self.pid = None

        if not os.path.isfile(exe):
            return

        self.engine = Engine(exe, priority, args)
        self.engine.start()

        time.sleep(0.01)
        n = 100
        while self.engine.starting and n:
            time.sleep(0.1 if n > 50 else 0.3)
            n-= 1

        self.lockAC = True
        self.pid = self.engine.pid

        self.order_uci()

        if liOpcionesUCI:
            for opcion, valor in liOpcionesUCI:
                if type(valor) == bool:
                    valor = str(valor).lower()
                self.set_option(opcion, valor)
                if opcion.lower() == "ponder":
                    self.ponder = valor == "true"

        self.nMultiPV = nMultiPV
        if nMultiPV:
            self.ponMultiPV(nMultiPV)

    def get_lines(self):
        return self.engine.get_lines()

    def put_line(self, line):
        self.engine.put_line(line)

    def reset(self):
        self.get_lines()
        self.mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, self.is_white)

    def dispatch(self):
        QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.ExcludeUserInputEvents)
        if self.guiDispatch:
            tm = time.time()
            if tm - self.ultDispatch < self.minDispatch:
                return True
            self.ultDispatch = tm
            self.mrm.ordena()
            rm = self.mrm.mejorMov()
            rm.whoDispatch = self.whoDispatch
            if not self.guiDispatch(rm):
                return False
        return True

    def wait_mrm(self, seektxt, msStop):
        iniTiempo = time.time()
        stop = False
        while True:
            if self.engine.hay_datos():
                for line in self.get_lines():
                    self.mrm.dispatch(line)
                    if seektxt in line:
                        self.dispatch()
                        return True

            queda = msStop - int((time.time() - iniTiempo) * 1000)
            if queda <= 0:
                if stop:
                    return True
                self.put_line("stop")
                msStop += 2000
                stop = True
            if not self.engine.hay_datos():
                if not self.dispatch():
                    return False
                time.sleep(0.090)

    def wait_list(self, txt, msStop):
        iniTiempo = time.time()
        stop = False
        ok = False
        li = []
        while True:
            lt = self.get_lines()
            for line in lt:
                if txt in line:
                    ok = True
                    break
            li.extend(lt)
            if ok:
                return li, True

            queda = msStop - int((time.time() - iniTiempo) * 1000)
            if queda <= 0:
                if stop:
                    return li, False
                self.put_line("stop")
                msStop += 2000
                stop = True
            if not self.engine.hay_datos():
                time.sleep(0.090)

    def wait_txt(self, seektxt, msStop):
        iniTiempo = time.time()
        while True:
            lt = self.get_lines()
            for line in lt:
                if seektxt in line:
                    return True

            queda = msStop - int((time.time() - iniTiempo) * 1000)
            if queda <= 0:
                return False
            if not self.engine.hay_datos():
                time.sleep(0.090)

    def work_ok(self, orden):
        self.reset()
        self.put_line(orden)
        self.put_line("isready")
        return self.wait_list("readyok", 1000)

    def work_bestmove(self, orden, msmax_time):
        self.reset()
        self.put_line(orden)
        self.wait_mrm("bestmove", msmax_time)

    def work_infinite(self, busca, msmax_time):
        self.reset()
        self.put_line("go infinite")
        self.wait_mrm(busca, msmax_time)

    def seek_bestmove(self, max_time, max_depth, is_savelines):
        env = "go"
        if max_depth:
            env += " depth %d" % max_depth
        elif max_time:
            env += " movetime %d" % max_time

        ms_time = 10000
        if max_time:
            ms_time = max_time + 3000
        elif max_depth:
            ms_time = int(max_depth * ms_time / 3.0)

        self.reset()
        if is_savelines:
            self.mrm.save_lines()
        self.mrm.setTimeDepth(max_time, max_depth)

        self.work_bestmove(env, ms_time)

        self.mrm.ordena()

        return self.mrm

    def seek_infinite(self, max_depth, max_time):
        if max_depth:
            busca = " depth %d " % (max_depth + 1,)

            max_time = max_depth * 2000
            if max_depth > 9:
                max_time += (max_depth - 9) * 20000
        else:
            busca = " @@ "  # que no busque nada
            max_depth = None

        self.reset()
        self.mrm.setTimeDepth(max_time, max_depth)

        self.work_infinite(busca, max_time)

        self.mrm.ordena()
        return self.mrm

    def seek_bestmove_time(self, time_white, time_black, inc_time_move):
        env = "go wtime %d btime %d" % (time_white, time_black)
        if inc_time_move:
            env += " winc %d" % inc_time_move
        max_time = time_white if self.is_white else time_black

        self.reset()
        self.mrm.setTimeDepth(max_time, None)

        self.work_bestmove(env, max_time)

        self.mrm.ordena()
        return self.mrm

    def set_game_position(self, partida):
        posInicial = "startpos" if partida.siFenInicial() else "fen %s" % partida.iniPosicion.fen()
        li = [jg.movimiento().lower() for jg in partida.liJugadas]
        moves = " moves %s" % (" ".join(li)) if li else ""
        self.work_ok("position %s%s" % (posInicial, moves))
        self.is_white = partida.siBlancas()

    def set_fen_position(self, fen):
        self.work_ok("position fen %s" % fen)
        self.is_white = "w" in fen

    def ac_inicio(self, partida):
        self.lockAC = True
        self.set_game_position(partida)
        self.reset()
        self.put_line("go infinite")
        self.lockAC = False

    def ac_lee(self):
        if self.lockAC:
            return
        for line in self.get_lines():
            self.mrm.dispatch(line)

    def ac_estado(self):
        self.ac_lee()
        self.mrm.ordena()
        return self.mrm

    def ac_minimo(self, minimoTiempo, lockAC):
        self.ac_lee()
        self.mrm.ordena()
        rm = self.mrm.mejorMov()
        dt = rm.time
        while dt < minimoTiempo:
            self.ac_lee()
            time.sleep(0.1)
            dt += 100
        self.lockAC = lockAC
        return self.ac_estado()

    def ac_minimoTD(self, minTime, minDepth, lockAC):
        self.ac_lee()
        self.mrm.ordena()
        rm = self.mrm.mejorMov()
        dt = rm.time
        while dt < minTime or rm.depth < minDepth:
            self.ac_lee()
            time.sleep(0.1)
            dt += 100
        self.lockAC = lockAC
        return self.ac_estado()

    def ac_final(self, minimoTiempo):
        self.ac_minimo(minimoTiempo, True)
        self.put_line("stop")
        time.sleep(0.1)
        return self.ac_estado()

    def ponGuiDispatch(self, guiDispatch, whoDispatch=None):
        self.guiDispatch = guiDispatch
        if whoDispatch is not None:
            self.whoDispatch = whoDispatch

    def ponMultiPV(self, nMultiPV):
        self.work_ok("setoption name MultiPV value %s" % nMultiPV)

    def close(self):
        if self.pid:
            try:
                self.engine.close()
                os.kill(self.pid, signal.SIGTERM)
            except:
                pass
            self.pid = None

    def order_uci(self):
        self.reset()
        self.put_line("uci")
        # self.put_line("isready")
        li, self.uci_ok = self.wait_list("uciok", 10000)
        self.uci_lines = [x for x in li if x.startswith("id ") or x.startswith("option name")] if self.uci_ok else []

    def set_option(self, name, value):
        if value:
            self.put_line("setoption name %s value %s" % (name, value))
        else:
            self.put_line("setoption name %s" % name)

    def bestmove_game(self, partida, max_time, max_depth):
        self.set_game_position(partida)
        return self.seek_bestmove(max_time, max_depth, False)

    def bestmove_fen(self, fen, max_time, max_depth, is_savelines=False):
        self.set_fen_position(fen)
        return self.seek_bestmove(max_time, max_depth, is_savelines)

    def bestmove_infinite_depth(self, partida, max_depth):
        self.set_game_position(partida)
        mrm = self.seek_infinite(max_depth, None)
        self.put_line("stop")
        return mrm

    def bestmove_infinite(self, partida, max_time):
        self.set_game_position(partida)
        mrm = self.seek_infinite(None, max_time)
        self.put_line("stop")
        return mrm

    def bestmove_time(self, partida, time_white, time_black, inc_time_move):
        self.set_game_position(partida)
        return self.seek_bestmove_time(time_white, time_black, inc_time_move)

    def run_ponder(self, partida, mrm):
        posInicial = "startpos" if partida.siFenInicial() else "fen %s" % partida.iniPosicion.fen()
        li = [jg.movimiento().lower() for jg in partida.liJugadas]
        rm = mrm.rmBest()
        pv = rm.getPV()
        li1 = pv.split(" ")
        li.extend(li1[:2])
        moves = " moves %s" % (" ".join(li)) if li else ""
        self.pondering = True
        self.work_ok("position %s%s" % (posInicial, moves))
        self.put_line("go ponder")

    def stop_ponder(self):
        self.work_ok("stop")
        self.pondering = False

