import os
import random
import time

import LCEngine4 as LCEngine

from Code import ControlPosicion
from Code import Gestor
from Code.QT import DatosNueva
from Code.QT import QTUtil2
from Code import Util
from Code.Constantes import *


class Control60:
    def __init__(self, gestor, siJugador):

        self.db = Util.recuperaVar("./IntFiles/lista60.dkv")
        mas = "J" if siJugador else "R"
        self.fichPuntos = "%s/puntos60%s.dkv" % (gestor.configuracion.carpeta, mas)
        if os.path.isfile(self.fichPuntos):
            self.liPuntos = Util.recuperaVar(self.fichPuntos)
        else:
            self.liPuntos = [[0, 0]] * len(self.db)

    def guardar(self):
        Util.guardaVar(self.fichPuntos, self.liPuntos)

    def numDatos(self):
        return len(self.db)

    def primeroSinHacer(self):
        nd = self.numDatos()
        for i in range(nd):
            if self.liPuntos[i][0] == 0:
                return i
        return nd - 1

    def analisis(self, fila, clave):  # compatibilidad
        return ""

    def conInformacion(self, fila, clave):  # compatibilidad
        return None

    def soloJugada(self, fila, clave):  # compatibilidad
        return None

    def mueve(self, fila, clave):  # compatibilidad
        return False

    def dato(self, fila, clave):
        if clave == "NIVEL":
            return str(fila + 1)
        else:
            tiempo, errores = self.liPuntos[fila]
            ctiempo = str(tiempo)
            ctiempo = "-" if tiempo == 0 else (ctiempo[:-2] + "." + ctiempo[-2:])
            cerrores = "-" if tiempo == 0 else str(errores)
            return ctiempo if clave == "TIEMPO" else cerrores

    def dame(self, numero):
        li = self.db[numero]
        pos = random.randint(0, len(li) - 1)
        return li[pos] + " 0 1"

    def mensResultado(self, numero, tiempo, errores):
        ctiempo = str(tiempo)
        ctiempo = ctiempo[:-2] + "." + ctiempo[-2:]

        if self.liPuntos[numero][0] > 0:
            t0, e0 = self.liPuntos[numero]
            siRecord = False
            if e0 > errores:
                siRecord = True
            elif e0 == errores:
                siRecord = tiempo < t0
        else:
            siRecord = True

        mensaje = "<b>%s</b> : %d<br><b>%s</b> : %d<br><b>%s</b> : %s" % (
            _("Level"), numero + 1, _("Errors"), errores, _("Time"), ctiempo)
        if siRecord:
            mensaje += "<br><br><b>%s</b><br>" % _("New record!")
            self.liPuntos[numero] = [tiempo, errores]
            self.guardar()

        return mensaje, siRecord


