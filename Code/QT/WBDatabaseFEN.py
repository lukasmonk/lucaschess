# -*- coding: latin-1 -*-

from PyQt4 import QtGui

import Code.DBgamesFEN as DBgamesFEN
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Colocacion as Colocacion
import Code.QT.QTVarios as QTVarios
import Code.QT.WBG_GamesFEN as WBG_GamesFEN
import Code.QT.WBG_InfoMove as WBG_InfoMove

class WBDatabaseFEN(QTVarios.WDialogo):
    def __init__(self, wParent, procesador):

        icono = Iconos.DatabaseF()
        extparam = "databasepositions"
        titulo = _("Positions database")
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.dbGamesFEN = DBgamesFEN.DBgamesFEN(self.configuracion.ficheroDBgamesFEN)

        dicVideo = self.recuperarDicVideo()

        self.wgamesFEN = WBG_GamesFEN.WGamesFEN(procesador, self, self.dbGamesFEN)

        self.registrarGrid(self.wgamesFEN.grid)

        self.ultFocus = None

        self.tab = Controles.Tab()
        self.tab.nuevaTab(self.wgamesFEN, _("Games"))
        self.tab.ponTipoLetra(puntos=procesador.configuracion.puntosTB)

        self.infoMove = WBG_InfoMove.WInfomove(self, siMoves=False)

        self.splitter = splitter = QtGui.QSplitter(self)
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
        self.dbGamesFEN.close()
        self.dbGamesFEN = DBgamesFEN.DBgamesFEN(self.configuracion.ficheroDBgamesFEN)
        self.setdbGames()

    def setdbGames(self):
        self.dbGamesFEN.leer()
        self.tab.ponValor(0, "%s: %s" % (_("Database"), self.dbGamesFEN.rotulo() ))
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

