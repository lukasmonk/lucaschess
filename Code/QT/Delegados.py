"""
Rutinas basicas para la edicion en las listas de registros.
"""

import os

from PyQt4 import QtCore, QtGui, QtSvg

from Code.QT import Iconos

dicPM = {}
dicPZ = {}
dicNG = {}


def generaPM(piezas):
    dicPM["V"] = Iconos.pmComentario()
    dicPM["R"] = Iconos.pmApertura()
    dicPM["M"] = Iconos.pmComentarioMas()
    dicPM["S"] = Iconos.pmAperturaComentario()
    for k in "KQRNBkqrnb":
        dicPZ[k] = piezas.render(k)

    carpNAGs = "./IntFiles/NAGs"
    for f in os.listdir(carpNAGs):
        if f.endswith(".svg") and f.startswith("$") and len(f) > 5:
            nag = f[1:-4]
            if nag.isdigit():
                fsvg = carpNAGs + "/" + f
                dicNG[nag] = QtSvg.QSvgRenderer(fsvg)


class ComboBox(QtGui.QItemDelegate):
    def __init__(self, liTextos):
        QtGui.QItemDelegate.__init__(self)
        self.liTextos = liTextos

    def createEditor(self, parent, option, index):
        editor = QtGui.QComboBox(parent)
        editor.addItems(self.liTextos)
        editor.installEventFilter(self)
        return editor

    def setEditorData(self, cb, index):
        value = index.model().data(index, QtCore.Qt.DisplayRole)
        num = self.liTextos.index(value)
        cb.setCurrentIndex(num)

    def setModelData(self, cb, model, index):
        num = cb.currentIndex()
        model.setData(index, self.liTextos[num])

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)


class LineaTexto(QtGui.QItemDelegate):
    def __init__(self, parent=None, siPassword=False, siEntero=False):
        QtGui.QItemDelegate.__init__(self, parent)
        self.siPassword = siPassword
        self.siEntero = siEntero

    def createEditor(self, parent, option, index):
        editor = QtGui.QLineEdit(parent)
        if self.siPassword:
            editor.setEchoMode(QtGui.QLineEdit.Password)
        if self.siEntero:
            editor.setValidator(QtGui.QIntValidator(self))
            editor.setAlignment(QtCore.Qt.AlignRight)
        editor.installEventFilter(self)
        return editor

    def setEditorData(self, sle, index):
        value = index.model().data(index, QtCore.Qt.DisplayRole)
        sle.setText(value)

    def setModelData(self, sle, model, index):
        value = str(sle.text())
        model.setData(index, value)

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)


class LineaTextoUTF8(QtGui.QItemDelegate):
    def __init__(self, parent=None, siPassword=False):
        QtGui.QItemDelegate.__init__(self, parent)
        self.siPassword = siPassword

    def createEditor(self, parent, option, index):
        editor = QtGui.QLineEdit(parent)
        if self.siPassword:
            editor.setEchoMode(QtGui.QLineEdit.Password)
        editor.installEventFilter(self)
        return editor

    def setEditorData(self, sle, index):
        value = index.model().data(index, QtCore.Qt.DisplayRole)
        sle.setText(value)

    def setModelData(self, sle, model, index):
        value = sle.text()
        model.setData(index, value)

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)


