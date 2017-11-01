# -*- coding: utf-8 -*-

import os
import base64
import copy
import operator

from Code.QT import Iconos
from Code.QT import TabTipos
from Code import TrListas
from Code import Util
from Code import VarGen
from Code.Constantes import *


class Grupo:
    def __init__(self, nombre, desde, hasta, minPuntos, liRivales):
        self.nombre = nombre
        self.desde = desde
        self.hasta = hasta
        self.minPuntos = minPuntos
        self.liRivales = liRivales

    def puntuacion(self):
        pts = 0
        for cm in self.liRivales:
            pts += cm.puntuacion()
        return pts

    def limpia(self):
        for cm in self.liRivales:
            cm.limpia()


class Grupos:
    def __init__(self, conf):
        self.liGrupos = []
        li = []
        for clave, cm in conf.dicRivales.iteritems():
            li.append((cm.elo, clave, cm))

        self.liRivales = sorted(li, key=operator.itemgetter(0))
        self.configuracion = conf

    def nuevo(self, nombre, desde, hasta, minPuntos):
        liRivalesUno = []
        minimo = 999999999
        for elo, clave, cm in self.liRivales:
            if desde <= elo <= hasta:
                liRivalesUno.append(cm)
                if elo < minimo:
                    minimo = elo
                    nombre = cm.clave
        self.liGrupos.append(Grupo(nombre.capitalize(), desde, hasta, minPuntos, liRivalesUno))

    def puntuacion(self):
        puntos = 0
        for g in self.liGrupos:
            if puntos < g.minPuntos:
                break
            else:
                puntos += g.puntuacion()
        return puntos

    def limpia(self):
        for g in self.liGrupos:
            g.limpia()


class Categoria:
    def __init__(self, clave, c_icono, ayudas, sinAyudasFinal, valorPuntos):
        self.clave = clave
        self.c_icono = c_icono
        self.ayudas = ayudas
        self.sinAyudasFinal = sinAyudasFinal
        self.nivelHecho = 0
        self.hecho = ""
        self.valorPuntos = valorPuntos

    def icono(self):
        return Iconos.icono(self.c_icono)

    def nombre(self):
        return TrListas.categoria(self.clave)

    def siHechoBlancas(self):
        return "B" in self.hecho

    def graba(self):
        return self.clave + "," + str(self.nivelHecho) + "," + self.hecho

    def lee(self, txt):
        li = txt.split(",")
        self.nivelHecho = int(li[1])
        self.hecho = li[2]

    def puntuacion(self, tope):
        p = 0
        tope = min(tope, self.nivelHecho + 1)
        for nv in range(1, tope):
            p += nv
        p *= 2
        if self.hecho:
            p += (self.nivelHecho + 1) * len(self.hecho)

        return p * self.valorPuntos

    def maxPuntos(self):
        return (self.nivelHecho + 1) * self.valorPuntos

    def limpia(self):
        self.nivelHecho = 0
        self.hecho = ""


