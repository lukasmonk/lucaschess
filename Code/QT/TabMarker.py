from PyQt4 import QtCore, QtGui, QtSvg

from Code.QT import TabBloques


class MarkerSC(TabBloques.BloqueEspSC):
    def __init__(self, escena, bloqueMarker, rutinaPulsada=None, siEditando=False):
        super(MarkerSC, self).__init__(escena, bloqueMarker)

        self.rutinaPulsada = rutinaPulsada
        self.rutinaPulsadaCarga = None

        self.distBordes = 0.20 * bloqueMarker.anchoCasilla

        self.pixmap = QtSvg.QSvgRenderer(QtCore.QByteArray(bloqueMarker.xml))

        self.posicion2xy()

        self.siMove = False
        self.tpSize = None

        self.siEditando = siEditando

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
        self.siMove = True
        self.tpSize = None

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
        if self.siActivo:
            if self.siMove or self.tpSize:
                self.xy2posicion()
                self.escena.update()
                self.siMove = False
                self.tpSize = None
            self.activa(False)

    def pixmapX(self):
        pm = QtGui.QPixmap(33, 33)
        pm.fill(QtCore.Qt.transparent)
        painter = QtGui.QPainter()
        painter.begin(pm)
        rect = QtCore.QRectF(0, 0, 32, 32)
        self.pixmap.render(painter, rect)
        painter.end()
        return pm

    def nombre(self):
        return _("Marker")

    def paint(self, painter, option, widget):
        bm = self.bloqueDatos
        posicion = bm.posicion
        ac = bm.anchoCasilla
        poscelda = bm.poscelda
        psize = bm.psize

        def haz(a1h8):

            alto = ancho = bm.anchoCasilla * 0.3
            df, dc, hf, hc = self.tablero.a1h8_fc(a1h8)

            if df > hf:
                df, hf = hf, df
            if dc > hc:
                dc, hc = hc, dc

            dx = ac * (dc - 1)
            dy = ac * (df - 1)

            if poscelda == 1:
                dx += ac - ancho
            elif poscelda == 2:
                dy += ac - ancho
            elif poscelda == 3:
                dy += ac - ancho
                dx += ac - ancho

            if psize != 100:
                anchon = ancho * psize / 100
                dx += (ancho - anchon) / 2
                ancho = anchon
                alton = alto * psize / 100
                dy += (alto - alton) / 2
                alto = alton

            rect = QtCore.QRectF(dx, dy, ancho, alto)
            self.pixmap.render(painter, rect)

        dl = bm.a1h8[0]
        hl = bm.a1h8[2]
        if dl > hl:
            dl, hl = hl, dl
        dn = bm.a1h8[1]
        hn = bm.a1h8[3]
        if dn > hn:
            dn, hn = hn, dn
        dn0 = dn
        while dl <= hl:
            while dn <= hn:
                haz(dl + dn + dl + dn)
                dn = chr(ord(dn) + 1)
            dl = chr(ord(dl) + 1)
            dn = dn0

        self.rect = QtCore.QRectF(posicion.x, posicion.y, posicion.ancho, posicion.alto)
        if self.siRecuadro:
            pen = QtGui.QPen()
            pen.setColor(QtGui.QColor("blue"))
            pen.setWidth(2)
            pen.setStyle(QtCore.Qt.DashLine)
            painter.setPen(pen)
            painter.drawRect(self.rect)

    def boundingRect(self):
        x = 1
        return QtCore.QRectF(self.rect).adjusted(-x, -x, x * 2, x * 2)
