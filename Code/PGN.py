import collections
import os

from Code import ControlPosicion
from Code import Jugada
from Code import PGNreader
from Code import Partida
from Code.QT import QTUtil2
import Code.SQL.Base as SQLBase
from Code import Util


class UnPGN:
    def __init__(self, dic=None):
        self.reset(dic)

    def reset(self, dic=None):
        self.dic = Util.SymbolDict(dic)
        self.texto = ""

        self.siError = False

    def listaCabeceras(self):
        li = []
        for k, v in self.dic.iteritems():
            li.append([k, v])
        return li

    def leeTexto(self, texto):
        game = PGNreader.read1Game(unicode(texto))
        if game.erroneo:
            return False
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(game.fen)
        p = Partida.Partida(cp)
        p.firstComment = game.moves.firstComment

        for mv in game.moves.liMoves:
            cpBase = ControlPosicion.ControlPosicion()
            cpBase.leeFen(mv.fenPrev)
            cpActual = ControlPosicion.ControlPosicion()
            cpActual.leeFen(mv.fen)
            jg = Jugada.Jugada()
            jg.ponDatos(cpBase, cpActual, mv.desde, mv.hasta, mv.coronacion)

            if mv.criticas:
                li = []
                for una in mv.criticas:
                    if una:
                        if una.isdigit():
                            li.append(una)
                        elif una[0] == "$":
                            una = una[1:]
                            if una:
                                li.append(una)
                        elif una[0] in "?!":
                            jg.criticaDirecta = una
                jg.critica = " ".join(li)
            if mv.comentarios:
                jg.comentario = "\n".join(mv.comentarios)
            if mv.variantes:
                li = []
                for una in mv.variantes:
                    li.append(una.toPGN())
                jg.variantes = "\n\n".join(li)
            p.append_jg(jg)
        if game.moves:
            p.ultPosicion = cpActual if game.moves.liMoves else cp.copia()
        self.partida = p
        self.dic = game.labels
        self.siError = False
        self.texto = texto
        return True

    def variable(self, clave, defecto=""):
        return self.dic.get(clave, defecto)

    def numPlies(self, jugadas):
        game = PGNreader.read1Game(jugadas)
        return game.plies()

    def leeCabecera(self, linea):
        linea = linea.strip()
        if linea.startswith("[") and linea.endswith("]"):
            li = linea[1:-1].replace('""', '"').split('"')
            if len(li) == 3:
                clave = li[0].strip()
                valor = li[1].strip()
                if clave and valor:
                    self.dic[clave] = valor


class PGN:
    def __init__(self, configuracion):
        self.cdir = configuracion.dataDB()
        self.cpkd = os.path.join(self.cdir, "dataDB8.pkd")
        self.maxdb = 10
        self.leePKD()

    def leePKD(self):
        # Comprueba si existe la carpeta dataDB, sino la crea
        if not os.path.isdir(self.cdir):
            os.mkdir(self.cdir)

        # Comprueba si existe en dataDB - dataDB.pk
        if os.path.isfile(self.cpkd):
            self.liDatos = Util.recuperaDIC(self.cpkd)
        else:
            self.liDatos = []

    def damePosicion(self, fichero):
        for n, uno in enumerate(self.liDatos):
            if uno["FICHERO"] == fichero:
                return n
        if len(self.liDatos) >= self.maxdb:
            nPrimero = -1
            fecha = "99999"
            for n, uno in enumerate(self.liDatos):
                try:
                    if uno["TAM"] == 0 or uno["FICHERO"] == "" or "FECHA" not in uno:
                        return n
                    if uno["FECHA"] < fecha:
                        fecha = uno["FECHA"]
                        nPrimero = n
                except:
                    return n
            return nPrimero

        # nuevo
        uno = {}
        uno["FICHERO"] = ""
        self.liDatos.append(uno)
        return len(self.liDatos) - 1

    def borraReferenciaA(self, fichero):
        n = self.damePosicion(fichero)
        self.liDatos[n]["TAM"] = 0
        Util.guardaDIC(self.liDatos, self.cpkd)

    def compruebaPosicion(self, fichero, uno):
        # Mismo fichero
        if uno["FICHERO"] != fichero:
            return False
        # Mismo tama_o
        tam = Util.tamFichero(uno["FICHERO"])
        if tam != uno["TAM"]:
            return False
        # Misma fecha
        fecha = os.path.getmtime(uno["FICHERO"])
        if fecha != uno["FECHA"]:
            return False
        # Que exista el DB
        if not os.path.isfile(uno["PATHDB"]):
            return False
        return True

    def leeFichero(self, ventana, fichero):

        fichero = os.path.abspath(fichero)

        # Pedimos la posicion que le corresponde
        nPos = self.damePosicion(fichero)

        uno = self.liDatos[nPos]

        if not self.compruebaPosicion(fichero, uno):
            uno["FICHERO"] = fichero
            uno["TAM"] = Util.tamFichero(fichero)
            uno["FECHA"] = os.path.getmtime(uno["FICHERO"])
            uno["PATHDB"] = self.cdir + "/pgnDB%03d.db3" % nPos
            if self.creaDB(ventana, fichero, uno):
                Util.guardaDIC(self.liDatos, self.cpkd)
            else:
                return None

        return uno

    def creaDB(self, ventana, fichero, uno):

        titulo = os.path.basename(fichero)
        tmpBP = QTUtil2.BarraProgreso(ventana, titulo, _("Reading..."), Util.tamFichero(fichero)).mostrar()

        dClaves = Util.SymbolDict()  # contiene tam maximo de los campos a mostrar

        def iniDB():
            fichDB = uno["PATHDB"]
            Util.borraFichero(fichDB)
            bd = SQLBase.DBBase(fichDB)

            tb = SQLBase.TablaBase("GAMES")
            tb.liCampos = []
            for clave in dClaves:
                tb.liCampos.append(SQLBase.Campo(clave.upper(), 'VARCHAR'))
            if "PLIES" not in dClaves:
                tb.liCampos.append(SQLBase.Campo("PLIES", 'VARCHAR'))
                dClaves["PLIES"] = 4
            if "PGN" not in dClaves:
                tb.liCampos.append(SQLBase.Campo("PGN", 'TEXT'))
            cursor = bd.conexion.cursor()
            tb.crearBase(cursor)
            cursor.close()
            dbf = bd.dbf("GAMES", (",".join(dClaves.keys())) + ",PGN")
            return bd, dbf

        jg = 0
        dbf = None
        bd = None
        for g in PGNreader.readGames(fichero):

            if tmpBP.siCancelado():
                break

            tmpBP.pon(g.nbytes)

            if g.erroneo:
                continue

            if not dbf:
                for clave, valor in g.labels.iteritems():
                    if valor == "?":
                        continue
                    dClaves[clave] = len(valor)
                bd, dbf = iniDB()

            else:
                for clave, valor in g.labels.iteritems():
                    if valor == "?":
                        continue
                    tam = len(valor)
                    if clave not in dClaves:
                        dbf.nuevaColumna(clave.upper(), 'VARCHAR')
                        dClaves[clave] = tam
                    else:
                        if dClaves[clave] < tam:
                            dClaves[clave] = tam

            dic = {}
            for k, v in g.labels.iteritems():
                if v == "?":
                    continue
                dic[k] = v
            dic["PGN"] = g.pgn
            pv = g.pv()
            dic["PLIES"] = pv.count(" ") + 1 if pv else 0
            jg += 1
            dbf.soloGrabar(dic, jg % 10000 == 0)

        if not dbf:
            bd, dbf = iniDB()
        dbf.commit()  # Graba los ultimos
        dbf.cerrar()
        bd.cerrar()

        tmpBP.close()

        uno["DCLAVES"] = dClaves

        return True


