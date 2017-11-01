import os
import random

from Code import MotoresExternos
from Code import Partida
from Code import Util
from Code import VarGen


class Engine(MotoresExternos.MotorExterno):
    def __init__(self):
        MotoresExternos.MotorExterno.__init__(self)

        self._huella = None

        self._depth = 0
        self._time = 0

        self._book = "*"  # "*":por defecto "-":el propio del motor otro:path to libro polyglot
        self._bookRR = "ap"

    def ponHuella(self, liEngines):
        liHuellas = [en._huella for en in liEngines if en != self]
        while True:
            self._huella = Util.huella()
            if self._huella not in liHuellas:
                return

    def copiar(self, liEngines):
        otro = Engine()
        otro.restore(self.save())
        otro._depth = self._depth
        otro._time = self._time
        otro._book = self._book
        otro._bookRR = self._bookRR
        otro.ponHuella(liEngines)
        otro.alias += "-1"
        d = 1
        esta = True
        while esta:
            esta = False
            for uno in liEngines:
                if uno.alias == otro.alias:
                    d += 1
                    otro.alias = self.alias + "-%d" % d
                    esta = True
                    break
        return otro

    def huella(self):
        return self._huella

    def book(self, valor=None):
        if valor is not None:
            self._book = valor
        return self._book

    def bookRR(self, valor=None):
        if valor is not None:
            self._bookRR = valor
        return self._bookRR

    def depth(self, valor=None):
        if valor is not None:
            self._depth = valor
        return self._depth

    def time(self, valor=None):
        if valor is not None:
            self._time = valor
        return self._time

    def leerTXT(self, txt):
        dic = Util.txt2dic(txt)
        self.exe = dic["EXE"]
        self.alias = dic["ALIAS"]
        self.idName = dic["IDNAME"]
        self.idAuthor = dic["IDAUTHOR"]
        self.idInfo = dic["IDINFO"]
        self.elo = dic["ELO"]
        self._huella = dic["HUELLA"]
        self._depth = dic["DEPTH"]
        self._time = dic["TIME"]
        self._book = dic["BOOK"]
        self._bookRR = dic["BOOKRR"]
        self.multiPV = 0
        txtop = dic["OPCIONES"]
        self.liOpciones = []
        for parte in txtop.split("|"):
            if parte:
                op = MotoresExternos.OpcionUCI()
                op.leeTXT(parte)
                if op.nombre == "MultiPV":
                    self.multiPV = op.max
                    self.maxMultiPV = op.max
                self.liOpciones.append(op)

    def grabarTXT(self):
        dic = {}
        dic["EXE"] = Util.dirRelativo(self.exe)
        dic["ALIAS"] = self.alias
        dic["IDNAME"] = self.idName
        dic["IDAUTHOR"] = self.idAuthor
        dic["IDINFO"] = self.idInfo
        dic["ELO"] = self.elo
        dic["HUELLA"] = self._huella
        dic["DEPTH"] = self._depth
        dic["TIME"] = self._time
        dic["BOOK"] = self._book
        dic["BOOKRR"] = self._bookRR
        txtop = ""
        for opcion in self.liOpciones:
            txtop += opcion.grabaTXT() + "|"
        dic["OPCIONES"] = txtop.strip("|")

        return Util.dic2txt(dic)

    def configMotor(self):
        return MotoresExternos.ConfigMotor(self)

    def leerConfigEngine(self, resp):
        if resp.startswith("*"):
            me = MotoresExternos.buscaMotor(resp)
            self.exe = Util.dirRelativo(me.exe)
            self.args = me.args
            self.alias = me.alias
            self.idName = me.idName
            self.clave = me.clave
            self.idAuthor = me.idAuthor
            self.idInfo = me.idInfo
            self.liOpciones = me.liOpciones
            self.maxMultiPV = me.maxMultiPV
            self.multiPV = me.multiPV
            self.elo = me.elo
        else:
            cm = VarGen.configuracion.buscaRival(resp)
            self.alias = cm.clave
            self.args = []
            self.idName = cm.nombre
            self.idAuthor = cm.autor
            self.idInfo = ""
            self.multiPV = cm.multiPV
            self.maxMultiPV = cm.maxMultiPV

            self.exe = cm.ejecutable()

            me = MotoresExternos.MotorExterno()
            me.leerUCI(self.exe, self.args)
            self.liOpciones = me.liOpciones
            for op in self.liOpciones:
                for comando, valor in cm.liUCI:
                    if op.nombre == comando:
                        if op.tipo == "check":
                            op.valor = valor.lower() == "true"
                        elif op.tipo == "spin":
                            op.valor = int(valor)
                        else:
                            op.valor = valor
                        break


