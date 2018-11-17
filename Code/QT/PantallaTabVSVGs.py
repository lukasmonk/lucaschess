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
from Code.QT import TabTipos
from Code.QT import Tablero
from Code import Util
from Code import VarGen

estrellaSVG = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- Created with Inkscape (http://www.inkscape.org/) -->

<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   version="1.1"
   width="64"
   height="64"
   id="svg2996">
  <defs
     id="defs2998" />
  <metadata
     id="metadata3001">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title></dc:title>
      </cc:Work>
    </rdf:RDF>
  </metadata>
  <g
     id="layer1">
    <path
       d="M 27.169733,29.728386 C 25.013244,32.2821 14.66025,23.129153 11.578235,24.408078 8.4962206,25.687003 6.6393909,39.1341 3.3071401,38.993129 -0.02511049,38.852157 0.91445325,26.949682 -1.9446847,25.158604 c -2.8591381,-1.791078 -15.0810253,3.34372 -17.0117283,0.569017 -1.930702,-2.774702 8.403637,-10.308248 8.19374,-13.566752 -0.209897,-3.2585049 -10.936253,-10.0036344 -9.712288,-13.06583173 1.223964,-3.06219717 12.0590479,-0.21386597 14.474209,-2.52953107 2.4151612,-2.3156652 0.7741756,-15.3362732 3.9137487,-16.2517622 3.139573,-0.915489 6.1950436,10.0329229 9.4593203,10.280471 3.264277,0.2475482 11.944248,-8.425366 14.803863,-6.774742 2.859615,1.650623 -1.726822,13.0796387 0.03239,15.84516339 1.759209,2.76552481 12.739744,3.15253011 13.354311,6.42891241 0.614567,3.2763822 -10.178057,6.5799722 -11.316244,9.7437452 -1.138187,3.163774 5.079588,11.337377 2.923098,13.891092 z"
       transform="matrix(1.0793664,0,0,1.021226,24.134217,21.975315)"
       id="path3024"
       style="fill:none;stroke:#136ad6;stroke-width:2.29143548;stroke-linecap:butt;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1;stroke-dasharray:none;stroke-dashoffset:0" />
  </g>
</svg>"""


class WTV_SVG(QtGui.QDialog):
    def __init__(self, owner, regSVG, xml=None, nombre=None):

        QtGui.QDialog.__init__(self, owner)

        self.setWindowTitle(_("Image"))
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.configuracion = VarGen.configuracion

        if not regSVG:
            regSVG = TabVisual.PSVG()
            regSVG.xml = xml
            if nombre:
                regSVG.nombre = nombre

        liAcciones = [(_("Save"), Iconos.Aceptar(), "grabar"), None,
                      (_("Cancel"), Iconos.Cancelar(), "reject"), None,
                      ]
        tb = Controles.TB(self, liAcciones)

        # Tablero
        confTablero = owner.tablero.confTablero
        self.tablero = Tablero.Tablero(self, confTablero, siDirector=False)
        self.tablero.crea()
        self.tablero.copiaPosicionDe(owner.tablero)

        # Datos generales
        liGen = []

        # nombre del svg que se usara en los menus del tutorial
        config = FormLayout.Editbox(_("Name"), ancho=120)
        liGen.append((config, regSVG.nombre))

        # ( "opacidad", "n", 1.0 ),
        config = FormLayout.Dial(_("Degree of transparency"), 0, 99)
        liGen.append((config, 100 - int(regSVG.opacidad * 100)))

        # ( "psize", "n", 100 ),
        config = FormLayout.Spinbox(_("Size") + " %", 1, 1600, 50)
        liGen.append((config, regSVG.psize))

        # orden
        config = FormLayout.Combobox(_("Order concerning other items"), QTUtil2.listaOrdenes())
        liGen.append((config, regSVG.posicion.orden))

        self.form = FormLayout.FormWidget(liGen, dispatch=self.cambios)

        # Layout
        layout = Colocacion.H().control(self.form).relleno().control(self.tablero)
        layout1 = Colocacion.V().control(tb).otro(layout)
        self.setLayout(layout1)

        # Ejemplos
        liMovs = ["b4c4", "e2e2", "e4g7"]
        self.liEjemplos = []
        for a1h8 in liMovs:
            regSVG.a1h8 = a1h8
            regSVG.siMovible = True
            svg = self.tablero.creaSVG(regSVG, siEditando=True)
            self.liEjemplos.append(svg)

    def procesarTB(self):
        accion = self.sender().clave
        eval("self.%s()" % accion)

    def cambios(self):
        if hasattr(self, "form"):
            li = self.form.get()
            for n, svg in enumerate(self.liEjemplos):
                regSVG = svg.bloqueDatos
                regSVG.nombre = li[0]
                regSVG.opacidad = (100.0 - float(li[1])) / 100.0
                regSVG.psize = li[2]
                regSVG.posicion.orden = li[3]
                svg.setOpacity(regSVG.opacidad)
                svg.setZValue(regSVG.posicion.orden)
                svg.update()
            self.tablero.escena.update()
            QTUtil.refreshGUI()

    def grabar(self):
        regSVG = self.liEjemplos[0].bloqueDatos
        nombre = regSVG.nombre.strip()
        if nombre == "":
            QTUtil2.mensError(self, _("Name missing"))
            return

        self.regSVG = regSVG

        pm = self.liEjemplos[0].pixmapX()
        bf = QtCore.QBuffer()
        pm.save(bf, "PNG")
        self.regSVG.png = str(bf.buffer())

        self.accept()


class WTV_SVGs(QTVarios.WDialogo):
    def __init__(self, owner, listaSVGs, dbSVGs):

        titulo = _("Images")
        icono = Iconos.SVGs()
        extparam = "svgs"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.owner = owner

        flb = Controles.TipoLetra(puntos=8)

        self.configuracion = VarGen.configuracion
        self.liPSVGs = listaSVGs
        self.dbSVGs = dbSVGs

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 60, siCentrado=True)
        oColumnas.nueva("NOMBRE", _("Name"), 256)

        self.grid = Grid.Grid(self, oColumnas, xid="M", siSelecFilas=True)

        liAcciones = [
            (_("Close"), Iconos.MainMenu(), "terminar"), None,
            (_("New"), Iconos.Nuevo(), "mas"), None,
            (_("Remove"), Iconos.Borrar(), "borrar"), None,
            (_("Modify"), Iconos.Modificar(), "modificar"), None,
            (_("Copy"), Iconos.Copiar(), "copiar"), None,
            (_("Up"), Iconos.Arriba(), "arriba"), None,
            (_("Down"), Iconos.Abajo(), "abajo"), None,
        ]
        tb = Controles.TB(self, liAcciones)
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

        # Ejemplos
        liMovs = ["g4h3", "e2e2", "d6f4"]
        self.liEjemplos = []
        regSVG = TabTipos.SVG()
        for a1h8 in liMovs:
            regSVG.a1h8 = a1h8
            regSVG.xml = estrellaSVG
            regSVG.siMovible = True
            svg = self.tablero.creaSVG(regSVG, siEditando=True)
            self.liEjemplos.append(svg)

        self.grid.gotop()
        self.grid.setFocus()

    def closeEvent(self, event):
        self.guardarVideo()

    def terminar(self):
        self.guardarVideo()
        self.close()

    def gridNumDatos(self, grid):
        return len(self.liPSVGs)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        if clave == "NUMERO":
            return str(fila + 1)
        elif clave == "NOMBRE":
            return self.liPSVGs[fila].nombre

    def gridDobleClick(self, grid, fila, oColumna):
        self.modificar()

    def gridCambiadoRegistro(self, grid, fila, oColumna):
        if fila >= 0:
            regSVG = self.liPSVGs[fila]
            for ejemplo in self.liEjemplos:
                a1h8 = ejemplo.bloqueDatos.a1h8
                bd = copy.deepcopy(regSVG)
                bd.a1h8 = a1h8
                bd.anchoCasilla = self.tablero.anchoCasilla
                ejemplo.bloqueDatos = bd
                ejemplo.reset()
            self.tablero.escena.update()

    def procesarTB(self):
        accion = self.sender().clave
        eval("self.%s()" % accion)

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

        menu.opcion("@", _X(_("To seek %1 file"), "SVG"), Iconos.Fichero())

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
        w = WTV_SVG(self, None, xml=contenido, nombre=nombre)
        if w.exec_():
            regSVG = w.regSVG
            regSVG.id = Util.nuevoID()
            regSVG.ordenVista = (self.liPSVGs[-1].ordenVista + 1) if self.liPSVGs else 1
            self.dbSVGs[regSVG.id] = regSVG
            self.liPSVGs.append(regSVG)
            self.grid.refresh()
            self.grid.gobottom()
            self.grid.setFocus()

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), self.liPSVGs[fila].nombre)):
                regSVG = self.liPSVGs[fila]
                nid = regSVG.id
                del self.liPSVGs[fila]
                del self.dbSVGs[nid]
                self.grid.refresh()
                self.grid.setFocus()

    def modificar(self):
        fila = self.grid.recno()
        if fila >= 0:
            w = WTV_SVG(self, self.liPSVGs[fila])
            if w.exec_():
                regSVG = w.regSVG
                xid = regSVG.id
                self.liPSVGs[fila] = regSVG
                self.dbSVGs[xid] = regSVG
                self.grid.refresh()
                self.grid.setFocus()
                self.gridCambiadoRegistro(self.grid, fila, None)

    def copiar(self):
        fila = self.grid.recno()
        if fila >= 0:
            regSVG = copy.deepcopy(self.liPSVGs[fila])
            n = 1

            def siEstaNombre(nombre):
                for rf in self.liPSVGs:
                    if rf.nombre == nombre:
                        return True
                return False

            nombre = "%s-%d" % (regSVG.nombre, n)
            while siEstaNombre(nombre):
                n += 1
                nombre = "%s-%d" % (regSVG.nombre, n)
            regSVG.nombre = nombre
            regSVG.id = Util.nuevoID()
            regSVG.ordenVista = self.liPSVGs[-1].ordenVista + 1
            self.dbSVGs[regSVG.id] = regSVG
            self.liPSVGs.append(regSVG)
            self.grid.refresh()
            self.grid.setFocus()

    def intercambia(self, fila1, fila2):
        regSVG1, regSVG2 = self.liPSVGs[fila1], self.liPSVGs[fila2]
        regSVG1.ordenVista, regSVG2.ordenVista = regSVG2.ordenVista, regSVG1.ordenVista
        self.dbSVGs[regSVG1.id] = regSVG1
        self.dbSVGs[regSVG2.id] = regSVG2
        self.liPSVGs[fila1], self.liPSVGs[fila2] = self.liPSVGs[fila1], self.liPSVGs[fila2]
        self.grid.goto(fila2, 0)
        self.grid.refresh()
        self.grid.setFocus()

    def arriba(self):
        fila = self.grid.recno()
        if fila > 0:
            self.intercambia(fila, fila - 1)

    def abajo(self):
        fila = self.grid.recno()
        if 0 <= fila < (len(self.liPSVGs) - 1):
            self.intercambia(fila, fila + 1)
