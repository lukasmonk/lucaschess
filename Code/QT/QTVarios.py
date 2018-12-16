import base64
import codecs
import os

from PyQt4 import QtCore, QtGui, QtSvg

from Code import BaseConfig
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code import Util
from Code import VarGen


class WSave():
    def __init__(self, titulo, icono, flag, extparam):
        self.key_video = extparam
        self.liGrids = []
        self.liSplitters = []
        self.setWindowTitle(titulo)
        self.setWindowIcon(icono)
        flags = flag | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint | QtCore.Qt.WindowMaximizeButtonHint
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | flags)

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

        VarGen.configuracion.save_video(self.key_video, dic)
        return dic

    def recuperarDicVideo(self):
        return VarGen.configuracion.restore_video(self.key_video)

    def recuperarVideo(self, siTam=True, anchoDefecto=None, altoDefecto=None, dicDef=None):
        dic = self.recuperarDicVideo()
        if not dic:
            dic = dicDef
        wE, hE = QTUtil.tamEscritorio()
        if dic:
            if "_POSICION_" in dic:
                x, y = dic["_POSICION_"].split(",")
                x = int(x)
                y = int(y)
                if not (0 <= x <= (wE - 50)):
                    x = 0
                if not (0 <= y <= (hE - 50)):
                    y = 0
                self.move(x, y)
            for grid in self.liGrids:
                grid.recuperarVideo(dic)
                grid.releerColumnas()
            for sp, name in self.liSplitters:
                k = "SP_%s" % name
                if k in dic:
                    sp.setSizes(dic[k])
            if siTam:
                if "_SIZE_" not in dic:
                    w, h = self.width(), self.height()
                    for k in dic:
                        if k.startswith("_TAMA"):
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


class WDialogo(QtGui.QDialog, WSave):
    def __init__(self, pantalla, titulo, icono, extparam):
        QtGui.QDialog.__init__(self, pantalla)
        WSave.__init__(self, titulo, icono, QtCore.Qt.Dialog, extparam)


class WWidget(QtGui.QWidget, WSave):
    def __init__(self, pantalla, titulo, icono, extparam):
        QtGui.QWidget.__init__(self, pantalla)
        WSave.__init__(self, titulo, icono, QtCore.Qt.Widget, extparam)

    def accept(self):
        self.guardarVideo()
        self.close()


class BlancasNegras(QtGui.QDialog):
    def __init__(self, parent):
        super(BlancasNegras, self).__init__(parent)
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        icoP = VarGen.todasPiezas.iconoDefecto("K")
        icop = VarGen.todasPiezas.iconoDefecto("k")
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
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        icoP = VarGen.todasPiezas.iconoDefecto("K")
        icop = VarGen.todasPiezas.iconoDefecto("k")
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

        # Fast moves
        self.chb_fastmoves = Controles.CHB(self, _("Fast moves"), False)

        self.color = None

        ly = Colocacion.H().control(btBlancas).control(btNegras)
        ly.margen(10)
        layout = Colocacion.V().otro(ly).espacio(10).control(self.gbT).control(self.chb_fastmoves).margen(5)
        self.setLayout(layout)

    def resultado(self):
        return self.color, self.gbT.isChecked(), self.edMinutos.valor(), self.edSegundos.valor(), self.chb_fastmoves.valor()

    def cambiaTiempo(self):
        si = self.gbT.isChecked()
        for control in (self.edMinutos, self.lbMinutos, self.edSegundos, self.lbSegundos):
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
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

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


def lyBotonesMovimiento(owner, clave, siLibre=True, siMas=False, siTiempo=True,
                        siGrabar=False, siGrabarTodos=False, siJugar=False, rutina=None, tamIcon=16,
                        liMasAcciones=None):
    liAcciones = []

    def x(tit, tr, icono):
        liAcciones.append((tr, icono, clave + tit))

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
        liAcciones.append((_("Save") + "++", Iconos.MoverGrabarTodos(), clave + "MoverGrabarTodos"))
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
        self.owner.ponCursor()

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


