import os

from Code import BaseConfig
from Code import Util
from Code import VarGen
from Code import XMotor


class OpcionUCI:
    def leeTXT(self, txt):
        li = txt.split(VarGen.XSEP)
        self.tipo = li[0]
        self.nombre = li[1]
        self.default = li[2]
        self.valor = li[3]
        # if self.default != self.valor:
        #     p rint self.nombre+":"+self.valor,

        if self.tipo == "spin":
            self.default = int(self.default)
            self.valor = int(self.valor)
            self.min = int(li[4])
            self.max = int(li[5])

        elif self.tipo == "check":
            self.default = self.default.lower() == "true"
            self.valor = self.valor.lower() == "true"

        elif self.tipo == "button":
            self.default = False
            self.valor = self.valor.lower() == "true"

        elif self.tipo == "combo":
            self.liVars = eval(li[4])

    def label_default(self):
        if self.tipo == "spin":
            return "%d:%d-%d" % (self.default, self.min, self.max)

        elif self.tipo == "check":
            return str(self.default).lower()

        elif self.tipo == "button":
            return str(self.default).lower()

        elif self.tipo == "combo":
            return self.default
        return ""

    def grabaTXT(self):
        x = VarGen.XSEP
        txt = self.tipo + x + self.nombre + x + str(self.default) + x + str(self.valor) + x

        if self.tipo == "spin":
            txt += str(self.min) + VarGen.XSEP + str(self.max)

        elif self.tipo == "combo":
            txt += str(self.liVars)

        return txt

    def lee(self, txt):
        while "  " in txt:
            txt = txt.replace("  ", " ")

        n = txt.find("type")
        if (n < 10) or ("chess960" in txt.lower()):
            return False

        self.nombre = txt[11:n].strip()
        li = txt[n:].split(" ")
        self.tipo = li[1]
        self.default = False

        if self.tipo == "spin":
            resp = self.leeSpin(li)

        elif self.tipo == "check":
            resp = self.leeCheck(li)

        elif self.tipo == "combo":
            resp = self.leeCombo(li)

        elif self.tipo == "string":
            resp = self.leeString(li)

        elif self.tipo == "button":
            resp = True

        if resp:
            self.valor = self.default

        return resp

    def leeSpin(self, li):
        if len(li) == 8:
            for x in [2, 4, 6]:
                n = li[x + 1]
                nm = n[1:] if n.startswith("-") else n
                if not nm.isdigit():
                    return False
                n = int(n)
                cl = li[x].lower()
                if cl == "default":
                    self.default = n
                elif cl == "min":
                    self.min = n
                elif cl == "max":
                    self.max = n
            return True
        else:
            return False

    def leeCheck(self, li):
        if len(li) == 4 and li[2] == "default":
            self.default = li[3] == "true"
            return True
        else:
            return False

    def leeString(self, li):
        if (len(li) == 3 or len(li) == 4) and li[2] == "default": # proposed by tico-tico in https://github.com/lukasmonk/lucaschess/issues/18
            self.default = "" if len(li) == 3 or li[3] == "<empty>" else li[3] # proposed by tico-tico
            return True
        else:
            return False

    def leeCombo(self, li):
        self.liVars = []
        self.default = ""
        siDefault = False
        nvar = -1
        for x in li[2:]:
            if x == "var":
                siDefault = False
                nvar += 1
                self.liVars.append("")
            elif x == "default":
                siDefault = True
            else:
                if siDefault:
                    if self.default:
                        self.default += " "
                    self.default += x
                else:
                    c = self.liVars[nvar]
                    if c:
                        c += " " + x
                    else:
                        c = x
                    self.liVars[nvar] = c

        return self.default and (self.default in self.liVars)


