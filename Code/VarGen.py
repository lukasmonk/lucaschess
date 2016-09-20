import os
import sys

isLinux = sys.platform == "linux2"
if isLinux:
    isWine = os.path.isfile("/usr/bin/wine")
    startfile = os.system
else:
    startfile = os.startfile
isWindows = not isLinux

dgt = None
dgtDispatch = None

configuracion = None  # Actualizado en Configuracion tras lee()

todasPiezas = None

tbook = "Openings/GMopenings.bin"
tbookPTZ = "Openings/fics15.bin"
tbookI = "Openings/irina.bin"
xtutor = None

XSEP = chr(183)
