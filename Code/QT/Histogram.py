# coding=utf-8

import os

from PyQt4 import QtCore, QtGui

from Code import Util
from Code import VarGen
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Iconos


class HSerie:
    def __init__(self, siPawns):
        self.liPoints = []
        self.siPawns = siPawns
        if siPawns:
            self.minimum = -5.0
            self.maximum = +5.0
        else:
            self.minimum = -500
            self.maximum = +500
        self.qcolor = { True:QtGui.QColor("#DACA99"), False:QtGui.QColor("#83C5F8") }

        # Elo para que 3400 - 1000 esten en los limites interiores
        self.maximum_elo = 4200
        self.minimum_elo = 200

    def addPoint(self, hpoint):
        hpoint.setGridPos(len(self.liPoints))
        self.liPoints.append(hpoint)

    def firstmove(self):
        return int(self.liPoints[0].nummove) if self.liPoints else 0

    def lastmove(self):
        return int(self.liPoints[-1].nummove) if self.liPoints else 0

    def lines(self):
        li = []
        for n in range(len(self.liPoints)-1):
            li.append((self.liPoints[n], self.liPoints[n+1]))
        return li

    def steps(self):
        return int(self.lastmove() - self.firstmove() + 1)

    def scenePoints(self, sz_width, sz_height, sz_left):
        ntotal_y = self.maximum - self.minimum
        self.factor = sz_height * 1.0 / ntotal_y
        ntotal_y_elo = self.maximum_elo - self.minimum_elo
        self.factor_elo = sz_height * 1.0 / ntotal_y_elo
        nmedia_y = ntotal_y / 2
        firstmove = self.firstmove()
        self.step = sz_width * 1.0 / self.steps()
        nmedia_x = len(self.liPoints)/2
        for npoint, point in enumerate(self.liPoints):
            point.minmax_rvalue(self.minimum, self.maximum)
            dr = ("s" if point.value > 0 else "n") + ("e" if npoint < nmedia_x else "w" )
            point.set_dir_tooltip(dr)
            rx = (point.nummove-firstmove)*self.step-sz_left
            ry = -(point.rvalue + nmedia_y)*self.factor
            ry_elo = -(point.elo-self.minimum_elo)*self.factor_elo
            point.set_rxy(rx, ry, ry_elo)


class HPoint:
    def __init__(self, nummove, value, lostp, lostp_abs, tooltip, elo):
        self.nummove = nummove
        self.rvalue = self.value = value
        self.tooltip = tooltip
        self.is_white = not "..." in tooltip
        self.dir_tooltip = ""
        self.rlostp = self.lostp = lostp
        self.lostp_abs = lostp_abs
        self.gridPos = None
        self.brush_color = self.setColor()
        self.elo = elo

    def setColor(self):
        # if self.lostp_abs > 80:
        #     return QtGui.QColor("#DC143C"), QtGui.QColor("#DC143C")
        if self.is_white:
            return QtCore.Qt.white, QtCore.Qt.black
        return QtCore.Qt.black, QtCore.Qt.black

    def setGridPos(self, gridPos):
        self.gridPos = gridPos

    def minmax_rvalue(self, minimum, maximum):
        if minimum > self.value:
            self.rvalue = minimum
        elif maximum < self.value:
            self.rvalue = maximum
        if self.rlostp > (maximum-minimum):
            self.rlostp = maximum-minimum

    def set_dir_tooltip(self, dr):
        self.dir_tooltip = dr

    def set_rxy(self, rx, ry, ry_elo):
        self.rx = rx
        self.ry = ry
        self.ry_elo = ry_elo

    def clone(self):
        return HPoint(self.nummove, self.value, self.lostp, self.lostp_abs, self.tooltip, self.elo)


class GraphPoint(QtGui.QGraphicsItem):
    def __init__(self, histogram, point, si_values):
        super(GraphPoint, self).__init__()

        self.histogram = histogram
        self.point = point

        self.setAcceptHoverEvents(True)

        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges)
        self.setCacheMode(QtGui.QGraphicsItem.DeviceCoordinateCache)
        self.setZValue(2)

        self.tooltipping = False
        self.si_values = si_values

    def hoverLeaveEvent(self, event):
        self.histogram.hide_tooltip()
        self.tooltipping = False

    def hoverMoveEvent(self, event):
        if not self.tooltipping:
            self.tooltipping = True
            ry = self.point.ry if self.si_values else self.point.ry_elo
            self.histogram.show_tooltip(self.point.tooltip, self.point.rx, ry, self.point.dir_tooltip)
            self.tooltipping = False

    def ponPos(self):
        ry = self.point.ry if self.si_values else self.point.ry_elo
        self.setPos(self.point.rx+4, ry+4)

    def boundingRect(self):
        return QtCore.QRectF(-6, -6, 6, 6)

    def paint(self, painter, option, widget):
        brush, color = self.point.brush_color
        painter.setPen(color)
        painter.setBrush(QtGui.QBrush(brush))
        painter.drawEllipse(-6, -6, 6, 6)

    def mousePressEvent(self, event):
        self.histogram.dispatch(self.point.gridPos)

    def mouseDoubleClickEvent(self, event):
        self.histogram.dispatch_enter(self.point.gridPos)


