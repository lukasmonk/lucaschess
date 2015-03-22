import os
import collections
import codecs
import base64
from encodings.aliases import aliases
import chardet.universaldetector

from PyQt4 import QtCore, QtGui, QtSvg

import Code.Util as Util
import Code.VarGen as VarGen
import Code.BaseConfig as BaseConfig
import Code.PGN as PGN
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.FormLayout as FormLayout

class DragUna(Controles.LB):
    def __init__(self, owner, pmVacio):
        self.owner = owner
        self.pixmap = None
        self.id = None
        self.toolTip = None
        Controles.LB.__init__(self, owner)
        self.ponImagen(pmVacio)

    def pon(self, pixmap, tooltip, id):
        if pixmap:
            self.ponImagen(pixmap)
        self.id = id
        self.setToolTip(tooltip)
        self.pixmap = pixmap

    def mousePressEvent(self, event):
        eb = event.button()
        if self.id is None or eb == QtCore.Qt.RightButton:

            self.owner.seleccionar(self)

        else:
            if eb == QtCore.Qt.LeftButton:
                self.owner.startDrag(self)

class DragBanda(QtGui.QWidget):
    def __init__(self, owner, liElem, ancho, margen=None):
        QtGui.QWidget.__init__(self)

        self.owner = owner
        self.ancho = ancho

        layout = Colocacion.G()
        self.liLB = []
        pm = Iconos.pmEnBlanco()
        if ancho != 32:
            pm = pm.scaled(ancho, ancho)
        self.pmVacio = pm
        for fila, numElem in enumerate(liElem):
            for n in range(numElem):
                lb = DragUna(self, self.pmVacio)
                self.liLB.append(lb)
                layout.control(lb, fila, n)
        if margen:
            layout.margen(margen)
        self.dicDatos = collections.OrderedDict()
        self.setLayout(layout)

    def seleccionar(self, lb):
        if not self.dicDatos:
            return

        # Los dividimos por tipos
        dic = collections.OrderedDict()
        for id, (nom, pm, tipo) in self.dicDatos.iteritems():
            if tipo not in dic:
                dic[tipo] = collections.OrderedDict()
            dic[tipo][id] = (nom, pm)

        menu = LCMenu(self)
        dicmenu = {}
        for id, (nom, pm, tp) in self.dicDatos.iteritems():
            if tp not in dicmenu:
                dicmenu[tp] = menu.submenu(tp, Iconos.PuntoVerde())
                menu.separador()
            dicmenu[tp].opcion(id, nom, QtGui.QIcon(pm))
        if lb.id is not None:
            menu.separador()
            menu.opcion(-1, _("Edit"), Iconos.Modificar())
            menu.separador()
            menu.opcion(-2, _("Remove"), Iconos.Delete())
        resp = menu.lanza()

        if resp is not None:
            if resp == -1:
                self.owner.editarBanda(lb.id)
            elif resp == -2:
                lb.pon(self.pmVacio, None, None)
            else:
                nom, pm, tp = self.dicDatos[resp]
                lb.pon(pm, nom, resp)

    def menuParaExterior(self, masOpciones):
        if not self.dicDatos:
            return None

        # Los dividimos por tipos
        dic = collections.OrderedDict()
        for id, (nom, pm, tipo) in self.dicDatos.iteritems():
            if tipo not in dic:
                dic[tipo] = collections.OrderedDict()
            dic[tipo][id] = (nom, pm)

        menu = LCMenu(self)
        dicmenu = {}
        for id, (nom, pm, tp) in self.dicDatos.iteritems():
            if tp not in dicmenu:
                dicmenu[tp] = menu.submenu(tp, Iconos.PuntoVerde())
                menu.separador()
            dicmenu[tp].opcion((id, tp), nom, QtGui.QIcon(pm))
        for clave, nombre, icono in masOpciones:
            menu.separador()
            menu.opcion(clave, nombre, icono)

        resp = menu.lanza()

        return resp

    def iniActualizacion(self):
        self.setControl = set()

    def actualiza(self, id, nombre, pixmap, tipo):
        self.dicDatos[id] = (nombre, pixmap, tipo)
        self.setControl.add(id)

    def finActualizacion(self):
        st = set()
        for id in self.dicDatos:
            if id not in self.setControl:
                st.add(id)
        for id in st:
            del self.dicDatos[id]

        for n, lb in enumerate(self.liLB):
            if lb.id is not None:
                if lb.id in st:
                    lb.pon(self.pmVacio, None, None)
                else:
                    self.pon(lb.id, n)

    def pon(self, id, a):
        if a < len(self.liLB):
            if id in self.dicDatos:
                nom, pm, tipo = self.dicDatos[id]
                lb = self.liLB[a]
                lb.pon(pm, nom, id)

    def idLB(self, num):
        if 0 <= num < len(self.liLB):
            return self.liLB[num].id
        else:
            return None

    def guardar(self):
        li = [(lb.id, n) for n, lb in enumerate(self.liLB) if lb.id is not None]
        return li

    def recuperar(self, li):
        for id, a in li:
            self.pon(id, a)

    def startDrag(self, lb):

        pixmap = lb.pixmap
        dato = lb.id
        itemData = QtCore.QByteArray(str(dato))

        mimeData = QtCore.QMimeData()
        mimeData.setData('image/x-lc-dato', itemData)

        drag = QtGui.QDrag(self)
        drag.setMimeData(mimeData)
        drag.setHotSpot(QtCore.QPoint(pixmap.width() / 2, pixmap.height() / 2))
        drag.setPixmap(pixmap)

        drag.exec_(QtCore.Qt.MoveAction)

