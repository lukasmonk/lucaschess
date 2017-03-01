import atexit
import os
import sqlite3
import time
import random

import LCEngine

from Code import Partida
from Code import Util
from Code import VarGen

posA1 = LCEngine.posA1
a1Pos = LCEngine.a1Pos
pv2xpv = LCEngine.pv2xpv
xpv2pv = LCEngine.xpv2pv
PGNreader = LCEngine.PGNreader
setFen = LCEngine.setFen
makeMove = LCEngine.makeMove
getFen = LCEngine.getFen
getExMoves = LCEngine.getExMoves
fen2fenM2 = LCEngine.fen2fenM2
makePV = LCEngine.makePV
num2move = LCEngine.num2move
move2num = LCEngine.move2num


class DBgamesFEN():
    def __init__(self, nomFichero):
        self.nomFichero = Util.dirRelativo(nomFichero)
        self.liCamposBase = ["FEN", "EVENT", "SITE", "DATE", "WHITE", "BLACK", "RESULT", "PLIES"]
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

        self.liRowids = []

        atexit.register(self.close)

        self.rowidReader = Util.RowidReader(self.nomFichero, self.tabla)

    def controlInicial(self):
        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(%s)" % self.tabla)
        liCampos = cursor.fetchall()
        cursor.close()

        if not liCampos:
            sql = "CREATE TABLE %s (" % self.tabla
            sql += "FEN VARCHAR NOT NULL PRIMARY KEY,"
            for field in self.liCamposBase:
                if field != "FEN":
                    sql += "%s VARCHAR," % field
            for field in self.liCamposWork:
                sql += "%s VARCHAR," % field
            for field in self.liCamposBLOB:
                sql += "%s BLOB," % field
            sql = sql[:-1] + " );"
            cursor = self._conexion.cursor()
            cursor.execute(sql)
            cursor.close()

    def close(self):
        if self._conexion:
            self._cursor.close()
            self._conexion.close()
            self._conexion = None
        if self.rowidReader:
            self.rowidReader.stopnow()
            self.rowidReader = None

    def ponOrden(self, liOrden):
        li = ["%s %s" % (campo, tipo) for campo, tipo in liOrden]
        self.order = ",".join(li)
        self.liRowids = []
        self.rowidReader.run(self.liRowids, self.filter, self.order)
        self.liOrden = liOrden

    def dameOrden(self):
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

    def reccount(self):
        if not self.rowidReader:
            return 0
        n = self.rowidReader.reccount()
        # Si es cero y no ha terminado de leer, se le da tiempo para que devuelva algo
        while n == 0 and not self.rowidReader.terminado():
            time.sleep(0.05)
            n = self.rowidReader.reccount()
        return n

    def setFilter(self, condicion):
        self.filter = condicion

        self.liRowids = []
        self.rowidReader.run(self.liRowids, condicion, self.order)

    def dameFEN_PV(self, fila):
        xpv = self.field(fila, "XPV")
        fen = self.field(fila, "FEN")
        return fen, xpv2pv(xpv)

    def all_reccount(self):
        self.liRowids = []
        self.rowidReader.run(self.liRowids, None, None)
        while not self.rowidReader.terminado():
            time.sleep(0.1)
        return self.reccount()

    def rotulo(self):
        rotulo = os.path.basename(self.nomFichero)[:-4]
        if rotulo == "Positions Database":
            rotulo = _("Positions Database")
        return rotulo

    def leerPGN(self, fichero, dlTmp):
        erroneos = duplicados = importados = n = 0

        t1 = time.time() - 0.7  # para que empiece enseguida

        next_n = random.randint(100, 200)

        codec = Util.file_encoding(fichero)
        sicodec = codec not in ("utf-8", "ascii")

        liRegs = []
        stRegs = set()
        nRegs = 0

        conexion = self._conexion
        cursor = self._cursor

        self.liCamposBase = ["FEN", "EVENT", "SITE", "DATE", "WHITE", "BLACK", "RESULT", "PLIES"]
        self.liCamposWork = ["XPV", ]
        self.liCamposBLOB = ["PGN", ]

        sql = "insert into games (FEN,EVENT,SITE,DATE,WHITE,BLACK,RESULT,XPV,PGN,PLIES) values (?,?,?,?,?,?,?,?,?,?);"
        liCabs = self.liCamposBase[:-1]  # all except PLIES PGN, TAGS
        liCabs.append("PLYCOUNT")

        with LCEngine.PGNreader(fichero, 0) as fpgn:
            for n, (pgn, pv, dCab, raw, liFens) in enumerate(fpgn, 1):
                if "FEN" not in dCab:
                    erroneos += 1
                else:
                    fen = dCab["FEN"]
                    if fen in stRegs:
                        dup = True
                    else:
                        cursor.execute("SELECT COUNT(*) FROM games WHERE FEN = ?", (fen,))
                        num = cursor.fetchone()[0]
                        dup = num > 0
                    if dup:
                        duplicados += 1
                    else:
                        stRegs.add(fen)
                        if sicodec:
                            for k, v in dCab.iteritems():
                                dCab[k] = unicode(v, encoding=codec, errors="ignore")
                            if pgn:
                                pgn = unicode(pgn, encoding=codec, errors="ignore")

                        event = dCab.get("EVENT", "")
                        site = dCab.get("SITE", "")
                        date = dCab.get("DATE", "")
                        white = dCab.get("WHITE", "")
                        black = dCab.get("BLACK", "")
                        result = dCab.get("RESULT", "")
                        plies = (pv.count(" ") + 1) if pv else 0
                        if pgn:
                            pgn = Util.var2blob(pgn)

                        xpv = pv2xpv(pv)

                        reg = (fen, event, site, date, white, black, result, xpv, pgn, plies)
                        liRegs.append(reg)
                        nRegs += 1
                        importados += 1
                        if nRegs == 10000:
                            cursor.executemany(sql, liRegs)
                            liRegs = []
                            stRegs = set()
                            conexion.commit()
                if n == next_n:
                    if time.time() - t1 > 0.8:
                        if not dlTmp.actualiza(erroneos + duplicados + importados, erroneos, duplicados, importados):
                            break
                        t1 = time.time()
                    next_n = n + random.randint(100, 500)

        if liRegs:
            cursor.executemany(sql, liRegs)
            conexion.commit()

        dlTmp.actualiza(erroneos + duplicados + importados, erroneos, duplicados, importados)
        dlTmp.ponSaving()

        conexion.commit()

        dlTmp.ponContinuar()

    def leeAllRecno(self, recno):
        rowid = self.liRowids[recno]
        select = ",".join(self.liCamposAll)
        self._cursor.execute("SELECT %s FROM %s WHERE rowid =%d" % (select, self.tabla, rowid))
        return self._cursor.fetchone()

    def leePartidaRecno(self, recno):
        raw = self.leeAllRecno(recno)

        p = Partida.PartidaCompleta(fen=raw["FEN"])
        xpgn = raw["PGN"]
        rtags = None
        if xpgn:
            xpgn = Util.blob2var(xpgn)
            if type(xpgn) == str:  # Version -9
                p.readPGN(VarGen.configuracion, xpgn)
                return p
            p.restore(xpgn["FULLGAME"])
            return p

        p.leerPV(xpv2pv(raw["XPV"]))
        rots = ["Event", "Site", "Date", "Round", "White", "Black", "Result",
                "WhiteTitle", "BlackTitle", "WhiteElo", "BlackElo", "WhiteUSCF", "BlackUSCF", "WhiteNA", "BlackNA",
                "WhiteType", "BlackType", "EventDate", "EventSponsor", "ECO", "UTCTime", "UTCDate", "TimeControl",
                "SetUp", "FEN", "PlyCount"]
        drots = {x.upper(): x for x in rots}
        drots["PLIES"] = "PlyCount"

        litags = []
        for field in self.liCamposBase:
            v = raw[field]
            if v:
                litags.append((drots.get(field, field), str(v)))
        if rtags:
            litags.extend(rtags)

        p.setTags(litags)
        return p

    def borrarLista(self, lista):
        cSQL = "DELETE FROM %s WHERE rowid = ?" % self.tabla
        lista.sort(reverse=True)
        for recno in lista:
            self._cursor.execute(cSQL,(self.liRowids[recno],))
            del self.liRowids[recno]
        self._conexion.commit()
