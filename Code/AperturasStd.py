import LCEngine4 as LCEngine

from operator import attrgetter

from Code import TrListas
from Code import Util
from Code import VarGen


class AperturaStd:
    def __init__(self, clave):
        self.nombre = clave
        self.trNombre = clave
        self.nivel = 0
        self.padre = None
        self.hijos = []
        self.a1h8 = ""
        self.pgn = ""
        self.eco = ""
        self.siBasic = False

    def trPGN(self):
        p = ""
        pzs = "KQRBNPkqrbnp"
        pgn = self.pgn
        for n, c in enumerate(pgn):
            if c in pzs and not pgn[n + 1].isdigit():
                c = TrListas.letterPiece(c)
            p += c
        return p

    def __str__(self):
        return self.nombre + " " + self.pgn


class ListaAperturasStd:
    def __init__(self):
        self.dic = None
        self.dicFenM2 = None
        self.hijos = None
        self.lia1h8 = None

    def reset(self, configuracion, siBasic, siEntrenar):
        self.dic = {}
        self.hijos = []
        ficheroPers = configuracion.ficheroPersAperturas
        self.lee(ficheroPers, siEntrenar)
        self.lia1h8 = self.dic.keys()

        if siBasic:
            for bl in self.dic.itervalues():
                bl.trOrdena = ("A" if bl.siBasic else "B") + bl.trNombre.upper()
            self.hijos = self.ordena(self.hijos, 0)

        dfen = {}
        makePV = LCEngine.makePV
        fen2fenM2 = LCEngine.fen2fenM2

        mx = 0
        for n, (pv, ap) in enumerate(self.dic.iteritems(), 1):
            fen = makePV(pv)
            dfen[fen2fenM2(fen)] = ap
            if n > mx:
                mx = n
        self.dicFenM2 = dfen
        self.max_ply = mx

    def ordena(self, hijos, n):
        if hijos:
            hijos = sorted(hijos, key=attrgetter("trOrdena"))
            for hijo in hijos:
                hijo.hijos = self.ordena(hijo.hijos, n + 8)
        return hijos

    def leeEstandar(self, dic):
        listSTD = TrListas.listSTD()

        for name, eco, a1h8, pgn, siBasic in listSTD:
            bloque = AperturaStd(name)
            bloque.eco = eco
            bloque.a1h8 = a1h8
            bloque.pgn = pgn
            bloque.siBasic = siBasic
            dic[bloque.a1h8] = bloque

    def leePersonal(self, ficheroPers, siEntrenar):
        lista = Util.recuperaVar(ficheroPers)
        txt = ""
        if lista:
            for reg in lista:
                estandar = reg["ESTANDAR"]
                if siEntrenar or estandar:
                    txt += "\n[%(NOMBRE)s]\nECO=%(ECO)s\nA1H8=%(A1H8)s\nPGN=%(PGN)s\nBASIC=True" % reg
        return txt

    def lee(self, ficheroPers, siEntrenar):
        d = {}
        self.leeEstandar(d)
        txt = self.leePersonal(ficheroPers, siEntrenar)

        li = txt.split("\n")

        bloque = None
        for linea in li:
            linea = linea.strip()
            if linea:
                if linea.startswith("["):
                    if bloque:
                        d[bloque.a1h8] = bloque
                    bloque = AperturaStd(linea.strip()[1:-1].strip())
                else:
                    c, v = linea.split("=")
                    if c == "A1H8":
                        bloque.a1h8 = v
                    elif c == "PGN":
                        bloque.pgn = v
                    elif c == "BASIC":
                        bloque.siBasic = v == "True"
                    elif c == "ECO":
                        bloque.eco = v
        if bloque:
            d[bloque.a1h8] = bloque

        self.dic = d

        li = d.keys()
        li.sort()
        for k in li:
            bloque = d[k]
            a1h8 = bloque.a1h8
            n = a1h8.rfind(" ")
            while n > 0:
                a1h8 = a1h8[:n]
                if a1h8 in d:
                    bloquePadre = d[a1h8]
                    nivel = bloquePadre.nivel + 1
                    if nivel > 8:
                        bloquePadre = bloquePadre.padre
                        nivel = bloquePadre.nivel + 1
                    hijos = bloquePadre.hijos
                    if hijos is None:
                        bloquePadre.hijos = hijos = []
                    hijos.append(bloque)
                    bloque.padre = bloquePadre
                    bloque.nivel = nivel
                    break
                n = a1h8.rfind(" ")
            if n <= 0:
                self.hijos.append(bloque)

    def asignaTransposition(self, partida):
        partida.transposition = None
        if not (partida.apertura is None or partida.pendienteApertura):
            for nj, jg in enumerate(partida.liJugadas):
                if not jg.siApertura:
                    fenm2 = jg.posicion.fenM2()
                    if fenm2 in self.dicFenM2:
                        partida.transposition = self.dicFenM2[fenm2]
                        partida.jg_transposition = jg

    def asignaApertura(self, partida):
        partida.apertura = None
        if not partida.siFenInicial():
            partida.pendienteApertura = False
            return
        partida.pendienteApertura = True
        a1h8 = ""
        for nj, jg in enumerate(partida.liJugadas):
            if jg.siApertura:
                jg.siApertura = False
            a1h8 += jg.movimiento()
            if a1h8 in self.dic:
                partida.apertura = self.dic[a1h8]
                for nP in range(nj + 1):
                    partida.jugada(nP).siApertura = True
                noHayMas = True
                for k in self.dic:
                    if k.startswith(a1h8) and k != a1h8:
                        noHayMas = False
                        break
                if noHayMas:
                    partida.pendienteApertura = False  # la ponemos como definitiva
                    return
            a1h8 += " "
        # Hay alguna posible ?
        a1h8 = a1h8.strip()
        for k in self.dic:
            if k.startswith(a1h8):
                return  # volvemos con la apertura pendiente aun
        partida.pendienteApertura = False  # No hay ninguna aplicable

    def asignaAperturaListaMoves(self, liMoves):  # PGO
        opening = ""

        a1h8 = ""
        for mv in liMoves:
            a1h8 += " " + mv.pv()
            a1h8 = a1h8.strip()
            if a1h8 in self.dic:
                opening = self.dic[a1h8]
        return opening

    def listaAperturasPosibles(self, partida, siTodas=False):
        a1h8 = ""
        for jugada in partida.liJugadas:
            a1h8 += " " + jugada.movimiento()
        a1h8 = a1h8[1:]
        li = []

        # Las ordenamos para que esten antes las principales que las variantes
        lik = self.dic.keys()
        lik.sort()

        siBasic = len(partida) == 0
        if siTodas:
            siBasic = False

        for k in lik:
            if k.startswith(a1h8) and len(k) > len(a1h8):
                # Comprobamos que no sea una variante de las a_adidas, no nos interesan para mostrar opciones al usuario
                siMas = True
                for ap in li:
                    if k.startswith(ap.a1h8):
                        siMas = False
                        break
                if siMas:
                    ap = self.dic[k]
                    ap.liMovs = a1h8[len(k):].strip().split(" ")
                    if siBasic and not ap.siBasic:
                        continue
                    li.append(ap)

        return li if li else None

    def baseXPV(self, xpv):
        lipv = LCEngine.xpv2pv(xpv).split(" ")
        last_ap = None

        LCEngine.setFenInicial()
        mx = self.max_ply + 3
        for n, pv in enumerate(lipv):
            if n > mx:
                break
            LCEngine.makeMove(pv)
            fen = LCEngine.getFen()
            fenM2 = LCEngine.fen2fenM2(fen)
            if fenM2 in self.dicFenM2:
                last_ap = self.dicFenM2[fenM2]
        return last_ap

    def XPV(self, xpv):
        last_ap = self.baseXPV(xpv)
        return last_ap.trNombre if last_ap else ""

ap = ListaAperturasStd()
apTrain = ListaAperturasStd()


def reset():
    configuracion = VarGen.configuracion
    ap.reset(configuracion, False, False)
    apTrain.reset(configuracion, True, True)