class WDialogo(QtGui.QDialog):
    def __init__(self, pantalla, titulo, icono, extparam):

        assert len(titulo)==0 or pantalla is not None

        super(WDialogo, self).__init__(pantalla)

        self.setWindowTitle(titulo)
        self.setWindowIcon(icono)
        flags = QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint | QtCore.Qt.WindowMaximizeButtonHint
        self.setWindowFlags(flags)

        self.ficheroVideo = VarGen.configuracion.plantillaVideo % extparam
        self.liGrids = []
        self.liSplitters = []

    def closeEvent(self, event):
        self.guardarVideo()

    def registrarGrid(self, grid):
        self.liGrids.append(grid)

    def registrarSplitter(self, splitter, name):
        self.liSplitters.append((splitter, name))

    def guardarVideo(self, dicExten=None):

        dic = {} if dicExten is None else dicExten

        pos = self.pos()
        dic["_POSICION_"] = "%d,%d" % (pos.x(), pos.y())

        tam = self.size()
        dic["_SIZE_"] = "%d,%d" % (tam.width(), tam.height())

        for grid in self.liGrids:
            grid.guardarVideo(dic)

        for sp, name in self.liSplitters:
            dic["SP_%s" % name] = sp.sizes()

        Util.guardaDIC(dic, self.ficheroVideo)

    def recuperarDicVideo(self):

        if Util.tamFichero(self.ficheroVideo) > 0:
            return Util.recuperaDIC(self.ficheroVideo)
        else:
            return None

    def recuperarVideo(self, siTam=True, anchoDefecto=None, altoDefecto=None):

        dic = self.recuperarDicVideo()
        wE, hE = QTUtil.tamEscritorio()
        if dic:
            wE, hE = QTUtil.tamEscritorio()
            x, y = dic["_POSICION_"].split(",")
            x = int(x)
            y = int(y)
            if not ( 0 <= x <= (wE - 50) ):
                x = 0
            if not ( 0 <= y <= (hE - 50) ):
                y = 0
            self.move(x, y)
            if siTam:
                if "_SIZE_" not in dic:
                    w, h = self.width(),self.height()
                    for k in dic:
                        if k.startswith( "_TAMA" ):
                            w, h = dic[k].split(",")
                else:
                    w, h = dic["_SIZE_"].split(",")
                w = int(w)
                h = int(h)
                if w > wE:
                    w = wE
                elif w < 20:
                    w = 20
                if h > (hE - 40):
                    h = hE - 40
                elif h < 20:
                    h = 20
                self.resize(w, h)
            for grid in self.liGrids:
                grid.recuperarVideo(dic)
                grid.ponAnchosColumnas()
            for sp, name in self.liSplitters:
                k = "SP_%s" % name
                if k in dic:
                    sp.setSizes(dic[k])
            return True
        else:
            if anchoDefecto or altoDefecto:
                if anchoDefecto is None:
                    anchoDefecto = self.width()
                if altoDefecto is None:
                    altoDefecto = self.height()
                if anchoDefecto > wE:
                    anchoDefecto = wE
                if altoDefecto > (hE - 40):
                    altoDefecto = hE - 40
                self.resize(anchoDefecto, altoDefecto)

        return False

