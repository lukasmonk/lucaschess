from PyQt4 import QtCore

from Code import Util
from Code import XRun

def xVoyager(wowner, configuracion, partida=None, fen=None, siFen=False):
    fdb = configuracion.ficheroTemporal("db")
    db = Util.DicRaw(fdb)
    db["USER"] = configuracion.user
    if partida:
        db["PARTIDA"] = partida.guardaEnTexto()
    if siFen:
        db["FEN"] = fen
    db["MODO_PARTIDA"] = partida is not None

    popen = XRun.run_lucas("-voyager", fdb)

    popen.wait()

    txtp = db["RESULT"]
    db.close()
    return txtp

def xVoyagerFEN(wowner, configuracion, fen):
    wowner.showMinimized()
    resp = xVoyager(wowner, configuracion, fen=fen, siFen=True)
    wowner.setWindowState(QtCore.Qt.WindowActive)
    return resp