class ImportarFichero(QtGui.QDialog):
    def __init__(self, parent, titulo, siErroneos, icono):
        QtGui.QDialog.__init__(self, parent)
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.setWindowTitle(titulo)
        self.setWindowIcon(icono)
        self.fontB = f = Controles.TipoLetra(puntos=10, peso=75)

        self.siErroneos = siErroneos

        self.siCancelado = False

        lbRotLeidos = Controles.LB(self, _("Games read") + ":").ponFuente(f)
        self.lbLeidos = Controles.LB(self, "0").ponFuente(f)

        if siErroneos:
            lbRotErroneos = Controles.LB(self, _("Erroneous") + ":").ponFuente(f)
            self.lbErroneos = Controles.LB(self, "0").ponFuente(f)

        self.lbRotDuplicados = Controles.LB(self, _("Duplicated") + ":").ponFuente(f)
        self.lbDuplicados = Controles.LB(self, "0").ponFuente(f)

        self.lbRotImportados = lbRotImportados = Controles.LB(self, _("Imported") + ":").ponFuente(f)
        self.lbImportados = Controles.LB(self, "0").ponFuente(f)

        self.btCancelarSeguir = Controles.PB(self, _("Cancel"), self.cancelar, plano=False).ponIcono(Iconos.Delete())

        # Tiempo
        ly = Colocacion.G().margen(20)
        ly.controld(lbRotLeidos, 0, 0).controld(self.lbLeidos, 0, 1)
        if siErroneos:
            ly.controld(lbRotErroneos, 1, 0).controld(self.lbErroneos, 1, 1)
        ly.controld(self.lbRotDuplicados, 2, 0).controld(self.lbDuplicados, 2, 1)
        ly.controld(lbRotImportados, 3, 0).controld(self.lbImportados, 3, 1)

        lyBT = Colocacion.H().relleno().control(self.btCancelarSeguir)

        layout = Colocacion.V()
        layout.otro(ly)
        layout.espacio(20)
        layout.otro(lyBT)

        self.setLayout(layout)

    def pon_titulo(self, titulo):
        self.setWindowTitle(titulo)

    def hideDuplicados(self):
        self.lbRotDuplicados.hide()
        self.lbDuplicados.hide()

    def cancelar(self):
        self.siCancelado = True
        self.ponContinuar()

    def ponExportados(self):
        self.lbRotImportados.ponTexto(_("Exported") + ":")

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
        if self.siErroneos:
            self.lbErroneos.ponTexto(pts(erroneos))
        self.lbDuplicados.ponTexto(pts(duplicados))
        self.lbImportados.ponTexto(pts(importados))
        QTUtil.refreshGUI()
        return not self.siCancelado


class ImportarFicheroPGN(ImportarFichero):
    def __init__(self, parent):
        ImportarFichero.__init__(self, parent, _("PGN file"), True, Iconos.PGN())


class ImportarFicheroFNS(ImportarFichero):
    def __init__(self, parent):
        ImportarFichero.__init__(self, parent, _("FNS file"), True, Iconos.Fichero())


class ImportarFicheroDB(ImportarFichero):
    def __init__(self, parent):
        ImportarFichero.__init__(self, parent, _("Database file"), False, Iconos.Database())

    def actualiza(self, leidos, duplicados, importados):
        return ImportarFichero.actualiza(self, leidos, 0, duplicados, importados)


class MensajeFics(QtGui.QDialog):
    def __init__(self, parent, mens):
        QtGui.QDialog.__init__(self, parent)

        self.setWindowTitle(_("Fics-Elo"))
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
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
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
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


def select_pgn(wowner):
    configuracion = VarGen.configuracion
    path = QTUtil2.leeFichero(wowner, configuracion.dirPGN, "pgn")
    if path:
        carpeta, fichero = os.path.split(path)
        if configuracion.dirPGN != carpeta:
            configuracion.dirPGN = carpeta
            configuracion.graba()
    return path


def select_pgns(wowner):
    configuracion = VarGen.configuracion
    files = QTUtil2.leeFicheros(wowner, configuracion.dirPGN, "pgn")
    if files:
        path = files[0]
        carpeta, fichero = os.path.split(path)
        if configuracion.dirPGN != carpeta:
            configuracion.dirPGN = carpeta
            configuracion.graba()
    return files


def select_ext(wowner, ext):
    configuracion = VarGen.configuracion
    path = QTUtil2.leeFichero(wowner, configuracion.dirSalvados, ext)
    if path:
        carpeta, fichero = os.path.split(path)
        if configuracion.dirSalvados != carpeta:
            configuracion.dirSalvados = carpeta
            configuracion.graba()
    return path


