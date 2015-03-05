# -*- coding: latin-1 -*-

import sys
import time
import subprocess

import Code.VarGen as VarGen
import Code.Util as Util
import Code.QT.QTUtil as QTUtil

def xVoyager(wowner, configuracion, partida):
    fdb = configuracion.ficheroTemporal("db")
    db = Util.DicRaw(fdb)
    db["USER"] = configuracion.user
    db["PARTIDA"] = partida.guardaEnTexto()

    if sys.argv[0].endswith(".py"):
        li = ["pythonw.exe" if VarGen.isWindows else "python", "Lucas.py", "-voyager", fdb]
    else:
        li = ["Lucas.exe" if VarGen.isWindows else "Lucas", "-voyager", fdb]

    subprocess.Popen(li)
    time.sleep(1.2)
    wowner.setVisible(False)

    while True:
        time.sleep(0.2)
        if "TXTGAME" in db:
            txtp = db["TXTGAME"]
            db.close()
            wowner.setVisible(True)
            QTUtil.refreshGUI()
            return txtp