class Game:
    def __init__(self):
        self._hwhite = None  # la huella de un engine
        self._hblack = None  # la huella de un engine
        self._partida = None
        self._minutos = None
        self._segundosJugada = None
        self._result = None
        self._date = None

    def hwhite(self, valor=None):
        if valor is not None:
            self._hwhite = valor
        return self._hwhite

    def hblack(self, valor=None):
        if valor is not None:
            self._hblack = valor
        return self._hblack

    def partida(self, valor=None):
        if valor is not None:
            self._partida = valor
        return self._partida

    def minutos(self, valor=None):
        if valor is not None:
            self._minutos = valor
        return self._minutos

    def segundosJugada(self, valor=None):
        if valor is not None:
            self._segundosJugada = valor
        return self._segundosJugada

    def etiTiempo(self):
        if self._minutos:
            return "%d+%d" % (self._minutos * 60, self._segundosJugada)
        else:
            return ""

    def result(self, valor=None):
        if valor is not None:
            self._result = valor
            self.date(Util.hoy())
        return self._result

    def date(self, valor=None):
        if valor is not None:
            self._date = valor
        return self._date

    def leerDIC(self, dic):
        self._hwhite = dic["HWHITE"]
        self._hblack = dic["HBLACK"]
        self._minutos = dic["MINUTOS"]
        self._segundosJugada = dic["SEGUNDOSJUGADA"]

        txt = dic["PARTIDA"]
        if txt is None:
            self._partida = None
        else:
            self._partida = Partida.Partida()
            self._partida.recuperaDeTexto(txt)

        self._result = dic["RESULT"]
        self._date = dic["DATE"]

    def grabarDIC(self):
        dic = {}
        dic["HWHITE"] = self._hwhite
        dic["HBLACK"] = self._hblack
        dic["RESULT"] = self._result
        dic["DATE"] = self._date
        dic["MINUTOS"] = self._minutos
        dic["SEGUNDOSJUGADA"] = self._segundosJugada

        dic["PARTIDA"] = None if self._partida is None else self._partida.guardaEnTexto()
        return dic

    def pgn(self, torneo):
        if self._result is None:
            return None

        dt = self.date()
        enw = torneo.buscaHEngine(self.hwhite())
        if enw is None:
            return None
        enb = torneo.buscaHEngine(self.hblack())
        if enb is None:
            return None

        rs = {0: "1/2-1/2", 1: "1-0", 2: "0-1"}[self.result()]

        li = [
            ("Event", torneo.nombre()),
            ("Date", "%d.%02d.%02d" % (dt.year, dt.month, dt.day)),
            ("White", enw.alias),
            ("Black", enb.alias),
            ("Result", rs),
        ]
        if not self._partida.siFenInicial():
            li.append(("FEN", self._partida.iniPosicion.fen()))

        ap = self._partida.apertura
        if ap:
            li.append(("ECO", ap.eco))
            li.append(("Opening", ap.nombre))

        cabecera = ""
        for campo, valor in li:
            cabecera += '[%s "%s"]\n' % (campo, valor)

        base = self._partida.pgnBase()

        return cabecera + "\n" + base + " %s\n\n" % rs


