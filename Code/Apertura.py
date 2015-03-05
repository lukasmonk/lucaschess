# -*- coding: utf-8 -*-

import random

import Code.Books as Books
import Code.VarGen as VarGen
import Code.Util as Util
import Code.Partida as Partida

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
    def __init__(self, maxNivel):
        self.fichero = VarGen.tbookPTZ if 1 <= maxNivel <= 2 else VarGen.tbook
        self.book = Books.Polyglot()
        if not ((Util.tamFichero(self.fichero) / (len(self.fichero) - 9)) in ( 54161, 860795) ):
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
        total = 0
        for entry in li:
            total += entry.weight
        nli = len(li)
        total2 = int(total / nli) + nli
        for entry in li:
            entry.weight += total2
        total += nli * total2

        elec = random.randint(1, total)
        pesos = 0
        for entry in li:
            pesos += (entry.weight + 1)
            if pesos >= elec:
                return entry
        return li[-1]

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

        # ~ ap = Apertura(100)

        #~ while ap.activa:
        #~ p_rint ap.juegaMotor()

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

