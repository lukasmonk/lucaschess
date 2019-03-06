import atexit
import os
import sqlite3

import LCEngine4 as LCEngine

from Code import AperturasStd
from Code import Books
from Code import ControlPosicion
from Code import PGNreader
from Code.QT import QTUtil2
from Code.QT import QTVarios
import Code.SQL.DBF as SQLDBF
from Code import Util
from Code import VarGen


class UnMove:
    def __init__(self, bookGuide, father):
        self.bookGuide = bookGuide
        self.dbAnalisis = bookGuide.dbAnalisis
        self._rowid = None
        self._father = father
        self._pv = ""
        self._xpv = ""
        self._nag = 0
        self._adv = 0
        self._comment = ""
        self._pos = 0
        self._xdata = {}
        self._graphics = ""
        self._mark = ""

        self._children = []

        self._item = None

        if father:
            self._siBlancas = not father.siBlancas()
            self._numJugada = father.numJugada() + (1 if self._siBlancas else 0)
        else:  # root
            self._siBlancas = False
            self._numJugada = 0

        self._fen = ""
        self._pgn = ""  # set al crear el pv

        self.readedEXT = False

    def __str__(self):
        return "%s %s %s %s %s %s %s" % (
            self.rowid(), self.father().rowid(), self.pv(), self.xpv(), self.siBlancas(), self.numJugada(), self.fen())

    def rowid(self, valor=None):
        if valor is not None:
            self._rowid = valor
        return self._rowid

    def siBlancas(self):
        return self._siBlancas

    def numJugada(self):
        return self._numJugada

    def father(self):
        return self._father

    def pv(self, valor=None):
        if valor is not None:
            self._pv = valor
        return self._pv

    def xpv(self, valor=None):
        if valor is not None:
            self._xpv = valor
        return self._xpv

    def mark(self, valor=None, siLeyendo=False):
        if valor is not None:
            ant = self._mark
            self._mark = valor
            if not siLeyendo:
                if ant != valor:
                    self.bookGuide.pteGrabar(self)
                self.bookGuide.actualizaBookmark(self, len(valor) > 0)
        return self._mark

    def graphics(self, valor=None, siLeyendo=False):
        if valor is None:
            if not self.readedEXT:
                self.readEXT()
        else:
            ant = self._graphics
            self._graphics = valor
            if not siLeyendo and ant != valor:
                self.bookGuide.pteGrabar(self)
        return self._graphics

    def nag(self, valor=None, siLeyendo=False):
        if valor is not None:
            ant = self._nag
            self._nag = valor
            if not siLeyendo and ant != valor:
                self.bookGuide.pteGrabar(self)
        return self._nag

    def adv(self, valor=None, siLeyendo=False):
        if valor is not None:
            ant = self._adv
            self._adv = valor
            if not siLeyendo and ant != valor:
                self.bookGuide.pteGrabar(self)
        return self._adv

    def comment(self, valor=None, siLeyendo=False):
        if valor is None:
            self.readEXT()
        else:
            ant = self._comment
            self._comment = valor
            if not siLeyendo and ant != valor:
                self.bookGuide.pteGrabar(self)
        return self._comment

    def commentLine(self):
        c = self.comment()
        if c:
            li = c.split("\n")
            c = li[0]
            if len(li) > 1:
                c += "..."
        return c

    def xdata(self, valor=None, siLeyendo=False):
        if valor is None:
            self.readEXT()
        else:
            ant = self._xdata
            self._xdata = valor
            if not siLeyendo and ant != valor:
                self.bookGuide.pteGrabar(self)
            self._xdata = valor
        return self._xdata

    def pos(self, valor=None, siLeyendo=False):
        if valor is not None:
            ant = self._pos
            self._pos = valor
            if not siLeyendo and ant != valor:
                self.bookGuide.pteGrabar(self)
        return self._pos

    def fen(self, valor=None):
        if valor is not None:
            self._fen = valor
            self.bookGuide.setTransposition(self)
        return self._fen

    def fenM2(self):
        fen = self._fen
        sp2 = fen.rfind(" ", 0, fen.rfind(" "))
        return fen[:sp2]

    def transpositions(self):
        return self.bookGuide.getTranspositions(self)

    def fenBase(self):
        return self._father._fen

    def pgn(self):
        if not self._pgn:
            pv = self._pv
            d, h, c = pv[:2], pv[2:4], pv[4:]
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self._father.fen())
            self._pgn = cp.pgnSP(d, h, c)
            cp.mover(d, h, c)
            self._fen = cp.fen()
        return self._pgn

    def pgnEN(self):
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self._father.fen())
        pv = self._pv
        d, h, c = pv[:2], pv[2:4], pv[4:]
        return cp.pgn(d, h, c)

    def pgnNum(self):
        if self._siBlancas:
            return "%d.%s" % (self._numJugada, self.pgn())
        return self.pgn()

    def item(self, valor=None):
        if valor is not None:
            self._item = valor
        return self._item

    def children(self):
        return self._children

    def addChildren(self, move):
        self._children.append(move)
        self._children = sorted(self._children, key=lambda uno: uno._pos)

    def delChildren(self, move):
        for n, mv in enumerate(self._children):
            if move == mv:
                del self._children[n]
                return

    def brothers(self):
        li = []
        for mv in self._father.children():
            if mv.pv() != self.pv():
                li.append(mv)
        return li

    def analisis(self):
        return self.dbAnalisis.move(self)

    def etiPuntos(self):
        rm = self.dbAnalisis.move(self)
        if rm:
            return rm.abrTextoBase()
        else:
            return ""

    def historia(self):
        li = []
        p = self
        while True:
            li.insert(0, p)
            if not p.father():
                break
            p = p.father()
        return li

    def allPV(self):
        return LCEngine.xpv2pv(self._xpv)

    def allPGN(self):
        li = []
        for mv in self.historia():
            if mv._pv:
                if mv.siBlancas():
                    li.append("%d." % mv.numJugada())
                li.append(mv.pgn())
        return " ".join(li)

    def readEXT(self):
        if self.readedEXT:
            return
        self.readedEXT = True
        self.bookGuide.readEXT(self)


