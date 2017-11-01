from PyQt4 import QtCore, QtGui, QtSvg

from Code.QT import TabBloques


class SVGSC(TabBloques.BloqueEspSC):
    def __init__(self, escena, bloqueImgSVG, rutinaPulsada=None, siEditando=False):

        super(SVGSC, self).__init__(escena, bloqueImgSVG)

        self.rutinaPulsada = rutinaPulsada
        self.rutinaPulsadaCarga = None

        self.distBordes = 0.30 * bloqueImgSVG.anchoCasilla

        self.pixmap = QtSvg.QSvgRenderer(QtCore.QByteArray(bloqueImgSVG.xml))

        self.posicion2xy()

        self.siMove = False
        self.tpSize = None

        self.siRecuadro = False
        if siEditando:
            self.setAcceptsHoverEvents(True)

    def hoverEnterEvent(self, event):
        self.siRecuadro = True
        self.update()

    def hoverLeaveEvent(self, event):
        self.siRecuadro = False
        self.update()

    def ponRutinaPulsada(self, rutina, carga):
        self.rutinaPulsada = rutina
        self.rutinaPulsadaCarga = carga

    def reset(self):
        self.posicion2xy()
        bm = self.bloqueDatos
        self.pixmap = QtSvg.QSvgRenderer(QtCore.QByteArray(bm.xml))
        self.setOpacity(bm.opacidad)
        self.setZValue(bm.posicion.orden)
        self.update()

    def ponA1H8(self, a1h8):
        self.bloqueDatos.a1h8 = a1h8
        self.posicion2xy()

    def posicion2xy(self):

        bm = self.bloqueDatos
        posicion = bm.posicion
        ac = bm.anchoCasilla

        df, dc, hf, hc = self.tablero.a1h8_fc(bm.a1h8)

        if df > hf:
            df, hf = hf, df
        if dc > hc:
            dc, hc = hc, dc

        posicion.x = ac * (dc - 1)
        posicion.y = ac * (df - 1)
        posicion.ancho = (hc - dc + 1) * ac
        posicion.alto = (hf - df + 1) * ac

    def coordinaPosicionOtro(self, otroSVG):
        bs = self.bloqueDatos
        bso = otroSVG.bloqueDatos

        xk = float(bs.anchoCasilla * 1.0 / bso.anchoCasilla)
        posicion = bs.posicion
        posiciono = bso.posicion
        posicion.x = int(posiciono.x * xk)
        posicion.y = int(posiciono.y * xk)
        posicion.ancho = int(posiciono.ancho * xk)
        posicion.alto = int(posiciono.alto * xk)

    def contiene(self, p):
        p = self.mapFromScene(p)
        def distancia(p1, p2):
            t = p2 - p1
            return ((t.x()) ** 2 + (t.y()) ** 2) ** 0.5

        posicion = self.bloqueDatos.posicion
        dx = posicion.x
        dy = posicion.y
        ancho = posicion.ancho
        alto = posicion.alto

        self.rect = rect = QtCore.QRectF(dx, dy, ancho, alto)
        dicEsquinas = {"tl": rect.topLeft(), "tr": rect.topRight(), "bl": rect.bottomLeft(), "br": rect.bottomRight()}

        db = self.distBordes
        self.tpSize = None
        for k, v in dicEsquinas.iteritems():
            if distancia(p, v) <= db:
                self.tpSize = k
                return True
        self.siMove = self.rect.contains(p)
        return self.siMove

    def nombre(self):
        return _("Image")

    def mousePressEvent(self, event):
        QtGui.QGraphicsItem.mousePressEvent(self, event)

        p = event.scenePos()
        self.expX = p.x()
        self.expY = p.y()

    def mousePressExt(self, event):
        p = event.pos()
        p = self.mapFromScene(p)
        self.expX = p.x()
        self.expY = p.y()

    def mouseMoveEvent(self, event):
        event.ignore()
        if not (self.siMove or self.tpSize):
            return

        p = event.pos()
        p = self.mapFromScene(p)
        x = p.x()
        y = p.y()

        dx = x - self.expX
        dy = y - self.expY

        self.expX = x
        self.expY = y

        posicion = self.bloqueDatos.posicion
        if self.siMove:
            posicion.x += dx
            posicion.y += dy
        else:
            tp = self.tpSize
            if tp == "br":
                posicion.ancho += dx
                posicion.alto += dy
            elif tp == "bl":
                posicion.x += dx
                posicion.ancho -= dx
                posicion.alto += dy
            elif tp == "tr":
                posicion.y += dy
                posicion.ancho += dx
                posicion.alto -= dy
            elif tp == "tl":
                posicion.x += dx
                posicion.y += dy
                posicion.ancho -= dx
                posicion.alto -= dy

        self.escena.update()

    def mouseReleaseEvent(self, event):
        QtGui.QGraphicsItem.mouseReleaseEvent(self, event)
        if self.siActivo:
            if self.siMove or self.tpSize:
                self.escena.update()
                self.siMove = False
                self.tpSize = None
            self.activa(False)

        if self.rutinaPulsada:
            if self.rutinaPulsadaCarga:
                self.rutinaPulsada(self.rutinaPulsadaCarga)
            else:
                self.rutinaPulsada()

    def mouseReleaseExt(self):
        if self.siActivo:
            if self.siMove or self.tpSize:
                self.escena.update()
                self.siMove = False
                self.tpSize = None
            self.activa(False)

    def pixmapX(self):
        bm = self.bloqueDatos

        p = bm.posicion

        p.x = 0
        p.y = 0
        p.ancho = 32
        ant_psize = bm.psize
        bm.psize = 100

        p.alto = p.ancho

        pm = QtGui.QPixmap(p.ancho + 1, p.ancho + 1)
        pm.fill(QtCore.Qt.transparent)

        painter = QtGui.QPainter()
        painter.begin(pm)
        self.paint(painter, None, None)
        painter.end()

        self.ponA1H8(bm.a1h8)
        bm.psize = ant_psize

        return pm

    def paint(self, painter, option, widget):

        bm = self.bloqueDatos

        posicion = bm.posicion
        dx = posicion.x
        dy = posicion.y
        ancho = posicion.ancho
        alto = posicion.alto

        psize = bm.psize
        if psize != 100:
            anchon = ancho * psize / 100
            dx += (ancho - anchon) / 2
            ancho = anchon
            alton = alto * psize / 100
            dy += (alto - alton) / 2
            alto = alton

        self.rect = rect = QtCore.QRectF(dx, dy, ancho, alto)

        self.pixmap.render(painter, rect)

        if self.siRecuadro:
            pen = QtGui.QPen()
            pen.setColor(QtGui.QColor("blue"))
            pen.setWidth(2)
            pen.setStyle(QtCore.Qt.DashLine)
            painter.setPen(pen)
            painter.drawRect(rect)

            # def boundingRect(self):
            # x = self.bloqueDatos.grosor
            # return QtCore.QRectF(self.rect).adjusted( -x, -x, x*2, x*2 )


class SVGCandidate(SVGSC):
    def posicion2xy(self):

        bm = self.bloqueDatos
        posicion = bm.posicion
        ac = bm.anchoCasilla

        df, dc, hf, hc = self.tablero.a1h8_fc(bm.a1h8)

        if df > hf:
            df, hf = hf, df
        if dc > hc:
            dc, hc = hc, dc

        ancho = bm.anchoCasilla * 0.3
        posicion.x = ac * (dc - 1)
        posicion.y = ac * (df - 1)

        posCuadro = bm.posCuadro
        if posCuadro == 1:
            posicion.x += ac - ancho
        elif posCuadro == 2:
            posicion.y += ac - ancho
        elif posCuadro == 3:
            posicion.y += ac - ancho
            posicion.x += ac - ancho

        posicion.ancho = ancho
        posicion.alto = ancho