class MotorExterno:
    def __init__(self):
        self.exe = ""
        self.alias = ""
        self.idName = ""
        self.nombre = ""
        self.clave = ""
        self.idAuthor = ""
        self.idInfo = ""
        self.liOpciones = []
        self.maxMultiPV = 0
        self.multiPV = 0
        self.elo = 0
        self.args = []

    def actMultiPV(self, xMultiPV):
        if xMultiPV == "PD":
            pass
        elif xMultiPV == "MX":
            self.multiPV = self.maxMultiPV
        else:
            self.multiPV = int(xMultiPV)
            if self.multiPV > self.maxMultiPV:
                self.multiPV = self.maxMultiPV

    def leerUCI(self, exe, args=[]):
        self.exe = Util.dirRelativo(exe)
        self.args = args
        self.liOpciones = []

        motor = XMotor.XMotor("-", exe, args=args)
        if motor.uci_ok:
            self.idName = "-"
            self.idAuthor = "-"
            for linea in motor.uci_lines:
                linea = linea.strip()
                if linea.startswith("id name"):
                    self.idName = linea[8:]
                elif linea.startswith("id author"):
                    self.idAuthor = linea[10:]
                elif linea.startswith("option name "):
                    op = OpcionUCI()
                    if op.lee(linea):
                        self.liOpciones.append(op)
            self.alias = self.idName
            self.clave = self.idName
        motor.close()
        return motor.uci_ok

    def save(self):
        dic = {}
        dic["EXE"] = Util.dirRelativo(self.exe)
        dic["ARGS"] = self.args
        dic["ALIAS"] = self.alias
        dic["IDNAME"] = self.idName
        dic["IDAUTHOR"] = self.idAuthor
        dic["IDINFO"] = self.idInfo
        dic["ELO"] = self.elo
        txtop = ""
        for opcion in self.liOpciones:
            txtop += opcion.grabaTXT() + "|"
        dic["OPCIONES"] = txtop.strip("|")
        return dic

    def restore(self, dic):
        self.exe = dic["EXE"]
        self.args = dic.get("ARGS", [])
        self.alias = dic["ALIAS"]
        self.idName = dic["IDNAME"]
        self.clave = self.idName
        self.nombre = self.clave
        self.idAuthor = dic["IDAUTHOR"]
        self.idInfo = dic.get("IDINFO", "")
        self.elo = dic.get("ELO", 0)
        self.multiPV = 0
        txtop = dic["OPCIONES"]
        self.liOpciones = []

        for parte in txtop.split("|"):
            if parte:
                op = OpcionUCI()
                op.leeTXT(parte)
                if op.nombre == "MultiPV":
                    self.multiPV = op.max
                    self.maxMultiPV = op.max
                self.liOpciones.append(op)

    def leerTXT(self, txt):
        dic = Util.txt2dic(txt)
        self.restore(dic)
        return os.path.isfile(self.exe)

    def grabarTXT(self):
        dic = self.save()
        return Util.dic2txt(dic)

    def copiar(self):
        me = MotorExterno()
        me.leerTXT(self.grabarTXT())
        return me


class ListaMotoresExternos:
    def __init__(self, fichero):
        self.fichero = fichero
        self.liMotores = []
        self.ultCarpeta = ""

    def leer(self):
        self.liMotores = []
        if os.path.isfile(self.fichero):
            f = open(self.fichero, "rb")
            siIni = True
            siGrabar = False
            # st = set()
            for linea in f:
                linea = linea.strip()
                if siIni:
                    self.ultCarpeta = linea
                    siIni = False
                else:
                    me = MotorExterno()
                    if me.leerTXT(linea):
                        self.liMotores.append(me)
                        # li = me.idInfo.split("\n")
                        # for x in li:
                        # st.add(x)
                    else:
                        siGrabar = True
            f.close()
            # p rint st
            if siGrabar:
                self.grabar()

    def grabar(self):
        f = open(self.fichero, "wb")
        f.write(Util.dirRelativo(self.ultCarpeta) + "\n")
        for me in self.liMotores:
            f.write(me.grabarTXT() + "\n")
        f.close()

    def numDatos(self):
        return len(self.liMotores)

    def nuevo(self, motor):
        self.liMotores.append(motor)


class ConfigMotor(BaseConfig.ConfigMotorBase):
    def __init__(self, motor):
        clave = motor.alias
        autor = motor.idAuthor
        version = motor.idName
        BaseConfig.ConfigMotorBase.__init__(self, clave, autor, version)

        self.exe = motor.exe
        self.args = getattr(motor, "args", [])
        self.siExterno = True

        self.nombre = self.clave

        self.liUCI = []
        for opcion in motor.liOpciones:
            if opcion.default != opcion.valor:
                if opcion.tipo == "button":
                    opcion.valor = None
                self.liUCI.append((opcion.nombre, opcion.valor))
            if opcion.nombre == "MultiPV":
                self.multiPV = opcion.valor
                self.maxMultiPV = opcion.max

    def ejecutable(self):
        return self.exe

    def argumentos(self):
        return self.args

    def claveReal(self):
        return "*" + self.clave


def buscaRival(gestor, txtMotor):
    txtMotor = txtMotor[1:]
    le = ListaMotoresExternos(gestor.configuracion.ficheroMExternos)
    le.leer()
    for me in le.liMotores:
        if me.alias == txtMotor:
            cm = ConfigMotor(me)
            return cm
    return None


def buscaMotor(nomMotor):
    txtMotor = nomMotor[1:]
    le = ListaMotoresExternos(VarGen.configuracion.ficheroMExternos)
    le.leer()
    for me in le.liMotores:
        if me.alias == txtMotor:
            return me
    return None


def buscaRivalExt(nomMotor):
    me = buscaMotor(nomMotor)
    if me:
        cm = ConfigMotor(me)
        return cm
    return None
