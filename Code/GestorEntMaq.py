import LCEngine4 as LCEngine
from PyQt4 import QtCore

from Code import Analisis
from Code import Apertura
from Code import Books
from Code import ControlPosicion
from Code import DGT
from Code import Gestor
from Code import Jugada
from Code import Partida
from Code import Personalidades
from Code.QT import Iconos
from Code.QT import Motores
from Code.QT import PantallaBooks
from Code.QT import PantallaEntMaq
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Tutor
from Code import Util
from Code import VarGen
from Code import XMotorRespuesta
from Code.Constantes import *


class GestorEntMaq(Gestor.Gestor):
    def inicio(self, dic, aplazamiento=None, siPrimeraJugadaHecha=False):
        if aplazamiento:
            dic = aplazamiento["EMDIC"]
        self.reinicio = dic

        self.cache = dic.get("cache", {})

        self.tipoJuego = kJugEntMaq

        self.resultado = None
        self.siJuegaHumano = False
        self.siJuegaPorMi = True
        self.estado = kJugando
        self.siAnalizando = False
        self.timekeeper = Util.Timekeeper()

        self.summary = {}  # numJugada : "a"ccepted, "s"ame, "r"ejected, dif points, time used
        self.siSummary = dic.get("SUMMARY", False)

        siBlancas = dic["SIBLANCAS"]
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.cm = dic["RIVAL"].get("CM", None)
        if self.cm:
            if hasattr(self.cm, "icono"):
                delattr(self.cm, "icono") # problem with configuracion.escVariables and saving qt variables

        self.siAtras = dic["ATRAS"]

        self.rmRival = None
        self.liRMrival = []
        self.noMolestar = 0
        self.resignPTS = -99999  # never

        self.aperturaObl = self.aperturaStd = None

        self.fen = dic["FEN"]
        if self.fen:
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.fen)
            self.partida.reset(cp)
            self.partida.pendienteApertura = False
            if "MOVE" in dic:
                self.partida.leerPV(dic["MOVE"])
        else:
            if dic["APERTURA"]:
                self.aperturaObl = Apertura.JuegaApertura(dic["APERTURA"].a1h8)
                self.primeroBook = False  # la apertura es obligatoria

        self.pensando(True)

        self.book = dic.get("BOOK", None)
        elo = getattr(self.cm, "elo", 0)
        self.maxMoveBook = elo / 200 if 0 < elo <= 1700 else 9999
        if self.book:
            self.book.polyglot()
            self.bookRR = dic.get("BOOKRR", "mp")
            self.bookMandatory = dic.get("BOOKMANDATORY", False)
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
        self.nArrowsTt = dic.get("ARROWSTT", 0)
        self.chance = dic.get("2CHANCE", True)

        if self.nArrowsTt and self.ayudas == 0:
            self.nArrowsTt = 0

        self.childmode = self.nArrowsTt > 0 and self.ayudas > 0

        mx = max(self.thoughtOp, self.thoughtTt)
        if mx > -1:
            self.alturaRotulo3(nBoxHeight)

        dr = dic["RIVAL"]
        rival = dr["CM"]

        if dr["TIPO"] == Motores.ELO:
            r_t = 0
            r_p = rival.fixed_depth
            self.nAjustarFuerza = kAjustarMejor

        else:
            r_t = dr["TIEMPO"] * 100  # Se guarda en decimas -> milesimas
            r_p = dr["PROFUNDIDAD"]
            self.nAjustarFuerza = dic.get("AJUSTAR", kAjustarMejor)

        if not self.xrival: # reiniciando is not None
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

        self.siBookAjustarFuerza = self.nAjustarFuerza != kAjustarMejor

        self.xrival.ponGuiDispatch(self.guiDispatch, whoDispatch=False)

        self.xtutor.ponGuiDispatch(self.guiDispatch, whoDispatch=True)

        self.xrival.siBlancas = self.siRivalConBlancas

        self.siPrimeraJugadaHecha = siPrimeraJugadaHecha

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

        li = [k_cancelar, k_rendirse, k_tablas, k_atras, k_ayudaMover, k_reiniciar, k_aplazar, k_peliculaPausa, k_configurar, k_utilidades]
        if not self.siAtras:
            del li[3]
        self.pantalla.ponToolBar(li)
        self.pantalla.mostrarOpcionToolbar(k_tablas, True)

        self.pantalla.activaJuego(True, self.siTiempo)

        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        if self.ayudasPGN:
            self.ponAyudasEM()
        else:
            self.quitaAyudas(siQuitarAtras=not self.siAtras)
        self.ponPiezasAbajo(siBlancas)

        self.ponRotuloBasico()
        if self.ayudasPGN:
            self.ponRotulo2(_("Tutor") + ": <b>" + self.xtutor.nombre)
        else:
            self.ponRotulo2("")

        self.ponCapInfoPorDefecto()

        self.pgnRefresh(True)

        # -Aplazamiento 2/2--------------------------------------------------
        if aplazamiento or "MOVE" in dic:
            self.mueveJugada(kMoverFinal)

        if self.siTiempo:
            self.siPrimeraJugadaHecha = siPrimeraJugadaHecha
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

        if self.childmode:
            self.pantalla.base.btActivarTutor.setVisible(False)

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

        elif clave == k_peliculaPausa:
            self.pausa()

        elif clave == k_peliculaSeguir:
            self.seguir()

        elif clave == k_ayudaMover:
            self.analizaFinal()
            self.ayudaMover(999)

        elif clave == k_reiniciar:
            self.reiniciar(True)

        elif clave == k_configurar:
            liMasOpciones = []
            if self.estado == kJugando:
                liMasOpciones.append((None, None, None))
                liMasOpciones.append(("rival", _("Change opponent"), Iconos.Motor()))
            resp = self.configurar(liMasOpciones, siSonidos=True, siCambioTutor=self.ayudasPGN > 0)
            if resp == "rival":
                self.cambioRival()

        elif clave == k_utilidades:
            liMasOpciones = []
            if self.siJuegaHumano or self.siTerminada():
                liMasOpciones.append(("libros", _("Consult a book"), Iconos.Libros()))
                liMasOpciones.append((None, None, None))
                liMasOpciones.append(("bookguide", _("Personal Opening Guide"), Iconos.BookGuide()))
                liMasOpciones.append((None, None, None))
                liMasOpciones.append(("play", _('Play current position'), Iconos.MoverJugar()))

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
        self.analizaTerminar()
        self.partida.reset()
        if self.siTiempo:
            self.pantalla.paraReloj()
        self.reinicio["cache"] = self.cache
        self.inicio(self.reinicio)

    def genAplazamiento(self):
        aplazamiento = {}
        aplazamiento["EMDIC"] = self.reinicio

        aplazamiento["TIPOJUEGO"] = self.tipoJuego
        aplazamiento["MODO"] = "Basic"

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

    def aplazar(self):
        if QTUtil2.pregunta(self.pantalla, _("Do you want to adjourn the game?")):
            self.configuracion.graba(self.genAplazamiento())
            self.estado = kFinJuego
            self.pantalla.accept()

    def pausa(self):
        self.pausaReg = self.genAplazamiento()
        self.pantalla.ponToolBar((k_peliculaSeguir,))

    def seguir(self):
        siabajo = self.tablero.siBlancasAbajo
        self.inicio(None, self.pausaReg, siPrimeraJugadaHecha=True)
        if siabajo != self.tablero.siBlancasAbajo:
            self.tablero.rotaTablero()

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
            self.analizaTerminar()
            self.pantalla.activaJuego(False, False)
            self.quitaCapturas()
            self.procesador.inicio()

        return False

    def analizaTerminar(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.ac_final(-1)

    def atras(self):
        if self.partida.numJugadas():
            self.analizaTerminar()
            if self.ayudas:
                self.ayudas -= 1
                self.childmode = self.nArrowsTt > 0 and self.ayudas > 0
            self.ponAyudasEM()
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            if not self.fen:
                self.partida.asignaApertura()
            self.ponteAlFinal()
            self.reOpenBook()
            self.refresh()
            self.siguienteJugada()

    def testBook(self):
        if self.book:
            resp = self.book.miraListaJugadas(self.fenUltimo())
            if not resp:
                self.book = None
                self.ponRotuloBasico()

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

        if self.checkFinal(siBlancas):
            return

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        if self.book:
            self.testBook()

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

    def analizaInicio(self):
        self.siAnalizando = False
        self.siAnalizadoTutor = False
        if not self.childmode:
            if self.aperturaObl or not self.siTutorActivado or self.ayudasPGN <= 0:
                return
        if self.continueTt:
            if not self.siTerminada():
                self.xtutor.ac_inicio(self.partida)
                self.siAnalizando = True
                QtCore.QTimer.singleShot(2000 if self.childmode else 200, self.analizaSiguiente)
        else:
            mrm = self.analizaTutor()
            if mrm and self.childmode:
                self.ponFlechasTutor(mrm, self.nArrowsTt)
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
                        if self.childmode:
                            self.ponFlechasTutor(mrm, self.nArrowsTt)
                        QtCore.QTimer.singleShot(4000 if self.childmode else 2000, self.analizaSiguiente)

    def analizaFinal(self):
        estado = self.siAnalizando
        self.siAnalizando = False
        if not self.childmode:
            if self.siAnalizadoTutor or not self.siTutorActivado or self.ayudasPGN <= 0:
                return
        if self.continueTt and estado:
            self.pensando(True)
            self.mrmTutor = self.xtutor.ac_final(self.xtutor.motorTiempoJugada)
            self.pensando(False)
        else:
            self.mrmTutor = self.analizaTutor()
            if self.mrmTutor and self.childmode:
                self.ponFlechasTutor(self.mrmTutor, self.nArrowsTt)

    def ajustaPlayer(self, mrm):
        posicion = self.partida.ultPosicion

        LCEngine.setFen(posicion.fen())
        li = LCEngine.getExMoves()

        liOpciones = []
        for rm in mrm.liMultiPV:
            liOpciones.append((rm, "%s (%s)" % (posicion.pgnSP(rm.desde, rm.hasta, rm.coronacion), rm.abrTexto())))
            mv = rm.movimiento()
            for x in range(len(li)):
                if li[x].movimiento() == mv:
                    del li[x]
                    break

        for mj in li:
            rm = XMotorRespuesta.RespuestaMotor("", posicion.siBlancas)
            rm.desde = mj.desde()
            rm.hasta = mj.hasta()
            rm.coronacion = mj.coronacion()
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

    def juegaHumano(self, siBlancas):
        self.siJuegaHumano = True
        self.analizaInicio()

        self.relojStart(True)
        self.timekeeper.start()
        self.activaColor(siBlancas)

    def juegaRival(self):
        self.desactivaTodas()

        fenUltimo = self.fenUltimo()

        if fenUltimo in self.cache:
            jg = self.cache[fenUltimo]
            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)
            if self.siTiempo:
                self.tiempo[self.siRivalConBlancas].restore(jg.cacheTime)
            return True

        self.pensando(True)

        self.relojStart(False)

        desde = hasta = coronacion = ""
        siEncontrada = False

        if self.aperturaObl:
            siEncontrada, desde, hasta, coronacion = self.aperturaObl.juegaMotor(fenUltimo)
            if not siEncontrada:
                self.aperturaObl = None

        if not siEncontrada and self.book:
            if self.partida.ultPosicion.jugadas < self.maxMoveBook:
                siEncontrada, desde, hasta, coronacion = self.eligeJugadaBook()
            if not siEncontrada:
                self.book = None

        if not siEncontrada and self.aperturaStd:
            siEncontrada, desde, hasta, coronacion = self.aperturaStd.juegaMotor(fenUltimo)
            if not siEncontrada:
                self.aperturaStd = None

        if not siEncontrada and self.siBookAjustarFuerza:
            siEncontrada, desde, hasta, coronacion = self.eligeJugadaBookAjustada()  # libro de la personalidad
            if not siEncontrada:
                self.siBookAjustarFuerza = False

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

                self.rmRival = self.xrival.juegaTiempo(tiempoBlancas, tiempoNegras, self.segundosJugada,
                                                       nAjustado=self.nAjustarFuerza)
                if self.estado == kFinJuego:
                    return True
                if self.nAjustarFuerza == kAjustarPlayer:
                    self.rmRival = self.ajustaPlayer(self.rmRival)

            else:
                self.rmRival = self.xrival.juegaPartida(self.partida, nAjustado=self.nAjustarFuerza)
                if self.nAjustarFuerza == kAjustarPlayer:
                    self.rmRival = self.ajustaPlayer(self.rmRival)
            self.setSummary("TIMERIVAL", self.timekeeper.stop())

        self.relojStop(False)
        self.pensando(False)

        self.liRMrival.append(self.rmRival)
        if not (self.resignPTS < -1500):  # then not ask for draw
            if not self.valoraRMrival(self.siRivalConBlancas):
                return True

        siBien, self.error, jg = Jugada.dameJugada(self.partida.ultPosicion, self.rmRival.desde, self.rmRival.hasta, self.rmRival.coronacion)
        if siBien:
            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)
            if self.siTiempo:
                jg.cacheTime = self.tiempo[self.siRivalConBlancas].save()
            self.cache[fenUltimo] = jg
            return True
        else:
            return False

    def sigueHumanoAnalisis(self):
        self.analizaInicio()
        Gestor.Gestor.sigueHumano(self)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        movimiento = jg.movimiento()

        siAnalisis = False

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

        if not siElegido and self.aperturaObl:
            fenBase = self.fenUltimo()
            if self.aperturaObl.compruebaHumano(fenBase, desde, hasta):
                siElegido = True
            else:
                apdesde, aphasta = self.aperturaObl.desdeHastaActual(fenBase)
                if apdesde is None:
                    self.aperturaObl = None
                else:
                    self.tablero.ponFlechasTmp(((apdesde, aphasta, False),))
                    self.sigueHumano()
                    return False

        if not siElegido and self.aperturaStd:
            fenBase = self.fenUltimo()
            if self.aperturaStd.compruebaHumano(fenBase, desde, hasta):
                siElegido = True
            else:
                if not self.aperturaStd.isActive(fenBase):
                    self.aperturaStd = None

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        self.analizaFinal()  # tiene que acabar siempre
        if not siElegido and (self.siTutorActivado or self.childmode):
            rmUser, n = self.mrmTutor.buscaRM(movimiento)
            if not rmUser:
                rmUser = self.xtutor.valora(self.partida.ultPosicion, desde, hasta, jg.coronacion)
                if not rmUser:
                    self.sigueHumanoAnalisis()
                    return False
                self.mrmTutor.agregaRM(rmUser)
            siAnalisis = True
            pointsBest, pointsUser = self.mrmTutor.difPointsBest(movimiento)
            self.setSummary("POINTSBEST", pointsBest)
            self.setSummary("POINTSUSER", pointsUser)
            difpts = self.configuracion.tutorDifPts
            difporc = self.configuracion.tutorDifPorc
            if self.mrmTutor.mejorRMQue(rmUser, difpts, difporc):
                if not jg.siJaqueMate:
                    siTutor = not self.childmode
                    if self.chance:
                        num = self.mrmTutor.numMejorMovQue(movimiento)
                        if num:
                            rmTutor = self.mrmTutor.rmBest()
                            menu = QTVarios.LCMenu(self.pantalla)
                            menu.opcion("None", _("There are %d best moves") % num, Iconos.Motor())
                            menu.separador()
                            if siTutor:
                                menu.opcion("tutor", "&1. %s (%s)" % (_("Show tutor"), rmTutor.abrTextoBase()),
                                               Iconos.Tutor())
                                menu.separador()
                            menu.opcion("try", "&2. %s" % _("Try again"), Iconos.Atras())
                            menu.separador()
                            menu.opcion("user", "&3. %s (%s)" % (_("Select my move"), rmUser.abrTextoBase()),
                                           Iconos.Player())
                            self.pantalla.cursorFueraTablero()
                            resp = menu.lanza()
                            if resp == "try":
                                self.sigueHumanoAnalisis()
                                return False
                            elif resp == "user":
                                siTutor = False
                            elif resp != "tutor":
                                self.sigueHumanoAnalisis()
                                return False

                    if siTutor:
                        tutor = Tutor.Tutor(self, self, jg, desde, hasta, False)

                        if self.aperturaStd:
                            liApPosibles = self.listaAperturasStd.listaAperturasPosibles(self.partida)
                        else:
                            liApPosibles = None

                        if tutor.elegir(self.ayudas > 0, liApPosibles=liApPosibles):
                            if self.ayudas > 0:  # doble entrada a tutor.
                                self.reponPieza(desde)
                                self.ayudas -= 1
                                self.childmode = self.nArrowsTt > 0 and self.ayudas > 0
                                desde = tutor.desde
                                hasta = tutor.hasta
                                coronacion = tutor.coronacion
                                siBien, mens, jgTutor = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
                                if siBien:
                                    jg = jgTutor
                                    self.setSummary("SELECTTUTOR", True)
                        if self.configuracion.guardarVariantesTutor:
                            tutor.ponVariantes(jg, 1 + self.partida.numJugadas() / 2)

                        del tutor

        self.setSummary("TIMEUSER", self.timekeeper.stop())
        self.relojStop(True)

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

    def masJugada(self, jg, siNuestra):
        if not self.siPrimeraJugadaHecha:
            self.siPrimeraJugadaHecha = True

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

        self.ponAyudasEM()

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)

        self.ponPosicionDGT()

        self.refresh()

    def saveSummary(self):
        if not self.siSummary or not self.summary:
            return

        j_num = 0
        j_same = 0
        st_accept = 0
        st_reject = 0
        nt_accept = 0
        nt_reject = 0
        j_sum = 0

        time_user = 0.0
        ntime_user = 0
        time_rival = 0.0
        ntime_rival = 0

        for njg, d in self.summary.iteritems():
            if "POINTSBEST" in d:
                j_num += 1
                p = d["POINTSBEST"] - d["POINTSUSER"]
                if p:
                    if d.get("SELECTTUTOR", False):
                        st_accept += p
                        nt_accept += 1
                    else:
                        st_reject += p
                        nt_reject += 1
                    j_sum += p
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
            comment += _("Tutor") + ": %s\n" % self.xtutor.nombre
            comment += _("Number of moves") + ":%d\n" % j_num
            comment += _("Same move") + ":%d (%0.2f%%)\n" % (j_same, j_same * 1.0 / j_num)
            comment += _("Accepted") + ":%d (%0.2f%%) %s: %0.2f\n" % (nt_accept, nt_accept * 1.0 / j_num,
                                                                      _("Average points lost"), st_accept * 1.0 / nt_accept if nt_accept else 0.0)
            comment += _("Rejected") + ":%d (%0.2f%%) %s: %0.2f\n" % (nt_reject, nt_reject * 1.0 / j_num,
                                                                      _("Average points lost"), st_reject * 1.0 / nt_reject if nt_reject else 0.0)
            comment += _("Total") + ":%d (100%%) %s: %0.2f\n" % (j_num, _("Average points lost"), j_sum * 1.0 / j_num)

        if ntime_user or ntime_rival:
            comment += _("Average time (seconds)") + ":\n"
            if ntime_user:
                comment += "%s: %0.2f\n" % (self.configuracion.jugador, time_user / ntime_user)
            if ntime_rival:
                comment += "%s: %0.2f\n" % (self.xrival.nombre, time_rival / ntime_rival)

        self.partida.firstComment = comment

    def checkFinal(self, siBlancas):
        if self.partida.numJugadas() == 0:
            return False

        jgUltima = self.partida.last_jg()
        if jgUltima:
            if jgUltima.siJaqueMate:
                self.ponResultado(kGanaRival if self.siJugamosConBlancas == siBlancas else kGanamos)
                return True
            if jgUltima.siAhogado:
                self.ponResultado(kTablas)
                return True
            if jgUltima.siTablasRepeticion:
                self.ponResultado(kTablasRepeticion)
                return True
            if jgUltima.siTablas50:
                self.ponResultado(kTablas50)
                return True
            if jgUltima.siTablasFaltaMaterial:
                self.ponResultado(kTablasFaltaMaterial)
                return True
        return False

    def ponResultado(self, quien):
        self.estado = kFinJuego
        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.beepResultado(quien)

        self.saveSummary()

        nombreContrario = self.xrival.nombre

        mensaje = _("Game ended")
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
            if self.partida.ultPosicion.siFaltaMaterialColor(self.siJugamosConBlancas):
                return self.ponResultado(kTablasFaltaMaterial)
            mensaje = _X(_("Congratulations, you win %1 on time."), nombreContrario)
            self.resultado = kGanamos
            self.partida.last_jg().comentario = _X(_("%1 has won on time."), self.configuracion.jugador)

        elif quien == kGanaRivalTiempo:
            if self.partida.ultPosicion.siFaltaMaterialColor(not self.siJugamosConBlancas):
                return self.ponResultado(kTablasFaltaMaterial)
            mensaje = _X(_("%1 has won on time."), nombreContrario)
            self.resultado = kGanaRival
            self.partida.last_jg().comentario = _X(_("%1 has won on time."), nombreContrario)

        self.guardarGanados(quien == kGanamos)
        if quien != kGanaRivalTiempo:  # Ya que el mensaje ya se ha usado, para a_adir minutos
            self.mensajeEnPGN(mensaje)
        if QTUtil2.pregunta(self.pantalla, _("Do you want to play again?")):
            self.reiniciar(False)
        else:
            self.ponFinJuego()

    def ponAyudasEM(self):
        self.ponAyudas(self.ayudas, siQuitarAtras=not self.siAtras)

    def cambioRival(self):
        dic = PantallaEntMaq.cambioRival(self.pantalla, self.configuracion, self.reinicio)

        if dic:
            dr = dic["RIVAL"]
            rival = dr["CM"]
            if hasattr(rival, "icono"):
                delattr(rival, "icono")

            Util.guardaDIC(dic, self.configuracion.ficheroEntMaquina)
            for k, v in dic.iteritems():
                self.reinicio[k] = v

            siBlancas = dic["SIBLANCAS"]

            self.siAtras = dic["ATRAS"]
            if self.siAtras:
                self.pantalla.ponToolBar(
                        (k_mainmenu, k_rendirse, k_atras, k_reiniciar, k_aplazar, k_configurar, k_utilidades))
            else:
                self.pantalla.ponToolBar((k_mainmenu, k_rendirse, k_reiniciar, k_aplazar, k_configurar, k_utilidades))

            self.nAjustarFuerza = dic["AJUSTAR"]

            r_t = dr["TIEMPO"] * 100  # Se guarda en decimas -> milesimas
            r_p = dr["PROFUNDIDAD"]
            if r_t <= 0:
                r_t = None
            if r_p <= 0:
                r_p = None
            if r_t is None and r_p is None and not dic["SITIEMPO"]:
                r_t = 1000

            dr["RESIGN"] = self.resignPTS
            self.xrival.terminar()
            self.xrival = self.procesador.creaGestorMotor(rival, r_t, r_p, self.nAjustarFuerza != kAjustarMejor)

            self.xrival.siBlancas = not siBlancas

            self.rmRival = None

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
        if self.estado != kJugando:
            return
        if self.siJuegaHumano:
            if not rm.whoDispatch:  # juega humano y el rm es del rival
                return True
            tp = self.thoughtTt
        else:
            if rm.whoDispatch:  # juega rival y el rm es del humano
                return True
            tp = self.thoughtOp
            if tp > -1 and self.nArrows:
                self.showPV(rm.pv, self.nArrows)
        if tp > -1:

            if rm.time or rm.depth:
                colorEngine = "DarkBlue" if self.siJuegaHumano else "brown"
                if rm.nodes:
                    nps = "/%d" % rm.nps if rm.nps else ""
                    nodes = " | %d%s" % (rm.nodes, nps)
                else:
                    nodes = ""
                seldepth = "/%d" % rm.seldepth if rm.seldepth else ""
                li = [
                    '<span style="color:%s">%s' % (colorEngine, rm.nombre),
                    '<b>%s</b> | <b>%d</b>%s | <b>%d"</b>%s' % (rm.abrTextoBase(),
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
                    li.append(p.pgnBaseRAW())
                self.ponRotulo3("<br>".join(li) + "</span>")

        return True

    def pgnLabelsAdded(self):
        d = {}
        if self.nAjustarFuerza != kAjustarMejor:
            pers = Personalidades.Personalidades(None, self.configuracion)
            label = pers.label(self.nAjustarFuerza)
            if label:
                d["Strength"] = label
        return d

    def analizaPosicion(self, fila, clave):
        if fila < 0:
            return

        jg, siBlancas, siUltimo, tam_lj, pos = self.dameJugadaEn(fila, clave)
        if not jg:
            return

        maxRecursion = 9999

        if not (hasattr(jg, "analisis") and jg.analisis):
            me = QTUtil2.mensEspera.inicio(self.pantalla, _("Analyzing the move...."), posicion="ad")
            mrm, pos = self.xanalyzer.analizaJugada(jg, self.xanalyzer.motorTiempoJugada, self.xanalyzer.motorProfundidad)
            jg.analisis = mrm, pos
            me.final()

        Analisis.muestraAnalisis(self.procesador, self.xanalyzer, jg, self.tablero.siBlancasAbajo, maxRecursion, pos)
        self.ponVista()
