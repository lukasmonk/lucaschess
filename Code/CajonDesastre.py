import base64
import cPickle
import os
import shutil
import sqlite3

import LCEngine4 as LCEngine

from Code import BaseConfig
from Code import Util
from Code import VarGen


def compruebaCambioCarpetas(configuracion):
    usrdata = configuracion.carpeta

    def haz(original, nueva):
        original = os.path.join(usrdata, original)
        nueva = os.path.join(usrdata, nueva)
        if os.path.isfile(original) and not os.path.isfile(nueva):
            shutil.move(original, nueva)

    haz("Mate positions in GM gamesB.tdb", "Checkmates in GM gamesB.tdb")
    haz("From Uwe Auerswald's CollectionB.tdb", "Tactics by Uwe AuerswaldB.tdb")

    if os.path.isfile("dataDB/dataDB.pkd"):
        os.remove("dataDB/dataDB.pkd")

    # cambios de ficheros gm
    gmConvert(configuracion)

    # cambios en dic entmaquina
    dic = Util.recuperaDIC(configuracion.ficheroEntMaquina)
    if dic:
        liAperturasFavoritas = dic.get("APERTURASFAVORITAS", [])
        if liAperturasFavoritas:
            for npos, x in enumerate(liAperturasFavoritas):
                if type(x) == tuple:
                    liAperturasFavoritas[npos] = x[0]
        Util.guardaDIC(dic, configuracion.ficheroEntMaquina)

    # Dic -> DicSQL
    dicDisk(configuracion)

    # Fichero potencia
    ficheroPotencia(configuracion)


def ficheroPotencia(configuracion):
    if os.path.isfile(configuracion.ficheroPotencia):
        conexion = sqlite3.connect(configuracion.ficheroPotencia)
        conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")

        # Lista de tablas
        cursor = conexion.cursor()
        cursor.execute("pragma table_info(datos)")
        liFields = [uno[1].upper() for uno in cursor.fetchall()]
        if "LINE" not in liFields:
            sql = "ALTER TABLE datos ADD COLUMN LINE TEXT"
            cursor.execute(sql)
            conexion.commit()
            sql = "ALTER TABLE datos ADD COLUMN REF INT"
            cursor.execute(sql)
            conexion.commit()
        cursor.close()
        conexion.close()


def dicDisk(configuracion):
    backup = configuracion.carpeta + "/__BACKUP__"
    try:
        os.mkdir(backup)
    except:
        pass
    # comprobar si la tabla es unica si es asi crear nuevo fichero, sino, borrar tabla y generar nueva

    dicDisk_SQL(backup, configuracion.ficheroAlbumes)
    dicDisk_SQL(backup, configuracion.ficheroBoxing + "*")
    dicDisk_SQL(backup, configuracion.ficheroVariables)
    dicDisk_SQL(backup, configuracion.ficheroTrainings)
    dicDisk_SQL(backup, configuracion.fichEstadElo, tabla="color")
    dicDisk_SQL(backup, configuracion.fichEstadFicsElo, tabla="color")
    dicDisk_SQL(backup, configuracion.fichEstadFideElo, tabla="color")
    dicDisk_SQL(backup, configuracion.fichEstadMicElo, tabla="color")
    dicDisk_SQL(backup, configuracion.ficheroMate, siAllTables=True)

    # Cache
    dicDisk_SQL(backup, configuracion.ficheroAnalisisBookGuide, tabla="analisis")
    dicDisk_SQL(backup, configuracion.carpeta + "/*.tdb", siAllTables=True)
    dicDisk_SQL(backup, configuracion.ficheroLearnPGN)
    dicDisk_SQL(backup, configuracion.ficheroMoves)
    dicDisk_SQL(backup, configuracion.ficheroEntMaquinaConf)
    dicDisk_SQL(backup, configuracion.ficheroDailyTest, siAllTables=True)
    dicDisk_SQL(backup, configuracion.ficheroPotencia, tabla="parametros")
    dicDisk_SQL(backup, configuracion.ficheroPuntuaciones)
    dicDisk_SQL(backup, configuracion.carpeta + "/*.visdb")
    # Raw
    dicDisk_SQL(backup, configuracion.ficheroConfTableros)
    dicDisk_SQL(backup, configuracion.ficheroGMhisto)
    # dicDisk_SQL(backup, configuracion.ficheroRecursos, siAllTables=True)


