import os
import random
import shutil

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

        self._book = "-"  # "*":por defecto "-":el propio del motor otro:path to libro polyglot
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


class Game(object):
    def __init__(self):
        self._hwhite = None  # la huella de un engine
        self._hblack = None  # la huella de un engine
        self._partida = None
        self._minutos = None
        self._segundosJugada = None
        self._result = None
        self._date = None
        self._termination = None

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

    def termination(self, valor=None):
        if valor is not None:
            self._termination = valor
        return self._termination

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

        self._termination = dic.get("TERMINATION", "normal")

    def grabarDIC(self):
        dic = {}
        dic["HWHITE"] = self._hwhite
        dic["HBLACK"] = self._hblack
        dic["RESULT"] = self._result
        dic["DATE"] = self._date
        dic["MINUTOS"] = self._minutos
        dic["SEGUNDOSJUGADA"] = self._segundosJugada
        dic["TERMINATION"] = self._termination

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
        if enw.elo:
            li.append(("WhiteElo", "%d" % enw.elo))
        if enb.elo:
            li.append(("BlackElo", "%d" % enb.elo))
        if not self._partida.siFenInicial():
            li.append(("FEN", self._partida.iniPosicion.fen()))

        ap = self._partida.apertura
        if ap:
            li.append(("ECO", ap.eco))
            li.append(("Opening", ap.nombre))

        if self._termination:
            li.append(("Termination", self._termination))

        cabecera = ""
        for campo, valor in li:
            cabecera += '[%s "%s"]\n' % (campo, valor)

        base = self._partida.pgnBase()

        return cabecera + "\n" + base + " %s\n\n" % rs


class ListGames:
    def __init__(self, torneo):
        self.torneo = torneo
        self.db = None

    def test_db(self):
        if self.db is None:
            self.db = Util.DicSQL(self.torneo.fichero())

    def close(self):
        if self.db:
            self.db.close()
            self.db = None

    def __getitem__(self, pos):
        gm = Game()
        self.test_db()
        dc = self.db["GAME_%d" % pos]
        if dc:
            gm.leerDIC(dc)
            return gm
        return None

    def __setitem__(self, pos, gm):
        self.test_db()
        self.db["GAME_%d" % pos] = gm.grabarDIC()

    def __delitem__(self, pos):
        self.test_db()
        ng = self.db["NUM_GAMES"]
        if ng:
            del self.db["GAME_%d" % pos]
            if pos < ng-1:
                for x in range(pos, ng-1):
                    self.db["GAME_%d" % pos] = self.db["GAME_%d" % (pos +1,)]
            self.db["NUM_GAMES"] = ng -1

    def __iter__(self):
        self.test_db()
        self.pos_iter = 0
        return self

    def next(self):
        gm = self.__getitem__(self.pos_iter)
        self.pos_iter += 1
        if gm:
            return gm
        raise StopIteration

    def __len__(self):
        self.test_db()
        ng = self.db["NUM_GAMES"]
        if ng is None:
            ng = 0
        return ng

    def reset(self):
        self.test_db()
        ng = self.db["NUM_GAMES"]
        if ng:
            for x in range(ng):
                del self.db["GAME_%d" % x]
        self.db["NUM_GAMES"] = 0

    def append(self, gm):
        ng = self.db["NUM_GAMES"]
        if ng is None:
            ng = 0
        self.db["GAME_%d" % ng] = gm.grabarDIC()
        self.db["NUM_GAMES"] = ng + 1