def leeEntDirigidoBase(fen, solucion):
    dicDirigidoFen = collections.OrderedDict()

    pgn = UnPGN()
    pgn.leeTexto('[FEN "%s"]\n%s' % (fen, solucion))

    st = set()

    def hazPartida(partida, siMain):
        for jg in partida.liJugadas:
            fenBase = jg.posicionBase.fen()
            if fenBase not in dicDirigidoFen:
                dicDirigidoFen[fenBase] = []
            liDDF = dicDirigidoFen[fenBase]

            # Comprobamos si no hay una variante que nos lleve a la misma posicion
            siMas = True
            mv = jg.movimiento()
            for n, (sm, j) in enumerate(liDDF):
                if j.movimiento() == mv:
                    siMas = False
                    if siMain:
                        liDDF[n] = (siMain, jg)
                        if siMain:
                            st.add(fenBase)
                        break
            if siMas:
                liDDF.append((siMain, jg))
                if siMain:
                    st.add(fenBase)

            variantes = jg.variantes
            if variantes:
                for variante in variantes.split("\n"):
                    uno = UnPGN()
                    if uno.leeTexto('[FEN "%s"]\n%s' % (fenBase, variante)):
                        hazPartida(uno.partida, False)

    partida = pgn.partida
    hazPartida(partida, True)

    return dicDirigidoFen, len(st)


def leeEntDirigido(fen, solucion):
    """
    Utilizado en GestorEntPos y GestorEntTac, para crear un diccionario y usarlo en un entrenamiento dirigido
    """
    return leeEntDirigidoBase(fen, solucion)[0]


def leeEntDirigidoM2(fen, solucion):
    dic = leeEntDirigidoBase(fen, solucion)[0]
    d = collections.OrderedDict()
    if dic:
        for fen in dic:
            sp1 = fen.rfind(" ")
            sp2 = fen.rfind(" ", 0, sp1)
            fenM2 = fen[:sp2]
            d[fenM2] = dic[fen]
    return d


def leeEntDirigidoBaseM2(fen, solucion):
    dic, nMoves = leeEntDirigidoBase(fen, solucion)
    d = collections.OrderedDict()
    if dic:
        for fen in dic:
            sp1 = fen.rfind(" ")
            sp2 = fen.rfind(" ", 0, sp1)
            fenM2 = fen[:sp2]
            d[fenM2] = dic[fen]
    return d, nMoves


def rawPGN(pgn):
    g = PGNreader.read1Game(pgn)
    p = Partida.Partida(fen=g.fen)
    p.leerPV(g.pv())

    txt = ""
    for k, v in g.labels.iteritems():
        txt += "[%s \"%s\"]\n" % (k, v)
    txt += "\n\n"
    txt += p.pgnBase()

    return txt


def pgn_partida(fen, solucion):
    pgn = UnPGN()
    pgn.leeTexto('[FEN "%s"]\n%s' % (fen, solucion))
    partida = pgn.partida
    for jg in partida.liJugadas:
        jg.pvariantes = pvariantes = []
        variantes = jg.variantes
        if variantes:
            fenBase = jg.posicionBase.fen()
            for variante in variantes.split("\n"):
                uno = UnPGN()
                uno.leeTexto('[FEN "%s"]\n%s' % (fenBase, variante))
                if not uno.siError and uno.partida.numJugadas():
                    pvariantes.append(uno.partida)

    return partida
