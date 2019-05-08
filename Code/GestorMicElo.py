import datetime
import random

from Code import Books
from Code import DGT
from Code import EnginesMicElo
from Code import Gestor
from Code import Jugada
from Code.QT import QTUtil2
from Code import Util
from Code import VarGen
from Code import XMotorRespuesta
from Code.Constantes import *


class DicMicElos:
    def __init__(self):
        self.variable = "DicMicElos"
        self.configuracion = VarGen.configuracion
        self._dic = self.configuracion.leeVariables(self.variable)

    def dic(self):
        return self._dic

    def cambiaElo(self, claveMotor, nuevoElo):
        self._dic = self.configuracion.leeVariables(self.variable)
        self._dic[claveMotor] = nuevoElo
        self.configuracion.escVariables(self.variable, self._dic)


def lista():
    li = EnginesMicElo.listaCompleta()
    dicElos = DicMicElos().dic()
    for mt in li:
        k = mt.clave
        if k in dicElos:
            mt.elo = dicElos[k]

    return li


class GestorMicElo(Gestor.Gestor):
    def calcDifElo(self, eloJugador, eloRival, resultado):
        if resultado == kGanamos:
            result = 1
        elif resultado == kTablas:
            result = 0
        else:
            result = -1
        return Util.fideELO(eloJugador, eloRival, result)

    def listaMotores(self, elo):
        self.liT = (
            (0, 50, 3), (20, 53, 5), (40, 58, 4), (60, 62, 4), (80, 66, 5), (100, 69, 4), (120, 73, 3), (140, 76, 3),
            (160, 79, 3), (180, 82, 2), (200, 84, 9), (300, 93, 4), (400, 97, 3))
        self.liK = ((0, 60), (800, 50), (1200, 40), (1600, 30), (2000, 30), (2400, 10))

        li = []
        self.liMotores = lista()
        numX = len(self.liMotores)
        for num, mt in enumerate(self.liMotores):
            mtElo = mt.elo
            mt.siJugable = abs(mtElo - elo) < 400
            mt.siOut = not mt.siJugable
            mt.baseElo = elo  # servira para rehacer la lista y elegir en aplazamiento
            if mt.siJugable or (mtElo > elo):
                def rot(res):
                    return self.calcDifElo(elo, mtElo, res)

                def rrot(res):
                    return self.calcDifElo(mtElo, elo, res)

                mt.pgana = rot(kGanamos)
                mt.ptablas = rot(kTablas)
                mt.ppierde = rot(kGanaRival)

                mt.rgana = rrot(kGanamos)
                mt.rtablas = rrot(kTablas)
                mt.rpierde = rrot(kGanaRival)

                mt.numero = numX - num

                li.append(mt)

        return li

    def engineAplazado(self, alias, basElo):
        li = self.listaMotores(basElo)
        for mt in li:
            if mt.alias == alias:
                return mt
        return None

    def inicio(self, datosMotor, minutos, segundos, siCompetitivo, aplazamiento=None):

        self.tipoJuego = kJugMicElo

        self.resultado = None
        self.siJuegaHumano = False
        self.estado = kJugando
        self.siCompetitivo = siCompetitivo
        self.puestoResultado = False # Problema doble asignacion de ptos Thomas

        if aplazamiento:
            siBlancas = aplazamiento["SIBLANCAS"]
        else:
            siBlancas = self.determinaColor(datosMotor)

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.rmRival = None
        self.liRMrival = []
        self.noMolestar = 0
        self.resignPTS = -1000

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(False)
        self.ayudasPGN = self.ayudas = 0

        self.tiempo = {}
        self.maxSegundos = minutos * 60
        self.segundosJugada = segundos

        # -Aplazamiento 1/2--------------------------------------------------
        if aplazamiento:
            self.partida.recuperaDeTexto(aplazamiento["JUGADAS"])

            self.datosMotor = self.engineAplazado(aplazamiento["ALIAS"], aplazamiento["BASEELO"])

            self.tiempo[True] = Util.Timer(aplazamiento["TIEMPOBLANCAS"])
            self.tiempo[False] = Util.Timer(aplazamiento["TIEMPONEGRAS"])

            self.maxSegundos = aplazamiento["MAXSEGUNDOS"]
            self.segundosJugada = aplazamiento["SEGUNDOSJUGADA"]

            self.partida.asignaApertura()

        else:
            self.datosMotor = datosMotor
            self.tiempo[True] = Util.Timer(self.maxSegundos)
            self.tiempo[False] = Util.Timer(self.maxSegundos)

        cbook = self.datosMotor.book if self.datosMotor.book else VarGen.tbook
        self.book = Books.Libro("P", cbook, cbook, True)
        self.book.polyglot()

        elo = self.datosMotor.elo
        self.maxMoveBook = elo / 200 if 0 <= elo <= 1700 else 9999

        eloengine = self.datosMotor.elo
        eloplayer = self.configuracion.miceloActivo(siCompetitivo)
        self.whiteElo = eloplayer if siBlancas else eloengine
        self.blackElo = eloplayer if not siBlancas else eloengine

        self.xrival = self.procesador.creaGestorMotor(self.datosMotor, None, None,
                                                      siMultiPV=self.datosMotor.multiPV > 0)

        self.pteToolRendirse = False
        if not self.siCompetitivo:
            self.pteToolRendirse = True
            self.maxPlyRendirse = 6
        elif self.siJugamosConBlancas:
            self.pteToolRendirse = True
            self.maxPlyRendirse = 1
        else:
            self.maxPlyRendirse = 0

        if aplazamiento and self.partida.numJugadas() > self.maxPlyRendirse:
            self.pteToolRendirse = False

        self.ponToolBar()

        self.pantalla.activaJuego(True, True, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(siBlancas)
        self.quitaAyudas(True, siQuitarAtras=siCompetitivo)
        self.mostrarIndicador(True)

        nbsp = "&nbsp;" * 3

        txt = "%s:%+d%s%s:%+d%s%s:%+d" % (_("Win"), self.datosMotor.pgana, nbsp,
                                          _("Draw"), self.datosMotor.ptablas, nbsp,
                                          _("Loss"), self.datosMotor.ppierde)
        self.ponRotulo1("<center>%s</center>" % txt)
        self.ponRotulo2("")
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        # -Aplazamiento 2/2--------------------------------------------------
        if aplazamiento:
            self.mueveJugada(kMoverFinal)
            self.siPrimeraJugadaHecha = True
        else:
            self.siPrimeraJugadaHecha = False

        tpBL = self.tiempo[True].etiqueta()
        tpNG = self.tiempo[False].etiqueta()
        self.rival = rival = self.datosMotor.alias + " (%d)" % self.datosMotor.elo
        jugador = self.configuracion.jugador + " (%d)" % self.configuracion.miceloActivo(siCompetitivo)
        bl, ng = jugador, rival
        if self.siRivalConBlancas:
            bl, ng = ng, bl
        self.pantalla.ponDatosReloj(bl, tpBL, ng, tpNG)
        self.refresh()

        self.ponPosicionDGT()

        if not self.siJugamosConBlancas:
            mensaje = _("Press the continue button to start.")
            self.mensajeEnPGN(mensaje)

        self.siguienteJugada()

    def ponToolBar(self):
        if self.pteToolRendirse:
            liTool = (k_cancelar, k_aplazar, k_atras, k_configurar, k_utilidades)
        else:
            if self.siCompetitivo:
                liTool = (k_rendirse, k_tablas, k_aplazar, k_configurar, k_utilidades)
            else:
                liTool = (k_rendirse, k_tablas, k_aplazar, k_atras, k_configurar, k_utilidades)

        self.pantalla.ponToolBar(liTool)

    def procesarAccion(self, clave):

        if clave in (k_rendirse, k_cancelar):
            self.rendirse()

        elif clave == k_tablas:
            self.tablasPlayer()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_atras:
            self.atras()

        elif clave == k_utilidades:
            self.utilidadesElo()

        elif clave == k_aplazar:
            self.aplazar()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def aplazar(self):
        if self.partida.numJugadas() and QTUtil2.pregunta(self.pantalla, _("Do you want to adjourn the game?")):
            aplazamiento = {}
            aplazamiento["TIPOJUEGO"] = self.tipoJuego
            aplazamiento["SIBLANCAS"] = self.siJugamosConBlancas
            aplazamiento["JUGADAS"] = self.partida.guardaEnTexto()
            aplazamiento["SICOMPETITIVO"] = self.siCompetitivo

            aplazamiento["BASEELO"] = self.datosMotor.baseElo
            aplazamiento["ALIAS"] = self.datosMotor.alias

            aplazamiento["MAXSEGUNDOS"] = self.maxSegundos
            aplazamiento["SEGUNDOSJUGADA"] = self.segundosJugada
            aplazamiento["TIEMPOBLANCAS"] = self.tiempo[True].tiempoAplazamiento()
            aplazamiento["TIEMPONEGRAS"] = self.tiempo[False].tiempoAplazamiento()

            self.configuracion.graba(aplazamiento)
            self.estado = kFinJuego
            self.pantalla.accept()

    def finalX(self):
        return self.rendirse()

    def rendirse(self):
        if self.estado == kFinJuego:
            return True
        if (self.partida.numJugadas() > 0) and not self.pteToolRendirse:
            if not QTUtil2.pregunta(self.pantalla, _("Do you want to resign?") + " (%d)" % self.datosMotor.ppierde):
                return False  # no abandona
            self.partida.abandona(self.siJugamosConBlancas)
            self.ponResultado(kGanaRival)
        else:
            self.procesador.inicio()

        return False

    def siguienteJugada(self):

        if self.estado == kFinJuego:
            return

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
            self.relojStart(False)
            self.pensando(True)
            self.desactivaTodas()

            siEncontrada = False

            if self.book:
                if self.partida.ultPosicion.jugadas >= self.maxMoveBook:
                    self.book = None
                else:
                    fen = self.fenUltimo()
                    pv = self.book.eligeJugadaTipo(fen, "ap")
                    if pv:
                        self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
                        self.rmRival.desde = pv[:2]
                        self.rmRival.hasta = pv[2:4]
                        self.rmRival.coronacion = pv[4:]
                        siEncontrada = True
                    else:
                        self.book = None
            if not siEncontrada:
                tiempoBlancas = self.tiempo[True].tiempoPendiente
                tiempoNegras = self.tiempo[False].tiempoPendiente
                mrm = self.xrival.juegaTiempoTorneo(tiempoBlancas, tiempoNegras, self.segundosJugada)
                if mrm is None:
                    self.pensando(False)
                    return False
                self.rmRival = mrm.mejorMov()

            self.relojStop(False)

            self.pensando(False)
            if self.mueveRival(self.rmRival):
                self.liRMrival.append(self.rmRival)
                if self.valoraRMrival(self.siRivalConBlancas):
                    self.siguienteJugada()
            else:
                self.ponResultado(kGanamos)
        else:
            self.relojStart(True)

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        self.movimientosPiezas(jg.liMovs)
        self.relojStop(True)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):

        if not self.siPrimeraJugadaHecha:
            self.siPrimeraJugadaHecha = True

        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.liJugadas.append(jg)
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

        if self.pteToolRendirse:
            if self.partida.numJugadas() > self.maxPlyRendirse:
                self.pteToolRendirse = False
                self.ponToolBar()

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
        if self.puestoResultado: # Problema doble asignacion de ptos Thomas
            return

        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.estado = kFinJuego

        self.beepResultado(quien)

        nombreContrario = self.rival

        mensaje = _("Game ended")
        if quien == kGanamos:
            mensaje = _X(_("Congratulations you have won against %1."), nombreContrario)

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

        elif quien == kGanamosTiempo:
            if self.partida.ultPosicion.siFaltaMaterialColor(self.siJugamosConBlancas):
                return self.ponResultado(kTablasFaltaMaterial)
            mensaje = _X(_("Congratulations, you win against %1 on time."), nombreContrario)
            self.resultado = kGanamos

        elif quien == kGanaRivalTiempo:
            if self.partida.ultPosicion.siFaltaMaterialColor(not self.siJugamosConBlancas):
                return self.ponResultado(kTablasFaltaMaterial)
            mensaje = _X(_("%1 has won on time."), nombreContrario)
            self.resultado = kGanaRival

        elo = self.configuracion.miceloActivo(self.siCompetitivo)
        relo = self.datosMotor.elo
        if self.resultado == kGanamos:
            difelo = self.datosMotor.pgana
            # rdifelo = self.datosMotor.ppierde

        elif self.resultado == kGanaRival:
            difelo = self.datosMotor.ppierde
            # rdifelo = self.datosMotor.pgana

        else:
            difelo = self.datosMotor.ptablas
            # rdifelo = self.datosMotor.ptablas

        nelo = elo + difelo
        if nelo < 0:
            nelo = 0
        self.configuracion.ponMiceloActivo(nelo, self.siCompetitivo)

        rnelo = relo - difelo
        if rnelo < 100:
            rnelo = 100
        dme = DicMicElos()
        dme.cambiaElo(self.datosMotor.clave, rnelo)

        # self.configuracion.ponMiceloActivo(nelo, self.siCompetitivo)

        if not self.siCompetitivo:
            self.procesador.entrenamientos.rehaz()

        self.historial(elo, nelo)
        self.configuracion.graba()

        mensaje += "<br><br>%s : %d<br>" % (_("New Tourney-Elo"), nelo)

        self.guardarGanados(quien == kGanamos)
        self.puestoResultado = True
        self.mensajeEnPGN(mensaje)
        self.ponFinJuego()

    def historial(self, elo, nelo):
        dic = {}
        dic["FECHA"] = datetime.datetime.now()
        dic["RIVAL"] = self.datosMotor.nombre
        dic["RESULTADO"] = self.resultado
        dic["AELO"] = elo
        dic["NELO"] = nelo
        dic["MODONC"] = not self.siCompetitivo

        lik = Util.LIdisk(self.configuracion.fichEstadMicElo)
        lik.append(dic)
        lik.close()

        dd = Util.DicSQL(self.configuracion.fichEstadMicElo, tabla="color")
        clave = self.datosMotor.nombre
        dd[clave] = self.siJugamosConBlancas
        dd.close()

    def determinaColor(self, datosMotor):
        clave = datosMotor.nombre
        if not self.siCompetitivo:
            clave += "NC"

        dd = Util.DicSQL(self.configuracion.fichEstadMicElo, tabla="color")
        previo = dd.get(clave, random.randint(0, 1) == 0)
        dd.close()
        return not previo

    def ponReloj(self):

        if (not self.siPrimeraJugadaHecha) or self.estado != kJugando:
            return

        def mira(siBlancas):
            ot = self.tiempo[siBlancas]

            eti, eti2 = ot.etiquetaDif2()
            if eti:
                if siBlancas:
                    self.pantalla.ponRelojBlancas(eti, eti2)
                else:
                    self.pantalla.ponRelojNegras(eti, eti2)

            if ot.siAgotado():
                siJugador = self.siJugamosConBlancas == siBlancas
                self.ponResultado(kGanaRivalTiempo if siJugador else kGanamosTiempo)
                return False

            return True

        if VarGen.dgt:
            DGT.writeClocks(self.tiempo[True].etiquetaDGT(), self.tiempo[False].etiquetaDGT())

        if self.siJuegaHumano:
            siBlancas = self.siJugamosConBlancas
        else:
            siBlancas = not self.siJugamosConBlancas
        mira(siBlancas)

    def relojStart(self, siUsuario):
        if self.siPrimeraJugadaHecha:
            self.tiempo[siUsuario == self.siJugamosConBlancas].iniciaMarcador()
            self.pantalla.iniciaReloj(self.ponReloj, transicion=200)

    def relojStop(self, siUsuario):
        if self.siPrimeraJugadaHecha:
            self.tiempo[siUsuario == self.siJugamosConBlancas].paraMarcador(self.segundosJugada)
            self.ponReloj()
            self.pantalla.paraReloj()
            self.refresh()

    def atras(self):
        if self.partida.numJugadas() > 2:
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.partida.asignaApertura()
            self.ponteAlFinal()
            self.refresh()
            self.siguienteJugada()
