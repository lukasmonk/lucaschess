import StringIO
import sys
import time
import wave

import pyaudio

from PyQt4 import QtCore

from Code import Util

DATABASE = "D"
PLAY_ESPERA = "P"
PLAY_SINESPERA = "N"
STOP = "S"
TERMINAR = "T"


class Orden:
    def __init__(self):
        self.clave = ""
        self.dv = {}

    def ponVar(self, nombre, valor):
        self.dv[nombre] = valor

    def bloqueEnvio(self):
        self.dv["__CLAVE__"] = self.clave
        return self.dv


class RunReplay:
    def __init__(self):
        self.CHUNK = 1024

        self.dbw = {}
        self.lock = 0

    def procesa(self, io, orden):
        dv = orden.dv
        if orden.clave == DATABASE:
            fichero = dv["FICHERO"]
            tabla = dv["TABLA"]

            self.dbw = {}  # como un reset
            self.add_db(fichero, tabla)
        elif orden.clave in (PLAY_ESPERA, PLAY_SINESPERA):

            if "CLAVE" in dv:
                li = (dv["CLAVE"],)
            else:
                li = dv["LISTACLAVES"]

            return self.play(io, li)

        elif orden.clave in TERMINAR:
            sys.exit(1)

        return None

    def add_db(self, fichero, tabla):
        db = Util.DicBLOB(fichero, tabla)
        keys = db.keys()

        for k in keys:
            self.add_bin(k, db[k])

        db.close()

    def add_bin(self, clave, xbin):

        f = StringIO.StringIO(xbin)
        self.add_wav(clave, f)

    def add_wav(self, clave, wav):

        wf = wave.open(wav, 'rb')

        if self.dbw is None:
            self.dbw = {}

        p = pyaudio.PyAudio()
        xformat = p.get_format_from_width(wf.getsampwidth())
        channels = wf.getnchannels()
        rate = wf.getframerate()
        p.terminate()

        self.dbw[clave] = (xformat, channels, rate, wf.readframes(wf.getnframes()))
        wf.close()

    def siClave(self, clave):
        return clave in self.dbw

    def play(self, io, liClaves):
        li = []
        for clave in liClaves:
            if clave in self.dbw:
                xformat, channels, rate, frames = self.dbw[clave]
                li.append(frames)
        frames = "".join(li)

        if not frames:
            return None

        p = pyaudio.PyAudio()
        stream = p.open(format=xformat, channels=channels, rate=rate, output=True)

        nTam = len(frames)
        nPos = 0
        orden = None

        t0 = time.time()
        for n in range(nTam):
            hasta = nPos + self.CHUNK
            stream.write(frames[nPos:hasta])
            nPos = hasta
            t1 = time.time()
            if (t1-t0) > 0.2:
                if io.orden_acabar():
                    break
                t0 = t1

        if orden is None:
            stream.stop_stream()
        stream.close()
        p.terminate()

        return orden


class IO(QtCore.QThread):
    def __init__(self):
        QtCore.QThread.__init__(self)
        self.ipc = []
        self.mutex = QtCore.QMutex()
        self.continuar = True

    def push(self, orden):
        self.mutex.lock()
        self.ipc.append(orden)
        self.mutex.unlock()

    def orden_acabar(self):
        for orden in self.ipc:
            if orden["__CLAVE__"] in (STOP, PLAY_SINESPERA, TERMINAR):
                return True
        return False

    def pop(self):
        self.mutex.lock()
        dv = self.ipc.pop(0) if self.ipc else None
        self.mutex.unlock()
        if not dv:
            return None

        orden = Orden()
        orden.clave = dv["__CLAVE__"]
        orden.dv = dv
        return orden

    def close(self):
        if self.continuar:
            self.continuar = False
        self.wait()

    def run(self):
        xreplay = RunReplay()
        orden = None
        while self.continuar:
            if orden:
                if orden.clave == TERMINAR:
                    break
                orden = xreplay.procesa(self, orden)
                if orden:
                    continue
            orden = self.pop()
            if orden is None:
                self.msleep(500)
        self.continuar = False
