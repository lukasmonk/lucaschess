import collections

from PyQt4 import QtGui, QtCore

from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import PantallaColores
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import WBG_Comun
from Code import TrListas
from Code.Constantes import *

SIN_VALORACION, MUY_MALO, MALO, BUENO, MUY_BUENO, INTERESANTE, DUDOSA = (0, 4, 2, 1, 3, 5, 6)
V_SIN, V_IGUAL, V_BLANCAS, V_NEGRAS, V_BLANCAS_MAS, V_NEGRAS_MAS, V_BLANCAS_MAS_MAS, V_NEGRAS_MAS_MAS = (
    0, 11, 14, 15, 16, 17, 18, 19)


class TreeMoves(QtGui.QTreeWidget):
    def __init__(self, wmoves):
        QtGui.QTreeWidget.__init__(self)
        self.wmoves = wmoves
        self.itemActivo = None
        self.setAlternatingRowColors(True)

        self.dicItems = {}

        self.posMoves = 0
        self.posTransposition = 1
        self.posBookmark = 2
        self.posAnalisis = 3
        self.posComment = 4
        self.setHeaderLabels((_("Moves"), "", "", _("Analysis"), _("Comments"), ""))
        self.setColumnHidden(5, True)
        self.setIndentation(14)
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.menuContexto)

        self.setStyleSheet("selection-background-color: #F1D369; selection-color: #000000;")

        ftxt = Controles.TipoLetra(puntos=9)

        self.setFont(ftxt)

        self.connect(self, QtCore.SIGNAL("itemExpanded(QTreeWidgetItem *)"), self.expandido)
        self.connect(self, QtCore.SIGNAL("itemSelectionChanged()"), self.seleccionadoISC)
        self.connect(self, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem *,int)"), self.dobleClick)

        self.noIcon = QtGui.QIcon()
        self.iconBookmark = Iconos.Favoritos()
        self.iconTransposition = Iconos.Transposition()

        dicNAGs = TrListas.dicNAGs()
        self.dicValoracion = collections.OrderedDict()
        self.dicValoracion[BUENO] = (dicNAGs[1], PantallaColores.nag2ico(1, 16))
        self.dicValoracion[MALO] = (dicNAGs[2], PantallaColores.nag2ico(2, 16))
        self.dicValoracion[MUY_BUENO] = (dicNAGs[3], PantallaColores.nag2ico(3, 16))
        self.dicValoracion[MUY_MALO] = (dicNAGs[4], PantallaColores.nag2ico(4, 16))
        self.dicValoracion[INTERESANTE] = (dicNAGs[5], PantallaColores.nag2ico(5, 16))
        self.dicValoracion[DUDOSA] = (dicNAGs[6], PantallaColores.nag2ico(6, 16))
        self.dicValoracion[SIN_VALORACION] = (_("No rating"), self.noIcon)

        self.dicVentaja = collections.OrderedDict()
        self.dicVentaja[V_SIN] = (_("Undefined"), self.noIcon)
        self.dicVentaja[V_IGUAL] = (dicNAGs[11], Iconos.V_Blancas_Igual_Negras())
        self.dicVentaja[V_BLANCAS] = (dicNAGs[14], Iconos.V_Blancas())
        self.dicVentaja[V_BLANCAS_MAS] = (dicNAGs[16], Iconos.V_Blancas_Mas())
        self.dicVentaja[V_BLANCAS_MAS_MAS] = (dicNAGs[18], Iconos.V_Blancas_Mas_Mas())
        self.dicVentaja[V_NEGRAS] = (dicNAGs[15], Iconos.V_Negras())
        self.dicVentaja[V_NEGRAS_MAS] = (dicNAGs[17], Iconos.V_Negras_Mas())
        self.dicVentaja[V_NEGRAS_MAS_MAS] = (dicNAGs[19], Iconos.V_Negras_Mas_Mas())

    def dobleClick(self, item, col):
        move = self.dicItems.get(str(item), None)
        if move is None:
            return

        elif col == self.posTransposition:
            tr = move.transpositions()
            ntr = len(tr)
            if ntr == 0:
                return
            menutr = QTVarios.LCMenu(self)
            menutr.opcion(None, move.allPGN(), Iconos.Transposition(), siDeshabilitado=True)
            menutr.separador()
            for n, mv in enumerate(tr):
                menutr.opcion("tr_%d" % n, mv.allPGN(), Iconos.PuntoVerde())
            resp = menutr.lanza()
            if resp:
                move = tr[int(resp[3:])]

                self.wmoves.seleccionaMove(move)

        elif col == self.posAnalisis:
            rm = move.analisis()
            if rm:
                fen = move.father().fen()
                pv = move.pv() + " " + rm.pv
                self.analisis.showAnalisis(fen, pv, rm)

    def setBookGuide(self, bookGuide, procesador):
        self.bookGuide = bookGuide
        self.analisis = WBG_Comun.Analisis(self, bookGuide, self.resetAnalisis, procesador)

    def menuContexto(self, position):
        item, move = self.moveActual()
        if not move:
            return

        menu = QTVarios.LCMenu(self)

        menu1 = menu.submenu(_("Expand"), Iconos.Mas22())
        menu1.opcion("expandall", _("All"), Iconos.PuntoVerde())
        menu1.separador()
        menu1.opcion("expandthis", _("This branch"), Iconos.PuntoAmarillo())
        menu.separador()
        menu1 = menu.submenu(_("Collapse"), Iconos.Menos22())
        menu1.opcion("collapseall", _("All"), Iconos.PuntoVerde())
        menu1.separador()
        menu1.opcion("collapsethis", _("This branch"), Iconos.PuntoAmarillo())
        menu.separador()
        menu.opcion("remove", _("Remove"), Iconos.Borrar())
        menu.separador()
        padre = move.father()
        fenM2 = padre.fenM2()
        rmAnalisis = move.analisis()
        siShowMoves = rmAnalisis is not None
        self.analisis.menuAnalizar(fenM2, menu, siShowMoves)
        menu.separador()
        if move.mark():
            menu.opcion("mark_rem", _("Remove bookmark"), self.iconBookmark)
        else:
            menu.opcion("mark_add", _("Add bookmark"), self.iconBookmark)

        tr = move.transpositions()
        if tr and tr>1:
            menu.separador()
            menutr = menu.submenu(_("Transpositions"), Iconos.Transposition())
            for n, mv in enumerate(tr):
                menutr.opcion("tr_%d" % n, mv.allPGN(), Iconos.PuntoVerde())
                menutr.separador()

        resp = menu.lanza()
        if resp:
            if resp == "expandthis":
                self.showFrom(move, True)
                self.resizeColumnToContents(0)

            elif resp == "expandall":
                self.showFrom(self.bookGuide.root, True, siRoot=True)
                self.resizeColumnToContents(0)

            elif resp == "collapsethis":
                self.showFrom(move, False)

            elif resp == "collapseall":
                for move in self.bookGuide.root.children():
                    self.showFrom(move, False)

            elif resp == "remove":
                self.borrar()

            elif resp.startswith("mark_rem"):
                move.mark("")
                self.ponIconoBookmark(item, move.mark())
                self.wmoves.compruebaBookmarks()

            elif resp.startswith("mark_add"):
                self.newBookmark(move)

            elif resp.startswith("tr_"):
                self.wmoves.seleccionaMove(tr[int(resp[3:])])

            elif resp.startswith("an_"):
                fen = padre.fen()
                pv = move.pv() + " " + rmAnalisis.pv if siShowMoves else None
                self.analisis.exeAnalizar(fenM2, resp, padre, fen, pv, rmAnalisis)

    def showUnMove(self, itemBase, unMove):
        if unMove.item() is None:
            pgn = unMove.pgnNum()
            comentario = unMove.commentLine()
            posicion = str(unMove.pos())
            puntos = unMove.etiPuntos()
            item = QtGui.QTreeWidgetItem(itemBase, [pgn, "", "", puntos, comentario, posicion])
            item.setTextAlignment(self.posAnalisis, QtCore.Qt.AlignRight)
            self.ponIconoValoracion(item, unMove.nag())
            self.ponIconoVentaja(item, unMove.adv())
            self.ponIconoBookmark(item, unMove.mark())
            if unMove.transpositions():
                item.setIcon(self.posTransposition, self.iconTransposition)
            self.dicItems[str(item)] = unMove
            unMove.item(item)

    def showFrom(self, unMove, siExpand, siRoot=False):
        if siExpand:
            mens = QTUtil2.mensEspera
            mens.inicio(self, _("Expanding"), siCancelar=True)

        def work(move):
            if siExpand and mens.cancelado():
                return False
            itemBase = move.item()

            for uno in move.children():
                if uno.item() is None:
                    self.showUnMove(itemBase, uno)
                if siExpand:
                    uno.item().setExpanded(True)
                    work(uno)
            if not siRoot:
                itemBase.setExpanded(siExpand)

        work(unMove)
        if siExpand:
            mens.final()

        return True

    def showChildren(self, unMove, siNietos):
        itemBase = unMove.item()

        for uno in unMove.children():
            if uno.item() is None:
                self.showUnMove(itemBase, uno)
            if siNietos:
                self.showChildren(uno, False)
        self.resizeColumnToContents(0)

    def focusInEvent(self, event):
        self.seleccionado(self.itemActivo)
        self.wmoves.focusInEvent(event)

    def moveActual(self):
        item = self.itemActivo
        if item:
            mov = self.dicItems[str(item)]
        else:
            mov = None
        return item, mov

    def seleccionadoISC(self):
        self.itemActivo = self.currentItem()
        self.seleccionado(self.itemActivo)

    def seleccionado(self, item):
        if item:
            uno = self.dicItems[str(item)]
            self.wmoves.seleccionado(uno)
            self.resizeColumnToContents(self.posMoves)

    def expandido(self, item):
        uno = self.dicItems[str(item)]
        self.showChildren(uno, True)

    def ponIconoVentaja(self, item, ventaja):
        if ventaja not in self.dicVentaja:
            ventaja = 0
        item.setIcon(self.posAnalisis, self.dicVentaja[ventaja][1])

    def ponIconoValoracion(self, item, valoracion):
        item.setIcon(self.posMoves, self.dicValoracion[valoracion][1])

    def ponIconoBookmark(self, item, mark):
        ico = self.iconBookmark if mark else self.noIcon
        item.setIcon(self.posBookmark, ico)

    def resetValoracion(self, move):
        self.ponIconoValoracion(move.item(), move.nag())

    def resetVentaja(self, move):
        self.ponIconoVentaja(move.item(), move.adv())

    def resetComentario(self, move):
        if move.pv():
            move.item().setText(self.posComment, move.commentLine())

    def resetAnalisis(self, padre):
        for uno in padre.children():
            item = uno.item()
            item.setText(self.posAnalisis, uno.etiPuntos())

    def borrar(self):
        item, mov = self.moveActual()
        if item:
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), mov.pgn())):
                um = QTUtil2.unMomento(self)
                self.removeRow()
                liBorrados, liQuitarTransposition = self.bookGuide.borrar(mov)
                for mv in liBorrados:
                    item = mv.item()
                    if item:
                        del self.dicItems[str(item)]
                for mv in liQuitarTransposition:
                    item = mv.item()
                    if item:
                        item.setIcon(self.posTransposition, QtGui.QIcon())

                mov.father().delChildren(mov)

                um.final()
                self.wmoves.compruebaBookmarks()

    def removeRow(self):
        index = self.currentIndex()
        model = self.model()
        model.removeRow(index.row(), index.parent())

    def newBookmark(self, move):
        comment = move.comment()
        allpgn = move.allPGN()
        siComment = len(comment) > 0

        txt = comment if siComment else allpgn

        liGen = [(None, None)]
        liGen.append((_("Name") + ":", txt))

        liGen.append((_("Copy PGN") + ":", False))
        if siComment:
            liGen.append((_("Copy comment") + ":", False))

        reg = KRegistro()
        reg.allpgn = allpgn
        reg.comment = comment.split("\n")[0].strip()
        reg.form = None

        def dispatch(valor):
            if reg.form is None:
                reg.form = valor
                reg.wname = valor.getWidget(0)
                reg.wpgn = valor.getWidget(1)
                reg.wcomment = valor.getWidget(2)
                reg.wpgn.setText(reg.allpgn)
                if reg.wcomment:
                    reg.wcomment.setText(reg.comment)
            else:
                QTUtil.refreshGUI()
                if reg.wpgn.isChecked():
                    reg.wname.setText(reg.allpgn)
                elif reg.wcomment and reg.wcomment.isChecked():
                    reg.wname.setText(reg.comment)
                if reg.wcomment:
                    reg.wcomment.setChecked(False)
                reg.wpgn.setChecked(False)
                QTUtil.refreshGUI()

        resultado = FormLayout.fedit(liGen, title=_("Bookmark"), parent=self.wmoves, anchoMinimo=460,
                                     icon=Iconos.Favoritos(), dispatch=dispatch)
        if resultado is None:
            return None

        accion, liResp = resultado
        txt = liResp[0].strip()
        if txt:
            move.mark(txt)
            self.ponIconoBookmark(move.item(), move.mark())
            self.wmoves.compruebaBookmarks()
