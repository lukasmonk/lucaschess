import copy

from PyQt4 import QtCore, QtGui

from Code.QT import TabBloques


class FlechaSC(TabBloques.BloqueEspSC):
    def __init__(self, escena, bloqueFlecha, rutinaPulsada=None):
        super(FlechaSC, self).__init__(escena, bloqueFlecha)

        self.rutinaPulsada = rutinaPulsada
        self.rutinaPulsadaCarga = None

        self.poligonoSizeTop = None
        self.poligonoSizeBottom = None
        self.poligonoMove = None

        self.posicion2xy()

    def ponRutinaPulsada(self, rutina, carga):
        self.rutinaPulsada = rutina
        self.rutinaPulsadaCarga = carga

    def ponA1H8(self, a1h8):
        self.bloqueDatos.a1h8 = a1h8
        self.posicion2xy()

    def reset(self):
        self.posicion2xy()
        bf = self.bloqueDatos
        self.setOpacity(bf.opacidad)
        self.setZValue(bf.posicion.orden)
        self.update()

    def posicion2xy(self):
        bf = self.bloqueDatos
        posicion = bf.posicion
        ac = bf.anchoCasilla

        df, dc, hf, hc = self.tablero.a1h8_fc(bf.a1h8)

        # siempre sale del centro
        dx = posicion.x = dc * ac - ac / 2
        dy = posicion.y = df * ac - ac / 2

        if bf.destino == "c":
            hx = hc * ac - ac / 2
            hy = hf * ac - ac / 2
        elif bf.destino == "m":  # minimo
            min_v = 99999999999
            min_hx = min_hy = 0
            for x in (3, 2, 1):  # 3/4 = izquierda 1/2 y 1/4 izquierda
                for y in (3, 2, 1):  # 3/4 = arriba 1/2 y 1/4
                    hx = hc * ac - ac * x / 4
                    hy = hf * ac - ac * y / 4
                    v = (hx - dx) ** 2 + (hy - dy) ** 2
                    if v < min_v:
                        min_hx = hx
                        min_hy = hy
                        min_v = v
            hx = min_hx
            hy = min_hy

        posicion.ancho = hx
        posicion.alto = hy

    def xy2posicion(self):

        bf = self.bloqueDatos
        posicion = bf.posicion
        ac = bf.anchoCasilla

        f = lambda xy: int(round((float(xy) + ac / 2.0) / float(ac), 0))

        dc = f(posicion.x)
        df = f(posicion.y)
        hc = f(posicion.ancho)
        hf = f(posicion.alto)

        bien = lambda fc: (fc < 9) and (fc > 0)
        if bien(dc) and bien(df) and bien(hc) and bien(hf):
            if dc != hc or df != hf:
                bf.a1h8 = self.tablero.fc_a1h8(df, dc, hf, hc)

        self.posicion2xy()

    def contiene(self, p):
        p = self.mapFromScene(p)
        for x in (self.poligonoSizeTop, self.poligonoSizeBottom, self.poligonoMove):
            if x:
                if x.containsPoint(p, QtCore.Qt.OddEvenFill):
                    return True
        return False

    def nombre(self):
        return _("Arrow")

    def mousePressEvent(self, event):
        QtGui.QGraphicsItem.mousePressEvent(self, event)
        if self.poligonoSizeTop:
            self.siSizeTop = self.poligonoSizeTop.containsPoint(event.pos(), QtCore.Qt.OddEvenFill)
            self.siSizeBottom = self.poligonoSizeBottom.containsPoint(event.pos(), QtCore.Qt.OddEvenFill)
            self.siMove = self.poligonoMove.containsPoint(event.pos(), QtCore.Qt.OddEvenFill)

        p = event.scenePos()
        self.expX = p.x()
        self.expY = p.y()

    def mousePressExt(self, event):
        p = event.pos()
        p = self.mapFromScene(p)
        if self.poligonoSizeTop:
            self.siSizeTop = self.poligonoSizeTop.containsPoint(p, QtCore.Qt.OddEvenFill)
            self.siSizeBottom = self.poligonoSizeBottom.containsPoint(p, QtCore.Qt.OddEvenFill)
            self.siMove = self.poligonoMove.containsPoint(p, QtCore.Qt.OddEvenFill)

        self.expX = p.x()
        self.expY = p.y()

    def mouseMoveEvent(self, event):
        event.ignore()
        if not (self.siMove or self.siSizeTop or self.siSizeBottom):
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
            posicion.ancho += dx
            posicion.alto += dy
        elif self.siSizeTop:
            posicion.ancho += dx
            posicion.alto += dy
        elif self.siSizeBottom:
            posicion.x += dx
            posicion.y += dy

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
            if self.siMove or self.siSizeTop or self.siSizeBottom:
                self.xy2posicion()
                self.escena.update()
                self.siMove = self.siSizeTop = self.siSizeBottom = False
            self.activa(False)

        if self.rutinaPulsada:
            if self.rutinaPulsadaCarga:
                self.rutinaPulsada(self.rutinaPulsadaCarga)
            else:
                self.rutinaPulsada()

    def mouseReleaseExt(self):
        self.xy2posicion()
        self.escena.update()
        self.siMove = self.siSizeTop = self.siSizeBottom = False
        self.activa(False)

    def pixmap(self):
        bf = self.bloqueDatos

        a1h8 = bf.a1h8
        destino = bf.destino
        anchoCasilla = bf.anchoCasilla

        bf.anchoCasilla = 8
        bf.destino = "c"
        self.ponA1H8("a8d5")

        pm = QtGui.QPixmap(self.rect.width(), self.rect.height())
        pm.fill(QtCore.Qt.transparent)

        painter = QtGui.QPainter()
        painter.begin(pm)
        painter.setRenderHint(painter.Antialiasing, True)
        painter.setRenderHint(painter.SmoothPixmapTransform, True)
        self.paint(painter, None, None)
        painter.end()

        pm1 = pm.copy(0, 0, 32, 32)

        bf.destino = destino
        bf.anchoCasilla = anchoCasilla
        self.ponA1H8(a1h8)

        return pm1

    def paint(self, painter, option, widget):

        bf = self.bloqueDatos

        resp = paintArrow(painter, bf)
        if resp:
            self.poligonoSizeBottom, self.poligonoMove, self.poligonoSizeTop = resp