class Torneo(object):
    def __init__(self, nombre=""):
        self._nombre = nombre
        self._resign = 150
        self._drawMinPly = 50
        self._drawRange = 10
        self._liEngines = []
        self._liGames = ListGames(self)
        self._ultCarpetaEngines = ""
        self._ultMinutos = 15
        self._ultSegundosJugada = 0
        self._book = ""  # "":por defecto otro:path to libro polyglot
        self._bookDepth = 0
        self._fen = ""
        self._norman = True

        self._liResult = None

    def close(self):
        self._liGames.close()

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

    def bookDepth(self, valor=None):
        if valor is not None:
            self._bookDepth = valor
        return self._bookDepth if self._bookDepth else 0

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

        self._book = dic.get("BOOK", "")
        self._bookDepth = dic.get("BOOKDEPTH", "")

        liTxt = dic["ENGINES"]
        li = []
        for txt in liTxt:
            me = Engine()
            me.leerTXT(txt)
            li.append(me)
        self._liEngines = li

    def convert(self, fichero):
        dic = Util.recuperaVar(fichero)
        os.remove(fichero)
        if not dic:
            return
        self._resign = dic["RESIGN"]
        self._drawMinPly = dic["DRAWMINPLY"]
        self._drawRange = dic["DRAWRANGE"]
        self._ultCarpetaEngines = dic["ULTCARPETAENGINES"]
        self._ultMinutos = dic["ULTMINUTOS"]
        self._ultSegundosJugada = dic["ULTSEGUNDOSJUGADA"]
        self._fen = dic.get("FEN", "")
        self._norman = dic.get("NORMAN", False)

        self._book = dic.get("BOOK", "")
        self._bookDepth = dic.get("BOOKDEPTH", "")

        liTxt = dic["ENGINES"]
        li = []
        for txt in liTxt:
            me = Engine()
            me.leerTXT(txt)
            li.append(me)
        self._liEngines = li

        if "GAMES" in dic:
            liDic = dic["GAMES"]
            self._liGames.reset()
            for dc in liDic:
                gm = Game()
                gm.leerDIC(dc)
                self._liGames.append(gm)
        self.grabarDIC(fichero)

    def leer(self):
        fichero = self.fichero()
        if fichero and Util.existeFichero(fichero):
            si_sqlite = False
            with open(fichero) as f:
                if f.read(3) == "SQL":
                    si_sqlite = True
            if not si_sqlite:
                self.convert(fichero)
            if Util.existeFichero(fichero):
                db = Util.DicSQL(fichero)
                self.leerDIC(db)
                db.close()

    def grabarDIC(self, fichero):
        self._liGames.test_db()
        db = self._liGames.db
        db["RESIGN"] = self._resign
        db["DRAWMINPLY"] = self._drawMinPly
        db["DRAWRANGE"] = self._drawRange
        db["ULTCARPETAENGINES"] = self._ultCarpetaEngines
        db["ULTMINUTOS"] = self._ultMinutos
        db["ULTSEGUNDOSJUGADA"] = self._ultSegundosJugada
        db["FEN"] = self._fen
        db["NORMAN"] = self._norman
        db["BOOK"] = self._book
        db["BOOKDEPTH"] = self._bookDepth
        db["ENGINES"] = [en.grabarTXT() for en in self._liEngines]

    def grabar(self):
        self.grabarDIC(self.fichero())

    def clone(self, nom_tmp, liNumGames):
        t = Torneo(nom_tmp)
        shutil.copy(self.fichero(), t.fichero())
        liNumGames.sort(reverse=True)
        for num in liNumGames:
            del t._liGames[num]
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

    def delEngines(self, lista=None):
        liBgm = []
        if lista is None:
            lista = range(len(self._liEngines))
        for pos in lista:
            en = self._liEngines[pos]
            huella = en.huella()
            for n, gm in enumerate(self._liGames):
                if gm.hwhite() == huella or gm.hblack() == huella:
                    liBgm.append(n)
        if liBgm:
            for x in range(len(self._liGames)-1, -1, -1):
                if x in liBgm:
                    del self._liGames[x]

        for x in range(len(self._liEngines)-1, -1, -1):
            if x in lista:
                del self._liEngines[x]

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
        return
        # num_games = len(self._liGames)
        # lista = range(num_games)
        # random.shuffle(lista)
        # liOtro = []
        # for dest in lista:
        #     liOtro.append(self._liGames[dest])
        # self._liGames.reset()\
        # for n in range(1, num_games-1):
        #     gm1 = self._liGames[n]
        #     gm0 = self._liGames[n-1]
        #     if gm0.hwhite() == gm1.hwhite() or gm0.hblack() == gm1.hblack():
        #         for pos in range(n+1, num_games):
        #             gm2 = self._liGames[pos]
        #             if not(gm2.hwhite() == gm1.hwhite() or gm2.hblack() == gm1.hblack()
        #                    or gm2.hwhite() == gm0.hwhite() or gm2.hblack() == gm0.hblack()):
        #                 self._liGames[pos] = gm1
        #                 self._liGames[n] = gm2
        #                 break

    def delGames(self, lista=None):
        for x in range(len(self._liGames)-1, -1, -1):
            if lista is None or x in lista:
                del self._liGames[x]

    def reiniciar(self, nombre):
        self._liGames.close()
        # self.delGames()
        # self.delEngines()
        self.__init__(nombre)
        if self._nombre:
            self.leer()

    def reiniciarTmp(self, torneoBase, liFiltro):
        self._liGames.reset()
        st = set(liFiltro)
        sten = set()
        for num, gm in enumerate(torneoBase._liGames):
            if num in st:
                self._liGames.append(gm)
                hw = gm._hwhite
                hb = gm._hblack
                if hw not in sten:
                    sten.add(hw)
                if hb not in sten:
                    sten.add(hb)
        self._liEngines = []
        for en in torneoBase._liEngines:
            if en.huella() in sten:
                self._liEngines.append(en)

    def nuevoGame(self, hwhite, hblack, minutos, segundosJugada):
        gm = Game()
        gm.hwhite(hwhite)
        gm.hblack(hblack)
        gm.minutos(minutos)
        gm.segundosJugada(segundosJugada)
        self._liGames.append(gm)

    def rehacerResult(self, st_filtro=None):
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

        for n in range(len(self._liGames)):
            if st_filtro and n not in st_filtro:
                continue
            gm = self._liGames[n]
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