class OpeningGuide:
    def __init__(self, wowner, nomFichero=None):

        self.configuracion = VarGen.configuracion

        siGenerarStandard = False
        if nomFichero is None:
            nomFichero = self.configuracion.ficheroBookGuide
            if not os.path.isfile(nomFichero):
                siGenerarStandard = "Standard opening guide" in nomFichero

        self.name = os.path.basename(nomFichero)[:-4]

        self.ultPos = 0
        self.dicPtes = {}
        self.nomFichero = nomFichero
        self.conexion = sqlite3.connect(nomFichero)
        self.conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        atexit.register(self.cerrar)
        self.tablaDatos = "GUIDE"

        self.checkInitBook(wowner, siGenerarStandard)

        self.transpositions = {}
        self.bookmarks = []

        self.dbAnalisis = DBanalisis()

    def pathGuide(self, nameGuide):
        return Util.dirRelativo(os.path.join(self.configuracion.carpeta, nameGuide + ".pgo"))

    def getOtras(self):
        li = Util.listdir(self.configuracion.carpeta)
        lwbase = self.name.lower()
        liresp = []
        for uno in li:
            lw = uno.name.lower()
            if lw.endswith(".pgo"):
                if lwbase != lw[:-4]:
                    liresp.append(uno.name[:-4])
        return liresp

    def getTodas(self):
        li = self.getOtras()
        li.append(self.name)
        return li

    def changeTo(self, wowner, nomGuide):
        self.grabar()
        nomFichero = self.pathGuide(nomGuide)
        self.name = nomGuide

        self.ultPos = 0
        self.dicPtes = {}
        self.nomFichero = nomFichero
        self.conexion.close()
        self.conexion = sqlite3.connect(nomFichero)
        self.conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        atexit.register(self.cerrar)

        self.checkInitBook(wowner, False)

        self.transpositions = {}
        self.bookmarks = []

        self.root = UnMove(self, None)
        self.root._fen = ControlPosicion.FEN_INICIAL
        self.readAllDB()

        self.configuracion.ficheroBookGuide = nomFichero
        self.configuracion.graba()

    def copyTo(self, otraGuide):
        self.grabar()
        otroFichero = self.pathGuide(otraGuide)
        Util.copiaFichero(self.nomFichero, otroFichero)

    def renameTo(self, wowner, otraGuide):
        self.grabar()
        self.conexion.close()
        otroFichero = self.pathGuide(otraGuide)
        Util.renombraFichero(self.nomFichero, otroFichero)
        self.changeTo(wowner, otraGuide)

    def removeOther(self, otraGuide):
        self.grabar()
        otroFichero = self.pathGuide(otraGuide)
        Util.borraFichero(otroFichero)

    def appendFrom(self, wowner, otraGuide):
        self.grabar()

        otroFichero = self.pathGuide(otraGuide)
        otraConexion = sqlite3.connect(otroFichero)
        otraConexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")

        cursor = otraConexion.cursor()
        cursor.execute("pragma table_info(%s)" % self.tablaDatos)
        liCamposOtra = cursor.fetchall()
        cursor.close()
        if not liCamposOtra:
            return False

        st = set()
        for x in liCamposOtra:
            st.add(x[1])  # nombre
        liselect = ("XPV", "PV", "NAG", "ADV", "COMMENT", "FEN", "MARK", "GRAPHICS", "XDATA")
        libasic = ("XPV", "PV", "FEN")
        li = []
        for x in liselect:
            if x not in st:
                if x in libasic:
                    otraConexion.close()
                    QTUtil2.mensError(wowner, _("This guide file is not valid"))
                    return False
            else:
                li.append(x)
        select = ",".join(li)
        dbfOtra = SQLDBF.DBF(otraConexion, self.tablaDatos, select)
        dbfOtra.leer()
        reccount = dbfOtra.reccount()

        bp = QTUtil2.BarraProgreso(wowner, otraGuide, "", reccount).mostrar()

        liReg = []
        for recno in range(reccount):
            bp.pon(recno)
            if bp.siCancelado():
                break

            dbfOtra.goto(recno)
            reg = dbfOtra.registroActual()
            liReg.append(reg)
        dbfOtra.cerrar()
        otraConexion.close()

        def dispatch(recno):
            bp.pon(recno)

        if liReg:
            dbf = SQLDBF.DBF(self.conexion, self.tablaDatos, select)
            dbf.insertarLista(liReg, dispatch)
            dbf.cerrar()

        bp.cerrar()

        return len(liReg) > 0

    def generarStandard(self, ventana, siBasic):

        oLista = AperturasStd.apTrain
        dic = oLista.dic

        titulo = _("Openings")
        tmpBP2 = QTUtil2.BarraProgreso2(ventana, titulo)
        tf = len(dic)
        tmpBP2.ponTotal(1, tf)
        tmpBP2.ponRotulo(1, "1. " + _X(_("Reading %1"), titulo))
        tmpBP2.ponTotal(2, tf)
        tmpBP2.ponRotulo(2, "")
        tmpBP2.mostrar()

        liRegs = []
        dRegs = {}  # se guarda una lista con los pv, para determinar el padre
        for nR, k in enumerate(oLista.dic):
            tmpBP2.pon(1, nR)
            tmpBP2.siCancelado()
            ae = oLista.dic[k]
            if siBasic and not ae.siBasic:
                continue
            liPV = ae.a1h8.split(" ")

            ult = len(liPV) - 1
            seqFather = ""
            cp = ControlPosicion.ControlPosicion()
            cp.posInicial()
            for pos, pv in enumerate(liPV):
                desde, hasta, coronacion = pv[:2], pv[2:4], pv[4:]
                seq = seqFather + LCEngine.pv2xpv(pv)
                cp.mover(desde, hasta, coronacion)

                if seq not in dRegs:
                    reg = SQLDBF.Almacen()
                    reg.XPV = seq
                    reg.PV = pv
                    reg.FEN = cp.fen()
                    reg.COMMENT = ae.trNombre if pos == ult else ""
                    self.ultPos += 1
                    reg.POS = self.ultPos

                    liRegs.append(reg)
                    dRegs[seq] = reg

                seqFather = seq

        tmpBP2.ponRotulo(2, "2. " + _X(_("Converting %1"), titulo))
        tmpBP2.ponTotal(2, len(liRegs))

        select = "XPV,PV,COMMENT,FEN,POS"
        dbf = SQLDBF.DBF(self.conexion, self.tablaDatos, select)

        def dispatch(num):
            tmpBP2.pon(2, num)
            tmpBP2.siCancelado()

        dbf.insertarLista(liRegs, dispatch)
        dbf.cerrar()

        tmpBP2.cerrar()

    def creaTabla(self):
        cursor = self.conexion.cursor()
        sql = ("CREATE TABLE %s( XPV TEXT UNIQUE,PV VARCHAR(5),NAG INTEGER,ADV INTEGER,COMMENT TEXT,"
               "FEN VARCHAR,MARK VARCHAR, POS INTEGER,GRAPHICS TEXT,XDATA BLOB);") % self.tablaDatos
        cursor.execute(sql)
        self.conexion.commit()
        cursor.close()

    def checkInitBook(self, wowner, siGenerarStandard):
        cursor = self.conexion.cursor()
        cursor.execute("pragma table_info(%s)" % self.tablaDatos)
        liCampos = cursor.fetchall()
        cursor.close()
        if not liCampos:
            self.creaTabla()
            if siGenerarStandard:
                self.generarStandard(wowner, True)

    def grabarPGN(self, ventana, ficheroPGN, maxDepth):
        select = "XPV,PV,COMMENT,NAG,ADV,FEN,POS"
        SQLDBF.DBF(self.conexion, self.tablaDatos, select)

        erroneos = duplicados = importados = 0
        dlTmp = QTVarios.ImportarFicheroPGN(ventana)
        dlTmp.hideDuplicados()
        dlTmp.show()

        select = "XPV,PV,COMMENT,NAG,ADV,FEN,POS"
        dbf = SQLDBF.DBF(self.conexion, self.tablaDatos, select)
        dnag = {"!!": 3, "!": 1, "?": 2, "??": 4, "!?": 5, "?!": 6}

        n = 0
        liReg = []

        for n, g in enumerate(PGNreader.readGames(ficheroPGN), 1):
            if not dlTmp.actualiza(n, erroneos, duplicados, importados):
                break
            if g.erroneo:
                erroneos += 1
                continue
            if not g.moves:
                erroneos += 1
                continue

            liReg = []

            def addMoves(moves, depth, seq):
                for mv in moves.liMoves:
                    if depth > maxDepth:
                        break
                    seqM1 = seq
                    pv = mv.pv
                    seq += LCEngine.pv2xpv(pv)
                    reg = SQLDBF.Almacen()
                    reg.PV = pv
                    reg.XPV = seq
                    reg.COMMENT = "\n".join(mv.comentarios)
                    reg.FEN = mv.fen
                    reg.NAG = 0
                    reg.ADV = 0
                    self.ultPos += 1
                    reg.POS = self.ultPos
                    for critica in mv.criticas:
                        if critica.isdigit():
                            t = int(critica)
                            if t in (4, 2, 1, 3, 5, 6):
                                reg.NAG = t
                            elif t in (11, 14, 15, 16, 17, 18, 19):
                                reg.ADV = t
                        else:
                            if critica in dnag:
                                reg.NAG = dnag[critica]
                    liReg.append(reg)
                    if mv.variantes:
                        for variante in mv.variantes:
                            addMoves(variante, depth, seqM1)
                    depth += 1

            addMoves(g.moves, 1, "")
            if liReg:
                dbf.insertarLista(liReg, None)

        dbf.cerrar()
        dlTmp.actualiza(n, erroneos, duplicados, importados)
        dlTmp.ponContinuar()

        return len(liReg) > 0

    def grabarPolyglot(self, ventana, ficheroBIN, depth, whiteBest, blackBest):

        titulo = _("Import a polyglot book")
        tmpBP2 = QTUtil2.BarraProgreso2(ventana, titulo)
        tmpBP2.ponTotal(1, 1)
        tmpBP2.ponRotulo(1, "1. " + _X(_("Reading %1"), os.path.basename(ficheroBIN)))
        tmpBP2.ponTotal(2, 1)
        tmpBP2.ponRotulo(2, "")
        tmpBP2.mostrar()

        basePos = self.ultPos

        book = Books.Libro("P", ficheroBIN, ficheroBIN, True)
        book.polyglot()
        cp = ControlPosicion.ControlPosicion()

        lireg = []
        stFenM2 = set()  # para que no se produzca un circulo vicioso

        def hazFEN(fen, ply, seq):
            plyN = ply + 1
            siWhite = " w " in fen
            siMax = False
            if whiteBest:
                siMax = siWhite
            if blackBest:
                siMax = siMax or not siWhite

            liPV = book.miraListaPV(fen, siMax)
            for pv in liPV:
                cp.leeFen(fen)
                cp.mover(pv[:2], pv[2:4], pv[4:])
                fenN = cp.fen()
                reg = SQLDBF.Almacen()
                lireg.append(reg)
                reg.PV = pv
                seqN = seq + LCEngine.pv2xpv(pv)
                reg.XPV = seqN
                reg.COMMENT = ""
                reg.NAG = 0
                reg.FEN = fenN
                reg.ADV = 0
                self.ultPos += 1
                reg.POS = self.ultPos
                tmpBP2.ponTotal(1, self.ultPos - basePos)
                tmpBP2.pon(1, self.ultPos - basePos)
                if plyN < depth:
                    fenM2 = cp.fenM2()
                    if fenM2 not in stFenM2:
                        stFenM2.add(fenM2)
                        hazFEN(fenN, plyN, seqN)

        hazFEN(ControlPosicion.FEN_INICIAL, 0, "")
        select = "XPV,PV,COMMENT,NAG,ADV,FEN,POS"
        dbf = SQLDBF.DBF(self.conexion, self.tablaDatos, select)
        tmpBP2.ponTotal(2, len(lireg))
        tmpBP2.ponRotulo(2, _("Writing..."))

        def dispatch(num):
            tmpBP2.pon(2, num)

        dbf.insertarLista(lireg, dispatch)

        dbf.cerrar()
        tmpBP2.cerrar()

        return len(lireg) > 0

    def reset(self):
        self.grabar()
        self.dicPtes = {}
        self.root = UnMove(self, None)
        self.root._fen = ControlPosicion.FEN_INICIAL
        self.readAllDB()

    def readAllDB(self):
        self.transpositions = {}
        self.bookmarks = []
        self.ultPos = 0
        select = "ROWID,XPV,PV,NAG,ADV,FEN,MARK,POS"
        orden = "XPV"
        condicion = ""
        dbf = SQLDBF.DBFT(self.conexion, self.tablaDatos, select, condicion, orden)
        dbf.leer()

        dicMoves = {}
        dicMoves[""] = self.root
        for recno in range(dbf.reccount()):
            dbf.goto(recno)
            xpv = dbf.XPV
            pv = dbf.PV
            if not pv:
                self.root.rowid(dbf.ROWID)
                continue
            xpvfather = xpv[:-2 if len(pv) == 4 else -3]
            if xpvfather in dicMoves:
                father = dicMoves[xpvfather]
                mv = UnMove(self, father)
                mv.pv(pv)
                mv.xpv(xpv)
                mv.fen(dbf.FEN)
                mv.rowid(dbf.ROWID)
                mv.nag(dbf.NAG, True)
                mv.adv(dbf.ADV, True)
                mark = dbf.MARK
                if mark:
                    self.bookmarks.append(mv)
                mv.mark(dbf.MARK, True)
                mv.pos(dbf.POS, True)
                if dbf.POS >= self.ultPos:
                    self.ultPos = dbf.POS
                dicMoves[xpv] = mv
                father.addChildren(mv)

        dbf.cerrar()

    def setTransposition(self, move):
        fenM2 = move.fenM2()
        if fenM2 not in self.transpositions:
            self.transpositions[fenM2] = [move]
        else:
            li = self.transpositions[fenM2]
            if move not in li:
                li.append(move)

    def getTranspositions(self, move):
        li = self.transpositions.get(move.fenM2(), [])
        if len(li) <= 1:
            return []
        n = li.index(move)
        li = li[:]
        del li[n]
        return li

    def getMovesFenM2(self, fenM2):
        return self.transpositions.get(fenM2, None)

    def actualizaBookmark(self, move, siPoner):
        siEsta = move in self.bookmarks

        if siEsta:
            if not siPoner:
                del self.bookmarks[self.bookmarks.index(move)]
        else:
            if siPoner:
                self.bookmarks.append(move)

    def readEXT(self, move):
        select = "COMMENT,GRAPHICS,XDATA"
        condicion = "XPV='%s'" % move.xpv()
        dbf = SQLDBF.DBFT(self.conexion, self.tablaDatos, select, condicion)
        dbf.leer()
        if dbf.reccount():
            dbf.goto(0)
            move.comment(dbf.COMMENT, True)
            move.graphics(dbf.GRAPHICS, True)
            move.xdata(Util.blob2var(dbf.XDATA), True)
        dbf.cerrar()

    def pteGrabar(self, move):
        huella = move.xpv()
        if huella not in self.dicPtes:
            self.dicPtes[huella] = move
            if len(self.dicPtes) > 5:
                self.grabar()

    def mixTable(self, tableFrom, tableTo):
        self.grabar()
        nameFrom = "DATA%d" % tableFrom
        nameTo = "DATA%d" % tableTo
        cursor = self.conexion.cursor()
        cursor.execute("SELECT ROWID,XPV FROM %s" % nameFrom)
        liValores = cursor.fetchall()
        for rowid, xpv in liValores:
            cursor.execute('SELECT ROWID FROM %s WHERE XPV="%s"' % (nameTo, xpv))
            li = cursor.fetchone()
            if li:
                rowidTo = li[0]
                sql = "DELETE FROM %s WHERE rowid = %d" % (nameTo, rowidTo)
                cursor.execute(sql)
            sql = "INSERT INTO %s SELECT * FROM %s WHERE %s.ROWID = %d;" % (nameTo, nameFrom, nameFrom, rowid)
            cursor.execute(sql)
        self.conexion.commit()
        cursor.close()

    def grabarFichSTAT(self, nomGuide, fich):
        # Para convertir datos de games a bookGuide
        self.changeTo(None, nomGuide)
        f = open(fich, "rb")
        liRegs = []
        for linea in f:
            linea = linea.strip()
            xpv, pv, fen = linea.split("|")
            reg = SQLDBF.Almacen()
            reg.XPV = xpv
            reg.PV = pv
            reg.FEN = fen
            self.ultPos += 1
            reg.POS = self.ultPos

            liRegs.append(reg)

        select = "XPV,PV,FEN,POS"
        dbf = SQLDBF.DBFT(self.conexion, self.tablaDatos, select)

        def dispatch(num):
            pass

        dbf.insertarLista(liRegs, dispatch)
        dbf.cerrar()

    def cerrar(self):
        if self.conexion:
            self.conexion.close()
            self.conexion = None
            self.dbAnalisis.cerrar()

    def grabar(self):
        if len(self.dicPtes) == 0:
            return
        dic = self.dicPtes
        self.dicPtes = {}
        # Creamos una tabla de trabajo
        dbf = SQLDBF.DBF(self.conexion, self.tablaDatos, "")
        for k, uno in dic.items():
            reg = SQLDBF.Almacen()
            reg.XPV = uno.xpv()
            reg.PV = uno.pv()
            reg.NAG = uno.nag()
            reg.ADV = uno.adv()
            reg.COMMENT = uno.comment()
            reg.POS = uno.pos()
            reg.FEN = uno.fen()
            reg.MARK = uno.mark()
            reg.GRAPHICS = uno.graphics()
            reg.XDATA = Util.var2blob(uno.xdata())
            if uno.rowid() is None:
                xid = dbf.insertarSoloReg(reg)
                uno.rowid(xid)
            else:
                dbf.modificarROWID(uno.rowid(), reg)
        dbf.cerrar()

    def dameMovimiento(self, father, pv):
        mv = UnMove(self, father)
        xpv = father.xpv() + LCEngine.pv2xpv(pv)
        mv.xpv(xpv)
        mv.pv(pv)
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(father.fen())
        cp.moverPV(pv)
        mv.fen(cp.fen())
        self.ultPos += 1
        mv.pos(self.ultPos)
        father.addChildren(mv)
        self.pteGrabar(mv)
        return mv

    def borrar(self, uno):

        liBorrados = [uno]

        def allChildren(li, uno):
            for x in uno.children():
                li.append(x.rowid())
                liBorrados.append(x)
                allChildren(li, x)

        liRowid = []
        if uno.rowid():
            liRowid.append(uno.rowid())
        allChildren(liRowid, uno)
        if liRowid:
            dbf = SQLDBF.DBF(self.conexion, self.tablaDatos, "")
            dbf.borrarListaRaw(liRowid)
            if len(liRowid) > 10:
                dbf.pack()
            dbf.cerrar()

        liQuitarTrasposition = []
        for mov in liBorrados:
            if mov in self.bookmarks:
                del self.bookmarks[self.bookmarks.index(mov)]
            fenM2 = mov.fenM2()
            li = self.transpositions[fenM2]
            if len(li) <= 1:
                del self.transpositions[fenM2]
            else:
                del li[li.index(mov)]
                if len(li) == 1:
                    xm = li[0]
                    if xm not in liBorrados:
                        liQuitarTrasposition.append(xm)

        return liBorrados, liQuitarTrasposition

    def allLines(self):
        rt = self.root
        liT = []

        def uno(mv):
            li = mv.children()
            if li:
                for mv in li:
                    uno(mv)
            else:
                liT.append(mv.historia())

        uno(rt)

        return liT


