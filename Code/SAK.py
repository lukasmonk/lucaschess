# -*- coding: latin-1 -*-
"""
Rutinas internas para la conexion con winglet .dll .so
"""
from ctypes import *

import Code.VarGen as VarGen

class SAKmove:
    def __init__(self, txt, siExtended=False):
        if siExtended:
            self._pieza = txt[0]
            self._desde = txt[1:3]
            self._hasta = txt[3:5]
            self._coronacion = txt[5:6].strip()
            self._jaque = "+" in txt
            self._mate = "#" in txt
            self._captura = "x" in txt

        else:
            self._pieza = txt[0]
            self._desde = txt[1:3]
            self._hasta = txt[3:5]
            self._coronacion = txt[5:]

    def desde(self):
        return self._desde

    def hasta(self):
        return self._hasta

    def coronacion(self):
        return self._coronacion.lower()

    def movimiento(self):
        return self._desde+self._hasta+self._coronacion.lower()

    def jaque(self):
        return self._jaque

    def mate(self):
        return self._mate

    def captura(self):
        return self._captura

    def pieza(self):
        return self._pieza

    def __str__(self):
        return self._pieza+self._desde+self._hasta+(self._coronacion if self._coronacion else "")

class SAK:
    def __init__(self):
        if VarGen.isWindows:
            ext = "dll"
            tipo = "Windows"
        else:
            ext = "so"
            tipo = "Linux"
        dll = CDLL("Engines%s/winglet/winglet.%s" % (tipo, ext))

        # extern "C" __declspec(dllexport) void lc_inicio(void)

        # extern "C" __declspec(dllexport) void lc_setupFen(char *fen, char *fencolor, char *fencastling, char *fenenpassant, int fenhalfmoveclock, int fenfullmovenumber)
        dll.lc_setupFen.argtype = [c_char_p, c_char_p, c_char_p, c_char_p, c_int, c_int]

        # extern "C" __declspec(dllexport) char * lc_moves(void)
        dll.lc_moves.restype = c_char_p

        # extern "C" __declspec(dllexport) char * lc_exmoves(void)
        dll.lc_exmoves.restype = c_char_p

        # extern "C" __declspec(dllexport) void lc_makemove(int pos)
        dll.lc_makemove.argtype = [c_int]

        # extern "C" __declspec(dllexport) char * lc_getFen(void)
        dll.lc_getFen.restype = c_char_p

        # extern "C" __declspec(dllexport) char * lc_getPosition(void)
        dll.lc_getPosition.restype = c_char_p

        # extern "C" __declspec(dllexport) char * lc_toSan(void)
        dll.lc_toSAN.restype = c_char_p

        # extern "C" __declspec(dllexport) char * lc_pgn2pv(char * pgn)
        dll.lc_pgn2pv.argtype = [c_char_p]
        dll.lc_pgn2pv.restype = c_char_p

        # extern "C" __declspec(dllexport) char * lc_think(int sd)
        dll.lc_think.argtype = [c_int]
        dll.lc_think.restype = c_char_p

        # extern "C" __declspec(dllexport) int lc_check(void)
        dll.lc_check.restype = c_int

        dll.lc_inicio()

        self.dll = dll

    def setXFEN(self, posicion, color, enroques, alPaso, movPeonCap, jugadas):
        self.dll.lc_setupFen(str(posicion), str(color), str(enroques), str(alPaso), movPeonCap, jugadas)

    def setFEN(self, fen):
        posicion, color, enroques, alPaso, movPeonCap, jugadas = fen.split(" ")
        self.dll.lc_setupFen(str(posicion), str(color), str(enroques), str(alPaso), int(movPeonCap), int(jugadas))

    def getFEN(self):
        return self.dll.lc_getFen()

    def getPosition(self):
        return self.dll.lc_getPosition()

    def searchPV(self, desde, hasta, coronacion):
        liMoves = self.dll.lc_moves().split(" ")
        for n, uno in enumerate(liMoves):
            if uno[1:3] == desde and uno[3:5] == hasta:
                if not coronacion or (coronacion and coronacion.upper() == uno[5].upper()):
                    return n
        return None

    def getMoves(self):
        liMoves = self.dll.lc_moves().split(" ")
        lista = []
        for mv in liMoves:
            if mv and mv != "-":
                lista.append(SAKmove(mv))
        return lista

    def numValidMoves(self, fen):
        self.setFEN(fen)
        return len(self.getMoves())

    def getExMoves(self):
        liMoves = self.dll.lc_exmoves().split("|")
        lista = []
        for mv in liMoves:
            if mv:
                lista.append(SAKmove(mv, True))
        return lista

    def searchExPV(self, desde, hasta, coronacion):
        liMoves = self.dll.lc_exmoves().split("|")
        for n, uno in enumerate(liMoves):
            if uno[1:3] == desde and uno[3:5] == hasta:
                if (not coronacion) or (coronacion.upper() == uno[5].upper()):
                    return n, uno
        return None, None

    def getPGN(self, desde, hasta, coronacion):
        pos = self.searchPV(desde, hasta, coronacion)
        if pos is None:
            return "?"
        resp = self.dll.lc_toSAN(pos)
        return resp

    def moveExPV(self, desde, hasta, coronacion):
        pos, mv = self.searchExPV(desde, hasta, coronacion)
        if pos is not None:
            self.dll.lc_makemove(pos)
        return mv

    def pgn2pv(self, pgn):
        # Hace también el movimiento en el board
        return self.dll.lc_pgn2pv(str(pgn))

    def think(self, depth):
        return self.dll.lc_think(depth)

    def isCheck(self):
        return self.dll.lc_check()

    def multiPV(self, fen, depth):
        self.setFEN(fen)
        liMoves = self.dll.lc_moves().split(" ")
        lista = []
        for pos, mv in enumerate(liMoves):
            if mv and mv != "-":
                self.setFEN(fen)
                pos = self.searchPV(mv[1:3], mv[3:5], mv[5:])
                if pos is not None:
                    self.dll.lc_makemove(pos)
                    xmv = mv[1:]
                    resp = self.think(depth)
                    if resp:
                        bm, pt = resp.split("|")
                        pt = -int(pt)
                        lista.append((xmv, pt))
        lista = sorted(lista, key=lambda uno: -uno[1])
        return lista

sak = SAK()

