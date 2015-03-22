import collections

import VarGen
import Code.MotoresExternos as MotoresExternos

class MotorMicElo:
    def __init__(self, nombre, exe):
        self.nombre = nombre[0].upper() + nombre[1:]
        self.clave = nombre
        self.exe = exe
        self.elo = 0
        self.liUCI = []
        self.book = "Openings/%s.bin" % nombre.split(" ")[0].lower()
        self.multiPV = 0
        self.siDebug = False

    def ejecutable(self):
        return self.exe

    def rotulo(self):
        return self.nombre

    def opcionUCI(self, nombre, valor):
        self.liUCI.append((nombre, valor))
        if nombre == "MultiPV":
            self.multiPV = int(valor)

    def guardaUCI(self):
        return str(self.liUCI)

    def recuperaUCI(self, txt):
        self.liUCI = eval(txt)

def convert(liMotores):
    li = []
    for mt in liMotores:
        mt1 = MotoresExternos.ConfigMotor(mt)
        mt1.book = mt.book
        mt1.elo = mt.elo
        mt1.idInfo = mt.idInfo
        mt1.alias = mt.alias
        li.append(mt1)
    return li

def listaPersonal():
    lme = MotoresExternos.ListaMotoresExternos("IntFiles/michelep.pkt")
    lme.leer()
    for mt in lme.liMotores:
        mt.book = None
    return convert(lme.liMotores)

def listaGM():
    dic = {"champion": _("Champion"), "expert": _("Expert"), "master": _("Master")}
    lme = MotoresExternos.ListaMotoresExternos("IntFiles/micheleg.pkt")
    lme.leer()
    liR = dic.keys()
    for mt in lme.liMotores:
        alias = mt.alias.strip()
        for x in liR:
            if alias.endswith(x):
                alias = alias[:-len(x)]
                mt.book = "Openings/%s.bin" % alias.lower()
                alias = alias[0].upper()+alias[1:].lower() + " - " + dic[x]
                mt.alias = alias
                mt.nombre = alias
                break

    return convert(lme.liMotores)

def listaCompleta():
    li = listaPersonal()
    li.extend(listaGM())
    li.sort(key=lambda uno: uno.elo)
    return li

def dicGM():
    dic = collections.OrderedDict()
    if VarGen.isLinux and not VarGen.isWine:
        return dic

    lista = listaGM()
    for cm in lista:
        gm = cm.alias.split(" ")[0]
        if gm not in dic:
            dic[gm] = []
        dic[gm].append(cm)
    return dic
