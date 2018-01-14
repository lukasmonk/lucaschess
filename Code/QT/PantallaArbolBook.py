import os

from PyQt4 import QtGui, QtCore

from Code import Books
from Code import ControlPosicion
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import VarGen


class UnMove:
    def __init__(self, listaMovesPadre, book, fenBase, movBook):

        self.listaMovesPadre = listaMovesPadre
        self.listaMovesHijos = None
        self.book = book
        self.fenBase = fenBase
        self.desde, self.hasta, self.coronacion, rotulo, self.ratio = movBook
        rotulo = rotulo.replace("-", "").strip()
        while "  " in rotulo:
            rotulo = rotulo.replace("  ", " ")
        self.pgn, self.porcentaje, self.absoluto = rotulo.split(" ")
        self.porcentaje += "  " * listaMovesPadre.nivel
        self.absoluto += "  " * listaMovesPadre.nivel

        pv = self.desde + self.hasta + self.coronacion

        self.partida = listaMovesPadre.partidaBase.copia()
        self.partida.leerPV(pv)

        self.item = None

        self.posActual = self.partida.numJugadas() - 1

    def row(self):
        return self.listaMovesPadre.liMoves.index(self)

    def analisis(self):
        return self.listaMovesPadre.analisisMov(self)

    def conHijosDesconocidos(self, dbCache):
        if self.listaMovesHijos:
            return False
        fenM2 = self.partida.ultPosicion.fenM2()
        return fenM2 in dbCache

    def etiPuntos(self, siExten):
        pts = self.listaMovesPadre.etiPuntosUnMove(self, siExten)
        if not siExten:
            return pts
        nom = self.listaMovesPadre.nomAnalisis()
        if nom:
            return nom + ": " + pts
        else:
            return ""

    def creaHijos(self):
        self.listaMovesHijos = ListaMoves(self, self.book, self.partida.ultPosicion.fen())
        return self.listaMovesHijos

    def inicio(self):
        self.posActual = -1

    def atras(self):
        self.posActual -= 1
        if self.posActual < -1:
            self.inicio()

    def adelante(self):
        self.posActual += 1
        if self.posActual >= self.partida.numJugadas():
            self.final()

    def final(self):
        self.posActual = self.partida.numJugadas() - 1

    def numVariantes(self):
        return len(self.variantes)

    def damePosicion(self):
        if self.posActual == -1:
            posicion = self.partida.iniPosicion
            desde, hasta = None, None
        else:
            jg = self.partida.jugada(self.posActual)
            posicion = jg.posicion
            desde = jg.desde
            hasta = jg.hasta
        return posicion, desde, hasta

    def ponValoracion(self, valoracion):
        self.valoracion = valoracion

    def ponComentario(self, comentario):
        self.comentario = comentario


class ListaMoves:
    def __init__(self, moveOwner, book, fen):

        if not moveOwner:
            self.nivel = 0
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(fen)
            self.partidaBase = Partida.Partida(cp)
        else:
            self.nivel = moveOwner.listaMovesPadre.nivel + 1
            self.partidaBase = moveOwner.partida.copia()

        self.book = book
        self.fen = fen
        self.moveOwner = moveOwner
        book.polyglot()
        liMovesBook = book.miraListaJugadas(fen)
        self.liMoves = []
        for uno in liMovesBook:
            self.liMoves.append(UnMove(self, book, fen, uno))

    def cambiaLibro(self, book):
        self.book = book
        book.polyglot()
        liMovesBook = book.miraListaJugadas(self.fen)
        self.liMoves = []
        for uno in liMovesBook:
            self.liMoves.append(UnMove(self, book, self.fen, uno))

    def siEstaEnLibro(self, book):
        book.polyglot()
        liMovesBook = book.miraListaJugadas(self.fen)
        return len(liMovesBook) > 0


