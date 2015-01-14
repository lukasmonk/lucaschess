# -*- coding: latin-1 -*-

import random

import Movimientos

dicPuntos = {"Q": 1000, "N": 300, "B": 320, "R": 500, "P": 100, "K": 30000,
             "MQ": 1.5, "MN": 1.3, "MB": 1.2, "MR": 1.3, "MP": 1.0, "MK": 1.6,
             "XQ": 1.5, "XN": 1.3, "XB": 1.2, "XR": 1.3, "XP": 1.0, "XK": 1.8,
             "P7": 20, "P6": 10, "P5": 5, "P4": 0.5, "P3": 0.2, "P2": 0,
             "CR": 100,
             "EN": -5.0}

MOVIMIENTO, CAPTURA, ALPASO, ENROQUE, CORONACIONM, CORONACIONC, DEFENSA = range(7)

class JMov:
    def __init__(self, siMB, pz, pzO, tipo, apos, npos):
        self.siMB = siMB
        self.tipo = tipo
        self.pz = pz
        self.pzO = pzO
        self.pzN = ""
        self.apos = apos
        self.npos = npos

    def conv(self, pos):
        f, c = Movimientos.posFC(pos)
        return chr(c + 97) + str(f + 1)

    def pv(self):
        resp = self.conv(self.apos) + self.conv(self.npos)
        if self.tipo in (CORONACIONM, CORONACIONC):
            resp += self.pzN.lower()

        return resp

    def pon_a1h8(self):
        self.a1h8 = self.conv(self.apos) + self.conv(self.npos)

    def a1(self):
        return self.conv(self.apos)

    def h8(self):
        return self.conv(self.npos)

    def coronacion(self):
        if self.tipo in (CORONACIONM, CORONACIONC):
            return self.pzN.lower()
        return ""

    def __str__(self):
        d = {MOVIMIENTO: "m", CAPTURA: "x", ALPASO: "ap", ENROQUE: "e", CORONACIONC: "cc", CORONACIONM: "cm",
             DEFENSA: "d"}
        # ~ return "%s%s:%s->%s %s"%(d[self.tipo],self.pz,str(Movimientos.posFC(self.apos)),str(Movimientos.posFC(self.npos)),self.pv() )
        return "%s de %s: %s(%d->%d) %s" % (
            d[self.tipo], self.pz, self.pv(), self.apos, self.npos, "" if self.siMB else "OB" )