def list_irina():
    return (
        ("Monkey", _("Monkey"), Iconos.Monkey()),
        ("Donkey", _("Donkey"), Iconos.Donkey()),
        ("Bull", _("Bull"), Iconos.Bull()),
        ("Wolf", _("Wolf"), Iconos.Wolf()),
        ("Lion", _("Lion"), Iconos.Lion()),
        ("Rat", _("Rat"), Iconos.Rat()),
        ("Snake", _("Snake"), Iconos.Snake()),
        ("Steven", _("Steven"), Iconos.Steven())
    )


def listaDB(configuracion, siFEN, siAll=False):
    if siFEN:
        ext = "lcf"
        base = configuracion.ficheroDBgamesFEN
        carpeta = configuracion.carpetaPositions
    else:
        ext = "lcg"
        base = configuracion.ficheroDBgames
        carpeta = configuracion.carpetaGames
    base = os.path.abspath(base)
    if siAll:
        lista = [fich for fich in os.listdir(carpeta) if fich.endswith("." + ext)]
    else:
        lista = [fich for fich in os.listdir(carpeta)
                 if fich.endswith("." + ext) and os.path.abspath(os.path.join(carpeta, fich)) != base
            ]
    return lista


def createDB(owner, configuracion, siFEN):
    if siFEN:
        ext = "lcf"
        rot = _("Positions Database")
        carpeta = configuracion.carpetaPositions
    else:
        ext = "lcg"
        rot = _("Database of complete games")
        carpeta = configuracion.carpetaGames
    database = QTUtil2.leeCreaFichero(owner, carpeta, ext, rot)
    if database:
        if not database.lower().endswith("." + ext):
            database = database + "." + ext
    return database


def selectDB(owner, configuracion, siFEN, siAll=False):
    lista = listaDB(configuracion, siFEN, siAll)
    if not lista:
        return None
    menu = LCMenu(owner)
    rp = rondoPuntos()
    carpeta = configuracion.carpetaPositions if siFEN else configuracion.carpetaGames
    for fich in lista:
        menu.opcion(os.path.join(carpeta, fich), _F(fich[:-4]), rp.otro())
        menu.separador()
    return menu.lanza()


def menuDB(submenu, configuracion, siFEN, siAll=False):
    lista = listaDB(configuracion, siFEN, siAll)
    if not lista:
        return
    rp = rondoPuntos()
    carpeta = configuracion.carpetaPositions if siFEN else configuracion.carpetaGames
    for fich in lista:
        submenu.opcion("dbf_%s" % os.path.join(carpeta, fich), _F(fich[:-4]), rp.otro())
        submenu.separador()


class ReadAnnotation(QtGui.QDialog):
    def __init__(self, parent, objetivo):
        QtGui.QDialog.__init__(self, parent)
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.FramelessWindowHint)

        self.edAnotacion = Controles.ED(self, "")
        btAceptar = Controles.PB(self, "", rutina=self.aceptar).ponIcono(Iconos.Aceptar())
        btCancelar = Controles.PB(self, "", rutina=self.cancelar).ponIcono(Iconos.Cancelar())
        btAyuda = Controles.PB(self, "", rutina=self.ayuda).ponIcono(Iconos.AyudaGR())

        self.objetivo = objetivo
        self.conAyuda = False
        self.errores = 0
        self.resultado = None

        layout = Colocacion.H().relleno(1).control(btAyuda).control(self.edAnotacion).control(btAceptar).control(btCancelar).margen(3)
        self.setLayout(layout)
        self.move(parent.x()+parent.tablero.width()-212, parent.y()+parent.tablero.y()-3)

    def aceptar(self):
        txt = self.edAnotacion.texto()
        txt = txt.strip().replace(" ", "").upper()

        if txt:
            if txt == self.objetivo.upper():
                self.resultado = self.conAyuda, self.errores
                self.accept()
            else:
                self.errores += 1
                self.edAnotacion.setStyleSheet("QWidget { color: red }")

    def cancelar(self):
        self.reject()

    def ayuda(self):
        self.conAyuda = True
        self.edAnotacion.ponTexto(self.objetivo)


class LCTB(Controles.TBrutina):
    def __init__(self, parent, liAcciones=None, siTexto=True, tamIcon=32, puntos=None, background=None):
        Controles.TBrutina.__init__(self, parent, liAcciones=liAcciones, siTexto=siTexto, tamIcon=tamIcon, puntos=puntos,
                                    background=background, style=VarGen.configuracion.iconsTB)
