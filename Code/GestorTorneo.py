from Code import Books
from Code import Gestor
from Code import Jugada
from Code import Partida
from Code.QT import PantallaTorneos
from Code import Util
from Code import VarGen
from Code import XGestorMotor
from Code.Constantes import *


class GestorTorneo(Gestor.Gestor):
    def inicio(self, torneo, liGames):

        self.tipoJuego = kJugMvM

        self.torneo = torneo
        self.torneoTMP = torneo.clone()
        self.torneoTMP._liGames = liGames
        self.fenInicial = self.torneo.fenNorman()
        self.liGames = liGames
        self.pantalla.ponActivarTutor(False)
        self.ponPiezasAbajo(True)
        self.mostrarIndicador(True)
        self.siTerminar = False
        self.pantalla.ponToolBar((k_cancelar,))
        self.colorJugando = True
        self.ponCapPorDefecto()

        self.wresult = PantallaTorneos.WResult(self.pantalla, torneo, self.torneoTMP, self)
        self.wresult.show()

        numGames = len(self.liGames)
        for ng, gm in enumerate(self.liGames):
            self.siguienteJuego(gm, ng + 1, numGames)
            if self.siTerminar:
                break
            if self.wresult:
                self.wresult.refresh()

        if self.wresult:
            self.wresult.terminar()

    def siguienteJuego(self, gm, ngame, numGames):

        self.estado = kJugando

        self.gm = gm
        self.maxSegundos = self.gm.minutos() * 60.0
        self.segundosJugada = self.gm.segundosJugada()

        rival = {
            True: self.torneo.buscaHEngine(self.gm.hwhite()),
            False: self.torneo.buscaHEngine(self.gm.hblack()),
        }

        self.tiempo = {}

        self.book = {}
        self.bookRR = {}

        self.xmotor = {}

        for color in (True, False):

            rv = rival[color]
            self.xmotor[color] = XGestorMotor.GestorMotor(self.procesador, rv.configMotor())
            self.xmotor[color].opciones(rv.time() * 1000, rv.depth(), False)
            self.xmotor[color].ponGuiDispatch(self.guiDispatch)

            self.tiempo[color] = Util.Timer(self.maxSegundos)

            bk = rv.book()
            if bk == "*":
                bk = VarGen.tbook
            elif bk == "-":
                bk = None
            if bk:
                self.book[color] = Books.Libro("P", bk, bk, True)
                self.book[color].polyglot()
            else:
                self.book[color] = None
            self.bookRR[color] = rv.bookRR()

        self.partida = Partida.Partida(fen=self.fenInicial)
        self.pgn.partida = self.partida

        self.alturaRotulo3(32)

        self.desactivaTodas()
        self.ponPosicion(self.partida.ultPosicion)
        self.pgnRefresh(True)

        # self.siPrimeraJugadaHecha = False
        tpBL = self.tiempo[True].etiqueta()
        tpNG = self.tiempo[False].etiqueta()
        bl = self.xmotor[True].nombre
        ng = self.xmotor[False].nombre
        self.pantalla.activaJuego(True, True, siAyudas=False)
        self.ponRotulo1("<center><b>%s %d/%d</b></center>" % (_("Game"), ngame, numGames))
        self.ponRotulo2(None)
        self.quitaAyudas()
        self.pantalla.ponDatosReloj(bl, tpBL, ng, tpNG)
        self.refresh()

        self.finPorTiempo = None
        while self.siguienteJugada():
            pass

        self.xmotor[True].terminar()
        self.xmotor[False].terminar()
        self.pantalla.paraReloj()

    def guiDispatch(self, rm):
        if not rm.sinInicializar:
            p = Partida.Partida(self.partida.ultPosicion)
            p.leerPV(rm.pv)
            rm.siBlancas = self.partida.ultPosicion.siBlancas
            txt = "<b>[%s]</b> (%s) %s" % (rm.nombre, rm.abrTexto(), p.pgnSP())
            self.ponRotulo3(txt)
            self.showPV(rm.pv, 3)
            # self.ponFlechaSC( rm.pv[:2], rm.pv[2:4] )
            # QTUtil.refreshGUI()
        self.refresh()
        return not self.siTerminar

    def compruebaFinal(self):
        if self.partida.numJugadas() == 0:
            return False

        if self.finPorTiempo is not None:
            result = self.finPorTiempo

        else:
            jgUlt = self.partida.last_jg()
            result = None
            if self.partida.siTerminada():
                if jgUlt.siJaqueMate:
                    result = 1 if jgUlt.posicionBase.siBlancas else 2
                else:
                    result = 0
            elif self.partida.numJugadas() >= 2:
                tiempoBlancas = self.tiempo[True].tiempoPendiente
                tiempoNegras = self.tiempo[False].tiempoPendiente
                if tiempoBlancas < 60 or tiempoNegras < 60:
                    return False
                if not jgUlt.analisis:
                    return False
                mrm, pos = jgUlt.analisis
                rmUlt = mrm.liMultiPV[pos]
                jgAnt = self.partida.jugada(-2)
                if not jgAnt.analisis:
                    return False
                mrm, pos = jgAnt.analisis
                rmAnt = mrm.liMultiPV[pos]

                # Draw
                pUlt = rmUlt.puntosABS()
                pAnt = rmAnt.puntosABS()
                if self.partida.numJugadas() >= self.torneo.drawMinPly():
                    dr = self.torneo.drawRange()
                    if abs(pUlt) <= dr and abs(pAnt) <= dr:
                        if abs(tiempoBlancas - tiempoNegras) > 60:
                            return False
                        mrmTut = self.xtutor.analiza(self.partida.ultPosicion.fen())
                        rmTut = mrmTut.mejorMov()
                        pTut = rmTut.puntosABS()
                        if abs(pTut) <= dr:
                            result = 0

                # Dif puntos
                if result is None:
                    rs = self.torneo.resign()
                    if pUlt >= rs:
                        rmTut = self.xtutor.analiza(self.partida.ultPosicion.fen())
                        pTut = -rmTut.mejorMov().puntosABS()
                        if pTut >= rs:
                            result = 1 if jgAnt.posicion.siBlancas else 2

            if result is None:
                return False

        self.gm.partida(self.partida)
        self.gm.result(result)
        self.torneo.grabar()

        return True

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return False

        self.estado = kJugando

        siBlancas = self.partida.siBlancas()
        self.colorJugando = siBlancas

        if self.compruebaFinal():
            return False

        self.ponIndicador(siBlancas)
        self.refresh()

        self.relojStart(siBlancas)

        siEncontrada = False
        analisis = None
        bk = self.book[siBlancas]
        if bk:
            siEncontrada, desde, hasta, coronacion = self.eligeJugadaBook(bk, self.bookRR[siBlancas])
            if not siEncontrada:
                self.book[siBlancas] = None

        if not siEncontrada:
            xrival = self.xmotor[siBlancas]
            tiempoBlancas = self.tiempo[True].tiempoPendiente
            tiempoNegras = self.tiempo[False].tiempoPendiente
            segundosJugada = xrival.motorTiempoJugada
            if self.siTerminar:
                return False
            mrm = xrival.juegaTiempoTorneo(tiempoBlancas, tiempoNegras, segundosJugada)
            if mrm is None:
                return False
            rm = mrm.mejorMov()
            desde = rm.desde
            hasta = rm.hasta
            coronacion = rm.coronacion
            analisis = mrm, 0

        self.relojStop(siBlancas)
        if self.siTerminar:
            return False

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        if not jg:
            return False
        if analisis:
            jg.analisis = analisis
            jg.critica = ""
        self.masJugada(jg)
        self.movimientosPiezas(jg.liMovs, True)

        return not self.siTerminar

    def ponReloj(self):

        # if (not self.siPrimeraJugadaHecha) or (self.estado != kJugando):
        if self.estado != kJugando:
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
                self.finPorTiempo = 2 if siBlancas else 1
                return False

            return True

        mira(self.colorJugando)
        self.refresh()

    def relojStart(self, siBlancas):
        # if self.siPrimeraJugadaHecha:
        self.tiempo[siBlancas].iniciaMarcador()
        self.pantalla.iniciaReloj(self.ponReloj, transicion=200)

    def relojStop(self, siBlancas):
        # if self.siPrimeraJugadaHecha:
        self.tiempo[siBlancas].paraMarcador(self.segundosJugada)
        self.ponReloj()
        self.pantalla.paraReloj()

    def procesarAccion(self, clave):
        if clave == k_cancelar:
            self.siTerminar = True
            self.xmotor[True].terminar()
            self.xmotor[False].terminar()

    def finalX(self):
        self.siTerminar = True
        self.xmotor[True].terminar()
        self.xmotor[False].terminar()
        return False

    def eligeJugadaBook(self, book, tipo):
        fen = self.fenUltimo()
        pv = book.eligeJugadaTipo(fen, tipo)
        if pv:
            return True, pv[:2], pv[2:4], pv[4:]
        else:
            return False, None, None, None

    def masJugada(self, jg):

        # if not self.siPrimeraJugadaHecha:
        # self.siPrimeraJugadaHecha = True

        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.append_jg(jg)
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

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)

        self.refresh()
