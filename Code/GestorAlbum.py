from Code import Albums
from Code import Gestor
from Code import Jugada
from Code.QT import QTUtil2
from Code.Constantes import *


class GestorAlbum(Gestor.Gestor):
    def inicio(self, album, cromo, aplazamiento=None):

        if aplazamiento:
            album = aplazamiento["ALBUM"]
            cromo = aplazamiento["CROMO"]
            self.reinicio = aplazamiento
        else:
            self.reinicio = {"ALBUM": album, "CROMO": cromo, "SIBLANCAS":cromo.siBlancas}

        siBlancas = cromo.siBlancas

        self.tipoJuego = kJugAlbum

        self.album = album
        self.cromo = cromo

        self.resultado = None
        self.siJuegaHumano = False
        self.estado = kJugando

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(False)
        self.ayudasPGN = self.ayudas = 0

        # -Aplazamiento 1/2--------------------------------------------------
        if aplazamiento:
            self.partida.recuperaDeTexto(aplazamiento["JUGADAS"])
            self.partida.asignaApertura()

        self.xrival = Albums.GestorMotorAlbum(self, self.cromo)
        self.pantalla.ponToolBar((k_rendirse, k_aplazar, k_configurar, k_utilidades))

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(siBlancas)
        self.quitaAyudas(True, siQuitarAtras=True)
        self.mostrarIndicador(True)

        self.pantalla.base.lbRotulo1.ponImagen(self.cromo.pixmap_level())
        self.pantalla.base.lbRotulo2.ponImagen(self.cromo.pixmap())
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        # -Aplazamiento 2/2--------------------------------------------------
        if aplazamiento:
            self.mueveJugada(kMoverFinal)
            self.siPrimeraJugadaHecha = True
        else:
            self.siPrimeraJugadaHecha = False

        self.ponPosicionDGT()

        self.siguienteJugada()

    def procesarAccion(self, clave):

        if clave in (k_rendirse, k_cancelar):
            self.rendirse()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_aplazar:
            self.aplazar()

        elif clave == k_mainmenu:
            self.procesador.inicio()
            self.procesador.reabrirAlbum(self.album)

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def aplazar(self):
        if self.partida.numJugadas() and QTUtil2.pregunta(self.pantalla, _("Do you want to adjourn the game?")):
            aplazamiento = self.reinicio
            aplazamiento["TIPOJUEGO"] = self.tipoJuego
            aplazamiento["JUGADAS"] = self.partida.guardaEnTexto()

            self.configuracion.graba(aplazamiento)
            self.estado = kFinJuego
            self.pantalla.accept()

    def finalX(self):
        return self.rendirse()

    def rendirse(self):
        if self.estado == kFinJuego:
            return True
        if self.partida.numJugadas() > 0:
            if not QTUtil2.pregunta(self.pantalla, _("Do you want to resign?")):
                return False  # no abandona
            self.partida.abandona(self.siJugamosConBlancas)
            self.ponResultado(kGanaRival)
        else:
            self.procesador.inicio()

        return False

    def siguienteJugada(self):

        if self.estado == kFinJuego:
            return

        # self.ponResultado(kGanamos)
        # self.ponFinJuego( )
        # self.procesador.inicio()
        # self.procesador.reabrirAlbum(self.album)
        # return
        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()
        siBlancas = self.partida.ultPosicion.siBlancas

        numJugadas = self.partida.numJugadas()

        if numJugadas > 0:
            jgUltima = self.partida.last_jg()
            if jgUltima:
                if jgUltima.siJaqueMate:
                    self.ponResultado(kGanaRival if self.siJugamosConBlancas == siBlancas else kGanamos)
                    return
                if jgUltima.siAhogado:
                    self.ponResultado(kTablas)
                    return
                if jgUltima.siTablasRepeticion:
                    self.ponResultado(kTablasRepeticion)
                    return
                if jgUltima.siTablas50:
                    self.ponResultado(kTablas50)
                    return
                if jgUltima.siTablasFaltaMaterial:
                    self.ponResultado(kTablasFaltaMaterial)
                    return

        siRival = siBlancas == self.siRivalConBlancas
        self.ponIndicador(siBlancas)

        self.refresh()

        if siRival:
            self.pensando(True)
            self.desactivaTodas()

            fen = self.fenUltimo()
            rmRival = self.xrival.juega(fen)

            self.pensando(False)
            if self.mueveRival(rmRival):
                self.siguienteJugada()

        else:

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):
        if not self.siPrimeraJugadaHecha:
            self.siPrimeraJugadaHecha = True

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

            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)

            self.error = ""

            return True
        else:
            self.error = mens
            return False

    def ponResultado(self, quien):
        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.estado = kFinJuego

        self.beepResultado(quien)

        nombreContrario = self.cromo.nombre

        mensaje = _("Game ended")
        if quien == kGanamos:
            mensaje = _X(_("Congratulations you have a new sticker %1."), self.cromo.nombre)
            self.cromo.hecho = True
            self.album.guarda()
            if self.album.test_finished():
                mensaje += "\n\n%s" % _("You have finished this album.")
                nuevo = self.album.siguiente
                if nuevo:
                    mensaje += "\n\n%s" % _X(_("Now you can play with album %1"), _F(nuevo))

        elif quien == kGanaRival:
            mensaje = _X(_("Unfortunately you have lost against %1."), nombreContrario)

        elif quien == kTablas:
            mensaje = _X(_("Draw against %1."), nombreContrario)

        elif quien == kTablasRepeticion:
            mensaje = _X(_("Draw due to three times repetition (n. %1) against %2."), self.rotuloTablasRepeticion,
                         nombreContrario)
            self.resultado = kTablas

        elif quien == kTablas50:
            mensaje = _X(_("Draw according to the 50 move rule against %1."), nombreContrario)
            self.resultado = kTablas

        elif quien == kTablasFaltaMaterial:
            mensaje = _X(_("Draw, not enough material to mate %1."), nombreContrario)
            self.resultado = kTablas

        self.guardarGanados(quien == kGanamos)
        self.mensajeEnPGN(mensaje)
        self.ponFinJuego()
        self.xrival.cerrar()
        self.pantalla.ponToolBar((k_mainmenu, k_configurar, k_utilidades))
