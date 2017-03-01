import sys
import time
import StringIO
import audioop
import wave

import pyaudio

from PyQt4 import QtCore

from Code.QT import QTUtil
from Code import Util
from Code import VarGen

DATABASE = "D"
PLAY_ESPERA = "P"
PLAY_SINESPERA = "N"
STOP = "S"
TERMINAR = "T"


class RunSound:
    def __init__(self):
        VarGen.runSound = self
        self.replay = None
        self.replayBeep = None

    def compruebaReplay(self):
        if self.replay is None:
            self.replay = Replay()

    def close(self):
        if self.replay:
            self.replay.terminar()
            self.replay = None

    def terminarSonidoActual(self):
        if self.replay:
            self.replay.terminar()

    def playLista(self, li, siEsperar=False):
        self.compruebaReplay()
        self.replay.playLista(li, siEsperar)

    def playClave(self, clave, siEsperar=False):
        self.compruebaReplay()
        self.replay.playClave(clave, siEsperar)

    def playZeitnot(self):
        self.playClave("ZEITNOT")

    def playBeep(self):
        if self.replayBeep is None:
            db = Util.DicBLOB(VarGen.configuracion.ficheroSounds, "general")
            keys = db.keys()
            self.replayBeep = "MC" in keys

        if self.replayBeep:
            self.playClave("MC", False)
        else:
            QTUtil.beep()


def msc(centesimas):
    t = centesimas
    cent = t % 100
    t /= 100
    mins = t / 60
    t -= mins * 60
    seg = t
    return mins, seg, cent


class TallerSonido:
    def __init__(self, wav):
        self.wav = wav
        self.CHUNK = 1024

        if not wav:
            self.centesimas = 0
        else:
            f = StringIO.StringIO(self.wav)

            wf = wave.open(f, 'rb')
            self.centesimas = int(round(100.0 * wf.getnframes() / wf.getframerate(), 0))
            wf.close()

    def siDatos(self):
        return self.wav is not None

    def limpiar(self):
        self.wav = None
        self.centesimas = 0

    def micInicio(self):
        self.p = pyaudio.PyAudio()
        self.FORMAT = pyaudio.paInt16
        self.CHANNELS = 2
        self.RATE = 22050
        self.stream = self.p.open(format=self.FORMAT,
                                  channels=self.CHANNELS,
                                  rate=self.RATE,
                                  input=True,
                                  frames_per_buffer=self.CHUNK)
        self.datos = []

    def micGraba(self):
        self.datos.append(self.stream.read(self.CHUNK))

    def micFinal(self):
        self.stream.stop_stream()
        self.stream.close()
        self.p.terminate()
        resp = "".join(self.datos)
        tx = audioop.lin2alaw(resp, 2)
        frames = audioop.alaw2lin(tx, 2)
        io = StringIO.StringIO()
        wf = wave.open(io, 'wb')
        wf.setnchannels(self.CHANNELS)
        wf.setsampwidth(self.p.get_sample_size(self.FORMAT))
        wf.setframerate(self.RATE)
        wf.writeframes(frames)
        self.wav = io.getvalue()
        wf.close()

        self.centesimas = len(self.datos) * self.CHUNK * 100 / self.RATE

    def leeWAV(self, fichero):
        try:
            wf = wave.open(fichero, 'rb')
            self.centesimas = round(100.0 * wf.getnframes() / wf.getframerate(), 0)
            wf.close()
            f = open(fichero, "rb")
            self.wav = f.read()
            f.close()
            return True
        except:
            self.wav = None
            self.centesimas = 0
            return False

    def playInicio(self, cent_desde, cent_hasta):

        f = StringIO.StringIO(self.wav)

        wf = self.wf = wave.open(f, 'rb')

        self.kfc = kfc = 1.0 * wf.getframerate() / 100.0  # n. de frames por cada centesima

        minFrame = int(kfc * cent_desde)
        self.maxFrame = int(kfc * cent_hasta)

        if minFrame < self.maxFrame - 100:
            wf.setpos(minFrame)
        else:
            minFrame = self.maxFrame

        self.posFrame = minFrame

        p = self.p = pyaudio.PyAudio()

        # open stream
        self.stream = p.open(format=p.get_format_from_width(wf.getsampwidth()),
                             channels=wf.getnchannels(),
                             rate=wf.getframerate(),
                             output=True)

    def play(self):
        if self.posFrame >= self.maxFrame:
            return False, 0
        self.posFrame += self.CHUNK
        data = self.wf.readframes(self.CHUNK)
        if data:
            self.stream.write(data)
            return True, int(self.posFrame / self.kfc)
        else:
            return False, 0

    def playFinal(self):
        self.stream.stop_stream()
        self.stream.close()
        self.p.terminate()

    def recorta(self, centDesde, centHasta):
        f = StringIO.StringIO(self.wav)

        wf = wave.open(f, 'rb')
        nchannels, sampwidth, framerate, nframes, comptype, compname = wf.getparams()

        kfc = 1.0 * wf.getframerate() / 100.0  # n. de frames por cada centesima
        minFrame = int(kfc * centDesde)
        maxFrame = int(kfc * centHasta)

        wf.setpos(minFrame)
        frames = wf.readframes(maxFrame - minFrame)
        wf.close()

        io = StringIO.StringIO()
        wf = wave.open(io, 'wb')
        wf.setnchannels(nchannels)
        wf.setsampwidth(sampwidth)
        wf.setframerate(framerate)
        wf.writeframes(frames)
        self.wav = io.getvalue()
        wf.close()

        self.centesimas = centHasta - centDesde


class Replay:
    def __init__(self):
        self.io = IO()
        self.io.start()

        orden = Orden()
        orden.clave = DATABASE
        orden.ponVar("FICHERO", VarGen.configuracion.ficheroSounds)
        orden.ponVar("TABLA", "general")

        self.push(orden)
        self.siSonando = False

    def push(self, orden):
        self.io.push(orden.bloqueEnvio())

    def terminar(self):
        try:
            orden = Orden()
            orden.clave = TERMINAR
            self.push(orden)
            self.io.close()
        except:
            pass

    def playClave(self, clave, siEspera):
        orden = Orden()
        orden.clave = PLAY_ESPERA if siEspera else PLAY_SINESPERA
        orden.ponVar("CLAVE", clave)
        self.siSonando = True

        self.push(orden)

    def playLista(self, lista, siEspera):
        orden = Orden()
        orden.clave = PLAY_ESPERA if siEspera else PLAY_SINESPERA
        orden.ponVar("LISTACLAVES", lista)
        self.siSonando = True

        self.push(orden)

    def stop(self):
        if self.siSonando:
            orden = Orden()
            orden.clave = STOP
            self.siSonando = False

            self.push(orden)


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

        elif orden.clave == TERMINAR:
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
            if 'MC' not in liClaves:
                return self.play(io, ('MC',))
            else:
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