def paintArrow(painter, bf):
    posicion = bf.posicion
    dx = posicion.x
    dy = posicion.y
    hx = posicion.ancho
    hy = posicion.alto

    p_ini = QtCore.QPointF(dx, dy)
    p_fin = QtCore.QPointF(hx, hy)
    linea = QtCore.QLineF(p_ini, p_fin)
    tamLinea = linea.length()
    if linea.isNull():
        return None

    color = QtGui.QColor(bf.color)
    pen = QtGui.QPen()
    pen.setWidth(bf.grosor)
    pen.setColor(color)
    pen.setStyle(bf.tipoqt())
    if bf.redondeos:
        pen.setCapStyle(QtCore.Qt.RoundCap)
        pen.setJoinStyle(QtCore.Qt.RoundJoin)
    painter.setPen(pen)

    xk = bf.anchoCasilla / 32.0

    ancho = float(bf.ancho) * xk
    vuelo = float(bf.vuelo) * xk

    altoCab = float(bf.altocabeza) * xk
    if tamLinea * 0.65 < altoCab:
        nv = tamLinea * 0.65
        prc = nv / altoCab
        altoCab = nv
        ancho *= prc
        vuelo *= prc

    xp = 1.0 - float(altoCab) / tamLinea
    pbc = linea.pointAt(xp)  # base de la cabeza

    # Usamos una linea a 90 grados para calcular los puntos del final de la cabeza de flecha
    l90 = linea.normalVector()
    l90.setLength(ancho + vuelo * 2)
    l90.translate(pbc - p_ini)  # la llevamos a la base de la cabeza
    p_ala1 = l90.pointAt(0.5)  # final del ala de un lado
    l90.translate(p_ala1 - l90.p2())  # La colocamos que empiece en ala1
    p_ala2 = l90.p1()  # final del ala de un lado

    xp = 1.0 - float(altoCab - bf.descuelgue) / tamLinea
    p_basecab = linea.pointAt(xp)  # Punto teniendo en cuenta el angulo en la base de la cabeza, valido para tipo c y p

    # Puntos de la base, se calculan aunque no se dibujen para determinar el poligono de control
    l90 = linea.normalVector()
    l90.setLength(ancho)
    p_base1 = l90.pointAt(0.5)  # final de la base de un lado
    l90.translate(p_base1 - l90.p2())
    p_base2 = l90.p1()  # final de la base de un lado

    lf = QtCore.QLineF(p_ini, p_basecab)
    lf.translate(p_base1 - p_ini)
    p_cab1 = lf.p2()
    lf.translate(p_base2 - p_base1)
    p_cab2 = lf.p2()

    # Poligonos para determinar si se ha pulsado sobre la flecha
    xancho = max(ancho + vuelo * 2.0, 16.0)
    xl90 = linea.normalVector()
    xl90.setLength(xancho)
    xp_base2 = xl90.pointAt(0.5)
    xl90.translate(xp_base2 - xl90.p2())  # La colocamos que empiece en base1
    xp_base1 = xl90.p1()
    xpbb = linea.pointAt(0.15)  # Siempre un 15% para cambiar de tama_o por el pie
    xl90.translate(xpbb - p_ini)  # la llevamos a la base de la cabeza
    xp_medio1b = xl90.p1()
    xp_medio2b = xl90.p2()
    xl90.translate(p_ini - xpbb)  # la llevamos a la base para poderla trasladar
    xpbc = linea.pointAt(0.85)  # Siempre un 15% para cambiar de tama_o por la cabeza
    xl90.translate(xpbc - p_ini)  # la llevamos a la base de la cabeza
    xp_medio1t = xl90.p1()
    xp_medio2t = xl90.p2()
    xl90.translate(p_fin - xpbc)  # la llevamos al final
    xp_final1 = xl90.p1()
    xp_final2 = xl90.p2()

    poligonoSizeBottom = QtGui.QPolygonF([xp_base1, xp_medio1b, xp_medio2b, xp_base2, xp_base1])
    poligonoMove = QtGui.QPolygonF([xp_medio1b, xp_medio1t, xp_medio2t, xp_medio2b, xp_medio1b])
    poligonoSizeTop = QtGui.QPolygonF([xp_medio1t, xp_final1, xp_final2, xp_medio2t, xp_medio1t])

    forma = bf.forma
    # Abierta, forma normal
    if forma == "a":
        painter.drawLine(linea)

        if altoCab:
            lf = QtCore.QLineF(p_fin, p_ala1)
            painter.drawLine(lf)

            lf = QtCore.QLineF(p_fin, p_ala2)
            painter.drawLine(lf)

    else:
        if bf.colorinterior >= 0:
            color = QtGui.QColor(bf.colorinterior)
            if bf.colorinterior2 >= 0:
                color2 = QtGui.QColor(bf.colorinterior2)
                x, y = p_ini.x(), p_ini.y()
                gradient = QtGui.QLinearGradient(x, y, x, y - tamLinea - altoCab)
                gradient.setColorAt(0.0, color)
                gradient.setColorAt(1.0, color2)
                painter.setBrush(QtGui.QBrush(gradient))
            else:
                painter.setBrush(color)

        # Cabeza cerrada
        if forma == "c":
            lf = QtCore.QLineF(p_ini, p_basecab)
            painter.drawLine(lf)
            painter.drawPolygon(QtGui.QPolygonF([p_fin, p_ala1, p_basecab, p_ala2, p_fin]))

        # Poligonal
        elif forma in "123":

            # tipo 1
            if forma == "1":
                painter.drawPolygon(QtGui.QPolygonF([p_base1, p_cab1, p_ala1, p_fin, p_ala2, p_cab2, p_base2, p_base1]))
            # tipo 2 base = un punto
            elif forma == "2":
                painter.drawPolygon(QtGui.QPolygonF([p_ini, p_cab1, p_ala1, p_fin, p_ala2, p_cab2, p_ini]))
            # tipo 3 base cabeza = un punto
            elif forma == "3":
                painter.drawPolygon(
                        QtGui.QPolygonF([p_base1, p_basecab, p_ala1, p_fin, p_ala2, p_basecab, p_base2, p_base1]))

    return poligonoSizeBottom, poligonoMove, poligonoSizeTop


def pixmapArrow(bf, width, height):
    bf = copy.deepcopy(bf)

    pm = QtGui.QPixmap(width, height)
    pm.fill(QtCore.Qt.transparent)

    painter = QtGui.QPainter()
    painter.begin(pm)
    painter.setRenderHint(painter.Antialiasing, True)
    painter.setRenderHint(painter.SmoothPixmapTransform, True)
    paintArrow(painter, bf)
    painter.end()

    return pm
