DEBUG = False
VERSION = "10.07.2"

import os
import sys

from Code.Constantes import *

if DEBUG:
    prlkn("DEBUG " * 20)

from Code import VarGen
from Code import Procesador
from Code import Sonido

from Code.QT import Gui

def init():
    # Needed for feedback
    if not DEBUG:
        try:
            ferr = open("bug.log", "at")
            sys.stderr = ferr
            okFerr = True
        except:
            okFerr = False

    mainProcesador = Procesador.Procesador()
    mainProcesador.setVersion(VERSION)
    runSound = Sonido.RunSound()
    resp = Gui.lanzaGUI(mainProcesador)
    runSound.close()
    mainProcesador.pararMotores()
    mainProcesador.quitaKibitzers()

    # Needed for feedback
    if not DEBUG:
        if okFerr:
            ferr.close()

    if resp == kFinReinicio:
        if sys.argv[0].endswith(".py"):
            exe = os.path.abspath(sys.argv[0])
        else:
            exe = "Lucas.exe" if VarGen.isWindows else "Lucas"
        VarGen.startfile(exe)
