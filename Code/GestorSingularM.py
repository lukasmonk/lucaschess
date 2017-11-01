import time

from Code import Gestor
from Code import Partida
from Code.Constantes import *


class GestorSingularM(Gestor.Gestor):
    def inicio(self, singularMoves):
        self.singularMoves = singularMoves

        self.pos_bloque = 0
        self.tipoJuego = kJugSingularMoves

        self.rivalPensando = False

        self.siCompetitivo = True

        self.siJuegaHumano = False
        self.estado = kJugando

        self.pantalla.ponActivarTutor(False)
        self.quitaAyudas(True)

        self.ayudasPGN = 0

        self.pantalla.activaJuego(True, True, siAyudas=False)
        self.pantalla.quitaAyudas(True)
        self.ponMensajero(self.mueveHumano)

        self.pgnRefresh(True)

        self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_abandonar:
            self.resign()

        elif clave == k_siguiente:
            self.siguienteJugada()

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_configurar:
            self.configurar()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finPartida(self):
        self.procesador.inicio()
        self.procesador.strenght101()

    def siguienteJugada(self):
        self.pantalla.ponToolBar([k_mainmenu, k_abandonar])

        self.estado = kJugando

        self.siJuegaHumano = False
        self.siCompetitivo = True

        self.linea_bloque = self.singularMoves.linea_bloque(self.pos_bloque)

        siBlancas = " w " in self.linea_bloque.fen
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.pantalla.ponActivarTutor(False)
        self.quitaAyudas(True)
        self.partida = Partida.Partida(fen=self.linea_bloque.fen)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)
        self.ponIndicador(siBlancas)
        self.pantalla.cambiaRotulosReloj("%d/10" % (self.pos_bloque+1,), _("Complete"))

        self.pantalla.ponRelojBlancas("", None)
        self.pantalla.ponRelojNegras(self.singularMoves.rotulo_media(), None)

        self.refresh()

        self.siJuegaHumano = True
        self.activaColor(siBlancas)

        self.pantalla.iniciaReloj(self.ponReloj, transicion=1500)
        self.time_inicio = time.time()
        self.ponReloj()

    def calc_puntuacion(self, tiempo):
        if tiempo <= 3.0:
            return 100.00
        else:
            max_time = self.linea_bloque.max_time
            if tiempo > max_time:
                return 10.0
            return (max_time-tiempo)*100.0/max_time

    def ponReloj(self):
        p = self.calc_puntuacion(time.time() - self.time_inicio)
        self.pantalla.ponRelojBlancas("%0.2f" % p, None)

    def resign(self):
        self.masJugada(None)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jgSel = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgSel:
            return False
        self.masJugada(jgSel)

    def masJugada(self, jgSel):
        self.pantalla.paraReloj()
        tm = time.time() - self.time_inicio
        self.linea_bloque.time = tm
        score = self.calc_puntuacion(tm)

        resp = jgSel.movimiento() if jgSel else "a1a1"
        bm = self.linea_bloque.bm
        ok = bm == resp
        self.linea_bloque.score = score if ok else 0
        self.pantalla.ponRelojBlancas("%0.2f" % self.linea_bloque.score, None)

        self.singularMoves.add_bloque_sol(self.linea_bloque)
        self.pantalla.ponRelojNegras(self.singularMoves.rotulo_media(), None)

        if jgSel:
            self.movimientosPiezas(jgSel.liMovs)

            self.partida.append_jg(jgSel)
            self.ponFlechaSC(jgSel.desde, jgSel.hasta)
            if not ok:
                self.tablero.creaFlechaTmp(bm[:2], bm[2:], False)
            self.beepExtendido(True)

        else:
            self.ponFlechaSC(bm[:2], bm[2:])

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)

        self.pos_bloque += 1
        li = [k_mainmenu, k_configurar, k_utilidades]
        if self.pos_bloque < 10:
            li.append(k_siguiente)
        else:
            self.singularMoves.graba()
        self.pantalla.ponToolBar(li)

        self.estado = kFinJuego

        self.ponPosicionDGT()

        self.refresh()

    def actualPGN(self):
        resp = '[Event "%s"]\n' % _("Challenge 101")
        resp += '[FEN "%s"\n' % self.partida.iniPosicion.fen()

        resp += "\n" + self.partida.pgnBase()

        return resp

