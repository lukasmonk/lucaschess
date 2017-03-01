import codecs
import os
import random

from Code import Util


class BaseTactica:
    def __init__(self):

        self.PUZZLES = 999  # Max number of puzzles in each block
        self.JUMPS = []  # Puzzle repetitions, each puzzle can be repeated, this field determine separations among replicates, more repetitions are separated by comma, eg 3,5,7
        self.FILLEND = 0  # Si los flecos que quedan al final se rellenan con los puzzles mas antiguos,por orden
        self.REPEAT = [
            0, ]  # Block repetitions, Repetitions of total of puzzles, comma separated, indicating order of repetition as 0=original, 1=random, 2=previous, eg 1,2,0=3 repetitions,
        # first is random, second repeat previous, and third original. Total field in blanck is the same as 0
        # Penalties, divided in blocks, by example 1,3,5,7 means first 25%
        # (100%/4) of puzzles has one position of penalty when error, second 25%
        # three and so on. Blank means no penalties.
        self.PENALIZATION = []
        self.SHOWTEXT = [1, ]  # Text while training, in blocks like penalties, 1,0,0,0, 25% Yes, rest No
        self.POINTVIEW = 0  # El que corresponda, 1 = White 2 = Black
        self.REFERENCE = ""

    def comas2texto(self, campo):
        li = getattr(self, campo)
        if li:
            lix = [str(x) for x in li]
            return ",".join(lix)
        else:
            return ""


def leePOINTVIEW(dic, default="0"):
    c = dic.get("POINTVIEW", default)
    return int(c)


def leeREFERENCE(dic, default=""):
    c = dic.get("REFERENCE", default)
    return c


def leeJUMPS(dic, default=None):
    JUMPS = [] if default is None else default
    c = dic.get("JUMPS", None)
    if c:
        JUMPS = [int(x) for x in c.replace(" ", "").split(",")]
    return JUMPS


def leeFILLEND(dic, default=None):
    FILLEND = False if default is None else default
    c = dic.get("FILLEND", None)
    if c and c.isdigit():
        FILLEND = c == "1"
    return FILLEND


def leeREPEAT(dic, default=None):
    REPEAT = [0, ] if default is None else default
    c = dic.get("REPEAT", None)
    if c:
        REPEAT = [int(x) for x in c.replace(" ", "").split(",")]
    return REPEAT


def leePENALIZATION(dic, default=None):
    PENALIZATION = [] if default is None else default
    c = dic.get("PENALIZATION", None)
    if c:
        PENALIZATION = [int(x) for x in c.replace(" ", "").split(",")]
    return PENALIZATION


def leeSHOWTEXT(dic, default=None):
    SHOWTEXT = [1, ] if default is None else default
    c = dic.get("SHOWTEXT", None)
    if c:
        SHOWTEXT = [int(x) for x in c.replace(" ", "").split(",")]
    if not SHOWTEXT:
        SHOWTEXT = [1, ]
    return SHOWTEXT


def leePUZZLES(dic, default=None):
    PUZZLES = 999 if default is None else default
    c = dic.get("PUZZLES", None)
    if c is not None and c.isdigit():
        PUZZLES = int(c)
    if not PUZZLES:
        PUZZLES = 999
    return PUZZLES


class Tacticas:
    """
[CONFIGURATION]
JUMPS=3,5,9,16
# Puzzles 100 if possible
PUZZLES=100
# FillEND
FILLEND=0
# Repeat : 0 = No, 1=Igual 2=Random
REPEAT=2

FOLDER="."

[ALIAS]
D1=*0.fns
D2=*1.fns
D3=*2.fns
D4=*3.fns
D5=*4.fns

[MENU]
#Menu,= Fichero/alias:weight,...
CATEGORIAS|PRINCIPIANTE=D1:100
CATEGORIAS|AFICIONADO=D1:70,D2:30
    """

    def __init__(self, tipo, nombre, carpeta, ini):

        self.nombre = nombre
        self.carpeta = carpeta
        self.ini = ini
        self.tipo = tipo

        self.dic = Util.ini8dic(self.ini)

        self.defecto()

    def defecto(self):
        self.JUMPS = 2, 4, 8, 16
        self.PUZZLES = 100
        self.FILLEND = 0
        self.REPEAT = 0,
        self.PENALIZATION = []
        self.FOLDER = self.carpeta
        self.POINTVIEW = 0
        self.REFERENCE = ""

    def listaMenus(self):
        liMenu = []
        for k in self.dic:
            if k.upper().startswith("TACTIC"):
                d = self.dic[k]
                menu = d["MENU"]
                liMenu.append((k, menu.split(",")))
        return liMenu

    def eligeTactica(self, resp):
        return Tactica(self, resp)

    def leeCommon(self):

        dic = self.dic["COMMON"] if "COMMON" in self.dic else {}

        self.PUZZLES = leePUZZLES(dic)
        self.JUMPS = leeJUMPS(dic)
        self.FILLEND = leeFILLEND(dic)
        self.REPEAT = leeREPEAT(dic)
        self.PENALIZATION = leePENALIZATION(dic)
        self.SHOWTEXT = leeSHOWTEXT(dic)
        self.POINTVIEW = leePOINTVIEW(dic)
        self.REFERENCE = leeREFERENCE(dic)

        self.FOLDER = dic.get("FOLDER", self.carpeta)


