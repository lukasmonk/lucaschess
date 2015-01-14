# -*- coding: latin-1 -*-

from PyQt4 import QtCore, QtGui

import Code.Util as Util
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.QTVarios as QTVarios
import Code.QT.Columnas as Columnas
import Code.QT.Grid as Grid
import Code.QT.Delegados as Delegados
import Code.QT.FormLayout as FormLayout

def newServerUser(pantalla, dicServers):
    # Datos
    liGen = [(None, None)]

    # # Servidores
    li = dicServers.keys()
    liCombo = [li[0]]
    for x in li:
        liCombo.append(( x, x ))
    liGen.append(( _("Server") + ":", liCombo ))

    # # Usuario
    liGen.append(( _("User") + ":", "" ))

    ## Password
    config = FormLayout.Editbox(_("Password"), siPassword=True)
    liGen.append(( config, "" ))

    # Editamos
    resultado = FormLayout.fedit(liGen, title=_("New link"), parent=pantalla, icon=Iconos.XFCC())
    if resultado:
        accion, liResp = resultado
        server, user, password = liResp
        password = password.strip()
        user = user.strip()
        if user and password:
            return server, user, password
    return None

class WEdicionXFCC(QTVarios.WDialogo):
    def __init__(self, procesador, db):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, "%s: %s" % (db.server, db.user), Iconos.XFCC(), "xfcc")

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.db = db

        dicIconosTurn = {"0": Iconos.pmVerde(), "1": Iconos.pmNaranja(), "2": Iconos.pmGris()}

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("TURN", "", 26, edicion=Delegados.PmIconosBMT(dicIconos=dicIconosTurn), siCentrado=True)
        oColumnas.nueva("ID", "#", 50, siCentrado=True)
        oColumnas.nueva("WHITE", _("White"), 70, siCentrado=True)
        oColumnas.nueva("BLACK", _("Black"), 70, siCentrado=True)
        oColumnas.nueva("TIMEPLAYER", _("Time player"), 100, siCentrado=True)
        oColumnas.nueva("TIMEOPPONENT", _("Time opponent"), 100, siCentrado=True)
        oColumnas.nueva("WHITEELO", _("White elo"), 70, siCentrado=True)
        oColumnas.nueva("BLACKELO", _("Black elo"), 70, siCentrado=True)
        oColumnas.nueva("TIMECONTROL", _("Time control"), 80, siCentrado=True)
        oColumnas.nueva("EVENT", _("Event"), 120, siCentrado=True)
        oColumnas.nueva("EVENTDATE", _("Date"), 70, siCentrado=True)
        oColumnas.nueva("RESULT", _("Result"), 70, siCentrado=True)
        self.ghistorico = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.ghistorico.setMinimumWidth(self.ghistorico.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            ( _("Quit"), Iconos.MainMenu(), "terminar" ), None,
            ( _("Update"), Iconos.Refresh(), "update" ), None,
            ( _("Edit game"), Iconos.Modificar(), "editar" ), None,
        )
        self.tb = Controles.TB(self, liAcciones)

        # Status bar
        self.status = QtGui.QStatusBar(self)

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.ghistorico).control(self.status).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.ghistorico)
        self.recuperarVideo(siTam=False)

        self.siAutoUpdate = self.db.siPendienteTurnoOtro()
        self.autoUpdate()
        self.ghistorico.gotop()

        self.resultado = False

    def testAutoUpdate(self):
        self.siAutoUpdate = self.db.siPendienteTurnoOtro()
        if self.siAutoUpdate:
            QtCore.QTimer.singleShot(60 * 1000, self.autoUpdate)  # Cada 1 minuto

    def autoUpdate(self):
        if self.siAutoUpdate:
            if self.update():
                self.testAutoUpdate()
            else:
                self.siAutoUpdate = False

    def update(self):
        self.status.showMessage(_("Updating..."))
        resp = self.db.getMyGames()
        if resp:
            mens = "%s\n%s" % (_("There was an error while updating data."), resp)
            self.status.showMessage(mens.replace("\n", " "))
        else:
            t = Util.hoy()
            self.status.showMessage("%s %02d:%02d" % ( _("Last update"), t.hour, t.minute ))
            self.ghistorico.refresh()
            QTUtil.refreshGUI()
        return resp == ""

    def gridNumDatos(self, grid):
        return len(self.db)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if self.db.recno() != fila:
            self.db.goto(fila)
        return getattr(self.db.dbf, col)

    def gridDobleClick(self, grid, fila, oColumna):
        self.editar()

    def editar(self):
        fila = self.ghistorico.recno()
        if fila >= 0:
            self.db.goto(fila)
            self.guardarVideo()
            self.resultado = True
            self.accept()

    def closeEvent(self, event):
        self.terminar()

    def terminar(self):
        self.siAutoUpdate = False
        self.guardarVideo()
        self.db.close()

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "terminar":
            self.terminar()
            self.reject()

        elif accion == "update":
            self.update()

        elif accion == "editar":
            self.editar()

    def borrar(self):
        li = self.ghistorico.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.historico.borrarLista(li)
        self.ghistorico.gotop()
        self.ghistorico.refresh()

def pantallaXFCC(procesador, db):
    w = WEdicionXFCC(procesador, db)
    w.exec_()

    return w.resultado

