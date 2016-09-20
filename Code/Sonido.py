import StringIO
import audioop
import wave

import pyaudio

from Code.QT import QTUtil
from Code import Util
from Code import VarGen
from Code import XRun

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

class Orden:
    def __init__(self):
        self.clave = ""
        self.dv = {}

    def ponVar(self, nombre, valor):
        self.dv[nombre] = valor

    def bloqueEnvio(self):
        self.dv["__CLAVE__"] = self.clave
        return self.dv

class Replay:
    DATABASE = "D"
    PLAY_ESPERA = "P"
    PLAY_SINESPERA = "N"
    STOP = "S"
    TERMINAR = "T"

    def __init__(self):

        fdb = VarGen.configuracion.ficheroTemporal("db")

        self.ipc = Util.IPC(fdb, True)

        orden = Orden()
        orden.clave = self.DATABASE
        orden.ponVar("FICHERO", VarGen.configuracion.ficheroSounds)
        orden.ponVar("TABLA", "general")

        self.escribe(orden)
        self.siSonando = False

        self.popen = XRun.run_lucas("-sound", fdb)

    def escribe(self, orden):
        self.ipc.push(orden.bloqueEnvio())

    def terminar(self):
        try:
            orden = Orden()
            orden.clave = self.TERMINAR
            self.escribe(orden)
            self.ipc.close()
            self.close()
        except:
            pass

    def close(self):
        if self.popen:
            try:
                self.popen.terminate()
                self.popen = None
            except:
                pass

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    def playClave(self, clave, siEspera):
        orden = Orden()
        orden.clave = self.PLAY_ESPERA if siEspera else self.PLAY_SINESPERA
        orden.ponVar("CLAVE", clave)
        self.siSonando = True

        self.escribe(orden)

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    def playLista(self, lista, siEspera):
        orden = Orden()
        orden.clave = self.PLAY_ESPERA if siEspera else self.PLAY_SINESPERA
        orden.ponVar("LISTACLAVES", lista)
        self.siSonando = True

        self.escribe(orden)

    # -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
    def stop(self):
        if self.siSonando:
            orden = Orden()
            orden.clave = self.STOP
            self.siSonando = False

            self.escribe(orden)
