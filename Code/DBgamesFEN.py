# -*- coding: utf-8 -*-
import atexit
import os

import Code.VarGen as VarGen
import Code.Util as Util
from Code.Movimientos import pv2xpv
import Code.SQL.Base as Base
import Code.PGNreader as PGNreader
import Code.DBgames as DBgames

class DBgamesFEN(DBgames.DBgames):
    def __init__(self, nomFichero, segundosBuffer=0.8):
        self.nomFichero = nomFichero
        self.liCamposBase = ["FEN", "EVENT", "SITE", "DATE", "WHITE", "BLACK", "RESULT", "PLIES"]
        self.liCamposWork = ["XPV", "PGN"]

        self.segundosBuffer = segundosBuffer

        self.db = Base.DBBase(nomFichero)
        self.tabla = "games"
        if not self.db.existeTabla(self.tabla):
            self.creaTabla()

        liCampos = []
        liCampos.extend(self.liCamposWork)
        liCampos.extend(self.liCamposBase)
        self.dbf = self.db.dbfCache(self.tabla, ",".join(liCampos))
        self.liOrden = []
        atexit.register(self.close)

        self.miraReccountTotal()

    def miraReccountTotal(self):
        self.xReccountTotal = self.dbf.mxreccount()

    def reccountTotal(self):
        return self.xReccountTotal

    def rotulo(self):
        rotulo = os.path.basename(self.nomFichero)[:-4]
        if rotulo == "Positions Database":
            rotulo = _("Positions Database")
        return rotulo

    def listOthers(self):
        li = []
        nameActive = os.path.basename(self.nomFichero).lower()
        for f in Util.listdir(VarGen.configuracion.positionsFolder):
            low = f.lower()
            if low.endswith(".lcf"):
                if low != nameActive:
                    nombre = f[:-4]
                    if nombre == "Initial Database":
                        nombre = _F(nombre)
                    li.append(nombre)
        return li

    def close(self):
        if self.dbf:
            self.dbf.cerrar()
            self.dbf = None
        if self.db:
            self.db.cerrar()
            self.db = None

    def creaTabla(self):
        tb = Base.TablaBase(self.tabla)
        tb.nuevoCampo("FEN", "VARCHAR", notNull=True, primaryKey=True)
        tb.nuevoCampo("XPV", "VARCHAR")
        for campo in self.liCamposBase[1:]:
            tb.nuevoCampo(campo, "VARCHAR")
        tb.nuevoCampo("PGN", "BLOB")
        self.db.generarTabla(tb)

    def leer(self, condicion=""):
        self.dbf.ponCondicion(condicion)
        self.dbf.leerBuffer(segundos=self.segundosBuffer)
        self.dbf.gotop()

    def dameFEN_PV(self, fila):
        pv = self.damePV(fila)
        fen = getattr(self.dbf.reg, "FEN")
        return fen, pv

    def cambiarUno(self, recno, nuevoPGN, pvNue, dicS_PGN):
        siNuevo = recno is None

        resNue = dicS_PGN.get("Result", "*")

        br = self.dbf.baseRegistro()
        br.XPV = pv2xpv(pvNue)
        br.FEN = dicS_PGN["FEN"]
        br.EVENT = dicS_PGN.get("Event", "")
        br.SITE = dicS_PGN.get("Site", "")
        br.DATE = dicS_PGN.get("Date", "")
        br.WHITE = dicS_PGN.get("White", "")
        br.BLACK = dicS_PGN.get("Black", "")
        br.RESULT = resNue
        br.PLIES = "%3d" % (pvNue.strip().count(" ") + 1,)
        br.PGN = Util.var2blob(nuevoPGN)

        siRepetido = False
        if siNuevo:
            try:
                self.dbf.insertar(br, okCommit=True, okCursorClose=True)
                self.xReccountTotal += 1
            except:
                siRepetido = True
        else:
            try:
                self.dbf.modificarReg(recno, br)
            except:
                siRepetido = True
        return not siRepetido

    def borrarLista(self, lista):
        self.dbf.borrarLista(lista)
        self.xReccountTotal -= len(lista)
        self.dbf.leerBuffer(segundos=self.segundosBuffer)

    def append(self, pv, event, site, date, white, black, result, fen, pgn, okCommit):
        br = self.dbf.baseRegistro()
        br.XPV = pv2xpv(pv)
        br.FEN = fen
        br.EVENT = event
        br.SITE = site
        br.DATE = date
        br.WHITE = white
        br.BLACK = black
        br.RESULT = result
        br.PGN = Util.var2blob(pgn)
        br.PLIES = "%3d" % (pv.strip().count(" ") + 1,)
        siRepetido = False
        try:
            self.dbf.insertar(br, okCommit=okCommit, okCursorClose=okCommit)
            self.xReccountTotal += 1
        except:
            siRepetido = True
        return not siRepetido

    def leerPGN(self, fichero, dlTmp):

        erroneos = duplicados = importados = 0

        for n, g in enumerate(PGNreader.readGames(fichero)):

            if n % 100 == 0:
                if not dlTmp.actualiza(n + 1, erroneos, duplicados, importados):
                    break
            if g.erroneo:
                erroneos += 1
                continue
            pgn = g.pgn
            pv = g.pv()

            get = g.labels.get
            fen = get("FEN", None)
            if not fen:
                erroneos += 1
                continue
            event = get("EVENT", "")
            site = get("SITE", "")
            date = get("DATE", "")
            white = get("WHITE", "")
            black = get("BLACK", "")
            result = get("RESULT", "")

            if self.append(pv, event, site, date, white, black, result, fen, pgn, False):
                importados += 1
            else:
                duplicados += 1

        dlTmp.actualiza(n + 1, erroneos, duplicados, importados)

        dlTmp.ponSaving()
        self.dbf.commit()
        dlTmp.ponContinuar()

        return

    def blankPGN(self, fen):
        hoy = Util.hoy()
        # fen = ControlPosicion.FEN_INICIAL
        return '[Date "%d.%02d.%02d"]\n[FEN "%s"]\n\n*' % (hoy.year, hoy.month, hoy.day, fen)