def dicDisk_SQL(backup, path, tabla=None, siAllTables=False, pickle=True):
    if "*" in path:
        for uno in Util.listfiles(path):
            dicDisk_SQL(backup, uno, tabla, siAllTables, pickle)
        return
    if not os.path.isfile(path):
        return
    # Copia seguridad
    fdest = os.path.join(backup, os.path.basename(path))
    if not os.path.isfile(fdest):
        shutil.copy(path, fdest)

    conexion = sqlite3.connect(path)
    conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")

    # Lista de tablas
    cursor = conexion.cursor()
    cursor.execute('SELECT name FROM sqlite_master WHERE type = "table"')
    liRegs = cursor.fetchall()
    if liRegs:
        liTablas = [una[0] for una in liRegs]
        if not siAllTables:
            if tabla is None:
                tabla = "datos"
            else:
                tabla = tabla.lower()
            li = []
            for una in liTablas:
                if una.lower() == tabla:
                    li.append(una)
                    break
            liTablas = li
    else:
        liTablas = []

    for table in liTablas:
        # comprobamos que campos tiene, si son CLAVE, DATO, cambiamos as KEY, VALUE
        cursor = conexion.cursor()
        cursor.execute("pragma table_info(%s)" % table)
        liFields = [uno[1].upper() for uno in cursor.fetchall()]
        if len(liFields) == 2 and "CLAVE" in liFields and "DATO" in liFields:
            # Leemos todos los registros de table
            sql = "SELECT CLAVE,DATO FROM %s" % table
            cursor.execute(sql)
            liDatos = cursor.fetchall()
            sql = "DROP TABLE %s;" % table
            cursor.execute(sql)
            conexion.commit()
            cursor.close()
            if table.lower() == "datos":
                table = "data"
            cursor = conexion.cursor()
            sql = "CREATE TABLE %s( KEY TEXT PRIMARY KEY, VALUE TEXT );" % table
            try:
                cursor.execute(sql)
                conexion.commit()
            except:
                pass
            sql = "INSERT INTO %s (KEY,VALUE) values(?,?)" % table
            for row in liDatos:
                key = row[0]
                value = row[1]
                if not pickle:
                    value = cPickle.loads(value)
                value = base64.encodestring(value)
                cursor.execute(sql, (key, value))
            conexion.commit()
        cursor.close()

    cursor = conexion.cursor()
    cursor.execute("VACUUM")
    conexion.commit()
    cursor.close()
    conexion.close()


