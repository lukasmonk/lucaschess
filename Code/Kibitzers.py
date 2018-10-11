import os.path

from PyQt4 import QtGui

from Code import Util
from Code import VarGen
from Code import MotoresExternos
from Code import EngineThread
from Code import XRun
from Code.QT import Iconos


class Tipos:
    def __init__(self):
        self.liTipos = (
                    ("M", _("Candidates"), Iconos.pmPuntoRojo()),
                    ("I", _("Indexes") + " - RodentII", Iconos.pmPuntoNegro()),
                    ("S", _("Best move"), Iconos.pmPuntoVerde() ),
                    ("L", _("Best move in one line"), Iconos.pmPuntoMagenta()),
                    ("J", _("Select move"), Iconos.pmPuntoNaranja()),
                    ("C", _("Threats"), Iconos.pmPuntoAzul()),
                    ("E", _("Stockfish evaluation"), Iconos.pmPuntoAmarillo()),
                    ("B", _("Polyglot book"), Iconos.pmPuntoEstrella())
        )

    def combo(self):
        return [(label, key) for key, label, pm in self.liTipos]

    def comboSinIndices(self):
        return [(label, key) for key, label, pm in self.liTipos if key != "I"]

    def texto(self, tipo):
        for tp, nom, pm in self.liTipos:
            if tp == tipo:
                return nom

    def dicDelegado(self):
        return {tp: pm for tp, txt, pm in self.liTipos}

    def dicIconos(self):
        return {tp: QtGui.QIcon(pm) for tp, txt, pm in self.liTipos}


class Kibitzer(MotoresExternos.MotorExterno):
    def __init__(self):
        MotoresExternos.MotorExterno.__init__(self)

        self.tipo = None
        self.huella = None
        self.prioridad = EngineThread.priorities.normal
        self.posicionBase = False
        self.visible = True
        self.nombre = None

    def ponHuella(self, liEngines):
        liHuellas = [en.huella for en in liEngines if en != self]
        while True:
            self.huella = Util.huella()
            if self.huella not in liHuellas:
                return

    def clonar(self, liEngines):
        otro = Kibitzer()
        otro.leerTXT(self.save2txt())
        otro.tipo = self.tipo
        otro.prioridad = self.prioridad
        otro.nombre = self.nombre
        otro.posicionBase = self.posicionBase
        otro.visible = True
        otro.ponHuella(liEngines)
        lista = [en.nombre for en in liEngines]
        d = 0
        while otro.nombre in lista:
            d += 1
            otro.nombre = "%s-%d" % (self.nombre, d)
        return otro

    def huella(self):
        return self.huella

    def leerTXT(self, txt):
        dic = Util.txt2dic(txt)
        self.restore(dic)
        self.huella = dic["HUELLA"]
        self.tipo = dic["TIPO"]
        self.prioridad = dic["PRIORITY"]
        self.nombre = dic["NOMBRE"]
        self.posicionBase = dic.get("POSICIONBASE", False)
        self.visible =  dic.get("VISIBLE", True)
        return os.path.isfile(self.exe)

    def save2txt(self):
        dic = MotoresExternos.MotorExterno.save(self)
        dic["HUELLA"] = self.huella
        dic["PRIORITY"] = self.prioridad
        dic["TIPO"] = self.tipo
        dic["NOMBRE"] = self.nombre
        dic["POSICIONBASE"] = self.posicionBase
        dic["VISIBLE"] = self.visible
        return Util.dic2txt(dic)

    def configMotor(self):
        return MotoresExternos.ConfigMotor(self)

    def leerConfigEngine(self, resp):
        if resp.startswith("*"):
            me = MotoresExternos.buscaMotor(resp)
            if me:
                self.exe = Util.dirRelativo(me.exe)
                self.args = me.args
                self.alias = me.alias
                self.idName = me.idName
                self.clave = me.clave
                self.idAuthor = me.idAuthor
                self.idInfo = me.idInfo
                self.liOpciones = me.liOpciones
                self.maxMultiPV = me.maxMultiPV
                self.multiPV = me.multiPV
                self.elo = me.elo
            else:
                return False
        else:
            cm = VarGen.configuracion.buscaRival(resp)
            self.alias = cm.clave
            self.args = []
            self.idName = cm.nombre
            self.idAuthor = cm.autor
            self.idInfo = ""
            self.multiPV = cm.multiPV
            self.maxMultiPV = cm.maxMultiPV

            self.exe = cm.ejecutable()

            me = MotoresExternos.MotorExterno()
            me.leerUCI(self.exe, self.args)
            self.liOpciones = me.liOpciones
            for op in self.liOpciones:
                for comando, valor in cm.liUCI:
                    if op.nombre == comando:
                        if op.tipo == "check":
                            op.valor = valor.lower() == "true"
                        elif op.tipo == "spin":
                            op.valor = int(valor)
                        else:
                            op.valor = valor
                        break

        return True

    def readAntiguo(self, dic):
        nom_motor = dic["MOTOR"]
        if not self.leerConfigEngine(nom_motor):
            return False
        self.nombre = dic["NOMBRE"]
        self.tipo = dic["TIPO"]
        self.huella = Util.huella()
        self.visible = True
        self.posicionBase = False
        fvideo = dic.get("FVIDEO")
        if fvideo:
            try:
                fdest = os.path.join(os.path.dirname(fvideo), "KIB%s.video" % self.huella)
                os.rename(fvideo, fdest)
            except:
                pass

        return os.path.isfile(self.exe)

    def ctipo(self):
        return Tipos().texto(self.tipo)

    def cpriority(self):
        return EngineThread.priorities.texto(self.prioridad)