class DBanalisis:
    def __init__(self):
        self.db = Util.DicSQL(VarGen.configuracion.ficheroAnalisisBookGuide, tabla="analisis", maxCache=1024)

    def cerrar(self):
        self.db.close()

    def lista(self, fenM2):
        dic = self.db[fenM2]
        if dic:
            lista = dic.get("LISTA", None)
            activo = dic.get("ACTIVO", None)
        else:
            lista = None
            activo = None
        return lista, activo

    def mrm(self, fenM2):
        dic = self.db[fenM2]
        if dic:
            lista = dic.get("LISTA", None)
            if lista:
                nactive = dic.get("ACTIVO", None)
                if nactive is not None:
                    return lista[nactive]
        return None

    def move(self, move):
        fenM2 = move.father().fenM2()
        dic = self.db[fenM2]
        if dic:
            numactivo = dic.get("ACTIVO", None)
            if numactivo is not None:
                lista = dic.get("LISTA", None)
                if lista:
                    if 0 < numactivo >= len(lista):
                        numactivo = 0
                        dic["ACTIVO"] = 0
                        self.db[fenM2] = dic
                    analisis = lista[numactivo]
                    if analisis:
                        rm, k = analisis.buscaRM(move.pv())
                        return rm
        return None

    def getAnalisis(self, fenM2):
        dic = self.db[fenM2]
        if not dic:
            dic = {"ACTIVO": None, "LISTA": []}
        return dic

    def nuevo(self, fenM2, analisis):
        dic = self.getAnalisis(fenM2)
        li = dic["LISTA"]
        li.append(analisis)
        dic["ACTIVO"] = len(li) - 1
        self.db[fenM2] = dic

    def pon(self, fenM2, numActivo):
        dic = self.getAnalisis(fenM2)
        dic["ACTIVO"] = numActivo
        self.db[fenM2] = dic

    def activo(self, fenM2):
        dic = self.getAnalisis(fenM2)
        return dic["ACTIVO"]

    def quita(self, fenM2, num):
        dic = self.getAnalisis(fenM2)
        li = dic["LISTA"]
        del li[num]
        numActivo = dic["ACTIVO"]
        if numActivo is not None:
            if numActivo == num:
                numActivo = None
            elif numActivo > num:
                numActivo -= 1
        dic["ACTIVO"] = numActivo
        self.db[fenM2] = dic
