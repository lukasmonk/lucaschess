import collections
import time

from PyQt4 import QtCore

from Code.QT import QTUtil
from Code import Tareas


class CPU:
    def __init__(self, pantalla):
        self.pantalla = pantalla
        self.tablero = pantalla.tablero
        self.junks = 1000.0 / 33.0
        self.reset()

    def reset(self):
        self.ultimaID = 0
        self.timer = None
        self.dicTareas = collections.OrderedDict()

    def nuevaID(self):
        self.ultimaID += 1
        return self.ultimaID

    def masTarea(self, tarea, padre, siExclusiva):
        tid = tarea.id
        self.dicTareas[tid] = tarea
        tarea.padre = padre
        tarea.siExclusiva = siExclusiva
        return tid

    def duerme(self, segundos, padre=0, siExclusiva=False):
        tarea = Tareas.TareaDuerme(segundos)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, siExclusiva)

    def toolTip(self, texto, padre=0, siExclusiva=False):
        tarea = Tareas.TareaToolTip(texto)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, siExclusiva)

    def muevePieza(self, desdeA1H8, hastaA1H8, segundos=1.0, padre=0, siExclusiva=False):
        tarea = Tareas.TareaMuevePieza(desdeA1H8, hastaA1H8, segundos)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, siExclusiva)

    def borraPieza(self, a1h8, padre=0, siExclusiva=False, tipo=None):
        tarea = Tareas.TareaBorraPieza(a1h8, tipo)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, siExclusiva)

    def cambiaPieza(self, a1h8, pieza, padre=0, siExclusiva=False):
        tarea = Tareas.TareaCambiaPieza(a1h8, pieza)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, siExclusiva)

    def muevePiezaLI(self, lista, segundos=1.0, padre=0, siExclusiva=False):
        tarea = Tareas.TareaMuevePiezaLI(lista, segundos)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, siExclusiva)

    def ponPosicion(self, posicion, padre=0):
        tarea = Tareas.TareaPonPosicion(posicion)
        tarea.enlaza(self)
        return self.masTarea(tarea, padre, True)

    def start(self):
        if self.timer:
            self.timer.stop()
            del self.timer
        self.timer = QtCore.QTimer(self.pantalla)
        self.pantalla.connect(self.timer, QtCore.SIGNAL("timeout()"), self.run)
        self.timer.start(self.junks)

    def stop(self):
        if self.timer:
            self.timer.stop()
            del self.timer
            self.timer = None
        self.reset()

    def runLineal(self):
        self.start()

        while self.dicTareas:
            time.sleep(0.01)
            QTUtil.refreshGUI()

    def run(self):
        li = self.dicTareas.keys()
        # li.sort()
        nPasos = 0
        for tid in li:
            tarea = self.dicTareas[tid]

            if tarea.padre and tarea.padre in self.dicTareas:
                continue  # no ha terminado

            siExclusiva = tarea.siExclusiva
            if siExclusiva:
                if nPasos:
                    continue  # Hay que esperar a que terminen todos los anteriores

            siUltimo = tarea.unPaso()
            nPasos += 1
            if siUltimo:
                del self.dicTareas[tid]

            if siExclusiva:
                break

        if len(self.dicTareas) == 0:
            self.stop()