class GraphToolTip(QtGui.QGraphicsItem):
    def __init__(self, graph):
        super(GraphToolTip, self).__init__()

        self.graph = graph
        self.texto = ""

        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges)
        self.setCacheMode(QtGui.QGraphicsItem.DeviceCoordinateCache)
        self.setZValue(2)

    def setDispatch(self, dispatch):
        self.dispatch = dispatch

    def ponTextoPos(self, txt, x, y, dr):
        self.font = self.scene().font()
        self.font.setPointSize(12)
        self.metrics = QtGui.QFontMetrics(self.font)

        self.texto = txt
        self.dr = dr
        self.x = x
        self.y = y
        rancho = self.metrics.width(self.texto) + 10
        ralto = self.metrics.height() + 12

        rx = 10 if "e" in self.dr else -rancho
        ry = -ralto if "n" in self.dr else +ralto

        self.xrect = QtCore.QRectF(rx, ry, rancho, ralto)

        if "w" in self.dr:
            x -= 10
        if "n" in self.dr:
            y -= 10

        self.setPos(x, y)
        self.show()

    def boundingRect(self):
        return self.xrect

    def paint(self, painter, option, widget):
        painter.setFont(self.font)
        painter.setPen(QtGui.QColor("#545454"))
        painter.setBrush(QtGui.QBrush(QtGui.QColor("#F1EDED")))
        painter.drawRect(self.xrect)
        painter.drawText( self.xrect, QtCore.Qt.AlignCenter, self.texto)


