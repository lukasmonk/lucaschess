from Code import Apertura
from Code import DGT
from Code import GestorEntMaq
from Code import Jugada
from Code.QT import Iconos
from Code.QT import PantallaEntMaq
from Code.QT import QTUtil2
from Code import Util
from Code import VarGen
from Code import XMotorRespuesta
from Code.Constantes import *


class GestorPerson(GestorEntMaq.GestorEntMaq):
    def inicio(self, dic, aplazamiento=None, siPrimeraJugadaHecha=False):
        if aplazamiento:
            dic = aplazamiento["EMDIC"]
        self.reinicio = dic
        self.cache = {}

        self.tipoJuego = kJugEntMaq

        self.resultado = None
        self.siJuegaHumano = False
        self.estado = kJugando
        self.timekeeper = Util.Timekeeper()
        self.siAnalizando = False

        self.summary = {}
        self.siSummary = dic.get("SUMMARY", False)

        siBlancas = dic["SIBLANCAS"]
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.siAtras = True

        self.rmRival = None
        self.liRMrival = []

        self.aperturaStd = Apertura.AperturaPol(1)

        self.pensando(True)

        self.pantalla.ponActivarTutor(False)

        self.ayudas = 0
        self.ayudasPGN = 0

        cmrival = self.configuracion.buscaRival("irina", None)
        self.xrival = self.procesador.creaGestorMotor(cmrival, None, 2)
        self.rival_name = dic["RIVAL"]
        self.xrival.set_option("Personality", self.rival_name)
        if not dic["FASTMOVES"]:
            self.xrival.set_option("Max Time", "5")
            self.xrival.set_option("Min Time", "1")
        self.xrival.nombre = _F(self.rival_name)

        self.xrival.siBlancas = self.siRivalConBlancas

        self.siPrimeraJugadaHecha = siPrimeraJugadaHecha

        self.siTiempo = dic["SITIEMPO"]
        if self.siTiempo:
            self.maxSegundos = dic["MINUTOS"] * 60.0
            self.segundosJugada = dic["SEGUNDOS"]
            self.segExtra = dic.get("MINEXTRA", 0) * 60.0

            self.tiempo = {}
            self.tiempo[True] = Util.Timer(self.maxSegundos)
            self.tiempo[False] = Util.Timer(self.maxSegundos)

        self.pensando(False)

        # -Aplazamiento 1/2--------------------------------------------------
        if aplazamiento:
            self.partida.recuperaDeTexto(aplazamiento["JUGADAS"])
            self.partida.pendienteApertura = aplazamiento["PENDIENTEAPERTURA"]
            self.partida.apertura = None if aplazamiento["APERTURA"] is None else self.listaAperturasStd.dic[
                aplazamiento["APERTURA"]]

            self.siTiempo = aplazamiento["SITIEMPO"]
            if self.siTiempo:
                self.maxSegundos = aplazamiento["MAXSEGUNDOS"]
                self.segundosJugada = aplazamiento["SEGUNDOSJUGADA"]

                self.tiempo = {}
                self.tiempo[True] = Util.Timer(aplazamiento["TIEMPOBLANCAS"])
                self.tiempo[False] = Util.Timer(aplazamiento["TIEMPONEGRAS"])
                if self.segExtra:
                    self.tiempo[self.siJugamosConBlancas].ponSegExtra(self.segExtra)

                self.siPrimeraJugadaHecha = False

        li = [k_cancelar, k_rendirse, k_atras, k_reiniciar, k_aplazar, k_peliculaPausa, k_configurar, k_utilidades]
        self.pantalla.ponToolBar(li)

        self.pantalla.activaJuego(True, self.siTiempo)

        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.quitaAyudas(True, siQuitarAtras=False)
        self.ponPiezasAbajo(siBlancas)

        imagen = getattr(Iconos, "pm%s" % self.rival_name)

        self.pantalla.base.lbRotulo1.ponImagen(imagen())
        self.pantalla.base.lbRotulo1.show()

        self.ponCapInfoPorDefecto()

        self.pgnRefresh(True)

        # -Aplazamiento 2/2--------------------------------------------------
        if aplazamiento or "MOVE" in dic:
            self.mueveJugada(kMoverFinal)

        if self.siTiempo:
            self.siPrimeraJugadaHecha = False
            tpBL = self.tiempo[True].etiqueta()
            tpNG = self.tiempo[False].etiqueta()
            jugador = self.configuracion.jugador
            bl, ng = jugador, self.rival_name
            if self.siRivalConBlancas:
                bl, ng = ng, bl
            self.pantalla.ponDatosReloj(bl, tpBL, ng, tpNG)
            self.refresh()

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponReloj(self):
        if (not self.siTiempo) or \
                (not self.siPrimeraJugadaHecha) or \
                        self.estado != kJugando:
            return

        def mira(siBlancas):
            ot = self.tiempo[siBlancas]

            eti, eti2 = ot.etiquetaDif2()
            if eti:
                if siBlancas:
                    self.pantalla.ponRelojBlancas(eti, eti2)
                else:
                    self.pantalla.ponRelojNegras(eti, eti2)

            siJugador = self.siJugamosConBlancas == siBlancas
            if ot.siAgotado():
                if siJugador and QTUtil2.pregunta(self.pantalla,
                                                  _X(_("%1 has won on time."), self.xrival.nombre) + "\n\n" + _(
                                                          "Add time and keep playing?")):
                    minX = PantallaEntMaq.dameMinutosExtra(self.pantalla)
                    if minX:
                        ot.ponSegExtra(minX * 60)
                        return True
                self.ponResultado(kGanaRivalTiempo if siJugador else kGanamosTiempo)
                return False

            elif siJugador and ot.isZeitnot():
                self.beepZeitnot()

            return True

        if VarGen.dgt:
            DGT.writeClocks(self.tiempo[True].etiquetaDGT(), self.tiempo[False].etiquetaDGT())

        if self.siJuegaHumano:
            siBlancas = self.siJugamosConBlancas
        else:
            siBlancas = not self.siJugamosConBlancas
        mira(siBlancas)

    def relojStart(self, siUsuario):
        if self.siTiempo and self.siPrimeraJugadaHecha:
            self.tiempo[siUsuario == self.siJugamosConBlancas].iniciaMarcador()
            self.pantalla.iniciaReloj(self.ponReloj, transicion=200)

    def relojStop(self, siUsuario):
        if self.siTiempo and self.siPrimeraJugadaHecha:
            self.tiempo[siUsuario == self.siJugamosConBlancas].paraMarcador(self.segundosJugada)
            self.ponReloj()
            self.pantalla.paraReloj()
            self.refresh()

    def procesarAccion(self, clave):
        if clave == k_cancelar:
            self.finalizar()

        elif clave == k_rendirse:
            self.rendirse()

        elif clave == k_atras:
            self.atras()

        elif clave == k_peliculaPausa:
            self.pausa()

        elif clave == k_peliculaSeguir:
            self.seguir()

        elif clave == k_reiniciar:
            self.reiniciar(True)

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            liMasOpciones = []
            if self.siJuegaHumano or self.siTerminada():
                liMasOpciones.append(("libros", _("Consult a book"), Iconos.Libros()))
                liMasOpciones.append((None, None, None))
                liMasOpciones.append(("bookguide", _("Personal Opening Guide"), Iconos.BookGuide()))

            resp = self.utilidades(liMasOpciones)
            if resp == "libros":
                siEnVivo = self.siJuegaHumano and not self.siTerminada()
                liMovs = self.librosConsulta(siEnVivo)
                if liMovs and siEnVivo:
                    desde, hasta, coronacion = liMovs[-1]
                    self.mueveHumano(desde, hasta, coronacion)
            elif resp == "bookguide":
                self.bookGuide()

        elif clave == k_aplazar:
            self.aplazar()

        else:
            self.rutinaAccionDef(clave)

    def genAplazamiento(self):
        aplazamiento = {}
        aplazamiento["EMDIC"] = self.reinicio

        aplazamiento["TIPOJUEGO"] = self.tipoJuego
        aplazamiento["MODO"] = "Personalities"
        aplazamiento["SIBLANCAS"] = self.siJugamosConBlancas
        aplazamiento["JUGADAS"] = self.partida.guardaEnTexto()
        aplazamiento["SITUTOR"] = self.siTutorActivado
        aplazamiento["AYUDAS"] = self.ayudas
        aplazamiento["SUMMARY"] = self.summary

        aplazamiento["SIAPERTURA"] = self.aperturaStd is not None
        aplazamiento["PENDIENTEAPERTURA"] = self.partida.pendienteApertura
        aplazamiento["APERTURA"] = self.partida.apertura.a1h8 if self.partida.apertura else None

        aplazamiento["SITIEMPO"] = self.siTiempo
        if self.siTiempo:
            aplazamiento["MAXSEGUNDOS"] = self.maxSegundos
            aplazamiento["SEGUNDOSJUGADA"] = self.segundosJugada
            aplazamiento["TIEMPOBLANCAS"] = self.tiempo[True].tiempoAplazamiento()
            aplazamiento["TIEMPONEGRAS"] = self.tiempo[False].tiempoAplazamiento()
        return aplazamiento

    def finalizar(self):
        if self.estado == kFinJuego:
            return True
        siJugadas = self.partida.numJugadas() > 0
        if self.siTiempo:
            self.pantalla.paraReloj()
        if siJugadas:
            if not QTUtil2.pregunta(self.pantalla, _("End game?")):
                return False  # no abandona
            self.resultado = kDesconocido
            self.partida.liJugadas[-1].siDesconocido = True
            self.guardarNoTerminados()
            self.ponFinJuego()
        else:
            self.pantalla.activaJuego(False, False)
            self.quitaCapturas()
            self.procesador.inicio()

        return False

    def rendirse(self):
        if self.estado == kFinJuego:
            return True
        siJugadas = self.partida.numJugadas() > 0
        if self.siTiempo:
            self.pantalla.paraReloj()
        if siJugadas:
            if not QTUtil2.pregunta(self.pantalla, _("Do you want to resign?")):
                return False  # no abandona
            self.resultado = kGanaRival
            self.partida.abandona(self.siJugamosConBlancas)
            self.guardarGanados(False)
            self.saveSummary()
            self.ponFinJuego()
        else:
            self.pantalla.activaJuego(False, False)
            self.quitaCapturas()
            self.procesador.inicio()

        return False

    def atras(self):
        if self.partida.numJugadas():
            if self.ayudas:
                self.ayudas -= 1
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            if not self.fen:
                self.partida.asignaApertura()
            self.ponteAlFinal()
            self.refresh()
            self.siguienteJugada()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.siBlancas()

        if self.checkFinal(siBlancas):
            return

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        if siRival:
            if self.juegaRival():
                self.siguienteJugada()

        else:
            self.juegaHumano(siBlancas)

    def setSummary(self, key, value):
        njug = self.partida.numJugadas()
        if njug not in self.summary:
            self.summary[njug] = {}
        self.summary[njug][key] = value

    def juegaHumano(self, siBlancas):
        self.siJuegaHumano = True

        self.relojStart(True)
        self.timekeeper.start()
        self.activaColor(siBlancas)

    def juegaRival(self):
        self.pensando(True)
        self.desactivaTodas()

        self.relojStart(False)

        desde = hasta = coronacion = ""
        siEncontrada = False

        if self.aperturaStd:
            siEncontrada, desde, hasta, coronacion = self.aperturaStd.juegaMotor(self.fenUltimo())
            if not siEncontrada:
                self.aperturaStd = None

        if siEncontrada:
            self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
            self.rmRival.desde = desde
            self.rmRival.hasta = hasta
            self.rmRival.coronacion = coronacion

        else:
            self.timekeeper.start()
            if self.siTiempo:
                tiempoBlancas = self.tiempo[True].tiempoPendiente
                tiempoNegras = self.tiempo[False].tiempoPendiente

                self.rmRival = self.xrival.juegaTiempo(tiempoBlancas, tiempoNegras, self.segundosJugada)
                if self.estado == kFinJuego:
                    return True

            else:
                self.rmRival = self.xrival.juega()
            self.setSummary("TIMERIVAL", self.timekeeper.stop())

        self.relojStop(False)
        self.pensando(False)

        self.liRMrival.append(self.rmRival)

        siBien, self.error, jg = Jugada.dameJugada(self.partida.ultPosicion, self.rmRival.desde, self.rmRival.hasta, self.rmRival.coronacion)
        if siBien:
            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)
            return True
        else:
            return False

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        self.setSummary("TIMEUSER", self.timekeeper.stop())
        self.relojStop(True)

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

        self.ponPosicionDGT()

        self.refresh()

    def pgnLabelsAdded(self):
        # Para que sustituya al de estorEntMaq
        return {}
