import atexit
import os
import sqlite3
import time
import random

import LCEngine4 as LCEngine

from Code import ControlPosicion
from Code import Partida
from Code import Util
from Code import VarGen

posA1 = LCEngine.posA1
a1Pos = LCEngine.a1Pos
pv2xpv = LCEngine.pv2xpv
xpv2pv = LCEngine.xpv2pv
xpv2pgn = LCEngine.xpv2pgn
PGNreader = LCEngine.PGNreader
setFen = LCEngine.setFen
makeMove = LCEngine.makeMove
getFen = LCEngine.getFen
getExMoves = LCEngine.getExMoves
fen2fenM2 = LCEngine.fen2fenM2
makePV = LCEngine.makePV
num2move = LCEngine.num2move
move2num = LCEngine.move2num

rots = ["Event", "Site", "Date", "Round", "White", "Black", "Result",
        "WhiteTitle", "BlackTitle", "WhiteElo", "BlackElo", "WhiteUSCF", "BlackUSCF", "WhiteNA", "BlackNA",
        "WhiteType", "BlackType", "EventDate", "EventSponsor", "ECO", "UTCTime", "UTCDate", "TimeControl",
        "SetUp", "FEN", "PlyCount",
        "Section", "Stage", "Board", "Opening", "Variation", "SubVariation", "NIC", "Time",
        "Termination", "Annotator", "Mode"
]
drots = {x.upper(): x for x in rots}
drots["PLIES"] = "PlyCount"


