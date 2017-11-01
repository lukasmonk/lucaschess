import os
import sys
import platform

isLinux = sys.platform == "linux2"
is64bits = platform.architecture()[0] == "64bit"
isLinux64 = isLinux and is64bits
isLinux32 = isLinux and not is64bits
if isLinux:
    isWine = os.path.isfile("/usr/bin/wine")
    startfile = os.system
    folder_engines = "./Engines/Linux%d" % (64 if is64bits else 32)
else:
    folder_engines = "./Engines/Windows"
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

listaGestoresMotor = None

