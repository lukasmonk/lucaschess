# -*- coding: latin-1 -*-

from PyQt4 import QtCore

from Code.Constantes import *
import Code.VarGen as VarGen
import Code.Util as Util
import Code.Apertura as Apertura
import Code.ControlPosicion as ControlPosicion
import Code.Partida as Partida
import Code.Jugada as Jugada
import Code.MotorInternoGM as MotorInternoGM
import Code.Tutor as Tutor
import Code.XMotorRespuesta as XMotorRespuesta
import Code.Books as Books
import Code.MotorInterno as MotorInterno
import Code.DGT as DGT
import Code.Gestor as Gestor
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.QTVarios as QTVarios
import Code.QT.Iconos as Iconos
import Code.QT.PantallaEntMaq as PantallaEntMaq
import Code.QT.PantallaBooks as PantallaBooks
import Code.QT.Motores as Motores

class GestorEntMaq(Gestor.Gestor):
    def inicio(self, dic, aplazamiento=None):
        if aplazamiento:
            dic = aplazamiento["EMDIC"]
        self.reinicio = dic

        self.tipoJuego = kJugEntMaq

        self.resultado = None
        self.siJuegaHumano = False
        self.siJuegaPorMi = True
        self.estado = kJugando
        self.siAnalizando = False
        self.timekeeper = Util.Timekeeper()

        self.summary = {} # numJugada : "a"ccepted, "s"ame, "r"ejected, dif points, time used
        self.siSummary = dic.get("SUMMARY", False)

        siBlancas = dic["SIBLANCAS"]
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.cm = dic["RIVAL"].get("CM", None)

        self.siAtras = dic["ATRAS"]

        self.rmRival = None
        self.liRMrival = []
        self.noMolestar = 0
        self.resignPTS = -99999  # never

        self.siAperturaStd = None
        self.apertura = None
        self.siApertura = False

        self.fen = dic["FEN"]
        if self.fen:
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.fen)
            self.partida.reset(cp)
            self.partida.pendienteApertura = False
            if "MOVE" in dic:
                self.partida.leerPV(dic["MOVE"])
        else:
            self.siApertura = True

        self.pensando(True)
        if self.siApertura:
            if dic["APERTURA"]:
                self.apertura = Apertura.JuegaApertura(dic["APERTURA"].a1h8)
                self.siAperturaStd = True
            else:
                self.apertura = Apertura.AperturaPol(100)  # lee las aperturas

        self.book = dic.get("BOOK", None)
        elo = getattr(self.cm, "elo", 0)
        self.maxMoveBook = elo/200 if 0 <= elo <= 1700 else 9999
        if self.book:
            self.book.polyglot()
            self.bookRR = dic.get("BOOKRR", "mp")
            self.bookMandatory = dic.get("BOOKMANDATORY", False)
            self.siApertura = True
        elif dic["RIVAL"].get("TIPO", None) in (Motores.MICGM, Motores.MICPER):
            if self.cm.book:
                self.book = Books.Libro("P", self.cm.book, self.cm.book, True)
                self.book.polyglot()
                self.bookRR = "mp"
                self.bookMandatory = None
                self.maxMoveBook = 0

        self.siTutorActivado = (VarGen.dgtDispatch is None) and self.configuracion.tutorActivoPorDefecto
        self.pantalla.ponActivarTutor(self.siTutorActivado)

        self.ayudas = dic["AYUDAS"]
        self.ayudasPGN = self.ayudas  # Se guarda para guardar el PGN
        self.nArrows = dic.get("ARROWS", 0)
        nBoxHeight = dic.get("BOXHEIGHT", 24)
        self.thoughtOp = dic.get("THOUGHTOP", -1)
        self.thoughtTt = dic.get("THOUGHTTT", -1)
        self.continueTt = dic.get("CONTINUETT", False)
        self.chance = dic.get("2CHANCE", True)

        mx = max(self.thoughtOp, self.thoughtTt)
        if mx > -1:
            self.alturaRotulo3(nBoxHeight)

        self.nAjustarFuerza = dic.get("AJUSTAR", kAjustarMejor)
        if self.nAjustarFuerza == kAjustarPlayer:
            self.siApertura = False

        dr = dic["RIVAL"]
        motor = dr["MOTOR"]
        self.siRivalInterno = type(motor) == type(1)
        if self.siRivalInterno:
            self.xrival = MotorInternoGM.GestorMotor(motor, self)
            self.siApertura = dic["SIAPERTURA"]

        else:
            rival = dr["CM"]
            r_t = dr["TIEMPO"] * 100  # Se guarda en décimas -> milésimas
            r_p = dr["PROFUNDIDAD"]
            if r_t <= 0:
                r_t = None
            if r_p <= 0:
                r_p = None
            if r_t is None and r_p is None and not dic["SITIEMPO"]:
                r_t = 1000
            self.xrival = self.procesador.creaGestorMotor(rival, r_t, r_p, self.nAjustarFuerza != kAjustarMejor)
            if self.nAjustarFuerza != kAjustarMejor:
                self.xrival.maximizaMultiPV()
            self.resignPTS = dr["RESIGN"]

            self.xrival.ponGuiDispatch(self.guiDispatch, whoDispatch=False)

        self.xtutor.ponGuiDispatch(self.guiDispatch, whoDispatch=True)

        self.xrival.siBlancas = self.siRivalConBlancas

        self.siPrimeraJugadaHecha = False

        self.siTiempo = dic["SITIEMPO"]
        if self.siTiempo:
            self.maxSegundos = dic["MINUTOS"] * 60.0
            self.segundosJugada = dic["SEGUNDOS"]
            self.segExtra = dic.get("MINEXTRA", 0) * 60.0
            self.zeitnot = dic.get("ZEITNOT", 0)

            self.tiempo = {}
            self.tiempo[True] = Util.Timer(self.maxSegundos)
            self.tiempo[False] = Util.Timer(self.maxSegundos)
            if self.segExtra:
                self.tiempo[self.siJugamosConBlancas].ponSegExtra(self.segExtra)

        self.pensando(False)

        # -Aplazamiento 1/2--------------------------------------------------
        if aplazamiento:
            self.partida.recuperaDeTexto(aplazamiento["JUGADAS"])
            self.siApertura = False  # Si no no arranca correctamente, no tiene sentido que se aplace en la apertura
            self.partida.pendienteApertura = aplazamiento["PENDIENTEAPERTURA"]
            self.partida.apertura = None if aplazamiento["APERTURA"] is None else self.listaAperturasStd.dic[
                aplazamiento["APERTURA"]]

            self.siTutorActivado = aplazamiento["SITUTOR"]
            self.pantalla.ponActivarTutor(self.siTutorActivado)
            self.ayudas = aplazamiento["AYUDAS"]
            self.summary = aplazamiento["SUMMARY"]

            self.siTiempo = aplazamiento["SITIEMPO"]
            if self.siTiempo:
                self.maxSegundos = aplazamiento["MAXSEGUNDOS"]
                self.segundosJugada = aplazamiento["SEGUNDOSJUGADA"]
                self.segExtra = aplazamiento.get("SEGEXTRA", 0)

                self.tiempo = {}
                self.tiempo[True] = Util.Timer(aplazamiento["TIEMPOBLANCAS"])
                self.tiempo[False] = Util.Timer(aplazamiento["TIEMPONEGRAS"])
                if self.segExtra:
                    self.tiempo[self.siJugamosConBlancas].ponSegExtra(self.segExtra)

                self.siPrimeraJugadaHecha = False

        if self.siAtras:
            self.pantalla.ponToolBar((
                k_cancelar, k_rendirse, k_tablas, k_atras, k_ayudaMover, k_reiniciar, k_aplazar, k_configurar,
                k_utilidades ))
        else:
            self.pantalla.ponToolBar(
                ( k_cancelar, k_rendirse, k_tablas, k_ayudaMover, k_reiniciar, k_aplazar, k_configurar, k_utilidades ))
        self.pantalla.mostrarOpcionToolbar(k_tablas, not self.siRivalInterno)

        self.pantalla.activaJuego(True, self.siTiempo)

        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        if self.ayudasPGN:
            self.ponAyudasEM()
        else:
            self.quitaAyudas(siQuitarAtras=not self.siAtras)
        self.ponPiezasAbajo(siBlancas)

        if self.siRivalInterno:
            self.pantalla.base.lbRotulo1.ponImagen(self.xrival.imagen)
        else:
            self.ponRotuloBasico()
        if self.ayudasPGN:
            self.ponRotulo2(_("Tutor") + ": <b>" + self.xtutor.nombre)
        else:
            self.ponRotulo2("")

        self.ponCapInfoPorDefecto()

        self.pgnRefresh(True)

        #-Aplazamiento 2/2--------------------------------------------------
        if aplazamiento or "MOVE" in dic:
            self.mueveJugada(kMoverFinal)

        if self.siTiempo:
            self.siPrimeraJugadaHecha = False
            tpBL = self.tiempo[True].etiqueta()
            tpNG = self.tiempo[False].etiqueta()
            rival = self.xrival.nombre
            jugador = self.configuracion.jugador
            bl, ng = jugador, rival
            if self.siRivalConBlancas:
                bl, ng = ng, bl
            self.pantalla.ponDatosReloj(bl, tpBL, ng, tpNG)
            self.refresh()

        self.siAnalizadoTutor = False

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponRotuloBasico(self):
        rotulo1 = "%s: <b>%s</b>" % (_("Opponent"), self.xrival.nombre)
        if self.book:
            rotulo1 += "<br>%s: <b>%s</b>" % (_("Book"), self.book.nombre)
        self.ponRotulo1(rotulo1)

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
            self.tiempo[siUsuario == self.siJugamosConBlancas].setZeitnot(self.zeitnot)  # Cada vez que se active
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

        elif clave == k_tablas:
            self.tablasPlayer()

        elif clave == k_atras:
            self.atras()

        elif clave == k_ayudaMover:
            self.ayudaMover(999)

        elif clave == k_reiniciar:
            self.reiniciar(True)

        elif clave == k_configurar:
            liMasOpciones = []
            if self.estado == kJugando and not self.siRivalInterno:
                liMasOpciones.append((None, None, None))
                liMasOpciones.append(( "rival", _("Change opponent"), Iconos.Motor() ))
            resp = self.configurar(liMasOpciones, siSonidos=True, siCambioTutor=self.ayudasPGN > 0)
            if resp == "rival":
                self.cambioRival()

        elif clave == k_utilidades:
            liMasOpciones = []
            if self.siJuegaHumano or self.siTerminada():
                liMasOpciones.append(( "libros", _("Consult a book"), Iconos.Libros() ))
                liMasOpciones.append(( None, None, None ))
                liMasOpciones.append(( "bookguide", _("Personal Opening Guide"), Iconos.BookGuide() ))
                liMasOpciones.append(( None, None, None ))
                liMasOpciones.append(( "play", _('Play current position'), Iconos.MoverJugar() ))

            resp = self.utilidades(liMasOpciones)
            if resp == "libros":
                siEnVivo = self.siJuegaHumano and not self.siTerminada()
                liMovs = self.librosConsulta(siEnVivo)
                if liMovs and siEnVivo:
                    desde, hasta, coronacion = liMovs[-1]
                    self.mueveHumano(desde, hasta, coronacion)
            elif resp == "bookguide":
                self.bookGuide()
            elif resp == "play":
                self.jugarPosicionActual()

        elif clave == k_aplazar:
            self.aplazar()

        else:
            self.rutinaAccionDef(clave)

    def reiniciar(self, siPregunta):
        if siPregunta:
            if not QTUtil2.pregunta(self.pantalla, _("Restart the game?")):
                return

        self.partida.reset()
        if self.siTiempo:
            self.pantalla.paraReloj()
        self.inicio(self.reinicio)

    def aplazar(self):
        if QTUtil2.pregunta(self.pantalla, _("Do you want to adjourn the game?")):

            aplazamiento = {}
            aplazamiento["EMDIC"] = self.reinicio

            aplazamiento["TIPOJUEGO"] = self.tipoJuego
            aplazamiento["SIBLANCAS"] = self.siJugamosConBlancas
            aplazamiento["JUGADAS"] = self.partida.guardaEnTexto()
            aplazamiento["SITUTOR"] = self.siTutorActivado
            aplazamiento["AYUDAS"] = self.ayudas
            aplazamiento["SUMMARY"] = self.summary

            aplazamiento["SIAPERTURA"] = self.siApertura
            aplazamiento["PENDIENTEAPERTURA"] = self.partida.pendienteApertura
            aplazamiento["APERTURA"] = self.partida.apertura.a1h8 if self.partida.apertura else None

            aplazamiento["SITIEMPO"] = self.siTiempo
            if self.siTiempo:
                aplazamiento["MAXSEGUNDOS"] = self.maxSegundos
                aplazamiento["SEGUNDOSJUGADA"] = self.segundosJugada
                aplazamiento["TIEMPOBLANCAS"] = self.tiempo[True].tiempoAplazamiento()
                aplazamiento["TIEMPONEGRAS"] = self.tiempo[False].tiempoAplazamiento()

            self.configuracion.graba(aplazamiento)
            self.pantalla.accept()

    def finalX(self):
        return self.finalizar()

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
            if self.siAnalizando:
                self.siAnalizando = False
                self.xtutor.ac_final(-1)
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
            if self.siAnalizando:
                self.siAnalizando = False
                self.xtutor.ac_final(-1)
            self.pantalla.activaJuego(False, False)
            self.quitaCapturas()
            self.procesador.inicio()

        return False

    def atras(self):
        if self.partida.numJugadas():
            if self.ayudas:
                self.ayudas -= 1
            self.ponAyudasEM()
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.siApertura = False
            if not self.fen:
                self.listaAperturasStd.asignaApertura(self.partida)
            self.ponteAlFinal()
            self.siAnalizadoTutor = False
            self.reOpenBook()
            self.refresh()
            self.siguienteJugada()

    def testBook(self):
        if self.book:
            resp = self.book.miraListaJugadas(self.fenUltimo())
            if not resp:
                self.book = None
                self.ponRotuloBasico()
                self.siApertura = False

    def reOpenBook(self):
        self.book = self.reinicio.get("BOOK", None)
        if self.book:
            self.book.polyglot()
            self.ponRotuloBasico()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.siBlancas()

        if self.partida.numJugadas() > 0:
            jgUltima = self.partida.liJugadas[-1]
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

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        if self.book:
            self.testBook()

        if siRival:
            self.pensando(True)
            self.desactivaTodas()

            self.relojStart(False)

            siEncontrada = False
            if self.siApertura:
                if self.partida.ultPosicion.jugadas >= self.maxMoveBook:
                    self.siApertura = False
                else:
                    if self.siAperturaStd:
                        siEncontrada, desde, hasta, coronacion = self.apertura.juegaMotor(self.fenUltimo())
                    else:
                        if self.book:  # Sin apertura estandar, con apertura y libro
                            siEncontrada, desde, hasta, coronacion = self.eligeJugadaBook()
                            if siEncontrada:
                                self.siApertura = False  # para que no haya problemas de busqueda futura, ya seguimos siempre el libro
                        if not siEncontrada:  # Nada en el libro en su caso buscamos la apertura normal
                            if self.nAjustarFuerza:
                                siEncontrada, desde, hasta, coronacion = self.eligeJugadaBookAjustada()
                            if not siEncontrada and self.apertura:
                                siEncontrada, desde, hasta, coronacion = self.apertura.juegaMotor(self.fenUltimo())
                                if not siEncontrada:
                                    self.siApertura = False

                if not siEncontrada and self.book:  # Sin apertura y con libro
                    siEncontrada, desde, hasta, coronacion = self.eligeJugadaBook()

            if siEncontrada:
                self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
                self.rmRival.desde = desde
                self.rmRival.hasta = hasta
                self.rmRival.coronacion = coronacion

            else:
                self.siApertura = False
                self.timekeeper.start()
                if self.siTiempo:
                    tiempoBlancas = self.tiempo[True].tiempoPendiente
                    tiempoNegras = self.tiempo[False].tiempoPendiente

                    self.rmRival = self.xrival.juegaTiempo(tiempoBlancas, tiempoNegras, self.segundosJugada,
                                                           nAjustado=self.nAjustarFuerza)
                    if self.estado == kFinJuego:
                        return
                    if self.nAjustarFuerza == kAjustarPlayer:
                        self.rmRival = self.ajustaPlayer(self.rmRival)

                else:
                    if self.siRivalInterno:
                        self.rmRival = self.xrival.juega()
                    else:
                        self.rmRival = self.xrival.juegaPartida(self.partida, nAjustado=self.nAjustarFuerza)
                        if self.nAjustarFuerza == kAjustarPlayer:
                            self.rmRival = self.ajustaPlayer(self.rmRival)
                self.setSummary( "TIMERIVAL", self.timekeeper.stop() )

                self.liRMrival.append(self.rmRival)
                if not self.valoraRMrival():  # def valoraRMrival specific, no es necesario pasar self.siRivalConBlancas
                    self.relojStop(False)
                    self.pensando(False)
                    return

            self.relojStop(False)

            self.pensando(False)
            if self.estado != kFinJuego:
                resp = self.mueveRival(self.rmRival)

                if resp:
                    self.siguienteJugada()

        else:
            self.siJuegaHumano = True
            self.analizaTutorInicio()

            self.relojStart(True)
            self.timekeeper.start()
            self.activaColor(siBlancas)

    def setSummary(self, key, value):
        njug = self.partida.numJugadas()
        if njug not in self.summary:
            self.summary[njug] = {}
        self.summary[njug][key] = value

    def analizaTutorInicio(self):
        self.siAnalizando = False
        self.siAnalizadoTutor = False
        if self.siApertura or not self.siTutorActivado or self.ayudasPGN <= 0:
            return
        if self.continueTt:
            if not self.siTerminada():
                self.xtutor.ac_inicio(self.partida)
                self.siAnalizando = True
                QtCore.QTimer.singleShot(200, self.analizaSiguiente)
        else:
            self.analizaTutor()
            self.siAnalizadoTutor = True

    def analizaSiguiente(self):
        if self.siAnalizando:
            if self.siJuegaHumano and self.estado == kJugando:
                if not self.siTutorActivado:
                    self.xtutor.ac_final(-1)
                else:
                    mrm = self.xtutor.ac_estado()
                    if mrm:
                        rm = mrm.mejorMov()
                        rm.whoDispatch = True
                        self.guiDispatch(rm)
                        QtCore.QTimer.singleShot(1000, self.analizaSiguiente)

    def analizaTutorFinal(self):
        estado = self.siAnalizando
        self.siAnalizando = False
        if self.siAnalizadoTutor or not self.siTutorActivado or self.ayudasPGN <= 0:
            return
        if self.continueTt and estado:
            self.pensando(True)
            self.mrmTutor = self.xtutor.ac_final(self.xtutor.motorTiempoJugada)
            self.pensando(False)
        else:
            self.mrmTutor = self.analizaTutor()

    def ajustaPlayer(self, mrm):
        posicion = self.partida.ultPosicion

        mi = MotorInterno.MotorInterno()
        mi.ponFen(posicion.fen())

        li = mi.dameMovimientos()

        liOpciones = []
        for rm in mrm.liMultiPV:
            liOpciones.append((rm, "%s (%s)" % (posicion.pgnSP(rm.desde, rm.hasta, rm.coronacion), rm.abrTexto()) ))
            mv = rm.movimiento()
            for x in range(len(li)):
                if li[x].pv() == mv:
                    del li[x]
                    break

        for mj in li:
            rm = XMotorRespuesta.RespuestaMotor("", posicion.siBlancas)
            pv = mj.pv()
            rm.desde = pv[:2]
            rm.hasta = pv[2:4]
            rm.coronacion = pv[4:]
            rm.puntos = None
            liOpciones.append((rm, posicion.pgnSP(rm.desde, rm.hasta, rm.coronacion)))

        if len(liOpciones) == 1:
            return liOpciones[0][0]

        menu = QTVarios.LCMenu(self.pantalla)
        titulo = _("White") if posicion.siBlancas else _("Black")
        icono = Iconos.Carpeta()

        self.pantalla.cursorFueraTablero()
        menu.opcion(None, titulo, icono)
        menu.separador()
        icono = Iconos.PuntoNaranja() if posicion.siBlancas else Iconos.PuntoNegro()
        for rm, txt in liOpciones:
            menu.opcion(rm, txt, icono)
        while True:
            resp = menu.lanza()
            if resp:
                return resp

    def valoraRMrival(self):
        if self.siRivalInterno or self.resignPTS < -1500:  # then not ask for draw
            return True
        return Gestor.Gestor.valoraRMrival(self, self.siRivalConBlancas)

    def eligeJugadaBookBase(self, book, bookRR):
        jugadas = self.partida.ultPosicion.jugadas
        if self.maxMoveBook:
            if self.maxMoveBook <= jugadas:
                return False, None, None, None
        fen = self.fenUltimo()

        if bookRR == "su":
            listaJugadas = book.miraListaJugadas(fen)
            if listaJugadas:
                resp = PantallaBooks.eligeJugadaBooks(self.pantalla, listaJugadas, self.partida.ultPosicion.siBlancas)
                return True, resp[0], resp[1], resp[2]
        else:
            pv = book.eligeJugadaTipo(fen, bookRR)
            if pv:
                return True, pv[:2], pv[2:4], pv[4:]

        return False, None, None, None

    def eligeJugadaBook(self):
        return self.eligeJugadaBookBase(self.book, self.bookRR)

    def eligeJugadaBookAjustada(self):
        if self.nAjustarFuerza < 1000:
            return False, None, None, None
        dicPersonalidad = self.configuracion.liPersonalidades[self.nAjustarFuerza - 1000]
        nombook = dicPersonalidad.get("BOOK", None)
        if (nombook is None) or (not Util.existeFichero(nombook)):
            return False, None, None, None

        book = Books.Libro("P", nombook, nombook, True)
        book.polyglot()
        return self.eligeJugadaBookBase(book, "pr")

    def mueveHumano(self, desde, hasta, coronacion=None):

        if self.siJuegaHumano:
            self.paraHumano()
        else:
            self.sigueHumano()
            return False

        movimiento = desde + hasta

        # Peón coronando
        if not coronacion and self.partida.ultPosicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.partida.ultPosicion.siBlancas)
            if coronacion is None:
                self.sigueHumano()
                return False
        if coronacion:
            movimiento += coronacion

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)

        siAnalisis = False

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        if siBien:
            siElegido = False

            if self.book and self.bookMandatory:
                fen = self.fenUltimo()
                listaJugadas = self.book.miraListaJugadas(fen)
                if listaJugadas:
                    li = []
                    for apdesde, aphasta, apcoronacion, nada, nada1 in listaJugadas:
                        mx = apdesde + aphasta + apcoronacion
                        if mx.strip().lower() == movimiento:
                            siElegido = True
                            break
                        li.append((apdesde, aphasta, False))
                    if not siElegido:
                        self.tablero.ponFlechasTmp(li)
                        self.sigueHumano()
                        return False

            if not siElegido and self.siApertura:
                fenBase = self.fenUltimo()
                if self.siAperturaStd:
                    if self.apertura.compruebaHumano(fenBase, desde, hasta):
                        siElegido = True
                    else:
                        if self.apertura.activa:
                            apdesde, aphasta = self.apertura.desdeHastaActual(fenBase)

                            self.tablero.ponFlechasTmp(((apdesde, aphasta, False),))
                            self.sigueHumano()
                            return False
                if not siElegido and self.apertura and self.apertura.compruebaHumano(fenBase, desde, hasta):
                    siElegido = True

            if self.siTeclaPanico:
                self.sigueHumano()
                return False

            if not siElegido and self.siTutorActivado:
                self.analizaTutorFinal()
                siAnalisis = True
                pointsBest, pointsUser = self.mrmTutor.difPointsBest(movimiento)
                self.setSummary("POINTSBEST", pointsBest)
                self.setSummary("POINTSUSER", pointsUser)
                if (pointsBest-pointsUser)>0:
                    if not jg.siJaqueMate:
                        siTutor = True
                        if self.chance:
                            num = self.mrmTutor.numMejorMovQue(movimiento)
                            if num:
                                rmTutor = self.mrmTutor.rmBest()
                                rmUser, n = self.mrmTutor.buscaRM(movimiento)
                                menu = QTVarios.LCMenu(self.pantalla)
                                submenu = menu.submenu(_("There are %d best moves") % num, Iconos.Motor())
                                submenu.opcion("tutor", "%s (%s)" % (_("Show tutor"), rmTutor.abrTextoBase()), Iconos.Tutor())
                                submenu.separador()
                                submenu.opcion("try", _("Try again"), Iconos.Atras())
                                submenu.separador()
                                submenu.opcion("user", "%s (%s)" % (_("Select my move"), rmUser.abrTextoBase()),
                                               Iconos.Player())
                                self.pantalla.cursorFueraTablero()
                                resp = menu.lanza()
                                if resp == "try":
                                    self.sigueHumano()
                                    return False
                                elif resp == "user":
                                    siTutor = False
                        if siTutor:
                            tutor = Tutor.Tutor(self, self, jg, desde, hasta, False)

                            if self.siApertura:
                                liApPosibles = self.listaAperturasStd.listaAperturasPosibles(self.partida)
                            else:
                                liApPosibles = None

                            if tutor.elegir(self.ayudas > 0, liApPosibles=liApPosibles):
                                if self.ayudas > 0:  # doble entrada a tutor.
                                    self.reponPieza(desde)
                                    self.ayudas -= 1
                                    desde = tutor.desde
                                    hasta = tutor.hasta
                                    coronacion = tutor.coronacion
                                    siBien, mens, jgTutor = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta,
                                                                              coronacion)
                                    if siBien:
                                        jg = jgTutor
                                        self.setSummary("SELECTTUTOR", True)
                            if self.configuracion.guardarVariantesTutor:
                                tutor.ponVariantes(jg, 1 + self.partida.numJugadas() / 2)

                            del tutor

            if self.siTeclaPanico:
                self.sigueHumano()
                return False

            self.setSummary( "TIMEUSER", self.timekeeper.stop() )
            self.relojStop(True)

            if self.siApertura and self.siAperturaStd:
                self.siApertura = self.apertura.activa

            self.movimientosPiezas(jg.liMovs)

            if siAnalisis:
                rm, nPos = self.mrmTutor.buscaRM(jg.movimiento())
                if rm:
                    jg.analisis = self.mrmTutor, nPos

            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, True)
            self.error = ""
            self.siguienteJugada()
            return True
        else:
            self.error = mens
            self.sigueHumano()
            return False

    def masJugada(self, jg, siNuestra):
        if not self.siPrimeraJugadaHecha:
            self.siPrimeraJugadaHecha = True

        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.liJugadas.append(jg)
        if self.partida.pendienteApertura:
            self.listaAperturasStd.asignaApertura(self.partida)

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

        self.ponAyudasEM()

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)

        self.ponPosicionDGT()

        self.refresh()

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

    def saveSummary(self):
        if not self.siSummary or not self.summary:
            return

        j_num = 0
        j_same = 0
        st_accept = 0
        st_reject = 0
        nt_accept = 0
        nt_reject = 0

        time_user = 0.0
        ntime_user = 0
        time_rival = 0.0
        ntime_rival = 0

        for njg, d in self.summary.iteritems():
            if "POINTSBEST" in d:
                j_num += 1
                p = d["POINTSBEST"]-d["POINTSUSER"]
                if p:
                    if d.get("SELECTTUTOR", False):
                        st_accept += p
                        nt_accept += 1
                    else:
                        st_reject += p
                        nt_reject += 1
                else:
                    j_same += 1
            if "TIMERIVAL" in d:
                ntime_rival += 1
                time_rival += d["TIMERIVAL"]
            if "TIMEUSER" in d:
                ntime_user += 1
                time_user += d["TIMEUSER"]

        comment = self.partida.firstComment
        if comment:
            comment += "\n"

        if j_num:
            comment += _("Tutor")+": %s\n"%self.xtutor.nombre
            comment += _("Number of moves")+":%d\n"%j_num
            comment += _("Same move")+":%d (%0.2f%%)\n"%(j_same, j_same*1.0/j_num)
            comment += _("Accepted")+":%d (%0.2f%%) %s: %0.2f\n"%(nt_accept, nt_accept*1.0/j_num, _("Average points"), st_accept*1.0/nt_accept if nt_accept else 0.0)
            comment += _("Rejected")+":%d (%0.2f%%) %s: %0.2f\n"%(nt_reject, nt_reject*1.0/j_num, _("Average points"), st_reject*1.0/nt_reject if nt_reject else 0.0)

        if ntime_user or ntime_rival:
            comment += _("Average time (seconds)")+":\n"
            if ntime_user:
                comment += "%s: %0.2f\n"%(self.configuracion.jugador, time_user/ntime_user)
            if ntime_rival:
                comment += "%s: %0.2f\n"%(self.xrival.nombre, time_rival/ntime_rival)

        self.partida.firstComment = comment

    def ponResultado(self, quien):
        self.estado = kFinJuego
        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.beepResultado(quien)

        self.saveSummary()

        nombreContrario = self.xrival.nombre

        mensaje = _("End Game")
        if quien == kGanamos:
            mensaje = _X(_("Congratulations you have won against %1."), nombreContrario)

        elif quien == kGanaRival:
            mensaje = _X(_("Unfortunately you have lost against %1"), nombreContrario)

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
            mensaje = _X(_("Draw, not enough material to mate %1"), nombreContrario)
            self.resultado = kTablas

        elif quien == kGanamosTiempo:
            mensaje = _X(_("Congratulations, you win %1 on time."), nombreContrario)
            self.resultado = kGanamos
            self.partida.liJugadas[-1].comentario = _X(_("%1 has won on time."), self.configuracion.jugador)

        elif quien == kGanaRivalTiempo:
            mensaje = _X(_("%1 has won on time."), nombreContrario)
            self.resultado = kGanaRival
            self.partida.liJugadas[-1].comentario = _X(_("%1 has won on time."), nombreContrario)

        self.guardarGanados(quien == kGanamos)
        if quien != kGanaRivalTiempo:  # Ya que el mensaje ya se ha usado, para añadir minutos
            QTUtil2.mensaje(self.pantalla, mensaje)
        if QTUtil2.pregunta(self.pantalla, _("Do you want to play again?")):
            self.reiniciar(False)
        else:
            self.ponFinJuego()

    def ponFinJuego(self):
        self.pensando(False)
        if self.siTiempo:
            self.pantalla.paraReloj()
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.ac_final(-1)
        self.quitaRotulo3()
        Gestor.Gestor.ponFinJuego(self)

    def ponAyudasEM(self):
        self.ponAyudas(self.ayudas, siQuitarAtras=not self.siAtras)

    def cambioRival(self):
        dic = PantallaEntMaq.cambioRival(self.pantalla, self.configuracion, self.reinicio)

        if dic:
            Util.guardaDIC(dic, self.configuracion.ficheroEntMaquina)
            for k, v in dic.iteritems():
                self.reinicio[k] = v

            siBlancas = dic["SIBLANCAS"]

            self.siAtras = dic["ATRAS"]
            if self.siAtras:
                self.pantalla.ponToolBar(
                    ( k_mainmenu, k_rendirse, k_atras, k_reiniciar, k_aplazar, k_configurar, k_utilidades ))
            else:
                self.pantalla.ponToolBar(( k_mainmenu, k_rendirse, k_reiniciar, k_aplazar, k_configurar, k_utilidades ))

            self.rmRival = None

            self.nAjustarFuerza = dic["AJUSTAR"]

            dr = dic["RIVAL"]
            rival = dr["CM"]
            r_t = dr["TIEMPO"] * 100  # Se guarda en décimas -> milésimas
            r_p = dr["PROFUNDIDAD"]
            if r_t <= 0:
                r_t = None
            if r_p <= 0:
                r_p = None
            if r_t is None and r_p is None and not dic["SITIEMPO"]:
                r_t = 1000

            dr["RESIGN"] = self.resignPTS
            self.xrival = self.procesador.creaGestorMotor(rival, r_t, r_p, self.nAjustarFuerza != kAjustarMejor)

            self.xrival.siBlancas = not siBlancas

            rival = self.xrival.nombre
            jugador = self.configuracion.jugador
            bl, ng = jugador, rival
            if not siBlancas:
                bl, ng = ng, bl
            self.pantalla.cambiaRotulosReloj(bl, ng)

            # self.ponPiezasAbajo( siBlancas )
            self.ponRotuloBasico()

            self.ponPiezasAbajo(siBlancas)
            if siBlancas != self.siJugamosConBlancas:
                self.siJugamosConBlancas = siBlancas
                self.siRivalConBlancas = not siBlancas

                self.siguienteJugada()

    def guiDispatch(self, rm):
        if self.siJuegaHumano:
            if not rm.whoDispatch: # juega humano y el rm es del rival
                return True
            tp = self.thoughtTt
        else:
            if rm.whoDispatch: # juega rival y el rm es del humano
                return True
            tp = self.thoughtOp
            if tp > -1 and self.nArrows:
                self.showPV(rm.pv, self.nArrows)
        if tp > -1:

            if rm.time or rm.depth:
                colorEngine = "DarkBlue" if self.siJuegaHumano else "brown"
                if rm.nodes:
                    nps = "/%d"%rm.nps if rm.nps else ""
                    nodes = " | %d%s"%(rm.nodes, nps)
                else:
                    nodes = ""
                seldepth = "/%d"%rm.seldepth if rm.seldepth else ""
                li = [
                        '<span style="color:%s">%s'%(colorEngine, rm.nombre),
                        '<b>%s</b> | <b>%d</b>%s | <b>%d"</b>%s'%( rm.abrTextoBase(),
                                                                    rm.depth, seldepth,
                                                                    rm.time // 1000,
                                                                    nodes
                                                                    )

                    ]
                if tp:
                    pv = rm.pv
                    if tp < 999:
                        li1 = pv.split(" ")
                        if len(li1) > tp:
                            pv = " ".join(li1[:tp])
                    p = Partida.Partida(self.partida.ultPosicion)
                    p.leerPV(pv)
                    li.append( p.pgnBaseRAW() )
                self.ponRotulo3("<br>".join(li)+"</span>")

        return True

