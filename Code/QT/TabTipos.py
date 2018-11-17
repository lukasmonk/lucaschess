from PyQt4 import QtGui, QtCore

from Code import VarGen
from Code.QT import Controles
from Code.QT import Colocacion
from Code.QT import Iconos
from Code.QT import QTUtil


class Posicion:
    def __init__(self, x=0, y=0, ancho=0, alto=0, angulo=0, orden=15):
        self.x = x
        self.y = y
        self.ancho = ancho
        self.alto = alto
        self.angulo = angulo
        self.orden = int(orden)

    def copia(self):
        p = Posicion(self.x, self.y, self.ancho, self.alto, self.angulo, self.orden)
        return p

    def __str__(self):
        txt = ""
        for var in ("x", "y", "ancho", "alto", "angulo", "orden"):
            txt += "%s : %s\n" % (var, str(getattr(self, var)))
        return txt


class TipoLetra:
    def __init__(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False):
        self.nombre = nombre
        self.puntos = puntos
        self.peso = peso  # 50 = normal, 75 = negrita, 25 = light,...
        self.siCursiva = siCursiva
        self.siSubrayado = siSubrayado
        self.siTachado = siTachado

    def __str__(self):
        cursiva = 1 if self.siCursiva else 0
        subrayado = 1 if self.siSubrayado else 0
        tachado = 1 if self.siTachado else 0
        return "%s,%d,-1,5,%d,%d,%d,%d,0,0" % (self.nombre, self.puntos, self.peso, cursiva, subrayado, tachado)

    def copia(self):
        t = TipoLetra(self.nombre, self.puntos, self.peso, self.siCursiva, self.siSubrayado, self.siTachado)
        return t


class Bloque:
    def __init__(self):
        self.siMovible = False
        for num, dato in enumerate(self.liVars):
            var, tipo, ini = dato
            exec ("self.%s=ini" % var)
            self.liVars[num] = (var, tipo)

    def tipoqt(self):
        return {1: QtCore.Qt.SolidLine, 2: QtCore.Qt.DashLine, 3: QtCore.Qt.DotLine,
                4: QtCore.Qt.DashDotLine, 5: QtCore.Qt.DashDotDotLine,
                0: QtCore.Qt.NoPen}.get(self.tipo, QtCore.Qt.SolidLine)

    def __str__(self):
        txt = ""
        for var, tipo in self.liVars:
            txt += "%s : %s\n" % (var, str(getattr(self, var)))
        return txt


class Texto(Bloque):
    def __init__(self):
        self.liVars = [
            ("tipoLetra", "o", TipoLetra()),
            ("posicion", "o", Posicion(0, 0, 80, 16, 0)),
            ("alineacion", "t", "i"),
            ("colorTexto", "n", 0),
            ("colorFondo", "n", 0xFFFFFF),
            ("valor", "tn", "")
        ]
        Bloque.__init__(self)

    def copia(self):
        t = Texto()
        t.tipoLetra = self.tipoLetra.copia()
        t.posicion = self.posicion.copia()
        t.alineacion = self.alineacion
        t.colorTexto = self.colorTexto
        t.colorFondo = self.colorFondo
        t.valor = self.valor
        return t


class Imagen(Bloque):
    def __init__(self):
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 80, 0)),
            ("pixmap", "t", None),
        ]
        Bloque.__init__(self)

    def copia(self):
        t = Imagen()
        t.posicion = self.posicion.copia()
        t.pixmap = self.pixmap
        return t


class Caja(Bloque):
    def __init__(self):
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 80, 0)),
            ("color", "n", 0),
            ("colorRelleno", "n", -1),
            ("grosor", "n", 1),
            ("redEsquina", "n", 0),
            ("tipo", "n", 1),  # 1=SolidLine, 2=DashLine, 3=DotLine, 4=DashDotLine, 5=DashDotDotLine, 0=NoPen
        ]
        Bloque.__init__(self)

    def copia(self):
        c = Caja()
        c.posicion = self.posicion.copia()
        c.color = self.color
        c.colorRelleno = self.colorRelleno
        c.grosor = self.grosor
        c.redEsquina = self.redEsquina
        c.tipo = self.tipo
        return c


