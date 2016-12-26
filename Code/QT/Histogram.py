from PyQt4 import QtCore, QtGui

import math

from Code import Util

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
        nmedia_y = (self.maximum-self.minimum) / 2
        self.step = step = sz_width*1.0/self.steps()
        firstmove = self.firstmove()
        nmedia_x = len(self.liPoints)/2
        for npoint, point in enumerate(self.liPoints):
            point.minmax_rvalue(self.minimum, self.maximum)
            dr = ("s" if point.value > 0 else "n") + ("e" if npoint < nmedia_x else "w" )
            point.set_dir_tooltip(dr)
            rx = (point.nummove-firstmove)*step-sz_left
            ry = -(point.rvalue + nmedia_y)*self.factor
            point.set_rxy(rx, ry)

class HPoint:
    def __init__(self, nummove, value, lostp, lostp_abs, tooltip):
        self.nummove = nummove
        self.rvalue = self.value = value
        self.tooltip = tooltip
        self.is_white = not "..." in tooltip
        self.dir_tooltip = ""
        self.rlostp = self.lostp = lostp
        self.lostp_abs = lostp_abs
        self.gridPos = None
        self.brush_color = self.setColor()

    def setColor(self):
        if self.lostp_abs > 80:
            return QtGui.QColor("#DC143C"), QtGui.QColor("#DC143C")
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

    def set_rxy(self, rx, ry):
        self.rx = rx
        self.ry = ry

    def clone(self):
        return HPoint(self.nummove, self.value, self.lostp, self.lostp_abs, self.tooltip)

class GraphPoint(QtGui.QGraphicsItem):
    def __init__(self, histogram, point):
        super(GraphPoint, self).__init__()

        self.histogram = histogram
        self.point = point

        self.setAcceptHoverEvents(True)

        self.setFlag(QtGui.QGraphicsItem.ItemSendsGeometryChanges)
        self.setCacheMode(QtGui.QGraphicsItem.DeviceCoordinateCache)
        self.setZValue(2)

        self.tooltipping = False

    def hoverLeaveEvent(self, event):
        self.histogram.hide_tooltip()
        self.tooltipping = False

    def hoverMoveEvent(self, event):
        if not self.tooltipping:
            self.tooltipping = True
            self.histogram.show_tooltip(self.point.tooltip, self.point.rx, self.point.ry, self.point.dir_tooltip)
            self.tooltipping = True

    def ponPos(self):
        self.setPos(self.point.rx+4, self.point.ry+4)

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
    def __init__(self, owner, hserie, grid, ancho):
        super(Histogram, self).__init__()

        self.hserie = hserie

        self.owner = owner
        self.grid = grid

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

        for point in hserie.liPoints:
            node = GraphPoint(self, point)
            scene.addItem(node)
            node.ponPos()

        self.pointActive = 0

        self.tooltip = GraphToolTip(self)
        scene.addItem(self.tooltip)
        self.tooltip.hide()

        self.scale(0.90, 0.8)
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

        for x in range(njg-1):
            num = firstmove + x
            decimal = num/10
            if decimal:
                painter.drawText(textRect.translated(x*step, 0), str(decimal))

        for x in range(njg-1):
            num = firstmove + x
            ent = num%10
            painter.drawText(textRect.translated(x*step, 12), str(ent))

        painter.setPen(QtGui.QColor("#D9D9D9"))
        for x in range(1, njg-1):
            t = left+step*x
            painter.drawLine(t, top, t, bottom)

        painter.setPen(QtGui.QColor("#545454"))
        align = QtCore.Qt.AlignRight
        x = left-26
        h = 12
        w = 24
        coord = [-3.0, -1.5, 0.0, +1.5, +3.0]
        plant = "%+0.1f"
        if not serie.siPawns:
            coord = [int(rx*100) for rx in coord]
            plant = "%+d"

        for d in coord:
            y = bottom - height/2 - d*serie.factor - h/2
            painter.drawText( x, y, w, h, align, plant % d)

        painter.setPen(QtCore.Qt.black)
        t = top+height*0.50
        painter.drawLine(left, t, right, t)

        painter.setPen(QtGui.QColor("#D9D9D9"))
        for d in coord:
            if d:
                t = bottom-height/2-d*serie.factor
                painter.drawLine(left, t, right, t)

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

        pen = painter.pen()
        pen.setWidth(4)
        for is_white in (True, False):
            pen.setColor(serie.qcolor[is_white])
            painter.setPen(pen)
            for p, p1 in serie.lines():
                if p.is_white == is_white:
                    painter.drawLine(p.rx, p.ry, p1.rx, p1.ry)

        pen = painter.pen()
        pen.setWidth(1)
        pen.setColor(QtGui.QColor("#545454"))
        painter.setPen(pen)
        painter.drawRect(sceneRect)

        pen = painter.pen()
        pen.setWidth(2)
        pen.setColor(QtGui.QColor("#DE5044"))
        painter.setPen(pen)
        p = self.hserie.liPoints[self.pointActive]
        painter.drawLine(p.rx, bottom, p.rx, top)

    def mousePressEvent(self, event):
        super(Histogram, self).mousePressEvent(event)
        ep = self.mapToScene( event.pos() )
        for p in self.hserie.liPoints:
            if p.rlostp:
                if p.rect_lost.contains(ep):
                    self.dispatch(p.gridPos)

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
            pts = mrm.liMultiPV[pos].puntosABS_5()
            pts0 = mrm.liMultiPV[0].puntosABS_5()
            lostp_abs = pts0 - pts

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
            hp = HPoint(nj, pts, lostp, lostp_abs, tooltip)
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

    alm.porcT = porcT*1.0/len(lijg)
    alm.porcW = porcW*1.0/len(lijgW)
    alm.porcB = porcB*1.0/len(lijgB)

    return alm

