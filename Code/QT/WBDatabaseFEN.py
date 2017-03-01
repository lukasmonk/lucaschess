from PyQt4 import QtGui

from Code import DBgamesFEN
from Code.QT import Colocacion
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import WBG_GamesFEN
from Code.QT import WBG_InfoMove


class WBDatabaseFEN(QTVarios.WDialogo):
    def __init__(self, wParent, procesador):

        icono = Iconos.DatabaseF()
        extparam = "databasepositions"
        titulo = _("Positions Database")
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.dbGamesFEN = DBgamesFEN.DBgamesFEN(self.configuracion.ficheroDBgamesFEN)

        self.wgamesFEN = WBG_GamesFEN.WGamesFEN(procesador, self, self.dbGamesFEN)

        self.registrarGrid(self.wgamesFEN.grid)

        self.ultFocus = None

        self.infoMove = WBG_InfoMove.WInfomove(self, siMoves=False)

        self.splitter = splitter = QtGui.QSplitter(self)
        splitter.addWidget(self.wgamesFEN)
        splitter.addWidget(self.infoMove)
        self.registrarSplitter(self.splitter, "base")

        layout = Colocacion.H().control(splitter).margen(5)

        self.setLayout(layout)

        self.recuperarVideo(anchoDefecto=1000, altoDefecto=600)

        self.inicializa()

    def cambiaDBgames(self, fich):
        self.dbGamesFEN.close()
        self.dbGamesFEN = DBgamesFEN.DBgamesFEN(self.configuracion.ficheroDBgamesFEN)
        self.setdbGames()

    def setdbGames(self):
        self.wgamesFEN.setdbGames(self.dbGamesFEN)

    def inicializa(self):
        self.wgamesFEN.setInfoMove(self.infoMove)
        self.wgamesFEN.actualiza(True)
        self.setdbGames()
        self.wgamesFEN.grid.refresh()
        self.wgamesFEN.grid.gotop()

    def terminar(self):
        self.salvar()
        self.accept()

    def salvar(self):
        dicExten = {
            "SPLITTER": self.splitter.sizes(),
        }

        self.guardarVideo(dicExten)

        self.dbGamesFEN.close()

    def closeEvent(self, event):
        self.salvar()
