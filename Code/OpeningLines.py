import os
import sqlite3
import atexit

import LCEngine

from Code import Util
from Code import Partida
from Code import PGNreader
from Code import Books
from Code.QT import QTVarios
from Code.QT import QTUtil2


class ListaOpenings:
    def __init__(self, configuracion):
        self.folder = configuracion.folderOpenings
        self.fichero = os.path.join(self.folder, "index.pk")

        self.lista = Util.recuperaVar(self.fichero)
        if self.lista is None:
            self.lista = self.read()
            self.save()

    def __len__(self):
        return len(self.lista)

    def __getitem__(self, item):
        return self.lista[item] if self.lista and item < len(self.lista) else None

    def __delitem__(self, item):
        name, pv, tit = self.lista[item]
        del self.lista[item]
        os.remove(os.path.join(self.folder, name))
        self.save()

    def arriba(self, item):
        if item > 0:
            self.lista[item], self.lista[item - 1] = self.lista[item - 1], self.lista[item]
            self.save()
            return True
        else:
            return False

    def abajo(self, item):
        if item < (len(self.lista) - 1):
            self.lista[item], self.lista[item + 1] = self.lista[item + 1], self.lista[item]
            self.save()
            return True
        else:
            return False

    def refresh(self):
        dic = {}
        for entry in Util.listdir(self.folder):
            if entry.name.endswith(".opk"):
                op = Opening(entry.path)
                dic[entry.name] = (entry.name, op.basePV, op.title)
                op.close()
        liborrar = []
        st_esta = set()
        for n, (fichero, basePV, title) in enumerate(self.lista):
            if fichero in dic:
                self.lista[n] = dic[fichero]
                st_esta.add(fichero)
            else:
                liborrar.append(n)
        for x in range(len(liborrar)-1, -1, -1):
            del self.lista[x]

        for fichero in dic:
            if fichero not in st_esta:
                self.lista.append(dic[fichero])
        self.save()

    def read(self):
        li = []
        for entry in Util.listdir(self.folder):
            fichero = entry.name
            if fichero.endswith(".opk"):
                op = Opening(entry.path)
                li.append((fichero, op.basePV, op.title))
                op.close()
        return li

    def save(self):
        Util.guardaVar(self.fichero, self.lista)

    def select_filename(self, name):
        name = name.strip().replace(" ", "_")
        name = Util.validNomFichero(name)
        while "__" in name:
            name = name.replace("__", "_")
        plant = name + "%d.opk"
        file = name + ".opk"
        num = 0
        while os.path.isfile(os.path.join(self.folder, file)):
            num += 1
            file = plant % num
        return file

    def filepath(self, num):
        return os.path.join(self.folder, self.lista[num][0])

    def new(self, file, basepv, title):
        self.lista.append((file, basepv, title))
        op = Opening(self.filepath(len(self.lista)-1))
        op.setbasepv(basepv)
        op.settitle(title)
        op.close()
        self.save()

    def change_title(self, num, title):
        op = Opening(self.filepath(num))
        op.settitle(title)
        op.close()
        file, pv, ant_title = self.lista[num]
        self.lista[num] = (file, pv, title)

    def remove(self, lines):
        pass


