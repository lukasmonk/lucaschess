from PyQt4 import QtCore, QtGui

from Code.QT import TabBloques


class MarcoSC(TabBloques.BloqueEspSC):
    def __init__(self, escena, bloqueMarco, rutinaPulsada=None):

        super(MarcoSC, self).__init__(escena, bloqueMarco)

        self.rutinaPulsada = rutinaPulsada
        self.rutinaPulsadaCarga = None

        self.distBordes = 0.20 * bloqueMarco.anchoCasilla

        self.posicion2xy()

        self.siMove = False
        self.tpSize = None

        # bm = self.bloqueDatos
        # posicion = bm.posicion
        # dx = posicion.x
        # dy = posicion.y
        # ancho = posicion.ancho
        # alto = posicion.alto
        # rect = QtCore.QRectF( dx, dy, ancho, alto )
        # self.dicEsquinas = { "tl":rect.topLeft(), "tr":rect.topRight(), "bl":rect.bottomLeft(), "br":rect.bottomRight() }

    def ponRutinaPulsada(self, rutina, carga):
        self.rutinaPulsada = rutina
        self.rutinaPulsadaCarga = carga

    def reset(self):
        self.posicion2xy()
        bm = self.bloqueDatos
        self.setOpacity(bm.opacidad)
        self.setZValue(bm.posicion.orden)
        self.update()

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

    def xy2posicion(self):
        bm = self.bloqueDatos
        posicion = bm.posicion
        ac = bm.anchoCasilla

        f = lambda xy: int(round(float(xy) / float(ac), 0))

        dc = f(posicion.x) + 1
        df = f(posicion.y) + 1
        hc = f(posicion.x + posicion.ancho)
        hf = f(posicion.y + posicion.alto)

        bien = lambda fc: (fc < 9) and (fc > 0)
        if bien(dc) and bien(df) and bien(hc) and bien(hf):
            bm.a1h8 = self.tablero.fc_a1h8(df, dc, hf, hc)

        self.posicion2xy()

    def ponA1H8(self, a1h8):
        self.bloqueDatos.a1h8 = a1h8
        self.posicion2xy()

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
        return _("Box")

    def mousePressEvent(self, event):
        QtGui.QGraphicsItem.mousePressEvent(self, event)
        self.mousePressExt(event)

        p = event.scenePos()
        self.expX = p.x()
        self.expY = p.y()

    def mousePressExt(self, event):
        """Needed in Scripts"""
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

    def mouseMoveExt(self, event):
        p = event.pos()
        p = self.mapFromScene(p)
        x = p.x()
        y = p.y()

        dx = x - self.expX
        dy = y - self.expY

        self.expX = x
        self.expY = y

        posicion = self.bloqueDatos.posicion
        posicion.ancho += dx
        posicion.alto += dy
        self.escena.update()

    def mouseReleaseEvent(self, event):
        QtGui.QGraphicsItem.mouseReleaseEvent(self, event)
        if self.siActivo:
            if self.siMove or self.tpSize:
                self.xy2posicion()
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
        self.xy2posicion()
        self.escena.update()
        self.siMove = False
        self.tpSize = None
        self.activa(False)

    def pixmap(self):
        bm = self.bloqueDatos

        xk = float(bm.anchoCasilla / 32.0)

        p = bm.posicion
        g = int(bm.grosor * xk)

        p.x = g
        p.y = g
        p.ancho = 32
        p.alto = p.ancho

        pm = QtGui.QPixmap(p.ancho + g * 2 + 1, p.ancho + g * 2 + 1)
        pm.fill(QtCore.Qt.transparent)

        painter = QtGui.QPainter()
        painter.begin(pm)
        self.paint(painter, None, None)
        painter.end()

        self.ponA1H8(bm.a1h8)

        return pm

    def paint(self, painter, option, widget):

        bm = self.bloqueDatos

        xk = float(bm.anchoCasilla / 32.0)

        posicion = bm.posicion
        dx = posicion.x
        dy = posicion.y
        ancho = posicion.ancho
        alto = posicion.alto

        self.rect = QtCore.QRectF(dx, dy, ancho, alto)

        color = QtGui.QColor(bm.color)
        pen = QtGui.QPen()
        pen.setWidth(int(bm.grosor * xk))
        pen.setColor(color)
        pen.setStyle(bm.tipoqt())
        pen.setCapStyle(QtCore.Qt.RoundCap)
        pen.setJoinStyle(QtCore.Qt.RoundJoin)
        painter.setPen(pen)

        pen = QtGui.QPen()
        pen.setColor(QtGui.QColor(bm.color))
        pen.setWidth(int(bm.grosor * xk))
        pen.setStyle(bm.tipoqt())
        painter.setPen(pen)
        if bm.colorinterior >= 0:
            color = QtGui.QColor(bm.colorinterior)
            if bm.colorinterior2 >= 0:
                color2 = QtGui.QColor(bm.colorinterior2)
                gradient = QtGui.QLinearGradient(0, 0, bm.posicion.ancho, bm.posicion.alto)
                gradient.setColorAt(0.0, color)
                gradient.setColorAt(1.0, color2)
                painter.setBrush(QtGui.QBrush(gradient))
            else:
                painter.setBrush(color)

        if bm.redEsquina:
            red = int(bm.redEsquina * xk)
            painter.drawRoundedRect(self.rect, red, red)
        else:
            painter.drawRect(self.rect)

    def boundingRect(self):
        x = self.bloqueDatos.grosor
        return QtCore.QRectF(self.rect).adjusted(-x, -x, x * 2, x * 2)
