from PyQt4 import QtCore, QtGui


class BloqueEspSC(QtGui.QGraphicsItem):
    def __init__(self, escena, bloqueDatos):

        self.nAlrededor = 5

        super(BloqueEspSC, self).__init__()

        self.bloqueDatos = bloqueDatos

        self.tablero = escena.parent()

        p = self.tablero.baseCasillasSC.bloqueDatos.posicion
        margen = p.x
        self.setPos(margen, margen)

        # self.rect = QtCore.QRectF( p.x, p.y, p.ancho, p.alto )
        self.rect = QtCore.QRectF(0, 0, p.ancho, p.alto)
        self.angulo = bloqueDatos.posicion.angulo
        if self.angulo:
            self.rotate(self.angulo)

        escena.clearSelection()
        escena.addItem(self)
        self.escena = escena

        if bloqueDatos.siMovible:
            self.tablero.registraMovible(self)

        self.setZValue(bloqueDatos.posicion.orden)
        self.setOpacity(bloqueDatos.opacidad)

        self.activa(False)

    def activa(self, siActivar):
        self.siActivo = siActivar
        if siActivar:
            self.setSelected(True)
            self.siElegido = False
            self.setFlag(QtGui.QGraphicsItem.ItemIsSelectable, True)
            self.setFlag(QtGui.QGraphicsItem.ItemIsFocusable, True)
            self.setFocus()
        else:
            self.setFlag(QtGui.QGraphicsItem.ItemIsSelectable, False)
            self.setFlag(QtGui.QGraphicsItem.ItemIsFocusable, False)

    def tipo(self):
        return self.__class__.__name__[6:-2]

    def boundingRect(self):
        return self.rect
