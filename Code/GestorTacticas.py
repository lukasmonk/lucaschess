import time

from PyQt4.QtCore import Qt

from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code import PGN
from Code.QT import DatosNueva
from Code.QT import Iconos
from Code.QT import PantallaGM
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.Constantes import *


class GestorTacticas(Gestor.Gestor):
    def inicio(self, tactica, posSiguiente=None):

        if hasattr(self, "reiniciando"):
            if self.reiniciando:
                return
        self.reiniciando = True

        self.tactica = tactica

        liOrden = self.tactica.listaOrden()

        self.numPosiciones = self.tactica.numPosiciones()
        self.posActual = self.tactica.posActual()

        self.siError = False

        numEnt = liOrden[self.posActual]

        self.siPenalizable = True
        self.puestosPenalizacion = self.tactica.puestosPenalizacion(self.posActual, len(liOrden))

        self.pointView = self.tactica.pointView()

        self.siSaltoAutomatico = self.tactica.siSaltoAutomatico()

        txtEntreno = self.tactica.unFNS(numEnt)

        if posSiguiente is None:
            self.posSiguiente = self.posActual + 1

        li = txtEntreno.split("|")

        fenInicial = li[0]
        if fenInicial.endswith(" 0"):
            fenInicial = fenInicial[:-1] + "1"

        self.fenInicial = fenInicial

        self.dicEtiquetasPGN = None
        solucion = None
        siPartidaOriginal = False
        nli = len(li)
        if nli >= 2:
            etiDirigido = li[1]

            # # Solucion
            if nli >= 3:
                solucion = li[2]
                if solucion:
                    self.dicDirigidoFen = PGN.leeEntDirigido(fenInicial, solucion)

                # Partida original
                if nli >= 4:
                    pgn = PGN.UnPGN()
                    if nli > 4:
                        txt = "|".join(li[3:])
                    else:
                        txt = li[3]
                    txt = txt.replace("]", "]\n").replace(" [", "[")
                    pgn.leeTexto(txt)
                    partida = pgn.partida
                    siEstaFen = False
                    njug = partida.numJugadas()
                    for n in range(njug - 1, -1, -1):
                        jg = partida.jugada(n)
                        if jg.posicion.fen() == fenInicial:
                            siEstaFen = True
                            if n + 1 != njug:
                                partida.liJugadas = partida.liJugadas[:n + 1]
                                partida.ultPosicion = jg.posicion.copia()
                            break
                    if siEstaFen:
                        siPartidaOriginal = True
                        self.partida = partida
                        self.pgn.partida = partida
                        self.dicEtiquetasPGN = pgn.dic
                        # for k, v in pgn.dic.iteritems():
                        # if k.upper() != "FEN":
                        # if etiDirigido:
                        # etiDirigido += "<br>"
                        # etiDirigido += "%s: <b>%s</b>"%(k,v)

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fenInicial)

        self.fen = fenInicial

        siBlancas = cp.siBlancas

        if self.pointView:
            siBlancas = self.pointView == 1

        if not siPartidaOriginal:
            self.partida.reset(cp)
            if solucion:
                tmp_pgn = PGN.UnPGN()
                tmp_pgn.leeTexto('[FEN "%s"]\n%s' % (fenInicial, solucion))
                if tmp_pgn.partida.firstComment:
                    self.partida.setFirstComment(tmp_pgn.partida.firstComment, True)

        self.partida.pendienteApertura = False

        self.tipoJuego = kJugEntTac

        self.siJuegaHumano = False
        self.siJuegaPorMi = True

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.liVariantes = []

        self.rmRival = None

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(False)

        self.ayudasPGN = 0

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True, True)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)
        # txttact = "%dx%d"%(tactica.numFNS(),len(tactica.JUMPS)+1)
        # txttact = "(%s)x%d"%(txttact,len(tactica.REPEAT))
        self.siShowText = tactica.siShowText()
        titulo = "<b>%s</b><br>" % (self.tactica.titulo,)  # txttact)
        self.tituloAmpliado = titulo + etiDirigido
        if self.siShowText:
            titulo = self.tituloAmpliado
        else:
            self.siShowText = len(etiDirigido) == 0

        liOpciones = [k_mainmenu, k_configurar]
        if not self.siShowText:
            liOpciones.append(k_showtext)
        if self.dicEtiquetasPGN:
            liOpciones.append(k_pgnInformacion)
        self.pantalla.ponToolBar(liOpciones)
        self.ponRotulo1(titulo)
        self.pgnRefresh(True)
        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        self.siSeguirJugando = False
        tiempo = self.configuracion.tiempoTutor
        if tiempo < 1000 or tiempo > 5000:
            tiempo = 5000
        self.xrival = self.procesador.creaGestorMotor(self.configuracion.tutor, tiempo, None)

        self.reiniciando = False

        self.rivalPensando = False

        if siPartidaOriginal:
            self.repiteUltimaJugada()

        self.ponSiguiente()

        self.estado = kJugando
        self.siguienteJugada()

    def ponSiguiente(self):
        if self.posSiguiente == self.numPosiciones:
            txt = "%s: <big>%s</big>" % (_("Next"), _("Endgame"))
            color = "DarkMagenta"
        else:
            txt = "%s: %d" % (_("Next"), self.posSiguiente + 1)
            color = "red" if self.posSiguiente <= self.posActual else "blue"

        self.ponRotulo2(
                '<table border="1" with="100%%" align="center" cellpadding="5" cellspacing="0"><tr><td  align="center"><h4>%s: %d/%d<br><font color="%s">%s</font></h4></td></tr></table>' % (
                    _("Current position"), self.posActual + 1, self.numPosiciones, color, txt))

    def ponPenalizacion(self):

        if not self.siPenalizable:  # Ya penalizado
            return
        self.siPenalizable = False
        self.posSiguiente = self.posActual - self.puestosPenalizacion
        if self.posSiguiente < 0:
            self.posSiguiente = 0

        self.tactica.ponPosActual(self.posSiguiente)
        self.ponSiguiente()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_variantes:
            self.lanzaVariantes()

        elif clave == k_configurar:
            base = _("What to do after solving")
            if self.siSaltoAutomatico:
                liMasOpciones = [("lmo_stop", "%s: %s" %(base, _("Stop")), Iconos.PuntoRojo())]
            else:
                liMasOpciones = [("lmo_jump", "%s: %s" %(base, _("Jump to the next")), Iconos.PuntoVerde())]

            resp = self.configurar(siSonidos=True, siCambioTutor=False, liMasOpciones=liMasOpciones)
            if resp in ("lmo_stop", "lmo_jump"):
                self.siSaltoAutomatico = resp == "lmo_jump"
                self.tactica.setSaltoAutomatico(self.siSaltoAutomatico)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_siguiente:
            self.ent_siguiente()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_cambiar:
            self.cambiar()

        elif clave == k_pgnInformacion:
            self.pgnInformacionMenu(self.dicEtiquetasPGN)

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        elif clave == k_showtext:
            self.ponRotulo1(self.tituloAmpliado)
            self.pantalla.mostrarOpcionToolbar(k_showtext, False)
            self.siShowText = True
        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def controlTeclado(self, nkey):
        if nkey in (Qt.Key_Plus, Qt.Key_PageDown):
            if self.estado == kFinJuego:
                self.ent_siguiente()
        elif nkey == Qt.Key_T:
            self.saveSelectedPosition(self.fenInicial)

    def listHelpTeclado(self):
        return [
            ("+/%s"%_("Page Down"), _("Next position")),
            ("T",  _("Save position in 'Selected positions' file")),
        ]

    def reiniciar(self):
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.fen)
        self.partida.reset(cp)
        self.estado = kJugando
        self.ponteAlFinal()
        self.ponRevision(False)
        self.siSeguirJugando = False
        self.siguienteJugada()

    def ent_siguiente(self):
        self.siError = False # Para controlar salto, no es automatico si se produce un error
        if self.posSiguiente == self.numPosiciones:
            self.finPartida()
        else:
            self.inicio(self.tactica)

    def finPartida(self):
        self.procesador.inicio()
        self.procesador.entrenamientos.entrenaTactica(self.tactica)

    def finalX(self):
        self.finPartida()
        return False

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.compruebaComentarios()

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        if self.siTerminada() or (self.partida.numJugadas() and self.partida.jugada(-1).siTablas()):
            if not self.siSeguirJugando:
                self.finLinea()
            return

        siRival = siBlancas == self.siRivalConBlancas

        if siRival:
            fen = self.partida.ultPosicion.fen()
            siPiensaRival = False
            if fen in self.dicDirigidoFen:
                liOpciones = self.dicDirigidoFen[fen]
                if liOpciones:
                    liJugadas = []
                    siEncontradoMain = False
                    for siMain, jg in liOpciones:
                        desde, hasta, coronacion = jg.desde, jg.hasta, jg.coronacion
                        if self.siPenalizable and siMain:
                            siEncontradoMain = True
                            break
                        rotulo = _("Main line") if siMain else ""
                        pgn = self.partida.ultPosicion.pgn(desde, hasta, coronacion)
                        liJugadas.append((desde, hasta, coronacion, rotulo, pgn))
                    if self.siPenalizable and not siEncontradoMain:
                        return self.finLinea()
                    if len(liJugadas) > 1 and not self.siPenalizable:
                        desde, hasta, coronacion = PantallaGM.eligeJugada(self, liJugadas, False)
                    if len(liOpciones) > 1:
                        self.guardaVariantes()
                else:
                    if self.siSeguirJugando:
                        siPiensaRival = True
                    else:
                        return self.finLinea()
            else:
                if self.siSeguirJugando:
                    siPiensaRival = True
                else:
                    return self.finLinea()

            if siPiensaRival:
                self.pensando(True)
                self.desactivaTodas()

                self.rmRival = self.xrival.juega()

                self.pensando(False)
                desde, hasta, coronacion = self.rmRival.desde, self.rmRival.hasta, self.rmRival.coronacion

            if self.mueveRival(desde, hasta, coronacion):
                self.rivalPensando = False
                self.siguienteJugada()

        else:

            if not self.siSeguirJugando:
                fen = self.partida.ultPosicion.fen()
                if fen not in self.dicDirigidoFen:
                    return self.finLinea()

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

            if not self.siSeguirJugando:
                self.iniReloj = time.time()

    def finLinea(self):
        self.compruebaComentarios()
        self.estado = kFinJuego
        self.desactivaTodas()

        self.tactica.ponPosActual(self.posSiguiente)
        if self.tactica.terminada():
            self.finalEntrenamiento()

        self.siPenalizable = False

        if self.siSaltoAutomatico and not self.siError:
            self.ent_siguiente()
        else:
            QTUtil2.mensajeTemporal(self.pantalla, _("This line training is completed."), 0.7)
            liOpciones = [k_mainmenu, k_reiniciar, k_cambiar]

            if not (self.siTerminada() and len(self.liVariantes) == 0):
                liOpciones.append(k_variantes)

            if not self.siShowText:
                liOpciones.append(k_showtext)  # Si no se ha mostrado ahora es el momento

            if self.dicEtiquetasPGN:
                liOpciones.append(k_pgnInformacion)

            liOpciones.extend([k_configurar, k_utilidades, k_siguiente])

            self.pantalla.ponToolBar(liOpciones)

        return

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        movimiento = jg.movimiento()

        fen = self.partida.ultPosicion.fen()
        if fen in self.dicDirigidoFen:
            liOpciones = self.dicDirigidoFen[fen]
            if len(liOpciones) > 1:
                self.guardaVariantes()
            liMovs = []
            siEsta = False
            siPenalizar = True
            posMain = None
            for siMain, jg1 in liOpciones:
                mv = jg1.movimiento()
                if siMain:
                    posMain = mv[:2]
                if mv.lower() == movimiento.lower():
                    siEsta = siMain if self.siPenalizable else True
                    siPenalizar = False
                    if siEsta:
                        break
                liMovs.append((jg1.desde, jg1.hasta, siMain))

            if not siEsta:
                self.siError = True
                self.tactica.nuevoError()
                self.ponPosicion(self.partida.ultPosicion)
                if posMain and posMain != movimiento[:2]:
                    self.tablero.markPosition(posMain)
                else:
                    self.tablero.ponFlechasTmp(liMovs)

                self.sigueHumano()
                if siPenalizar:
                    self.ponPenalizacion()
                return False

        if not self.siSeguirJugando:
            segundos = time.time() - self.iniReloj
            self.tactica.masSegundos(segundos)
        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):

        # Preguntamos al mono si hay movimiento
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
        if siBien:
            self.partida.ultPosicion = jg.posicion
            if not self.siTutorActivado:
                self.siAnalizadoTutor = False
            else:

                fen = self.partida.ultPosicion.fen()
                if not (fen in self.dicDirigidoFen):
                    self.analizaTutor()  # Que analice antes de activar humano, para que no tenga que esperar
                    self.siAnalizadoTutor = True

            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)

            self.error = ""

            return True
        else:
            self.error = mens
            return False

    def cambiar(self):

        if self.posSiguiente >= 0:
            pos = DatosNueva.numEntrenamiento(self.pantalla, self.tactica.tituloAmpliado(), self.numPosiciones,
                                              pos=self.posSiguiente)
            if pos is not None:
                self.posSiguiente = pos - 1
                self.tactica.ponPosActual(self.posSiguiente)

                self.ent_siguiente()

    def lanzaVariantes(self):

        icoNegro = Iconos.PuntoNegro()
        icoVerde = Iconos.PuntoVerde()
        icoSeguimos = Iconos.PuntoAzul()

        menu = QTVarios.LCMenu(self.pantalla)
        for n, (tit, txtp, siBlancas) in enumerate(self.liVariantes):
            menu.opcion(n, tit, icoVerde if siBlancas else icoNegro)
            menu.separador()

        if not self.siTerminada() and not self.siSeguirJugando:
            menu.opcion(-1, _("Do you want to continue playing?"), icoSeguimos)

        resp = menu.lanza()
        if resp is not None:
            if resp == -1:
                self.siSeguirJugando = True
            else:
                self.partida.recuperaDeTexto(self.liVariantes[resp][1])
            self.estado = kJugando
            self.ponteAlFinal()
            self.siguienteJugada()

    def finalEntrenamiento(self):
        self.tactica.finalEntrenamiento()
        mensaje = "<big>%s<br>%s</big>" % (_("Congratulations goal achieved"), _("Endgame"))
        self.mensajeEnPGN(mensaje)
        self.finPartida()

    def guardaVariantes(self):
        njug = self.partida.numJugadas()
        siBlancas = self.partida.siBlancas()
        if njug:
            jg = self.partida.last_jg()
            numj = self.partida.primeraJugada() + (njug + 1) / 2 - 1
            titulo = "%d." % numj
            if siBlancas:
                titulo += "... "
            titulo += jg.pgnSP()
        else:
            titulo = _("Start position")

        for tit, txtp, siBlancas in self.liVariantes:
            if titulo == tit:
                return
        self.liVariantes.append((titulo, self.partida.guardaEnTexto(), siBlancas))

    def compruebaComentarios(self):
        if not self.partida.liJugadas:
            return
        fen = self.partida.ultPosicion.fen()
        if fen not in self.dicDirigidoFen:
            return
        jg = self.partida.last_jg()
        mv = jg.movimiento()
        fen = jg.posicion.fen()
        for k, liOpciones in self.dicDirigidoFen.iteritems():
            for siMain, jg1 in liOpciones:
                if jg1.posicion.fen() == fen and jg1.movimiento() == mv:
                    if jg1.critica and not jg.critica:
                        jg.critica = jg1.critica
                    if jg1.comentario and not jg.comentario:
                        jg.comentario = jg1.comentario
                    if jg1.variantes and not jg.variantes:
                        jg.variantes = jg1.variantes
                    break