class Circulo(Bloque):
    def __init__(self):
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 80, 0)),
            ("color", "n", 0),
            ("colorRelleno", "n", -1),
            ("grosor", "n", 1),
            ("grados", "n", 0),
            ("tipo", "n", 1),  # 1=SolidLine, 2=DashLine, 3=DotLine, 4=DashDotLine, 5=DashDotDotLine, 0=Sin borde
        ]
        Bloque.__init__(self)


class Pieza(Bloque):
    def __init__(self):
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 1, 0)),
            ("pieza", "t", "p"),
            ("fila", "n", 1),
            ("columna", "n", 1),
        ]
        Bloque.__init__(self)


class Flecha(Bloque):
    def __init__(self):
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 1, 0)),
            ("a1h8", "c", "a1h8"),
            ("grosor", "n", 1),  # ancho del trazo
            ("altocabeza", "n", 15),  # alto de la cabeza
            ("tipo", "n", 1),  # 1=SolidLine, 2=DashLine, 3=DotLine, 4=DashDotLine, 5=DashDotDotLine
            ("destino", "t", "c"),  # c = centro, m = minimo
            ("anchoCasilla", "n", 1),
            ("color", "n", 0),
            ("colorinterior", "n", -1),  # si es cerrada
            ("colorinterior2", "n", -1),  # para el gradiente
            ("opacidad", "n", 1.0),
            ("redondeos", "l", False),
            ("forma", "t", "a"),
            # a = abierta -> , c = cerrada la cabeza, 1 = poligono cuadrado, 2 = poligono 1 punto base, 3 = poligono 1 punto base cabeza
            ("ancho", "n", 10),  # ancho de la base de la flecha si es un poligono
            ("vuelo", "n", 5),  # ancho adicional en la base
            ("descuelgue", "n", 2),  # angulo de la base de la cabeza
            ("png", "c", "")  # png para usar como boton
        ]
        Bloque.__init__(self)

    def copia(self):
        c = Flecha()
        c.posicion = self.posicion.copia()
        c.a1h8 = self.a1h8
        c.grosor = self.grosor
        c.altocabeza = self.altocabeza
        c.tipo = self.tipo
        c.destino = self.destino
        c.anchoCasilla = self.anchoCasilla
        c.color = self.color
        c.colorinterior = self.colorinterior
        c.colorinterior2 = self.colorinterior2
        c.opacidad = self.opacidad
        c.redondeos = self.redondeos
        c.forma = self.forma
        c.ancho = self.ancho
        c.vuelo = self.vuelo
        c.descuelgue = self.descuelgue
        c.png = getattr(self, "png", "")
        return c


class Marco(Bloque):
    def __init__(self):
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 80, 0)),
            ("a1h8", "c", "a1h8"),
            ("color", "n", 0),
            ("colorinterior", "n", -1),
            ("colorinterior2", "n", -1),  # para el gradiente
            ("grosor", "n", 1),
            ("redEsquina", "n", 0),
            ("tipo", "n", 1),  # 1=SolidLine, 2=DashLine, 3=DotLine, 4=DashDotLine, 5=DashDotDotLine, 0=Sin borde
            ("opacidad", "n", 1.0),
            ("anchoCasilla", "n", 1),
            ("png", "c", "")  # png para usar como boton
        ]
        Bloque.__init__(self)


class SVG(Bloque):
    def __init__(self):
        # orden por debajo de las piezas
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 80, 0, 9)),
            ("fa1h8", "c", "0.0,0.0,0.0,0.0"),
            # se indica en unidades de ancho de casilla, podra tener valores negativos para que se pueda mover fuera de pantalla
            ("xml", "c", ""),
            ("opacidad", "n", 1.0),
            ("anchoCasilla", "n", 1),
            ("psize", "n", 100),  # ajustetama_o
            ("png", "c", "")  # png para usar como boton
        ]
        Bloque.__init__(self)


class Marker(Bloque):
    def __init__(self):
        # orden por debajo de las piezas
        self.liVars = [
            ("posicion", "o", Posicion(0, 0, 80, 80, 0, 9)),
            ("fa1h8", "c", "0.0,0.0,0.0,0.0"),
            # se indica en unidades de ancho de casilla, podra tener valores negativos para que se pueda mover fuera de pantalla
            ("xml", "c", ""),
            ("opacidad", "n", 1.0),
            ("anchoCasilla", "n", 1),
            ("psize", "n", 100),  # % ajustetama_o
            ("poscelda", "n", 1),  # 0 = Up-Left 1 = Up-Right 2 = Down-Right 3 = Down-Left
            ("png", "c", "")  # png para usar como boton
        ]
        Bloque.__init__(self)


