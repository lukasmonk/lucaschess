# -*- coding: latin-1 -*-

import os

import Code.ControlPosicion as ControlPosicion
import Code.Partida as Partida
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.PantallaPGN as PantallaPGN
import Code.QT.WBG_Games as WBG_Games
import Code.QT.WinPosition as WinPosition

class WGamesFEN(WBG_Games.WGames):
    def __init__(self, procesador, winBookGuide, dbGamesFEN):
        WBG_Games.WGames.__init__(self, procesador, winBookGuide, dbGamesFEN, None, siFEN=True)

    def setNameToolBar(self):
        nomFichero = self.dbGames.rotulo()
        self.lbName.ponTexto(nomFichero)

    def actualiza(self, siObligatorio=False):
        if siObligatorio or self.liFiltro:
            self.siFiltro = False

            self.numJugada = 0
            self.grid.refresh()
            self.grid.gotop()
            self.updateStatus()
        recno = self.grid.recno()
        if recno >= 0:
            self.gridCambiadoRegistro(None, recno, None)
        if self.gridNumDatos(None) == 0:
            self.btLeerMas.setVisible(False)

    def tw_filtrar(self):
        w = PantallaPGN.WFiltrar(self, self.grid.oColumnas, self.liFiltro)
        if w.exec_():
            self.liFiltro = w.liFiltro
            self.siFiltro = True

            where = w.where()
            self.dbGames.leer(where)
            self.grid.refresh()
            self.grid.gotop()
            self.updateStatus()
            self.siFiltro = len(where) > 0

    def tw_borrar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            if not QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                return

            um = QTUtil2.unMomento(self)

            self.dbGames.borrarLista(li)
            self.grid.refresh()
            self.updateStatus()

            um.final()

    def tg_change(self):
        pathFich = QTUtil2.leeCreaFichero(self, os.path.dirname(self.configuracion.ficheroDBgames), "lcf",
                                          _("Positions database"))
        if pathFich:
            if not pathFich.lower().endswith(".lcf"):
                pathFich += ".lcf"
            path = os.path.dirname(pathFich)
            if os.path.isdir(path):
                self.changeDBgames(pathFich)

    def changeDBgames(self, pathFich):
        self.configuracion.ficheroDBgamesFEN = pathFich
        self.configuracion.graba()
        self.winBookGuide.cambiaDBgames(pathFich)
        self.setNameToolBar()
        self.limpiaColumnas()
        self.actualiza(True)

    def gridCambiadoRegistro(self, grid, fila, oCol):
        fen, pv = self.dbGames.dameFEN_PV(fila)
        p = Partida.Partida()
        p.resetFEN(fen)
        p.leerPV(pv)
        p.siTerminada()
        self.infoMove.modoPartida(p, -1)
        self.setFocus()
        self.grid.setFocus()

    def tw_nuevo(self):
        # Se genera un PGN
        resp = WinPosition.editarPosicion(self, self.configuracion, ControlPosicion.FEN_INICIAL)
        if resp:
            pgn = self.dbGames.blankPGN(resp)
            nuevoPGN, pv, dicPGN = self.procesador.gestorUnPGN(self, pgn)
            if not nuevoPGN:
                nuevoPGN = ""
                for k in dicPGN:
                    nuevoPGN += '[%s "%s"]\n' % (k, dicPGN[k])
                nuevoPGN += "\n\n*"
            if not self.dbGames.cambiarUno(None, nuevoPGN, pv, dicPGN):
                QTUtil2.mensError(self, _("This game already exists."))
            else:
                self.actualiza()
                self.grid.gobottom()
