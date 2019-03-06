import os

import time

import psutil

from Code import VarGen
from Code.Constantes import *

DEBUG_ENGINE = False


def xpr(exe, line):
    if DEBUG_ENGINE:
        t = time.time()
        prlk("%0.04f %s" % (t - tdbg[0], line))
        tdbg[0] = t
    return True


def xprli(li):
    if DEBUG_ENGINE:
        t = time.time()
        dif = t - tdbg[0]
        for line in li:
            prlk("%0.04f %s" % (dif, line))
        tdbg[0] = t
    return True

if DEBUG_ENGINE:
    tdbg = [time.time()]
    xpr("", "DEBUG XMOTOR")


class Priorities:
    def __init__(self):
        self.normal, self.low, self.verylow, self.high, self.veryhigh = range(5)

        if VarGen.isLinux:
            p_normal = 0
            p_low, p_verylow = 10, 20
            p_high, p_veryhigh = -10, -20
        else:
            p_normal = psutil.NORMAL_PRIORITY_CLASS
            p_low, p_verylow = psutil.BELOW_NORMAL_PRIORITY_CLASS, psutil.IDLE_PRIORITY_CLASS
            p_high, p_veryhigh = psutil.ABOVE_NORMAL_PRIORITY_CLASS, psutil.HIGH_PRIORITY_CLASS

        self.values = [p_normal, p_low, p_verylow, p_high, p_veryhigh]

    def value(self, priority):
        return self.values[priority] if priority in range(5) else self.value(self.normal)

    def labels(self):
        return [_("Normal"), _("Low"), _("Very low"), _("High"), _("Very high")]

    def combo(self):
        labels = self.labels()
        return [(labels[pr], pr) for pr in range(5)]

    def texto(self, prioridad):
        return self.labels()[prioridad]

priorities = Priorities()

import subprocess
import threading
import collections


class Engine(object):
    def __init__(self, exe, priority, args):
        self.pid = None
        self.exe = os.path.abspath(exe)
        self.direxe = os.path.dirname(exe)
        self.priority = priority
        self.working = True
        self.liBuffer = []
        self.starting = True
        self.args = [os.path.basename(self.exe), ]
        if args:
            self.args.extend(args)

        if VarGen.isLinux and VarGen.isWine and self.exe.lower().endswith(".exe"):
            self.args.insert(0, "/usr/bin/wine")

    def cerrar(self):
        self.working = False

    def put_line(self, line):
        if self.working:
            assert xpr(self.exe, "put>>> %s\n" % line)
            self.stdin_lock.acquire()
            self.stdin.write(line + "\n")
            self.stdin_lock.release()

    def get_lines(self):
        self.stdout_lock.acquire()
        li = self.liBuffer
        self.liBuffer = []
        self.stdout_lock.release()
        return li

    def hay_datos(self):
        return len(self.liBuffer) > 0

    def reset(self):
        self.get_lines()

    def xstdout_thread(self, stdout, lock):
        try:
            while self.working:
                line = stdout.readline()
                assert xpr(self.exe, line)
                if not line:
                    break
                lock.acquire()
                self.liBuffer.append(line)
                lock.release()
        except:
            pass
        finally:
            stdout.close()

    def start(self):
        if VarGen.isWindows:
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = subprocess.SW_HIDE
        else:
            startupinfo = None
        curdir = os.path.abspath(os.curdir)  # problem with "." as curdir
        os.chdir(self.direxe)  # to fix problems with non ascii folders

        if VarGen.isLinux:
            argv0 = self.args[0]
            if "/" not in argv0:
                self.args[0] = os.path.join(os.path.abspath(os.curdir), argv0)

        self.process = subprocess.Popen(self.args, stdout=subprocess.PIPE, stdin=subprocess.PIPE,
                                         startupinfo=startupinfo, shell=False)
        os.chdir(curdir)

        self.pid = self.process.pid
        if self.priority is not None:
            p = psutil.Process(self.pid)
            p.nice(priorities.value(self.priority))

        self.stdout_lock = threading.Lock()
        self.stdout_queue = collections.deque()
        stdout_thread = threading.Thread(target=self.xstdout_thread, args=(self.process.stdout, self.stdout_lock))
        stdout_thread.daemon = True
        stdout_thread.start()

        self.stdin = self.process.stdin
        self.stdin_lock = threading.Lock()

        self.starting = False

    def close(self):
        self.working = False
        if self.pid:
            if self.process.poll() is None:
                self.put_line("stop")
                self.put_line("quit")
                self.process.kill()
                self.process.terminate()

            self.pid = None