class TreeMoves(QtGui.QTreeWidget):
    def __init__(self, owner):
        QtGui.QTreeWidget.__init__(self)
        self.owner = owner
        self.setAlternatingRowColors(True)
        self.listaMoves = owner.listaMoves
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.menuContexto)

        self.setHeaderLabels((_("Moves"), "", _("Games"), "", ""))
        self.setColumnHidden(3, True)

        ftxt = Controles.TipoLetra(puntos=9)

        self.setFont(ftxt)

        self.connect(self, QtCore.SIGNAL("currentItemChanged(QTreeWidgetItem *,QTreeWidgetItem *)"), self.seleccionado)
        self.connect(self, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem *,int)"), self.owner.aceptar)

        self.dicItemMoves = {}
        self.ponMoves(self.listaMoves)

        self.sortItems(4, QtCore.Qt.AscendingOrder)

    def ponMoves(self, listaMoves):

        liMoves = listaMoves.liMoves
        if liMoves:
            moveOwner = listaMoves.moveOwner
            padre = self if moveOwner is None else moveOwner.item
            for n, mov in enumerate(liMoves):
                item = QtGui.QTreeWidgetItem(padre, [mov.pgn, mov.porcentaje, mov.absoluto,
                                                     "%07d" % int(mov.absoluto.strip(VarGen.XSEP))])
                item.setTextAlignment(1, QtCore.Qt.AlignRight)
                item.setTextAlignment(2, QtCore.Qt.AlignRight)

                mov.item = item
                self.dicItemMoves[str(item)] = mov

            x = 0
            for t in range(3):
                x += self.columnWidth(t)
                self.resizeColumnToContents(t)

            mov = listaMoves.liMoves[0]
            self.setCurrentItem(mov.item)

            nv = 0
            for t in range(3):
                nv += self.columnWidth(t)

            dif = nv - x
            if dif > 0:
                sz = self.owner.splitter.sizes()
                sz[1] += dif
                self.owner.resize(self.owner.width() + dif, self.owner.height())
                self.owner.splitter.setSizes(sz)

    def menuContexto(self, position):
        self.owner.wmoves.menuContexto()

    def goto(self, mov):
        mov = mov.listaMovesPadre.buscaMovVisibleDesde(mov)
        self.setCurrentItem(mov.item)
        self.owner.muestra(mov)
        self.setFocus()

    def seleccionado(self, item, itemA):
        if item:
            self.owner.muestra(self.dicItemMoves[str(item)])
            self.setFocus()

    def keyPressEvent(self, event):
        resp = QtGui.QTreeWidget.keyPressEvent(self, event)
        k = event.key()
        if k == 43:
            self.mas()

        return resp

    def mas(self, mov=None):
        if mov is None:
            item = self.currentItem()
            mov = self.dicItemMoves[str(item)]
        else:
            item = mov.item
        if mov.listaMovesHijos is None:
            item.setText(0, mov.pgn)
            listaMovesHijos = mov.creaHijos()
            self.ponMoves(listaMovesHijos)

    def currentMov(self):
        item = self.currentItem()
        if item:
            mov = self.dicItemMoves[str(item)]
        else:
            mov = None
        return mov


class WMoves(QtGui.QWidget):
    def __init__(self, owner, siEnviar):
        QtGui.QWidget.__init__(self)

        self.owner = owner

        # Tree
        self.tree = TreeMoves(owner)

        # ToolBar
        tb = Controles.TBrutina(self, siTexto=False, tamIcon=16)
        if siEnviar:
            tb.new(_("Accept"), Iconos.Aceptar(), self.owner.aceptar)
            tb.new(_("Cancel"), Iconos.Cancelar(), self.owner.cancelar)
        else:
            tb.new(_("Close"), Iconos.MainMenu(), self.owner.cancelar)
        tb.new(_("Open new branch"), Iconos.Mas(), self.rama)
        tb.new(_("Books"), Iconos.Libros(), self.owner.menuLibros)

        layout = Colocacion.V().control(tb).control(self.tree).margen(1)

        self.setLayout(layout)

    def rama(self):
        if self.tree.currentMov():
            self.tree.mas()


class InfoMove(QtGui.QWidget):
    def __init__(self, fenActivo):
        QtGui.QWidget.__init__(self)

        confTablero = VarGen.configuracion.confTablero("INFOMOVE", 32)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(" w " in fenActivo)

        self.cpDefecto = ControlPosicion.ControlPosicion()
        self.cpDefecto.leeFen(fenActivo)
        self.porDefecto()

        btInicio = Controles.PB(self, "", self.inicio).ponIcono(Iconos.MoverInicio())
        btAtras = Controles.PB(self, "", self.atras).ponIcono(Iconos.MoverAtras())
        btAdelante = Controles.PB(self, "", self.adelante).ponIcono(Iconos.MoverAdelante())
        btFinal = Controles.PB(self, "", self.final).ponIcono(Iconos.MoverFinal())

        self.lbTituloLibro = Controles.LB(self, "")

        lybt = Colocacion.H().relleno()
        for x in (btInicio, btAtras, btAdelante, btFinal):
            lybt.control(x)
        lybt.relleno()

        lyt = Colocacion.H().relleno().control(self.tablero).relleno()

        lya = Colocacion.H().relleno().control(self.lbTituloLibro).relleno()

        layout = Colocacion.V()
        layout.otro(lyt)
        layout.otro(lybt)
        layout.otro(lya)
        layout.relleno()
        self.setLayout(layout)

        self.movActual = None

    def porDefecto(self):
        self.tablero.ponPosicion(self.cpDefecto)

    def ponValores(self):
        posicion, desde, hasta = self.movActual.damePosicion()
        self.tablero.ponPosicion(posicion)

        if desde:
            self.tablero.ponFlechaSC(desde, hasta)

    def ponTituloLibro(self, titulo):
        self.lbTituloLibro.ponTexto("<h2>" + titulo + "</h2>")

    def inicio(self):
        if self.movActual:
            self.movActual.inicio()
        self.ponValores()

    def atras(self):
        if self.movActual:
            self.movActual.atras()
        self.ponValores()

    def adelante(self):
        if self.movActual:
            self.movActual.adelante()
        self.ponValores()

    def final(self):
        if self.movActual:
            self.movActual.final()
        self.ponValores()

    def muestra(self, mov):
        self.movActual = mov
        self.ponValores()