class Torneo:
    def __init__(self, nombre=""):
        self._nombre = nombre
        self._resign = 150
        self._drawMinPly = 50
        self._drawRange = 10
        self._liEngines = []
        self._liGames = []
        self._ultCarpetaEngines = ""
        self._ultMinutos = 15
        self._ultSegundosJugada = 0
        self._book = ""  # "":por defecto otro:path to libro polyglot
        self._fen = ""
        self._norman = True

        self._liResult = None

    def nombre(self, valor=None):
        if valor is not None:
            self._nombre = valor
        return self._nombre

    def fen(self, valor=None):
        if valor is not None:
            self._fen = valor
        return self._fen

    def norman(self, valor=None):
        if valor is not None:
            self._norman = valor
        return self._norman

    def fenNorman(self):
        if self._fen:
            return self._fen
        if self._norman:
            with open("./IntFiles/40H-Openings.epd") as f:
                lista = [linea for linea in f.read().split("\n") if linea.strip()]
                fen = random.choice(lista)
                fen = fen[:fen.index("id")].strip()
            return fen + " 0 1"
        return ""

    def resign(self, valor=None):
        if valor is not None:
            self._resign = valor
        return self._resign

    def drawMinPly(self, valor=None):
        if valor is not None:
            self._drawMinPly = valor
        return self._drawMinPly

    def drawRange(self, valor=None):
        if valor is not None:
            self._drawRange = valor
        return self._drawRange

    def ultCarpetaEngines(self, valor=None):
        if valor is not None:
            self._ultCarpetaEngines = valor
        return self._ultCarpetaEngines

    def ultMinutos(self, valor=None):
        if valor is not None:
            self._ultMinutos = valor
        return self._ultMinutos

    def ultSegundosJugada(self, valor=None):
        if valor is not None:
            self._ultSegundosJugada = valor
        return self._ultSegundosJugada

    def book(self, valor=None):
        if valor is not None:
            self._book = valor
        return self._book

    def fichero(self):
        if self._nombre:
            return os.path.join(VarGen.configuracion.carpeta, self._nombre + ".mvm")
        else:
            return None

    def grabaPGNgames(self, lista):
        pgn = ""
        for pos in lista:
            gm = self._liGames[pos]
            pgn1 = gm.pgn(self)
            if pgn1:
                pgn += pgn1 + "\n"
        return pgn

    def leerDIC(self, dic):
        self._resign = dic["RESIGN"]
        self._drawMinPly = dic["DRAWMINPLY"]
        self._drawRange = dic["DRAWRANGE"]
        self._ultCarpetaEngines = dic["ULTCARPETAENGINES"]
        self._ultMinutos = dic["ULTMINUTOS"]
        self._ultSegundosJugada = dic["ULTSEGUNDOSJUGADA"]
        self._fen = dic.get("FEN", "")
        self._norman = dic.get("NORMAN", False)

        liTxt = dic["ENGINES"]
        li = []
        for txt in liTxt:
            me = Engine()
            me.leerTXT(txt)
            li.append(me)
        self._liEngines = li

        liDic = dic["GAMES"]
        li = []
        for dic in liDic:
            gm = Game()
            gm.leerDIC(dic)
            li.append(gm)
        self._liGames = li

    def leer(self):
        dic = Util.recuperaVar(self.fichero())
        if not dic:
            return
        self.leerDIC(dic)

    def grabarDIC(self):
        dic = {}
        dic["RESIGN"] = self._resign
        dic["DRAWMINPLY"] = self._drawMinPly
        dic["DRAWRANGE"] = self._drawRange
        dic["ULTCARPETAENGINES"] = self._ultCarpetaEngines
        dic["ULTMINUTOS"] = self._ultMinutos
        dic["ULTSEGUNDOSJUGADA"] = self._ultSegundosJugada
        dic["FEN"] = self._fen
        dic["NORMAN"] = self._norman

        dic["ENGINES"] = [en.grabarTXT() for en in self._liEngines]
        dic["GAMES"] = [gm.grabarDIC() for gm in self._liGames]
        return dic

    def grabar(self):
        Util.guardaVar(self.fichero(), self.grabarDIC())

    def clone(self):
        t = Torneo()
        t.leerDIC(self.grabarDIC())
        return t

    def numEngines(self):
        return len(self._liEngines)

    def liEngines(self):
        return self._liEngines

    def appendEngine(self, me):
        self._liEngines.append(me)

    def copyEngine(self, me):
        otro = me.copiar(self._liEngines)
        self._liEngines.append(otro)

    def delEngines(self, lista):
        liBgm = []
        for pos in lista:
            en = self._liEngines[pos]
            huella = en.huella()
            for n, gm in enumerate(self._liGames):
                if gm.hwhite() == huella or gm.hblack() == huella:
                    liBgm.append(n)
        if liBgm:
            li = []
            for x in range(len(self._liGames)):
                if x not in liBgm:
                    li.append(self._liGames[x])
            self._liGames = li

        li = []
        for x in range(len(self._liEngines)):
            if x not in lista:
                li.append(self._liEngines[x])
        self._liEngines = li

    def buscaHEngine(self, huella):
        for en in self._liEngines:
            if en.huella() == huella:
                return en
        return None

    def numGames(self):
        return len(self._liGames)

    def liGames(self):
        return self._liGames

    def randomize(self):
        random.shuffle(self._liGames)
        num_games = len(self._liGames)
        for n in range(1, num_games-1):
            gm1 = self._liGames[n]
            gm0 = self._liGames[n-1]
            if gm0.hwhite() == gm1.hwhite() or gm0.hblack() == gm1.hblack():
                for pos in range(n+1, num_games):
                    gm2 = self._liGames[pos]
                    if not(gm2.hwhite() == gm1.hwhite() or gm2.hblack() == gm1.hblack()
                           or gm2.hwhite() == gm0.hwhite() or gm2.hblack() == gm0.hblack()):
                        self._liGames[pos] = gm1
                        self._liGames[n] = gm2
                        break

    def delGames(self, lista):
        li = []
        for x in range(len(self._liGames)):
            if x not in lista:
                li.append(self._liGames[x])
        self._liGames = li

    def nuevoGame(self, hwhite, hblack, minutos, segundosJugada):
        gm = Game()
        gm.hwhite(hwhite)
        gm.hblack(hblack)
        gm.minutos(minutos)
        gm.segundosJugada(segundosJugada)
        self._liGames.append(gm)

    def rehacerResult(self):
        liResult = []
        dbus = {}
        for pos, en in enumerate(self._liEngines):
            dic = {}
            dic["EN"] = en
            dic["WIN"] = [0, 0]
            dic["DRAW"] = [0, 0]
            dic["LOST"] = [0, 0]
            dic["PTS"] = 0
            dbus[en.huella()] = pos
            liResult.append(dic)

        for gm in self._liGames:
            rs = gm.result()
            if rs is None:
                continue
            hwhite = gm.hwhite()
            hblack = gm.hblack()
            pw = dbus.get(hwhite, -1)
            pb = dbus.get(hblack, -1)
            if pw >= 0 and pb >= 0:
                if rs == 0:
                    liResult[pw]["DRAW"][0] += 1
                    liResult[pb]["DRAW"][1] += 1
                    liResult[pw]["PTS"] += 5
                    liResult[pb]["PTS"] += 5
                elif rs == 1:
                    liResult[pw]["WIN"][0] += 1
                    liResult[pb]["LOST"][1] += 1
                    liResult[pw]["PTS"] += 10
                else:
                    liResult[pw]["LOST"][0] += 1
                    liResult[pb]["WIN"][1] += 1
                    liResult[pb]["PTS"] += 10

        return sorted(liResult, key=lambda x: x["PTS"], reverse=True)