class BlancasNegras(QtGui.QDialog):
    def __init__(self, parent):
        super(BlancasNegras, self).__init__(parent)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        icoP = VarGen.todasPiezas.iconoDefecto("P")
        icop = VarGen.todasPiezas.iconoDefecto("p")
        self.setWindowTitle(_("Choose Color"))
        self.setWindowIcon(icoP)

        btBlancas = Controles.PB(self, "", rutina=self.accept, plano=False).ponIcono(icoP, tamIcon=64)
        btNegras = Controles.PB(self, "", rutina=self.negras, plano=False).ponIcono(icop, tamIcon=64)

        self.resultado = True

        ly = Colocacion.H().control(btBlancas).control(btNegras)
        ly.margen(10)
        self.setLayout(ly)

    def negras(self):
        self.resultado = False
        self.accept()

def blancasNegras(owner):
    w = BlancasNegras(owner)
    if w.exec_():
        return w.resultado
    return None

class BlancasNegrasTiempo(QtGui.QDialog):
    def __init__(self, parent):
        QtGui.QDialog.__init__(self, parent)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        icoP = VarGen.todasPiezas.iconoDefecto("P")
        icop = VarGen.todasPiezas.iconoDefecto("p")
        self.setWindowTitle(_("Choose Color"))
        self.setWindowIcon(icoP)

        btBlancas = Controles.PB(self, "", rutina=self.blancas, plano=False).ponIcono(icoP, tamIcon=64)
        btNegras = Controles.PB(self, "", rutina=self.negras, plano=False).ponIcono(icop, tamIcon=64)

        # Tiempo
        self.edMinutos, self.lbMinutos = QTUtil2.spinBoxLB(self, 5, 0, 999, maxTam=50, etiqueta=_("Total minutes"))
        self.edSegundos, self.lbSegundos = QTUtil2.spinBoxLB(self, 10, 0, 999, maxTam=50,
                                                             etiqueta=_("Seconds added per move"))
        ly = Colocacion.G()
        ly.controld(self.lbMinutos, 0, 0).control(self.edMinutos, 0, 1)
        ly.controld(self.lbSegundos, 0, 2).control(self.edSegundos, 0, 3)
        self.gbT = Controles.GB(self, _("Time"), ly).conectar(self.cambiaTiempo)
        self.cambiaTiempo()

        self.color = None

        ly = Colocacion.H().control(btBlancas).control(btNegras)
        ly.margen(10)
        layout = Colocacion.V().otro(ly).espacio(10).control(self.gbT).margen(5)
        self.setLayout(layout)

    def resultado(self):
        return self.color, self.gbT.isChecked(), self.edMinutos.valor(), self.edSegundos.valor()

    def cambiaTiempo(self):
        si = self.gbT.isChecked()
        for control in ( self.edMinutos, self.lbMinutos, self.edSegundos, self.lbSegundos ):
            control.setVisible(si)

    def blancas(self):
        self.color = True
        self.accept()

    def negras(self):
        self.color = False
        self.accept()

def blancasNegrasTiempo(owner):
    w = BlancasNegrasTiempo(owner)
    if w.exec_():
        return w.resultado()
    return None

class Tiempo(QtGui.QDialog):
    def __init__(self, parent, minMinutos, minSegundos, maxMinutos, maxSegundos):
        super(Tiempo, self).__init__(parent)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.setWindowTitle(_("Time"))
        self.setWindowIcon(Iconos.MoverTiempo())

        tb = QTUtil2.tbAcceptCancel(self)

        # Tiempo
        self.edMinutos, self.lbMinutos = QTUtil2.spinBoxLB(self, 10, minMinutos, maxMinutos, maxTam=50,
                                                           etiqueta=_("Total minutes"))
        self.edSegundos, self.lbSegundos = QTUtil2.spinBoxLB(self, 0, minSegundos, maxSegundos, maxTam=50,
                                                             etiqueta=_("Seconds added per move"))

        # # Tiempo
        lyT = Colocacion.G()
        lyT.controld(self.lbMinutos, 0, 0).control(self.edMinutos, 0, 1)
        lyT.controld(self.lbSegundos, 1, 0).control(self.edSegundos, 1, 1).margen(20)

        ly = Colocacion.V().control(tb).espacio(20).otro(lyT)
        self.setLayout(ly)

    def aceptar(self):
        self.accept()

    def resultado(self):
        minutos = self.edMinutos.value()
        segundos = self.edSegundos.value()

        return minutos, segundos

