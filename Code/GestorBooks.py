import random

from Code import Gestor
from Code import Jugada
from Code.QT import PantallaBooks
from Code import XMotorRespuesta
from Code.Constantes import *


class GestorBooks(Gestor.Gestor):
    def inicio(self, libro, siBlancas, jugContrario, jugJugador):

        self.tipoJuego = kJugBooks

        self.ayudas = 9999  # Para que analice sin problemas

        self.libro = libro
        self.libro.polyglot()

        self.jugContrario = jugContrario
        self.jugJugador = jugJugador
        self.aciertos = 0
        self.movimientos = 0
        self.sumar_aciertos = True

        self.liReiniciar = libro, siBlancas, jugContrario, jugJugador

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_atras, k_ayuda, k_configurar, k_utilidades))
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.quitaAyudas(siQuitarAtras=False)
        self.ponPiezasAbajo(siBlancas)
        self.ponRotulo1(libro.nombre)
        self.ponRotulo2("")
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        self.estado = kJugando

        self.ponPosicionDGT()

        self.quitaInformacion(siActivable=True)

        self.siguienteJugada()

    def procesarAccion(self, clave):

        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_atras:
            self.atras()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_ayuda:
            self.ayuda()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        return self.finPartida()

    def finPartida(self):
        self.procesador.inicio()
        return False

    def reiniciar(self):
        self.partida.reset()
        libro, siBlancas, jugContrario, jugJugador = self.liReiniciar
        self.inicio(libro, siBlancas, jugContrario, jugJugador)

    def siguienteJugada(self):

        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        fen = self.partida.ultPosicion.fen()
        self.listaJugadas = self.libro.miraListaJugadas(fen)

        if not self.listaJugadas:
            self.ponResultado()
            return

        siRival = siBlancas == self.siRivalConBlancas

        if siRival:
            self.desactivaTodas()

            nli = len(self.listaJugadas)
            if nli > 1:
                resp = self.eligeJugadaRival()
            else:
                resp = self.listaJugadas[0][0], self.listaJugadas[0][1], self.listaJugadas[0][2]
            desde, hasta, coronacion = resp

            self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
            self.rmRival.desde = desde
            self.rmRival.hasta = hasta
            self.rmRival.coronacion = coronacion

            self.mueveRival(self.rmRival)
            self.siguienteJugada()

        else:

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def eligeJugadaRival(self):

        if self.jugContrario == "su":
            resp = PantallaBooks.eligeJugadaBooks(self.pantalla, self.listaJugadas, self.partida.ultPosicion.siBlancas)
        elif self.jugContrario == "mp":
            resp = self.listaJugadas[0][0], self.listaJugadas[0][1], self.listaJugadas[0][2]
            nmax = self.listaJugadas[0][4]
            for desde, hasta, coronacion, pgn, peso in self.listaJugadas:
                if peso > nmax:
                    resp = desde, hasta, coronacion
                    nmax = peso
        elif self.jugContrario == "au":
            pos = random.randint(0, len(self.listaJugadas) - 1)
            resp = self.listaJugadas[pos][0], self.listaJugadas[pos][1], self.listaJugadas[pos][2]
        else:
            li = [int(x[4] * 100000) for x in self.listaJugadas]
            t = sum(li)
            num = random.randint(1, t)
            pos = 0
            t = 0
            for n, x in enumerate(li):
                t += x
                if num <= t:
                    pos = n
                    break
            resp = self.listaJugadas[pos][0], self.listaJugadas[pos][1], self.listaJugadas[pos][2]

        return resp

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        siEncontrado = False
        actpeso = 0
        for jdesde, jhasta, jcoronacion, jpgn, peso in self.listaJugadas:
            if desde == jdesde and hasta == jhasta and jg.coronacion == jcoronacion:
                siEncontrado = True
                actpeso = peso
                break

        if siEncontrado and self.jugJugador:  # si el jugador busca elegir el maximo
            maxpeso = 0.0
            for jdesde, jhasta, jcoronacion, jpgn, peso in self.listaJugadas:
                if peso > maxpeso:
                    maxpeso = peso
            if actpeso < maxpeso:
                siEncontrado = False

        if not siEncontrado:
            self.tablero.ponPosicion(self.partida.ultPosicion)

            main = self.listaJugadas[0][4]
            saux = False
            paux = 0

            for n, jug in enumerate(self.listaJugadas):
                opacity = p = jug[4]
                simain = p == main
                if not simain:
                    if not saux:
                        paux = p
                        saux = True
                    opacity = 1.0 if p == paux else max(p, 0.25)
                self.tablero.creaFlechaMulti(jug[0]+jug[1], siMain=simain, opacidad=opacity)

            resp = PantallaBooks.eligeJugadaBooks(self.pantalla, self.listaJugadas, self.siJugamosConBlancas, siSelectSiempre=False)
            self.tablero.quitaFlechas()
            if resp is None:
                self.sumar_aciertos = False
                self.sigueHumano()
                return False

            desde, hasta, coronacion = resp
            siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        else:
            if self.sumar_aciertos:
                self.aciertos += actpeso
        self.movimientos += 1

        self.ponRotulo2(self.txtAciertos())

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.sumar_aciertos = True
        self.siguienteJugada()
        return True

    def ayuda(self):
        if self.siJuegaHumano:
            self.paraHumano()
        else:
            return
        self.tablero.ponPosicion(self.partida.ultPosicion)

        main = self.listaJugadas[0][4]
        saux = False
        paux = 0

        for n, jug in enumerate(self.listaJugadas):
            opacity = p = jug[4]
            simain = p == main
            if not simain:
                if not saux:
                    paux = p
                    saux = True
                opacity = 1.0 if p == paux else max(p, 0.25)
            self.tablero.creaFlechaMulti(jug[0]+jug[1], siMain=simain, opacidad=opacity)

        resp = PantallaBooks.eligeJugadaBooks(self.pantalla, self.listaJugadas, self.siJugamosConBlancas, siSelectSiempre=False)
        self.tablero.quitaFlechas()
        if resp is None:
            self.sumar_aciertos = False
            self.sigueHumano()
            return

        desde, hasta, coronacion = resp
        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        self.movimientos += 1

        self.ponRotulo2(self.txtAciertos())

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.sumar_aciertos = True
        self.siguienteJugada()

    def masJugada(self, jg, siNuestra):

        # Para facilitar el salto a variantes
        jg.aciertos = self.aciertos
        jg.movimientos = self.movimientos
        jg.numpos = self.partida.numJugadas()

        self.ponVariantes(jg)
        # Preguntamos al motor si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

    def atras(self):
        if self.partida.numJugadas():
            self.estado = kJugando
            self.movimientos -= 1
            if self.movimientos < 0:
                self.movimientos = 0
            self.aciertos -= 1
            if self.aciertos < 0:
                self.aciertos = 0
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.ponteAlFinal()
            self.refresh()
            self.siguienteJugada()

    def ponVariantes(self, jg):
        desde = jg.desde
        hasta = jg.hasta
        coronacion = jg.coronacion
        if coronacion is None:
            coronacion = ""

        comentario = ""

        linea = "-" * 24 + "\n"
        for jdesde, jhasta, jcoronacion, jpgn, peso in self.listaJugadas:
            siLineas = desde == jdesde and hasta == jhasta and coronacion == jcoronacion
            if siLineas:
                comentario += linea
            comentario += jpgn + "\n"
            if siLineas:
                comentario += linea

        jg.comentario = comentario

    def pgnInformacion(self):
        posJugada, jg = self.jugadaActiva()
        if jg:
            fen = jg.posicionBase.fen()
            lista = self.libro.miraListaJugadas(fen)

            siEditar = True
            if len(lista) > 0:
                resp = PantallaBooks.saltaJugadaBooks(self, lista, jg)
                if resp is None:
                    return
                siEditar = resp[0] is None
            if siEditar:
                Gestor.Gestor.pgnInformacion(self)
            else:
                # Eliminamos todas las jugadas desde esta hasta el final
                numpos = jg.numpos
                self.partida.liJugadas = self.partida.liJugadas[:numpos]
                if self.partida.numJugadas() == 0:
                    self.aciertos = 0
                    self.movimientos = 0
                    self.partida.ultPosicion = self.partida.iniPosicion
                else:
                    jg = self.partida.last_jg()
                    self.aciertos = jg.aciertos
                    self.movimientos = jg.movimientos
                    self.partida.ultPosicion = jg.posicion
                self.ponRotulo2(self.txtAciertos())
                self.partida.pendienteApertura = True

                # realizamos el movimiento
                desde, hasta, coronacion = resp
                siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
                self.movimientosPiezas(jg.liMovs, False)
                self.partida.ultPosicion = jg.posicion
                self.tablero.ponPosicion(jg.posicion)
                self.pantalla.base.pgnRefresh()
                self.ponRevision(False)

                self.listaJugadas = lista

                self.ponVariantes(jg)

                self.masJugada(jg, True)

                # refrescamos
                self.refresh()

                # siguienteJugada
                self.estado = kJugando
                self.siguienteJugada()

    def mueveRival(self, respMotor):
        desde = respMotor.desde
        hasta = respMotor.hasta

        coronacion = respMotor.coronacion

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        if siBien:
            self.partida.ultPosicion = jg.posicion
            self.ponVariantes(jg)

            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)

            self.error = ""

            return True
        else:
            self.error = mens
            return False

    def txtAciertos(self):
        if self.movimientos:
            return "%s : %d/%d (%0.2f%%)" % (
                _("Hints"), self.aciertos, self.movimientos, 100.0 * self.aciertos / self.movimientos)
        else:
            return ""

    def ponResultado(self):
        self.estado = kFinJuego
        self.tablero.desactivaTodas()

        txt = self.txtAciertos()

        mensaje = "%s\n%s\n" % (_("Line completed"), txt)
        self.mensajeEnPGN(mensaje)