# if __name__ == '__main__':

#     import sys

#     app = QtGui.QApplication(sys.argv)

#     wt = ['7.Ngf3', '8.O-O', '9.Qb3', '10.Qc3', '11.e3', '12.b3', '13.a3', '14.h4', '15.Ng5', '16.f3', '17.f4', '18.Rfd1', '19.Nh3', '20.Nf1', '21.a4', '22.Rd2', '23.Bh1', '24.Rg2', '25.bxc4', '26.Nf2', '27.Nd2', '28.Nd1', '29.Rxa5', '30.Rxa8', '31.Rxe8', '32.Rxf8+', '33.Nf2', '34.Nf1', '35.Rh2', '36.Rb8', '37.Rxb5', '38.g4', '39.Bxf3', '40.gxf5', '41.d5', '42.h5', '43.e4', '44.Nxe4', '45.d6', '46.f6', '47.Rd2', '48.Rxe2', '49.Kf2', '50.Kxf1', '51.Ke2', '52.Ke3', '53.Ke4']
#     wr = [0.66, 0.65, 0.78, 0.86, -1.35, -1.32, -1.37, -1.11, 0.2, 0.83, -0.03, -1.01, -0.83, -0.72, -0.96, -0.68, -1.54, -1.92, -2.24, -1.96, -2.05, -1.46, -1.68, -8.3, -1.51, -1.51, -1.51, -1.51, -1.51, -1.03, -1.98, -1.98, -1.58, -2.39, -2.24, -2.84, -2.23, -3.57, -4.15, -4.35, -4.27, -0.79, -0.79, -0.8, -0.79, -0.96, -51.39]
#     bt = ['6...Nc6', '7...O-O', '8...d6', '9...Kh8', '10...e5', '11...a5', '12...Qe8', '13...Qh5', '14...Ng4', '15...Bd7', '16...Nf6', '17...e4', '18...h6', '19...d5', '20...Ne7', '21...Nc6', '22...Nb4', '23...Qe8', '24...dxc4', '25...Bxa4', '26...Bd7', '27...b5', '28...Nd3', '29...b4', '30...bxc3', '31...c2', '32...Kh7', '33...c1=Q+', '34...Ne1', '35...Qxc4', '36...Bb5', '37...Qxb5', '38...Nf3+', '39...exf3', '40...Qe2', '41...Kg8', '42...Kh7', '43...Nxe4', '44...Qxe4', '45...cxd6', '46...gxf6', '47...Qe2', '48...fxe2', '49...exf1=Q+', '50...Kg7', '51...Kf7', '52...Ke6', '53...d5+']
#     br = [-0.91, -0.08, -0.65, -1.02, -1.23, -1.36, -0.84, -1.65, -1.55, -2.18, -2.08, -1.87, 1.44, 0.83, 0.59, 0.83, 1.1, 1.94, 1.92, 2.24, 2.05, 2.05, 1.68, 1.68, 1.51, 1.51, 1.51, 1.51, 1.51, 0.34, 0.01, 1.98, 1.58, 1.58, 0.2, 1.2, 1.17, 2.23, 3.57, 4.15, 4.32, 0.79, 0.79, 0.79, 0.78, 0.78, 51.57, 49.11]

#     hserie = HSerie(True, "#DACA99")

#     for x in range(len(bt)):
#         lb = bt[x]
#         nummove = int(lb.split(".")[0])
#         value = br[x]
#         tooltip = lb + " %+0.02f" % value
#         lostp = abs(value/3)
#         p = HPoint(nummove, value, lostp, tooltip)
#         hserie.addPoint(p)

#     widget = Histogram(hserie, True)
#     widget.show()

#     sys.exit(app.exec_())
