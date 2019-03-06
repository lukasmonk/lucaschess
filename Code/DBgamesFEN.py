import atexit
import os
import sqlite3
import time
import random

import LCEngine4 as LCEngine

from Code import Partida
from Code import Util

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

    def reset_cache(self):
        self.cache = {}

    def controlInicial(self):
        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(GAMES)")
        liCampos = cursor.fetchall()
        cursor.close()

        if not liCampos:
            sql = "CREATE TABLE GAMES (FEN VARCHAR NOT NULL PRIMARY KEY,"
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

    def lee_rowids(self):
        sql = "SELECT ROWID FROM GAMES"
        if self.filter:
            sql += " WHERE %s" % self.filter
        if self.order:
            sql += " ORDER BY %s" % self.order
        self._cursor.execute(sql)
        self.liRowids = [ row[0] for row in self._cursor.fetchall()]

    def close(self):
        if self._conexion:
            self._cursor.close()
            self._conexion.close()
            self._conexion = None

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

        # Problema con error por FEN unico cuando se intercambia, en RowidOther ponemos un fen ficticio
        sql = "UPDATE GAMES SET FEN=? WHERE ROWID = %d" % rowidOther
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
            self._cursor.execute("SELECT %s FROM GAMES WHERE rowid =%d" % (self.select, rowid))
            reg = self._cursor.fetchone()
            self.addcache(rowid, reg)
        return self.cache[rowid][name]

    def reccount(self):
        return len(self.liRowids)

    def setFilter(self, condicion):
        self.filter = condicion
        self.lee_rowids()

    def dameFEN_PV(self, fila):
        xpv = self.field(fila, "XPV")
        fen = self.field(fila, "FEN")
        return fen, xpv2pv(xpv)

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
        nRegs = 0

        conexion = self._conexion
        cursor = self._cursor

        cursor.execute("SELECT FEN FROM GAMES")
        liRows = cursor.fetchall()
        stRegs = set(row[0] for row in liRows)

        sql = "insert into GAMES (FEN,EVENT,SITE,DATE,WHITE,BLACK,RESULT,XPV,PGN,PLIES) values (?,?,?,?,?,?,?,?,?,?);"
        liCabs = self.liCamposBase[:-1]  # all except PLIES PGN, TAGS
        liCabs.append("PLYCOUNT")

        with LCEngine.PGNreader(fichero, 0) as fpgn:
            for n, (pgn, pv, dCab, raw, liFens, dCablwr) in enumerate(fpgn, 1):
                if "FEN" not in dCab:
                    erroneos += 1
                else:
                    fen = dCab["FEN"]
                    if fen in stRegs:
                        dup = True
                    else:
                        cursor.execute("SELECT COUNT(*) FROM GAMES WHERE FEN = ?", (fen,))
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
        self.lee_rowids()

    def leeAllRecno(self, recno):
        rowid = self.liRowids[recno]
        select = ",".join(self.liCamposAll)
        self._cursor.execute("SELECT %s FROM GAMES WHERE rowid =%d" % (select, rowid))
        return self._cursor.fetchone()

    def leeRegAllRecno(self, recno):
        raw = self.leeAllRecno(recno)
        alm = Util.Almacen()
        for campo in self.liCamposAll:
            setattr(alm, campo, raw[campo])
        return alm, raw

    def leePartidaRecno(self, recno):
        raw = self.leeAllRecno(recno)
        return self.leePartidaRaw(raw)

    def leePartidaRaw(self, raw):
        p = Partida.PartidaCompleta(fen=raw["FEN"])
        xpgn = raw["PGN"]
        if xpgn:
            xpgn = Util.blob2var(xpgn)
            if type(xpgn) in (str, unicode):  # Version -9
                p.readPGN(xpgn)
                return p
            p.restore(xpgn["FULLGAME"])
            return p

        rtags = None

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
        cSQL = "DELETE FROM GAMES WHERE rowid = ?"
        lista.sort(reverse=True)
        for recno in lista:
            self._cursor.execute(cSQL,(self.liRowids[recno],))
            del self.liRowids[recno]
        self._conexion.commit()

    def modifica(self, recno, partidaCompleta):
        reg_ant = self.leeAllRecno(recno)

        pgn = {}
        pgn["FULLGAME"] = partidaCompleta.save()
        xpgn = Util.var2blob(pgn)

        dTags = {}
        for key, value in partidaCompleta.liTags:
            dTags[key.upper()] = value
        dTags["PLIES"] = partidaCompleta.numJugadas()

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
        liFields.append("XPV=?")
        liData.append(xpv)

        rowid = self.liRowids[recno]
        if len(liFields) == 0:
            return True
        fields = ",".join(liFields)
        sql = "UPDATE GAMES SET %s WHERE ROWID = %d" % (fields, rowid)
        self._cursor.execute(sql, liData)
        self._conexion.commit()

        del self.cache[rowid]

        return True

    def si_existe_fen(self, fen):
        li = fen.split(" ")
        busca = " ".join(li[:-2]) + "%"
        self._cursor.execute("SELECT COUNT(*) FROM GAMES WHERE FEN LIKE ?", (busca,))
        num = self._cursor.fetchone()[0]
        return num

    def inserta(self, partidaCompleta):
        fen = partidaCompleta.iniPosicion.fen()
        if self.si_existe_fen(fen):
            return False

        pgn = {}
        pgn["FULLGAME"] = partidaCompleta.save()
        xpgn = Util.var2blob(pgn)

        dTags = {}
        for key, value in partidaCompleta.liTags:
            dTags[key.upper()] = value
        dTags["PLIES"] = partidaCompleta.numJugadas()

        pv = partidaCompleta.pv()
        xpv = pv2xpv(pv)
        data = [xpv, xpgn]
        for field in self.liCamposBase:
            data.append(dTags.get(field, None))

        sql = "insert into GAMES (XPV,PGN,FEN,EVENT,SITE,DATE,WHITE,BLACK,RESULT,PLIES)" \
              " values (?,?,?,?,?,?,?,?,?,?);"
        self._cursor.execute(sql, data)
        self._conexion.commit()

        self.liRowids.append(self._cursor.lastrowid)

        return True

    def guardaConfig(self, clave, valor):
        with Util.DicRaw(self.nomFichero, "config") as dbconf:
            dbconf[clave] = valor

    def recuperaConfig(self, clave, default=None):
        with Util.DicRaw(self.nomFichero, "config") as dbconf:
            return dbconf.get(clave, default)

    def guardaPartidaRecno(self, recno, partidaCompleta):
        return self.inserta(partidaCompleta) if recno is None else self.modifica(recno, partidaCompleta)

    def appendDB(self, db, liRecnos, dlTmp):
        duplicados = importados = 0

        conexion = self._conexion
        cursor = self._cursor

        sql = "insert into GAMES (FEN,EVENT,SITE,DATE,WHITE,BLACK,XPV,PGN,PLIES) values (?,?,?,?,?,?,?,?,?);"

        for recno in liRecnos:
            raw = db.leeAllRecno(recno)

            fen = raw["FEN"]
            cursor.execute("SELECT COUNT(*) FROM GAMES WHERE FEN = ?", (fen,))
            num = cursor.fetchone()[0]
            dup = num > 0
            if dup:
                duplicados += 1
            else:
                reg = (fen, raw["EVENT"], raw["SITE"], raw["DATE"], raw["WHITE"], raw["BLACK"], raw["XPV"], raw["PGN"],
                       raw["PLIES"])
                importados += 1
                cursor.execute(sql, reg)

            if not dlTmp.actualiza(duplicados + importados, duplicados, importados):
                break

        dlTmp.actualiza(duplicados + importados, duplicados, importados)
        dlTmp.ponSaving()

        conexion.commit()

        dlTmp.ponContinuar()
        self.lee_rowids()

    def massive_change_tags(self, li_tags_change, liRegistros, remove, overwrite):
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

            rowid = self.liRowids[recno]
            pgn = {"FULLGAME": p.save()}
            xpgn = Util.var2blob(pgn)
            sql = "UPDATE GAMES SET EVENT=?, SITE=?, DATE=?, WHITE=?, BLACK=?, PGN=?, RESULT=? WHERE ROWID = %d" % rowid
            self._cursor.execute(sql, (alm.EVENT, alm.SITE, alm.DATE, alm.WHITE, alm.BLACK, xpgn, alm.RESULT))

        self._conexion.commit()

        self.reset_cache()

    def insert_pks(self, path_pks):
        f = open(path_pks, "rb")
        txt = f.read()
        f.close()
        dic = Util.txt2dic(txt)
        fen = dic.get("FEN")
        if not fen:
            return _("This pks file is a complete game")

        if self.si_existe_fen(fen):
            return _("This position already exists.")

        liTags = dic.get("liPGN", [])

        partidaCompleta = Partida.PartidaCompleta(fen=fen, liTags=liTags)
        partidaCompleta.recuperaDeTexto(dic["PARTIDA"])

        self.inserta(partidaCompleta)

        return None