class Histogram(QtGui.QGraphicsView):
    def __init__(self, owner, hserie, grid, ancho, si_values, elo_medio=None):
        super(Histogram, self).__init__()

        self.hserie = hserie

        self.owner = owner
        self.grid = grid

        self.elo_medio = elo_medio

        self.steps = hserie.steps()
        self.step = ancho / self.steps

        sz_width = self.steps * self.step
        sz_height = sz_left = ancho*300/900

        scene = QtGui.QGraphicsScene(self)
        scene.setItemIndexMethod(QtGui.QGraphicsScene.NoIndex)
        scene.setSceneRect(-sz_height, -sz_height, sz_width, sz_height)
        self.setScene(scene)
        self.scene = scene
        # self.setCacheMode(QtGui.QGraphicsView.CacheBackground)
        self.setViewportUpdateMode(QtGui.QGraphicsView.BoundingRectViewportUpdate)
        self.setRenderHint(QtGui.QPainter.Antialiasing)
        self.setTransformationAnchor(QtGui.QGraphicsView.AnchorUnderMouse)
        self.setResizeAnchor(QtGui.QGraphicsView.AnchorViewCenter)

        hserie.scenePoints(sz_width, sz_height, sz_left)

        self.si_values = si_values

        for point in hserie.liPoints:
            node = GraphPoint(self, point, si_values)
            scene.addItem(node)
            node.ponPos()

        self.pointActive = 0

        self.tooltip = GraphToolTip(self)
        scene.addItem(self.tooltip)
        self.tooltip.hide()

        self.scale(0.90, 0.8)
        #self.scale(0.45, 0.4)
        self.setMinimumSize(sz_width+26, sz_height+26)
        self.setPointActive(0)

    def dispatch(self, gridPos):
        self.grid.goto(gridPos, 0)
        self.grid.setFocus()

    def setPointActive(self, num):
        self.pointActive = num
        self.scene.invalidate()

    def dispatch_enter(self, gridPos):
        self.grid.setFocus()
        self.owner.gridDobleClick(self.grid, gridPos, 0)

    def show_tooltip(self, txt, x, y, dr):
        self.tooltip.ponTextoPos(txt, x, y, dr)

    def hide_tooltip(self):
        self.tooltip.hide()

    def drawBackground(self, painter, rect):
        sr = sceneRect = self.sceneRect()
        width = sr.width()
        height = sr.height()
        left = sr.left()
        right = sr.right()
        top = sr.top()
        bottom = sr.bottom()
        serie = self.hserie

        firstmove = self.hserie.firstmove()

        painter.setBrush(QtCore.Qt.NoBrush)

        textRect = QtCore.QRectF(left - 2, bottom + 4, width + 2, height)
        font = painter.font()
        font.setPointSize(8)
        painter.setFont(font)
        njg = self.steps + 1
        step = self.step

        # Numeros de jugada, en dos lineas
        for x in range(njg-1):
            num = firstmove + x
            decimal = num/10
            if decimal:
                painter.drawText(textRect.translated(x*step, 0), str(decimal))
        for x in range(njg-1):
            num = firstmove + x
            ent = num%10
            painter.drawText(textRect.translated(x*step, 12), str(ent))

        # Lineas verticales de referencia
        painter.setPen(QtGui.QColor("#D9D9D9"))
        for x in range(1, njg-1):
            t = left+step*x
            painter.drawLine(t, top, t, bottom)

        # Eje de las y a la izquierda
        painter.setPen(QtGui.QColor("#545454"))
        align_right = QtCore.Qt.AlignRight
        h = 12
        w = 24
        coord = [-3.0, -1.5, 0.0, +1.5, +3.0]
        plant = "%+0.1f"

        x = left - 31
        if self.si_values:
            if not serie.siPawns:
                coord = [int(rx*100) for rx in coord]
                plant = "%+d"
            for d in coord:
                y = bottom - height/2 - d*serie.factor - h/2
                painter.drawText(x, y, w, h, align_right, plant % d)
        else:
            coord[4] = +3.25  # 3500 = max = 1300/400
            for n, d in enumerate(coord):
                y = bottom - height/2 - d * serie.factor - h/2
                if n == 0:
                    rot = _("Min elo")
                elif n == 4:
                    rot = _("Max elo")
                else:
                    d = int(1000 + 600 * (d * 2 + 6) / 3)
                    rot = str(d)
                painter.drawText(x-100, y, w+100, h, align_right, rot)
            pen = painter.pen()
            pen.setWidth(4)
            pen.setColor(QtCore.Qt.darkGreen)
            painter.setPen(pen)
            d = (self.elo_medio - 1000)/400.0 - 3.0
            y = bottom - height / 2 - d * serie.factor
            painter.drawLine(left, y, right, y)
            painter.drawText(right + 5, y - h/2, 500, h*2, QtCore.Qt.AlignLeft, "%d %s" % (self.elo_medio, _("Average")))

        # Linea de referencia en la mitad-horizontal
        painter.setPen(QtCore.Qt.black)
        t = top+height*0.50
        painter.drawLine(left, t, right, t)

        # Lineas referencia horizontal
        painter.setPen(QtGui.QColor("#D9D9D9"))
        for d in coord:
            if d:
                t = bottom-height/2-d*serie.factor
                painter.drawLine(left, t, right, t)

        # Barras de los puntos perdidos
        if self.owner.valorShowLostPoints():
            n = max(serie.step / 2.0 - 2, 4)/2.0
            color = QtGui.QColor("#FFCECE")
            painter.setBrush(QtGui.QBrush(color))
            painter.setPen(color)
            for p in serie.liPoints:
                if p.rlostp:
                    y = bottom - p.rlostp * serie.factor
                    rect = QtCore.QRectF(p.rx-n, bottom-1, n*2, y+2)
                    painter.drawRect(rect)
                    p.rect_lost = rect

            painter.setBrush(QtGui.QBrush())

        # Lineas que unen los puntos
        pen = painter.pen()
        pen.setWidth(4)
        for is_white in (True, False):
            pen.setColor(serie.qcolor[is_white])
            painter.setPen(pen)
            for p, p1 in serie.lines():
                if p.is_white == is_white:
                    if self.si_values:
                        ry = p.ry
                        ry1 = p1.ry
                    else:
                        ry = p.ry_elo
                        ry1 = p1.ry_elo
                    painter.drawLine(p.rx+1, ry, p1.rx, ry1)

        painter.setBrush(QtGui.QBrush())

        # Caja exterior
        pen = painter.pen()
        pen.setWidth(1)
        pen.setColor(QtGui.QColor("#545454"))
        painter.setPen(pen)
        painter.drawRect(sceneRect)

        # Linea roja de la posicion actual
        pen = painter.pen()
        pen.setWidth(2)
        pen.setColor(QtGui.QColor("#DE5044"))
        painter.setPen(pen)
        if 0 <= self.pointActive < len(self.hserie.liPoints):
            p = serie.liPoints[self.pointActive]
            painter.drawLine(p.rx, bottom, p.rx, top)

    def mousePressEvent(self, event):
        super(Histogram, self).mousePressEvent(event)
        ep = self.mapToScene(event.pos())
        if self.owner.valorShowLostPoints():
            for p in self.hserie.liPoints:
                if p.rlostp:
                    if p.rect_lost.contains(ep):
                        self.dispatch(p.gridPos)
        if event.button() == QtCore.Qt.RightButton:
            menu = QTVarios.LCMenu(self)
            menu.opcion("clip", _("Copy to clipboard"), Iconos.Clip())
            menu.separador()
            menu.opcion("file", _("Save") + " png", Iconos.GrabarFichero())
            resp = menu.lanza()
            if resp:
                pm = QtGui.QPixmap.grabWidget(self)
                if resp == "clip":
                    QTUtil.ponPortapapeles(pm, tipo="p")
                else:
                    configuracion = VarGen.configuracion
                    path = QTUtil2.salvaFichero(self, _("File to save"), configuracion.dirSalvados,
                                                "%s PNG (*.png)" % _("File"), False)
                    if path:
                        pm.save(path, "png")
                        configuracion.dirSalvados = os.path.dirname(path)
                        configuracion.graba()

    def mouseDoubleClickEvent(self, event):
        super(Histogram, self).mouseDoubleClickEvent(event)
        ep = self.mapToScene( event.pos() )
        for p in self.hserie.liPoints:
            if p.rlostp:
                if p.rect_lost.contains(ep):
                    self.dispatch_enter(p.gridPos)

    def wheelEvent(self, event):
        k = QtCore.Qt.Key_Left if event.delta() > 0 else QtCore.Qt.Key_Right
        self.owner.gridTeclaControl(self.grid, k, False, False, False)