class Kibitzers:
    def __init__(self):
        self.fichero = VarGen.configuracion.ficheroKibitzers
        self.lista, self.lastfolder = self.read()

    def readAntiguo(self, listaAnt):
        lastfolder = ""
        lista = []
        for dic in listaAnt:
            kib = Kibitzer()
            if kib.readAntiguo(dic):
                lista.append(kib)
        self.lista, self.lastfolder = lista, lastfolder
        self.save()
        return lista, lastfolder

    def read(self):
        lista = []
        lastfolder = ""
        dic = Util.recuperaVar(self.fichero)
        if type(dic) == list:
            return self.readAntiguo(dic)

        if dic:
            lastfolder = dic.get("LASTFOLDER", "")
            lista_txt = dic.get("LISTA", [])
            if lista_txt:
                for txt in lista_txt:
                    kib = Kibitzer()
                    if kib.leerTXT(txt):
                        lista.append(kib)
        return lista, lastfolder

    def save(self):
        dic = {
            "LISTA":[en.save2txt() for en in self.lista],
            "LASTFOLDER": self.lastfolder
        }
        Util.guardaVar(self.fichero, dic)

    def nuevo(self, nombre, motor, tipo, prioridad):
        kib = Kibitzer()
        kib.ponHuella(self.lista)
        kib.leerConfigEngine(motor)
        kib.alias = kib.nombre = nombre
        kib.tipo = tipo
        kib.prioridad = prioridad
        self.lista.append(kib)
        self.save()
        return len(self.lista)-1

    def nuevoPolyglot(self, book):
        kib = Kibitzer()
        kib.ponHuella(self.lista)
        kib.alias = kib.nombre = book.nombre
        kib.tipo = "B"
        kib.exe = book.path
        kib.clave = book.nombre
        self.lista.append(kib)
        self.save()
        return len(self.lista)-1

    def __len__(self):
        return len(self.lista)

    def kibitzer(self, num):
        return self.lista[num]

    def remove(self, num):
        del self.lista[num]
        self.save()

    def up(self, num):
        if num > 0:
            self.lista[num], self.lista[num-1] = self.lista[num-1], self.lista[num]
            self.save()
            return num-1
        return None

    def down(self, num):
        if num < (len(self.lista)-1):
            self.lista[num], self.lista[num + 1] = self.lista[num + 1], self.lista[num]
            self.save()
            return num + 1
        return None

    def clonar(self, num):
        kib = self.lista[num].clonar(self.lista)
        self.lista.append(kib)
        self.save()
        return len(self.lista)-1

    def lista_menu(self):
        dIco = Tipos().dicIconos()
        return [(kib.nombre, dIco[kib.tipo]) for kib in self.lista if kib.visible]


class Orden:
    def __init__(self):
        self.clave = ""
        self.dv = {}

    def ponVar(self, nombre, valor):
        self.dv[nombre] = valor

    def bloqueEnvio(self):
        self.dv["__CLAVE__"] = self.clave
        return self.dv


class IPCKibitzer:
    CONFIGURACION = "C"
    FEN = "F"
    TERMINAR = "T"

    def __init__(self, gestor, numkibitzer):
        configuracion = gestor.configuracion

        fdb = configuracion.ficheroTemporal("db")

        self.ipc = Util.IPC(fdb, True)

        orden = Orden()
        orden.clave = self.CONFIGURACION
        orden.dv["USER"] = configuracion.user
        orden.dv["NUMKIBITZER"] = numkibitzer

        self.escribe(orden)

        self.popen = XRun.run_lucas("-kibitzer", fdb)

    def escribe(self, orden):
        self.ipc.push(orden.bloqueEnvio())

    def siActiva(self):
        if self.popen is None:
            return False
        return self.popen.poll() is None

    def ponFen(self, fen, fenBase):
        orden = Orden()
        orden.clave = self.FEN
        orden.dv["FEN"] = "%s|%s" % (fen, fenBase)
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
