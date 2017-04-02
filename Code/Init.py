DEBUG = False
VERSION = "10.12a"

import os
import sys

from Code.Constantes import *

if DEBUG:
    prlkn("DEBUG " * 20)

from Code import VarGen
from Code import Util
from Code import Procesador
from Code import Sonido

from Code.QT import Gui


def init():
    if not DEBUG:
        sys.stderr = Util.Log("bug.log")

    mainProcesador = Procesador.Procesador()
    mainProcesador.setVersion(VERSION)
    runSound = Sonido.RunSound()
    resp = Gui.lanzaGUI(mainProcesador)
    runSound.close()
    mainProcesador.pararMotores()
    mainProcesador.quitaKibitzers()

    if resp == kFinReinicio:
        if sys.argv[0].endswith(".py"):
            exe = os.path.abspath(sys.argv[0])
        else:
            exe = "Lucas.exe"
        VarGen.startfile(exe)