class Categorias:
    def __init__(self):
        li = []
        li.append(Categoria("PRINCIPIANTE", "Amarillo", 7, False, 5))
        li.append(Categoria("AFICIONADO", "Naranja", 5, False, 10))
        li.append(Categoria("CANDIDATOMAESTRO", "Verde", 3, False, 20))
        li.append(Categoria("MAESTRO", "Azul", 2, False, 40))
        li.append(Categoria("CANDIDATOGRANMAESTRO", "Magenta", 1, True, 80))
        li.append(Categoria("GRANMAESTRO", "Rojo", 0, True, 160))
        self.lista = li

    def numero(self, num):
        return self.lista[num]

    def segunClave(self, clave):
        for una in self.lista:
            if una.clave == clave:
                return una
        return None

    def compruebaNivelesHechos(self):
        maxn = 10000
        for num, cat in enumerate(self.lista):
            if len(cat.hecho) == 2:
                if cat.nivelHecho < maxn:
                    cat.nivelHecho += 1
                    cat.hecho = ""
            maxn = cat.nivelHecho - 1

    def ponResultado(self, categoria, nivel, hecho):
        if hecho not in categoria.hecho:
            categoria.hecho += hecho
            self.compruebaNivelesHechos()
            return categoria.nivelHecho == nivel
        return False

    def graba(self):
        txt = ""
        for cat in self.lista:
            txt += cat.graba() + "|"
        return txt.rstrip("|")

    def lee(self, txt):
        for una in txt.split("|"):
            clave = una.split(",")[0]
            for cat in self.lista:
                if cat.clave == clave:
                    cat.lee(una)

    def puntuacion(self):
        p = 0
        maxNivel = 999999
        for cat in self.lista:
            n = cat.puntuacion(maxNivel)
            if n:
                p += n
            else:
                break
            maxNivel = cat.nivelHecho
        return p

    def limpia(self):
        for cat in self.lista:
            cat.limpia()

    def maxNivelCategoria(self, categoria):
        maxNivel = categoria.nivelHecho + 1
        for num, cat in enumerate(self.lista):
            if categoria.clave == cat.clave:
                if maxNivel > cat.nivelHecho and cat.nivelHecho < 36:
                    maxNivel = cat.nivelHecho + 1
                    hecho = categoria.hecho
                    puntos = maxNivel * cat.valorPuntos
                else:
                    hecho = ""
                    puntos = 0
                return maxNivel, hecho, puntos
            maxNivel = cat.nivelHecho


class ConfigMotorBase:
    def __init__(self, clave, autor, version):
        self.clave = clave
        self.autor = autor
        self.args = []
        self.version = version
        self.path = ""
        self.siUCI = True
        self.liUCI = []
        self.multiPV = 0
        self.maxMultiPV = 0
        self.siDebug = False
        self.siExterno = False
        self.path_64 = None

    def argumentos(self):
        return self.args

    def debug(self, txt):
        self.siDebug = True
        self.nomDebug = self.clave + "-" + txt

    def ordenUCI(self, comando, valor):
        self.liUCI.append((comando, valor))

    def removeUCI(self, del_comando):
        for n, (comando, valor) in enumerate(self.liUCI):
            if comando == del_comando:
                del self.liUCI[n]
                return

    def winboard(self):
        self.siUCI = False

    def ponMultiPV(self, num, maximo):
        self.multiPV = num
        self.maxMultiPV = maximo

    def actMultiPV(self, xMultiPV):
        if xMultiPV == "PD":
            multiPV = min(self.maxMultiPV, 10)
            multiPV = max(multiPV, self.multiPV)
            for comando, valor in self.liUCI:
                if comando == "MultiPV":
                    multiPV = int(valor)
                    break
            self.multiPV = multiPV

        elif xMultiPV == "MX":
            self.multiPV = self.maxMultiPV
        else:
            self.multiPV = int(xMultiPV)
            if self.multiPV > self.maxMultiPV:
                self.multiPV = self.maxMultiPV

    def puedeSerTutor(self):
        return self.maxMultiPV >= 10