def compruebaCambioVersion(configuracion):
    v1 = "lk.pik"
    v2 = "lk2.pik"
    v30 = "lk30.pik"
    v35 = "lk35.pik"
    v40 = configuracion.carpeta + "/lk40.pik"
    v54 = configuracion.carpeta + "/lk54.pik"
    if os.path.isfile(v1):
        dic = Util.recuperaDIC(v1)
        Util.renombraNum(v1)
        if dic:
            dic["MAXNIVEL0"] = dic["MAXNIVEL"]
            dic["MAXNIVELHECHO0"] = dic["MAXNIVELHECHO"]
            del dic["MAXNIVEL"]
            del dic["MAXNIVELHECHO"]
            dic["MAXNIVEL1"] = "0" if dic["MAXNIVEL0"] == "1" else "1"
            dic["MAXNIVELHECHO1"] = ""
            dic["MAXNIVEL2"] = "0"
            dic["MAXNIVELHECHO2"] = ""
            dic["MAXNIVEL3"] = "0"
            dic["MAXNIVELHECHO3"] = ""
            dic["MAXNIVEL4"] = "0"
            dic["MAXNIVELHECHO4"] = ""
            dic["MAXNIVEL5"] = "0"
            dic["MAXNIVELHECHO5"] = ""
            Util.guardaDIC(dic, v2)

    if os.path.isfile(v2):
        dic = Util.recuperaDIC(v2)
        Util.renombraNum(v2)
        if dic:

            dic["JUGADOR"] = dic["USUARIO"]
            configuracion.dirSalvados = dic["DIRSALVADOS"]
            configuracion.dirPGN = dic["DIRSALVADOS"]
            configuracion.siSuenaBeep = dic["SIBEEP"] == "S"
            configuracion.traductor = dic["TRADUCTOR"].lower()
            dic["TIEMPOTUTOR"] = configuracion.tiempoTutor
            dic["SIBEEP"] = configuracion.siSuenaBeep
            dic["DIRSALVADOS"] = configuracion.dirSalvados
            dic["DIRPGN"] = configuracion.dirPGN
            dic["TRADUCTOR"] = configuracion.traductor
            dic["SALVAR_FICHERO"] = configuracion.salvarFichero
            dic["SALVAR_GANADOS"] = configuracion.salvarGanados
            dic["SALVAR_PERDIDOS"] = configuracion.salvarPerdidos
            dic["RIVAL"] = configuracion.rivalInicial
            dic["TUTOR"] = configuracion.tutorInicial

            rv = configuracion.buscaRival(configuracion.rivalInicial)
            for num, cat in enumerate(rv.categorias.lista):
                cat.nivelHecho = int(dic["MAXNIVEL%d" % num])
                cat.hecho = dic["MAXNIVELHECHO%d" % num]
            dic["RIVAL_%s" % configuracion.rivalInicial] = rv.graba()

            Util.guardaDIC(dic, v30)

    def cambiaRival(dic, antiguo, nuevo):
        claveN = "RIVAL_" + nuevo
        claveA = "RIVAL_" + antiguo
        if claveA in dic:
            cm = configuracion.buscaRival(nuevo)
            cm.lee(dic[claveA])
            del dic[claveA]
            dic[claveN] = cm.graba()

    def borraRival(dic, antiguo):
        claveA = "RIVAL_" + antiguo
        if claveA in dic:
            del dic[claveA]

    if os.path.isfile(v30):
        dic = Util.recuperaDIC(v30)
        Util.renombraNum(v30)
        if dic:
            cambiaRival(dic, "gfruit", "gaia")
            cambiaRival(dic, "pawny", "bikjump")
            cambiaRival(dic, "greko", "umko")
            cambiaRival(dic, "bison", "critter")
            borraRival(dic, "demon")
            dic["ID"] = Util.creaID()
            dic["SALVAR_FICHERO"] = ""
            dic["SALVAR_GANADOS"] = False
            dic["SALVAR_PERDIDOS"] = False
            Util.guardaDIC(dic, v35)

    if os.path.isfile(v35):

        def cambiaDir(fich):
            if os.path.isfile(fich):
                shutil.move(fich, configuracion.carpeta)

        cambiaDir("listaMotores.pkt")
        cambiaDir("entmaquina.pke")
        cambiaDir("memo.pk")
        cambiaDir("gm.pke")
        cambiaDir("cliente.pke")
        cambiaDir("remnueva.pke")
        cambiaDir("remoto.pke")

        dic = Util.recuperaDIC(v35)
        Util.renombraNum(v35)
        Util.guardaDIC(dic, v40)

    if os.path.isfile(v40):
        dic = Util.recuperaDIC(v40)

        for k in dic:
            if k.startswith("RIVAL_"):
                txt = dic[k]
                li = txt.split(",")
                for n in range(len(li)):
                    c = li[n]
                    if c.isdigit() and c != "0":
                        i = int(c) - 1
                        li[n] = str(i)
                dic[k] = ",".join(li)
                if k.upper() == "RIVAL_TARRASCH":
                    # tarrasch#PRINCIPIANTE,9,|AFICIONADO,8,|CANDIDATOMAESTRO,7,|MAESTRO,6,|CANDIDATOGRANMAESTRO,5,|GRANMAESTRO,4,
                    li = dic[k].split("|")
                    for n, t in enumerate(li):
                        li1 = t.split(",")
                        pt = int(li1[1])
                        maximo = 9 - n
                        if pt > maximo:
                            li1[1] = str(maximo)
                            li1[2] = ""  # hecho
                            li[n] = ",".join(li1)
                    dic[k] = "|".join(li)

        Util.renombraNum(v40)
        Util.guardaDIC(dic, configuracion.fichero)
        configuracion.lee()
        configuracion.graba()

    if os.path.isfile(v54):
        dic = Util.recuperaDIC(v54)

        def convierteTablero(xid, tamDef, txt):
            ct = BaseConfig.ConfigTablero(xid, tamDef)
            tema = ct._tema
            base = ct._base
            li = txt.split("#")
            nli = len(li)
            if nli >= 5:
                tema._colorExterior = int(li[0])
                ct.anchoPieza(int(li[1]))
                tipoPieza = li[2]
                dic = {
                    "c": "Chessicons",
                    "m": "Merida",
                    "i": "Internet",
                    "s": "Spatial",
                    "f": "Fantasy",
                    "n": "NikNak",
                }
                base._nomPiezas = dic.get(tipoPieza, "NikNak")
                tema._colorBlancas = int(li[3])
                tema._colorNegras = int(li[4])
                tema._colorTexto = tema._colorBlancas
                tema._colorFrontera = tema._colorExterior
                if nli >= 6:
                    txt = li[5]
                    if txt.isdigit():
                        tema._fTransicion.color = int(txt)
                    else:
                        tema._fTransicion = Util.txt2dic(txt)
                    if nli >= 7:  # version 6.0
                        tema._colorTexto = int(li[6])
                        if nli >= 8:  # version 6.1
                            tema._colorFrontera = int(li[7])
                            if nli >= 9:  # version 6.2
                                ct.anchoPieza(32)
            return ct

        ct = convierteTablero("BASE", 48, dic["TABLERON"])
        ct._base._nCoordenadas = 4 if dic.get("SICOORDENADAS4", True) else 2
        configuracion.cambiaConfTablero(ct)
        ct = convierteTablero("TUTOR", 16, dic["TABLEROT"])
        configuracion.cambiaConfTablero(ct)

        # Temas propios
        ficheroTemas = configuracion.ficheroTemas
        liTemas = Util.recuperaVar(ficheroTemas)
        if liTemas:
            for nTema, tema in enumerate(liTemas):
                if "CONFIG" in tema:
                    ct = convierteTablero("BASE", 48, tema["CONFIG"])  # temporal
                    tema["TEXTO"] = ct._tema.graba()
                    del tema["CONFIG"]
            Util.guardaVar(ficheroTemas, liTemas)

        Util.renombraNum(v54)
        Util.guardaDIC(dic, configuracion.fichero)
        configuracion.lee()
        configuracion.graba()