def genHistograms(partida, sicentipawns):
    siPawns = not sicentipawns
    hgame = HSerie(siPawns)
    hwhite = HSerie(siPawns)
    hblack = HSerie(siPawns)

    lijg = []
    lijgW = []
    lijgB = []

    porcT = 0
    porcW = 0
    porcB = 0

    for num, jg in enumerate(partida.liJugadas):
        if jg.analisis:
            mrm, pos = jg.analisis
            siBlancas = jg.siBlancas()
            pts = mrm.liMultiPV[pos].puntosABS()
            pts0 = mrm.liMultiPV[0].puntosABS()
            jg.lostp_abs = lostp_abs = pts0 - pts

            porc = jg.porcentaje = 100 - lostp_abs if lostp_abs < 100 else 0
            porcT += porc

            lijg.append(jg)
            if siBlancas:
                lijgW.append(jg)
                porcW += porc
            else:
                pts = -pts
                pts0 = -pts0
                lijgB.append(jg)
                porcB += porc

            if siPawns:
                pts /= 100.0
                pts0 /= 100.0
            lostp = abs(pts0-pts)

            nj = num / 2.0 + 1.0
            rotulo = "%d." % int(nj)
            if not siBlancas:
                rotulo += ".."
            jg.xnum = rotulo
            rotulo += jg.pgnSP()

            jg.xsiW = siBlancas

            tooltip = rotulo + " " + ("%+0.02f" if siPawns else "%+d") % pts
            if lostp:
                tooltip += " ?" +("%0.02f" if siPawns else "%d") % lostp
            else:
                tooltip += "!"
            tooltip += " (%d)" % jg.elo
            hp = HPoint(nj, pts, lostp, lostp_abs, tooltip, jg.elo)
            hgame.addPoint(hp)
            if siBlancas:
                hwhite.addPoint(hp.clone())
            else:
                hblack.addPoint(hp.clone())

    alm = Util.Almacen()
    alm.hgame = hgame
    alm.hwhite = hwhite
    alm.hblack = hblack

    alm.lijg = lijg
    alm.lijgW = lijgW
    alm.lijgB = lijgB

    alm.porcT = porcT*1.0/len(lijg) if len(lijg) else 0
    alm.porcW = porcW*1.0/len(lijgW) if len(lijgW) else 0
    alm.porcB = porcB*1.0/len(lijgB) if len(lijgB) else 0

    return alm

