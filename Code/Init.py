DEBUG = False
VERSION = "9.06"

import sys

from Code.Constantes import *

if DEBUG:
    prlkn("DEBUG " * 20)

import Code.VarGen as VarGen
import Code.Procesador as Procesador
import Code.Sonido as Sonido

import Code.QT.Gui as Gui

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
            exe = "./%s"%sys.argv[0]
        else:
            exe = "Lucas.exe" if VarGen.isWindows else "Lucas"
        VarGen.startfile(exe)

