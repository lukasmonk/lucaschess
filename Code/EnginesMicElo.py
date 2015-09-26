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


def leeMicEngines():
    lme = MotoresExternos.ListaMotoresExternos("./IntFiles/michele.pkt")
    lme.leer()
    li = []
    for mt in lme.liMotores:
        mt1 = MotoresExternos.ConfigMotor(mt)
        if mt.alias.isupper():
            mt1.alias = mt1.nombre = mt.alias[0] + mt.alias[1:].lower()
            mt1.book = "Openings/%s.bin" % mt.alias.lower()
        else:
            mt1.alias = mt1.nombre = mt.alias
            mt1.book = None

        mt1.elo = mt.elo
        mt1.idInfo = mt.idInfo
        li.append(mt1)
    return li


def listaGM():
    li = [mtl for mtl in leeMicEngines() if mtl.nombre[0].isupper()]
    li.sort(key=lambda uno: uno.nombre)
    return li


def listaCompleta():
    li = leeMicEngines()
    li.sort(key=lambda uno: uno.elo)
    return li