class Opening:
    def __init__(self, nomFichero):
        self.nomFichero = nomFichero

        self._conexion = sqlite3.connect(nomFichero)
        atexit.register(self.close)

        self.cache = {}
        self.max_cache = 4000
        self.del_cache = 1000

        self.grupo = 0

        self.li_xpv = self.init_database()

        self.db_config = Util.DicSQL(nomFichero, tabla="CONFIG")
        self.db_fenvalues = Util.DicSQL(nomFichero, tabla="FENVALUES")
        self.basePV = self.getconfig("BASEPV", "")
        self.title = self.getconfig("TITLE", os.path.basename(nomFichero).split(".")[0])

    def getOtras(self, configuracion):
        liOp = ListaOpenings(configuracion)
        fich = os.path.basename(self.nomFichero)
        liOp = [(fichero, titulo) for fichero, pv, titulo in liOp.lista if fichero != fich and pv.startswith(self.basePV)]
        return liOp

    def getfenvalue(self, fenM2):
        resp = self.db_fenvalues[fenM2]
        return resp if resp else {}

    def setfenvalue(self, fenM2, dic):
        self.db_fenvalues[fenM2] = dic

    def getconfig(self, key, default=None):
        return self.db_config.get(key, default)

    def setconfig(self, key, value):
        self.db_config[key] = value

    def settitle(self, title):
        self.setconfig("TITLE", title)

    def gettitle(self):
        return self.getconfig("TITLE")

    def setbasepv(self, basepv):
        self.setconfig("BASEPV", basepv)

    def getpartidabase(self):
        base = self.getconfig("BASEPV")
        p = Partida.Partida()
        if base:
            p.leerPV(base)
        return p

    def add_cache(self, xpv, partida):
        if len(self.cache) >= self.max_cache:
            li = self.cache.keys()
            for n, xpv in enumerate(li):
                del self.cache[xpv]
                if n > self.del_cache:
                    break
        self.cache[xpv] = partida

    def init_database(self):
        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(LINES)")
        if not cursor.fetchall():
            sql = "CREATE TABLE LINES( XPV TEXT PRIMARY KEY, GRUPO INTEGER, LINE BLOB );"
            cursor.execute(sql)
            sql = "CREATE INDEX IDX_GRUPO ON LINES( GRUPO );"
            cursor.execute(sql)
            self._conexion.commit()
            li_xpv = []
        else:
            sql = "select XPV from LINES ORDER BY XPV"
            cursor.execute(sql)
            li_xpv = [ raw[0] for raw in cursor.fetchall()]
        return li_xpv

    def append(self, partida):
        xpv = LCEngine.pv2xpv(partida.pv())
        line_blob = partida.save2blob()
        sql = "INSERT INTO LINES( XPV, LINE ) VALUES( ?, ? )"
        cursor = self._conexion.cursor()
        cursor.execute(sql, (xpv, line_blob))
        cursor.close()
        self._conexion.commit()
        self.li_xpv.append(xpv)
        self.li_xpv.sort()
        self.add_cache(xpv, partida)

    def posPartida(self, partida):
        # return siNueva, numlinea, siAppend
        xpv_busca = LCEngine.pv2xpv(partida.pv())
        for n, xpv in enumerate(self.li_xpv):
            if xpv.startswith(xpv_busca):
                return False, n, False
            if xpv == xpv_busca[:-2]:
                return False, n, True
        return True, None, None

    def __contains__(self, xpv):
        return xpv in self.li_xpv

    def __setitem__(self, num, partida_nue):
        xpv_ant = self.li_xpv[num]
        xpv_nue = LCEngine.pv2xpv(partida_nue.pv())
        if xpv_nue != xpv_ant:
            if xpv_ant in self.cache:
                del self.cache[xpv_ant]
            self.li_xpv[num] = xpv_nue
            si_sort = False
            if num > 0:
                si_sort = xpv_nue < self.li_xpv[num-1]
            if not si_sort and num < len(self.li_xpv)-1:
                si_sort = xpv_nue > self.li_xpv[num+1]
            if si_sort:
                self.li_xpv.sort()
                num = self.li_xpv.index(xpv_nue)
        cursor = self._conexion.cursor()
        sql = "UPDATE LINES SET XPV=?, LINE=? WHERE XPV=?"
        cursor.execute(sql, (xpv_nue, partida_nue.save2blob(), xpv_ant))
        self._conexion.commit()
        self.add_cache(xpv_nue, partida_nue)
        return num

    def __getitem__(self, num):
        xpv = self.li_xpv[num]
        if xpv in self.cache:
            return self.cache[xpv]

        sql = "select LINE from LINES where XPV=?"
        cursor = self._conexion.cursor()
        cursor.execute(sql, (xpv,))
        blob = cursor.fetchone()[0]
        partida = Partida.Partida()
        partida.blob2restore(blob)
        self.add_cache(xpv, partida)
        return partida

    def __delitem__(self, num):
        xpv = self.li_xpv[num]
        sql = "DELETE FROM LINES where XPV=?"
        cursor = self._conexion.cursor()
        cursor.execute(sql, (xpv,))
        if xpv in self.cache:
            del self.cache[xpv]
        del self.li_xpv[num]
        self._conexion.commit()

    def __len__(self):
        return len(self.li_xpv)

    def close(self):
        if self._conexion:
            conexion = self._conexion
            self._conexion = None
            ult_pack = self.getconfig("ULT_PACK", 0)
            si_pack = ult_pack > 50
            self.setconfig("ULT_PACK", 0 if si_pack else ult_pack+1)
            self.db_config.close()
            self.db_config = None

            if si_pack:
                cursor = conexion.cursor()
                cursor.execute("VACUUM")
                cursor.close()
                conexion.commit()

            conexion.close()

    def grabarPGN(self, owner, ficheroPGN, maxDepth):
        erroneos = duplicados = importados = 0
        dlTmp = QTVarios.ImportarFicheroPGN(owner)
        dlTmp.hideDuplicados()
        dlTmp.show()

        sql = "INSERT INTO LINES( XPV, LINE ) VALUES( ?, ? )"
        cursor = self._conexion.cursor()

        base = self.getconfig("BASEPV")
        partidabase = self.getpartidabase()
        njugbase = partidabase.numJugadas()
        n = 0

        sql_insert = "INSERT INTO LINES( XPV, LINE ) VALUES( ?, ? )"
        sql_update = "UPDATE LINES SET XPV=?, LINE=? WHERE XPV=?"
        for n, g in enumerate(PGNreader.readGames(ficheroPGN), 1):
            if not dlTmp.actualiza(n, erroneos, duplicados, importados):
                break
            if g.erroneo:
                erroneos += 1
                continue
            if not g.moves:
                erroneos += 1
                continue

            def haz_partida(partida, liMoves):
                njg = partida.numJugadas()
                if len(liMoves) + njg > maxDepth:
                    liMoves = liMoves[:maxDepth+njg]
                pv = " ".join([move.pv for move in liMoves])
                partida.leerPV(pv)
                pv = partida.pv()
                if base and not pv.startswith(base) or partida.numJugadas() <= njugbase:
                    return
                xpv = LCEngine.pv2xpv(pv)
                if xpv in self.li_xpv:
                    return
                line_blob = partida.save2blob()
                updated = False
                for npos, xpv_ant in enumerate(self.li_xpv):
                    if xpv.startswith(xpv_ant):
                        cursor.execute(sql_update, (xpv, line_blob, xpv_ant))
                        self.li_xpv[npos] = xpv
                        updated = True
                        break
                if not updated:
                    cursor.execute(sql_insert, (xpv, line_blob))
                    self.li_xpv.append(xpv)

                for njug, move in enumerate(liMoves):
                    if move.variantes:
                        for lim in move.variantes:
                            p = partida.copia(njug-1) if njug > 0 else Partida.Partida()
                            haz_partida(p, lim.liMoves)

            partida = Partida.Partida()
            haz_partida(partida, g.moves.liMoves)
            if n %1000:
                self._conexion.commit()

        cursor.close()
        self.li_xpv.sort()
        self._conexion.commit()

        dlTmp.actualiza(n, erroneos, duplicados, importados)
        dlTmp.ponContinuar()

    def grabarPolyglot(self, ventana, ficheroBIN, depth, whiteBest, blackBest):
        titulo = _("Import a polyglot book")
        bp = QTUtil2.BarraProgreso1(ventana, titulo)
        bp.ponTotal(0)
        bp.ponRotulo(_X(_("Reading %1"), os.path.basename(ficheroBIN)))
        bp.mostrar()

        book = Books.Libro("P", ficheroBIN, ficheroBIN, True)
        book.polyglot()

        stFenM2 = set()  # para que no se produzca un circulo vicioso

        partidabase = self.getpartidabase()
        cp = partidabase.ultPosicion

        liPartidas = []

        setFen = LCEngine.setFen
        makeMove = LCEngine.makeMove
        getFen = LCEngine.getFen
        fen2fenM2 = LCEngine.fen2fenM2

        def hazFEN(fen, ply, lipv_ant):
            plyN = ply + 1
            siWhite = " w " in fen
            siMax = False
            if whiteBest:
                siMax = siWhite
            if blackBest:
                siMax = siMax or not siWhite

            liPV = book.miraListaPV(fen, siMax)
            if liPV:
                sigue = False
                for pv in liPV:
                    setFen(fen)
                    makeMove(pv)
                    fenN = getFen()

                    lipv_nue = lipv_ant[:]
                    lipv_nue.append(pv)
                    if plyN < depth:
                        fenM2 = fen2fenM2(fenN)
                        if fenM2 not in stFenM2:
                            stFenM2.add(fenM2)
                            hazFEN(fenN, plyN, lipv_nue)
                            sigue = True
            else:
                sigue = False
            if not sigue:
                p = Partida.Partida()
                p.leerPV(" ".join(lipv_ant))
                liPartidas.append(p)
                bp.ponTotal(len(liPartidas))
                bp.pon(len(liPartidas))

        hazFEN(cp.fen(), 0, partidabase.lipv())

        bp.ponRotulo(_("Writing..."))

        sql_insert = "INSERT INTO LINES( XPV, LINE ) VALUES( ?, ? )"
        sql_update = "UPDATE LINES SET XPV=?, LINE=? WHERE XPV=?"
        cursor = self._conexion.cursor()
        for partida in liPartidas:
            if partida.numJugadas() > partidabase.numJugadas():
                xpv = LCEngine.pv2xpv(partida.pv())
                if xpv not in self.li_xpv:
                    line_blob = partida.save2blob()
                    updated = False
                    for npos, xpv_ant in enumerate(self.li_xpv):
                        if xpv.startswith(xpv_ant):
                            cursor.execute(sql_update, (xpv, line_blob, xpv_ant))
                            self.li_xpv[npos] = xpv
                            updated = True
                            break
                    if not updated:
                        cursor.execute(sql_insert, (xpv, line_blob))
                        self.li_xpv.append(xpv)

        cursor.close()
        self._conexion.commit()
        self.li_xpv.sort()

        bp.cerrar()

        return True


