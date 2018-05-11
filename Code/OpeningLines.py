import os
import sqlite3
import random

import LCEngineV1 as LCEngine

from Code import Util
from Code import Partida
from Code import PGNreader
from Code import DBgames
from Code.QT import QTVarios
from Code.QT import QTUtil2

class ListaOpenings:
    def __init__(self, configuracion):
        self.folder = configuracion.folderOpenings
        self.fichero = os.path.join(self.folder, "openinglines.pk")

        self.lista = Util.recuperaVar(self.fichero)
        if self.lista is None:
            self.lista = self.read()    # file, lines, title, pv
            self.save()
        else:
            self.testdates()

    def testdates(self):
        index_date = Util.datefile(self.fichero)

        for pos, dic in enumerate(self.lista):
            pathfile = os.path.join(self.folder, dic["file"])
            file_date = Util.datefile(pathfile)
            if file_date is None:
                self.reiniciar()
                break
            if file_date > index_date:
                op = Opening(pathfile)
                self.lista[pos]["lines"] = len(op)
                op.close()
                self.save()

    def reiniciar(self):
        self.lista = self.read()
        self.save()

    def __len__(self):
        return len(self.lista)

    def __getitem__(self, item):
        return self.lista[item] if self.lista and item < len(self.lista) else None

    def __delitem__(self, item):
        dicline = self.lista[item]
        del self.lista[item]
        os.remove(os.path.join(self.folder, dicline["file"]))
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

    def read(self):
        li = []
        for entry in Util.listdir(self.folder):
            fichero = entry.name
            if fichero.endswith(".opk"):
                op = Opening(entry.path)
                dicline = {
                    "file": fichero,
                    "pv": op.basePV,
                    "title": op.title,
                    "lines": len(op),
                    "withtrainings": op.withTrainings()
                }
                li.append(dicline)
                op.close()
        return li

    def save(self):
        Util.guardaVar(self.fichero, self.lista)

    def select_filename(self, name):
        name = name.strip().replace(" ", "_")
        name = Util.asciiNomFichero(name)

        plant = name + "%d.opk"
        file = name + ".opk"
        num = 0
        while os.path.isfile(os.path.join(self.folder, file)):
            num += 1
            file = plant % num
        return file

    def filepath(self, num):
        return os.path.join(self.folder, self.lista[num]["file"])

    def new(self, file, basepv, title):
        dicline = {
            "file": file,
            "pv": basepv,
            "title": title,
            "lines": 0,
            "withtrainings": False
        }
        self.lista.append(dicline)
        op = Opening(self.filepath(len(self.lista)-1))
        op.setbasepv(basepv)
        op.settitle(title)
        op.close()
        self.save()

    def change_title(self, num, title):
        op = Opening(self.filepath(num))
        op.settitle(title)
        op.close()
        self.lista[num]["title"] = title
        self.save()

    def add_training_file(self, file):
        for dicline in self.lista:
            if file == dicline["file"]:
                dicline["withtrainings"] = True
                self.save()
                return