def tiempo(owner, minMinutos=1, minSegundos=0, maxMinutos=999, maxSegundos=999):
    w = Tiempo(owner, minMinutos, minSegundos, maxMinutos, maxSegundos)
    if w.exec_():
        return w.resultado()
    return None

def lyBotonesMovimiento(owner, clave, siLibre=True, siMas=False, siTiempo=True, \
                        siGrabar=False, siGrabarTodos=False, siJugar=False, rutina=None, tamIcon=16,
                        liMasAcciones=None):
    liAcciones = []

    def x(tit, tr, icono):
        liAcciones.append(( tr, icono, clave + tit ))

    # liAcciones.append( None )
    x("MoverInicio", _("First move"), Iconos.MoverInicio())
    liAcciones.append(None)
    x("MoverAtras", _("Previous move"), Iconos.MoverAtras())
    liAcciones.append(None)
    x("MoverAdelante", _("Next move"), Iconos.MoverAdelante())
    liAcciones.append(None)
    x("MoverFinal", _("Last move"), Iconos.MoverFinal())
    liAcciones.append(None)
    if siLibre:
        x("MoverLibre", _("Analysis of variant"), Iconos.MoverLibre())
        liAcciones.append(None)
    if siJugar:
        x("MoverJugar", _("Play"), Iconos.MoverJugar())
        liAcciones.append(None)
    if siTiempo:
        x("MoverTiempo", _("Timed movement"), Iconos.MoverTiempo())
    liAcciones.append(None)
    if siGrabar:
        x("MoverGrabar", _("Save"), Iconos.MoverGrabar())
        liAcciones.append(None)
    if siGrabarTodos:
        liAcciones.append(( _("Save") + "++", Iconos.MoverGrabarTodos(), clave + "MoverGrabarTodos" ))
        liAcciones.append(None)
    if siMas:
        x("MoverMas", _("New analysis"), Iconos.MoverMas())

    if liMasAcciones:
        for trad, tit, icono in liMasAcciones:
            liAcciones.append(None)
            liAcciones.append((trad, icono, clave + tit))

    tb = Controles.TB(owner, liAcciones, False, tamIcon=tamIcon, rutina=rutina)
    ly = Colocacion.H().relleno().control(tb).relleno()
    return ly, tb

class LCNumero(QtGui.QWidget):
    def __init__(self, maxdigits):
        QtGui.QWidget.__init__(self)

        f = Controles.TipoLetra("", 11, 80, False, False, False, None)

        ly = Colocacion.H()
        self.liLB = []
        for x in range(maxdigits):
            lb = QtGui.QLabel(self)
            lb.setStyleSheet("* { border: 2px solid black; padding: 2px; margin: 0px;}")
            lb.setFont(f)
            ly.control(lb)
            self.liLB.append(lb)
            lb.hide()
        self.setLayout(ly)

    def pon(self, numero):
        c = str(numero)
        n = len(c)
        for x in range(n):
            lb = self.liLB[x]
            lb.setText(c[x])
            lb.show()
        for x in range(n, len(self.liLB)):
            self.liLB[x].hide()

class TwoImages(QtGui.QLabel):
    def __init__(self, pmTrue, pmFalse):
        self.pm = {True: pmTrue, False: pmFalse}
        self.pmFalse = pmFalse
        QtGui.QLabel.__init__(self)
        self.valor(False)

    def valor(self, ok=None):
        if ok is None:
            return self._valor
        else:
            self._valor = ok
            self.setPixmap(self.pm[ok])

    def mousePressEvent(self, event):
        self.valor(not self._valor)

def svg2ico(svg, tam):
    pm = QtGui.QPixmap(tam, tam)
    pm.fill(QtCore.Qt.transparent)
    qb = QtCore.QByteArray(svg)
    render = QtSvg.QSvgRenderer(qb)
    painter = QtGui.QPainter()
    painter.begin(pm)
    render.render(painter)
    painter.end()
    ico = QtGui.QIcon(pm)
    return ico

def fsvg2ico(fsvg, tam):
    f = codecs.open(fsvg, "r", 'utf-8', 'ignore')
    svg = f.read()
    f.close()
    return svg2ico(svg, tam)