class Tactica(BaseTactica):
    def __init__(self, tts, nombre):
        BaseTactica.__init__(self)

        self.tts = tts
        self.nombre = nombre
        self.dic = self.tts.dic[nombre]
        menu = self.dic["MENU"]
        li = menu.split(",")
        for x in range(len(li)):
            li[x] = _SP(li[x])
        self.titulo = "-".join(li)

        tts.leeCommon()
        dic = self.dic

        self.PUZZLES = leePUZZLES(dic, tts.PUZZLES)
        self.JUMPS = leeJUMPS(dic, tts.JUMPS)
        self.FILLEND = leeFILLEND(dic, tts.FILLEND)
        self.REPEAT = leeREPEAT(dic, tts.REPEAT)
        self.PENALIZATION = leePENALIZATION(dic, tts.PENALIZATION)
        self.SHOWTEXT = leeSHOWTEXT(dic, tts.SHOWTEXT)
        self.POINTVIEW = leePOINTVIEW(dic, tts.POINTVIEW)
        self.REFERENCE = leeREFERENCE(dic, tts.REFERENCE)

        fw = self.dic["FILESW"]
        li = fw.split(",")
        self.filesw = []
        for x in li:
            li = x.split(":")
            nli = len(li)
            d = None
            h = None
            if nli == 2:
                f, w = li
                w = int(w)
            elif nli == 0:
                f = x
                w = 100
            elif nli == 4:
                f, w, d, h = li
                w = int(w)
                d = int(d)
                h = int(h)
            else:
                f = li[0]
                w = int(li[1])

            self.filesw.append((f, w, d, h))  # file, weight,from,to

        self.db = None

    def tituloAmpliado(self):
        return self.tts.nombre + " " + self.titulo

    def ponPosActual(self, pos):
        self.db["POSACTIVE"] = pos

    def close(self):
        if self.db:
            self.db.close()
            self.db = None

    def leeDatos(self, carpetaUser):
        fcache = "%s/%s%s.tdb" % (carpetaUser, self.tts.nombre, self.tts.tipo)
        self.db = Util.DicSQL(fcache, tabla=self.nombre)
        self.liFNS = self.db["LIFNS"]
        # if self.liFNS is None:
        # self.genera()
        # self.liFNS = self.db["LIFNS"]

        self.liOrder = self.db["ORDER"]

        self.posActive = self.db["POSACTIVE"]

        if self.liFNS is None:
            self.liFNS = []
        if self.liOrder is None:
            self.liOrder = []

        showtext = self.db["SHOWTEXT"]
        if showtext:
            self.SHOWTEXT = showtext
        if not self.SHOWTEXT:
            self.SHOWTEXT = [1, ]

        errores = self.db["ERRORS"]
        if errores is None:
            self.db["ERRORS"] = 0

        reference = self.db["REFERENCE"]
        if reference is None:
            self.db["REFERENCE"] = ""

    def listaFicheros(self, clave):
        dalias = self.tts.dic.get("ALIAS", {})
        if clave in dalias:
            return self.listaFicheros(dalias[clave])
        lif = []
        if "," in clave:
            for uno in clave.split(","):
                lif.extend(self.listaFicheros(uno))
        elif "*" in clave or "?" in clave or "[" in clave:
            lif.extend(Util.listfiles(self.tts.FOLDER, clave))
        else:
            lif.append(os.path.join(self.tts.FOLDER, clave))
        return lif

    def calculaTotales(self):
        li = []
        for f, w, d, h in self.filesw:
            t = 0
            for fich in self.listaFicheros(f):
                q = open(fich)
                for linea in q:
                    linea = linea.strip()
                    if linea and "|" in linea:
                        t += 1
                q.close()
            li.append(t)
        return li

    def genera(self):
        num = self.PUZZLES

        # Determinamos la lista de fens, teniendo en cuenta el peso asociado a cada fichero
        lif = []

        wt = 0
        for f, w, d, h in self.filesw:
            lif0 = []
            for fich in self.listaFicheros(f):
                q = codecs.open(fich, "r", "utf-8", 'ignore')
                for linea in q:
                    linea = linea.strip()
                    if linea and "|" in linea:
                        lif0.append(linea)
                q.close()
            if d and d <= h:
                d -= 1
                lif0 = lif0[d:h]
            lif.append([w, lif0])
            wt += w
        t = 0
        for li in lif:
            li[0] = int(li[0] * num / wt)
            t += li[0]
        t -= self.PUZZLES
        n = 0
        while t:
            lif[0][n] += 1
            t -= 1
            n += 1
            if n == len(lif):
                n = 0

        liFNS = []
        for li in lif:
            n = li[0]
            lif0 = li[1]
            if len(lif0) < n:
                n = len(lif0)
            lir = lif0[:n]
            for x in lir:
                liFNS.append(x)

        self.db["LIFNS"] = liFNS
        self.liFNS = liFNS

        numPuzzles = len(liFNS)

        # Deteminamos la lista indice con el orden de cada fen en liFNS
        liJUMPS = self.JUMPS

        li = [None] * (len(liJUMPS) * 2 * numPuzzles)  # Creamos un list muy grande, mayor del que vamos a usar

        def busca(desde, salto):
            if salto == 0:
                for x in range(desde, len(li)):
                    if li[x] is None:
                        return x
                li.extend([None] * 1000)
                return busca(desde, salto)
            else:
                while salto:
                    desde = busca(desde + 1, 0)
                    salto -= 1
                return desde

        for x in range(numPuzzles):
            n = busca(0, 0)
            li[n] = x
            for m in liJUMPS:
                n = busca(n + 1, int(m))
                li[n] = x

        liBase = []
        for x in li:
            if x is not None:
                liBase.append(x)

        liOrder = []

        liNueva = liBase[:]
        for repeat in self.REPEAT:
            if repeat == 0:  # Original
                liNueva = liBase[:]
            elif repeat == 1:
                liNueva = liBase[:]
                random.shuffle(liNueva)
            else:
                liNueva = liNueva[:]
            liOrder.extend(liNueva)

        self.db["ORDER"] = liOrder
        self.liOrder = liOrder

        self.db["POSACTIVE"] = 0

        self.db["SHOWTEXT"] = self.SHOWTEXT

        self.db["POINTVIEW"] = self.POINTVIEW

        self.db["REFERENCE"] = self.REFERENCE

        # 6.3d---------------+
        liHisto = self.db["HISTO"]
        if not liHisto:
            liHisto = []
        dicActual = {"FINICIAL": Util.hoy(), "FFINAL": None, "SECONDS": 0.0, "POS": self.numPosiciones(), "ERRORS": 0,
                     "PUZZLES": self.PUZZLES, "FILESW": self.filesw, "JUMPS": self.JUMPS, "REPEAT": self.REPEAT,
                     "SHOWTEXT": self.SHOWTEXT, "PENALIZATION": self.PENALIZATION}
        liHisto.insert(0, dicActual)
        self.db["HISTO"] = liHisto
        # 6.3d---------------+

        # 7.0---------------+
        self.db["SECONDS"] = 0.0
        self.db["ERRORS"] = 0
        # 7.0---------------+

        self.db.pack()

    def listaOrden(self):
        return self.liOrder

    def posActual(self):
        return self.db["POSACTIVE"]

    def terminada(self):
        pactive = self.db["POSACTIVE"]
        return not self.historico() or pactive is None or pactive >= self.numPosiciones()

    def numPosiciones(self):
        return len(self.liOrder)

    def siShowText(self):
        n = len(self.SHOWTEXT)
        if n == 0:
            return True
        posActual = self.posActual()
        numPosiciones = self.numPosiciones()
        bloque = numPosiciones * 1.0 / n
        ns = int(posActual / bloque)
        if ns >= n:
            ns = n - 1
        return self.SHOWTEXT[ns] == 1

    def numFNS(self):
        return len(self.liFNS)

    def puestosPenalizacion(self, posic, total):
        if self.PENALIZATION:
            li = self.PENALIZATION
            n = total / len(li)
            if n == 0:
                return 0
            n1 = posic / n
            if n1 >= len(li):
                n1 = len(li) - 1
            return li[n1]
        else:
            return 0

    def unFNS(self, pos):
        return self.liFNS[pos]

    def pointView(self):
        n = self.db["POINTVIEW"]
        if n is None:
            n = 0
        return int(n)

    def masSegundos(self, mas):
        if "SECONDS" in self.db:
            self.db["SECONDS"] += mas

    def segundosActivo(self):
        return self.db["SECONDS"]

    def referenciaActivo(self):
        return self.db["REFERENCE"]

    def erroresActivo(self):
        return self.db["ERRORS"]

    def nuevoError(self):
        self.db["ERRORS"] += 1

    def finalEntrenamiento(self):
        liHisto = self.db["HISTO"]
        if not liHisto:
            liHisto = []
            dicActual = {"FINICIAL": Util.hoy(), "FFINAL": None, "SECONDS": 0.0, "POS": self.numPosiciones(),
                         "ERRORS": 0}
            liHisto.insert(0, dicActual)
        liHisto[0]["FFINAL"] = Util.hoy()
        liHisto[0]["SECONDS"] = self.db["SECONDS"]
        liHisto[0]["ERRORS"] = self.db["ERRORS"]
        liHisto[0]["REFERENCE"] = self.db["REFERENCE"]

        self.db["HISTO"] = liHisto
        self.db["POSACTIVE"] = None

    def historico(self):
        liHisto = self.db["HISTO"]
        return [] if liHisto is None else liHisto

    def borraListaHistorico(self, liNum):
        liHisto = self.historico()
        liNueHisto = []
        for x in range(len(liHisto)):
            if x not in liNum:
                liNueHisto.append(liHisto[x])
        self.db["HISTO"] = liNueHisto
        if 0 in liNum:
            self.db["POSACTIVE"] = None
