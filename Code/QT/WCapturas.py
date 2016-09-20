from PyQt4 import QtCore, QtGui

from Code.QT import Colocacion
from Code.QT import QTUtil
from Code import VarGen

class CapturaLista(QtGui.QWidget):
    def __init__(self, wParent, tablero):
        super(CapturaLista, self).__init__(wParent)

        self.setFixedWidth(tablero.ancho / 10)
        anchoPZ = int(tablero.ancho / 12)
        self.pantalla = wParent.parent()

        self.tipoMaterial = VarGen.configuracion.tipoMaterial

        li = self.li = [["P", 8], ["N", 2], ["B", 2], ["R", 2], ["Q", 1]]
        self.dic = {}
        for pieza, numero in li:
            dW = self.dic[pieza] = []
            dB = self.dic[pieza.lower()] = []
            for i in range(numero):
                lbW = tablero.piezas.widget(pieza)
                lbW.setFixedSize(anchoPZ, anchoPZ)
                lbW.hide()
                dW.append(lbW)
                lbB = tablero.piezas.widget(pieza.lower())
                lbB.setFixedSize(anchoPZ, anchoPZ)
                lbB.hide()
                dB.append(lbB)

        self.ponLayout(True)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.RightButton:
            self.pantalla.activaCapturas(False)

    def resetPZ(self, tablero):
        anchoPZ = tablero.anchoPieza
        for k, li in self.dic.iteritems():
            for lb in li:
                lb.setFixedSize(anchoPZ, anchoPZ)

        self.setMinimumWidth(anchoPZ + 4)
        self.adjustSize()

    def ponLayout(self, siBlancasAbajo):
        layout = self.layout()
        if layout:
            while True:
                item = layout.takeAt(0)
                if item:
                    del item
                else:
                    break
        else:
            layout = Colocacion.V().margen(0)
            self.setLayout(layout)

        dlayout = {}
        for color in (True, False):
            ly = dlayout[color] = Colocacion.V().margen(0)
            for pieza, numero in self.li:
                if not color:
                    pieza = pieza.lower()
                for i in range(numero):
                    ly.control(self.dic[pieza][i])
        ly0, ly1 = dlayout[siBlancasAbajo], dlayout[not siBlancasAbajo]

        layout.otro(ly0).relleno().otro(ly1)
        QTUtil.refreshGUI()

    def ponLI(self, liPiezas):
        piezas = "PNBRQpnbrq"
        for pieza, num in liPiezas:
            piezas = piezas.replace(pieza, "")
            d = self.dic[pieza]
            for x in range(num):
                d[x].show()
            for x in range(num, len(d)):
                d[x].hide()
        for pieza in piezas:
            d = self.dic[pieza]
            for x in range(len(d)):
                d[x].hide()

    def pon(self, dicCapturas, jg, apertura):
        if dicCapturas:
            cPiezas = "PNBRQ"

            liDif = []

            tipo = self.tipoMaterial

            if tipo == "C":
                tt = 0
                for pieza in cPiezas:
                    tt += dicCapturas[pieza]
                    tt += dicCapturas[pieza.lower()]
                    if tt > 8:
                        tipo = "D"
                        break

            if tipo == "D":
                for pieza in cPiezas:
                    vW = dicCapturas[pieza]
                    vB = dicCapturas[pieza.lower()]
                    vD = vW - vB
                    if vD != 0:
                        if vD < 0:
                            pieza = pieza.lower()
                            vD = -vD
                        liDif.append((pieza, vD))
            else:
                for pieza in cPiezas:
                    vW = dicCapturas[pieza]
                    vB = dicCapturas[pieza.lower()]
                    if vW:
                        liDif.append((pieza, vW))
                    if vB:
                        liDif.append((pieza.lower(), vB))

            self.ponLI(liDif)