def svg2pm(svg, tam):
    pm = QtGui.QPixmap(tam, tam)
    pm.fill(QtCore.Qt.transparent)
    qb = QtCore.QByteArray(svg)
    render = QtSvg.QSvgRenderer(qb)
    painter = QtGui.QPainter()
    painter.begin(pm)
    render.render(painter)
    painter.end()
    return pm

def fsvg2pm(fsvg, tam):
    f = codecs.open(fsvg, "r", 'utf-8', 'ignore')
    svg = f.read()
    f.close()
    return svg2pm(svg, tam)

def iconoTema(tema, tam):
    svg = """<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- Created with Inkscape (http://www.inkscape.org/) -->
<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:xlink="http://www.w3.org/1999/xlink"
   version="1.1"
   width="388pt"
   height="388pt"
   viewBox="0 0 388 388"
   id="svg2">
  <metadata
     id="metadata117">
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
  <defs
     id="defs115" />
  <g
     id="layer3"
     style="display:inline">
    <rect
       width="486.81006"
       height="486.81006"
       x="0"
       y="-0.35689625"
       transform="scale(0.8,0.8)"
       id="rect4020"
       style="fill:FONDO;fill-opacity:1;fill-rule:nonzero;stroke:none" />
  </g>
  <g
     id="layer1"
     style="display:inline">
    <rect
       width="316.67606"
       height="317.12463"
       ry="0"
       x="35.708782"
       y="34.520344"
       id="rect3095"
       style="fill:BLANCAS;stroke:RECUADRO;stroke-width:4.54554987;stroke-linecap:round;stroke-linejoin:miter;stroke-miterlimit:4;stroke-opacity:1;stroke-dasharray:none;stroke-dashoffset:0" />
  </g>
  <g
     id="layer2"
     style="display:inline">
    <rect
       width="38.841644"
       height="39.047188"
       x="154.92021"
       y="36.90279"
       id="rect3104"
       style="fill:NEGRAS;fill-opacity:1;stroke:NEGRAS;stroke-width:0.16;stroke-linecap:round;stroke-linejoin:miter;stroke-miterlimit:4;stroke-opacity:1;stroke-dasharray:none;stroke-dashoffset:0" />
    <use
       transform="translate(-78.883927,0)"
       id="use3887"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-118.64494,118.02342)"
       id="use3889"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-39.492576,196.10726)"
       id="use3891"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-118.64494,274.01176)"
       id="use3893"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(78.161342,3.0019919e-8)"
       id="use3903"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(156.08573,78.779427)"
       id="use3905"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-118.64494,196.10726)"
       id="use3907"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(38.395272,274.01176)"
       id="use3909"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(156.08573,3.0019984e-8)"
       id="use3919"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(0,78.779427)"
       id="use3921"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-78.883927,156.79797)"
       id="use3923"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-39.492576,274.01176)"
       id="use3925"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-118.64494,39.217809)"
       id="use3935"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(78.161342,78.779427)"
       id="use3937"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(0,156.79797)"
       id="use3939"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(0,235.54546)"
       id="use3941"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-39.492576,39.217809)"
       id="use3951"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-39.492576,118.02342)"
       id="use3953"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(38.395272,196.10726)"
       id="use3955"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(78.161342,235.54546)"
       id="use3957"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(38.395272,39.217809)"
       id="use3967"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(38.395272,118.02342)"
       id="use3969"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(78.161342,156.79797)"
       id="use3971"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(156.08573,235.54546)"
       id="use3973"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(116.52539,39.217809)"
       id="use3983"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(116.52539,118.02342)"
       id="use3985"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(116.52539,196.10726)"
       id="use3987"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(116.52539,274.01176)"
       id="use3989"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-78.883927,78.779427)"
       id="use3999"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(156.08573,156.79797)"
       id="use4001"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
    <use
       transform="translate(-78.883927,235.54546)"
       id="use4003"
       x="0"
       y="0"
       width="388"
       height="388"
       xlink:href="#rect3104" />
  </g>
</svg>
"""

    confTema = BaseConfig.ConfigTabTema()
    confTema.lee(tema["TEXTO"])

    thumbail = confTema._png64Thumb
    if thumbail:
        pm = QtGui.QPixmap()
        png = base64.b64decode(thumbail)
        pm.loadFromData(png)
        icono = QtGui.QIcon(pm)
        return icono

    def ccolor(ncolor):
        x = QtGui.QColor(ncolor)
        return x.name()

    svg = svg.replace("BLANCAS", ccolor(confTema._colorBlancas))
    svg = svg.replace("NEGRAS", ccolor(confTema._colorNegras))
    svg = svg.replace("FONDO", ccolor(confTema._colorExterior))
    svg = svg.replace("RECUADRO", ccolor(confTema._colorFrontera))

    return svg2ico(svg, tam)

