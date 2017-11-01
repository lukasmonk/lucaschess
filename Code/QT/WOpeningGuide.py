from PyQt4 import QtGui, QtCore

from Code import OpeningGuide
from Code import DBgames
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import WBG_Games
from Code.QT import WBG_InfoMove
from Code.QT import WBG_Moves
from Code.QT import WBG_Summary


class WOpeningGuide(QTVarios.WDialogo):
    def __init__(self, wParent, procesador, fenM2inicial=None, pvInicial=None):

        icono = Iconos.BookGuide()
        extparam = "edicionMyOwnBook"
        titulo = _("Personal Opening Guide")
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.fenM2inicial = fenM2inicial
        self.pvInicial = pvInicial
        self.bookGuide = OpeningGuide.OpeningGuide(self)
        self.dbGames = DBgames.DBgames(self.configuracion.ficheroDBgames)

        dicVideo = self.recuperarDicVideo()

        self.wmoves = WBG_Moves.WMoves(procesador, self)

        self.wsummary = WBG_Summary.WSummary(procesador, self, self.dbGames)

        self.wgames = WBG_Games.WGames(procesador, self, self.dbGames, self.wsummary)

        self.registrarGrid(self.wsummary.grid)
        self.registrarGrid(self.wgames.grid)

        self.ultFocus = None

        self.splitterMoves = QtGui.QSplitter(self)
        self.splitterMoves.setOrientation(QtCore.Qt.Vertical)
        self.splitterMoves.addWidget(self.wmoves)
        self.splitterMoves.addWidget(self.wsummary)

        self.tab = Controles.Tab()
        self.tab.nuevaTab(self.splitterMoves, _("Moves"))
        self.tab.nuevaTab(self.wgames, _("Games"))
        self.tab.dispatchChange(self.tabChanged)

        self.infoMove = WBG_InfoMove.WInfomove(self)

        self.splitter = splitter = QtGui.QSplitter(self)
        splitter.addWidget(self.infoMove)
        splitter.addWidget(self.tab)

        layout = Colocacion.H().control(splitter).margen(5)

        self.setLayout(layout)

        self.wmoves.tree.setFocus()

        self.recuperarVideo(anchoDefecto=1175)
        if not dicVideo:
            dicVideo = {'SPLITTER': [380, 816],
                        'TREE_1': 25,
                        'TREE_2': 25,
                        'TREE_3': 50,
                        'TREE_4': 661,
                        'SPLITTERMOVES': [344, 244]}
        sz = dicVideo.get("SPLITTER", None)
        if sz:
            self.splitter.setSizes(sz)
        for x in range(1, 6):
            w = dicVideo.get("TREE_%d" % x, None)
            if w:
                self.wmoves.tree.setColumnWidth(x, w)

        self.inicializa()

    def cambiaDBgames(self, fich):
        self.dbGames.close()
        self.dbGames = DBgames.DBgames(self.configuracion.ficheroDBgames)
        self.setdbGames()

    def setdbGames(self):
        self.wgames.setdbGames(self.dbGames)
        self.wsummary.setdbGames(self.dbGames)

    def tabChanged(self, ntab):
        QtGui.QApplication.processEvents()
        tablero = self.infoMove.tablero
        tablero.desactivaTodas()
        if ntab == 1:
            self.wgames.actualiza()
        elif ntab == 0:
            self.wmoves.actualiza()

    def inicializa(self):
        self.wsummary.setInfoMove(self.infoMove)
        # self.wsummary.actualiza( )
        self.wsummary.setwmoves(self.wmoves)

        self.wgames.setInfoMove(self.infoMove)
        self.wmoves.setSummary(self.wsummary)
        self.wmoves.setInfoMove(self.infoMove)

        self.bookGuide.reset()
        self.wmoves.setBookGuide(self.bookGuide)

        self.infoMove.setBookGuide(self.bookGuide)

        self.wmoves.ponFenM2inicial(self.fenM2inicial, self.pvInicial)

    def terminar(self):
        self.salvar()
        self.accept()

    def salvar(self):
        dicExten = {
            "SPLITTER": self.splitter.sizes(),
            "SPLITTERMOVES": self.splitterMoves.sizes(),
        }
        for x in range(1, 6):
            dicExten["TREE_%d" % x] = self.wmoves.tree.columnWidth(x)

        self.guardarVideo(dicExten)

        self.bookGuide.grabar()
        self.bookGuide.cerrar()

        self.dbGames.close()

    def closeEvent(self, event):
        self.salvar()

    def seleccionaPV(self, pv):
        ma = self.infoMove.movActual
        mSel = None
        for uno in ma.children():
            if uno.pv() == pv:
                mSel = uno
                break
        if mSel is None:
            mSel = self.bookGuide.dameMovimiento(ma, pv)
            siNuevo = True
        else:
            siNuevo = False

        self.wmoves.showChildren(ma)
        self.wmoves.seleccionado(mSel, True)
        if siNuevo:
            for mv in mSel.transpositions():
                if mv.item():
                    mv.item().setIcon(1, Iconos.Transposition())