class ElemGM:
    def __init__(self):
        self.juega = ""
        self.partida = 0
        self.movimiento = ""
        self.coronacion = " "
        self.siguienteByte = 0
        self.siHijos = False

    def a1_num(self, a1):
        letra = a1[0]
        numero = int(a1[1])
        letra = ord(letra) - 96
        return letra * 10 + numero

    def num_a1(self, cnum):
        num = ord(cnum)
        letra = chr(int(num / 10) + 96)
        numero = str(num % 10)
        return letra + numero

    def tresBytes(self, n):
        n1 = n % 256
        n -= n1
        n /= 256
        n2 = n % 256
        n -= n2
        n /= 256
        n3 = n
        return n3, n2, n1

    def tres_bytes_num(self, c3, c2, c1):
        return ord(c3) * 256 * 256 + ord(c2) * 256 + ord(c1)

    def lee(self, f):
        bloque = f.read(8)
        desde = self.num_a1(bloque[0])
        hasta = self.num_a1(bloque[1])
        self.siguienteByte = self.tres_bytes_num(bloque[2], bloque[3], bloque[4])
        self.partida = self.tres_bytes_num("\0", bloque[5], bloque[6])

        n = ord(bloque[7])
        self.siHijos = n > 128
        if self.siHijos:
            n -= 128
        coronacion = chr(n).strip().lower()
        self.movimiento = (desde + hasta + coronacion).strip()


