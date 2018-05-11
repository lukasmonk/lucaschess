from PyQt4 import QtGui

from Code import OpeningGuide
from Code import DBgames
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import WBG_Games
from Code.QT import WBG_Players
from Code.QT import WBG_InfoMove
from Code.QT import WBG_Summary


class WBDatabase(QTVarios.WDialogo):
    def __init__(self, wParent, procesador):

        icono = Iconos.DatabaseC()
        extparam = "database"
        titulo = _("Database of complete games")
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.dbGames = DBgames.DBgames(self.configuracion.ficheroDBgames)

        dicVideo = self.recuperarDicVideo()

        self.bookGuide = OpeningGuide.OpeningGuide(self)
        self.wsummary = WBG_Summary.WSummary(procesador, self, self.dbGames, siMoves=False)

        self.wgames = WBG_Games.WGames(procesador, self, self.dbGames, self.wsummary, siMoves=False)

        self.wplayer = WBG_Players.WPlayer(procesador, self, self.dbGames)

        self.registrarGrid(self.wsummary.grid)
        self.registrarGrid(self.wgames.grid)

        self.ultFocus = None

        self.tab = Controles.Tab()
        self.tab.nuevaTab(self.wgames, _("Games"))
        self.tab.nuevaTab(self.wsummary, _("Summary"))
        self.tab.nuevaTab(self.wplayer, _("Player"))
        self.tab.dispatchChange(self.tabChanged)
        self.tab.ponTipoLetra(puntos=procesador.configuracion.puntosTB)

        self.infoMove = WBG_InfoMove.WInfomove(self, siMoves=False)

        self.splitter = splitter = QtGui.QSplitter()
        splitter.addWidget(self.tab)
        splitter.addWidget(self.infoMove)

        layout = Colocacion.H().control(splitter).margen(5)

        self.setLayout(layout)

        self.recuperarVideo(anchoDefecto=1200, altoDefecto=600)
        if not dicVideo:
            dicVideo = {
                'SPLITTER': [800, 380],
                'TREE_1': 25,
                'TREE_2': 25,
                'TREE_3': 50,
                'TREE_4': 661,
            }
        sz = dicVideo.get("SPLITTER", None)
        if sz:
            self.splitter.setSizes(sz)

        self.inicializa()

    def cambiaDBgames(self, fich):
        self.dbGames.close()
        self.dbGames = DBgames.DBgames(self.configuracion.ficheroDBgames)
        self.setdbGames()
        self.wsummary.actualizaPV("")

    def setdbGames(self):
        self.tab.ponValor(0, self.dbGames.rotulo())
        self.wsummary.setdbGames(self.dbGames)
        self.wgames.setdbGames(self.dbGames)
        self.wplayer.setdbGames(self.dbGames)

    def listaGamesSelected(self, no1=False):
        return self.wgames.listaSelected(no1)

    def tabChanged(self, ntab):
        QtGui.QApplication.processEvents()
        tablero = self.infoMove.tablero
        tablero.desactivaTodas()
        if ntab == 0:
            self.wgames.actualiza()
        else:
            self.wsummary.gridActualiza()

    def inicializa(self):
        self.wsummary.setInfoMove(self.infoMove)
        self.wgames.setInfoMove(self.infoMove)
        self.wplayer.setInfoMove(self.infoMove)
        self.setdbGames()
        self.wsummary.actualizaPV("")
        self.wgames.actualiza(True)

    def terminar(self):
        self.salvar()
        self.accept()

    def salvar(self):
        dicExten = {
            "SPLITTER": self.splitter.sizes(),
        }

        self.guardarVideo(dicExten)

        self.dbGames.close()

    def closeEvent(self, event):
        self.salvar()