class LBPieza(Controles.LB):
    def __init__(self, owner, pieza, tablero, tam):
        self.pieza = pieza
        self.owner = owner
        pixmap = tablero.piezas.pixmap(pieza, tam=tam)
        self.dragpixmap = pixmap
        Controles.LB.__init__(self, owner)
        self.ponImagen(pixmap).anchoFijo(tam).altoFijo(tam)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.owner.startDrag(self)

class ListaPiezas(QtGui.QWidget):
    def __init__(self, owner, lista, tablero, tam=None, margen=None):
        QtGui.QWidget.__init__(self)

        self.owner = owner
        # self.tablero = tablero = owner.tablero

        if tam is None:
            tam = tablero.anchoPieza

        liLB = []
        for fila, valor in enumerate(lista.split(";")):
            for columna, pieza in enumerate(valor.split(",")):
                lb = LBPieza(self, pieza, tablero, tam)
                liLB.append((lb, fila, columna))

        layout = Colocacion.G()
        for lb, fila, columna in liLB:
            layout.control(lb, fila, columna)
        if margen is not None:
            layout.margen(margen)

            # l1 = Colocacion.H().otro(layout).relleno()
            # if margen:
            # l1.margen(margen)
            # l2 = Colocacion.V().otro(l1)
            # if margen:
            # l2.margen(margen)

        self.setLayout(layout)

    def startDrag(self, lb):

        pixmap = lb.dragpixmap
        pieza = lb.pieza
        itemData = QtCore.QByteArray(str(pieza))

        self.owner.ultimaPieza = pieza

        mimeData = QtCore.QMimeData()
        mimeData.setData('image/x-lc-dato', itemData)

        drag = QtGui.QDrag(self)
        drag.setMimeData(mimeData)
        drag.setHotSpot(QtCore.QPoint(pixmap.width() / 2, pixmap.height() / 2))
        drag.setPixmap(pixmap)

        drag.exec_(QtCore.Qt.MoveAction)

def rondoPuntos():
    nico = Util.Rondo(Iconos.PuntoAmarillo(), Iconos.PuntoNaranja(), Iconos.PuntoVerde(), Iconos.PuntoAzul(),
                      Iconos.PuntoMagenta(), Iconos.PuntoRojo())
    nico.shuffle()
    return nico

def rondoColores():
    nico = Util.Rondo(Iconos.Amarillo(), Iconos.Naranja(), Iconos.Verde(), Iconos.Azul(), Iconos.Magenta(),
                      Iconos.Rojo())
    nico.shuffle()
    return nico

class LCMenu(Controles.Menu):
    def __init__(self, parent, puntos=None):
        configuracion = VarGen.configuracion
        if not puntos:
            puntos = configuracion.puntosMenu
        bold = configuracion.boldMenu
        Controles.Menu.__init__(self, parent, puntos=puntos, siBold=bold)

        # def pixelMetric( metric, option = None, widget = None):
        # s = QtGui.QStyle.pixelMetric(metric, option, widget)
        # pr metric
        # if metric == QtGui.QStyle.PM_SmallIconSize:
        # s = 32
        # return s

        # def setEstiloMenu( menu, pixels ):
        # sc = QtGui.QStyleFactory().create(VarGen.configuracion.estilo)
        # sc.pixelMetric = pixelMetric
        # menu.setStyle(sc)

