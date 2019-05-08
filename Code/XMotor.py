import sys
import os
import signal
import time
import psutil

from PyQt4 import QtCore
import subprocess

from Code import VarGen
from Code import Util
from Code import XMotorRespuesta
from Code import EngineThread
from Code.QT import QTUtil2


class XMotor:
    def __init__(self, nombre, exe, liOpcionesUCI=None, nMultiPV=0, priority=None, args=None):
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

        self.log = None

        self.uci_lines = []

        if not os.path.isfile(exe):
            QTUtil2.mensError(None, "%s:\n  %s" % (_("Engine not found"), exe))
            return

        self.engine = EngineThread.Engine(exe, priority, args)
        self.engine.start()

        self.lockAC = True
        self.pid = self.engine.pid

        self.order_uci()

        txt_uci_analysemode = "UCI_AnalyseMode"
        uci_analysemode = False

        setoptions = False
        if liOpcionesUCI:
            for opcion, valor in liOpcionesUCI:
                if type(valor) == bool:
                    valor = str(valor).lower()
                self.set_option(opcion, valor)
                setoptions = True
                if opcion == txt_uci_analysemode:
                    uci_analysemode = True
                if opcion.lower() == "ponder":
                    self.ponder = valor == "true"

        self.nMultiPV = nMultiPV
        if nMultiPV:
            self.ponMultiPV(nMultiPV)
            if not uci_analysemode:
                for line in self.uci_lines:
                    if "UCI_AnalyseMode" in line:
                        self.set_option("UCI_AnalyseMode", "true")
                        setoptions = True
        if setoptions:
            self.put_line("isready")
            self.wait_mrm("readyok", 1000)

        self.ucinewgame()

    def log_open(self, fichero):
        self.log = open(fichero, "ab")
        self.log.write("%s %s\n\n" % (str(Util.hoy()), "-"*70))

    def log_close(self):
        if self.log:
            self.log.close()
            self.log = None

    def log_write(self, line):
        self.log.write(line)

    def get_lines(self):
        li = self.engine.get_lines()
        if self.log:
            for line in li:
                self.log_write(line)
        return li

    def put_line(self, line):
        self.engine.put_line(line)
        if self.log:
            self.log_write(">>> %s\n" % line)

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
                    self.put_line("stop")
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

    def set_game_position(self, partida, njg=99999):
        posInicial = "startpos" if partida.siFenInicial() else "fen %s" % partida.iniPosicion.fen()
        li = [jg.movimiento().lower() for n, jg in enumerate(partida.liJugadas) if n < njg]
        moves = " moves %s" % (" ".join(li)) if li else ""
        if not li:
            self.ucinewgame()
        self.work_ok("position %s%s" % (posInicial, moves))
        self.is_white = partida.siBlancas() if njg > 9000 else partida.jugada(njg).siBlancas()

    def set_fen_position(self, fen):
        self.ucinewgame()
        self.work_ok("position fen %s" % fen)
        self.is_white = "w" in fen

    def ucinewgame(self):
        self.work_ok("ucinewgame")

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
        tm = rm.time # problema cuando da por terminada la lectura y el rm.time siempre es el mismo
        while rm.time < minimoTiempo and tm < minimoTiempo:
            self.ac_lee()
            time.sleep(0.1)
            tm += 100
            rm = self.mrm.mejorMov()
        self.lockAC = lockAC
        return self.ac_estado()

    def ac_minimoTD(self, minTime, minDepth, lockAC):
        self.ac_lee()
        self.mrm.ordena()
        rm = self.mrm.mejorMov()
        while rm.time < minTime or rm.depth < minDepth:
            self.ac_lee()
            time.sleep(0.1)
            rm = self.mrm.mejorMov()
        self.lockAC = lockAC
        return self.ac_estado()

    def ac_final(self, minimoTiempo):
        self.ac_minimo(minimoTiempo, True)
        self.put_line("stop")
        time.sleep(0.1)
        return self.ac_estado()

    def analysis_stable(self, partida, njg, ktime, kdepth, is_savelines, st_centipawns, st_depths, st_timelimit):
        self.set_game_position(partida, njg)
        self.reset()
        if is_savelines:
            self.mrm.save_lines()
        self.put_line("go infinite")
        def lee():
            for line in self.get_lines():
                self.mrm.dispatch(line)
            self.mrm.ordena()
            return self.mrm.mejorMov()
        ok_time = False if ktime else True
        ok_depth = False if kdepth else True
        while self.guiDispatch(None):
            rm = lee()
            if not ok_time:
                ok_time = rm.time >= ktime
            if not ok_depth:
                ok_depth = rm.depth >= kdepth
            if ok_time and ok_depth:
                break
            time.sleep(0.1)

        if st_timelimit == 0:
            st_timelimit = 999999
        while not self.mrm.is_stable(st_centipawns, st_depths) and self.guiDispatch(None) and st_timelimit > 0.0:
            time.sleep(0.1)
            st_timelimit -= 0.1
            lee()
        self.put_line("stop")
        return self.mrm

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
            except:
                os.kill(self.pid, signal.SIGTERM)
                sys.stderr.write("INFO X CLOSE: except - the engine %s won't close properly.\n" % self.nombre)
            self.pid = None
        if self.log:
            self.log_close()

    def order_uci(self):
        self.reset()
        self.put_line("uci")
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

    def bestmove_game_jg(self, partida, njg, max_time, max_depth, is_savelines=False):
        self.set_game_position(partida, njg)
        return self.seek_bestmove(max_time, max_depth, is_savelines)

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
        if not li:
            self.ucinewgame()
        self.pondering = True
        self.work_ok("position %s%s" % (posInicial, moves))
        self.put_line("go ponder")

    def stop_ponder(self):
        self.work_ok("stop")
        self.pondering = False


