# -*- coding: latin-1 -*-
import os
import collections
import wave
import pyaudio
import audioop
import subprocess
import shutil
import codecs

from pocketsphinx import Decoder

import Code.VarGen as VarGen
import Code.Util as Util

from PyQt4 import QtCore

class Listen(QtCore.QThread):
    def __init__(self, lang, siSave):
        QtCore.QThread.__init__(self)

        self._terminate = False
        self._stop = False
        self.config = readConfig(lang)
        self.save = siSave
        if siSave:
            self.maxSave = 16000*10*60/1024 # Máximo 10 minutos
            self.numSave = 0
            self.liSave = []

    def saveFile(self, fichWAV):
        self.save = False
        resp = "".join(self.liSave)
        tx = audioop.lin2alaw(resp, 2)
        frames = audioop.alaw2lin(tx, 2)
        wf = wave.open(fichWAV, "wb")
        wf.setnchannels(1)
        wf.setsampwidth(self.samplewith)
        wf.setframerate(16000)
        wf.writeframes(frames)
        wf.close()

    def desinstalar(self):
        self._terminate = True
        self._stop = True

    def run( self ):
        conf = Decoder.default_config()
        conf.set_string('-hmm', self.config.hmmPS)
        conf.set_string('-lm', self.config.lmPS)
        conf.set_string('-dict', self.config.dictPS)
        if os.path.isfile(self.config.mllrPS):
            conf.set_string('-mllr', self.config.mllrPS)
        decoder = Decoder(conf)

        p = pyaudio.PyAudio()
        stream = p.open( format=pyaudio.paInt16,
                         channels=1,
                         rate=16000,
                         input=True,
                         frames_per_buffer=1024 )
        stream.start_stream()
        self.samplewith = p.get_sample_size(pyaudio.paInt16)

        in_speech_bf = True
        decoder.start_utt('')
        while not self._terminate:
            buf = stream.read(1024)
            if buf:
                if self.save:
                    self.liSave.append(buf)
                    self.numSave += 1
                    if self.numSave > self.maxSave: # nos protegemos de dejar el microfono encendido
                        self.activeSave(self.fichWAV)
                decoder.process_raw(buf, False, False)
                if decoder.get_in_speech() != in_speech_bf:
                    in_speech_bf = decoder.get_in_speech()
                    if not in_speech_bf:
                        decoder.end_utt()
                        try:
                            if decoder.hyp().hypstr != '':
                                self.decode(decoder.hyp().hypstr)
                        except AttributeError:
                            pass
                        decoder.start_utt('')
            else:
                break
        decoder.end_utt()

    def decode(self, result):
        if result:
            if not self.save:
                lir = [ self.config.dic[x] for x in result.split(" ") if x in self.config.dic ]
                if "STOP" in lir:
                    self._stop = True
                if "GO" in lir:
                    self._stop = False
                    lir = [x for x in lir if x != "GO"]
                if "TERMINATE" in lir:
                    self._terminate = True
                    self._stop = True
                if self._stop:
                    return
                else:
                    if lir:
                        result = " ".join(lir)
                    else:
                        return

            self.emit( QtCore.SIGNAL('voice(QString)'), result )

class RunVoice:
    def __init__(self):
        self.liRutinasVoice = []
        self.voiceListen = None

    def setConf(self, pantalla, siUnico):
        self.pantalla = pantalla
        self.siUnico = siUnico
        self.siSave = siUnico
        self.voice = VarGen.configuracion.voice
        self.siVoice = len(self.voice)>0

    def start(self, rutina):
        if not self.siVoice:
            return
        if self.siUnico and self.voiceListen:
            self.liRutinasVoice = []
            self.close()
        self.liRutinasVoice.append(rutina)
        if not self.voiceListen:
            self.voiceListen = Listen(self.voice, self.siSave)
            self.voiceListen.start()
            self.pantalla.connect(self.voiceListen, QtCore.SIGNAL("voice(QString)"), self.pantalla.voice)

    def stop(self, fichWAV=None):
        if not self.siVoice:
            return
        if self.liRutinasVoice:
            self.liRutinasVoice = self.liRutinasVoice[:-1]
        if self.voiceListen and fichWAV:
            self.voiceListen.saveFile(fichWAV)
        if not self.liRutinasVoice or self.siUnico:
            self.close()

    def close(self):
        if self.voiceListen:
            self.voiceListen.desinstalar()
            self.voiceListen.wait()
            self.voiceListen = None
            self.liRutinasVoice = []

    def voiceDispatch(self, txt):
        if self.liRutinasVoice:
            self.liRutinasVoice[-1](txt)
        else:
            self.stop()

    def isActive(self):
        return len(self.liRutinasVoice)>0

runVoice = RunVoice()

