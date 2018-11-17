import codecs
import copy
import os

from PyQt4 import QtCore, QtGui

from Code import TabVisual
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import Util
from Code import VarGen


class WTV_Marker(QtGui.QDialog):
    def __init__(self, owner, regMarker, xml=None, nombre=None):

        QtGui.QDialog.__init__(self, owner)

        self.setWindowTitle(_("Marker"))
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.configuracion = VarGen.configuracion

        if not regMarker:
            regMarker = TabVisual.PMarker()
            regMarker.xml = xml
            if nombre:
                regMarker.nombre = nombre

        liAcciones = [(_("Save"), Iconos.Aceptar(), self.grabar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.reject), None,
                      ]
        tb = Controles.TBrutina(self, liAcciones)

        # Tablero
        confTablero = owner.tablero.confTablero
        self.tablero = Tablero.Tablero(self, confTablero, siDirector=False)
        self.tablero.crea()
        self.tablero.copiaPosicionDe(owner.tablero)

        # Datos generales
        liGen = []

        # nombre del svg que se usara en los menus del tutorial
        config = FormLayout.Editbox(_("Name"), ancho=120)
        liGen.append((config, regMarker.nombre))

        # ( "opacidad", "n", 1.0 ),
        config = FormLayout.Dial(_("Degree of transparency"), 0, 99)
        liGen.append((config, 100 - int(regMarker.opacidad * 100)))

        # ( "psize", "n", 100 ),
        config = FormLayout.Spinbox(_("Size") + " %", 1, 1600, 50)
        liGen.append((config, regMarker.psize))

        # ( "poscelda", "n", 1 ),
        li = (
            ("%s-%s" % (_("Top"), _("Left")), 0),
            ("%s-%s" % (_("Top"), _("Right")), 1),
            ("%s-%s" % (_("Bottom"), _("Left")), 2),
            ("%s-%s" % (_("Bottom"), _("Right")), 3),
        )
        config = FormLayout.Combobox(_("Position in the square"), li)
        liGen.append((config, regMarker.poscelda))

        # orden
        config = FormLayout.Combobox(_("Order concerning other items"), QTUtil2.listaOrdenes())
        liGen.append((config, regMarker.posicion.orden))

        self.form = FormLayout.FormWidget(liGen, dispatch=self.cambios)

        # Layout
        layout = Colocacion.H().control(self.form).relleno().control(self.tablero)
        layout1 = Colocacion.V().control(tb).otro(layout)
        self.setLayout(layout1)

        # Ejemplos
        liMovs = ["b4c4", "e2e2", "e4g7"]
        self.liEjemplos = []
        for a1h8 in liMovs:
            regMarker.a1h8 = a1h8
            regMarker.siMovible = True
            marker = self.tablero.creaMarker(regMarker, siEditando=True)
            self.liEjemplos.append(marker)

    def cambios(self):
        if hasattr(self, "form"):
            li = self.form.get()
            for n, marker in enumerate(self.liEjemplos):
                regMarker = marker.bloqueDatos
                regMarker.nombre = li[0]
                regMarker.opacidad = (100.0 - float(li[1])) / 100.0
                regMarker.psize = li[2]
                regMarker.poscelda = li[3]
                regMarker.posicion.orden = li[4]
                marker.setOpacity(regMarker.opacidad)
                marker.setZValue(regMarker.posicion.orden)
                marker.update()
            self.tablero.escena.update()
            QTUtil.refreshGUI()

    def grabar(self):
        regMarker = self.liEjemplos[0].bloqueDatos
        nombre = regMarker.nombre.strip()
        if nombre == "":
            QTUtil2.mensError(self, _("Name missing"))
            return

        pm = self.liEjemplos[0].pixmapX()
        bf = QtCore.QBuffer()
        pm.save(bf, "PNG")
        regMarker.png = str(bf.buffer())

        self.regMarker = regMarker
        self.accept()


