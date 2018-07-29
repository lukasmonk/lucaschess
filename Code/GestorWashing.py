from PyQt4 import QtCore

from Code import Apertura
from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code.QT import PantallaJuicio
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code import Tutor
from Code import Util
from Code import XMotorRespuesta
from Code import Washing
from Code.Constantes import *


def gestorWashing(procesador):
    dbwashing = Washing.DBWashing(procesador.configuracion)
    washing = dbwashing.washing
    engine = washing.lastEngine(procesador.configuracion)
    if engine.state == Washing.CREATING:
        procesador.gestor = GestorWashingCreate(procesador)
        procesador.gestor.inicio(dbwashing, washing, engine)

    elif engine.state == Washing.TACTICS:
        procesador.gestor = GestorWashingTactics(procesador)
        procesador.gestor.inicio(dbwashing, washing, engine)

    elif engine.state == Washing.REPLAY:
        procesador.gestor = GestorWashingReplay(procesador)
        procesador.gestor.inicio(dbwashing, washing, engine)


class GestorWashingReplay(Gestor.Gestor):
    def inicio(self, dbwashing, washing, engine):
        self.dbwashing = dbwashing
        self.washing = washing
        self.engine = engine

        self.dbwashing.addGame()

        self.tipoJuego = kJugWashingReplay

        self.timekeeper = Util.Timekeeper()

        self.siJuegaHumano = False

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(False)
        self.ayudasPGN = 0

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True, True)
        self.ponMensajero(self.mueveHumano)
        self.mostrarIndicador(True)

        self.partidaObj = self.dbwashing.restoreGame(self.engine)
        self.numJugadasObj = self.partidaObj.numJugadas()
        self.posJugadaObj = 0

        liOpciones = [k_mainmenu]
        self.pantalla.ponToolBar(liOpciones)

        self.errores = 0

        self.book = Apertura.AperturaPol(999, elo=engine.elo)

        siBlancas = self.engine.color
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(siBlancas)

        self.ponRotulo1("%s: %s\n%s: %s" % (_("Rival"), self.engine.nombre, _("Task"), self.engine.lbState()))

        self.pgnRefresh(True)

        self.partida.pendienteApertura = True

        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        self.estado = kJugando

        self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.terminar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=False)

        elif clave == k_utilidades:
            self.utilidades()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.ponRotulo2("<b>%s: %d</b>" % (_("Errors"), self.errores))

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        if siRival:
            jg = self.partidaObj.jugada(self.posJugadaObj)
            self.posJugadaObj += 1
            self.mueveRival(jg.desde, jg.hasta, jg.coronacion)
            self.siguienteJugada()

        else:
            self.siJuegaHumano = True
            self.timekeeper.start()
            self.activaColor(siBlancas)

    def finPartida(self):
        ok = self.errores == 0
        self.dbwashing.done_reinit(self.engine)

        self.estado = kFinJuego
        self.desactivaTodas()

        liOpciones = [k_mainmenu, k_configurar, k_utilidades]
        self.pantalla.ponToolBar(liOpciones)

        if ok:
            mens = _("Congratulations, this washing is done")
        else:
            mens = "%s<br>%s: %d" %( _("Done with errors."), _("Errors"), self.errores)
        self.mensajeEnPGN(mens)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        movUsu = jg.movimiento().lower()
        self.dbwashing.addTime(self.timekeeper.stop())

        jgObj = self.partidaObj.jugada(self.posJugadaObj)
        movObj = jgObj.movimiento().lower()
        if movUsu != movObj:
            lic = []
            if jgObj.analisis:
                mrmObj, posObj = jgObj.analisis
                rmObj = mrmObj.liMultiPV[posObj]
                lic.append("%s: %s (%s)" % (_("Played previously"), jgObj.pgnSP(), rmObj.abrTextoBase()))
                ptsObj = rmObj.puntosABS()
                rmUsu, posUsu = mrmObj.buscaRM(movUsu)
                if posUsu >= 0:
                    lic.append("%s: %s (%s)" % (_("Played now"), jg.pgnSP(), rmUsu.abrTextoBase()))
                    ptsUsu = rmUsu.puntosABS()
                    if ptsUsu < ptsObj-10:
                        lic[-1] += ' <span style="color:red"><b>%s</b></span>' % _("Bad move")
                        self.errores += 1
                        self.dbwashing.addHint()

                else:
                    lic.append("%s: %s (?) %s" % (_("Played now"), jg.pgnSP(), _("Bad move")))
                    self.errores += 1
                    self.dbwashing.addHint()

            else:
                # Debe ser una jugada de libro para aceptarla
                fen = self.fenUltimo()
                siBookUsu = self.book.compruebaHumano(fen, desde, hasta)
                bmove = _("book move")
                lic.append("%s: %s (%s)" % (_("Played previously"), jgObj.pgnSP(), bmove))
                if siBookUsu:
                    lic.append("%s: %s (%s)" % (_("Played now"), jg.pgnSP(), bmove))
                else:
                    lic.append("%s: %s (?) %s" % (_("Played now"), jg.pgnSP(), _("Bad move")))
                    self.errores += 1
                    self.dbwashing.addHint()

            comentario = "<br>".join(lic)
            w = PantallaJuicio.MensajeF(self.pantalla, comentario)
            w.mostrar()
            self.ponPosicion(jg.posicionBase)

        # Creamos un jg sin analisis
        siBien, self.error, jg = Jugada.dameJugada(self.partida.ultPosicion, jgObj.desde, jgObj.hasta, jgObj.coronacion)

        self.movimientosPiezas(jg.liMovs)
        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.posJugadaObj += 1
        if self.partida.numJugadas() == self.partidaObj.numJugadas():
            self.finPartida()

        else:
            self.error = ""
            self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):
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

    def mueveRival(self, desde, hasta, coronacion):
        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, False)
        self.movimientosPiezas(jg.liMovs, True)
        self.error = ""

    def terminar(self):
        self.procesador.inicio()
        self.procesador.showWashing()

    def finalX(self):
        self.procesador.inicio()
        return False


