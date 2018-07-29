import copy
import random

from Code import Apertura
from Code import GM
from Code import Gestor
from Code import Jugada
from Code.QT import PantallaGM
from Code.QT import PantallaJuicio
from Code.QT import QTUtil2
from Code import Util
from Code.Constantes import *


class GestorGM(Gestor.Gestor):
    def inicio(self, record):

        self.tipoJuego = kJugGM

        self.ayudas = 9999  # Para que analice sin problemas

        self.puntos = 0

        self.record = record

        self.siWoman = self.record.siWoman

        self.gm = record.gm
        self.siBlancas = record.siBlancas
        self.modo = record.modo
        self.siJuez = record.siJuez
        self.showevals = record.showevals
        self.motor = record.motor
        self.tiempo = record.tiempo
        self.depth = record.depth
        self.multiPV = record.multiPV
        self.mostrar = record.mostrar
        self.jugContrario = record.jugContrario
        self.jugInicial = record.jugInicial
        self.partidaElegida = record.partidaElegida
        self.bypassBook = record.bypassBook
        self.apertura = record.apertura
        self.onBypassBook = True if self.bypassBook else False
        if self.onBypassBook:
            self.bypassBook.polyglot()
        self.onApertura = True if self.apertura else False

        self.siAnalizando = False

        if self.siJuez:
            self.puntos = 0
            tutor = self.configuracion.buscaRivalExt(self.motor)
            t_t = self.tiempo * 100
            self.xtutor = self.procesador.creaGestorMotor(tutor, t_t, self.depth)
            self.xtutor.actMultiPV(self.multiPV)
            self.analisis = None

        self.book = Apertura.AperturaPol(999)

        self.pensando(True)

        default = "WGM" if self.siWoman else "GM"
        carpeta = default if self.modo == "estandar" else self.configuracion.dirPersonalTraining
        self.motorGM = GM.GM(carpeta, self.gm)
        self.motorGM.colorFilter(self.siBlancas)
        if self.partidaElegida is not None:
            self.motorGM.ponPartidaElegida(self.partidaElegida)

        self.siJugamosConBlancas = self.siBlancas
        self.siRivalConBlancas = not self.siBlancas
        self.pensando(False)

        self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_configurar, k_utilidades))
        self.pantalla.activaJuego(True, False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.quitaAyudas()
        self.ponPiezasAbajo(self.siBlancas)
        dic = GM.dicGM(self.siWoman)
        self.nombreGM = dic[self.gm.lower()] if self.modo == "estandar" else self.gm
        rot = _("Woman Grandmaster") if self.siWoman else _("Grandmaster")
        rotulo1 = rot + ": <b>%s</b>" if self.modo == "estandar" else "<b>%s</b>"
        self.ponRotulo1(rotulo1 % self.nombreGM)

        self.nombreRival = ""
        self.textoPuntuacion = ""
        self.ponRotuloSecundario()
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        self.estado = kJugando

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponRotuloSecundario(self):
        self.ponRotulo2(self.nombreRival + "<br><br>" + self.textoPuntuacion)

    def procesarAccion(self, clave):

        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        if self.estado == kFinJuego:
            return True
        return self.finPartida()

    def finPartida(self):
        self.analizaTerminar()
        siJugadas = self.partida.numJugadas() > 0
        if siJugadas and self.estado != kFinJuego:
            self.resultado = kDesconocido
            self.partida.last_jg().siDesconocido = True
            # self.guardarNoTerminados( )
            self.ponFinJuego()
        self.procesador.inicio()

        return False

    def reiniciar(self):
        if QTUtil2.pregunta(self.pantalla, _("Restart the game?")):
            self.analizaTerminar()
            self.partida.reset()
            self.inicio(self.record)

    def analizaInicio(self):
        if not self.siTerminada():
            self.xtutor.ac_inicio(self.partida)
            self.siAnalizando = True

    def analizaEstado(self):
        self.xtutor.motor.ac_lee()
        self.mrm = copy.deepcopy(self.xtutor.ac_estado())
        return self.mrm

    def analizaMinimo(self, minTime):
        self.mrm = copy.deepcopy(self.xtutor.ac_minimo(minTime, False))
        return self.mrm

    def analizaFinal(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.ac_final(-1)

    def siguienteJugada(self):
        self.analizaTerminar()
        self.desactivaTodas()

        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        if (self.partida.numJugadas() > 0) and self.motorGM.isFinished():
            self.ponResultado()
            return

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        if self.jugInicial > 1:
            siJugInicial = (self.partida.numJugadas() / 2 + 1) <= self.jugInicial
        else:
            siJugInicial = False

        liAlternativas = self.motorGM.alternativas()
        nliAlternativas = len(liAlternativas)

        # Movimiento automatico
        if siJugInicial or self.onApertura or self.onBypassBook:
            siBuscar = True
            if self.onApertura:
                liPV = self.apertura.a1h8.split(" ")
                nj = self.partida.numJugadas()
                if len(liPV) > nj:
                    move = liPV[nj]
                    if move in liAlternativas:
                        siBuscar = False
                    else:
                        self.onApertura = False
                else:
                    self.onApertura = False

            if siBuscar:
                if self.onBypassBook:
                    liJugadas = self.bypassBook.miraListaJugadas(self.fenUltimo())
                    liN = []
                    for desde, hasta, coronacion, pgn, peso in liJugadas:
                        move = desde + hasta + coronacion
                        if move in liAlternativas:
                            liN.append(move)
                    if liN:
                        siBuscar = False
                        nliAlternativas = len(liN)
                        if nliAlternativas > 1:
                            pos = random.randint(0, nliAlternativas - 1)
                            move = liN[pos]
                        else:
                            move = liN[0]
                    else:
                        self.onBypassBook = None

            if siBuscar:
                if siJugInicial:
                    siBuscar = False
                    if nliAlternativas > 1:
                        pos = random.randint(0, nliAlternativas - 1)
                        move = liAlternativas[pos]
                    elif nliAlternativas == 1:
                        move = liAlternativas[0]

            if not siBuscar:
                self.mueveRival(move)
                self.siguienteJugada()
                return

        if siRival:
            if nliAlternativas > 1:
                if self.jugContrario:
                    liJugadas = self.motorGM.dameJugadasTXT(self.partida.ultPosicion, False)
                    desde, hasta, coronacion = PantallaGM.eligeJugada(self, liJugadas, False)
                    move = desde + hasta + coronacion
                else:
                    pos = random.randint(0, nliAlternativas - 1)
                    move = liAlternativas[pos]
            else:
                move = liAlternativas[0]

            self.mueveRival(move)
            self.siguienteJugada()

        else:
            self.siJuegaHumano = True
            self.pensando(True)
            self.analizaInicio()
            self.activaColor(siBlancas)
            self.pensando(False)

    def analizaTerminar(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.terminar()

    def mueveHumano(self, desde, hasta, coronacion=None):
        jgUsu = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgUsu:
            return False

        movimiento = jgUsu.movimiento()
        posicion = self.partida.ultPosicion
        isValid = self.motorGM.isValidMove(movimiento)
        analisis = None

        if not isValid:
            self.tablero.ponPosicion(posicion)
            self.tablero.activaColor(self.siJugamosConBlancas)
            liJugadas = self.motorGM.dameJugadasTXT(posicion, True)
            desdeGM, hastaGM, coronacionGM = PantallaGM.eligeJugada(self, liJugadas, True)
            siAnalizaJuez = self.siJuez
            if siAnalizaJuez:
                if self.book:
                    fen = self.fenUltimo()
                    siH = self.book.compruebaHumano(fen, desde, hasta)
                    siGM = self.book.compruebaHumano(fen, desdeGM, hastaGM)
                    if siGM and siH:
                        siAnalizaJuez = False
                    else:
                        self.book = False
        else:
            siAnalizaJuez = self.siJuez and self.mostrar is None  # None es ver siempre False no ver nunca True ver si diferentes
            if len(movimiento) == 5:
                coronacion = movimiento[4].lower()
            desdeGM, hastaGM, coronacionGM = desde, hasta, coronacion

        siBien, mens, jgGM = Jugada.dameJugada(posicion, desdeGM, hastaGM, coronacionGM)
        movGM = jgGM.pgnSP()
        movUsu = jgUsu.pgnSP()

        if siAnalizaJuez:
            um = QTUtil2.analizando(self.pantalla)
            mrm = self.analizaMinimo(self.tiempo * 100)

            rmUsu, nada = mrm.buscaRM(jgUsu.movimiento())
            if rmUsu is None:
                um = QTUtil2.analizando(self.pantalla)
                self.analizaFinal()
                rmUsu = self.xtutor.valora(posicion, desde, hasta, coronacion)
                mrm.agregaRM(rmUsu)
                self.analizaInicio()
                um.final()

            rmGM, posGM = mrm.buscaRM(jgGM.movimiento())
            if rmGM is None:
                self.analizaFinal()
                rmGM = self.xtutor.valora(posicion, desdeGM, hastaGM, coronacionGM)
                posGM = mrm.agregaRM(rmGM)
                self.analizaInicio()

            um.final()

            analisis = mrm, posGM
            dpts = rmUsu.puntosABS() - rmGM.puntosABS()

            if self.mostrar is None or (self.mostrar == True and not isValid):
                w = PantallaJuicio.WJuicio(self, self.xtutor, self.nombreGM, posicion, mrm, rmGM, rmUsu, analisis, siCompetitivo=not self.showevals)
                w.exec_()

                rm, posGM = w.analisis[0].buscaRM(jgGM.movimiento())
                analisis = w.analisis[0], posGM

                dpts = w.difPuntos()

            self.puntos += dpts

            comentario0 = "<b>%s</b> : %s = %s<br>" % (self.configuracion.jugador, movUsu, rmUsu.texto())
            comentario0 += "<b>%s</b> : %s = %s<br>" % (self.nombreGM, movGM, rmGM.texto())
            comentario1 = "<br><b>%s</b> = %+d<br>" % (_("Difference"), dpts)
            comentario2 = "<b>%s</b> = %+d<br>" % (_("Points accumulated"), self.puntos)
            self.textoPuntuacion = comentario2
            self.ponRotuloSecundario()

            if not isValid:
                jgGM.comentario = (comentario0 + comentario1 + comentario2).replace("<b>", "").replace("</b>", "").replace(
                        "<br>", "\n")

        self.analizaFinal()

        self.movimientosPiezas(jgGM.liMovs)

        self.partida.ultPosicion = jgGM.posicion
        jgGM.analisis = analisis
        self.masJugada(jgGM, True)
        self.error = ""
        self.siguienteJugada()
        return True

    def analizaPosicion(self, fila, clave):
        if self.estado != kFinJuego:
            return
        Gestor.Gestor.analizaPosicion(self, fila, clave)

    def mueveRival(self, move):
        desde = move[:2]
        hasta = move[2:4]
        coronacion = move[4:]

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

    def masJugada(self, jg, siNuestra):

        # Preguntamos al motor si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        txt = self.motorGM.rotuloPartidaSiUnica(siGM=self.modo == "estandar")
        if txt:
            self.nombreRival = txt
        self.ponRotuloSecundario()

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.motorGM.play(jg.movimiento())

        self.ponPosicionDGT()

    def ponResultado(self):
        self.estado = kFinJuego
        self.tablero.desactivaTodas()

        mensaje = _("Game ended")

        txt, porc, txtResumen = self.motorGM.resultado(self.partida)
        mensaje += "<br><br>" + txt
        if self.siJuez:
            mensaje += "<br><br><b>%s</b> = %+d<br>" % (_("Points accumulated"), self.puntos)

        # QTUtil2.mensaje(self.pantalla, mensaje, siResalta=False)
        self.mensajeEnPGN(mensaje)

        dbHisto = Util.DicSQL(self.configuracion.ficheroGMhisto)

        gmK = "P_%s" % self.gm if self.modo == "personal" else self.gm

        dic = {}
        dic["FECHA"] = Util.hoy()
        dic["PUNTOS"] = self.puntos
        dic["PACIERTOS"] = porc
        dic["JUEZ"] = self.motor
        dic["TIEMPO"] = self.tiempo
        dic["RESUMEN"] = txtResumen

        liHisto = dbHisto[gmK]
        if liHisto is None:
            liHisto = []
        liHisto.insert(0, dic)
        dbHisto[gmK] = liHisto
        dbHisto.pack()
        dbHisto.close()
