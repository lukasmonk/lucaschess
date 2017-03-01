import os

import time

import psutil

from Code import VarGen
from Code.Constantes import *

DEBUG = False


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
            prlk("%0.04f %s" % (dif, line))
        tdbg[0] = t
    return True

if DEBUG:
    tdbg = [time.time()]

    xpr("DEBUG XMOTOR")

PRIORITY_NORMAL                  = psutil.NORMAL_PRIORITY_CLASS
PRIORITY_LOW, PRIORITY_VERYLOW   = psutil.BELOW_NORMAL_PRIORITY_CLASS, psutil.IDLE_PRIORITY_CLASS
PRIORITY_HIGH, PRIORITY_VERYHIGH = psutil.ABOVE_NORMAL_PRIORITY_CLASS, psutil.HIGH_PRIORITY_CLASS

import subprocess
import threading
import collections


class EnginePOP(object):
    def __init__(self, exe, priority, args):
        self.pid = None
        self.exe = os.path.abspath(exe)
        self.direxe = os.path.dirname(exe)
        self.priority = priority
        self.working = True
        self.liBuffer = []
        self.starting = True
        self.args = [self.exe, ]
        if args:
            self.args.extend(args)

    def cerrar(self):
        self.working = False

    def put_line(self, line):
        assert xpr("put>>> %s\n" % line)
        self.stdin_lock.acquire()
        self.stdin.write(line + "\n")
        self.stdin_lock.release()

    def get_lines(self):
        self.stdout_lock.acquire()
        li = self.liBuffer
        self.liBuffer = []
        self.stdout_lock.release()
        assert xprli(li)
        return li

    def hay_datos(self):
        return len(self.liBuffer) > 0

    def reset(self):
        self.get_lines()

    def xstdout_thread(self, stdout, lock):
        try:
            while self.working:
                line = stdout.readline()
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
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = subprocess.SW_HIDE
        self.process = subprocess.Popen(self.args, stdout=subprocess.PIPE, stdin=subprocess.PIPE, cwd=self.direxe,
                                         startupinfo=startupinfo, shell=False)

        self.pid = self.process.pid
        if self.priority != PRIORITY_NORMAL:
            p = psutil.Process(self.pid)
            p.nice(self.priority)

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
            self.process.kill()
            self.process.terminate()
            self.pid = None