class GestorWashingTactics(Gestor.Gestor):
    def inicio(self, dbwashing, washing, engine):
        self.dbwashing = dbwashing
        self.washing = washing
        self.engine = engine

        self.tipoJuego = kJugWashingTactics

        self.timekeeper = Util.Timekeeper()

        self.siJuegaHumano = False

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(False)
        self.ayudasPGN = 0

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True, True)
        self.ponMensajero(self.mueveHumano)
        self.mostrarIndicador(True)

        self.next_line()

    def next_line(self):
        self.line = self.dbwashing.next_tactic(self.engine)
        self.num_lines = self.engine.numTactics()
        if not self.line:
            return

        liOpciones = [k_mainmenu, k_ayuda]
        self.pantalla.ponToolBar(liOpciones)

        self.num_move = -1
        self.ayudas = 0
        self.errores = 0
        self.time_used = 0.0

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.line.fen)
        self.partida.reset(cp)

        siBlancas = cp.siBlancas
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(siBlancas)
        #r1 = self.line.label
        self.ponRotulo1("")
        r2 = "<b>%s: %d</b>" % (_("Pending"), self.num_lines)
        self.ponRotulo2(r2)
        self.pgnRefresh(True)

        self.partida.pendienteApertura = False

        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        self.estado = kJugando

        self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_ayuda:
            self.ayuda()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=False)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_siguiente:
            self.next_line()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        self.num_move += 1
        if self.num_move >= self.line.total_moves():
            self.finLinea()
            return

        if siRival:
            pv = self.line.get_move(self.num_move)
            desde, hasta, coronacion = pv[:2], pv[2:4], pv[4:]
            self.mueveRival(desde, hasta, coronacion)
            self.siguienteJugada()

        else:
            self.ayudasEsteMov = 0
            self.erroresEsteMov = 0
            self.siJuegaHumano = True
            self.timekeeper.start()
            self.activaColor(siBlancas)

    def finLinea(self):
        ok = (self.ayudas + self.errores) == 0
        self.dbwashing.done_tactic(self.engine, ok)
        self.num_lines = self.engine.numTactics()

        self.estado = kFinJuego
        self.desactivaTodas()

        liOpciones = [k_mainmenu, k_configurar, k_utilidades]

        if self.num_lines:
            liOpciones.append(k_siguiente)

        self.pantalla.ponToolBar(liOpciones)

        self.ponRotulo1(self.line.label)

        if ok:
            r2 = "<b>%s: %d</b>" % (_("Pending"), self.num_lines)
            self.ponRotulo2(r2)
            mens = _("This line training is completed.")
            if self.num_lines == 0:
                mens = "%s\n%s" % (mens, _("You have solved all puzzles"))

            self.mensajeEnPGN(mens)
        else:
            QTUtil2.mensError(self.pantalla, "%s: %d, %s: %d" % (_("Errors"), self.errores, _("Hints"), self.ayudas))

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            self.errores += 1
            return False

        movimiento = jg.movimiento().lower()
        if movimiento == self.line.get_move(self.num_move).lower():
            self.movimientosPiezas(jg.liMovs)
            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, True)
            self.error = ""
            self.time_used += self.timekeeper.stop()
            self.siguienteJugada()
            return True

        self.errores += 1
        self.erroresEsteMov += 1
        self.sigueHumano()
        return False

    def masJugada(self, jg, siNuestra):
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.append_jg(jg)

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

    def mueveRival(self, desde, hasta, coronacion):
        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, False)
        self.movimientosPiezas(jg.liMovs, True)
        self.error = ""

    def ayuda(self):
        self.ponRotulo1(self.line.label)
        self.ayudas += 1
        mov = self.line.get_move(self.num_move).lower()
        self.tablero.markPosition(mov[:2])
        self.ayudasEsteMov += 1
        if self.ayudasEsteMov > 1 and self.erroresEsteMov > 0:
            self.tablero.ponFlechasTmp([(mov[:2], mov[2:], True), ], 1200 )

    def finPartida(self):
        self.procesador.inicio()
        self.procesador.showWashing()

    def finalX(self):
        self.procesador.inicio()
        return False