class ConfigVoice:
    pass

def readConfig( lang ):
    baseUser = VarGen.configuracion.folderVoice
    baseGen = os.path.abspath(os.path.join( "Voice", lang))
    ini = os.path.join(baseGen, "config.ini")
    config = ConfigVoice()
    config.dic = collections.OrderedDict()
    with open(ini) as f:
        for linea in f:
            linea = linea.strip()
            if linea.startswith("#") or not linea:
                continue
            n = linea.find("=")
            if n > 0:
                key = linea[:n].strip()
                value = linea[n+1:].strip()
                if key == "VOICE":
                    config.voice = value
                    continue
                while "  " in value:
                    value = value.replace( "  ", " " )
                for v in value.split(" "):
                    config.dic[v] = key
    config.mllrPS = os.path.join(baseUser,"mllr_matrix")
    if os.path.isfile(os.path.join(baseUser, "hmm", "feat.params")):
        config.hmmPS = os.path.join(baseUser, "hmm" )
    else:
        config.hmmPS = os.path.join(baseGen, "hmm" )
    config.dictPS = os.path.join(baseGen, "lm", "lc.dic" )
    config.lmPS = os.path.join(baseGen, "lm", "lc.lm" )

    return config

def readConfigExt( lang ):
    config = readConfig( lang )
    config.dicPhonemes = {}
    with open(config.dictPS) as f:
        for linea in f:
            linea = linea.strip()
            if linea.count("\t") == 1:
                word, phonemes = linea.split("\t")
                if "(" in word:
                    word = word.split("(")[0].strip()
                    config.dicPhonemes[word] += " - %s"%phonemes
                else:
                    config.dicPhonemes[word] = phonemes
    return config

class Recompile:
    def __init__(self):
        configuracion = VarGen.configuracion
        lang = configuracion.voice

        self.baseUser = os.path.abspath(configuracion.folderVoice)
        self.baseGen = os.path.abspath(os.path.join( "Voice", lang))
        self.baseWavs = os.path.abspath(configuracion.folderVoiceWavs)

        self.hmmUser = os.path.join( self.baseUser, "hmm" )
        self.hmmGen = os.path.join( self.baseGen, "hmm" )

        Util.creaCarpeta(self.hmmUser)
        for fich in os.listdir(self.hmmGen):
            shutil.copy( os.path.join(self.hmmGen,fich), self.hmmUser )

        exes = os.path.join("Voice", "_bin")
        for fich in os.listdir(exes):
            shutil.copy( os.path.join(exes,fich), self.baseWavs )

        self.createIDS()

    def createIDS(self):
        self.lc_fileids = os.path.join(self.baseWavs,"lc.fileids")
        self.lc_transcription = os.path.join(self.baseWavs,"lc.transcription")
        fileids = open(self.lc_fileids, "w")
        transcription = codecs.open(self.lc_transcription, "w", "utf-8")
        self.stWavIni = set()
        for x in os.listdir(self.baseWavs):
            if x.endswith( "ini" ):
                fbase = os.path.join( self.baseWavs, x[:-3])
                if os.path.isfile( fbase + "wav" ):
                    with open(fbase+"ini") as f:
                        target = None
                        for linea in f:
                            if linea.startswith( "TARGET"):
                                target = linea.split("=")[1].strip()
                    if target:
                        self.stWavIni.add((fbase+"ini").lower())
                        self.stWavIni.add((fbase+"wav").lower())
                        fileids.write( "%s\n"%x[:-4] )
                        transcription.write( "<s> %s </s> (%s)\n"%(target, x[:-4]))
        fileids.close()
        transcription.close()

    def sphinx_fe(self):
        li = [  os.path.join(self.baseWavs,"sphinx_fe.exe"),
                "-argfile",  os.path.join(self.hmmGen, "feat.params"),
                "-samprate", "16000",
                "-c", self.lc_fileids,
                "-di", self.baseWavs,
                "-do", self.baseWavs,
                "-ei", "wav",
                "-eo", "mfc",
                "-mswav", "yes"
            ]
        subprocess.call(li,cwd=self.baseWavs, shell=True)
        # process = subprocess.Popen(li, cwd=self.baseWavs, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        # process.wait()

    def bw(self):
        li = [  os.path.join(self.baseWavs,"bw.exe"),
                "-hmmdir", self.hmmGen,
                "-moddeffn", os.path.join(self.hmmGen, "mdef"),
                "-ts2cbfn", ".semi.",
                "-feat", "1s_c_d_dd",
                "-svspec", "0-12/13-25/26-38",
                "-cmn", "current",
                "-agc", "none",
                "-dictfn", os.path.join(self.baseGen, "lm", "lc.dic"),
                "-ctlfn", self.lc_fileids,
                "-lsnfn", self.lc_transcription,
                "-accumdir", self.baseWavs
            ]
        subprocess.call(li,cwd=self.baseWavs, shell=True)
        # process = subprocess.Popen(li, cwd=self.baseWavs, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        # process.wait()

    def mllr_solve(self):
        li = [  os.path.join(self.baseWavs,"mllr_solve.exe"),
                "-meanfn", os.path.join(self.hmmGen, "means"),
                "-varfn", os.path.join(self.hmmGen, "variances"),
                "-outmllrfn", os.path.join(self.baseUser,"mllr_matrix"),
                "-accumdir", self.baseWavs
            ]
        subprocess.call(li,cwd=self.baseWavs, shell=True)
        # process = subprocess.Popen(li, cwd=self.baseWavs, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        # process.wait()

    def map_adapt(self):
        li = [  os.path.join(self.baseWavs,"map_adapt.exe"),
                "-meanfn", os.path.join(self.hmmGen, "means"),
                "-varfn", os.path.join(self.hmmGen, "variances"),
                "-mixwfn", os.path.join(self.hmmGen, "mixture_weights"),
                "-tmatfn", os.path.join(self.hmmGen, "transition_matrices"),
                "-accumdir", self.baseWavs,
                "-mapmeanfn", os.path.join(self.hmmUser, "means"),
                "-mapvarfn", os.path.join(self.hmmUser, "variances"),
                "-mapmixwfn", os.path.join(self.hmmUser, "mixture_weights"),
                "-maptmatfn", os.path.join(self.hmmUser, "transition_matrices"),
            ]
        subprocess.call(li,cwd=self.baseWavs, shell=True)
        # process = subprocess.Popen(li, cwd=self.baseWavs, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        # process.wait()

    def limpia(self):
        for x in os.listdir(self.baseWavs):
            ext = x[-3:].lower()
            if not ext in ( "wav", "ini" ):
                fx = os.path.join( self.baseWavs, x)
                os.remove(fx)

