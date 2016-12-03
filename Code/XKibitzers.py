import datetime

from Code.QT import FormLayout
from Code.QT import Iconos
from Code import Util
from Code import VarGen
from Code import XRun

def listaKibitzersRecuperar(configuracion):
    lista = Util.recuperaVar(configuracion.ficheroKibitzers)
    if lista is None:
        lista = []
    return lista

def listaKibitzersGrabar(configuracion, lista):
    Util.guardaVar(configuracion.ficheroKibitzers, lista)

def nuevaKibitzer(ventana, configuracion):
    # Datos generales
    liGen = [(None, None)]

    # # Nombre
    liGen.append((_("Kibitzer") + ":", ""))

    # Motor
    config = FormLayout.Combobox(_("Engine"), configuracion.comboMotoresCompleto())
    liGen.append((config, "stockfish"))

    # Tipo
    liTipos = ["M",
               ("M", _("Candidates")),
               ("I", _("Indexes") + " - RodentII" ),
               ("S", _("Best move")),
               ("L", _("Best move in one line")),
               ("J", _("Select move")),
               ("C", _("Threats")),
               ("E", _("Stockfish evaluation")),
               ]
    liGen.append((_("Type"), liTipos))

    # Editamos
    resultado = FormLayout.fedit(liGen, title=_("New"), parent=ventana, anchoMinimo=460, icon=Iconos.Kibitzer())

    if resultado:
        accion, resp = resultado

        kibitzer = resp[0]
        motor = resp[1]
        tipo = resp[2]

        # Indexes only with Rodent II
        if tipo == "I":
            motor = "rodentII"
            if not kibitzer: # para que no repita rodent II
                kibitzer = _("Indexes") + " - RodentII"

        if not kibitzer:
            for xtipo, txt in liTipos[1:]:
                if xtipo == tipo:
                    kibitzer = "%s: %s" % (txt, motor)

        d = datetime.datetime.now()
        xid = "MOS" + d.isoformat()[2:].strip("0").replace("-", "").replace("T", "").replace(":", "").replace(".", "")
        fvideo = configuracion.plantillaVideo % xid

        dic = {"NOMBRE": kibitzer, "MOTOR": motor, "TIPO": tipo, "FVIDEO": fvideo}

        liKibitzers = listaKibitzersRecuperar(configuracion)
        liKibitzers.append(dic)

        listaKibitzersGrabar(configuracion, liKibitzers)

        return liKibitzers
    else:
        return None

class Orden:
    def __init__(self):
        self.clave = ""
        self.dv = {}

    def ponVar(self, nombre, valor):
        self.dv[nombre] = valor

    def bloqueEnvio(self):
        self.dv["__CLAVE__"] = self.clave
        return self.dv

class XKibitzer:
    CONFIGURACION = "C"
    FEN = "F"
    TERMINAR = "T"

    def __init__(self, gestor, kibitzer):

        fdb = VarGen.configuracion.ficheroTemporal("db")

        self.ipc = Util.IPC(fdb, True)

        motor = kibitzer["MOTOR"]
        if kibitzer["TIPO"] == "I":
            motor = "rodentII"
        configMotor = gestor.configuracion.buscaRivalExt(motor)

        orden = Orden()
        orden.clave = self.CONFIGURACION
        orden.dv["CONFIGURACION"] = gestor.configuracion
        orden.dv["TITULO"] = kibitzer["NOMBRE"]
        orden.dv["FVIDEO"] = kibitzer["FVIDEO"]
        orden.dv["TIPO"] = kibitzer["TIPO"]
        orden.dv["CONFIG_MOTOR"] = configMotor

        self.escribe(orden)

        self.popen = XRun.run_lucas("-kibitzer", fdb)

    def escribe(self, orden):
        self.ipc.push(orden.bloqueEnvio())

    def siActiva(self):
        return self.popen.poll() is None

    def ponFen(self, fen):
        orden = Orden()
        orden.clave = self.FEN
        orden.dv["FEN"] = fen
        self.escribe(orden)

    def terminar(self):
        try:
            orden = Orden()
            orden.clave = self.TERMINAR
            self.escribe(orden)
            self.ipc.close()
            self.close()
        except:
            pass

    def close(self):
        if self.popen:
            try:
                self.popen.terminate()
                self.popen = None
            except:
                pass