class GMconvert:
    def __init__(self, carpeta):
        self.carpeta = carpeta

    def exporta(self):
        for fich in os.listdir(self.carpeta):
            if fich.lower().endswith("gmi"):
                gm = fich[:-4]
                self.unGM(gm)

    def mueveFicheros(self):
        dbak = os.path.join(self.carpeta, "bak_gm")
        try:
            os.mkdir(dbak)
        except:
            pass
        for fich in os.listdir(self.carpeta):
            f4 = fich[-4:].lower()
            fichero = os.path.join(self.carpeta, fich)
            if os.path.isfile(fichero) and f4 in (".gmi", ".gmb", ".gmw"):
                try:
                    shutil.move(fichero, os.path.join(dbak, fich))
                except:
                    pass

    def borraFicheros(self):
        for fich in os.listdir(self.carpeta):
            f4 = fich[-4:].lower()
            if os.path.isfile(fich) and f4 in (".gmi", ".gmb", ".gmw"):
                fichero = os.path.join(self.carpeta, fich)
                try:
                    os.remove(fichero)
                except:
                    pass

    def validFile(self, fichero):
        return os.path.getsize(fichero) if os.path.isfile(fichero) else 0

    def unGM(self, gm):
        fichero = os.path.join(self.carpeta, "%s.gmi" % gm)
        if self.validFile(fichero):
            f = open(fichero)
            liPartidas = f.read().strip().split("\n")
            f.close()

            dicPVs = {}
            for siBlancas in (True, False):
                self.leePartidasPV(gm, dicPVs, siBlancas)

            numSiError = -1  # para que haya siempre una, por el historico
            for num, pv in dicPVs.iteritems():
                if pv:
                    numSiError = num
                    break
            repe = 0
            if numSiError >= 0:
                fichero = os.path.join(self.carpeta, "%s.xgm" % gm)
                q = open(fichero, "wb")
                for num, datos in enumerate(liPartidas):
                    pv = dicPVs.get(num, None)
                    if not pv:
                        pv = dicPVs[numSiError]
                        datos = liPartidas[numSiError]
                        repe += 1
                    datos = datos.replace("|", "-").replace(VarGen.XSEP, "|")
                    q.write("%s||%s\n" % (LCEngine.pv2xpv(pv.strip()), datos))
                q.close()

    def leePartidasPV(self, gm, dicPVs, siBlancas):
        color = "w" if siBlancas else "b"

        ficheroGM = os.path.join(self.carpeta, gm + ".gm" + color)
        if not self.validFile(ficheroGM):
            return
        f = open(ficheroGM, "rb")

        def lee(offset, pvBase):
            f.seek(offset)
            nElem = ord(f.read(1))
            liElementos = []
            for i in range(nElem):
                el = ElemGM()
                el.lee(f)  # hay que leerlos seguidos
                liElementos.append(el)
            for i in range(nElem):
                el = liElementos[i]
                move = el.movimiento
                siguiente = el.siguienteByte
                if siguiente <= offset:
                    dicPVs[el.partida] = pvBase + " " + move
                else:
                    lee(siguiente, pvBase + " " + move)

        lee(0, "")

        f.close()


def gmConvert(configuracion):
    # GM solo se borran
    gmc = GMconvert("GM")
    gmc.borraFicheros()

    # Personales, se convierten
    gmc = GMconvert(configuracion.dirPersonalTraining)
    gmc.exporta()
    gmc.mueveFicheros()