def readPGN(liVoice, posicion):
    posicion.setSAK()

    moves = posicion.sak.getExMoves()
    pieza = "p"
    dHasta = collections.defaultdict(list)
    dDesde = collections.defaultdict(list)
    for move in moves:
        dHasta[move.pieza().lower()+move.hasta()].append(move)
        dDesde[move.desde()].append(move)
    col = ""
    desde = ""
    hasta = ""
    promotion = ""
    espieza = lambda c: c in "QRNBKP"
    espromo = lambda c: c in "QRNB"
    esletra = lambda c: c in "abcdefgh"
    esnumero = lambda c: c in "12345678"
    goodMove = None
    for nVoice, c in enumerate(liVoice):
        if c.startswith("O"):
            desde = "e1" if posicion.siBlancas else "e8"
            if c == "O-O":
                hasta ="g1" if posicion.siBlancas else "g8"
            else:
                hasta ="c1" if posicion.siBlancas else "c8"
            goodMove = None
            for move in dHasta["k"+hasta]:
                if move.desde() == desde:
                    goodMove = move
                    break
            if goodMove:
                break
            else:
                return None #error

        if espieza(c):
            if hasta and espromo(c) and hasta.endswith("8") and pieza == "p":
                promotion = c.lower()
            else:
                pieza = c.lower()
                if hasta:
                    return None #error
                col=""
                desde=""
                hasta = ""
                promotion = ""
        elif esletra(c):
            if col:
                desde = col
            col = c
        elif esnumero(c):
            if col and esletra(col):
                pos = col+c
                if hasta:
                    desde = hasta
                    hasta = pos
                else:
                    hasta = pos
        else:
            continue

        if hasta:
            if pieza+hasta not in dHasta:
                desde = hasta
                hasta = None
                continue
            for move in dHasta[pieza+hasta]:
                coronacion = move.coronacion()
                if (promotion or coronacion) and promotion != coronacion.lower():
                    continue
                ndesde = len(desde)
                if ndesde == 0:
                    if len(dHasta[pieza+hasta]) == 1:
                        goodMove = move
                        break
                elif ndesde == 1:
                    if desde not in move.desde():
                        continue
                    nc = 0
                    for desdeB in dDesde:
                        if desde in desdeB:
                            for moveT in dDesde[desdeB]:
                                if pieza == moveT.pieza().lower() and hasta == moveT.hasta():
                                    nc += 1
                    if nc == 1:
                        goodMove = move
                        break
                    else:
                        desde = hasta
                        hasta = None
                        continue
                elif ndesde == 2:
                    if desde == move.desde():
                        goodMove = move
                        break

            if goodMove:
                break

    if goodMove:
        return True, goodMove, nVoice+1
    return False, None, None