class GestorWashingCreate(Gestor.Gestor):
    def inicio(self, dbwashing, washing, engine):
        self.dbwashing = dbwashing
        self.washing = washing

        self.engine = engine

        self.tipoJuego = kJugWashingCreate

        self.resultado = None
        self.siJuegaHumano = False
        self.estado = kJugando
        self.timekeeper = Util.Timekeeper()

        siBlancas = self.engine.color
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas
        self.siCompetitivo = True

        self.apertura = Apertura.AperturaPol(30, self.engine.elo)

        self.siTutorActivado = True
        self.siAnalizando = False
        # self.pantalla.ponActivarTutor(self.siTutorActivado)

        rival = self.configuracion.buscaRival(self.engine.clave)

        self.xrival = self.procesador.creaGestorMotor(rival, None, None)
        self.xrival.siBlancas = self.siRivalConBlancas
        self.rmRival = None
        self.tmRival = 15.0*60.0*engine.elo/3000.0

        self.xtutor.maximizaMultiPV()
        self.siAnalizadoTutor = False

        self.pantalla.activaJuego(True, False, False)
        self.quitaAyudas()
        li = [k_mainmenu, k_reiniciar, k_atras]
        self.pantalla.ponToolBar(li)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)

        self.ponRotulo1("%s: %s\n%s: %s\n %s: %s" % (_("Rival"), self.engine.nombre,
                                            _("Task"), self.engine.lbState(),
                                            _("Tutor"), self.xtutor.nombre))
        self.ponRotuloDatos()

        self.ponCapInfoPorDefecto()

        self.pgnRefresh(True)

        game = dbwashing.restoreGame(engine)
        if game:
            if not game.siTerminada():
                self.partida = game
                self.ponteAlFinal()
                self.pantalla.base.pgnRefresh()

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponRotuloDatos(self):
        datos = "%s: %d | %s: %d/%d | %s: %s" % (
                    _("Games"), self.engine.games,
                    _("Hints"), self.engine.hints_current, self.engine.hints,
                    _("Time"), self.engine.lbTime()
        )
        self.ponRotulo2(datos)

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

    def juegaRival(self):
        self.pensando(True)
        self.desactivaTodas()

        desde = hasta = coronacion = ""
        siEncontrada = False

        if self.apertura:
            siEncontrada, desde, hasta, coronacion = self.apertura.juegaMotor(self.fenUltimo())
            if not siEncontrada:
                self.apertura = None

        if siEncontrada:
            self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
            self.rmRival.desde = desde
            self.rmRival.hasta = hasta
            self.rmRival.coronacion = coronacion

        else:
            self.rmRival = self.xrival.juegaTiempo(self.tmRival, self.tmRival, 0)

        self.pensando(False)

        siBien, self.error, jg = Jugada.dameJugada(self.partida.ultPosicion, self.rmRival.desde, self.rmRival.hasta, self.rmRival.coronacion)
        if siBien:
            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)
            return True
        else:
            return False

    def juegaHumano(self, siBlancas):
        self.siJuegaHumano = True
        self.analizaInicio()
        self.timekeeper.start()
        self.activaColor(siBlancas)

    def procesarAccion(self, clave):
        if clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_atras:
            self.atras()

        elif clave == k_mainmenu:
            self.finalX()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        else:
            self.rutinaAccionDef(clave)

    def analizaInicio(self):
        self.siAnalizando = False
        self.siAnalizadoTutor = False
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
                if self.xtutor.motor:
                    mrm = self.xtutor.ac_estado()
                    if mrm:
                        QtCore.QTimer.singleShot(1000, self.analizaSiguiente)

    def analizaFinal(self):
        estado = self.siAnalizando
        self.siAnalizando = False
        if self.siAnalizadoTutor:
            return
        if self.continueTt and estado:
            self.pensando(True)
            self.mrmTutor = self.xtutor.ac_final(max(self.xtutor.motorTiempoJugada,5000))
            self.pensando(False)
        else:
            self.mrmTutor = self.analizaTutor()

    def analizaTerminar(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.ac_final(-1)

    def sigueHumanoAnalisis(self):
        self.analizaInicio()
        Gestor.Gestor.sigueHumano(self)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        movimiento = jg.movimiento()
        self.addTime()

        siAnalisis = False

        siElegido = False

        if self.apertura:
            fenBase = self.fenUltimo()
            if self.apertura.compruebaHumano(fenBase, desde, hasta):
                siElegido = True
            else:
                self.apertura = None

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        self.analizaFinal()  # tiene que acabar siempre
        if not siElegido:
            rmUser, n = self.mrmTutor.buscaRM(movimiento)
            if not rmUser:
                rmUser = self.xtutor.valora(self.partida.ultPosicion, desde, hasta, jg.coronacion)
                if not rmUser:
                    self.sigueHumanoAnalisis()
                    return False
                self.mrmTutor.agregaRM(rmUser)
            siAnalisis = True
            pointsBest, pointsUser = self.mrmTutor.difPointsBest(movimiento)
            if (pointsBest - pointsUser) > 0:
                if not jg.siJaqueMate:
                    tutor = Tutor.Tutor(self, self, jg, desde, hasta, False)
                    if tutor.elegir(True):
                        self.reponPieza(desde)
                        desde = tutor.desde
                        hasta = tutor.hasta
                        coronacion = tutor.coronacion
                        siBien, mens, jgTutor = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
                        if siBien:
                            jg = jgTutor
                            self.addHint()
                    del tutor

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

    def finalizar(self):
        self.analizaTerminar()
        self.pantalla.activaJuego(False, False)
        self.quitaCapturas()
        self.procesador.inicio()
        self.procesador.showWashing()

    def finalX(self):
        if self.partida.numJugadas() > 0:
            self.addTime()
            self.saveGame(False)
        self.finalizar()

    def addHint(self):
        self.dbwashing.addHint()
        self.ponRotuloDatos()

    def addTime(self):
        secs = self.timekeeper.stop()
        if secs:
            self.dbwashing.addTime(secs)
            self.ponRotuloDatos()

    def addGame(self):
        self.dbwashing.addGame()
        self.ponRotuloDatos()

    def saveGame(self, siFinal):
        self.dbwashing.saveGame(self.partida, siFinal)

    def cancelGame(self):
        self.dbwashing.saveGame(None, False)
        self.addGame()

    def atras(self):
        if self.partida.numJugadas():
            self.analizaTerminar()
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.partida.asignaApertura()
            self.ponteAlFinal()
            self.apertura = Apertura.AperturaPol(30, self.engine.elo)
            self.siAnalizadoTutor = False
            self.addHint()
            self.addTime()
            self.refresh()
            self.siguienteJugada()

    def reiniciar(self):
        if not QTUtil2.pregunta(self.pantalla, _("Restart the game?")):
            return

        self.addTime()
        self.addGame()
        self.partida.reset()
        self.dbwashing.saveGame(None, False)

        self.inicio(self.dbwashing, self.washing, self.engine)

    def ponResultado(self, quien):
        self.estado = kFinJuego
        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.beepResultado(quien)

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

        self.guardarGanados(quien == kGanamos)
        self.mensajeEnPGN(mensaje)
        self.estado = kFinJuego
        self.desactivaTodas()
        liOpciones = [k_mainmenu, k_configurar, k_utilidades]
        if quien != kGanamos:
            liOpciones.insert(1, k_reiniciar)
        self.pantalla.ponToolBar(liOpciones)
        self.quitaAyudas()

        if quien == kGanamos:
            self.saveGame(True)
        else:
            self.cancelGame()

    def analizaPosicion(self, fila, clave):
        if self.estado != kFinJuego:
            return
        Gestor.Gestor.analizaPosicion(self, fila, clave)
