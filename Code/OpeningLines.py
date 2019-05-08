import os
import shutil
import sqlite3
import random
import datetime
import collections

import LCEngine4 as LCEngine

from Code import Util
from Code import Partida
from Code import ControlPosicion
from Code import PGNreader
from Code import DBgames
from Code import AperturasStd
from Code import EnginesBunch
from Code.QT import QTVarios
from Code.QT import QTUtil2


class ListaOpenings:
    def __init__(self, configuracion):
        self.folder = configuracion.folderOpenings
        if not self.folder or not os.path.isdir(self.folder):
            self.folder = configuracion.folderBaseOpenings

        self.fichero = os.path.join(self.folder, "openinglines.pk")

        self.lista = Util.recuperaVar(self.fichero)
        if self.lista is None:
            self.lista = self.read()  # file, lines, title, pv
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
                    "withtrainings": op.withTrainings(),
                    "withtrainings_engines": op.withTrainingsEngines(),
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
        dicline = {"file": file, "pv": basepv, "title": title, "lines": 0, "withtrainings": False}
        self.lista.append(dicline)
        op = Opening(self.filepath(len(self.lista) - 1))
        op.setbasepv(basepv)
        op.settitle(title)
        op.close()
        self.save()

    def copy(self, pos):
        dicline = dict(self.lista[pos])
        base = dicline["file"][:-3]
        if base.split("-")[-1].isdigit():
            li = base.split("-")
            base = "-".join(li[:-1])
        filenew = "%s-1.opk" % base
        n = 1
        while os.path.isfile(os.path.join(self.folder, filenew)):
            filenew = "%s-%d.opk" % (base, n)
            n += 1
        try:
            shutil.copy(self.filepath(pos), os.path.join(self.folder, filenew))
        except:
            return

        dicline["file"] = filenew
        dicline["title"] = dicline["title"] + " -%d" % (n - 1 if n > 1 else 1)
        self.lista.append(dicline)
        op = Opening(self.filepath(len(self.lista) - 1))
        op.settitle(dicline["title"])
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

    def add_training_engines_file(self, file):
        for dicline in self.lista:
            if file == dicline["file"]:
                dicline["withtrainings_engine"] = True
                self.save()
                return