class Opening:
    def __init__(self, nomFichero):
        self.nomFichero = nomFichero

        self._conexion = sqlite3.connect(nomFichero)

        self.cache = {}
        self.max_cache = 4000
        self.del_cache = 1000

        self.grupo = 0

        self.li_xpv = self.init_database()

        self.db_config = Util.DicSQL(nomFichero, tabla="CONFIG")
        self.db_fenvalues = Util.DicSQL(nomFichero, tabla="FENVALUES")
        self.basePV = self.getconfig("BASEPV", "")
        self.title = self.getconfig("TITLE", os.path.basename(nomFichero).split(".")[0])

        self.tablero = None

    def setdbVisual_Tablero(self, tablero):
        self.tablero = tablero

    def getOtras(self, configuracion, partida):
        liOp = ListaOpenings(configuracion)
        fich = os.path.basename(self.nomFichero)
        pvbase = partida.pv()
        liOp = [(dic["file"], dic["title"]) for dic in liOp.lista if dic["file"] != fich and (pvbase.startswith(dic["pv"]) or dic["pv"].startswith(pvbase))]
        return liOp

    def getfenvalue(self, fenM2):
        resp = self.db_fenvalues[fenM2]
        return resp if resp else {}

    def setfenvalue(self, fenM2, dic):
        self.db_fenvalues[fenM2] = dic

    def removeAnalisis(self, tmpBP, mensaje):
        for n, fenM2 in enumerate(self.db_fenvalues.keys()):
            tmpBP.inc()
            tmpBP.mensaje(mensaje%n)
            if tmpBP.siCancelado():
                break
            dic = self.getfenvalue(fenM2)
            if "ANALISIS" in dic:
                del dic["ANALISIS"]
                self.setfenvalue(fenM2, dic)
        self.packAlTerminar()


    def getconfig(self, key, default=None):
        return self.db_config.get(key, default)

    def setconfig(self, key, value):
        self.db_config[key] = value

    def training(self):
        return self.getconfig("TRAINING")

    def setTraining(self, reg):
        return self.setconfig("TRAINING", reg)

    def preparaTraining(self, reg, procesador):
        maxmoves = reg["MAXMOVES"]
        siBlancas = reg["COLOR"] == "WHITE"
        siRandom = reg["RANDOM"]
        siRepetir = False

        lilipv = [LCEngine.xpv2pv(xpv).split(" ") for xpv in self.li_xpv]

        if maxmoves:
            for pos, lipv in enumerate(lilipv):
                if len(lipv) > maxmoves:
                    lilipv[pos] = lipv[:maxmoves]

        # Ultimo el usuario
        for pos, lipv in enumerate(lilipv):
            if len(lipv) % 2 == (0 if siBlancas else 1):
                lilipv[pos] = lipv[:-1]

        # Quitamos las repetidas
        lilipvfinal = []
        nt = len(lilipv)
        for x in range(nt-1):
            pvmirar = "".join(lilipv[x])
            esta = False
            for y in range(x+1, nt):
                pvotro = "".join(lilipv[y])
                if pvotro.startswith(pvmirar):
                    esta = True
                    break
            if not esta:
                lilipvfinal.append(lilipv[x])
        lilipv = lilipvfinal

        ligamesST = []
        ligamesSQ = []
        dicFENm2 = {}
        for lipv in lilipv:
            game = {}
            game["LIPV"] = lipv
            game["NOERROR"] = 0
            game["TRIES"] = []

            ligamesST.append(game)
            game = dict(game)
            ligamesSQ.append(game)
            LCEngine.setFenInicial()
            for pv in lipv:
                fen = LCEngine.getFen()
                fenM2 = LCEngine.fen2fenM2(fen)
                if fenM2 not in dicFENm2:
                    dicFENm2[fenM2] = set()
                dicFENm2[fenM2].add(pv)
                LCEngine.makeMove(pv)

        if not siRepetir:
            stBorrar = set()
            xanalyzer = procesador.XAnalyzer()
            for stpv, fenM2 in dicFENm2.iteritems():
                if len(stpv) > 1:
                    siW = " w " in fenM2
                    if siW and siBlancas:
                        dic = self.getfenvalue(fenM2)
                        if "ANALISIS" not in dic:
                            dic["ANALISIS"] = xanalyzer.analiza(fen)
                            self.setfenvalue(fenM2, dic)
                        mrm = dic["ANALISIS"]
                        pvsel = stpv[0]  # el primero que encuentre por defecto
                        for rm in mrm.liMultiPV():
                            pv0 = rm.movimiento()
                            if pv0 in stpv:
                                pvsel = pv0
                                stpv.remove(pvsel)
                                break
                        dicFENm2[fenM2] = {pvsel}
                        for pv in stpv:
                            stBorrar.add("%s|%s" % (fenM2, pv))
            liBorrar = []
            for n, game in enumerate(ligamesSQ):
                LCEngine.setFenInicial()
                for pv in game["LIPV"]:
                    fen = LCEngine.getFen()
                    fenM2 = LCEngine.fen2fenM2(fen)
                    key = "%s|%s" % (fenM2, pv)
                    if key in stBorrar:
                        liBorrar.append(n)
                        break
                    LCEngine.makeMove(pv)
            liBorrar.sort(reverse=True)
            for n in liBorrar:
                del ligamesSQ[n]
                del ligamesST[n]

        if siRandom:
            random.shuffle(ligamesSQ)
            random.shuffle(ligamesST)
        reg["LIGAMES_STATIC"] = ligamesST
        reg["LIGAMES_SEQUENTIAL"] = ligamesSQ
        reg["DICFENM2"] = dicFENm2

        bcolor = " w " if siBlancas else " b "
        liTrainPositions = []
        for fenM2 in dicFENm2:
            if bcolor in fenM2:
                data = {}
                data["FENM2"] = fenM2
                data["MOVES"] = dicFENm2[fenM2]
                data["NOERROR"] = 0
                data["TRIES"] = []
                liTrainPositions.append(data)
        random.shuffle(liTrainPositions)
        reg["LITRAINPOSITIONS"] = liTrainPositions

    def createTraining(self, reg, procesador):
        self.preparaTraining(reg, procesador)

        reg["DATECREATION"] = Util.hoy()
        self.setconfig("TRAINING", reg)
        self.setconfig("ULT_PACK", 100)  # Se le obliga al VACUUM

        lo = ListaOpenings(procesador.configuracion)
        lo.add_training_file(os.path.basename(self.nomFichero))

    def withTrainings(self):
        return "TRAINING" in self.db_config

    def updateTraining(self):
        reg = self.training()
        reg1 = {}
        for key in ("MAXMOVES", "COLOR", "RANDOM"):
            reg1[key] = reg[key]
        self.preparaTraining(reg1)

        for tipo in ("LIGAMES_SEQUENTIAL", "LIGAMES_STATIC"):
            # Los que estan pero no son, los borramos
            liBorrados = []
            for pos, game in enumerate(reg[tipo]):
                pv = " ".join(game["LIPV"])
                ok = False
                for game1 in reg1[tipo]:
                    pv1 = " ".join(game1["LIPV"])
                    if pv == pv1:
                        ok = True
                        break
                if not ok:
                    liBorrados.append(pos)
            if liBorrados:
                li = reg[tipo]
                liBorrados.sort(reverse=True)
                for x in liBorrados:
                    del li[x]
                reg[tipo] = li

            # Los que son pero no estan
            liMas = []
            for game1 in reg1[tipo]:
                pv1 = " ".join(game1["LIPV"])
                ok = False
                for game in reg[tipo]:
                    pv = " ".join(game["LIPV"])
                    if pv == pv1:
                        ok = True
                        break
                if not ok:
                    liMas.append(game1)
            if liMas:
                li = reg[tipo]
                liMas.sort(reverse=True)
                for game in liMas:
                    li.insert(0, game)
                reg[tipo] = li

        reg["DICFENM2"] = reg1["DICFENM2"]

        # Posiciones

        # Estan pero no son
        liBorrados = []
        tipo = "LITRAINPOSITIONS"
        for pos, data in enumerate(reg[tipo]):
            fen = data["FENM2"]
            ok = False
            for data1 in reg1[tipo]:
                fen1 = data1["FENM2"]
                if fen == fen1:
                    ok = True
                    break
            if not ok:
                liBorrados.append(pos)
        if liBorrados:
            li = reg[tipo]
            liBorrados.sort(reverse=True)
            for x in liBorrados:
                del li[x]
            reg[tipo] = li

        # Los que son pero no estan
        liMas = []
        for data1 in reg1[tipo]:
            fen1 = data1["FENM2"]
            ok = False
            for data in reg[tipo]:
                fen = data["FENM2"]
                if fen == fen1:
                    ok = True
                    break
            if not ok:
                liMas.append(data)
        if liMas:
            li = reg[tipo]
            li.insert(0, liMas)
            reg[tipo] = li

        self.setconfig("TRAINING", reg)
        self.packAlTerminar()

    def packAlTerminar(self):
        self.setconfig("ULT_PACK", 100)  # Se le obliga al VACUUM

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
        cursor.close()
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
        cursor.close()
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
        cursor.close()
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
        cursor.close()

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

            self.db_fenvalues.close()
            self.db_fenvalues = None

            if self.tablero:
                self.tablero.dbVisual_close()
                self.tablero = None

            if si_pack:
                cursor = conexion.cursor()
                cursor.execute("VACUUM")
                cursor.close()
                conexion.commit()

            conexion.close()

    def importarPGN(self, owner, partidabase, ficheroPGN, maxDepth):
        erroneos = duplicados = importados = 0
        dlTmp = QTVarios.ImportarFicheroPGN(owner)
        dlTmp.hideDuplicados()
        dlTmp.show()

        cursor = self._conexion.cursor()

        base = self.getconfig("BASEPV")
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
                njg = len(partida)
                if len(liMoves) + njg > maxDepth:
                    liMoves = liMoves[:maxDepth - njg]
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
            if n % 50:
                self._conexion.commit()

        cursor.close()
        self.li_xpv.sort()
        self._conexion.commit()

        dlTmp.actualiza(n, erroneos, duplicados, importados)
        dlTmp.ponContinuar()

    def guardaPartidas(self, liPartidas, minMoves=0):
        partidabase = self.getpartidabase()
        sql_insert = "INSERT INTO LINES( XPV, LINE ) VALUES( ?, ? )"
        sql_update = "UPDATE LINES SET XPV=?, LINE=? WHERE XPV=?"
        cursor = self._conexion.cursor()
        for partida in liPartidas:
            if minMoves <= partida.numJugadas() > partidabase.numJugadas():
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

    def importarPolyglot(self, ventana, partida, bookW, bookB, titulo, depth, siWhite, minMoves):
        bp = QTUtil2.BarraProgreso1(ventana, titulo)
        bp.ponTotal(0)
        bp.ponRotulo(_X(_("Reading %1"), "..."))
        bp.mostrar()

        cp = partida.ultPosicion

        liPartidas = []

        setFen = LCEngine.setFen
        makeMove = LCEngine.makeMove
        getFen = LCEngine.getFen

        def hazFEN(fen, lipv_ant):
            if bp.siCancelado():
                return
            siWhite1 = " w " in fen
            book = bookW if siWhite1 else bookB
            liPV = book.miraListaPV(fen, siWhite1 == siWhite)
            if liPV and len(lipv_ant) < depth:
                for pv in liPV:
                    setFen(fen)
                    makeMove(pv)
                    fenN = getFen()
                    lipv_nue = lipv_ant[:]
                    lipv_nue.append(pv)
                    hazFEN(fenN, lipv_nue)
            else:
                p = Partida.Partida()
                p.leerLIPV(lipv_ant)
                liPartidas.append(p)
                bp.ponTotal(len(liPartidas))
                bp.pon(len(liPartidas))

        hazFEN(cp.fen(), partida.lipv())

        bp.ponRotulo(_("Writing..."))
        self.guardaPartidas(liPartidas, minMoves)
        bp.cerrar()

        return True

    def importarSummary(self, ventana, partidabase, ficheroSummary, depth, siWhite, minMoves):
        titulo = _("Importing the summary of a database")
        bp = QTUtil2.BarraProgreso1(ventana, titulo)
        bp.ponTotal(0)
        bp.ponRotulo(_X(_("Reading %1"), os.path.basename(ficheroSummary)))
        bp.mostrar()

        dbSTAT = DBgames.TreeSTAT(ficheroSummary)

        if depth == 0:
            depth = 99999

        pvBase = partidabase.pv()
        len_partidabase = len(partidabase)

        liPartidas = []

        def hazPV(lipv_ant):
            if bp.siCancelado():
                return
            siWhite1 = len(lipv_ant) % 2 == 0

            pv_ant = " ".join(lipv_ant)
            liChildren = dbSTAT.children(pv_ant, False)

            if len(liChildren) == 0 or len(lipv_ant) > depth:
                p = Partida.Partida()
                p.leerLIPV(lipv_ant)
                if len(p) > len_partidabase:
                    liPartidas.append(p)
                    bp.ponTotal(len(liPartidas))
                    bp.pon(len(liPartidas))
                return

            if siWhite1 == siWhite:
                alm_max = None
                tt_max = 0
                for alm in liChildren:
                    tt = alm.W + alm.B + alm.O + alm.D
                    if tt > tt_max:
                        tt_max = tt
                        alm_max = alm
                liChildren = [] if tt_max == 0 else [alm_max,]

            for alm in liChildren:
                li = lipv_ant[:]
                li.append(alm.move)
                hazPV(li)

        hazPV(pvBase.split(" "))

        bp.ponRotulo(_("Writing..."))
        self.guardaPartidas(liPartidas)
        bp.cerrar()

        return True

    def importarOtra(self, pathFichero, partida):
        xpvbase = LCEngine.pv2xpv(partida.pv())
        tambase = len(xpvbase)
        otra = Opening(pathFichero)
        liPartidas = []
        for n, xpv in enumerate(otra.li_xpv):
            if xpv.startswith(xpvbase) and tambase < len(xpv):
                liPartidas.append(otra[n])
        otra.close()
        self.guardaPartidas(liPartidas)

    def getAllFen(self):
        stFENm2 = set()
        lilipv = [LCEngine.xpv2pv(xpv).split(" ") for xpv in self.li_xpv]
        for lipv in lilipv:
            LCEngine.setFenInicial()
            for pv in lipv:
                fen = LCEngine.getFen()
                fenM2 = LCEngine.fen2fenM2(fen)
                stFENm2.add(fenM2)
                LCEngine.makeMove(pv)
        return stFENm2

    def getNumLinesPV(self, pv):
        xpv = LCEngine.pv2xpv(" ".join(pv))
        li = [num for num, xpv0 in enumerate(self.li_xpv, 1) if xpv0.startswith(xpv)]
        return li