class Pizarra(QtGui.QWidget):
    def __init__(self, guion, tablero, ancho, editMode=False, withContinue=False):
        QtGui.QWidget.__init__(self)

        self.guion = guion
        self.tarea = None

        self.mensaje = Controles.EM(self).ponTipoLetra(puntos=VarGen.configuracion.tamFontRotulos)

        self.pb = None
        self.chb = None
        if editMode:
            self.chb = Controles.CHB(self, _("With continue button"), False).capturaCambiado(self, self.save)
            self.mensaje.capturaCambios(self.save)
        elif withContinue:
            self.pb = Controles.PB(self, _("Continue"), self.continuar, plano=False)
            self.bloqueada = True
            self.mensaje.soloLectura()
        else:
            self.mensaje.soloLectura()

        self.pbLeft = Controles.PB(self, "", self.goLeft).ponIcono(Iconos.AnteriorF()).anchoFijo(24)
        self.pbRight = Controles.PB(self, "", self.goRight).ponIcono(Iconos.SiguienteF()).anchoFijo(24)
        self.pbDown = Controles.PB(self, "", self.goDown).ponIcono(Iconos.Abajo()).anchoFijo(24)
        self.pbClose = Controles.PB(self, "", self.borrar).ponIcono(Iconos.CancelarPeque()).anchoFijo(24)

        cajon = QtGui.QWidget(self)
        ly = Colocacion.H()
        ly.control(self.pbLeft).control(self.pbDown)
        ly.control(self.pbRight).control(self.pbClose).margen(0)
        if self.pb:
            ly.control(self.pb)
        if self.chb:
            ly.control(self.chb)
        cajon.setLayout(ly)
        cajon.setFixedHeight(20)

        layout = Colocacion.V().control(self.mensaje).espacio(-6).control(cajon).margen(0)

        self.setLayout(layout)
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.ToolTip)

        posTabl = tablero.pos()
        posTablGlobal = tablero.mapToGlobal(posTabl)
        self.anchoTabl = tablero.width()
        self.anchoPizarra = ancho
        self.x = posTablGlobal.x() - posTabl.x()
        self.y = posTablGlobal.y() - posTabl.y()

        if self.guion.posPizarra == "R":
            self.goRight()
        elif self.guion.posPizarra == "L":
            self.goLeft()
        else:
            self.goDown()

        if editMode:
            self.clearFocus()
            self.mensaje.setFocus()

    def showLRD(self, l, r, d):
        self.pbRight.setVisible(r)
        self.pbLeft.setVisible(l)
        self.pbDown.setVisible(d)

    def goDown(self):
        y = self.y + self.anchoTabl
        self.setGeometry(self.x, y, self.anchoTabl, self.anchoPizarra)
        self.showLRD(True, True, False)
        self.guion.posPizarra = "D"

    def goRight(self):
        x = self.x + self.anchoTabl
        self.setGeometry(x, self.y, self.anchoPizarra, self.anchoTabl)
        self.showLRD(True, False, True)
        self.guion.posPizarra = "R"

    def goLeft(self):
        x = self.x - self.anchoPizarra
        self.setGeometry(x, self.y, self.anchoPizarra, self.anchoTabl)
        self.showLRD(False, True, True)
        self.guion.posPizarra = "L"

    def write(self, tarea):
        self.mensaje.ponHtml(tarea.texto())
        self.tarea = tarea
        if self.chb:
            ok = self.tarea.continuar()
            self.chb.ponValor(False if ok is None else ok)

    def save(self):
        if not self.tarea:
            return
        self.tarea.texto(self.mensaje.html())
        if self.chb:
            self.tarea.continuar(self.chb.valor())
        self.guion.savedPizarra()

    def siBloqueada(self):
        if self.bloqueada:
            QTUtil.refreshGUI()
        return self.bloqueada

    def continuar(self):
        self.bloqueada = False
        self.pb.hide()

    def mousePressEvent(self, event):
        m = int(event.modifiers())
        siCtrl = (m & QtCore.Qt.ControlModifier) > 0
        if siCtrl and event.button() == QtCore.Qt.LeftButton:
            self.guion.borrarPizarraActiva()

    def borrar(self):
        self.guion.borrarPizarraActiva()