class ConfigMotor(ConfigMotorBase):
    def __init__(self, clave, autor, version, url, carpeta=None):
        ConfigMotorBase.__init__(self, clave, autor, version)
        self.puntos = 0
        self.url = url
        self.categorias = Categorias()
        if carpeta:
            self.carpeta = carpeta
        else:
            self.carpeta = clave

        self.siExterno = False

        self._nombre = None

    def removeLog(self, fich):
        Util.borraFichero(os.path.join(VarGen.folder_engines, self.carpeta, fich))

    def graba(self):
        return self.clave + "#" + self.categorias.graba()

    def lee(self, txt):
        clave, txtCat = txt.split("#")
        self.categorias.lee(txtCat)

    def test_bmi2(self):
        if self.path_64 and VarGen.configuracion.bmi2:
            self.path, self.version = self.path_64
            self.path_64 = None

    @property
    def nombre(self):
        self.test_bmi2()
        if self._nombre:
            return self._nombre
        return Util.primeraMayuscula(self.carpeta) + " " + self.version

    @nombre.setter
    def nombre(self, value):
        self._nombre = value

    def ejecutable(self):
        self.test_bmi2()
        return "%s/%s/%s" % (VarGen.folder_engines, self.carpeta, self.path)

    def puntuacion(self):
        return self.categorias.puntuacion()

    def rotuloPuntos(self):
        puntos = self.puntuacion()
        eti = self.nombre
        if puntos:
            eti += " [%d %s]" % (puntos, _("pts"))
        return eti

    def maxNivelCategoria(self, categoria):
        return self.categorias.maxNivelCategoria(categoria)

    def claveReal(self):
        return self.clave

    def limpia(self):
        self.categorias.limpia()

    def clona(self):
        return copy.deepcopy(self)


