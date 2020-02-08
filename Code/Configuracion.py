# -*- coding: utf-8 -*-

import operator
import os
import shutil

from PyQt4 import QtGui
from PyQt4.QtCore import Qt

from Code import BaseConfig
from Code import CajonDesastre
from Code import MotoresExternos
from Code import TrListas
from Code import Traducir
from Code import Util
from Code import VarGen
from Code.Constantes import *

if VarGen.isLinux32:
    import Code.EnginesLinux32 as Engines
elif VarGen.isLinux64:
    import Code.EnginesLinux64 as Engines
else:
    import Code.EnginesWindows as Engines

NIVELBAK = 1

LCFILEFOLDER = "./lc.folder"
LCBASEFOLDER = "./UsrData"


def activeFolder():
    if os.path.isfile(LCFILEFOLDER):
        f = open(LCFILEFOLDER)
        x = f.read().strip()
        f.close()
        if os.path.isdir(x):
            return x
    return LCBASEFOLDER


def isDefaultFolder():
    return activeFolder() == os.path.abspath(LCBASEFOLDER)


def changeFolder(nueva):
    if nueva:
        if os.path.abspath(nueva) == os.path.abspath(LCBASEFOLDER):
            return changeFolder(None)
        f = open(LCFILEFOLDER, "wb")
        f.write(nueva)
        f.close()
    else:
        Util.borraFichero(LCFILEFOLDER)


class Perfomance:
    def __init__(self):
        self.limit_max = 3500.0
        self.limit_min = 800.0
        self.lost_factor = 15.0
        self.lost_exp = 1.35

        self.very_bad_lostp = 200
        self.bad_lostp = 90
        self.bad_limit_min = 1200.0
        self.very_bad_factor = 8
        self.bad_factor = 2

        self.very_good_depth = 6
        self.good_depth = 3

    def elo(self, xlost):
        # 3500.0 - ((60 * xlost) / (xgmo ** 0.4)) + abs(xeval ** 0.8)
        return min(max(int(self.limit_max - self.lost_factor * (xlost ** self.lost_exp)), self.limit_min), self.limit_max)

    def elo_bad_vbad(self, xlost):
        elo = self.elo(xlost)
        vbad = xlost > self.very_bad_lostp
        bad = False if vbad else xlost > self.bad_lostp
        return elo, bad, vbad

    def limit(self, verybad, bad, nummoves):
        if verybad or bad:
            return int(max(self.limit_max - self.very_bad_factor*1000.0 * verybad / nummoves - self.bad_factor*1000.0 * bad / nummoves, self.bad_limit_min))
        else:
            return self.limit_max

    def save_dic(self):
        dic = {}
        default = Perfomance()
        for x in dir(self):
            if not x.startswith("_"):
                atr = getattr(self, x)
                if not callable(atr):
                    if atr != getattr(default, x):
                        dic[x] = atr
        return dic

    def restore_dic(self, dic):
        for x in dir(self):
            if x in dic:
                setattr(self, x, dic[x])

    def save(self):
        dic = self.save_dic()
        return str(dic)

    def restore(self, txt):
        if txt:
            dic = eval(txt)
            self.restore_dic(dic)