class PantallaArbolBook(QTVarios.WDialogo):
    def __init__(self, gestor, siEnVivo):

        titulo = _("Consult a book")
        icono = Iconos.Libros()
        extparam = "treebook"
        QTVarios.WDialogo.__init__(self, gestor.pantalla, titulo, icono, extparam)

        # Se lee la lista de libros1
        self.listaLibros = Books.ListaLibros()
        self.fvar = gestor.configuracion.ficheroBooks
        self.listaLibros.recuperaVar(self.fvar)

        # Comprobamos que todos esten accesibles
        self.listaLibros.comprueba()
        self.book = self.listaLibros.porDefecto()

        # fens
        fenActivo = gestor.fenActivoConInicio()  # Posicion en el tablero
        fenUltimo = gestor.fenUltimo()
        self.siEnviar = siEnVivo and (fenActivo == fenUltimo)

        self.listaMoves = ListaMoves(None, self.book, fenActivo)

        self.infoMove = InfoMove(fenActivo)

        self.wmoves = WMoves(self, self.siEnviar)

        self.splitter = splitter = QtGui.QSplitter(self)
        splitter.addWidget(self.infoMove)
        splitter.addWidget(self.wmoves)

        ly = Colocacion.H().control(splitter).margen(0)

        self.setLayout(ly)

        self.wmoves.tree.setFocus()

        anchoTablero = self.infoMove.tablero.width()

        self.resize(600 - 278 + anchoTablero, anchoTablero + 30)
        self.splitter.setSizes([296 - 278 + anchoTablero, 290])
        for col, ancho in enumerate((100, 59, 87, 0, 38)):
            self.wmoves.tree.setColumnWidth(col, ancho)

        self.ponTitulo(self.book)

    def muestra(self, mov):
        self.infoMove.muestra(mov)

    def aceptar(self):
        if self.siEnviar:
            mov = self.wmoves.tree.currentMov()
            li = []
            while True:
                nv = mov.listaMovesPadre.nivel
                li.append((mov.desde, mov.hasta, mov.coronacion))
                if nv == 0:
                    break
                mov = mov.listaMovesPadre.moveOwner
            self.resultado = li
            self.accept()
        else:
            self.reject()
        self.guardarVideo()

    def keyPressEvent(self, event):
        if event.key() == 16777266:  # F3
            self.buscaSiguiente()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def closeEvent(self, event):
        self.guardarVideo()

    def cambiaLibro(self, book):
        self.listaMoves.cambiaLibro(book)
        self.wmoves.tree.clear()
        self.wmoves.tree.ponMoves(self.listaMoves)
        self.listaLibros.porDefecto(book)
        self.listaLibros.guardaVar(self.fvar)
        self.ponTitulo(book)
        self.book = book

    def ponTitulo(self, book):
        titulo = book.nombre
        self.infoMove.ponTituloLibro(titulo)
        self.setWindowTitle(_("Consult a book") + " [%s]" % titulo)

    def compruebaApertura(self):
        pass

    def menuLibros(self):
        menu = QTVarios.LCMenu(self)
        nBooks = len(self.listaLibros.lista)

        for book in self.listaLibros.lista:
            ico = Iconos.PuntoVerde() if book == self.book else Iconos.PuntoNaranja()
            menu.opcion(("x", book), book.nombre, ico)

        menu.separador()
        menu.opcion(("n", None), _("Install new book"), Iconos.Nuevo())
        if nBooks > 1:
            menu.separador()
            menub = menu.submenu(_("Remove a book from the list"), Iconos.Delete())
            for book in self.listaLibros.lista:
                if not book.pordefecto:
                    menub.opcion(("b", book), book.nombre, Iconos.Delete())
            menu.separador()
            menu.opcion(("1", None), _("Find Next") + " <F3>", Iconos.Buscar())

        resp = menu.lanza()
        if resp:
            orden, book = resp
            if orden == "x":
                self.cambiaLibro(book)
            elif orden == "n":
                fbin = QTUtil2.leeFichero(self, self.listaLibros.path, "bin", titulo=_("Polyglot book"))
                if fbin:
                    self.listaLibros.path = os.path.dirname(fbin)
                    nombre = os.path.basename(fbin)[:-4]
                    book = Books.Libro("P", nombre, fbin, True)
                    self.listaLibros.nuevo(book)
                    self.cambiaLibro(book)
            elif orden == "b":
                self.listaLibros.borra(book)
                self.listaLibros.guardaVar(self.fvar)
            elif orden == "1":
                self.buscaSiguiente()

    def buscaSiguiente(self):
        # del siguiente al final
        si = False
        for book in self.listaLibros.lista:
            if si:
                if self.listaMoves.siEstaEnLibro(book):
                    self.cambiaLibro(book)
                    return
            if book == self.book:
                si = True
        # del principio al actual
        for book in self.listaLibros.lista:
            if self.listaMoves.siEstaEnLibro(book):
                self.cambiaLibro(book)
                return
            if book == self.book:
                return