class ConfigTabTema:
    def __init__(self):
        self.defecto()

        # def defecto( self ):
        # self._colorBlancas = 4294769904
        # self._colorNegras = 4289514196
        # self._colorExterior = 4294967295
        # self._colorTexto = 4278190196
        # self._colorFrontera = 4289514196
        # self._fTransicion = self.flechaDefecto()
        # self._siTemaDefecto = False

        # def flechaDefecto( self ):
        # bf = TabTipos.Flecha()
        # bf.grosor=3
        # bf.altocabeza=20
        # bf.tipo=1
        # bf.destino="m"
        # bf.anchoCasilla=36
        # bf.color=4287203824
        # bf.colorinterior=4287203824
        # bf.opacidad=0.8
        # bf.redondeos=True
        # bf.forma="3"
        # bf.ancho=4
        # bf.vuelo=5
        # bf.descuelgue=6
        # bf.posicion.orden = kZvalue_pieza-1
        # return bf

    def defecto(self):
        # self._colorBlancas = 4294243572
        # self._colorNegras = 4286419133
        # self._colorExterior = 4278206043
        # self._colorTexto = 4294243572
        # self._colorFrontera = 4278206043
        # self._fTransicion = self.flechaDefecto()
        # self._siTemaDefecto = False
        self._colorBlancas = 4294769635
        self._colorNegras = 4291672985
        self._colorExterior = 4294967295
        self._colorTexto = 4286212691
        self._colorFrontera = 4288262839
        self._fTransicion = self.flechaDefecto()
        self._fAlternativa = self.flechaAlternativaDefecto()
        self._siTemaDefecto = False
        self._png64Blancas = ""
        self._png64Negras = ""
        self._transBlancas = 0
        self._transNegras = 0
        self._colorFondo = self._colorBlancas
        self._png64Fondo = ""
        self._fActivo = self.flechaActivoDefecto()
        self._fRival = self.flechaRivalDefecto()
        self._png64Thumb = ""
        self._extendedColor = False

    def flechaDefecto(self):
        bf = TabTipos.Flecha()
        bf.grosor = 1
        bf.altocabeza = 20
        bf.tipo = 1
        bf.destino = "m"
        bf.anchoCasilla = 32
        bf.color = 4293848576
        bf.colorinterior = 4292532736
        bf.opacidad = 0.8
        bf.redondeos = True
        bf.forma = "3"
        bf.ancho = 4
        bf.vuelo = 5
        bf.descuelgue = 6
        bf.posicion.orden = kZvalue_pieza - 1
        return bf

    def flechaAlternativaDefecto(self):
        bf = self.flechaDefecto()
        bf.grosor = 4
        # bf.altocabeza=6
        bf.forma = "a"
        bf.tipo = 2
        return bf

    def flechaActivoDefecto(self):
        bf = TabTipos.Flecha()
        bf.posicion.orden = kZvalue_pieza - 2
        bf.grosor = 1
        bf.altocabeza = 22
        bf.tipo = 1
        bf.destino = "m"
        bf.anchoCasilla = 36
        bf.color = 4283760767
        bf.colorinterior = 4294923520
        bf.colorinterior2 = -1
        bf.redondeos = False
        bf.forma = "3"
        bf.ancho = 5
        bf.vuelo = 4
        bf.descuelgue = 6
        return bf

    def flechaRivalDefecto(self):
        bf = TabTipos.Flecha()
        bf.posicion.orden = kZvalue_pieza - 2
        bf.grosor = 1
        bf.altocabeza = 22
        bf.tipo = 1
        bf.destino = "m"
        bf.anchoCasilla = 36
        bf.color = 4281749760
        bf.colorinterior = 4289396480
        bf.colorinterior2 = -1
        bf.redondeos = False
        bf.forma = "3"
        bf.ancho = 5
        bf.vuelo = 4
        bf.descuelgue = 6
        return bf

    def graba(self):
        return str(self._colorExterior) + "#" \
               + str(self._colorBlancas) + "#" \
               + str(self._colorNegras) + "#" \
               + Util.dic2txt(self._fTransicion) + "#" \
               + str(self._colorTexto) + "#" \
               + str(self._colorFrontera) + "#" \
               + str(self._siTemaDefecto) + "#" \
               + Util.dic2txt(self._fAlternativa) + "#" \
               + self._png64Blancas + "#" \
               + self._png64Negras + "#" \
               + str(self._transBlancas) + "#" \
               + str(self._transNegras) + "#" \
               + str(self._colorFondo) + "#" \
               + self._png64Fondo + "#" \
               + Util.dic2txt(self._fActivo) + "#" \
               + Util.dic2txt(self._fRival) + "#" \
               + self._png64Thumb + "#" \
               + ("1" if self._extendedColor else "0") + "#"

    def lee(self, txt):
        self.defecto()
        try:
            li = txt.split("#")
            nli = len(li)
            falt = None
            if nli >= 6:
                self._colorExterior = int(li[0])
                self._colorBlancas = int(li[1])
                self._colorNegras = int(li[2])
                self._fTransicion = Util.txt2dic(li[3])
                self._colorTexto = int(li[4])
                self._colorFrontera = int(li[5])
                if nli >= 7:
                    self._siTemaDefecto = li[6] == "True"
                    if nli >= 8:
                        falt = Util.txt2dic(li[7])
                        if nli >= 9:
                            self._png64Blancas = li[8]
                            self._png64Negras = li[9]
                            self._transBlancas = int(li[10])
                            self._transNegras = int(li[11])
                            self._colorFondo = int(li[12])
                            self._png64Fondo = li[13]
                            self._fActivo = Util.txt2dic(li[14])
                            self._fRival = Util.txt2dic(li[15])
                            if nli >= 17:
                                self._png64Thumb = li[16]
                                if nli >= 18:
                                    self._extendedColor = li[17] == "1"

            if falt is None:
                falt = copy.deepcopy(self._fTransicion)
                falt.grosor = 3
                falt.forma = "a"
                falt.tipo = 2
            self._fAlternativa = falt

        except:
            pass

    def copia(self):
        ct = ConfigTabTema()
        ct.lee(self.graba())
        return ct


