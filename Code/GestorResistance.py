from Code import Apertura
from Code import Gestor
from Code import Jugada
from Code.QT import QTUtil2
from Code import Util
from Code import XMotorRespuesta
from Code.Constantes import *


class GestorResistance(Gestor.Gestor):
    def inicio(self, resistance, numEngine, clave):

        self.tipoJuego = kJugBoxing

        self.resistance = resistance
        self.numEngine = numEngine
        self.clave = clave
        siBlancas = "BLANCAS" in clave
        self.segundos, self.puntos, self.maxerror = resistance.actual()
        self.movimientos = 0
        self.puntosRival = 0

        self.siJuegaHumano = False
        self.estado = kJugando

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.siBoxing = True

        self.rmRival = None

        self.siApertura = False
        self.apertura = Apertura.AperturaPol(5)  # lee las aperturas

        # debe hacerse antes que rival
        self.xarbitro = self.procesador.creaGestorMotor(self.configuracion.tutor, self.segundos * 1000, None)
        self.xarbitro.anulaMultiPV()

        motor = resistance.dameClaveEngine(numEngine)
        rival = self.configuracion.buscaRivalExt(motor)
        self.xrival = self.procesador.creaGestorMotor(rival, self.segundos * 1000, None)

        self.pantalla.ponToolBar((k_rendirse, k_reiniciar, k_configurar, k_utilidades))
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(siBlancas)
        self.quitaAyudas()
        self.ponActivarTutor(False)
        self.mostrarIndicador(True)
        self.ponRotuloObjetivo()
        self.ponRotuloActual()
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()
        self.ponPosicionDGT()

        tp = self.resistance.tipo
        if tp:
            b = n = False
            if tp == "p2":
                if siBlancas:
                    b = True
                else:
                    n = True
            elif tp == "p1":
                if siBlancas:
                    n = True
                else:
                    b = True
            self.tablero.mostrarPiezas(b, n)

        self.siguienteJugada()

    def ponRotuloObjetivo(self):
        rotulo = self.resistance.rotuloActual()
        rotulo += "<br><br><b>%s</b>: %s" % (_("Opponent"), self.xrival.nombre)
        rotulo += "<br><b>%s</b>: %s" % (_("Record"), self.resistance.dameEtiRecord(self.clave, self.numEngine))

        self.ponRotulo1(rotulo)

    def ponRotuloActual(self):
        rotulo = "<b>%s</b>: %d" % (_("Moves"), self.movimientos)

        color = "black"
        if self.puntosRival != 0:
            color = "red" if self.puntosRival > 0 else "green"

        rotulo += '<br><b>%s</b>: <font color="%s"><b>%d</b></font>' % (_("Points"), color, -self.puntosRival)

        self.ponRotulo2(rotulo)

    def procesarAccion(self, clave):
        if clave == k_rendirse:
            self.finJuego(False)

        elif clave == k_mainmenu:
            self.procesador.pararMotores()
            self.procesador.inicio()
            self.procesador.entrenamientos.resistance(self.resistance.tipo)

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=False, siBlinfold=False)

        elif clave == k_utilidades:
            self.utilidades(siArbol=self.estado == kFinJuego)

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def reiniciar(self):
        if self.partida.numJugadas() and QTUtil2.pregunta(self.pantalla, _("Restart the game?")):
            self.partida.reset()
            self.inicio(self.resistance, self.numEngine, self.clave)

    def finalX(self):
        return self.finJuego(False)

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.ponRotuloActual()

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()
        siBlancas = self.partida.ultPosicion.siBlancas

        if self.partida.numJugadas() > 0:
            jgUltima = self.partida.last_jg()
            if jgUltima:
                if jgUltima.siJaqueMate:
                    siGanado = self.siJugamosConBlancas != siBlancas
                    if siGanado:
                        self.movimientos += 2001
                    self.finJuego(True)
                    self.guardarGanados(siGanado)
                    return
                if jgUltima.siAhogado or jgUltima.siTablasRepeticion or jgUltima.siTablas50 or jgUltima.siTablasFaltaMaterial:
                    self.movimientos += 1001
                    self.finJuego(True)
                    self.guardarGanados(False)
                    return

        siRival = siBlancas == self.siRivalConBlancas
        self.ponIndicador(siBlancas)

        self.refresh()

        if siRival:
            self.pensando(True)
            self.desactivaTodas()

            siPensar = True

            puntosRivalPrevio = self.puntosRival

            if self.siApertura:
                siBien, desde, hasta, coronacion = self.apertura.juegaMotor(self.fenUltimo())
                if siBien:
                    self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
                    self.rmRival.desde = desde
                    self.rmRival.hasta = hasta
                    self.rmRival.coronacion = coronacion
                    siPensar = False

            if siPensar:
                self.rmRival = self.xrival.juegaSegundos(self.segundos)
                self.puntosRival = self.rmRival.puntosABS()
                self.ponRotuloActual()
            self.pensando(False)

            if self.mueveRival(self.rmRival):
                lostmovepoints = self.puntosRival - puntosRivalPrevio
                if self.siBoxing and self.puntosRival > self.puntos:
                    if self.comprueba():
                        return
                if self.siBoxing and self.maxerror and lostmovepoints > self.maxerror:
                    if self.comprueba():
                        return

                self.siguienteJugada()
        else:

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def comprueba(self):
        self.desactivaTodas()
        if self.xrival.confMotor.clave != self.xarbitro.confMotor.clave:
            if self.segundos > 10:
                sc = 10
            elif self.segundos < 3:
                sc = 3
            else:
                sc = self.segundos

            um = QTUtil2.mensEspera.inicio(self.pantalla, _("Checking..."))

            rm = self.xarbitro.juegaSegundos(sc)
            um.final()
            previoRival = self.puntosRival
            self.puntosRival = -rm.puntosABS()
            self.ponRotuloActual()
            if self.maxerror:
                lostmovepoints = self.puntosRival - previoRival
                if lostmovepoints > self.maxerror:
                    self.movimientos -= 1
                    return self.finJuego(False)

        if self.puntosRival > self.puntos:
            self.movimientos -= 1
            return self.finJuego(False)

        return False

    def finJuego(self, siFinPartida):
        if self.siBoxing and self.movimientos:
            siRecord = self.resistance.ponResultado(self.numEngine, self.clave, self.movimientos)
            if siRecord:
                txt = "<h2>%s<h2>" % (_("New record!"))
                txt += "<h3>%s<h3>" % (self.resistance.dameEtiRecord(self.clave, self.numEngine))
                self.ponRotuloObjetivo()
            else:
                if siFinPartida:
                    txt = "<h2>%s<h2>" % (_("Game ended"))
                    txt += "<h3>%s<h3>" % (self.resistance.dameEti(Util.hoy(), self.movimientos))
                else:
                    txt = "<h3>%s</h3>" % (_X(_("You have lost %1 points."), str(-self.puntosRival)))

            if siFinPartida:
                self.mensajeEnPGN(txt)
            else:
                resp = QTUtil2.pregunta(self.pantalla,
                                        txt + "<br>%s" % (_("Do you want to resign or continue playing?")),
                                        etiSi=_("Resign"), etiNo=_("Continue"))
                if not resp:
                    self.siBoxing = False
                    return False

        self.desactivaTodas()
        self.estado = kFinJuego
        self.procesador.pararMotores()
        self.xarbitro.terminar()
        self.pantalla.ajustaTam()
        self.pantalla.resize(0, 0)
        if self.movimientos >= 1:
            liOpciones = [k_mainmenu, k_configurar, k_utilidades]
            self.pantalla.ponToolBar(liOpciones)
        else:
            self.procesarAccion(k_mainmenu)

        return True

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        if self.siApertura:
            self.apertura.compruebaHumano(self.fenUltimo(), desde, hasta)

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.movimientos += 1
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):
        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        resp = self.partida.si3repetidas()
        if resp:
            jg.siTablasRepeticion = True
            rotulo = ""
            for j in resp:
                rotulo += "%d," % (j / 2 + 1,)
            rotulo = rotulo.strip(",")
            self.rotuloTablasRepeticion = rotulo

        if self.partida.ultPosicion.movPeonCap >= 100:
            jg.siTablas50 = True

        if self.partida.ultPosicion.siFaltaMaterial():
            jg.siTablasFaltaMaterial = True

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

    def mueveRival(self, respMotor):
        desde = respMotor.desde
        hasta = respMotor.hasta

        coronacion = respMotor.coronacion

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        if siBien:
            self.partida.ultPosicion = jg.posicion

            self.error = ""
            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)

            return True
        else:
            self.error = mens
            return False