class Gestor60(Gestor.Gestor):
    def inicio(self, siJugador):

        self.siJugador = siJugador

        self.pgn = Control60(self, siJugador)

        self.pantalla.columnas60(True)

        self.finJuego()

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.pantalla.ponRotulo1(None)
        self.pantalla.ponRotulo2(None)
        self.mostrarIndicador(False)
        self.ponPiezasAbajo(True)
        self.ponMensajero(self.mueveHumano)
        self.pgnRefresh(True)
        self.pantalla.base.pgn.gotop()
        self.pantalla.tablero.siPosibleRotarTablero = False

        self.tablero.exePulsadoNum = None
        self.quitaInformacion()
        self.refresh()

    def numDatos(self):
        return self.pgn.numDatos()

    def procesarAccion(self, clave):

        if clave == k_mainmenu:
            self.fin60()

        elif clave == k_jugar:
            self.jugar()

        elif clave == k_abandonar:
            self.finJuego()

        else:
            self.rutinaAccionDef(clave)

    def fin60(self):
        self.pantalla.tablero.siPosibleRotarTablero = True
        self.tablero.quitaFlechas()
        self.pantalla.columnas60(False)
        self.procesador.inicio()

    def finJuego(self):
        self.pantalla.ponToolBar((k_mainmenu, k_jugar))
        self.desactivaTodas()
        self.estado = kFinJuego

    def jugar(self, numero=None):

        if self.estado == kJugando:
            self.estado = kFinJuego
            self.desactivaTodas()

        if numero is None:
            pos = self.pgn.primeroSinHacer() + 1
            numero = DatosNueva.numEntrenamiento(self.pantalla, _("Find all moves"),
                                                 pos, etiqueta=_("Level"), pos=pos, mensAdicional="<b>" + _(
                        "Movements must be indicated in the following order: King, Queen, Rook, Bishop, Knight and Pawn.") + "</b>")
            if numero is None:
                return
            numero -= 1

        fen = self.pgn.dame(numero)
        self.numero = numero
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fen)
        self.siJugamosConBlancas = self.siBlancas = cp.siBlancas
        if self.siBlancas:
            siP = self.siJugador
        else:
            siP = not self.siJugador
        self.ponPiezasAbajo(siP)
        self.ponPosicion(cp)
        self.cp = cp
        self.refresh()

        LCEngine.setFen(fen)
        self.liMovs = LCEngine.getExMoves()

        # Creamos un avariable para controlar que se mueven en orden
        d = {}
        fchs = "KQRBNP"
        if not cp.siBlancas:
            fchs = fchs.lower()
        for k in fchs:
            d[k] = ""
        for mov in self.liMovs:
            mov.siElegido = False
            pz = mov.pieza()
            d[pz] += pz
        self.ordenPZ = ""
        for k in fchs:
            self.ordenPZ += d[k]

        self.errores = 0
        self.iniTiempo = time.time()
        self.pendientes = len(self.liMovs)
        self.estado = kJugando

        self.tablero.quitaFlechas()

        mens = ""
        if cp.enroques:
            if ("K" if cp.siBlancas else "k") in cp.enroques:
                mens = "O-O"
            if ("Q" if cp.siBlancas else "q") in cp.enroques:
                if mens:
                    mens += " + "
                mens += "O-O-O"
            if mens:
                mens = _("Castling moves possible") + ": " + mens
        if cp.alPaso != "-":
            mens += " " + _("En passant") + ": " + cp.alPaso

        self.pantalla.ponRotulo1(mens)

        self.nivel = numero
        self.siBlancas = cp.siBlancas
        self.ponRotulo2n()

        self.pantalla.ponToolBar((k_abandonar,))
        self.pantalla.base.pgn.goto(numero, 0)
        self.activaColor(self.siBlancas)

    def ponRotulo2n(self):
        self.pantalla.ponRotulo2("<h3>%s - %s %d - %s : %d</h3>" % (_("White") if self.siBlancas else _("Black"),
                                                                    _("Level"), self.nivel + 1, _("Errors"),
                                                                    self.errores))

    def finalX(self):
        self.procesador.inicio()
        return False

    def mueveHumano(self, desde, hasta, coronacion=None):
        # coronacion = None por compatibilidad
        if desde == hasta:
            return
        a1h8 = desde + hasta
        for mov in self.liMovs:
            if (mov.desde()+mov.hasta()) == a1h8:
                if not mov.siElegido:
                    if mov.pieza() == self.ordenPZ[0]:
                        self.tablero.creaFlechaMulti(a1h8, False, opacidad=0.4)
                        mov.siElegido = True
                        self.ordenPZ = self.ordenPZ[1:]
                        if len(self.ordenPZ) == 0:
                            self.ponResultado()
                    else:
                        break
                self.atajosRatonReset()
                return
        self.errores += 1
        self.ponRotulo2n()
        self.atajosRatonReset()

    def ponResultado(self):
        tiempo = int((time.time() - self.iniTiempo) * 100.0)
        self.finJuego()
        self.ponPosicion(self.cp)

        self.refresh()

        mensaje, siRecord = self.pgn.mensResultado(self.numero, tiempo, self.errores)
        QTUtil2.mensajeTemporal(self.pantalla, mensaje, 3, background="#F22D52" if siRecord else None)

    def analizaPosicion(self, fila, clave):
        if self.estado == kJugando:
            self.finJuego()
            return
        if fila <= self.pgn.primeroSinHacer():
            self.jugar(fila)

    def mueveJugada(self, tipo):
        fila, col = self.pantalla.pgnPosActual()
        if tipo == "+":
            if fila > 0:
                fila -= 1
        elif tipo == "-":
            if fila < (self.pgn.numDatos() - 1):
                fila += 1
        elif tipo == "p":
            fila = 0
        elif tipo == "f":
            fila = self.pgn.numDatos() - 1

        self.pantalla.base.pgn.goto(fila, 0)

    def pgnInformacion(self):
        pass  # Para anular el efecto del boton derecho
