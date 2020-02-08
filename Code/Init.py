import os
import sys
from Code import VarGen
from Code import Util
from Code import Procesador
from Code import Sonido
from Code.QT import Gui

from Code.Constantes import *

DEBUG = False
VERSION = "11.17"

if DEBUG:
    prlkn("DEBUG " * 20)


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
            exe = "Lucas.exe" if VarGen.isWindows else "Lucas"
        VarGen.startfile(exe)