class ImportarFicheroPGN(QtGui.QDialog):
    def __init__(self, parent):
        QtGui.QDialog.__init__(self, parent)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.setWindowTitle(_("PGN file"))
        self.setWindowIcon(Iconos.PGN())
        self.fontB = f = Controles.TipoLetra(puntos=10, peso=75)

        self.siCancelado = False

        lbRotLeidos = Controles.LB(self, _("Games read") + ":").ponFuente(f)
        self.lbLeidos = Controles.LB(self, "0").ponFuente(f)

        lbRotErroneos = Controles.LB(self, _("Erroneous") + ":").ponFuente(f)
        self.lbErroneos = Controles.LB(self, "0").ponFuente(f)

        self.lbRotDuplicados = Controles.LB(self, _("Duplicated") + ":").ponFuente(f)
        self.lbDuplicados = Controles.LB(self, "0").ponFuente(f)

        lbRotImportados = Controles.LB(self, _("Imported") + ":").ponFuente(f)
        self.lbImportados = Controles.LB(self, "0").ponFuente(f)

        self.btCancelarSeguir = Controles.PB(self, _("Cancel"), self.cancelar, plano=False).ponIcono(Iconos.Delete())

        # Tiempo
        ly = Colocacion.G().margen(20)
        ly.controld(lbRotLeidos, 0, 0).controld(self.lbLeidos, 0, 1)
        ly.controld(lbRotErroneos, 1, 0).controld(self.lbErroneos, 1, 1)
        ly.controld(self.lbRotDuplicados, 2, 0).controld(self.lbDuplicados, 2, 1)
        ly.controld(lbRotImportados, 3, 0).controld(self.lbImportados, 3, 1)

        lyBT = Colocacion.H().relleno().control(self.btCancelarSeguir)

        layout = Colocacion.V()
        layout.otro(ly)
        layout.espacio(20)
        layout.otro(lyBT)

        self.setLayout(layout)

    def hideDuplicados(self):
        self.lbRotDuplicados.hide()
        self.lbDuplicados.hide()

    def cancelar(self):
        self.siCancelado = True
        self.ponContinuar()

    def ponSaving(self):
        self.btCancelarSeguir.setDisabled(True)
        self.btCancelarSeguir.ponTexto(_("Saving..."))
        self.btCancelarSeguir.ponFuente(self.fontB)
        self.btCancelarSeguir.ponIcono(Iconos.Grabar())
        QTUtil.refreshGUI()

    def ponContinuar(self):
        self.btCancelarSeguir.ponTexto(_("Continue"))
        self.btCancelarSeguir.conectar(self.continuar)
        self.btCancelarSeguir.ponFuente(self.fontB)
        self.btCancelarSeguir.ponIcono(Iconos.Aceptar())
        self.btCancelarSeguir.setDisabled(False)
        QTUtil.refreshGUI()

    def continuar(self):
        self.accept()

    def actualiza(self, leidos, erroneos, duplicados, importados):
        def pts(x): return "{:,}".format(x).replace(",", ".")

        self.lbLeidos.ponTexto(pts(leidos))
        self.lbErroneos.ponTexto(pts(erroneos))
        self.lbDuplicados.ponTexto(pts(duplicados))
        self.lbImportados.ponTexto(pts(importados))
        QTUtil.refreshGUI()
        return not self.siCancelado

class MensajeFics(QtGui.QDialog):
    def __init__(self, parent, mens):
        QtGui.QDialog.__init__(self, parent)

        self.setWindowTitle(_("Fics-Elo"))
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
        self.setWindowIcon(Iconos.Fics())
        self.setStyleSheet("QDialog, QLabel { background: #E3F1F9 }")

        lbm = Controles.LB(self, "<big><b>%s</b></big>" % mens)
        self.bt = Controles.PB(self, _("One moment please..."), rutina=self.final, plano=True)
        self.bt.setDisabled(True)
        self.siFinalizado = False

        ly = Colocacion.G().control(lbm, 0, 0).controlc(self.bt, 1, 0)

        ly.margen(20)

        self.setLayout(ly)

    def continua(self):
        self.bt.ponTexto(_("Continue"))
        self.bt.ponPlano(False)
        self.bt.setDisabled(False)
        self.mostrar()

    def colocaCentrado(self, owner):
        self.move(owner.x() + owner.width() / 2 - self.width() / 2, owner.y() + owner.height() / 2 - self.height() / 2)
        QTUtil.refreshGUI()
        self.show()
        QTUtil.refreshGUI()
        return self

    def mostrar(self):
        QTUtil.refreshGUI()
        self.exec_()
        QTUtil.refreshGUI()

    def final(self):
        if not self.siFinalizado:
            self.accept()
        self.siFinalizado = True
        QTUtil.refreshGUI()

