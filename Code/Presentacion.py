import random
import time

import LCEngine

from Code import ControlPosicion
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code import Util


class GestorM1:
    def __init__(self, procesador):
        self.pantalla = procesador.pantalla
        self.tablero = self.pantalla.tablero
        self.procesador = procesador

        fmt = "./IntFiles/Mate/mate1.lst"
        with open(fmt) as f:
            li = [linea for linea in f.read().split("\n") if linea.strip()]
        linea = random.choice(li)
        li = linea.split("|")
        uno = random.choice(li)
        fen, mv0 = uno.split(",")
        fen += " w - - 1 1"
        LCEngine.setFen(fen)
        liMv = LCEngine.getExMoves()
        self.liMovs = []
        for mv in liMv:
            if mv.mate():
                self.liMovs.append(mv.movimiento())

        self.cp = ControlPosicion.ControlPosicion()
        self.cp.leeFen(fen)

        self.iniTime = time.time()

        self.siBlancas = " w " in fen
        self.tablero.bloqueaRotacion(False)
        self.tablero.ponMensajero(self.mueveHumano)
        self.tablero.ponPosicion(self.cp)
        self.tablero.ponerPiezasAbajo(self.siBlancas)
        self.tablero.activaColor(self.siBlancas)
        self.tablero.ponIndicador(self.siBlancas)

    def muestraMensaje(self):
        tm = time.time() - self.iniTime

        mensaje = "%s: %0.02f" % (_("Time"), tm)
        if tm < 10.0:
            pmImagen = Iconos.pmSol()
        elif tm < 20.0:
            pmImagen = Iconos.pmSolNubes()
        elif tm < 30.0:
            pmImagen = Iconos.pmNubes()
        else:
            pmImagen = Iconos.pmTormenta()

        background = "#5DB0DB"

        segundos = 2.6
        QTUtil2.mensajeTemporal(self.pantalla, mensaje, segundos, background=background, pmImagen=pmImagen)

    def mueveHumano(self, desde, hasta, coronacion=None):
        movimiento = desde + hasta
        if not coronacion and self.cp.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.siBlancas)
            if coronacion is None:
                return False
        if coronacion:
            movimiento += coronacion.lower()

        for mov in self.liMovs:
            if mov.lower() == movimiento:
                self.tablero.desactivaTodas()
                self.cp.mover(desde, hasta, coronacion)
                self.tablero.ponPosicion(self.cp)
                self.tablero.ponFlechaSC(desde, hasta)

                self.muestraMensaje()
                self.__init__(self.procesador)
                return True

        return False


def basico(procesador, hx, factor=1.0):
    def m(cl, t=4.0):
        n = len(cl) / 2
        li = []
        for x in range(n):
            li.append(cl[x * 2] + cl[x * 2 + 1])
        lista = []
        for x in range(n - 1):
            lista.append((li[x], li[x + 1]))
        return procesador.cpu.muevePiezaLI(lista, t * factor, padre=hx)

    li = ["b6a6a8", "b5a7c6b8", "b3d5d8", "c2c6e8",
          "f2h2h8", "g7h7h1e1", "g6f4h3g1", "g4h4h1",
          "d2f3h4f5h6g8", "e4a4a1", "e2a6c8", "g5c1", "e7d7d1", "b4f8", "b2b7", "e5f3d2b1", "e6c4f1", "f6f2"]

    n = random.randint(0, 7)
    primer = li[n]
    del li[n]

    hx = m(primer, 2.0 * factor)
    for uno in li:
        m(uno)
    return procesador.cpu.ponPosicion(procesador.posicionInicial)


def partidaDia(procesador, hx):
    dia = Util.hoy().day
    lid = Util.LIdisk("./IntFiles/31.pkl")
    dic = lid[dia - 1]
    lid.close()

    liMovs = dic["XMOVS"].split("|")

    cpu = procesador.cpu

    padre = cpu.ponPosicion(procesador.posicionInicial)
    padre = cpu.duerme(0.6, padre=padre, siExclusiva=True)

    for txt in liMovs:
        li = txt.split(",")
        tipo = li[0]

        if tipo == "m":
            desde, hasta, segundos = li[1], li[2], float(li[3])
            hx = cpu.muevePieza(desde, hasta, segundos=segundos, padre=padre)

        elif tipo == "b":
            segundos, movim = float(li[1]), li[2]
            n = cpu.duerme(segundos, padre=padre)
            cpu.borraPieza(movim, padre=n)

        elif tipo == "c":
            m1, m2 = li[1], li[2]
            cpu.cambiaPieza(m1, m2, padre=hx)

        elif tipo == "d":
            dato = float(li[1])
            padre = cpu.duerme(dato, padre=hx, siExclusiva=True)

        elif tipo == "t":
            li = dic["TOOLTIP"].split(" ")
            t = 0
            texto = ""
            for x in li:
                texto += x + " "
                t += len(x) + 1
                if t > 40:
                    texto += "<br>"
                    t = 0
            texto += '<br>Source wikipedia: http://en.wikipedia.org/wiki/List_of_chess_games'
            cpu.toolTip(texto, padre=hx)

            # hx = cpu.duerme( 3.0, padre = hx )