class TreeSTAT:
    def __init__(self, nomFichero, depth=None):
        self.nomFichero = nomFichero
        self.defaultDepth = 30
        self.iniFen = ControlPosicion.FEN_INICIAL
        self.hiniFen = self._fen2hash(self.iniFen)
        self._conexion = sqlite3.connect(self.nomFichero)

        self.depth, self.riniFen = self.checkTable(depth)

        self.fsum = self._sum  # called method needed to massive append

        atexit.register(self.close)

    def checkTable(self, depth):
        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(STATS)")
        if not cursor.fetchall():
            cursor.execute("PRAGMA page_size = 4096")
            cursor.execute("PRAGMA synchronous = NORMAL")
            if depth is None:
                depth = self.defaultDepth
            sql = "CREATE TABLE STATS( HASHFEN INT, W INT, B INT, D INT, O INT, RFATHER INT, XMOVE INT );"
            cursor.execute(sql)
            sql = "CREATE INDEX HASHFEN ON STATS( HASHFEN );"
            cursor.execute(sql)
            sql = "INSERT INTO STATS( HASHFEN, W, B, D, O, RFATHER, XMOVE ) VALUES( ?, ?, ?, ?, ?, ?, ? );"
            cursor.execute(sql, (self.hiniFen, 0, 0, 0, 0, 0, 0))
            riniFen = cursor.lastrowid

            sql = "CREATE TABLE CONFIG( KEY TEXT PRIMARY KEY, VALUE TEXT );"
            cursor.execute(sql)
            sql = "INSERT INTO CONFIG( KEY, VALUE ) VALUES( ?, ? );"
            cursor.execute(sql, ("DEPTH", str(depth)))

            self._conexion.commit()
        else:
            sql = "SELECT VALUE FROM CONFIG WHERE KEY= ?"
            cursor.execute(sql, ("DEPTH",))
            raw = cursor.fetchone()
            depth = int(raw[0])
            sql = "SELECT ROWID FROM STATS WHERE HASHFEN = ?"
            cursor.execute(sql, (self.hiniFen,))
            raw = cursor.fetchone()
            riniFen = raw[0]

        cursor.close()
        return depth, riniFen

    def close(self):
        if self._conexion:
            self._conexion.close()
            self._conexion = None

    def reset(self, depth=None):
        self.close()
        Util.borraFichero(self.nomFichero)
        self._conexion = sqlite3.connect(self.nomFichero)
        self.depth, self.riniFen = self.checkTable(depth)

    def _readRow(self, hfen, rfather, xmove):
        sql = "SELECT ROWID, W, B, D, O, RFATHER, XMOVE FROM STATS WHERE HASHFEN = ?"
        cursor = self._conexion.cursor()
        cursor.execute(sql, (hfen,))
        liRows = cursor.fetchall()
        if liRows:
            for row in liRows:
                RFATHER = row[5]
                if rfather == RFATHER:
                    alm = Util.Almacen()
                    alm.ROWID = row[0]
                    alm.W = row[1]
                    alm.B = row[2]
                    alm.D = row[3]
                    alm.O = row[4]
                    alm.RFATHER = RFATHER
                    alm.XMOVE = row[6]
                    return alm

        alm = Util.Almacen()
        alm.ROWID = None
        alm.W = 0
        alm.B = 0
        alm.D = 0
        alm.O = 0
        alm.RFATHER = rfather
        alm.XMOVE = xmove
        return alm

    def _readRowExt(self, rfather, hashFen, fen):
        sql = "SELECT ROWID, W, B, D, O, RFATHER, XMOVE FROM STATS WHERE HASHFEN = ?"
        cursor = self._conexion.cursor()
        cursor.execute(sql, (hashFen,))
        liRows = cursor.fetchall()
        basefenM2 = fen2fenM2(fen)

        def history(xmove, rfather):
            li = [xmove,]
            sql = "SELECT RFATHER, XMOVE FROM STATS WHERE ROWID = ?"
            while rfather != self.riniFen:
                cursor.execute(sql, (rfather,))
                resp = cursor.fetchone()
                if resp is None:
                    break
                rfather, xmove = resp
                li.insert(0, xmove)
            li = [num2move(x) for x in li]
            pv = " ".join(li)
            setFen(self.iniFen)
            makePV(pv)
            fenM2 = fen2fenM2(getFen())
            return pv, fenM2

        liAlms = []
        alm_base = None
        tW = tB = tD = tO = 0
        if liRows:
            li = []
            for row in liRows:
                alm = Util.Almacen()
                alm.ROWID, alm.W, alm.B, alm.D, alm.O, alm.RFATHER, alm.XMOVE = row
                alm.PV, alm.FENM2 = history(alm.XMOVE, alm.RFATHER)
                if alm.RFATHER == rfather:
                    alm_base = alm
                li.append(alm)
            for alm in li:
                if alm.FENM2 == basefenM2:
                    liAlms.append(alm)
                    tW += alm.W
                    tB += alm.B
                    tD += alm.D
                    tO += alm.O
        alm = Util.Almacen()
        alm.W = tW
        alm.B = tB
        alm.D = tD
        alm.O = tO
        alm.BASE = alm_base
        alm.LIALMS = liAlms
        cursor.close()
        return alm

    def _writeRow(self, hashFen, alm):
        rowid = alm.ROWID
        cursor = self._conexion.cursor()
        if rowid is None:
            sql = "INSERT INTO STATS( HASHFEN, W, B, D, O, RFATHER, XMOVE ) VALUES( ?, ?, ?, ?, ?, ?, ? )"
            cursor.execute(sql, (hashFen, alm.W, alm.B, alm.D, alm.O, alm.RFATHER, alm.XMOVE))
            rowid = cursor.lastrowid
        else:
            sql = "UPDATE STATS SET W=?, B=?, D=?, O=? WHERE ROWID=?"
            cursor.execute(sql, (alm.W, alm.B, alm.D, alm.O, rowid))
        cursor.close()
        return rowid

    def commit(self):
        self._conexion.commit()

    def _fen2hash(self, fen):
        return hash(fen2fenM2(fen))

    def _sum(self, hfen, rfather, xmove, w, b, d, o, tdepth ):
        alm = self._readRow(hfen, rfather, xmove)
        if w:
            alm.W += w
        elif b:
            alm.B += b
        elif d:
            alm.D += d
        else:
            alm.O += o
        return self._writeRow(hfen, alm)

    def append(self, pv, result, r=+1, siCommit=False):
        w = b = d = o = 0
        if result == "1-0":
            w += r
        elif result == "0-1":
            b += r
        elif result == "1/2-1/2":
            d += r
        else:
            o += r

        self.fsum(self.hiniFen, 0, 0, w, b, d, o, 0)
        rfather = self.riniFen
        setFen(self.iniFen)
        liPV = pv.split(" ")
        for depth, move in enumerate(liPV):
            if depth >= self.depth:
                break
            if makeMove(move):
                fen = getFen()
                hfen = self._fen2hash(fen)
                rfather = self.fsum(hfen, rfather, move2num(move), w, b, d, o, depth)

    def append_fen(self, pv, result, liFens):
        w = b = d = o = 0
        if result == "1-0":
            w += 1
        elif result == "0-1":
            b += 1
        elif result == "1/2-1/2":
            d += 1
        else:
            o += 1

        self.fsum(self.hiniFen, 0, 0, w, b, d, o, 0)
        rfather = self.riniFen
        liPV = pv.split(" ")
        for depth, move in enumerate(liPV):
            if depth >= self.depth:
                break
            hfen = hash(liFens[depth])
            rfather = self.fsum(hfen, rfather, move2num(move), w, b, d, o, depth)

    def massive_append_set(self, start):
        if start:
            self.massive = {}
            self.fsum = self.massive_sum
        else:
            self.fsum = self._sum
            for (hfen, rfather), alm in self.massive.iteritems():
                self._writeRow(hfen, alm)
            self.massive = {}

    def massive_sum(self, hfen, rfather, xmove, w, b, d, o, tdepth ):
        if tdepth > 10:
            return self._sum(hfen, rfather, xmove, w, b, d, o, tdepth )

        alm = self.massive.get((hfen, rfather))
        if not alm:
            alm = self._readRow(hfen, rfather, xmove)
        if w:
            alm.W += w
        elif b:
            alm.B += b
        elif d:
            alm.D += d
        else:
            alm.O += o
        if not alm.ROWID:
            alm.ROWID = self._writeRow(hfen, alm)
        self.massive[(hfen, rfather)] = alm
        return alm.ROWID

    def appendColor(self, pv, result, siWhite, r=+1):
        w = b = d = o = 0
        if result == "1-0":
            w += r
        elif result == "0-1":
            b += r
        elif result == "1/2-1/2":
            d += r
        else:
            o += r

        self.fsum(self.hiniFen, 0, 0, w, b, d, o, 0)
        rfather = self.riniFen
        setFen(self.iniFen)
        liPV = pv.split(" ")
        for depth, move in enumerate(liPV):
            if depth >= self.depth:
                break
            if makeMove(move):
                fen = getFen()
                hfen = self._fen2hash(fen)
                if (depth % 2 == 0 and siWhite) or (depth % 2 == 1 and not siWhite):
                    rfather = self.fsum(hfen, rfather, move2num(move), w, b, d, o, depth)
                else:
                    rfather = self.fsum(hfen, rfather, move2num(move), 0, 0, 0, r, depth)

    def root(self):
        return self._readRow(self.hiniFen, 0, 0)

    def rootGames(self):
        alm = self.root()
        return alm.W + alm.B + alm.D + alm.O

    def children(self, pvBase, allmoves=True):
        fen_base = makePV(pvBase)
        li = getExMoves()
        liResp = []
        rfather = self.riniFen
        for n, mv in enumerate(li):
            setFen(fen_base)
            move = mv.movimiento()
            makeMove(move)
            fen = getFen()
            hashFen = self._fen2hash(fen)
            alm = self._readRowExt(rfather, hashFen, fen)
            if not allmoves and (alm.W +alm.B+alm.D+alm.O) == 0:
                continue
            alm.move = move
            liResp.append(alm)
        return liResp

    def flistAllpvs(self, maxDepth, minGames, siWhite, siDraw, pvBase):
        fich = VarGen.configuracion.ficheroTemporal("tmp")
        f = open(fich, "wb")

        stFenM2 = set()

        def mejor(pvPrevio, depth, cp):
            if depth > maxDepth:
                return
            liChildren = self.children(pvPrevio)
            liAlmx = []
            totx = 0
            for alm in liChildren:
                t = alm.W if siWhite else alm.B
                if siDraw:
                    t += alm.D
                if t >= minGames:
                    if t > totx:
                        liAlmx = [alm]
                        totx = t
                    elif t == totx:
                        liAlmx.append(alm)
            if totx:
                for alm in liAlmx:
                    wm = alm.move
                    pv = (pvPrevio + " " + wm).strip()
                    cpN = cp.copia()
                    cpN.moverPV(wm)
                    f.write("%s|%s|%s\n" % (pv2xpv(pv), wm, cpN.fen()))
                    todos(pv, depth + 1, cpN)

        def todos(pvPrevio, depth, cp):
            if depth > maxDepth:
                return
            if depth > maxDepth:
                return
            liChildren = self.children(pvPrevio)
            if not liChildren:
                return
            for alm in liChildren:
                games = alm.B + alm.W + alm.D
                if games >= minGames:
                    wm = alm.move
                    pv = (pvPrevio + " " + alm.move).strip()
                    cpN = cp.copia()
                    cpN.moverPV(wm)
                    f.write("%s|%s|%s\n" % (pv2xpv(pv), wm, cpN.fen()))
                    fm2 = cpN.fenM2()
                    if fm2 not in stFenM2:  # Para que no se repitan los movimientos de los transpositions
                        stFenM2.add(fm2)
                        mejor(pv, depth + 1, cpN)

        def inicia():
            cp = ControlPosicion.ControlPosicion()
            cp.posInicial()
            pvActual = ""
            for pv in pvBase.split(" "):
                cp.moverPV(pv)
                pvActual += " " + pv
                f.write("%s|%s|%s\n" % (pv2xpv(pvActual.strip()), pv, cp.fen()))
            return cp

        cp = inicia()

        if siWhite and cp.siBlancas:
            mejor(pvBase, 0, cp)
        else:
            todos(pvBase, 0, cp)
        f.close()
        return fich

    def getSummary(self, pvBase, dicAnalisis, siFigurinesPGN, allmoves=True):
        liMoves = []
        siBlancas = pvBase.count(" ") % 2 == 1 if pvBase else True

        liChildren = self.children(pvBase, allmoves)

        tt = 0

        lipvmove = []
        for alm in liChildren:
            w, d, b, o = alm.W, alm.D, alm.B, alm.O
            t = w + d + b + o
            # if t == 0:
            # continue

            dic = {}
            pvmove = alm.move
            pv = pvBase + " " + pvmove
            pv = pv.strip()
            lipvmove.append(pvmove)
            dic["numero"] = ""
            dic["pvmove"] = pvmove
            dic["pv"] = pv
            dic["analisis"] = dicAnalisis.get(pvmove, None)
            dic["games"] = t
            tt += t
            dic["white"] = w
            dic["draw"] = d
            dic["black"] = b
            dic["other"] = o
            dic["pwhite"] = w * 100.0 / t if t else 0.0
            dic["pdraw"] = d * 100.0 / t if t else 0.0
            dic["pblack"] = b * 100.0 / t if t else 0.0
            dic["pother"] = o * 100.0 / t if t else 0.0

            dic["alm"] = alm

            liMoves.append(dic)

        if allmoves:
            for pvmove in dicAnalisis:
                if pvmove not in lipvmove:
                    dic = {}
                    pv = pvBase + " " + pvmove
                    pv = pv.strip()
                    dic["pvmove"] = pvmove
                    dic["pv"] = pv
                    dic["analisis"] = dicAnalisis[pvmove]
                    dic["games"] = 0
                    dic["white"] = 0
                    dic["draw"] = 0
                    dic["black"] = 0
                    dic["other"] = 0
                    dic["pwhite"] = 0.00
                    dic["pdraw"] = 0.00
                    dic["pblack"] = 0.00
                    dic["pother"] = 0.00
                    dic["alm"] = None

                    liMoves.append(dic)

        liMoves = sorted(liMoves, key=lambda dic: -dic["games"])

        tg = w = d = l = 0
        for dic in liMoves:
            dic["pgames"] = dic["games"] * 100.0 / tt if tt else 0.0
            dic["pdrawwhite"] = dic["pwhite"] + dic["pdraw"]
            dic["pdrawblack"] = dic["pblack"] + dic["pdraw"]
            if siBlancas:
                dic["win"] = dic["white"]
                dic["lost"] = dic["black"]
                dic["pwin"] = dic["pwhite"]
                dic["plost"] = dic["pblack"]
                dic["pdrawwin"] = dic["pdrawwhite"]
                dic["pdrawlost"] = dic["pdrawblack"]
            else:
                dic["lost"] = dic["white"]
                dic["win"] = dic["black"]
                dic["pdrawlost"] = dic["pdrawwhite"]
                dic["pdrawwin"] = dic["pdrawblack"]
                dic["plost"] = dic["pwhite"]
                dic["pwin"] = dic["pblack"]

            g = dic["games"]
            tg += g
            w += dic["win"]
            l += dic["lost"]
            d += dic["draw"]

            pvmove = dic["pvmove"]
            if pvmove:
                pv = dic["pv"]
                p = Partida.Partida()
                p.leerPV(pv)
                if p.numJugadas():
                    jg = p.last_jg()
                    jugadas = jg.numMove()
                    pgn = jg.pgnFigurinesSP() if siFigurinesPGN else jg.pgnSP()
                    dic["move"] = pgn
                    dic["numero"] = "%d." % jugadas
                    if not jg.siBlancas():
                        # dic["move"] = pgnSP.lower()
                        dic["numero"] += ".."
                else:
                    dic["move"] = pvmove
                dic["partida"] = p

        dic = {}
        dic["games"] = tg
        dic["win"] = w
        dic["draw"] = d
        dic["lost"] = l
        dic["pwin"] = w*100.0/tg if tg else 0.0
        dic["pdraw"] = d*100.0/tg if tg else 0.0
        dic["plost"] = l*100.0/tg if tg else 0.0
        dic["pdrawwin"] = (w+d)*100.0/tg if tg else 0.0
        dic["pdrawlost"] = (l+d)*100.0/tg if tg else 0.0
        liMoves.append(dic)

        return liMoves