class MensajeFide(QtGui.QDialog):
    def __init__(self, parent, mens):
        QtGui.QDialog.__init__(self, parent)

        self.setWindowTitle(_("Fide-Elo"))
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
        self.setWindowIcon(Iconos.Fide())
        self.setStyleSheet("QDialog, QLabel { background: #E9E9E9 }")

        lbm = Controles.LB(self, "<big><b>%s</b></big>" % mens)
        self.bt = Controles.PB(self, _("One moment please..."), rutina=self.final, plano=True)
        self.bt.setDisabled(True)
        self.siFinalizado = False

        ly = Colocacion.G().control(lbm, 0, 0).controlc(self.bt, 1, 0)

        ly.margen(20)

        self.setLayout(ly)

    def continua(self):
        self.bt.ponTexto(_("Continue"))
        self.bt.ponPlano(False)
        self.bt.setDisabled(False)
        self.mostrar()

    def colocaCentrado(self, owner):
        self.move(owner.x() + owner.width() / 2 - self.width() / 2, owner.y() + owner.height() / 2 - self.height() / 2)
        QTUtil.refreshGUI()
        self.show()
        QTUtil.refreshGUI()
        return self

    def mostrar(self):
        QTUtil.refreshGUI()
        self.exec_()
        QTUtil.refreshGUI()

    def final(self):
        if not self.siFinalizado:
            self.accept()
        self.siFinalizado = True
        QTUtil.refreshGUI()

def savePGN( owner, pgn ):
    configuracion = VarGen.configuracion
    dicVariables = configuracion.leeVariables("SAVEPGN")

    liGen = [(None, None)]

    liHistorico = dicVariables.get("LIHISTORICO")

    config = FormLayout.Fichero(_("File to save"), "pgn", True, liHistorico=liHistorico, anchoMinimo=300)
    liGen.append(( config, "" ))

    #Codec
    liCodecs = [k for k in set(v for k,v in aliases.iteritems())]
    liCodecs.sort()
    liCodecs = [(k,k) for k in liCodecs]
    liCodecs.insert( 0, (_("Same as file"), "file" ) )
    liCodecs.insert( 0, ("%s: UTF-8"%_("By default"), "default" ) )
    config = FormLayout.Combobox(_("Write with the codec"), liCodecs)
    codec = dicVariables.get("CODEC", "default")
    liGen.append(( config, codec ))

    #Overwrite
    liGen.append( ( _("Overwrite"), dicVariables.get("OVERWRITE", False)) )

    #Remove comments
    liGen.append( ( _("Remove comments and variations"), dicVariables.get("REMCOMMENTSVAR", False)) )

    # Editamos
    resultado = FormLayout.fedit(liGen, title=_("Save PGN"), parent=owner, icon=Iconos.PGN())
    if resultado is None:
        return

    accion, liResp = resultado
    fichero, codec, overwrite, remcommentsvar = liResp
    if not fichero:
        return
    if not liHistorico:
        liHistorico = []
    if fichero in liHistorico:
        del liHistorico[liHistorico.index(fichero)]
        chardet
    liHistorico.insert(0,fichero)

    dicVariables["LIHISTORICO"] = liHistorico[:20]
    dicVariables["CODEC"] = codec
    dicVariables["OVERWRITE"] = overwrite
    dicVariables["REMCOMMENTSVAR"] = remcommentsvar

    configuracion.escVariables("SAVEPGN",dicVariables)
    carpeta, name = os.path.split(fichero)
    if carpeta != configuracion.dirSalvados:
        configuracion.dirSalvados = carpeta
        configuracion.graba()

    if remcommentsvar:
        pgn = PGN.rawPGN(pgn)
    pgn = pgn.replace( "\n", "\r\n" )

    modo = "w" if overwrite else "a"
    if not overwrite:
        if not Util.existeFichero(fichero):
            modo = "w"
    if codec == "default":
        codec = "utf-8"
    elif codec == "file":
        codec = "utf-8"
        if Util.existeFichero(fichero):
            with open(fichero) as f:
                u = chardet.universaldetector.UniversalDetector()
                for n, x in enumerate(f):
                    u.feed(x)
                    if n == 1000:
                        break
                u.close()
                codec = u.result.get("encoding", "utf-8")

    try:
        f = codecs.open( fichero, modo, codec, 'ignore' )
        if modo == "a":
            f.write( "\r\n\r\n" )
        f.write(pgn)
        f.close()
        QTUtil2.mensajeTemporal( owner, _( "Saved" ), 1.2 )
    except:
        QTUtil.ponPortapapeles(pgn)
        QTUtil2.mensError(owner, "%s : %s\n\n%s" % (_("Unable to save"), fichero, _("It is saved in the clipboard to paste it wherever you want.") ))