class EtiquetaPGN(QtGui.QStyledItemDelegate):
    def __init__(self, siBlancas, siAlineacion=False, siFondo=False):
        self.siBlancas = siBlancas  # None = no hacer
        self.siFigurinesPGN = siBlancas is not None
        self.siAlineacion = siAlineacion
        self.siFondo = siFondo
        QtGui.QStyledItemDelegate.__init__(self, None)

    def setWhite(self, isWhite):
        self.siBlancas = isWhite

    def rehazPosicion(self):
        posicion = self.bloquePieza.posicion
        self.setPos(posicion.x, posicion.y)

    def paint(self, painter, option, index):
        data = index.model().data(index, QtCore.Qt.DisplayRole)
        if type(data) == tuple:
            pgn, color, info, indicadorInicial, liNAGs = data
            if liNAGs:
                li = []
                for x in liNAGs:
                    if x in dicNG:
                        li.append(dicNG[x])
                liNAGs = li
        else:
            pgn, color, info, indicadorInicial, liNAGs = data, None, None, None, None

        iniPZ = None
        finPZ = None
        if self.siFigurinesPGN and pgn:
            if pgn[0] in "QBKRN":
                iniPZ = pgn[0] if self.siBlancas else pgn[0].lower()
                pgn = pgn[1:]
            elif pgn[-1] in "QBRN":
                finPZ = pgn[-1] if self.siBlancas else pgn[-1].lower()
                pgn = pgn[:-1]

        if info and not finPZ:
            pgn += info
            info = None

        rect = option.rect
        wTotal = rect.width()
        hTotal = rect.height()
        xTotal = rect.x()
        yTotal = rect.y()

        if option.state & QtGui.QStyle.State_Selected:
            painter.fillRect(rect, QtGui.QColor("#678DB2"))  # sino no se ve en CDE-Motif-Windows
            # painter.fillRect(option.rect, palette.highlight().color())
        elif self.siFondo:
            fondo = index.model().getFondo(index)
            if fondo:
                painter.fillRect(rect, fondo)

        if indicadorInicial:
            painter.save()
            painter.translate(xTotal, yTotal)
            painter.drawPixmap(0, 0, dicPM[indicadorInicial])
            painter.restore()

        documentPGN = QtGui.QTextDocument()
        documentPGN.setDefaultFont(option.font)
        if color:
            pgn = '<font color="%s"><b>%s</b></font>' % (color, pgn)
        documentPGN.setHtml(pgn)
        wPGN = documentPGN.idealWidth()
        hPGN = documentPGN.size().height()
        hx = hPGN * 80 / 100
        wpz = int(hx * 0.8)

        if info:
            documentINFO = QtGui.QTextDocument()
            documentINFO.setDefaultFont(option.font)
            if color:
                info = '<font color="%s"><b>%s</b></font>' % (color, info)
            documentINFO.setHtml(info)
            wINFO = documentINFO.idealWidth()

        ancho = wPGN
        if iniPZ:
            ancho += wpz
        if finPZ:
            ancho += wpz
        if info:
            ancho += wINFO
        if liNAGs:
            ancho += wpz * len(liNAGs)

        x = xTotal + (wTotal - ancho) / 2
        if self.siAlineacion:
            alineacion = index.model().getAlineacion(index)
            if alineacion == "i":
                x = xTotal + 3
            elif alineacion == "d":
                x = xTotal + (wTotal - ancho - 3)

        y = yTotal + (hTotal - hPGN * 0.9) / 2

        if iniPZ:
            painter.save()
            painter.translate(x, y)
            pm = dicPZ[iniPZ]
            pmRect = QtCore.QRectF(0, 0, hx, hx)
            pm.render(painter, pmRect)
            painter.restore()
            x += wpz

        painter.save()
        painter.translate(x, y)
        documentPGN.drawContents(painter)
        painter.restore()
        x += wPGN

        if finPZ:
            painter.save()
            painter.translate(x - 0.3 * wpz, y)
            pm = dicPZ[finPZ]
            pmRect = QtCore.QRectF(0, 0, hx, hx)
            pm.render(painter, pmRect)
            painter.restore()
            x += wpz

        if info:
            painter.save()
            painter.translate(x, y)
            documentINFO.drawContents(painter)
            painter.restore()
            x += wINFO

        if liNAGs:
            for rndr in liNAGs:
                painter.save()
                painter.translate(x - 0.2 * wpz, y)
                pmRect = QtCore.QRectF(0, 0, hx, hx)
                rndr.render(painter, pmRect)
                painter.restore()
                x += wpz


class PmIconosBMT(QtGui.QStyledItemDelegate):
    """
    Delegado para la muestra con html
    """

    def __init__(self, parent=None, dicIconos=None):
        QtGui.QStyledItemDelegate.__init__(self, parent)

        if dicIconos:
            self.dicIconos = dicIconos
        else:
            self.dicIconos = {"0": Iconos.pmPuntoBlanco(),
                              "1": Iconos.pmPuntoNegro(), "2": Iconos.pmPuntoAmarillo(),
                              "3": Iconos.pmPuntoNaranja(), "4": Iconos.pmPuntoVerde(),
                              "5": Iconos.pmPuntoAzul(), "6": Iconos.pmPuntoMagenta(),
                              "7": Iconos.pmPuntoRojo(), "8": Iconos.pmPuntoEstrella()}

    def paint(self, painter, option, index):
        pos = str(index.model().data(index, QtCore.Qt.DisplayRole))
        if pos not in self.dicIconos:
            return
        painter.save()
        painter.translate(option.rect.x(), option.rect.y())
        painter.drawPixmap(4, 1, self.dicIconos[pos])
        painter.restore()


class PmIconosColor(QtGui.QStyledItemDelegate):
    """ Usado en TurnOnLigths"""
    def __init__(self, parent=None):
        QtGui.QStyledItemDelegate.__init__(self, parent)

        self.dicIconos = {"0": Iconos.pmGris32(),
                          "1": Iconos.pmAmarillo32(),
                          "2": Iconos.pmNaranja32(),
                          "3": Iconos.pmVerde32(),
                          "4": Iconos.pmAzul32(),
                          "5": Iconos.pmMagenta32(),
                          "6": Iconos.pmRojo32(),
                          "7": Iconos.pmLight32()
                          }

    def paint(self, painter, option, index):
        pos = str(index.model().data(index, QtCore.Qt.DisplayRole))
        if pos not in self.dicIconos:
            return
        painter.save()
        painter.translate(option.rect.x(), option.rect.y())
        painter.drawPixmap(4, 4, self.dicIconos[pos])
        painter.restore()