class ConfigTabBase:
    def __init__(self):
        self.inicio()

    def inicio(self):
        self._nomPiezas = ""
        self._tipoLetra = ""
        self._cBold = ""
        self._tamLetra = -1
        self._tamRecuadro = -1
        self._tamFrontera = -1
        self._nCoordenadas = -1
        self._sepLetras = -9999

    def defecto(self):
        self._nomPiezas = "Chessicons"
        self._tipoLetra = "Arial"
        self._cBold = "S"
        self._tamLetra = 72
        self._tamRecuadro = 100
        self._tamFrontera = 100
        self._nCoordenadas = 4
        self._sepLetras = 100

    def graba(self):
        return self._nomPiezas + "#" \
               + self._tipoLetra + "#" \
               + self._cBold + "#" \
               + str(self._tamLetra) + "#" \
               + str(self._tamRecuadro) + "#" \
               + str(self._nCoordenadas) + "#" \
               + str(self._sepLetras) + "#" \
               + str(self._tamFrontera)

    def lee(self, txt):
        li = txt.split("#")
        nli = len(li)
        if nli >= 6:
            self._nomPiezas = li[0]
            self._tipoLetra = li[1]
            self._cBold = li[2]
            self._tamLetra = int(li[3])
            self._tamRecuadro = int(li[4])
            self._nCoordenadas = int(li[5])
            if nli >= 7:
                self._sepLetras = int(li[6])
                if nli >= 8:
                    self._tamFrontera = int(li[7])

    def copia(self):
        ct = ConfigTabBase()
        ct.lee(self.graba())
        return ct