class Configuracion:
    def __init__(self, user):

        self.ponCarpetas(user)

        self.user = user

        self.siMain = user == ""

        self.id = Util.creaID()
        self.jugador = ""
        self.dirSalvados = ""
        self.dirPGN = ""
        self.dirJS = ""
        self.traductor = ""
        self.estilo = "Cleanlooks"
        self.vistaTutor = kTutorH

        self.efectosVisuales = False
        self.rapidezMovPiezas = 100
        self.guardarVariantesTutor = True

        self.siAtajosRaton = False  # predictivo=True
        self.showCandidates = False

        self.siActivarCapturas = False
        self.siActivarInformacion = False

        self.tutorActivoPorDefecto = True

        self.version = ""

        self.elo = 0
        self.eloNC = 1600

        self.michelo = 1600
        self.micheloNC = 1600

        self.fics = 1200
        self.ficsNC = 1200

        self.fide = 1600
        self.fideNC = 1600

        self.lichess = 1600
        self.lichessNC = 1600

        self.siDGT = False

        self.opacityToolBoard = 10
        self.positionToolBoard = "T"

        self.directorIcon = False
        self.directGraphics = True

        self.coloresPGNdefecto()

        self.tablaSelBackground = None

        self.tamFontRotulos = 10
        self.anchoPGN = 283
        self.puntosPGN = 10
        self.altoFilaPGN = 22
        self.figurinesPGN = True

        self.autocoronacion = False

        self.showVariantes = False
        self.tipoMaterial = "D"

        self.familia = ""

        self.puntosMenu = 11
        self.boldMenu = False

        self.puntosTB = 11
        self.boldTB = False
        self.iconsTB = Qt.ToolButtonTextUnderIcon

        self.centipawns = False

        self.cursorThinking = True

        self.salvarGanados = False
        self.salvarPerdidos = False
        self.salvarAbandonados = False
        self.salvarFichero = ""

        self.folderOpenings = self.folderBaseOpenings

        self.salvarCSV = ""

        self.liTrasteros = []

        self.liFavoritos = []

        self.liPersonalidades = []

        self.dicRivales = Engines.leeRivales()

        self.rivalInicial = "rocinante" if VarGen.isLinux else "irina"
        self.rival = self.buscaRival(self.rivalInicial)

        self.tutorInicial = "honey"
        self.tutor = self.buscaRival(self.tutorInicial)
        self.tutorMultiPV = 10  # 0: maximo
        self.tutorDifPts = 0
        self.tutorDifPorc = 0

        self.tiempoTutor = 3000
        self.depthTutor = 0

        self.siSuenaBeep = False
        self.siSuenaNuestro = False
        self.siSuenaJugada = False
        self.siSuenaResultados = False

        self.siNomPiezasEN = False

        self.siAplazada = False

        self.notbackground = False
        self.bmi2 = False

        self.checkforupdate = False

        self.siLogEngines = False

        self.palette = {}

        self.perfomance = Perfomance()

        self.grupos = BaseConfig.Grupos(self)
        self.grupos.nuevo("TarraschToy", 0, 1999, 0)
        self.grupos.nuevo("Bikjump", 2000, 2400, 600)
        self.grupos.nuevo("Greko", 2401, 2599, 1800)
        self.grupos.nuevo("Alaric", 2600, 2799, 3600)
        self.grupos.nuevo("Rybka", 2800, 3400, 6000)

    def start(self, version):
        self.lee()
        if version != self.version:
            CajonDesastre.compruebaCambioCarpetas(self)
            self.version = version
            self.graba()
        self.leeConfTableros()

    def changeActiveFolder(self, nueva):
        changeFolder(nueva)
        self.ponCarpetas(None)  # Siempre sera el principal
        CajonDesastre.compruebaCambioCarpetas(self)
        self.lee()

    def ponCarpetas(self, user):
        self.carpetaBase = activeFolder()

        self.carpetaUsers = "%s/users" % self.carpetaBase

        if user:
            Util.creaCarpeta(self.carpetaUsers)
            self.carpeta = "%s/users/%s" % (self.carpetaBase, user)
            Util.creaCarpeta(self.carpeta)
        else:
            Util.creaCarpeta(self.carpetaBase)
            self.carpeta = self.carpetaBase

        self.fichero = self.carpeta + "/lk70.pik"

        self.siPrimeraVez = not Util.existeFichero(self.fichero)

        # Util.creaCarpeta()
        # self.plantillaVideo = self.carpeta + "/confvid/%s.video"

        self.ficheroVideo = "%s/confvid.pkd" % self.carpeta

        self.ficheroSounds = "%s/sounds.pkd" % self.carpeta
        self.fichEstadElo = "%s/estad.pkli" % self.carpeta
        self.fichEstadMicElo = "%s/estadMic.pkli" % self.carpeta
        self.fichEstadFicsElo = "%s/estadFics.pkli" % self.carpeta
        self.fichEstadFideElo = "%s/estadFide.pkli" % self.carpeta
        self.fichEstadLichessElo = "%s/estadLichess.pkli" % self.carpeta
        self.ficheroBooks = "%s/books.lkv" % self.carpeta
        self.ficheroTrainBooks = "%s/booksTrain.lkv" % self.carpeta
        self.ficheroMate = "%s/mate.ddb" % self.carpeta
        self.ficheroMemoria = "%s/memo.pk" % self.carpeta
        self.ficheroMExternos = "%s/listaMotores.pkt" % self.carpeta
        self.ficheroRemoto = "%s/remoto.pke" % self.carpeta
        self.ficheroCliente = "%s/cliente.pke" % self.carpeta
        self.ficheroRemNueva = "%s/remnueva.pke" % self.carpeta
        self.ficheroEntMaquina = "%s/entmaquina.pke" % self.carpeta
        self.ficheroEntMaquinaConf = "%s/entmaquinaconf.pkd" % self.carpeta
        self.ficheroGM = "%s/gm.pke" % self.carpeta
        self.ficheroGMhisto = "%s/gmh.db" % self.carpeta
        self.ficheroPuntuacion = "%s/punt.pke" % self.carpeta
        self.ficheroDirSound = "%s/direc.pkv" % self.carpeta
        self.ficheroKibitzers = "%s/moscas.pkv" % self.carpeta
        self.ficheroKibitzersN = "%s/kibitzers.pkv" % self.carpeta
        self.ficheroEntAperturas = "%s/entaperturas.pkd" % self.carpeta
        self.ficheroEntAperturasPar = "%s/entaperturaspar.pkd" % self.carpeta
        self.ficheroPersAperturas = "%s/persaperturas.pkd" % self.carpeta
        self.ficheroAnalisis = "%s/paranalisis.pkd" % self.carpeta
        self.ficheroDailyTest = "%s/nivel.pkd" % self.carpeta
        self.ficheroTemas = "%s/themes.pkd" % self.carpeta
        self.dirPersonalTraining = "%s/Personal Training" % self.carpeta
        self.ficheroBMT = "%s/lucas.bmt" % self.carpeta
        self.ficheroPotencia = "%s/power.db" % self.carpeta
        self.ficheroPuente = "%s/bridge.db" % self.carpeta
        self.ficheroMoves = "%s/moves.dbl" % self.carpeta
        self.ficheroRecursos = "%s/recursos.dbl" % self.carpeta
        self.ficheroFEN = self.ficheroRecursos
        self.ficheroConfTableros = "%s/confTableros.pk" % self.carpeta
        self.ficheroBoxing = "%s/boxing.pk" % self.carpeta
        self.ficheroTrainings = "%s/trainings.pk" % self.carpeta
        self.ficheroHorses = "%s/horses.db" % self.carpeta
        self.ficheroBookGuide = "%s/Standard opening guide.pgo" % self.carpeta  # fix
        self.ficheroAnalisisBookGuide = "%s/analisisBookGuide.pkd" % self.carpeta  # fix
        self.ficheroLearnPGN = "%s/LearnPGN.db" % self.carpeta
        self.ficheroPlayPGN = "%s/PlayPGN.db" % self.carpeta
        self.ficheroAlbumes = "%s/albumes.pkd" % self.carpeta
        self.ficheroPuntuaciones = "%s/hpoints.pkd" % self.carpeta
        self.ficheroAnotar = "%s/anotar.db" % self.carpeta

        self.ficheroSelectedPositions = "%s/Selected positions.fns" % self.dirPersonalTraining
        self.ficheroPresentationPositions = "%s/Challenge 101.fns" % self.dirPersonalTraining

        self.ficheroVariables = "%s/Variables.pk" % self.carpeta

        self.ficheroFiltrosPGN = "%s/pgnFilters.db" % self.carpeta

        Util.creaCarpeta(self.dirPersonalTraining)

        self.carpetaGames = "%s/%s" % (self.carpeta, "DatabasesGames")
        Util.creaCarpeta(self.carpetaGames)

        self.carpetaPositions = "%s/%s" % (self.carpeta, "DatabasesPositions")
        Util.creaCarpeta(self.carpetaPositions)

        self.ficheroDBgames = "%s/%s.lcg" % (self.carpetaGames, _("Initial Database Games"))

        self.ficheroDBgamesFEN = "%s/%s.lcf" % (self.carpetaPositions, _("Positions Database"))

        self.carpetaSTS = "%s/sts" % self.carpeta

        self.carpetaScanners = "%s/%s" % (self.carpeta, "scanners")
        Util.creaCarpeta(self.carpetaScanners)

        self.ficheroExpeditions = "%s/Expeditions.db" % self.carpeta
        self.ficheroSingularMoves = "%s/SingularMoves.db" % self.carpeta

        if not Util.existeFichero(self.ficheroRecursos):
            Util.copiaFichero("IntFiles/recursos.dbl", self.ficheroRecursos)

        self.folderBaseOpenings = os.path.join(self.carpeta, "OpeningLines")
        Util.creaCarpeta(self.folderBaseOpenings)

        if os.path.isdir(self.carpeta + "/confvid"):
            self.salto_version()

    def compruebaBMT(self):
        if not Util.existeFichero(self.ficheroBMT):
            self.ficheroBMT = "%s/lucas.bmt" % self.carpeta

    def puntuacion(self):
        return self.grupos.puntuacion()

    def maxNivelCategoria(self, categoria):
        return self.rival.maxNivelCategoria(categoria)

    def limpia(self, nombre):
        self.elo = 0
        self.eloNC = 1600
        self.michelo = 1600
        self.micheloNC = 1600
        self.fics = 1200
        self.ficsNC = 1200
        self.fide = 1600
        self.fideNC = 1600
        self.grupos.limpia()
        self.id = Util.creaID()
        self.jugador = nombre
        self.dirSalvados = ""
        self.dirPGN = ""
        self.dirJS = ""
        self.liTrasteros = []

        self.salvarGanados = False
        self.salvarPerdidos = False
        self.salvarAbandonados = False
        self.salvarFichero = ""

        self.siActivarCapturas = False
        self.siActivarInformacion = False
        self.siAtajosRaton = False
        self.showCandidates = False

        self.salvarCSV = ""

        self.rival = self.buscaRival(self.rivalInicial)

        self.perfomance = Perfomance()

    def buscaRival(self, clave, defecto=None):
        if clave in self.dicRivales:
            return self.dicRivales[clave]
        if defecto is None:
            defecto = self.rivalInicial
        return self.buscaRival(defecto)

    def buscaRivalExt(self, claveMotor):
        if claveMotor.startswith("*"):
            rival = MotoresExternos.buscaRivalExt(claveMotor)
            if rival is None:
                rival = self.buscaRival("critter")
        else:
            rival = self.buscaRival(claveMotor)
        return rival

    def buscaTutor(self, clave, defecto=None):
        if clave in self.dicRivales and self.dicRivales[clave].puedeSerTutor():
            return self.dicRivales[clave]

        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        for motor in listaMotoresExt.liMotores:
            if clave == motor.alias:
                return MotoresExternos.ConfigMotor(motor)

        if defecto is None:
            defecto = self.tutorInicial
        return self.buscaRival(defecto)

    def buscaMotor(self, clave, defecto=None):
        if clave in self.dicRivales:
            return self.dicRivales[clave]

        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        if clave.startswith("*"):
            clave = clave[1:]
        for motor in listaMotoresExt.liMotores:
            if clave == motor.alias:
                return MotoresExternos.ConfigMotor(motor)

        return self.buscaRival(defecto)

    def ayudaCambioTutor(self):
        li = []
        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        for motor in listaMotoresExt.liMotores:
            if motor.multiPV > 10:
                li.append((motor.alias, motor.alias + " *"))
        for clave, cm in self.dicRivales.iteritems():
            if cm.puedeSerTutor():
                li.append((clave, cm.nombre))
        li = sorted(li, key=operator.itemgetter(1))
        li.insert(0, self.tutor.clave)
        return li

    def comboMotores(self):
        li = []
        for clave, cm in self.dicRivales.iteritems():
            li.append((cm.nombre, clave))
        li.sort(key=lambda x:x[0])
        return li

    def comboMotoresCompleto(self):
        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        liMotoresExt = []
        for motor in listaMotoresExt.liMotores:
            liMotoresExt.append((motor.alias + "*", "*" + motor.alias))

        li = self.comboMotores()
        li.extend(liMotoresExt)
        li = sorted(li, key=operator.itemgetter(0))
        return li

    def comboMotoresMultiPV10(self, minimo=10):  # %#
        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        liMotores = []
        for motor in listaMotoresExt.liMotores:
            if motor.multiPV >= minimo:
                liMotores.append((motor.alias + "*", "*" + motor.alias))

        for clave, cm in self.dicRivales.iteritems():
            if cm.multiPV >= minimo:
                liMotores.append((cm.nombre, clave))

        li = sorted(liMotores, key=operator.itemgetter(0))
        return li

    def ayudaCambioCompleto(self, cmotor):
        li = []
        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        for motor in listaMotoresExt.liMotores:
            li.append(("*" + motor.alias, motor.alias + " *"))
        for clave, cm in self.dicRivales.iteritems():
            li.append((clave, cm.nombre))
        li = sorted(li, key=operator.itemgetter(1))
        li.insert(0, cmotor)
        return li

    def estilos(self):

        li = [(x, x) for x in QtGui.QStyleFactory.keys()]
        li.insert(0, self.estilo)
        return li

    def coloresPGNdefecto(self):
        self.color_nag1 = "#0707FF"
        self.color_nag2 = "#FF7F00"
        self.color_nag3 = "#820082"
        self.color_nag4 = "#FF0606"
        self.color_nag5 = "#008500"
        self.color_nag6 = "#ECC70A"

    def graba(self, aplazamiento=None):

        dic = {}
        dic["VERSION"] = self.version
        dic["ID"] = self.id
        dic["JUGADOR"] = self.jugador if self.jugador else _("User")
        dic["ESTILO"] = self.estilo
        dic["TIEMPOTUTOR"] = self.tiempoTutor
        dic["DEPTHTUTOR"] = self.depthTutor

        dic["SIBEEP"] = self.siSuenaBeep
        dic["SISUENANUESTRO"] = self.siSuenaNuestro
        dic["SISUENAJUGADA"] = self.siSuenaJugada
        dic["SISUENARESULTADOS"] = self.siSuenaResultados
        dic["GUARDAR_VARIANTES"] = self.guardarVariantesTutor

        dic["DIRSALVADOS"] = self.dirSalvados
        dic["DIRPGN"] = self.dirPGN
        dic["DIRJS"] = self.dirJS
        dic["TRADUCTOR"] = self.traductor
        dic["SALVAR_FICHERO"] = self.salvarFichero
        dic["SALVAR_GANADOS"] = self.salvarGanados
        dic["SALVAR_PERDIDOS"] = self.salvarPerdidos
        dic["SALVAR_ABANDONADOS"] = self.salvarAbandonados
        dic["SALVAR_CSV"] = self.salvarCSV
        dic["VISTA_TUTOR"] = self.vistaTutor
        dic["EFECTOS_VISUALES"] = self.efectosVisuales
        dic["RAPIDEZMOVPIEZAS"] = self.rapidezMovPiezas
        dic["ATAJOS_RATON"] = self.siAtajosRaton
        dic["SHOW_CANDIDATES"] = self.showCandidates
        dic["ACTIVAR_CAPTURAS"] = self.siActivarCapturas
        dic["ACTIVAR_INFORMACION"] = self.siActivarInformacion
        dic["RIVAL"] = self.rival.clave
        dic["TUTOR"] = self.tutor.clave
        dic["TUTOR_DIFPTS"] = self.tutorDifPts
        dic["TUTOR_DIFPORC"] = self.tutorDifPorc
        dic["TUTORACTIVODEFECTO"] = self.tutorActivoPorDefecto
        dic["TUTOR_MULTIPV"] = self.tutorMultiPV

        dic["SILOGENGINES"] = self.siLogEngines

        dic["SINOMPIEZASEN"] = self.siNomPiezasEN

        dic["DBGAMES"] = Util.dirRelativo(self.ficheroDBgames)
        dic["DBGAMESFEN"] = Util.dirRelativo(self.ficheroDBgamesFEN)

        dic["BOOKGUIDE"] = self.ficheroBookGuide

        dic["SIDGT"] = self.siDGT

        dic["OPACITYTOOLBOARD"] = self.opacityToolBoard
        dic["POSITIONTOOLBOARD"] = self.positionToolBoard

        dic["DIRECTORICON"] = self.directorIcon
        dic["DIRECTGRAPHICS"] = self.directGraphics

        dic["FICHEROBMT"] = self.ficheroBMT
        dic["FICHEROFEN"] = self.ficheroFEN

        dic["FAMILIA"] = self.familia

        dic["PUNTOSMENU"] = self.puntosMenu
        dic["BOLDMENU"] = self.boldMenu

        dic["PUNTOSTB"] = self.puntosTB
        dic["BOLDTB"] = self.boldTB
        dic["ICONSTB"] = self.iconsTB

        dic["COLOR_NAG1"] = self.color_nag1
        dic["COLOR_NAG2"] = self.color_nag2
        dic["COLOR_NAG3"] = self.color_nag3
        dic["COLOR_NAG4"] = self.color_nag4
        dic["COLOR_NAG5"] = self.color_nag5
        dic["COLOR_NAG6"] = self.color_nag6

        dic["TABLASELBACKGROUND"] = self.tablaSelBackground

        dic["TAMFONTROTULOS"] = self.tamFontRotulos
        dic["ANCHOPGN"] = self.anchoPGN
        dic["PUNTOSPGN"] = self.puntosPGN
        dic["ALTOFILAPGN"] = self.altoFilaPGN
        dic["FIGURINESPGN"] = self.figurinesPGN
        dic["AUTOCORONACION"] = self.autocoronacion

        dic["SHOW_VARIANTES"] = self.showVariantes
        dic["TIPOMATERIAL"] = self.tipoMaterial

        dic["ELO"] = self.elo
        dic["ELONC"] = self.eloNC
        dic["MICHELO"] = self.michelo
        dic["MICHELONC"] = self.micheloNC
        dic["FICS"] = self.fics
        dic["FICSNC"] = self.ficsNC
        dic["FIDE"] = self.fide
        dic["FIDENC"] = self.fideNC
        dic["LICHESS"] = self.lichess
        dic["LICHESSNC"] = self.lichessNC
        dic["TRASTEROS"] = self.liTrasteros
        dic["FAVORITOS"] = self.liFavoritos
        dic["PERSONALIDADES"] = self.liPersonalidades

        dic["CENTIPAWNS"] = self.centipawns

        dic["CURSORTHINKING"] = self.cursorThinking

        dic["NOTBACKGROUND"] = self.notbackground
        dic["BMI2"] = self.bmi2

        dic["CHECKFORUPDATE"] = self.checkforupdate
        dic["PALETTE"] = self.palette

        dic["PERFOMANCE"] = self.perfomance.save()

        dic["FOLDEROPENINGS"] = self.folderOpenings

        for clave, rival in self.dicRivales.iteritems():
            dic["RIVAL_%s" % clave] = rival.graba()
        if aplazamiento:
            dic["APLAZAMIENTO"] = Util.dic2txt(aplazamiento)
        Util.guardaDIC(dic, self.fichero)

        self.releeTRA()

    def lee(self):
        self.siAplazada = False

        if not os.path.isfile(self.fichero):
            CajonDesastre.compruebaCambioVersion(self)

        else:
            fbak = self.fichero + ".CP.%d" % NIVELBAK
            if not Util.existeFichero(fbak):
                Util.copiaFichero(self.fichero, fbak)
            dic = Util.recuperaDIC(self.fichero)
            if dic:
                dg = dic.get
                self.id = dic["ID"]
                self.version = dic.get("VERSION", "")
                self.jugador = dic["JUGADOR"] if dic["JUGADOR"] else _("User")
                self.estilo = dg("ESTILO", "Cleanlooks")
                self.tiempoTutor = dic["TIEMPOTUTOR"]
                self.depthTutor = dg("DEPTHTUTOR", 0)
                if self.tiempoTutor == 0 and self.depthTutor == 0:
                    self.tiempoTutor = 3000

                self.siSuenaBeep = dic["SIBEEP"]
                self.siSuenaJugada = dg("SISUENAJUGADA", False)
                self.siSuenaResultados = dg("SISUENARESULTADOS", False)
                self.siSuenaNuestro = dg("SISUENANUESTRO", False)

                self.efectosVisuales = dg("EFECTOS_VISUALES", True)
                self.rapidezMovPiezas = dg("RAPIDEZMOVPIEZAS", self.rapidezMovPiezas)
                self.siAtajosRaton = dg("ATAJOS_RATON", False)
                self.showCandidates = dg("SHOW_CANDIDATES", False)
                self.siActivarCapturas = dg("ACTIVAR_CAPTURAS", self.siActivarCapturas)
                self.siActivarInformacion = dg("ACTIVAR_INFORMACION", self.siActivarInformacion)
                self.guardarVariantesTutor = dg("GUARDAR_VARIANTES", True)

                self.dirSalvados = dic["DIRSALVADOS"]
                self.dirPGN = dg("DIRPGN", "")
                self.dirJS = dg("DIRJS", "")
                self.traductor = dic["TRADUCTOR"].lower()
                self.salvarFichero = dic["SALVAR_FICHERO"]
                self.salvarGanados = dic["SALVAR_GANADOS"]
                self.salvarPerdidos = dic["SALVAR_PERDIDOS"]
                self.salvarAbandonados = dg("SALVAR_ABANDONADOS", False)
                self.salvarCSV = dg("SALVAR_CSV", "")
                self.vistaTutor = dg("VISTA_TUTOR", kTutorH)
                self.rival = self.buscaRival(dic["RIVAL"], self.rivalInicial)
                self.tutor = self.buscaTutor(dic["TUTOR"], self.tutorInicial)

                self.siNomPiezasEN = dg("SINOMPIEZASEN", self.siNomPiezasEN)

                self.tutorDifPts = dg("TUTOR_DIFPTS", 0)
                self.tutorDifPorc = dg("TUTOR_DIFPORC", 0)
                self.tutorActivoPorDefecto = dg("TUTORACTIVODEFECTO", True)
                self.tutorMultiPV = dg("TUTOR_MULTIPV", "MX")

                self.siLogEngines = dg("SILOGENGINES", False)

                fich = dg("DBGAMES", self.ficheroDBgames)
                if os.path.isfile(fich):
                    self.ficheroDBgames = fich
                fich = dg("DBGAMESFEN", self.ficheroDBgamesFEN)
                if os.path.isfile(fich):
                    self.ficheroDBgamesFEN = fich
                fich = dg("BOOKGUIDE", self.ficheroBookGuide)
                if os.path.isfile(fich):
                    self.ficheroBookGuide = fich

                self.elo = dg("ELO", 0)
                self.eloNC = dg("ELONC", 1600)
                self.michelo = dg("MICHELO", self.michelo)
                self.micheloNC = dg("MICHELONC", self.micheloNC)
                self.fics = dg("FICS", self.fics)
                self.ficsNC = dg("FICSNC", self.ficsNC)
                self.fide = dg("FIDE", self.fide)
                self.fideNC = dg("FIDENC", self.fideNC)
                self.lichess = dg("LICHESS", self.fide)
                self.lichessNC = dg("LICHESSNC", self.fideNC)

                self.siDGT = dg("SIDGT", False)

                self.opacityToolBoard = dg("OPACITYTOOLBOARD", self.opacityToolBoard)
                self.positionToolBoard = dg("POSITIONTOOLBOARD", self.positionToolBoard)

                self.directorIcon = dg("DIRECTORICON", self.directorIcon)
                self.directGraphics = dg("DIRECTGRAPHICS", self.directGraphics)

                self.familia = dg("FAMILIA", self.familia)

                self.puntosMenu = dg("PUNTOSMENU", self.puntosMenu)
                self.boldMenu = dg("BOLDMENU", self.boldMenu)

                self.puntosTB = dg("PUNTOSTB", self.puntosTB)
                self.boldTB = dg("BOLDTB", self.boldTB)
                self.iconsTB = dg("ICONSTB", self.iconsTB)

                self.color_nag1 = dg("COLOR_NAG1", self.color_nag1)
                self.color_nag2 = dg("COLOR_NAG2", self.color_nag2)
                self.color_nag3 = dg("COLOR_NAG3", self.color_nag3)
                self.color_nag4 = dg("COLOR_NAG4", self.color_nag4)
                self.color_nag5 = dg("COLOR_NAG5", self.color_nag5)
                self.color_nag6 = dg("COLOR_NAG6", self.color_nag6)

                self.tablaSelBackground = dg("TABLASELBACKGROUND", None)

                self.tamFontRotulos = dg("TAMFONTROTULOS", self.tamFontRotulos)
                self.anchoPGN = dg("ANCHOPGN", self.anchoPGN)
                self.puntosPGN = dg("PUNTOSPGN", self.puntosPGN)
                self.altoFilaPGN = dg("ALTOFILAPGN", self.altoFilaPGN)
                self.figurinesPGN = dg("FIGURINESPGN", False)

                self.autocoronacion = dg("AUTOCORONACION", self.autocoronacion)
                self.showVariantes = dg("SHOW_VARIANTES", False)
                self.tipoMaterial = dg("TIPOMATERIAL", self.tipoMaterial)

                self.ficheroBMT = dg("FICHEROBMT", self.ficheroBMT)
                self.ficheroFEN = dg("FICHEROFEN", self.ficheroFEN)

                self.liTrasteros = dg("TRASTEROS", [])
                self.liFavoritos = dg("FAVORITOS", [])
                self.testFavoritos()
                self.liPersonalidades = dg("PERSONALIDADES", [])

                self.centipawns = dg("CENTIPAWNS", self.centipawns)

                self.cursorThinking = dg("CURSORTHINKING", self.cursorThinking)

                self.notbackground = dg("NOTBACKGROUND", self.notbackground)
                self.bmi2 = dg("BMI2", self.bmi2)
                if self.bmi2 and VarGen.isWindows and not Util.is64Windows():
                    self.bmi2 = False

                self.checkforupdate = dg("CHECKFORUPDATE", self.checkforupdate)
                self.palette = dg("PALETTE", self.palette)

                perf = dg("PERFOMANCE")
                if perf:
                    self.perfomance.restore(perf)

                self.folderOpenings = dg("FOLDEROPENINGS", self.folderBaseOpenings)
                if not os.path.isdir(self.folderOpenings):
                    self.folderOpenings = self.folderBaseOpenings

                for k in dic.keys():
                    if k.startswith("RIVAL_"):
                        claveK = k[6:]
                        for clave, rival in self.dicRivales.iteritems():
                            if rival.clave == claveK:
                                rival.lee(dic[k])
                if "APLAZAMIENTO" in dic:
                    self.siAplazada = True
                    try:
                        self.aplazamiento = Util.txt2dic(dic["APLAZAMIENTO"])
                    except:
                        self.aplazamiento = None
                        self.siAplazada = False
                    self.graba()

        self.dicTrad = {'english': "en", 'español': "es", 'francais': "fr",
                        'deutsch': "de", 'portuguese': "pt", 'russian': "ru",
                        "italiano": "it", "azeri": "az", "català": "ca",
                        "vietnamese": "vi", "swedish": "sv"}

        # Si viene de la instalacion
        for k, v in self.dicTrad.iteritems():
            if os.path.isfile(v + '.pon'):
                self.traductor = v
                self.graba()
                os.remove(v + '.pon')
        # Versiones antiguas
        if self.traductor in self.dicTrad:
            self.traductor = self.dicTrad[self.traductor]

        self.releeTRA()

        TrListas.ponPiecesLNG(self.siNomPiezasEN or self.traductor == "en")

    def dataDB(self):
        return os.path.join(self.carpeta, "_dataDB")

    def testFavoritos(self):
        if len(self.liFavoritos) > 0:
            if type(self.liFavoritos[0]) != type({}):
                self.liFavoritos = [{"OPCION": valor} for valor in self.liFavoritos]

    def releeTRA(self):
        Traducir.install(self.traductor)

    def eloActivo(self, siModoCompetitivo):
        return self.elo if siModoCompetitivo else self.eloNC

    def miceloActivo(self, siModoCompetitivo):
        return self.michelo if siModoCompetitivo else self.micheloNC

    def ficsActivo(self, siModoCompetitivo):
        return self.fics if siModoCompetitivo else self.ficsNC

    def fideActivo(self, siModoCompetitivo):
        return self.fide if siModoCompetitivo else self.fideNC

    def lichessActivo(self, siModoCompetitivo):
        return self.lichess if siModoCompetitivo else self.lichessNC

    def ponEloActivo(self, elo, siModoCompetitivo):
        if siModoCompetitivo:
            self.elo = elo
        else:
            self.eloNC = elo

    def ponMiceloActivo(self, elo, siModoCompetitivo):
        if siModoCompetitivo:
            self.michelo = elo
        else:
            self.micheloNC = elo

    def ponFicsActivo(self, elo, siModoCompetitivo):
        if siModoCompetitivo:
            self.fics = elo
        else:
            self.ficsNC = elo

    def ponFideActivo(self, elo, siModoCompetitivo):
        if siModoCompetitivo:
            self.fide = elo
        else:
            self.fideNC = elo

    def ponLichessActivo(self, elo, siModoCompetitivo):
        if siModoCompetitivo:
            self.lichess = elo
        else:
            self.lichessNC = elo

    def listaTraducciones(self):
        li = []
        dlang = "Locale"
        for uno in Util.listdir(dlang):
            fini = os.path.join(dlang, uno.name, "lang.ini")
            if os.path.isfile(fini):
                dic = Util.iniBase8dic(fini)
                li.append((uno.name, dic["NAME"], dic["%"], dic["AUTHOR"]))
        li = sorted(li, key=lambda lng: lng[0])
        return li

    def listaMotoresInternos(self):
        li = [v for k, v in self.dicRivales.iteritems()]
        li = sorted(li, key=lambda cm: cm.nombre)
        return li

    def listaMotoresExternos(self, ordenados=True):
        listaMotoresExt = MotoresExternos.ListaMotoresExternos(self.ficheroMExternos)
        listaMotoresExt.leer()
        li = listaMotoresExt.liMotores
        if ordenados:
            li.sort(key=lambda cm: cm.alias)
        return li

    def listaMotores(self):
        li = []
        for k, v in self.dicRivales.iteritems():
            li.append((v.nombre, v.autor, v.url))
        li = sorted(li, key=operator.itemgetter(0))
        return li

    def listaMotoresCompleta(self):
        li = self.listaMotores()
        li.append(("Greko 7.1", "Vladimir Medvedev", "http://greko.110mb.com/index.html"))
        li = sorted(li, key=operator.itemgetter(0))
        return li

    def carpetaTemporal(self):
        dirTmp = os.path.join(self.carpeta, "tmp")
        Util.creaCarpeta(dirTmp)
        return dirTmp

    def ficheroTemporal(self, extension):
        dirTmp = os.path.join(self.carpeta, "tmp")
        return Util.ficheroTemporal(dirTmp, extension)

    def limpiaTemporal(self):
        try:
            dirTmp = os.path.join(self.carpeta, "tmp")
            for entry in Util.listdir(dirTmp):
                Util.borraFichero(entry.path)
        except:
            pass

    def leeVariables(self, nomVar):
        db = Util.DicSQL(self.ficheroVariables)
        resp = db[nomVar]
        db.close()
        return resp if resp else {}

        # "DicMicElos": _("Tourney-Elo")
        # "ENG_GESTORSOLO": _("Create your own game")
        # "FICH_GESTORSOLO": _("Create your own game")
        # "ENG_VARIANTES": _("Variants") _("Edition")
        # "TRANSSIBERIAN": _("Transsiberian Railway")
        # "STSFORMULA": _("Formula to calculate elo") -  _("STS: Strategic Test Suite")
        # "PantallaColores": _("Colors")
        # "PCOLORES": _("Colors")
        # "manual_save": _("Save positions to FNS/PGN")
        # "FOLDER_ENGINES": _("External engines")
        # "MICELO":
        # "MICPER":
        # "SAVEPGN":
        # "STSRUN":
        # "crear_torneo":
        # "PARAMPELICULA":
        # "BLINDFOLD":
        # "WBG_MOVES":
        # "DBSUMMARY":

    def escVariables(self, nomVar, dicValores):
        db = Util.DicSQL(self.ficheroVariables)
        db[nomVar] = dicValores
        db.close()

    def leeConfTableros(self):
        db = Util.DicSQL(self.ficheroConfTableros)
        self.dicConfTableros = db.asDictionary()
        if "BASE" not in self.dicConfTableros:
            with open("IntFiles/base.board") as f:
                db["BASE"] = self.dicConfTableros["BASE"] = f.read()
        # with open("IntFiles/base.board", "wb") as f:
        #     f.write(db["BASE"])
        db.close()

    def resetConfTablero(self, key, tamDef):
        db = Util.DicSQL(self.ficheroConfTableros)
        del db[key]
        db.close()
        self.leeConfTableros()
        return self.confTablero(key, tamDef)

    def cambiaConfTablero(self, confTablero):
        xid = confTablero._id
        if xid:
            db = Util.DicSQL(self.ficheroConfTableros)
            self.dicConfTableros[xid] = db[xid] = confTablero.graba()
            db.close()
            self.leeConfTableros()

    def confTablero(self, xid, tamDef, padre="BASE"):
        if xid == "BASE":
            ct = BaseConfig.ConfigTablero(xid, tamDef)
        else:
            ct = BaseConfig.ConfigTablero(xid, tamDef, padre=padre)
            ct.anchoPieza(tamDef)

        if xid in self.dicConfTableros:
            ct.lee(self.dicConfTableros[xid])
        else:
            db = Util.DicSQL(self.ficheroConfTableros)
            self.dicConfTableros[xid] = db[xid] = ct.graba()
            db.close()

        ct._anchoPiezaDef = tamDef

        return ct

    def dicMotoresFixedElo(self):
        return Engines.dicMotoresFixedElo()

    def save_dbFEN(self, fenM2, data):
        dbFEN = self.fich_dbFEN()
        dbFEN[fenM2] = data

    def save_video(self, key, dic):
        db = Util.DicSQL(self.ficheroVideo)
        db[key] = dic
        db.close()

    def restore_video(self, key):
        db = Util.DicSQL(self.ficheroVideo)
        dic = db[key]
        db.close()
        return dic

    def salto_version(self):
        def cambio_confvid():
            try:
                folder = self.carpeta + "/confvid"
                if os.path.isdir(folder):
                    db = Util.DicSQL(self.ficheroVideo)
                    li = os.listdir(folder)
                    for fichero in li:
                        if fichero.endswith(".video") and not fichero.startswith("MOS"):
                            key = fichero[:-6]
                            dic = Util.recuperaDIC(os.path.join(folder, fichero))
                            if dic:
                                db[key] = dic
                    db.pack()
                    db.close()
            except:
                pass
            shutil.rmtree(folder)

        def cambio_databases():
            li = os.listdir(self.carpeta)
            for fichero in li:
                if fichero.endswith(".lcg") or fichero.endswith(".lcg_s1"):
                    origen = os.path.join(self.carpeta, fichero)
                    shutil.move(origen, self.carpetaGames)
                elif fichero.endswith(".lcf"):
                    origen = os.path.join(self.carpeta, fichero)
                    shutil.move(origen, self.carpetaPositions)

        cambio_confvid()
        cambio_databases()