class MotorInterno:
    def ponDM(self):
        self.dM = {"P": self.liMovsP, "R": self.liMovsR, "N": self.liMovsN, "B": self.liMovsB, "Q": self.liMovsQ,
                   "K": self.liMovsK}

    def ponFen(self, fen):

        self.ponDM()

        posicion, color, enroques, alPaso, mp, jg = fen.split(" ")
        self.alPaso = None if (alPaso == "-" or alPaso == "" ) else ((ord(alPaso[0]) - 97) + (int(alPaso[1]) - 1) * 8)

        self.siW = color == "w"
        self.jugadas = int(jg)
        self.movPeonCap = int(mp)

        self.siEnK = "K" in enroques
        self.siEnQ = "Q" in enroques

        self.siEnk = "k" in enroques
        self.siEnq = "q" in enroques

        self.tb = [None] * 64

        for fil, linea in enumerate(posicion.split("/")):
            col = 0
            for c in linea:
                if c.isdigit():
                    col += int(c)
                else:
                    siW = c.isupper()
                    post = ( 7 - fil ) * 8 + col
                    self.tb[post] = c
                    col += 1
                    if c in "Kk":
                        if siW:
                            posKW = post
                        else:
                            posKB = post

        self.posKW = posKW
        self.posKB = posKB
        self.posK = posKW if self.siW else posKB

        return self

    def clona(self):
        nv = MotorInterno()

        nv.ponDM()

        nv.alPaso = self.alPaso

        nv.siW = self.siW
        nv.jugadas = self.jugadas
        nv.movPeonCap = self.movPeonCap

        nv.siEnK = self.siEnK
        nv.siEnQ = self.siEnQ

        nv.siEnk = self.siEnk
        nv.siEnq = self.siEnq

        nv.tb = self.tb[:]

        nv.posKW = self.posKW
        nv.posKB = self.posKB
        nv.posK = self.posK

        return nv

    def copiaControlPosicion(self, cp):

        self.ponDM()

        alPaso = cp.alPaso
        self.alPaso = ((ord(alPaso[0]) - 97) + (int(alPaso[1]) - 1) * 8) if len(alPaso) == 2 else None

        self.siW = cp.siBlancas
        self.jugadas = cp.jugadas
        self.movPeonCap = cp.movPeonCap

        cp.controlEnroques()
        enroques = cp.enroques
        self.siEnK = "K" in enroques
        self.siEnQ = "Q" in enroques

        self.siEnk = "k" in enroques
        self.siEnq = "q" in enroques

        tb = self.tb = [None] * 64
        posA1 = Movimientos.posA1
        casillas = cp.casillas

        for pos in range(64):
            x = casillas[posA1(pos)]
            if x:
                tb[pos] = x
                if x == "K":
                    self.posKW = pos
                elif x == "k":
                    self.posKB = pos

        self.posK = self.posKW if self.siW else self.posKB

    def otrosAtaquesA1H8(self, pz, desde, hasta):
        self.calculaEstado()
        li = self.listaMovimientos()
        posDesde = Movimientos.a1Pos(desde)
        posHasta = Movimientos.a1Pos(hasta)
        lir = []
        for movim in li:
            if movim.pz == pz and posHasta == movim.npos and posDesde != movim.apos:
                lir.append(Movimientos.posA1(movim.apos))
        return lir

    def muestra(self, filas, columnas):
        resp = "   " + "+---" * 8 + "+" + "\n"
        for fil in range(8):
            resp += "   |"
            for col in range(8):
                t = (7 - fil) * 8 + col
                pz = self.tb[t]
                if pz is None:
                    pz = "·" if (col + fil) % 2 else " "
                resp += " " + pz + " |"
            resp += " " + filas[7 - fil] + "\n"
            resp += "   " + "+---" * 8 + "+" + "\n"
        resp += "    "
        for columna in columnas:
            resp += " " + columna + "  "

        return resp

    def muestraC(self):
        return self.muestra("12345678", "abcdefgh")

    def muestraH(self):
        return self.muestra("01234567", "01234567")

    def siEnJaque(self):
        return self.posAtacada(self.posK)

    def mb(self, pos):
        pz = self.tb[pos]
        if pz is None:
            return False
        if self.siW:
            return pz.isupper()
        else:
            return pz.islower()

    def ob(self, pos):
        pz = self.tb[pos]
        if pz is None:
            return False
        if self.siW:
            return pz.islower()
        else:
            return pz.isupper()

    def mov(self, siMB, tipo, pos, npos):
        return JMov(siMB, self.tb[pos], self.tb[npos], tipo, pos, npos)

    def valida(self, movim):
        if movim.siMB and movim.tipo != DEFENSA:
            self.comprueba(movim)
            sej = self.siEnJaque()
            self.compruebaAtras(movim)
            if sej:
                movim.tipo = DEFENSA

        li = self.estado[movim.npos]
        if li is None:
            self.estado[movim.npos] = li = []
        li.append(movim)

    def ponK(self, npos):
        self.posK = npos
        if self.siW:
            self.posKW = npos
        else:
            self.posKB = npos

    def comprueba(self, jmov):
        tipo = jmov.tipo
        self.tb[jmov.apos] = None
        if tipo in ( CORONACIONM, CORONACIONC ):
            self.tb[jmov.npos] = jmov.pzN
        else:
            self.tb[jmov.npos] = jmov.pz
            if tipo == ALPASO:
                self.tb[jmov.posPZO] = None
            elif tipo == ENROQUE:
                self.tb[jmov.aposR] = None
                self.tb[jmov.nposR] = jmov.pzR
            if jmov.pz in "Kk":
                self.ponK(jmov.npos)

    def compruebaAtras(self, jmov):
        tipo = jmov.tipo
        pz = jmov.pz
        self.tb[jmov.apos] = pz
        if pz in "Kk":
            self.ponK(jmov.apos)
        if tipo == MOVIMIENTO:
            self.tb[jmov.npos] = None
        elif tipo in (CAPTURA, CORONACIONM, CORONACIONC):
            self.tb[jmov.npos] = jmov.pzO
        elif tipo == ALPASO:
            self.tb[jmov.npos] = None
            self.tb[jmov.posPZO] = jmov.pzO
        elif tipo == ENROQUE:
            self.tb[jmov.npos] = None
            self.tb[jmov.nposR] = None
            self.tb[jmov.aposR] = jmov.pzR

    def juega(self, jmov):

        tipo = jmov.tipo
        npos = jmov.npos
        tb = self.tb
        tb[jmov.apos] = None
        pz = jmov.pz
        pzO = None
        if tipo == MOVIMIENTO:
            tb[npos] = pz
            if pz in "Pp":
                if abs(jmov.apos - npos) == 16:
                    self.alPaso = npos + (jmov.apos - npos) / 2
        elif tipo == CAPTURA:
            tb[npos] = pz
            pzO = jmov.pzO
        elif tipo == ALPASO:
            tb[npos] = pz
            tb[jmov.posPZO] = None
        elif tipo == ENROQUE:
            tb[npos] = pz
            tb[jmov.aposR] = None
            tb[jmov.nposR] = jmov.pzR
            if self.siW:
                self.siEnK = False
                self.siEnQ = False
            else:
                self.siEnk = False
                self.siEnq = False
        elif tipo == CORONACIONM:
            tb[npos] = jmov.pzN
        elif tipo == CORONACIONC:
            tb[npos] = jmov.pzN
            pzO = jmov.pzO

        siMiraEnroque = False
        if pz in "Kk":
            self.ponK(npos)
            siMiraEnroque = True
        elif pz in "Rr" or (jmov.pzO and jmov.pzO in "Rr"):
            siMiraEnroque = True
        if siMiraEnroque:
            if self.siEnK:
                if self.tb[7] != "R" or pz == "K":
                    self.siEnK = False
            if self.siEnQ:
                if self.tb[0] != "R" or pz == "K":
                    self.siEnQ = False
            if self.siEnk:
                if self.tb[63] != "r" or pz == "k":
                    self.siEnk = False
            if self.siEnq:
                if self.tb[56] != "r" or pz == "k":
                    self.siEnq = False

        if pz in "Pp" or ( pzO and pzO in "Pp" ):
            self.movPeonCap = 0
        else:
            self.movPeonCap += 1

        self.jugadas += 1
        self.siW = not self.siW

        self.posK = self.posKW if self.siW else self.posKB

    def enroque(self, pos, posR, dif):
        if self.posAtacada(pos):
            return

        n = pos + dif
        while n != posR:
            if self.tb[n] is not None:
                return
            n += dif

        for x in ( 0, dif, 2 * dif ):
            if self.posAtacada(pos + x):
                return

        m = self.mov(True, ENROQUE, pos, pos + dif * 2)
        m.pzR = "R" if self.siW else "r"
        m.aposR = posR
        m.nposR = pos + dif
        self.valida(m)

    def enroques(self):
        if self.siW:
            pos = 4
            siK = self.siEnK
            siQ = self.siEnQ
        else:
            pos = 60
            siK = self.siEnk
            siQ = self.siEnq

        if siQ:
            self.enroque(pos, pos - 4, -1)
        if siK:
            self.enroque(pos, pos + 3, +1)

    def liMovsQRB(self, pos, dic):
        siMB = self.mb(pos)
        for li in dic[pos]:
            for npos in li:
                if self.tb[npos] is None:
                    tp = MOVIMIENTO
                else:
                    if self.mb(npos):
                        tp = DEFENSA if siMB else CAPTURA
                    else:
                        tp = CAPTURA if siMB else DEFENSA

                m = self.mov(siMB, tp, pos, npos)
                self.valida(m)
                if tp in [CAPTURA, DEFENSA]:
                    break

    def liMovsQ(self, pos):
        self.liMovsQRB(pos, Movimientos.dicQ)

    def liMovsR(self, pos):
        self.liMovsQRB(pos, Movimientos.dicR)

    def liMovsB(self, pos):
        self.liMovsQRB(pos, Movimientos.dicB)

    def liMovsKN(self, pos, dic):
        siMB = self.mb(pos)

        for npos in dic[pos]:
            if self.tb[npos] is None:
                tp = MOVIMIENTO
            elif self.ob(npos):
                tp = CAPTURA if siMB else DEFENSA
            else:
                tp = DEFENSA if siMB else CAPTURA
            m = self.mov(siMB, tp, pos, npos)
            self.valida(m)

    def liMovsK(self, pos):
        self.liMovsKN(pos, Movimientos.dicK)

    def liMovsN(self, pos):
        self.liMovsKN(pos, Movimientos.dicN)

    def liMovsP(self, pos):
        siMB = self.mb(pos)

        if siMB:
            siW = self.siW
        else:
            siW = not self.siW

        dic = Movimientos.dicPW if siW else Movimientos.dicPB
        lM, lX = dic[pos]

        for npos in lM:
            if self.tb[npos] is None:
                if npos < 8 or npos > 55:
                    lpz = "QRBN" if siW else "qrbn"
                    for nv in lpz:
                        m = self.mov(siMB, CORONACIONM, pos, npos)
                        m.pzN = nv
                        self.valida(m)
                else:
                    # Si salta dos, que no haya nada en medio
                    d = npos - pos
                    if abs(d) == 16:
                        if self.tb[pos + (d / 2)]:
                            continue
                    m = self.mov(siMB, MOVIMIENTO, pos, npos)
                    self.valida(m)

        for npos in lX:
            if npos < 8 or npos > 55:
                lpz = "QRBN" if siW else "qrbn"
                for nv in lpz:
                    if self.ob(npos):
                        siOtro = siMB
                    elif self.mb(npos):
                        siOtro = not siMB
                    else:
                        continue
                    tp = CORONACIONC if siOtro else DEFENSA
                    m = self.mov(siMB, tp, pos, npos)
                    m.pzN = nv
                    self.valida(m)
            else:

                if self.ob(npos):
                    siOtro = siMB
                elif self.mb(npos):
                    siOtro = not siMB
                else:
                    continue
                tp = CAPTURA if siOtro else DEFENSA
                m = self.mov(siMB, tp, pos, npos)
                self.valida(m)

        if siMB and self.alPaso:
            for npos in lX:
                if npos == self.alPaso:
                    m = self.mov(siMB, ALPASO, pos, npos)
                    if self.siW:
                        otra = npos - 8
                        pzO = "p"
                    else:
                        otra = npos + 8
                        pzO = "P"
                    m.posPZO = otra
                    m.pzO = pzO
                    self.valida(m)
                    break

    def calculaEstado(self):
        self.estado = [None] * 64

        for pos in range(64):
            if self.tb[pos]:
                v = self.tb[pos]
                self.dM[v.upper()](pos)

        self.enroques()

    def listaMovimientos(self):
        liMovs = []
        for li in self.estado:
            if li:
                for m in li:
                    if m.siMB and m.tipo != DEFENSA:
                        liMovs.append(m)

        return liMovs

    def listaDefensas(self, siMB):
        liMovs = []
        for li in self.estado:
            if li:
                for m in li:
                    if m.siMB == siMB:
                        if m.tipo == DEFENSA:
                            liMovs.append(m)

        return liMovs

    def listaCapturas2(self, siMB, siTodos):
        liMovs = []

        for li in self.estado:
            if li:
                for m in li:
                    if m.siMB == siMB:
                        if m.tipo != DEFENSA:
                            otro = self.clona()
                            otro.juega(m)
                            otro.calculaEstado()
                            lc = otro.listaCapturas(not siMB)
                            if not siTodos:
                                ldef = otro.listaDefensas(siMB)
                            for om in lc:
                                if om.apos == m.npos:
                                    if siTodos:
                                        liMovs.append((m, om))
                                    else:  # sólo los no defendidos
                                        siDefen = False
                                        if om.pzO.lower() != "k":
                                            for om1 in ldef:
                                                if om1.npos == om.npos:
                                                    siDefen = True
                                                    break
                                        if not siDefen:
                                            liMovs.append((m, om))
        return liMovs

    def dameMovimientos(self):
        self.calculaEstado()
        return self.listaMovimientos()

    def listaMovimientosOB(self):
        liMovs = []
        for li in self.estado:
            if li:
                for m in li:
                    if not m.siMB and m.tipo != DEFENSA:
                        liMovs.append(m)

        return liMovs

    def listaCapturas(self, siMB):
        liMovs = []
        for li in self.estado:
            if li:
                for m in li:
                    if m.siMB == siMB:
                        if m.tipo in [CAPTURA, ALPASO, CORONACIONC]:
                            liMovs.append(m)

        return liMovs

    def posAtacada(self, pos):

        f, c = Movimientos.posFC(pos)

        FCpos = Movimientos.FCpos

        def mp(fi, ci, contras):
            for x in range(1, 8):
                ft = f + fi * x
                if ft < 0 or ft > 7:
                    break
                ct = c + ci * x
                if ct < 0 or ct > 7:
                    break
                pt = FCpos(ft, ct)
                pz = self.tb[pt]
                if pz:
                    return pz in contras

        contras = "rq" if self.siW else "RQ"
        for fi, ci in ( (+1, 0), (-1, 0), (0, +1), (0, -1) ):
            if mp(fi, ci, contras):
                return True

        contras = "qb" if self.siW else "QB"
        for fi, ci in ( (+1, +1), (-1, -1), (+1, -1), (-1, +1) ):
            if mp(fi, ci, contras):
                return True

        def mp1(contra, li):
            if self.siW:
                contra = contra.lower()
            for fi, ci in li:
                ft = f + fi
                if ft < 0 or ft > 7:
                    continue
                ct = c + ci
                if ct < 0 or ct > 7:
                    continue
                pt = FCpos(ft, ct)
                pz = self.tb[pt]
                if pz == contra:
                    return True

        # Caballo
        if mp1("N", ( (+1, +2), (+1, -2), (-1, +2), (-1, -2), (+2, +1), (+2, -1), (-2, +1), (-2, -1) )):
            return True

        # Peón
        if mp1("P", ( (+1, +1), (+1, -1) ) if self.siW else ( (-1, -1), (-1, +1) )):
            return True

        # Rey
        if mp1("K", ( (+1, +1), (+1, -1), (-1, +1), (-1, -1), (+1, 0), (-1, 0), (0, +1), (0, -1) )):
            return True

        return False

    def siTerminada(self):
        self.calculaEstado()
        liM = self.listaMovimientos()
        return len(liM) == 0

    def valorPiezas(self):
        ptB = ptW = 0.0
        for n in range(64):
            pz = self.tb[n]
            if pz:
                if pz.isupper():
                    ptW += dicPuntos[pz]
                    if pz == "P":
                        f = int(n / 8) + 1
                        ptW += dicPuntos["P%d" % f]
                else:
                    ptB += dicPuntos[pz.upper()]
                    if pz == "p":
                        f = int((63 - n) / 8) + 1
                        ptB += dicPuntos["P%d" % f]

        def calc(liM):
            pt = 0.0
            for jmov in liM:
                pz = jmov.pz
                tp = jmov.tipo
                if tp in (CAPTURA, ALPASO):
                    pt += dicPuntos["X" + jmov.pzO.upper()]
                elif tp == MOVIMIENTO:
                    pt += dicPuntos["M" + pz.upper()]
                elif tp in ( CORONACIONM, CORONACIONC):
                    pt += dicPuntos["CR"]
                elif tp == ENROQUE:
                    pt += dicPuntos["EN"]
            return pt

        liM = self.listaMovimientos()
        ptMB = calc(liM)
        liM = self.listaMovimientosOB()
        ptOB = calc(liM)
        if self.siW:
            ptW += ptMB
            ptB += ptOB
        else:
            ptB += ptMB
            ptW += ptOB

        return ptW, ptB

    def valores(self):
        dom = [0, 0, 0, 0, 0, 0, 0, 0,
               0, 10, 20, 30, 30, 20, 10, 0,
               0, 10, 20, 30, 30, 20, 10, 0,
               0, 10, 30, 50, 50, 30, 10, 0,
               0, 10, 30, 50, 50, 30, 10, 0,
               0, 10, 20, 30, 30, 20, 10, 0,
               0, 10, 20, 30, 30, 20, 10, 0,
               0, 0, 0, 0, 0, 0, 0, 0]
        lidomk = (
            (-3, -3, 10), (-3, -2, 10), (-3, -1, 10), (-3, 0, 10), (-3, +1, 10), (-3, +2, 10), (-3, +3, 10),
            (-2, -3, 10), (-2, -2, 20), (-2, -1, 20), (-2, 0, 20), (-2, +1, 20), (-2, +2, 20), (-2, +3, 10),
            (-1, -3, 10), (-1, -2, 20), (-1, -1, 80), (-1, 0, 80), (-1, +1, 80), (-1, +2, 20), (-1, +3, 10),
            ( 0, -3, 10), ( 0, -2, 20), ( 0, -1, 80), ( 0, +1, 80), ( 0, +2, 20), ( 0, +3, 10),
            (+1, -3, 10), (+1, -2, 20), (+1, -1, 80), (+1, 0, 80), (+1, +1, 80), (+1, +2, 20), (+1, +3, 10),
            (+2, -3, 10), (+2, -2, 20), (+2, -1, 20), (+2, 0, 20), (+2, +1, 20), (+2, +2, 20), (+2, +3, 10),
            (+3, -3, 10), (+3, -2, 10), (+3, -1, 10), (+3, 0, 10), (+3, +1, 10), (+3, +2, 10), (+3, +3, 10),
        )
        FCpos = Movimientos.FCpos

        def hazK(pos):
            f, c = Movimientos.posFC(pos)
            for fi, ci, pts in lidomk:
                ft = f + fi
                if ft < 0 or ft > 7:
                    continue
                ct = c + ci
                if ct < 0 or ct > 7:
                    continue
                pt = FCpos(ft, ct)
                if pts > dom[pt]:
                    dom[pt] = pts

        hazK(self.posKW)
        hazK(self.posKB)

        dominio = [0, 0]
        ataque = [0, 0]
        defensa = [0, 0]

        for k in range(64):
            li = self.estado[k]
            if li is None:
                continue
            siD = [False, False]
            for m in li:
                tipo = m.tipo
                wb = 0 if m.pz.isupper() else 1
                siD[wb] = True
                if m.pzO:
                    p = dicPuntos[m.pzO.upper()]
                    if tipo == DEFENSA and m.pzO.upper() != "K":
                        defensa[wb] += p
                    elif tipo in [CAPTURA, ALPASO, CORONACIONC]:
                        ataque[wb] += p

            for x in range(2):
                if siD[x]:
                    dominio[x] += dom[k]

        valorPiezas = self.valorPiezas()

        return valorPiezas, dominio, ataque, defensa

    def eligeMovimiento(self, pvalorPiezas, pdominio, pataque, pdefensa):
        self.calculaEstado()

        valorPiezas, dominio, ataque, defensa = self.valores()

        def calc(movim):
            mi = self.clona()
            mi.juega(movim)
            mi.calculaEstado()
            valorPiezas1, dominio1, ataque1, defensa1 = mi.valores()

            t = [0, 0]
            for x in range(2):
                t[x] += (valorPiezas1[x] - valorPiezas[x]) * pvalorPiezas
                t[x] += (dominio1[x] - dominio[x]) * pdominio * (-1) / 4
                t[x] += (ataque1[x] - ataque[x]) * pataque
                t[x] += (defensa1[x] - defensa[x]) * pdefensa / 400

            total = t[0] - t[1]
            if not self.siW:
                total = -total
            return total

        liMovs = self.listaMovimientos()

        dmax = -9999999999
        ele = None
        for n, m in enumerate(liMovs):
            dif = calc(m)
            if dif > dmax:
                ele = [n]
                dmax = dif
            elif dif == max:
                ele.append(n)
        if ele is None:
            return ""
        else:
            n = 0
            if len(ele) > 1:
                n = random.randint(0, len(ele) - 1)
            return liMovs[ele[n]].pv()

    def mp1(self, fen):
        self.ponFen(fen)

        self.calculaEstado()

        liMovs = self.listaMovimientos()

        nMovs = len(liMovs)
        if nMovs:
            ele = random.randint(0, nMovs - 1) if nMovs > 1 else 0
            return liMovs[ele].pv()
        else:
            return ""

    def mp2(self, fen):
        self.ponFen(fen)

        self.calculaEstado()

        liMovs = self.listaMovimientos()
        nMovs = len(liMovs)

        if nMovs:
            ele = random.randint(0, nMovs - 1) if nMovs > 1 else 0
            dif = 0
            for n, m in enumerate(liMovs):
                if m.tipo == CAPTURA:
                    ndif = dicPuntos[m.pzO.upper()]
                    if ndif >= dif:
                        dif = ndif
                        ele = n
                elif m.tipo == ENROQUE:
                    if dif == 0:
                        ele = n
            return liMovs[ele].pv()
        else:
            return ""

    def mp2_1(self, fen):  # no se deja capturar
        self.ponFen(fen)

        self.calculaEstado()

        liMovs = self.listaMovimientos()
        nMovs = len(liMovs)

        if nMovs:
            dic = {}
            for m in liMovs:
                nmi = MotorInterno()
                nmi.ponFen(fen)
                nmi.juega(m)
                nmi.calculaEstado()
                liMovs1 = nmi.listaCapturas(True)
                mx = 0
                for m1 in liMovs1:
                    pz = nmi.tb[m1.npos]
                    if pz:
                        pts = dicPuntos[pz.upper()]
                        if mx < pts:
                            mx = pts
                dic[m.pv()] = mx
            mn = 99999
            sel = ""
            for k, pts in dic.iteritems():
                if pts < mn:
                    sel = k
                    mn = pts
            return sel if sel else liMovs[0].pv()
        else:
            return ""

    def mp3(self, fen):
        self.ponFen(fen)
        return self.eligeMovimiento(1000, 1000, 0, 1000)

    def mp4(self, fen):

        if not hasattr(self, "dicMR"):
            self.creaMR()
        li = fen.split(" ")
        mf = li[0] + " " + li[1]
        if mf in self.dicMR:
            lim = self.dicMR[mf]
            nlim = len(lim)
            nr = 0
            if nlim > 1:
                nr = random.randint(0, nlim - 1)
            return lim[nr]

        self.ponFen(fen)
        return self.eligeMovimiento(1000, 1000, 1000, 1000)

    def mp5(self, fen):
        self.ponFen(fen)
        return self.eligeMovimiento(1000, 0, 5000, 0)

    def calcLento(self, nivel):
        self.calculaEstado()

        liMovs = self.listaMovimientos()

        dif = -99999999
        ele = -1
        if len(liMovs) == 0:
            return dif, ""
        for n, movim in enumerate(liMovs):
            mi = self.clona()
            mi.juega(movim)
            if nivel:
                ndif, num = mi.calcLento(nivel - 1)
            else:
                mi.calculaEstado()
                ndif = mi.valorPiezas()
            if ndif > dif:
                ele = n
                dif = ndif

        return dif, liMovs[ele].pv()

    def mp6(self, fen):
        self.ponFen(fen)
        dif, pv = self.calcLento(0)
        return pv

    def mp7(self, fen):
        self.ponFen(fen)
        dif, pv = self.calcLento(0)
        return pv

    def creaMR(self):
        txt = """# Mate guardamarinas
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - e2e4
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - e7e5
rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - g1f3
rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - b8c6
r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - d2d4
r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b KQkq - e5d4
r1bqkbnr/pppp1ppp/2n5/8/3pP3/5N2/PPP2PPP/RNBQKB1R w KQkq - c2c3
r1bqkbnr/pppp1ppp/2n5/8/3pP3/2P2N2/PP3PPP/RNBQKB1R b KQkq - d4c3
r1bqkbnr/pppp1ppp/2n5/8/4P3/2p2N2/PP3PPP/RNBQKB1R w KQkq - b1c3
r1bqkbnr/pppp1ppp/2n5/8/4P3/2N2N2/PP3PPP/R1BQKB1R b KQkq - d7d6
r1bqkbnr/ppp2ppp/2np4/8/4P3/2N2N2/PP3PPP/R1BQKB1R w KQkq - f1c4
r1bqkbnr/ppp2ppp/2np4/8/2B1P3/2N2N2/PP3PPP/R1BQK2R b KQkq - c8g4
r2qkbnr/ppp2ppp/2np4/8/2B1P1b1/2N2N2/PP3PPP/R1BQK2R w KQkq - e1g1
r2qkbnr/ppp2ppp/2np4/8/2B1P1b1/2N2N2/PP3PPP/R1BQ1RK1 b kq - c6e5
r2qkbnr/ppp2ppp/3p4/4n3/2B1P1b1/2N2N2/PP3PPP/R1BQ1RK1 w kq - f3e5
r2qkbnr/ppp2ppp/3p4/4N3/2B1P1b1/2N5/PP3PPP/R1BQ1RK1 b kq - g4d1
r2qkbnr/ppp2ppp/3p4/4N3/2B1P3/2N5/PP3PPP/R1Bb1RK1 w kq - c4f7
r2qkbnr/ppp2Bpp/3p4/4N3/4P3/2N5/PP3PPP/R1Bb1RK1 b kq - e8e7
r2q1bnr/ppp1kBpp/3p4/4N3/4P3/2N5/PP3PPP/R1Bb1RK1 w - - c3d5

# Shillit gambit
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - e2e4
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - e7e5
rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - g1f3
rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - b8c6
r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - f1c4
r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - c6d4
r1bqkbnr/pppp1ppp/8/4p3/2BnP3/5N2/PPPP1PPP/RNBQK2R w KQkq - f3e5
r1bqkbnr/pppp1ppp/8/4N3/2BnP3/8/PPPP1PPP/RNBQK2R b KQkq - d8g5
r1b1kbnr/pppp1ppp/8/4N1q1/2BnP3/8/PPPP1PPP/RNBQK2R w KQkq - e5f7
r1b1kbnr/pppp1Npp/8/6q1/2BnP3/8/PPPP1PPP/RNBQK2R b KQkq - g5g2
r1b1kbnr/pppp1Npp/8/8/2BnP3/8/PPPP1PqP/RNBQK2R w KQkq - h1f1
r1b1kbnr/pppp1Npp/8/8/2BnP3/8/PPPP1PqP/RNBQKR2 b Qkq - g2e4
r1b1kbnr/pppp1Npp/8/8/2Bnq3/8/PPPP1P1P/RNBQKR2 w Qkq - c4e2
r1b1kbnr/pppp1Npp/8/8/3nq3/8/PPPPBP1P/RNBQKR2 b Qkq - d4f3

# Pastor
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - e2e4
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - e7e5
rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - f1c4
rnbqkbnr/pppp1ppp/8/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR b KQkq - b8c6
r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - d1h5
r1bqkbnr/pppp1ppp/2n5/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - g8f6
r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - h5f7

# Legal
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - e2e4
rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - e7e5
rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - g1f3
rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - d7d6
rnbqkbnr/ppp2ppp/3p4/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - f1c4
rnbqkbnr/ppp2ppp/3p4/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - c8g4
rn1qkbnr/ppp2ppp/3p4/4p3/2B1P1b1/5N2/PPPP1PPP/RNBQK2R w KQkq - b1c3
rn1qkbnr/ppp2ppp/3p4/4p3/2B1P1b1/2N2N2/PPPP1PPP/R1BQK2R b KQkq - g7g6
rn1qkbnr/ppp2p1p/3p2p1/4p3/2B1P1b1/2N2N2/PPPP1PPP/R1BQK2R w KQkq - f3e5
rn1qkbnr/ppp2p1p/3p2p1/4N3/2B1P1b1/2N5/PPPP1PPP/R1BQK2R b KQkq - g4d1
rn1qkbnr/ppp2p1p/3p2p1/4N3/2B1P3/2N5/PPPP1PPP/R1BbK2R w KQkq - c4f7
rn1qkbnr/ppp2B1p/3p2p1/4N3/4P3/2N5/PPPP1PPP/R1BbK2R b KQkq - e8e7
rn1q1bnr/ppp1kB1p/3p2p1/4N3/4P3/2N5/PPPP1PPP/R1BbK2R w KQ - c3d5
"""
        self.dicMR = {}

        for linea in txt.split("\n"):
            linea = linea.strip()
            if linea and not linea.startswith("#"):
                li = linea.split(" ")
                t = li[0] + " " + li[1]
                c = li[4]
                if t not in self.dicMR:
                    self.dicMR[t] = []
                lim = self.dicMR[t]
                if c not in lim:
                    lim.append(c)

# mi = MotorInterno()
# mi.ponFen("3r2k1/1pp1qppp/p1n1p3/5r2/3P2P1/1PP1QN1P/P3RP2/4R1K1 b - - 0 20")
# p rint mi.muestraC()

# # li = mi.otrosAtaquesA1H8( "r", "f5", "d5" )

# p rint mi.mp2_1("3r2k1/1pp1qppp/p1n1p3/5r2/3P2P1/1PP1QN1P/P3RP2/4R1K1 b - - 0 20")