class ConfigTablero:
    def __init__(self, xid, anchoPieza, padre="BASE"):
        self._id = xid
        self._anchoPieza = anchoPieza
        self._anchoPiezaDef = anchoPieza
        self._tema = ConfigTabTema()
        self._tema._siTemaDefecto = xid != "BASE"
        self._base = ConfigTabBase()
        self._confPadre = None
        self._padre = padre
        self.siBase = xid == "BASE"

    def __str__(self):
        return self._tema.graba()

    def id(self):
        return self._id

    def confPadre(self):
        if self._confPadre is None:
            self._confPadre = VarGen.configuracion.confTablero(self._padre, self._anchoPiezaDef)
        return self._confPadre

    def anchoPieza(self, valor=None):
        if valor is None:
            return self._anchoPieza if self._anchoPieza else self.ponDefAnchoPieza()
        else:
            self._anchoPieza = valor
            return self._anchoPieza

    def ponDefAnchoPieza(self):
        return self.anchoPieza(self._anchoPiezaDef)

    def cambiaPiezas(self, nomPiezas):
        self._base._nomPiezas = nomPiezas

    def nomPiezas(self):
        np = self._base._nomPiezas
        if np == "" and self._id == "BASE":
            return "Chessicons"
        return np if np else self.confPadre().nomPiezas()

    def nCoordenadas(self):
        nc = self._base._nCoordenadas
        if self._id == "BASE" and nc == -1:
            return 4
        return self.confPadre().nCoordenadas() if nc < 0 else nc

    def sepLetras(self):
        nc = self._base._sepLetras
        if self._id == "BASE" and nc == -9999:
            return 190
        return self.confPadre().sepLetras() if nc == -9999 else nc

    def siDefPiezas(self):
        return self._base._nomPiezas == ""

    def siDefBold(self):
        return self._base._cBold == ""

    def siDefTipoLetra(self):
        return self._base._tipoLetra == ""

    def siDefTamLetra(self):
        return self._base._tamLetra == -1

    def siDefTamRecuadro(self):
        return self._base._tamRecuadro == -1

    def siDefTamFrontera(self):
        return self._base._tamFrontera == -1

    def siDefCoordenadas(self):
        return self._base._nCoordenadas == -1

    def siDefSepLetras(self):
        return self._base._sepLetras == -9999

    def siDefTema(self):
        return self._tema._siTemaDefecto

    def ponDefTema(self, si):
        self._tema._siTemaDefecto = si

    def guardaEnDisco(self):
        VarGen.configuracion.cambiaConfTablero(self)

    def porDefecto(self, tipo):
        tp = tipo[0]
        siC = siS = siR = False
        if tp == "t":
            siC = siS = siR = True
        elif tp == "c":
            siC = True
        elif tp == "s":
            siS = True
        elif tp == "r":
            siR = True
        siBase = self._id == "BASE"
        if siC:
            if siBase:
                self._tema.defecto()
            else:
                self._tema._siTemaDefecto = True
        if siS:
            self.anchoPieza(self._anchoPiezaDef)
        if siR:
            if siBase:
                self._base.defecto()
            else:
                self._base.inicio()

    def graba(self):
        txt = str(self._anchoPieza) + "·" \
              + "" + "·" \
              + self._tema.graba() + "·" \
              + self._base.graba()
        return "A" + base64.encodestring(
                txt)  # nuevo metodo ya que daba problemas la grabacion de tableros en sqlite+cpickle

    def grabaTema(self):
        return self._tema.graba()

    def leeTema(self, txt):
        self._tema.lee(txt)

    def grabaBase(self):
        return self._base.graba()

    def leeBase(self, txt):
        self._base.lee(txt)

    def lee(self, txt):
        if txt.startswith("A"):  # metodo nuevo
            txt = base64.decodestring(txt[1:])
        li = txt.split("·")
        nli = len(li)
        if nli >= 4:
            self._anchoPieza = int(li[0])
            # ancho_Tablero = int(li[1])
            self._tema.lee(li[2])
            if self._id == "BASE":
                self._tema._siTemaDefecto = False
            self._base.lee(li[3])

    def copia(self, xid):
        ct = ConfigTablero(xid, self._anchoPiezaDef)
        ct.lee(self.graba())
        return ct

    def colorBlancas(self, nColor=None):
        if nColor is not None:
            self._tema._colorBlancas = nColor
        return self.confPadre().colorBlancas() if self._tema._siTemaDefecto else self._tema._colorBlancas

    def colorNegras(self, nColor=None):
        if nColor is not None:
            self._tema._colorNegras = nColor
        return self.confPadre().colorNegras() if self._tema._siTemaDefecto else self._tema._colorNegras

    def colorFondo(self, nColor=None):
        if nColor is not None:
            self._tema._colorFondo = nColor
        return self.confPadre().colorFondo() if self._tema._siTemaDefecto else self._tema._colorFondo

    def transBlancas(self, nTrans=None):
        if nTrans is not None:
            self._tema._transBlancas = nTrans
        return self.confPadre().transBlancas() if self._tema._siTemaDefecto else self._tema._transBlancas

    def transNegras(self, nTrans=None):
        if nTrans is not None:
            self._tema._transNegras = nTrans
        return self.confPadre().transNegras() if self._tema._siTemaDefecto else self._tema._transNegras

    def png64Blancas(self, png64=None):
        if png64 is not None:
            self._tema._png64Blancas = png64
        return self.confPadre().png64Blancas() if self._tema._siTemaDefecto else self._tema._png64Blancas

    def png64Negras(self, png64=None):
        if png64 is not None:
            self._tema._png64Negras = png64
        return self.confPadre().png64Negras() if self._tema._siTemaDefecto else self._tema._png64Negras

    def png64Fondo(self, png64=None):
        if png64 is not None:
            self._tema._png64Fondo = png64
        return self.confPadre().png64Fondo() if self._tema._siTemaDefecto else self._tema._png64Fondo

    def png64Thumb(self, png64=None):
        if png64 is not None:
            self._tema._png64Thumb = png64
        return self.confPadre().png64Thumb() if self._tema._siTemaDefecto else self._tema._png64Thumb

    def extendedColor(self, ext=None):
        if ext is not None:
            self._tema._extendedColor = ext
        return self.confPadre().extendedColor() if self._tema._siTemaDefecto else self._tema._extendedColor

    def colorExterior(self, nColor=None):
        if nColor is not None:
            self._tema._colorExterior = nColor
        return self.confPadre().colorExterior() if self._tema._siTemaDefecto else self._tema._colorExterior

    def colorTexto(self, nColor=None):
        if nColor is not None:
            self._tema._colorTexto = nColor
        return self.confPadre().colorTexto() if self._tema._siTemaDefecto else self._tema._colorTexto

    def colorFrontera(self, nColor=None):
        if nColor is not None:
            self._tema._colorFrontera = nColor
        return self.confPadre().colorFrontera() if self._tema._siTemaDefecto else self._tema._colorFrontera

    def fTransicion(self, valor=None):
        if valor:
            self._tema._fTransicion = valor
        else:
            return self.confPadre().fTransicion() if self._tema._siTemaDefecto else self._tema._fTransicion

    def fAlternativa(self, valor=None):
        if valor:
            self._tema._fAlternativa = valor
        else:
            return self.confPadre().fAlternativa() if self._tema._siTemaDefecto else self._tema._fAlternativa

    def flechaDefecto(self):
        return self._tema.flechaDefecto()

    def flechaAlternativaDefecto(self):
        return self._tema.flechaAlternativaDefecto()

    def fActivo(self, valor=None):
        if valor:
            self._tema._fActivo = valor
        else:
            return self.confPadre().fActivo() if self._tema._siTemaDefecto else self._tema._fActivo

    def flechaActivoDefecto(self):
        return self._tema.flechaActivoDefecto()

    def fRival(self, valor=None):
        if valor:
            self._tema._fRival = valor
        else:
            return self.confPadre().fRival() if self._tema._siTemaDefecto else self._tema._fRival

    def flechaRivalDefecto(self):
        return self._tema.flechaRivalDefecto()

    def tipoLetra(self):
        t = self._base._tipoLetra
        if not t:
            return "Arial" if self.siBase else self.confPadre().tipoLetra()
        else:
            return t

    def siBold(self):
        t = self._base._cBold
        if not t:
            return True if self.siBase else self.confPadre().siBold()
        else:
            return t == "S"

    def tamLetra(self):
        t = self._base._tamLetra
        if t < 0:
            return 72 if self.siBase else self.confPadre().tamLetra()
        else:
            return t

    def tamRecuadro(self):
        t = self._base._tamRecuadro
        if t < 0:
            return 100 if self.siBase else self.confPadre().tamRecuadro()
        else:
            return t

    def tamFrontera(self):
        t = self._base._tamFrontera
        if t < 0:
            return 100 if self.siBase else self.confPadre().tamFrontera()
        else:
            return t

    def ponNomPiezas(self, valor=None):
        if not valor:
            self._base._nomPiezas = "Chessicons" if self.siBase else ""
        else:
            self._base._nomPiezas = valor
        return self._base._nomPiezas

    def ponTipoLetra(self, valor=None):
        if not valor:
            self._base._tipoLetra = "Arial" if self.siBase else ""
        else:
            self._base._tipoLetra = valor
        return self._base._tipoLetra

    def ponBold(self, valor=None):
        if valor is None:
            self._base._cBold = "S" if self.siBase else ""
        else:
            self._base._cBold = "S" if valor else "N"
        return self._base._cBold == "S"

    def ponTamLetra(self, valor=None):
        if valor is None:
            self._base._tamLetra = 100 if self.siBase else -1
        else:
            self._base._tamLetra = valor
        return self._base._tamLetra

    def ponTamRecuadro(self, valor=None):
        if valor is None:
            self._base._tamRecuadro = 100 if self.siBase else -1
        else:
            self._base._tamRecuadro = valor
        return self._base._tamRecuadro

    def ponTamFrontera(self, valor=None):
        if valor is None:
            self._base._tamFrontera = 100 if self.siBase else -1
        else:
            self._base._tamFrontera = valor
        return self._base._tamFrontera

    def ponCoordenadas(self, valor=None):
        if valor is None:
            self._base._nCoordenadas = 4 if self.siBase else -1
        else:
            self._base._nCoordenadas = valor
        return self._base._nCoordenadas

    def ponSepLetras(self, valor=None):
        if valor is None:
            self._base._sepLetras = 190 if self.siBase else -9999
        else:
            self._base._sepLetras = valor
        return self._base._nCoordenadas