class WTV_Markers(QTVarios.WDialogo):
    def __init__(self, owner, listaMarkers, dbMarkers):

        titulo = _("Markers")
        icono = Iconos.Markers()
        extparam = "markers"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.owner = owner

        flb = Controles.TipoLetra(puntos=8)

        self.configuracion = VarGen.configuracion
        self.liPMarkers = listaMarkers
        self.dbMarkers = dbMarkers

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 60, siCentrado=True)
        oColumnas.nueva("NOMBRE", _("Name"), 256)

        self.grid = Grid.Grid(self, oColumnas, xid="M", siSelecFilas=True)

        liAcciones = [
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("New"), Iconos.Nuevo(), self.mas), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Modify"), Iconos.Modificar(), self.modificar), None,
            (_("Copy"), Iconos.Copiar(), self.copiar), None,
            (_("Up"), Iconos.Arriba(), self.arriba), None,
            (_("Down"), Iconos.Abajo(), self.abajo), None,
        ]
        tb = Controles.TBrutina(self, liAcciones)
        tb.setFont(flb)

        ly = Colocacion.V().control(tb).control(self.grid)

        # Tablero
        confTablero = owner.tablero.confTablero
        self.tablero = Tablero.Tablero(self, confTablero, siDirector=False)
        self.tablero.crea()
        self.tablero.copiaPosicionDe(owner.tablero)

        # Layout
        layout = Colocacion.H().otro(ly).control(self.tablero)
        self.setLayout(layout)

        self.grid.gotop()
        self.grid.setFocus()

    def closeEvent(self, event):
        self.guardarVideo()

    def terminar(self):
        self.guardarVideo()
        self.close()

    def gridNumDatos(self, grid):
        return len(self.liPMarkers)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        if clave == "NUMERO":
            return str(fila + 1)
        elif clave == "NOMBRE":
            return self.liPMarkers[fila].nombre

    def gridDobleClick(self, grid, fila, oColumna):
        self.modificar()

    def gridCambiadoRegistro(self, grid, fila, oColumna):
        if fila >= 0:
            regMarker = self.liPMarkers[fila]
            self.tablero.borraMovibles()
            # Ejemplos
            liMovs = ["g4h3", "e2e4", "d6f4"]
            for a1h8 in liMovs:
                regMarker.a1h8 = a1h8
                regMarker.siMovible = True
                self.tablero.creaMarker(regMarker, siEditando=True)
            self.tablero.escena.update()

    def mas(self):

        menu = QTVarios.LCMenu(self)

        def miraDir(submenu, base, dr):
            if base:
                pathCarpeta = base + dr + "/"
                smenu = submenu.submenu(dr, Iconos.Carpeta())
            else:
                pathCarpeta = dr + "/"
                smenu = submenu
            li = []
            for fich in os.listdir(pathCarpeta):
                pathFich = pathCarpeta + fich
                if os.path.isdir(pathFich):
                    miraDir(smenu, pathCarpeta, fich)
                elif pathFich.lower().endswith(".svg"):
                    li.append((pathFich, fich))

            for pathFich, fich in li:
                ico = QTVarios.fsvg2ico(pathFich, 32)
                if ico:
                    smenu.opcion(pathFich, fich[:-4], ico)

        miraDir(menu, "", "imgs")

        menu.separador()

        menu.opcion("@", _X(_("To seek %1 file"), "Marker"), Iconos.Fichero())

        resp = menu.lanza()

        if resp is None:
            return

        if resp == "@":
            fichero = QTUtil2.leeFichero(self, "imgs", "svg", titulo=_("Image"))
            if not fichero:
                return
        else:
            fichero = resp
        f = codecs.open(fichero, "r", 'utf-8', 'ignore')
        contenido = f.read()
        f.close()
        nombre = os.path.basename(fichero)[:-4]
        w = WTV_Marker(self, None, xml=contenido, nombre=nombre)
        if w.exec_():
            regMarker = w.regMarker
            regMarker.id = Util.nuevoID()
            regMarker.ordenVista = (self.liPMarkers[-1].ordenVista + 1) if self.liPMarkers else 1
            self.dbMarkers[regMarker.id] = regMarker
            self.liPMarkers.append(regMarker)
            self.grid.refresh()
            self.grid.gobottom()
            self.grid.setFocus()

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), self.liPMarkers[fila].nombre)):
                regMarker = self.liPMarkers[fila]
                nid = regMarker.id
                del self.liPMarkers[fila]
                del self.dbMarkers[nid]
                self.grid.refresh()
                self.grid.setFocus()

    def modificar(self):
        fila = self.grid.recno()
        if fila >= 0:
            w = WTV_Marker(self, self.liPMarkers[fila])
            if w.exec_():
                regMarker = w.regMarker
                xid = regMarker.id
                self.liPMarkers[fila] = regMarker
                self.dbMarkers[xid] = regMarker
                self.grid.refresh()
                self.grid.setFocus()
                self.gridCambiadoRegistro(self.grid, fila, None)

    def copiar(self):
        fila = self.grid.recno()
        if fila >= 0:
            regMarker = copy.deepcopy(self.liPMarkers[fila])
            n = 1

            def siEstaNombre(nombre):
                for rf in self.liPMarkers:
                    if rf.nombre == nombre:
                        return True
                return False

            nombre = "%s-%d" % (regMarker.nombre, n)
            while siEstaNombre(nombre):
                n += 1
                nombre = "%s-%d" % (regMarker.nombre, n)
            regMarker.nombre = nombre
            regMarker.id = Util.nuevoID()
            regMarker.ordenVista = self.liPMarkers[-1].ordenVista + 1
            self.dbMarkers[regMarker.id] = regMarker
            self.liPMarkers.append(regMarker)
            self.grid.refresh()
            self.grid.setFocus()

    def intercambia(self, fila1, fila2):
        regMarker1, regMarker2 = self.liPMarkers[fila1], self.liPMarkers[fila2]
        regMarker1.ordenVista, regMarker2.ordenVista = regMarker2.ordenVista, regMarker1.ordenVista
        self.dbMarkers[regMarker1.id] = regMarker1
        self.dbMarkers[regMarker2.id] = regMarker2
        self.liPMarkers[fila1], self.liPMarkers[fila2] = self.liPMarkers[fila1], self.liPMarkers[fila2]
        self.grid.goto(fila2, 0)
        self.grid.refresh()
        self.grid.setFocus()

    def arriba(self):
        fila = self.grid.recno()
        if fila > 0:
            self.intercambia(fila, fila - 1)

    def abajo(self):
        fila = self.grid.recno()
        if 0 <= fila < (len(self.liPMarkers) - 1):
            self.intercambia(fila, fila + 1)