class FastEngine(object):
    def __init__(self, nombre, exe, liOpcionesUCI=None, nMultiPV=0, priority=None, args=None):
        self.nombre = nombre

        self.ponder = False
        self.pondering = False

        self.is_white = True

        self.guiDispatch = None
        self.minDispatch = 1.0
        self.ultDispatch = 0

        self.uci_ok = False
        self.pid = None

        self.log = None

        self.uci_lines = []

        if not os.path.isfile(exe):
            return

        self.exe = exe = os.path.abspath(exe)
        direxe = os.path.dirname(exe)
        xargs = [os.path.basename(exe), ]
        if args:
            xargs.extend(args)

        if VarGen.isLinux and VarGen.isWine and exe.lower().endswith(".exe"):
            xargs.insert(0, "/usr/bin/wine")

        if VarGen.isWindows:
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = subprocess.SW_HIDE
        else:
            startupinfo = None
        curdir = os.path.abspath(os.curdir)
        os.chdir(direxe)
        self.process = subprocess.Popen(xargs, stdout=subprocess.PIPE, stdin=subprocess.PIPE,
                                         startupinfo=startupinfo, shell=False)
        os.chdir(curdir)

        self.pid = self.process.pid
        if priority is not None:
            p = psutil.Process(self.pid)
            p.nice(EngineThread.priorities.value(priority))

        self.stdout = self.process.stdout
        self.stdin = self.process.stdin

        self.orden_uci()

        setoptions = False
        if liOpcionesUCI:
            for opcion, valor in liOpcionesUCI:
                if type(valor) == bool:
                    valor = str(valor).lower()
                self.set_option(opcion, valor)
                setoptions = True
                if opcion.lower() == "ponder":
                    self.ponder = valor == "true"

        if setoptions:
            self.pwait_list("isready", "readyok", 1000)
        self.ucinewgame()

    def ponGuiDispatch(self, guiDispatch, whoDispatch=None):
        self.guiDispatch = guiDispatch

    def put_line(self, line):
        self.stdin.write(line + "\n")
        if self.log:
            self.log_write(">>> %s" % line)

    def log_open(self, fichero):
        self.log = open(fichero, "ab")
        self.log.write("%s %s\n\n" % (str(Util.hoy()), "-"*70))

    def log_close(self):
        if self.log:
            self.log.close()
            self.log = None

    def log_write(self, line):
        self.log.write("%s\n" % line)

    def pwait_list(self, orden, txt_busca, maxtime):
        self.put_line(orden)
        ini = time.time()
        li = []
        while time.time()-ini < maxtime:
            line = self.stdout.readline()
            if self.log:
                self.log_write(line.strip())
            li.append(line.strip())
            if line.startswith(txt_busca):
                return li, True
        return li, False

    def pwait_list_dispatch(self, orden, txt_busca, maxtime):
        self.put_line(orden)
        ini = time.time()
        tm_dispatch = ini
        li = []
        mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, self.is_white)
        while time.time()-ini < maxtime:
            if (time.time() - tm_dispatch) >= 1.0:
                mrm.ordena()
                rm = mrm.mejorMov()
                if not self.guiDispatch(rm):
                    return li, False
                tm_dispatch = time.time()
            line = self.stdout.readline().strip()
            if self.log:
                self.log_write(line)
            li.append(line)
            mrm.dispatch(line)
            if line.startswith(txt_busca):
                return li, True
        return li, False

    def orden_uci(self):
        li, self.uci_ok = self.pwait_list("uci", "uciok", 10000)
        self.uci_lines = [x for x in li if x.startswith("id ") or x.startswith("option name")] if self.uci_ok else []

    def ready_ok(self):
        li, readyok = self.pwait_list("isready", "readyok", 10000)
        return readyok

    def work_ok(self, orden):
        self.put_line(orden)
        return self.ready_ok()

    def ucinewgame(self):
        self.put_line("ucinewgame")

    def set_option(self, name, value):
        if value:
            self.put_line("setoption name %s value %s" % (name, value))
        else:
            self.put_line("setoption name %s" % name)

    def bestmove_fen(self, fen, maxTiempo, maxProfundidad):
        self.work_ok("position fen %s" % fen)
        self.siBlancas = siBlancas = "w" in fen
        return self._mejorMov(maxTiempo, maxProfundidad, siBlancas)

    def _mejorMov(self, maxTiempo, maxProfundidad, siBlancas):
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

        if self.guiDispatch:
            li_resp, result = self.pwait_list_dispatch(env, "bestmove", msTiempo)
        else:
            li_resp, result = self.pwait_list(env, "bestmove", msTiempo)

        if not result:
            return None

        mrm = XMotorRespuesta.MRespuestaMotor(self.nombre, siBlancas)
        for linea in li_resp:
            mrm.dispatch(linea)
        mrm.maxTiempo = maxTiempo
        mrm.maxProfundidad = maxProfundidad
        mrm.ordena()
        return mrm

    def close(self):
        if self.pid:
            if self.process.poll() is None:
                self.put_line("stop")
                self.put_line("quit")

                self.process.kill()
                self.process.terminate()

            self.pid = None
        if self.log:
            self.log_close()