class DBgames:
    def __init__(self, nomFichero, with_dbSTAT=True):
        self.nomFichero = Util.dirRelativo(nomFichero)
        self.with_dbSTAT = with_dbSTAT
        self.liCamposBase = ["EVENT", "SITE", "DATE", "WHITE", "BLACK", "RESULT", "ECO", "WHITEELO", "BLACKELO", "PLIES"]
        self.liCamposWork = ["XPV", ]
        self.liCamposBLOB = ["PGN", ]

        self.liCamposRead = []
        self.liCamposRead.extend(self.liCamposWork)
        self.liCamposRead.extend(self.liCamposBase)

        self.liCamposAll = []
        self.liCamposAll.extend(self.liCamposWork)
        self.liCamposAll.extend(self.liCamposBase)
        self.liCamposAll.extend(self.liCamposBLOB)

        self._conexion = sqlite3.connect(self.nomFichero)
        self._conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        self._conexion.row_factory = sqlite3.Row
        self._cursor = self._conexion.cursor()
        self.tabla = "games"
        self.select = ",".join(self.liCamposRead)
        self.order = None
        self.filter = None

        self.cache = {}
        self.mincache = 2000
        self.maxcache = 4000

        self.controlInicial()

        self.liOrden = []

        if self.with_dbSTAT:
            self.dbSTAT = TreeSTAT(self.nomFichero + "_s1")
        else:
            self.dbSTAT = None

        self.liRowids = []

        atexit.register(self.close)

        self.rowidReader = Util.RowidReader(self.nomFichero, self.tabla)

    def reset_cache(self):
        self.cache = {}

    def guardaConfig(self, clave, valor):
        with Util.DicRaw(self.nomFichero, "config") as dbconf:
            dbconf[clave] = valor

    def recuperaConfig(self, clave, default=None):
        with Util.DicRaw(self.nomFichero, "config") as dbconf:
            return dbconf.get(clave, default)

    def guardaOrden(self):
        self.guardaConfig("LIORDEN", self.liOrden)

    def recuperaOrden(self):
        liOrden = self.recuperaConfig("LIORDEN")
        if liOrden:
            self.ponOrden(liOrden)
        return self.liOrden

    def addcache(self, rowid, reg):
        if len(self.cache) > self.maxcache:
            keys = self.cache.keys()
            rkeys = random.sample(keys, self.mincache)
            ncache = {}
            for k in rkeys:
                ncache[k] = self.cache[k]
            self.cache = ncache
        self.cache[rowid] = reg

    def intercambia(self, nfila, siUP):
        rowid = self.liRowids[nfila]
        if siUP:
            # buscamos el mayor, menor que rowid
            filOther = None
            rowidOther = -1
            for fil0, rowid0 in enumerate(self.liRowids):
                if rowid0 < rowid:
                    if rowid0 > rowidOther:
                        filOther = fil0
                        rowidOther = rowid0
            if filOther is None:
                return None
        else:
            # buscamos el menor, mayor que rowid
            filOther = None
            rowidOther = 999999999999
            for fil0, rowid0 in enumerate(self.liRowids):
                if rowid0 > rowid:
                    if rowid0 < rowidOther:
                        filOther = fil0
                        rowidOther = rowid0
            if filOther is None:
                return None
        # Hay que intercambiar rowid, con rowidOther
        selectAll = ",".join(self.liCamposAll)
        self._cursor.execute("SELECT %s FROM GAMES WHERE rowid =%d" % (selectAll, rowid))
        reg = self._cursor.fetchone()
        self._cursor.execute("SELECT %s FROM GAMES WHERE rowid =%d" % (selectAll, rowidOther))
        regOther = self._cursor.fetchone()

        # Problema con error por XPV unico cuando se intercambia, en RowidOther ponemos un xpv ficticio
        sql = "UPDATE GAMES SET XPV=? WHERE ROWID = %d" % rowidOther
        self._cursor.execute(sql, ("?????",))

        updateAll = ",".join(["%s=?"%campo for campo in self.liCamposAll])
        sql = "UPDATE GAMES SET %s" % updateAll + " WHERE ROWID = %d"

        self._cursor.execute(sql % rowid, regOther)
        self._cursor.execute(sql % rowidOther, reg)
        self._conexion.commit()

        self.addcache(rowid, regOther)
        self.addcache(rowidOther, reg)

        return filOther

    def getROWID(self, nfila):
        return self.liRowids[nfila]

    def field(self, nfila, name):
        rowid = self.liRowids[nfila]
        if rowid not in self.cache:
            self._cursor.execute("SELECT %s FROM %s WHERE rowid =%d" % (self.select, self.tabla, rowid))
            reg = self._cursor.fetchone()
            self.addcache(rowid, reg)
        return self.cache[rowid][name]

    def siFaltanRegistrosPorLeer(self):
        if not self.rowidReader:
            return False
        return not self.rowidReader.terminado()

    def filterPV(self, pv, condicionAdicional=None):
        condicion = ""
        if type(pv) == list:  # transpositions
            if pv:
                li = []
                for unpv in pv:
                    xpv = pv2xpv(unpv)
                    li.append('XPV GLOB "%s*"' % xpv)
                condicion = "(%s)" % (" OR ".join(li),)
        elif pv:
            xpv = pv2xpv(pv)
            condicion = 'XPV GLOB "%s*"' % xpv if xpv else ""
        if condicionAdicional:
            if condicion:
                condicion += " AND (%s)" % condicionAdicional
            else:
                condicion = condicionAdicional
        self.filter = condicion

        self.liRowids = []
        self.rowidReader.run(self.liRowids, condicion, self.order)

    def reccount(self):
        if not self.rowidReader:
            return 0
        n = self.rowidReader.reccount()
        # Si es cero y no ha terminado de leer, se le da tiempo para que devuelva algo
        while n == 0 and not self.rowidReader.terminado():
            time.sleep(0.05)
            n = self.rowidReader.reccount()
        return n

    def all_reccount(self):
        self.liRowids = []
        self.rowidReader.run(self.liRowids, None, None)
        while not self.rowidReader.terminado():
            time.sleep(0.1)
        return self.reccount()

    def controlInicial(self):
        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(%s)" % self.tabla)
        liCampos = cursor.fetchall()
        cursor.close()

        if not liCampos:
            cursor = self._conexion.cursor()
            cursor.execute("PRAGMA page_size = 4096")
            sql = "CREATE TABLE %s (" % self.tabla
            sql += "XPV VARCHAR NOT NULL PRIMARY KEY,"
            for field in self.liCamposBase:
                sql += "%s VARCHAR,"% field
            for field in self.liCamposBLOB:
                sql += "%s BLOB,"% field
            sql = sql[:-1] + " );"
            cursor.execute(sql)
            cursor.close()

    def close(self):
        if self._conexion:
            self._cursor.close()
            self._conexion.close()
            self._conexion = None
        if self.dbSTAT:
            self.dbSTAT.close()
            self.dbSTAT = None
        if self.rowidReader:
            self.rowidReader.stopnow()
            self.rowidReader = None

    def rotulo(self):
        rotulo = os.path.basename(self.nomFichero)[:-4]
        if rotulo.lower() == "initial database games":
            rotulo = _("Initial Database Games")
        return rotulo

    def depthStat(self):
        return self.dbSTAT.depth

    def flistAllpvs(self, maxDepth, minGames, siWhite, siDraw, pvBase):
        return self.dbSTAT.flistAllpvs(maxDepth, minGames, siWhite, siDraw, pvBase)

    def damePV(self, fila):
        xpv = self.field(fila, "XPV")
        return xpv2pv(xpv)

    def ponOrden(self, liOrden):
        li = []
        for campo, tipo in liOrden:
            if campo == "PLIES":
                campo =  "CAST(PLIES AS INTEGER)"
            li.append( "%s %s" % (campo, tipo))
        self.order = ",".join(li)
        self.liRowids = []
        self.rowidReader.run(self.liRowids, self.filter, self.order)
        self.liOrden = liOrden

    def dameOrden(self):
        return self.liOrden

    def borrarLista(self, lista):
        cSQL = "DELETE FROM %s WHERE rowid = ?" % self.tabla
        lista.sort(reverse=True)
        for recno in lista:
            pv = self.damePV(recno)
            result = self.field(recno, "RESULT")
            if self.with_dbSTAT:
                self.dbSTAT.append(pv, result, -1)
            self._cursor.execute(cSQL,(self.liRowids[recno],))
            del self.liRowids[recno]
        if self.with_dbSTAT:
            self.dbSTAT.commit()
        self._conexion.commit()

    def getSummary(self, pvBase, dicAnalisis, siFigurinesPGN, allmoves=True):
        return self.dbSTAT.getSummary(pvBase, dicAnalisis, siFigurinesPGN, allmoves)

    def recrearSTAT(self, dispatch, depth):
        self.dbSTAT.reset(depth)
        if self.filter:
            self.filterPV("")
        while self.siFaltanRegistrosPorLeer():
            time.sleep(0.1)
            dispatch(0, self.reccount())
        reccount = self.reccount()
        if reccount:
            self._cursor.execute("SELECT XPV, RESULT FROM %s" % self.tabla)
            recno = 0
            t = 0
            while dispatch(recno, reccount):
                chunk = random.randint(1500, 3500)
                li = self._cursor.fetchmany(chunk)
                if li:
                    for XPV, RESULT in li:
                        pv = xpv2pv(XPV)
                        self.dbSTAT.append(pv, RESULT)
                    nli = len(li)
                    if nli < chunk:
                        break
                    recno += nli
                else:
                    break
                t += 1
                if t % 5 == 0:
                    self.dbSTAT.commit()
            self.dbSTAT.commit()

    def leerPGNs(self, ficheros, dlTmp):
        erroneos = duplicados = importados = n = 0

        t1 = time.time()-0.7  # para que empiece enseguida

        if self.with_dbSTAT:
            self.dbSTAT.massive_append_set(True)

        def write_logs(fich, pgn):
            with open(fich, "ab") as ferr:
                ferr.write(pgn)
                ferr.write("\n")

        codec = Util.file_encoding(ficheros[0])
        sicodec = codec not in ("utf-8", "ascii")

        liRegs = []
        stRegs = set()
        nRegs = 0

        conexion = self._conexion
        cursor = self._cursor

        sql = "insert into games (XPV,EVENT,SITE,DATE,WHITE,BLACK,RESULT,ECO,WHITEELO,BLACKELO,PGN,PLIES) values (?,?,?,?,?,?,?,?,?,?,?,?);"
        liCabs = self.liCamposBase[:-1] # all except PLIES PGN, TAGS
        liCabs.append("PLYCOUNT")

        for fichero in ficheros:
            nomfichero = os.path.basename(fichero)
            fich_erroneos = os.path.join(VarGen.configuracion.carpetaTemporal(), nomfichero[:-3] + "errors.pgn")
            fich_duplicados = os.path.join(VarGen.configuracion.carpetaTemporal(), nomfichero[:-3] + "duplicates.pgn")
            dlTmp.pon_titulo(nomfichero)
            next_n = random.randint(100, 200)
            with LCEngine.PGNreader(fichero, self.depthStat()) as fpgn:
                for n, (pgn, pv, dCab, raw, liFens, dCablwr) in enumerate(fpgn, 1):
                    if not pv:
                        erroneos += 1
                        write_logs(fich_erroneos, pgn)
                    else:
                        fen = dCab.get("FEN", None)
                        if fen and fen != ControlPosicion.FEN_INICIAL:
                            erroneos += 1
                        else:
                            xpv = pv2xpv(pv)
                            if xpv in stRegs:
                                dup = True
                            else:
                                cursor.execute("SELECT COUNT(*) FROM games WHERE XPV = ?", (xpv,))
                                num = cursor.fetchone()[0]
                                dup = num > 0
                            if dup:
                                duplicados += 1
                                write_logs(fich_duplicados, pgn)
                            else:
                                stRegs.add(xpv)
                                if sicodec:
                                    for k, v in dCab.iteritems():
                                        dCab[k] = unicode(v, encoding=codec, errors="ignore")
                                    if pgn:
                                        pgn = unicode(pgn, encoding=codec, errors="ignore")

                                if raw: # si no tiene variantes ni comentarios, se graba solo las tags que faltan
                                    liRTags = [(dCablwr[k],v) for k, v in dCab.iteritems() if k not in liCabs] # k is always upper
                                    if liRTags:
                                        pgn = {}
                                        pgn["RTAGS"] = liRTags
                                    else:
                                        pgn = None

                                event = dCab.get("EVENT", "")
                                site = dCab.get("SITE", "")
                                date = dCab.get("DATE", "")
                                white = dCab.get("WHITE", "")
                                black = dCab.get("BLACK", "")
                                result = dCab.get("RESULT", "")
                                eco = dCab.get("ECO", "")
                                whiteelo = dCab.get("WHITEELO", "")
                                blackelo = dCab.get("BLACKELO", "")
                                plies = (pv.count(" ")+1) if pv else 0
                                if pgn:
                                    pgn = Util.var2blob(pgn)

                                reg = (xpv, event, site, date, white, black, result, eco, whiteelo, blackelo, pgn, plies)
                                if self.with_dbSTAT:
                                    self.dbSTAT.append_fen(pv, result, liFens)
                                liRegs.append(reg)
                                nRegs += 1
                                importados += 1
                                if nRegs == 10000:
                                    nRegs = 0
                                    cursor.executemany(sql, liRegs)
                                    liRegs = []
                                    stRegs = set()
                                    conexion.commit()
                                    if self.with_dbSTAT:
                                        self.dbSTAT.massive_append_set(False)
                                        self.dbSTAT.commit()
                                        self.dbSTAT.massive_append_set(True)
                    if n == next_n:
                        if time.time()-t1> 0.8:
                            if not dlTmp.actualiza(erroneos+duplicados+importados, erroneos, duplicados, importados):
                                break
                            t1 = time.time()
                        next_n = n + random.randint(100, 500)

        if liRegs:
            cursor.executemany(sql, liRegs)
            conexion.commit()
        dlTmp.actualiza(erroneos+duplicados+importados, erroneos, duplicados, importados)
        dlTmp.ponSaving()

        if self.with_dbSTAT:
            self.dbSTAT.massive_append_set(False)
            self.dbSTAT.commit()
        conexion.commit()
        dlTmp.ponContinuar()

    def appendDB(self, db, liRecnos, dlTmp):
        duplicados = importados = 0

        if self.with_dbSTAT:
            self.dbSTAT.massive_append_set(True)

        t1 = time.time() - 0.7  # para que empiece enseguida

        next_n = random.randint(100, 200)

        liRegs = []
        nRegs = 0

        conexion = self._conexion
        cursor = self._cursor

        sql = "insert into games (XPV,EVENT,SITE,DATE,WHITE,BLACK,RESULT,ECO,WHITEELO,BLACKELO,PGN,PLIES) values (?,?,?,?,?,?,?,?,?,?,?,?);"

        for pos, recno in enumerate(liRecnos):
            raw = db.leeAllRecno(recno)

            xpv = raw["XPV"]
            cursor.execute("SELECT COUNT(*) FROM games WHERE XPV = ?", (xpv,))
            num = cursor.fetchone()[0]
            dup = num > 0
            if dup:
                duplicados += 1
            else:
                pv = xpv2pv(xpv)
                reg = (xpv, raw["EVENT"], raw["SITE"], raw["DATE"], raw["WHITE"], raw["BLACK"], raw["RESULT"], raw["ECO"], raw["WHITEELO"],
                       raw["BLACKELO"], raw["PGN"], raw["PLIES"])
                if self.with_dbSTAT:
                    self.dbSTAT.append(pv, raw["RESULT"])
                liRegs.append(reg)
                nRegs += 1
                importados += 1
                if nRegs == 10000:
                    cursor.executemany(sql, liRegs)
                    liRegs = []
                    conexion.commit()
                    if self.with_dbSTAT:
                        self.dbSTAT.massive_append_set(False)
                        self.dbSTAT.commit()
                        self.dbSTAT.massive_append_set(True)

            if pos == next_n:
                if time.time() - t1 > 0.8:
                    if not dlTmp.actualiza(duplicados + importados, duplicados, importados):
                        break
                    t1 = time.time()
                next_n = pos + random.randint(100, 500)

        if liRegs:
            cursor.executemany(sql, liRegs)
            conexion.commit()

        dlTmp.actualiza(duplicados + importados, duplicados, importados)
        dlTmp.ponSaving()

        if self.with_dbSTAT:
            self.dbSTAT.massive_append_set(False)
            self.dbSTAT.commit()
        conexion.commit()

        dlTmp.ponContinuar()

    def leeAllRecno(self, recno):
        rowid = self.liRowids[recno]
        select = ",".join(self.liCamposAll)
        self._cursor.execute("SELECT %s FROM %s WHERE rowid =%d" % (select, self.tabla, rowid))
        return self._cursor.fetchone()

    def leeRegAllRecno(self, recno):
        raw = self.leeAllRecno(recno)
        alm = Util.Almacen()
        for campo in self.liCamposAll:
            setattr(alm, campo, raw[campo])
        return alm, raw

    def countData(self, filtro):
        sql = "SELECT COUNT(*) FROM %s"%self.tabla
        if self.filter:
            sql += " WHERE %s"%self.filter
            if filtro:
                sql += " AND %s" % filtro
        else:
            if filtro:
                sql += " WHERE %s"% filtro

        self._cursor.execute(sql)
        return self._cursor.fetchone()[0]

    def yieldData(self, liFields, filtro):
        select = ",".join(liFields)
        sql = "SELECT %s FROM %s"%(select, self.tabla)
        if self.filter:
            sql += " WHERE %s"%self.filter
            if filtro:
                sql += " AND %s" % filtro
        else:
            if filtro:
                sql += " WHERE %s"% filtro

        self._cursor.execute(sql)
        while True:
            raw = self._cursor.fetchone()
            if raw:
                alm = Util.Almacen()
                for campo in liFields:
                    setattr(alm, campo, raw[campo])
                yield alm
            else:
                return

    def players(self):
        sql = "SELECT DISTINCT WHITE FROM %s"%self.tabla
        self._cursor.execute(sql)
        listaw = [raw[0] for raw in self._cursor.fetchall()]

        sql = "SELECT DISTINCT BLACK FROM %s" % self.tabla
        self._cursor.execute(sql)
        listab = [raw[0] for raw in self._cursor.fetchall()]

        listaw.extend(listab)

        lista = list(set(listaw))
        lista.sort()
        return lista

    def leePartidaRecno(self, recno):
        raw = self.leeAllRecno(recno)
        return self.leePartidaRaw(raw)

    def leePartidaRaw(self, raw):
        p = Partida.PartidaCompleta()
        xpgn = raw["PGN"]
        rtags = None
        if xpgn:
            xpgn = Util.blob2var(xpgn)
            if type(xpgn) in (str, unicode):  # Version -9
                p.readPGN(xpgn)
                return p
            if "RTAGS" in xpgn:
                rtags = xpgn["RTAGS"]
            else:
                p.restore(xpgn["FULLGAME"])
                return p

        p.leerPV(xpv2pv(raw["XPV"]))
        rots = ["Event", "Site", "Date", "Round", "White", "Black", "Result",
                "WhiteTitle", "BlackTitle", "WhiteElo", "BlackElo", "WhiteUSCF", "BlackUSCF", "WhiteNA", "BlackNA",
                "WhiteType", "BlackType", "EventDate", "EventSponsor", "ECO", "UTCTime", "UTCDate", "TimeControl",
                "SetUp", "FEN", "PlyCount"]
        drots = {x.upper():x for x in rots}
        drots["PLIES"] = "PlyCount"

        litags = []
        for field in self.liCamposBase:
             v = raw[field]
             if v:
                 litags.append((drots.get(field, field), v if type(v) == unicode else str(v)))
        if rtags:
            litags.extend(rtags)

        p.setTags(litags)
        p.asignaApertura()
        return p

    def leePGNRecno(self, recno):
        raw = self.leeAllRecno(recno)
        xpgn = raw["PGN"]
        result = raw["RESULT"]
        rtags = None
        if xpgn:
            xpgn = Util.blob2var(xpgn)
            if type(xpgn) in (str, unicode):
                return xpgn, result
            if "RTAGS" in xpgn:
                rtags = xpgn["RTAGS"]
            else:
                p = Partida.PartidaCompleta()
                p.restore(xpgn["FULLGAME"])
                return p.pgn(), result
        pgn = xpv2pgn(raw["XPV"])
        litags = []
        st = set()
        for field in self.liCamposBase:
            v = raw[field]
            if v:
                if field not in st:
                    litags.append('[%s "%s"]' % (drots.get(field, field), Util.primeraMayuscula(str(v))))
                    st.add(field)

        if rtags:
            for k, v in rtags:
                k = drots.get(k, Util.primeraMayuscula(k))
                if k not in st:
                    litags.append('[%s "%s"]' % (k, v))
                    st.add(k)

        tags = "\n".join(litags)
        return "%s\n\n%s\n" % (tags, pgn), result

    def blankPartida(self):
        hoy = Util.hoy()
        liTags = [["Date", "%d.%02d.%02d" % (hoy.year, hoy.month, hoy.day)],]
        return Partida.PartidaCompleta(liTags=liTags)

    def modifica(self, recno, partidaCompleta):
        reg_ant = self.leeAllRecno(recno)
        resAnt = reg_ant["RESULT"]

        pgn = {}
        pgn["FULLGAME"] = partidaCompleta.save()
        xpgn = Util.var2blob(pgn)

        dTags = {}
        for key, value in partidaCompleta.liTags:
            dTags[key.upper()] = value
        dTags["PLIES"] = partidaCompleta.numJugadas()
        if dTags["PLIES"] == 0:
            return True

        liFields = []
        liData = []
        for field in self.liCamposBase:
            if reg_ant[field] != dTags.get(field):
                liFields.append("%s=?"%field)
                liData.append(dTags.get(field))
        if xpgn != reg_ant["PGN"]:
            liFields.append("PGN=?")
            liData.append(xpgn)

        pvNue = partidaCompleta.pv()
        xpv = pv2xpv(pvNue)
        if xpv != reg_ant["XPV"]:
            self._cursor.execute("SELECT COUNT(*) FROM games WHERE XPV = ?", (xpv,))
            num = self._cursor.fetchone()[0]
            if num > 0:
                return False
            liFields.append("XPV=?")
            liData.append(xpv)

        rowid = self.liRowids[recno]
        if len(liFields) == 0:
            return True
        fields = ",".join(liFields)
        sql = "UPDATE games SET %s WHERE ROWID = %d" % (fields, rowid)
        self._cursor.execute(sql, liData)
        self._conexion.commit()
        pvAnt = xpv2pv(reg_ant["XPV"])
        resNue = dTags.get("RESULT", "*")
        if self.with_dbSTAT:
            self.dbSTAT.append(pvAnt, resAnt, -1)
            self.dbSTAT.append(pvNue, resNue, +1)
            self.dbSTAT.commit()

        del self.cache[rowid]

        return True

    def inserta(self, partidaCompleta):
        pv = partidaCompleta.pv()
        xpv = pv2xpv(pv)
        self._cursor.execute("SELECT COUNT(*) FROM games WHERE XPV = ?", (xpv,))
        raw = self._cursor.fetchone()
        num = raw[0]
        if num > 0:
            return False

        pgn = {}
        pgn["FULLGAME"] = partidaCompleta.save()
        xpgn = Util.var2blob(pgn)

        dTags = {}
        for key, value in partidaCompleta.liTags:
            dTags[key.upper()] = value
        dTags["PLIES"] = partidaCompleta.numJugadas()

        data = [xpv, ]
        for field in self.liCamposBase:
            data.append(dTags.get(field, None))
        data.append(xpgn)

        sql = "insert into games (XPV,EVENT,SITE,DATE,WHITE,BLACK,RESULT,ECO,WHITEELO,BLACKELO,PLIES,PGN) values (?,?,?,?,?,?,?,?,?,?,?,?);"
        self._cursor.execute(sql, data)
        self._conexion.commit()
        if self.with_dbSTAT:
            self.dbSTAT.append(pv, dTags.get("RESULT", "*"), +1)
            self.dbSTAT.commit()

        self.liRowids.append(self._cursor.lastrowid)

        return True

    def guardaPartidaRecno(self, recno, partidaCompleta):
        return self.inserta(partidaCompleta) if recno is None else self.modifica(recno, partidaCompleta)

    def massive_change_tags(self, li_tags_change, liRegistros, remove, overwrite, set_extend):
        dtag = Util.SymbolDict({tag:val for tag, val in li_tags_change})

        def work_tag(tag, alm):
            if tag in dtag:
                ant = getattr(alm, tag.upper())
                if (ant and overwrite) or not ant:
                    setattr(alm, tag.upper(), dtag[tag])

        if remove:
            remove = remove.upper()

        for recno in liRegistros:
            alm, raw = self.leeRegAllRecno(recno)

            work_tag("Event", alm)
            work_tag("Site", alm)
            work_tag("Date", alm)

            p = self.leePartidaRaw(raw)

            if remove:
                for n, (tag, val) in enumerate(p.liTags):
                    if tag.upper() == remove:
                        del p.liTags[n]
                        break
                setattr(alm, remove, "")

            st_tag_ant_upper = set()
            for n, (tag, val) in enumerate(p.liTags):
                if overwrite:
                    if tag in dtag:
                        p.liTags[n] = [tag, dtag[tag]]
                st_tag_ant_upper.add(tag.upper())
                setattr(alm, tag.upper(), p.liTags[n][1])

            for tag_new in dtag:
                if tag_new.upper() not in st_tag_ant_upper:
                    p.liTags.append([tag_new, dtag[tag_new]])
                    setattr(alm, tag_new.upper(), dtag[tag_new])

            if set_extend:
                p.set_extend_tags()
                if not alm.ECO:
                    alm.ECO = p.getTAG("ECO")

            rowid = self.liRowids[recno]
            pgn = {"FULLGAME": p.save()}
            xpgn = Util.var2blob(pgn)
            sql = "UPDATE GAMES SET EVENT=?, SITE=?, DATE=?, WHITE=?, BLACK=?, RESULT=?, " \
                  "ECO=?, WHITEELO=?, BLACKELO=?, PGN=? WHERE ROWID = %d" % rowid
            self._cursor.execute(sql, (alm.EVENT, alm.SITE, alm.DATE, alm.WHITE, alm.BLACK, alm.RESULT,
                                       alm.ECO, alm.WHITEELO, alm.BLACKELO, xpgn))

        self._conexion.commit()

        self.reset_cache()

    def pack(self):
        self._conexion.execute("VACUUM")
        if self.with_dbSTAT:
            self.dbSTAT._conexion.execute("VACUUM")

    def insert_pks(self, path_pks):
        f = open(path_pks, "rb")
        txt = f.read()
        f.close()
        dic = Util.txt2dic(txt)
        fen = dic.get("FEN")
        if fen:
            return _("This pks file is not a complete game")

        liTags = dic.get("liPGN", [])

        partidaCompleta = Partida.PartidaCompleta(liTags=liTags)
        partidaCompleta.recuperaDeTexto(dic["PARTIDA"])

        if not self.inserta(partidaCompleta):
            return _("This game already exists.")

        return None

    # def genPGNopen(self):
    #     conexion = sqlite3.connect(self.nomFichero)
    #     selectAll = ",".join(self.liCamposAll)
    #     sql = "SELECT %s FROM %s"%(selectAll, self.tabla)
    #     if self.filter:
    #         sql += " WHERE %s"%self.filter
    #     if self.order:
    #         sql += " ORDER BY %s"%self.order
    #     cursor = conexion.cursor()
    #     cursor.execute(sql)
    #     return conexion, cursor

    # def genPGN(self, cursor):
    #     dicCampoPos = {campo:pos for pos, campo in enumerate(self.liCamposAll)}
    #     posPGN = dicCampoPos["PGN"]
    #     posRESULT = dicCampoPos["RESULT"]
    #     posXPV = dicCampoPos["XPV"]
    #     while True:
    #         raw = cursor.fetchone()
    #         if not raw:
    #             break
    #         xpgn = raw[posPGN]
    #         rtags = None
    #         result = raw[posRESULT]
    #         if xpgn:
    #             xpgn = Util.blob2var(xpgn)
    #             if type(xpgn) in (str, unicode):
    #                 yield xpgn, result
    #             if "RTAGS" in xpgn:
    #                 rtags = xpgn["RTAGS"]
    #             else:
    #                 p = Partida.PartidaCompleta()
    #                 p.restore(xpgn["FULLGAME"])
    #                 yield p.pgn(), result
    #         pgn = xpv2pgn(raw[posXPV])
    #         litags = []
    #         for field in self.liCamposBase:
    #             v = raw[dicCampoPos[field]]
    #             if v:
    #                 litags.append('[%s "%s"]' % (drots.get(field, field), str(v)))
    #         if rtags:
    #             for k, v in rtags:
    #                  litags.append('[%s "%s"]' % (k, v))

    #         tags = "\n".join(litags)
    #         tags += "\n\n"

    #         pgn = "%s\n\n%s" % (tags, pgn)
    #         yield pgn, result

    # def genPGNclose(self, conexion, cursor):
    #     cursor.close()
    #     conexion.close()