class Opening:
    def __init__(self, nomFichero):
        self.nomFichero = nomFichero

        self._conexion = sqlite3.connect(nomFichero)

        self.cache = {}
        self.max_cache = 1000
        self.del_cache = 100

        self.grupo = 0

        self.history = collections.OrderedDict()

        self.li_xpv = self.init_database()

        self.db_config = Util.DicSQL(nomFichero, tabla="CONFIG")
        self.db_fenvalues = Util.DicSQL(nomFichero, tabla="FENVALUES")
        self.db_history = Util.DicSQL(nomFichero, tabla="HISTORY")
        self.db_cache_engines = None
        self.basePV = self.getconfig("BASEPV", "")
        self.title = self.getconfig("TITLE", os.path.basename(nomFichero).split(".")[0])

        self.tablero = None

    def open_cache_engines(self):
        if self.db_cache_engines is None:
            self.db_cache_engines = Util.DicSQL(self.nomFichero, tabla="CACHE_ENGINES")

    def get_cache_engines(self, engine, ms, fenM2, depth=None):
        if depth:
            key = "%s-%d-%s-%d" % (engine, ms, fenM2, depth)
        else:
            key = "%s-%d-%s" % (engine, ms, fenM2)
        return self.db_cache_engines[key]

    def set_cache_engines(self, engine, ms, fenM2, move, depth=None):
        if depth:
            key = "%s-%d-%s-%d" % (engine, ms, fenM2, depth)
        else:
            key = "%s-%d-%s" % (engine, ms, fenM2)
        self.db_cache_engines[key] = move

    def reinit_cache_engines(self):
        self.open_cache_engines()
        self.db_cache_engines.deleteall()

    def init_database(self):
        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(LINES)")
        if not cursor.fetchall():
            sql = "CREATE TABLE LINES( XPV TEXT PRIMARY KEY );"
            cursor.execute(sql)
            self._conexion.commit()
            li_xpv = []
        else:
            sql = "select XPV from LINES ORDER BY XPV"
            cursor.execute(sql)
            li_xpv = [raw[0] for raw in cursor.fetchall()]
        cursor.close()
        return li_xpv

    def setdbVisual_Tablero(self, tablero):
        self.tablero = tablero

    def getOtras(self, configuracion, partida):
        liOp = ListaOpenings(configuracion)
        fich = os.path.basename(self.nomFichero)
        pvbase = partida.pv()
        liOp = [
            (dic["file"], dic["title"])
            for dic in liOp.lista
            if dic["file"] != fich and (pvbase.startswith(dic["pv"]) or dic["pv"].startswith(pvbase))
        ]
        return liOp

    def getfenvalue(self, fenM2):
        resp = self.db_fenvalues[fenM2]
        return resp if resp else {}

    def setfenvalue(self, fenM2, dic):
        self.db_fenvalues[fenM2] = dic

    def removeAnalisis(self, tmpBP, mensaje):
        for n, fenM2 in enumerate(self.db_fenvalues.keys()):
            tmpBP.inc()
            tmpBP.mensaje(mensaje % n)
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
        self.setconfig("TRAINING", reg)

    def trainingEngines(self):
        return self.getconfig("TRAINING_ENGINES")

    def setTrainingEngines(self, reg):
        self.setconfig("TRAINING_ENGINES", reg)

    def preparaTraining(self, reg, procesador):
        maxmoves = reg["MAXMOVES"]
        siBlancas = reg["COLOR"] == "WHITE"
        siRandom = reg["RANDOM"]
        siRepetir = False

        lilipv = [LCEngine.xpv2pv(xpv).split(" ") for xpv in self.li_xpv]

        cp = ControlPosicion.ControlPosicion()

        if maxmoves:
            for pos, lipv in enumerate(lilipv):
                if len(lipv) > maxmoves:
                    lilipv[pos] = lipv[:maxmoves]

        # Ultimo el usuario
        for pos, lipv in enumerate(lilipv):
            if len(lipv) % 2 == (0 if siBlancas else 1):
                lilipv[pos] = lipv[:-1]

        # Quitamos las repetidas
        dicpv = {}
        for lipv in lilipv:
            pvmirar = "".join(lipv)
            if pvmirar in dicpv:
                continue

            siesta = False
            for pvotro in dicpv:
                if pvotro.startswith(pvmirar):
                    siesta = True
                    break
            if not siesta:
                dicpv[pvmirar] = lipv
        li = dicpv.keys()
        li.sort()
        lilipv = [value for key, value in dicpv.iteritems()]

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
                cp.leeFen(fen)
                fenM2 = cp.fenM2()
                if fenM2 not in dicFENm2:
                    dicFENm2[fenM2] = set()
                dicFENm2[fenM2].add(pv)
                LCEngine.makeMove(pv)

        if not siRepetir:
            stBorrar = set()
            xanalyzer = procesador.XAnalyzer()
            busca = " w " if siBlancas else " b "
            for stpv, fenM2 in dicFENm2.iteritems():
                if len(stpv) > 1:
                    if busca in fenM2:
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
                    cp.leeFen(fen)
                    fenM2 = cp.fenM2()
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

    def recalcFenM2(self):
        lilipv = [LCEngine.xpv2pv(xpv).split(" ") for xpv in self.li_xpv]

        cp = ControlPosicion.ControlPosicion()

        dicFENm2 = {}
        for lipv in lilipv:
            LCEngine.setFenInicial()
            for pv in lipv:
                fen = LCEngine.getFen()
                cp.leeFen(fen)
                fenM2 = cp.fenM2() # Tiene en cuenta unpassant no validos
                if fenM2 not in dicFENm2:
                    dicFENm2[fenM2] = set()
                dicFENm2[fenM2].add(pv)
                LCEngine.makeMove(pv)
        return dicFENm2

    def dicRepeFen(self, si_white):
        lilipv = [LCEngine.xpv2pv(xpv).split(" ") for xpv in self.li_xpv]

        dic = {}
        busca = " w " if si_white else " b "
        for nlinea, lipv in enumerate(lilipv):
            LCEngine.setFenInicial()
            for pv in lipv:
                fen = LCEngine.getFen()
                if busca in fen:
                    if fen not in dic:
                        dic[fen] = {}
                    dicPV = dic[fen]
                    if pv not in dicPV:
                        dicPV[pv] = []
                    dicPV[pv].append(nlinea)
                LCEngine.makeMove(pv)
        d = {}
        for fen, dicPV in dic.iteritems():
            if len(dicPV) > 1:
                d[fen] = dicPV
        return d

    def preparaTrainingEngines(self, configuracion, reg):
        reg["DICFENM2"] = self.recalcFenM2()
        reg["TIMES"] = [500, 1000, 2000, 4000, 8000]

        reg["ENGINES"] = EnginesBunch.getListaClave(configuracion, reg["NUM_ENGINES"], reg["KEY_ENGINE"])

    def updateTrainingEngines(self):
        reg = self.trainingEngines()
        reg["DICFENM2"] = self.recalcFenM2()
        self.setTrainingEngines(reg)

    def createTrainingSSP(self, reg, procesador):
        self.preparaTraining(reg, procesador)

        reg["DATECREATION"] = Util.hoy()
        self.setconfig("TRAINING", reg)
        self.setconfig("ULT_PACK", 100)  # Se le obliga al VACUUM

        lo = ListaOpenings(procesador.configuracion)
        lo.add_training_file(os.path.basename(self.nomFichero))

    def createTrainingEngines(self, reg, procesador):
        self.preparaTrainingEngines(procesador.configuracion, reg)
        reg["DATECREATION"] = Util.hoy()
        self.setTrainingEngines(reg)

        self.setconfig("ENG_LEVEL", 0)
        self.setconfig("ENG_ENGINE", 0)

        lo = ListaOpenings(procesador.configuracion)
        lo.add_training_engines_file(os.path.basename(self.nomFichero))
        self.reinit_cache_engines()

    def withTrainings(self):
        return "TRAINING" in self.db_config

    def withTrainingsEngines(self):
        return "TRAINING_ENGINES" in self.db_config

    def updateTraining(self, procesador):
        reg = self.training()
        if reg is None:
            return
        reg1 = {}
        for key in ("MAXMOVES", "COLOR", "RANDOM"):
            reg1[key] = reg[key]
        self.preparaTraining(reg1, procesador)

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

    def append(self, partida):
        xpv = LCEngine.pv2xpv(partida.pv())
        sql = "INSERT INTO LINES( XPV ) VALUES( ? )"
        cursor = self._conexion.cursor()
        cursor.execute(sql, (xpv,))
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
                si_sort = xpv_nue < self.li_xpv[num - 1]
            if not si_sort and num < len(self.li_xpv) - 1:
                si_sort = xpv_nue > self.li_xpv[num + 1]
            if si_sort:
                self.li_xpv.sort()
                num = self.li_xpv.index(xpv_nue)
        cursor = self._conexion.cursor()
        sql = "UPDATE LINES SET XPV=? WHERE XPV=?"
        cursor.execute(sql, (xpv_nue, xpv_ant))
        self._conexion.commit()
        self.add_cache(xpv_nue, partida_nue)
        cursor.close()
        return num

    def __getitem__(self, num):
        xpv = self.li_xpv[num]
        if xpv in self.cache:
            return self.cache[xpv]

        partida = Partida.Partida()
        pv = LCEngine.xpv2pv(xpv)
        partida.leerPV(pv)
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
        cursor.close()

    def __len__(self):
        return len(self.li_xpv)

    def removeLines(self, li, label):
        self.saveHistory(_("Removing"), label)
        li.sort(reverse=True)
        cursor = self._conexion.cursor()
        for num in li:
            xpv = self.li_xpv[num]
            sql = "DELETE FROM LINES where XPV=?"
            cursor.execute(sql, (xpv,))
            if xpv in self.cache:
                del self.cache[xpv]
            del self.li_xpv[num]
        self._conexion.commit()
        cursor.close()

    def remove_lastmove(self, is_white, label):
        self.saveHistory(_("Removing"), label)
        n = len(self.li_xpv)
        cursor = self._conexion.cursor()
        for x in range(n-1, -1, -1):
            xpv = self.li_xpv[x]
            pv = LCEngine.xpv2pv(xpv)
            nm = pv.count(" ")
            if nm%2 == 0 and is_white or nm%2 == 1 and not is_white:
                pv_nue = " ".join(pv.split(" ")[:-1])
                xpv_nue = LCEngine.pv2xpv(pv_nue)
                if xpv_nue in self.li_xpv or not xpv_nue:
                    sql = "DELETE FROM LINES where XPV=?"
                    cursor.execute(sql, (xpv,))
                    del self.li_xpv[x]
                else:
                    sql = "UPDATE LINES SET XPV=? WHERE XPV=?"
                    cursor.execute(sql, (xpv_nue, xpv))
                    self.li_xpv[x] = xpv_nue
                if xpv in self.cache:
                    del self.cache[xpv]
        self.li_xpv.sort()
        self._conexion.commit()
        cursor.close()

    def lihistory(self):
        return self.db_history.keys(siOrdenados=True, siReverse=True)

    def saveHistory(self, *label):
        d = datetime.datetime.now()
        s = "%s-%s" % (d.strftime("%Y-%m-%d %H:%M:%S"), ",".join(label))
        self.db_history[s] = self.li_xpv[:]

    def rechistory(self, key):
        self.saveHistory(_("Recovering"), key)

        stActivo = set(self.li_xpv)
        li_xpv_rec = self.db_history[key]
        stRecuperar = set(li_xpv_rec)

        cursor = self._conexion.cursor()

        # Borramos los que no estan en Recuperar
        sql = "DELETE FROM LINES where XPV=?"
        for xpv in stActivo:
            if xpv not in stRecuperar:
                cursor.execute(sql, (xpv,))
        self._conexion.commit()

        # Mas los que no estan en Activo
        sql = "INSERT INTO LINES( XPV ) VALUES( ? )"
        for xpv in stRecuperar:
            if xpv not in stActivo:
                cursor.execute(sql, (xpv,))
        self._conexion.commit()

        cursor.close()
        self.li_xpv = li_xpv_rec

    def close(self):
        if self._conexion:
            conexion = self._conexion
            self._conexion = None
            ult_pack = self.getconfig("ULT_PACK", 0)
            si_pack = ult_pack > 50
            self.setconfig("ULT_PACK", 0 if si_pack else ult_pack + 1)
            self.db_config.close()
            self.db_config = None

            self.db_fenvalues.close()
            self.db_fenvalues = None

            if self.db_cache_engines:
                self.db_cache_engines.close()
                self.db_cache_engines = None

            if self.tablero:
                self.tablero.dbVisual_close()
                self.tablero = None

            if si_pack:
                if len(self.db_history) > 70:
                    lik = self.db_history.keys(siOrdenados=True, siReverse=False)
                    liremove = lik[: len(self.db_history) - 50]
                    for k in liremove:
                        del self.db_history[k]
                self.db_history.close()

                cursor = conexion.cursor()
                cursor.execute("VACUUM")
                cursor.close()
                conexion.commit()

            else:
                self.db_history.close()

            conexion.close()

    def importarPGN(self, owner, partidabase, ficheroPGN, maxDepth, variations):

        erroneos = duplicados = importados = 0
        dlTmp = QTVarios.ImportarFicheroPGN(owner)
        dlTmp.hideDuplicados()
        dlTmp.show()

        self.saveHistory(_("Import"), _("PGN with variants"), os.path.basename(ficheroPGN))

        cursor = self._conexion.cursor()

        base = partidabase.pv() if partidabase else self.getconfig("BASEPV")

        sql_insert = "INSERT INTO LINES( XPV ) VALUES( ? )"
        sql_update = "UPDATE LINES SET XPV=? WHERE XPV=?"

        for n, g in enumerate(PGNreader.readGames(ficheroPGN), 1):
            if not dlTmp.actualiza(n, erroneos, duplicados, importados):
                break
            if g.erroneo:
                erroneos += 1
                continue
            if not g.moves:
                erroneos += 1
                continue

            def haz_partida(liMoves):
                njg = len(liMoves)
                if njg > maxDepth:
                    liMoves = liMoves[:maxDepth]
                pv = " ".join([move.pv for move in liMoves])
                partida = Partida.Partida()
                partida.leerPV(pv)
                pv = partida.pv()
                if base and not pv.startswith(base):
                    return
                xpv = LCEngine.pv2xpv(pv)
                updated = False
                for npos, xpv_ant in enumerate(self.li_xpv):
                    if xpv_ant.startswith(xpv):
                        return
                    if xpv.startswith(xpv_ant):
                        cursor.execute(sql_update, (xpv, xpv_ant))
                        self.li_xpv[npos] = xpv
                        updated = True
                        break
                if not updated:
                    cursor.execute(sql_insert, (xpv,))
                    self.li_xpv.append(xpv)

                if variations != "N":  #None
                    for njug, move in enumerate(liMoves):
                        ok = True
                        if variations != "A":
                            if variations == "W":
                                if njug % 2 == 1:
                                    ok = False
                            elif variations == "B":
                                if njug % 2 == 0:
                                    ok = False
                        if ok:
                            for lim in move.variantes:
                                limovnv = [liMoves[j].clona() for j in range(njug)]
                                for move in limovnv:
                                    move.variantes = []
                                limovnv.extend(lim.liMoves)
                                haz_partida(limovnv)

            haz_partida(g.moves.liMoves)
            if n % 50:
                self._conexion.commit()

        cursor.close()
        self.li_xpv.sort()
        self._conexion.commit()

        dlTmp.actualiza(n, erroneos, duplicados, importados)
        dlTmp.ponContinuar()

    def importarPGO(self, partidabase, ficheroPGO, maxDepth):

        self.saveHistory(_("Personal Opening Guide"), os.path.basename(ficheroPGO))

        base = partidabase.pv() if partidabase else self.getconfig("BASEPV")
        baseXPV = LCEngine.pv2xpv(base)

        conexionPGO = sqlite3.connect(ficheroPGO)
        liRawPGO = conexionPGO.execute("SELECT XPV from GUIDE")
        stPGO = set()
        for raw in liRawPGO:
            xpv = raw[0]
            if maxDepth:
                lipv = LCEngine.xpv2pv(xpv).split(" ")
                if len(lipv) > maxDepth:
                    lipv = lipv[:maxDepth]
                    xpv = LCEngine.pv2xpv(" ".join(lipv))
            if not xpv.startswith(baseXPV):
                continue
            ok = True
            lirem = []
            for xpv0 in stPGO:
                if xpv.startswith(xpv0):
                    lirem.append(xpv0)
                elif xpv0.startswith(xpv):
                    ok = False
                    break
            for xpv0 in lirem:
                stPGO.remove(xpv0)
            if ok:
                stPGO.add(xpv)
        conexionPGO.close()

        cursor = self._conexion.cursor()

        sql_insert = "INSERT INTO LINES( XPV ) VALUES( ? )"
        sql_update = "UPDATE LINES SET XPV=? WHERE XPV=?"

        for xpv in stPGO:
            add = True
            for npos, xpv_ant in enumerate(self.li_xpv):
                if xpv_ant.startswith(xpv):
                    add = False
                    break
                if xpv.startswith(xpv_ant):
                    cursor.execute(sql_update, (xpv, xpv_ant))
                    self.li_xpv[npos] = xpv
                    add = False
                    break
            if add:
                cursor.execute(sql_insert, (xpv,))
                self.li_xpv.append(xpv)

        cursor.close()
        self.li_xpv.sort()
        self._conexion.commit()

    def guardaPartidas(self, label, liPartidas, minMoves=0, with_history=True):
        if with_history:
            self.saveHistory(_("Import"), label)
        partidabase = self.getpartidabase()
        sql_insert = "INSERT INTO LINES( XPV) VALUES( ? )"
        sql_update = "UPDATE LINES SET XPV=? WHERE XPV=?"
        cursor = self._conexion.cursor()
        for partida in liPartidas:
            if minMoves <= partida.numJugadas() > partidabase.numJugadas():
                xpv = LCEngine.pv2xpv(partida.pv())
                if xpv not in self.li_xpv:
                    updated = False
                    for npos, xpv_ant in enumerate(self.li_xpv):
                        if xpv.startswith(xpv_ant):
                            cursor.execute(sql_update, (xpv, xpv_ant))
                            self.li_xpv[npos] = xpv
                            updated = True
                            break
                    if not updated:
                        cursor.execute(sql_insert, (xpv,))
                        self.li_xpv.append(xpv)

        cursor.close()
        self._conexion.commit()
        self.li_xpv.sort()

    def guardaLiXPV(self, label, liXPV):
        self.saveHistory(_("Import"), label)
        sql_insert = "INSERT INTO LINES( XPV) VALUES( ? )"
        sql_update = "UPDATE LINES SET XPV=? WHERE XPV=?"
        cursor = self._conexion.cursor()
        for xpv in liXPV:
            if xpv not in self.li_xpv:
                updated = False
                for npos, xpv_ant in enumerate(self.li_xpv):
                    if xpv.startswith(xpv_ant):
                        cursor.execute(sql_update, (xpv, xpv_ant))
                        self.li_xpv[npos] = xpv
                        updated = True
                        break
                if not updated:
                    cursor.execute(sql_insert, (xpv,))
                    self.li_xpv.append(xpv)
        cursor.close()
        self._conexion.commit()
        self.li_xpv.sort()

    def importarPolyglot(self, ventana, partida, bookW, bookB, titulo, depth, siWhite, onlyone, minMoves):
        bp = QTUtil2.BarraProgreso1(ventana, titulo, formato1="%m")
        bp.ponTotal(0)
        bp.ponRotulo(_X(_("Reading %1"), "..."))
        bp.mostrar()

        cp = partida.ultPosicion

        setFen = LCEngine.setFen
        makeMove = LCEngine.makeMove
        getFen = LCEngine.getFen

        def hazFEN(fen, lipv_ant, control):
            if bp.siCancelado():
                return
            siWhite1 = " w " in fen
            book = bookW if siWhite1 else bookB
            liPV = book.miraListaPV(fen, siWhite1 == siWhite, onlyone=onlyone)
            if liPV and len(lipv_ant) < depth:
                for pv in liPV:
                    setFen(fen)
                    makeMove(pv)
                    fenN = getFen()
                    lipv_nue = lipv_ant[:]
                    lipv_nue.append(pv)
                    hazFEN(fenN, lipv_nue, control)
            else:
                p = Partida.Partida()
                p.leerLIPV(lipv_ant)
                control.liPartidas.append(p)
                control.num_partidas += 1
                bp.ponTotal(control.num_partidas)
                bp.pon(control.num_partidas)
                if control.num_partidas and control.num_partidas % 1000 == 0:
                    self.guardaPartidas(control.rotulo, control.liPartidas, minMoves, with_history=control.with_history)
                    control.liPartidas = []
                    control.with_history = False

        control = Util.Almacen()
        control.liPartidas = []
        control.num_partidas = 0
        control.with_history = True
        control.rotulo = "%s,%s,%s" % (_("Polyglot book"), bookW.nombre, bookB.nombre)

        hazFEN(cp.fen(), partida.lipv(), control)

        bp.ponRotulo(_("Writing..."))

        if control.liPartidas:
            self.guardaPartidas(control.rotulo, control.liPartidas, minMoves, with_history=control.with_history)
        bp.cerrar()

        return True

    def importarSummary(self, ventana, partidabase, ficheroSummary, depth, siWhite, onlyone, minMoves):
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
            n_ant = len(lipv_ant)
            siWhite1 = n_ant % 2 == 0

            pv_ant = " ".join(lipv_ant) if n_ant else ""
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
                tt_max = 0
                limax = []
                for alm in liChildren:
                    tt = alm.W + alm.B + alm.O + alm.D
                    if tt > tt_max:
                        tt_max = tt
                        limax = [alm]
                    elif tt == tt_max and not onlyone:
                        limax.append(alm)
                liChildren = limax

            for alm in liChildren:
                li = lipv_ant[:]
                li.append(alm.move)
                hazPV(li)

        hazPV(pvBase.split(" ") if pvBase else [])

        bp.ponRotulo(_("Writing..."))
        self.guardaPartidas("%s,%s" % (_("Database summary"), os.path.basename(ficheroSummary)), liPartidas)
        bp.cerrar()

        return True

    def importarOtra(self, pathFichero, partida):
        xpvbase = LCEngine.pv2xpv(partida.pv())
        tambase = len(xpvbase)
        otra = Opening(pathFichero)
        lista = []
        for n, xpv in enumerate(otra.li_xpv):
            if xpv.startswith(xpvbase) and len(xpv) > tambase:
                if xpv not in self.li_xpv:
                    lista.append(xpv)
        otra.close()
        self.guardaLiXPV("%s,%s" % (_("Other opening lines"), otra.title), lista)

    def exportarPGN(self, ws, result):
        liTags = [("Event", self.title.replace('"', "")), ("Site", ""), ("Date", Util.hoy().strftime("%Y-%m-%d"))]
        if result:
            liTags.append(("Result", result))

        total = len(self)

        ws.pb(total)

        for recno in range(total):
            ws.pb_pos(recno+1)
            if ws.pb_cancel():
                break
            partida = self[recno]

            liTags[1] = ("Site", "%s %d" % (_("Line"), recno + 1))

            if recno > 0 or not ws.is_new:
                ws.write("\n\n")
            tags = "".join(['[%s "%s"]\n' % (k, v) for k, v in liTags])
            ws.write(tags)
            ws.write("\n%s" % partida.pgnBase())

        ws.pb_close()

    def getAllFen(self):
        stFENm2 = set()
        lilipv = [LCEngine.xpv2pv(xpv).split(" ") for xpv in self.li_xpv]
        for lipv in lilipv:
            LCEngine.setFenInicial()
            for pv in lipv:
                LCEngine.makeMove(pv)
                fen = LCEngine.getFen()
                fenM2 = LCEngine.fen2fenM2(fen)
                stFENm2.add(fenM2)
        return stFENm2

    def getNumLinesPV(self, lipv, base=1):
        xpv = LCEngine.pv2xpv(" ".join(lipv))
        li = [num for num, xpv0 in enumerate(self.li_xpv, base) if xpv0.startswith(xpv)]
        return li

    def totree(self):
        parent = ItemTree(None, None, None, None)
        dic = AperturasStd.ap.dicFenM2
        for xpv in self.li_xpv:
            lipv = LCEngine.xpv2pv(xpv).split(" ")
            lipgn = LCEngine.xpv2pgn(xpv).replace("\n", " ").strip().split(" ")
            linom = []
            LCEngine.setFenInicial()
            for pv in lipv:
                LCEngine.makeMove(pv)
                fen = LCEngine.getFen()
                fenM2 = LCEngine.fen2fenM2(fen)
                linom.append(dic[fenM2].trNombre if fenM2 in dic else "")
            parent.addLista(lipv, lipgn, linom)
        return parent


class ItemTree:
    def __init__(self, parent, move, pgn, opening):
        self.move = move
        self.pgn = pgn
        self.parent = parent
        self.opening = opening
        self.dicHijos = {}
        self.item = None

    def add(self, move, pgn, opening):
        if move not in self.dicHijos:
            self.dicHijos[move] = ItemTree(self, move, pgn, opening)
        return self.dicHijos[move]

    def addLista(self, limoves, lipgn, liop):
        n = len(limoves)
        if n > 0:
            item = self.add(limoves[0], lipgn[0], liop[0])
            if n > 1:
                item.addLista(limoves[1:], lipgn[1:], liop[1:])

    def game(self):
        li = []
        if self.pgn:
            li.append(self.pgn)

        item = self.parent
        while item is not None:
            if item.pgn:
                li.append(item.pgn)
            item = item.parent
        return " ".join(reversed(li))

    def listaPV(self):
        li = []
        if self.move:
            li.append(self.move)

        item = self.parent
        while item is not None:
            if item.move:
                li.append(item.move)
            item = item.parent
        return li[::-1]
