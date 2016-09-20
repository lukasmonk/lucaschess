import StringIO
import sys
import time
import wave

import pyaudio

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

class Replay:
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

        for n in range(nTam):
            hasta = nPos + self.CHUNK
            stream.write(frames[nPos:hasta])
            nPos = hasta
            orden = io.recibe()
            if orden:
                if orden.clave in (STOP, PLAY_SINESPERA, TERMINAR):
                    break

        if orden is None:
            stream.stop_stream()
        stream.close()
        p.terminate()

        return orden

class IO:
    def __init__(self, xcpu, fdb):
        self.ipc = Util.IPC(fdb, False)
        self.xcpu = xcpu

    def recibe(self):
        dv = self.ipc.pop()
        if not dv:
            return None

        orden = Orden()
        orden.clave = dv["__CLAVE__"]
        orden.dv = dv
        return orden

    def run(self):
        orden = None
        while True:
            if orden:
                if orden.clave == TERMINAR:
                    self.ipc.close()
                    break
                orden = self.xcpu.procesa(self, orden)
                if orden:
                    continue
            orden = self.recibe()
            if orden is None:
                time.sleep(0.1)

def run(fdb):
    ferr = open("./bug.sound", "at")
    sys.stderr = ferr

    r = Replay()
    io = IO(r, fdb)
    io.run()

    ferr.close()
