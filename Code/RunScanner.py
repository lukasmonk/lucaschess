import os
import sys

from PyQt4 import QtCore, QtGui

from Code.QT import QTUtil
from Code import Util

class Scanner_vars:
    def __init__(self, folder_scanners):
        self.fich_vars = os.path.join(folder_scanners, "last.data")
        self.read()

    def read(self):
        dic = Util.iniBase8dic(self.fich_vars)
        self.opacity = float(dic.get("OPACITY", 0.3))
        self.last_width = int(dic.get("LAST_WIDTH", 0))
        self.tolerance = int(dic.get("TOLERANCE", 6))
        self.scanner = dic.get("SCANNER", "")
        self.ask = dic.get("ASK", "True") == "True"

    def write(self):
        dic = {
            "OPACITY": self.opacity,
            "LAST_WIDTH": self.last_width,
            "TOLERANCE": self.tolerance,
            "SCANNER": self.scanner,
            "ASK": self.ask
        }
        Util.dic8iniBase(self.fich_vars, dic)

class Scanner(QtGui.QWidget):
    def __init__(self, cpu):
        QtGui.QWidget.__init__(self)

        self.cpu = cpu

        self.vars = Scanner_vars(cpu.folder_scanners)

        self.setWindowFlags(QtCore.Qt.FramelessWindowHint)
        self.setWindowOpacity(self.vars.opacity)
        self.setGeometry(QtGui.QDesktopWidget().availableGeometry())

        self.path = None
        self.selecting = False
        self.selected = False
        self.x = self.y = self.width = 0

    def quit(self, ok):
        if ok:
            self.vars.write()
            rect = QtCore.QRect(self.x, self.y, self.width, self.width)
        else:
            rect = None
        self.cpu.end(rect)

    def paintEvent(self, event):
        if self.path:
            painter = QtGui.QPainter(self)
            painter.setPen(QtGui.QPen(QtCore.Qt.red))
            painter.drawPath(self.path)

    def setPath(self, point):
        width = point.x() - self.x
        height = point.y() - self.y
        width = max(width, height)
        if width > 0:
            self.width = width
            self.setPathW()

    def setPathW(self):
        celda = self.width / 8
        self.width = celda * 8
        rect = QtGui.QPainterPath()
        x = self.x
        y = self.y
        width = self.width
        for c in range(8):
            dx = x + c * celda
            rect.moveTo(dx, y)
            rect.lineTo(dx + celda, y)
            rect.lineTo(dx + celda, y + width)
            rect.lineTo(dx, y + width)
            rect.lineTo(dx, y)
        for f in range(1, 8):
            dy = y + f * celda
            rect.moveTo(x, dy)
            rect.lineTo(x + width, dy)
        rect.closeSubpath()

        self.path = rect
        self.update()

    def mouseMoveEvent(self, eventMouse):
        if self.selecting:
            self.setPath(eventMouse.pos())

    def mousePressEvent(self, eventMouse):
        if eventMouse.button() == QtCore.Qt.LeftButton:
            self.selecting = True
            self.selected = False
            origin = eventMouse.pos()
            self.x = origin.x()
            self.y = origin.y()
            self.width = 0
        elif eventMouse.button() == QtCore.Qt.RightButton:
            self.quit(True)
            self.close()
        eventMouse.ignore()

    def mouseReleaseEvent(self, eventMouse):
        self.selecting = False
        self.selected = True
        if self.width < 10:
            if self.vars.last_width > 10:
                self.width = self.vars.last_width
                self.setPathW()
        else:
            self.vars.last_width = self.width

    def keyPressEvent(self, event):
        k = event.key()
        m = int(event.modifiers())
        siCtrl = (m & QtCore.Qt.ControlModifier) > 0
        x = self.x
        y = self.y
        width = self.width

        if k in (QtCore.Qt.Key_Return, QtCore.Qt.Key_Enter, QtCore.Qt.Key_S):
            self.quit(True)

        elif k == QtCore.Qt.Key_Escape:
            self.quit(False)

        elif k == QtCore.Qt.Key_Plus:
            self.vars.opacity += 0.05
            if self.vars.opacity > 0.5:
                self.vars.opacity = 0.5
            self.setWindowOpacity(self.vars.opacity)

        elif k == QtCore.Qt.Key_Minus:
            self.vars.opacity -= 0.05
            if self.vars.opacity < 0.1:
                self.vars.opacity = 0.1
            self.setWindowOpacity(self.vars.opacity)

        else:
            if k == QtCore.Qt.Key_Right:
                if siCtrl:
                    width += 8
                else:
                    x += 1
            elif k == QtCore.Qt.Key_Left:
                if siCtrl:
                    width -= 8
                else:
                    x -= 1
            elif k == QtCore.Qt.Key_Up:
                if siCtrl:
                    width += 8
                    y -= 8
                else:
                    y -= 1
            elif k == QtCore.Qt.Key_Down:
                if siCtrl:
                    width -= 8
                    y += 8
                else:
                    y += 1

            if self.selected:
                self.x = x
                self.y = y
                self.width = width
                self.setPathW()

        event.ignore()

class CPU:
    def __init__(self, fich_png, folder_scanners):
        self.fich_png = fich_png
        self.folder_scanners = folder_scanners

    def end(self, rect):
        self.scanner.close()
        if rect:
            desktop = QtGui.QPixmap.grabWindow(QtGui.QApplication.desktop().winId(), 0, 0,
                                               QTUtil.anchoEscritorio(), QTUtil.altoEscritorio())
            selected_pixmap = desktop.copy(rect)
            selected_pixmap = selected_pixmap.scaled(256, 256, transformMode=QtCore.Qt.SmoothTransformation)
            selected_pixmap.save(self.fich_png)
        else:
            with open(self.fich_png, "wb") as q:
                q.write("")

    def run(self):
        app = QtGui.QApplication(sys.argv)
        self.scanner = Scanner(self)
        self.scanner.show()
        app.exec_()

def run(fdb, folder_scanners):
    ferr = open("./bug.scanner", "at")
    sys.stderr = ferr

    cpu = CPU(fdb, folder_scanners)
    cpu.run()

    ferr.close()
