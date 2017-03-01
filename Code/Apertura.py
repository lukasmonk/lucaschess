import random

from Code import Books
from Code import Partida
from Code import Util
from Code import VarGen


class EtiApertura:
    def __init__(self, nombre, eco, a1h8, pgn):
        self.nombre = nombre
        self.eco = eco
        self.a1h8 = a1h8.split(" ")
        self.pgn = pgn
        self.liHijos = []

    def hijo(self, ea):
        self.liHijos.append(ea)


class AperturaPol:
    def __init__(self, maxNivel, elo=None):
        if elo:
            siPTZ = elo < 1700
        else:
            siPTZ = 1 <= maxNivel <= 2
        self.fichero = VarGen.tbookPTZ if siPTZ else VarGen.tbook
        self.book = Books.Polyglot()
        if not ((Util.tamFichero(self.fichero) / (len(self.fichero) - 9)) in (75876, 802116)):
            import sys

            sys.exit()
        self.activa = True
        self.maxNivel = maxNivel * 2
        self.nivelActual = 0
        self.siObligatoria = False

    def marcaEstado(self):
        dic = {"ACTIVA": self.activa, "NIVELACTUAL": self.nivelActual, "SIOBLIGATORIA": self.siObligatoria}
        return dic

    def recuperaEstado(self, dic):
        self.activa = dic["ACTIVA"]
        self.nivelActual = dic["NIVELACTUAL"]
        self.siObligatoria = dic["SIOBLIGATORIA"]

    def leeRandom(self, fen):
        li = self.book.lista(self.fichero, fen)
        if not li:
            return None
        liNum = []
        for nentry, entry in enumerate(li):
            liNum.extend([nentry] * (entry.weight + 1))  # Always entry.weight+1> 0
        return li[random.choice(liNum)]

    def isActive(self, fen):
        x = self.leeRandom(fen)
        return x is not None

    def juegaMotor(self, fen):
        self.nivelActual += 1
        if self.nivelActual > self.maxNivel:
            self.activa = False
            return False, None, None, None

        if not self.activa:
            return False, None, None, None

        entry = self.leeRandom(fen)
        if entry is None:
            self.activa = False
            return False, None, None, None

        pv = entry.pv()

        return True, pv[:2], pv[2:4], pv[4:]

    def compruebaHumano(self, fen, desde, hasta):
        if not self.activa:
            return False

        li = self.book.lista(self.fichero, fen)
        if not li:
            return False

        for entry in li:
            pv = entry.pv()
            if pv[:2] == desde and pv[2:4] == hasta:
                return True
        return False


class JuegaApertura:
    def __init__(self, a1h8):
        p = Partida.Partida()
        p.leerPV(a1h8)
        self.dicFEN = {}
        for jg in p.liJugadas:
            self.dicFEN[jg.posicionBase.fen()] = jg
        self.activa = True

    def juegaMotor(self, fen):
        try:
            jg = self.dicFEN[fen]
            return True, jg.desde, jg.hasta, jg.coronacion
        except:
            self.activa = False
            return False, None, None, None

    def compruebaHumano(self, fen, desde, hasta):
        if fen in self.dicFEN:
            jg = self.dicFEN[fen]
            return desde == jg.desde and hasta == jg.hasta
        else:
            self.activa = False
            return False

    def desdeHastaActual(self, fen):
        if fen in self.dicFEN:
            jg = self.dicFEN[fen]
            return jg.desde, jg.hasta
        self.activa = False
        return None, None
