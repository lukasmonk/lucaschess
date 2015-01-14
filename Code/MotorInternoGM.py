# -*- coding: latin-1 -*-
import time
import random

from PyQt4 import QtCore

from Code.Constantes import *
import Code.XMotorRespuesta as XMotorRespuesta
import Code.MotorInterno as MotorInterno
import Code.QT.Iconos as Iconos

class GestorMotor:
    def __init__(self, clave, gestor):

        self.procesador = gestor.procesador
        self.xtutor = gestor.xtutor
        self.clave = clave
        self.autor = ""
        self.version = ""
        self.siUCI = False
        self.siTutor = False
        self.siDebug = False
        self.exe = ""
        self.motorProfundidad = None
        self.motorTiempoJugada = None

        self.ml = MotorInterno.MotorInterno()
        dic = {kMP_1: ( self.ml.mp1, _("Monkey"), 7, Iconos.pmMonkey() ),
               kMP_2: ( self.ml.mp2, _("Donkey"), 6, Iconos.pmDonkey() ),
               kMP_3: ( self.ml.mp3, _("Bull"), 5, Iconos.pmBull() ),
               kMP_4: ( self.ml.mp4, _("Wolf"), 4, Iconos.pmWolf() ),
               kMP_5: ( self.ml.mp5, _("Lion"), 3, Iconos.pmLion() ),
               kMP_6: ( self.ml.mp6, _("Rat"), 2, Iconos.pmRat() ),
               kMP_7: ( self.ml.mp7, _("Snake"), 0, Iconos.pmSnake() ),
        }

        self.funcion, txt, self.jugadasTutor, self.imagen = dic[clave]
        self.nombre = txt
        self.numJugada = 0

        self.siListo = clave == kMP_7
        if self.siListo:
            self.siJuegaTutor = False
            self.rmComparar = XMotorRespuesta.RespuestaMotor("listo", True)
            self.rmComparar.mate = 0
            self.rmComparar.puntos = -80

    def debug(self, txt):
        self.siDebug = True
        self.nomDebug = self.nombre + "-" + txt

    def juega(self):
        fen = self.procesador.gestor.partida.fenUltimo()

        if self.jugadasTutor == 0:
            siTutor = self.siJuegaTutor
            mrm = self.xtutor.control(fen, 3)
            rm = mrm.liMultiPV[0]
            self.siJuegaTutor = self.rmComparar.siMejorQue(rm, 0, 0)
        else:
            self.numJugada += 1
            siTutor = self.numJugada % self.jugadasTutor == 0

        if siTutor:
            mrm = self.xtutor.control(fen, 3)

        else:
            pv = self.funcion(fen)
            mrm = XMotorRespuesta.MRespuestaMotor("interno", "w" in fen)
            mrm.dispatch("bestmove " + pv, None, None)

        return mrm.liMultiPV[0]

    def juegaTiempo(self, tiempoBlancas, tiempoNegras, segundosJugada, nAjustado=0):
        t = tiempoBlancas if self.procesador.gestor.partida.siBlancas() else tiempoNegras
        mx = int(t * 5 / 300)
        li = [1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4]
        if mx > 0:
            for x in range(mx):
                li.extend([x + 1] * (mx - x))
        t = random.choice(li)
        for x in range(t * 2):
            QtCore.QCoreApplication.processEvents(QtCore.QEventLoop.ExcludeUserInputEvents)
            time.sleep(0.5)
        return self.juega()

