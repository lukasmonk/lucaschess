DIVISOR = 12

from PyQt4 import QtCore, QtGui

from Code.QT import Colocacion
from Code.QT import QTUtil
from Code import VarGen


class CapturaLista(QtGui.QWidget):
    def __init__(self, wParent, tablero):
        super(CapturaLista, self).__init__(wParent)

        anchoPZ = int(tablero.ancho/DIVISOR)
        self.setFixedWidth(tablero.anchoCasilla+2)
        self.pantalla = wParent.parent()

        self.tipoMaterial = VarGen.configuracion.tipoMaterial

        li = self.li = [["P", 8], ["N", 2], ["B", 2], ["R", 2], ["Q", 1]]
        self.dic = {}
        for pieza, numero in li:
            dW = self.dic[pieza] = []
            dB = self.dic[pieza.lower()] = []
            for i in range(numero):
                lbW = tablero.piezas.label(self, pieza, anchoPZ)
                lbW.hide()
                lbW.alinCentrado()
                dW.append(lbW)
                lbB = tablero.piezas.label(self, pieza.lower(), anchoPZ)
                lbB.hide()
                lbB.alinCentrado()
                dB.append(lbB)

        self.ponLayout(True)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.RightButton:
            self.pantalla.activaCapturas(False)

    def resetPZ(self, tablero):
        anchoPZ = int(tablero.ancho/DIVISOR)
        for k, li in self.dic.iteritems():
            for lb in li:
                tablero.piezas.change_label(lb, anchoPZ)
        # self.setMinimumWidth(anchoPZ + 4)
        # self.adjustSize()

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
            ly = dlayout[color] = Colocacion.V().margen(4)
            for pieza, numero in self.li:
                if self.tipoMaterial == "M":
                    if color:
                        pieza = pieza.lower()
                else:
                    if not color:
                        pieza = pieza.lower()
                for i in range(numero):
                    ly.controlc(self.dic[pieza][i])
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

    def pon(self, dicCapturas):
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
            elif tipo == "M":
                for pieza in cPiezas:
                    vW = dicCapturas[pieza]
                    vB = dicCapturas[pieza.lower()]
                    vD = vW - vB
                    if vD != 0:
                        if vD < 0:
                            vD = -vD
                        else:
                            pieza = pieza.lower()
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
        else:
            self.ponLI([])
