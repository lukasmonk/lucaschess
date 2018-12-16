import codecs
import os

from PyQt4.QtCore import Qt

from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code import PGN
from Code import Partida
from Code.QT import DatosNueva
from Code.QT import Iconos
from Code.QT import PantallaGM
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import TrListas
from Code import Tutor
from Code import Util
from Code import VarGen
from Code.Constantes import *


class GestorEntPos(Gestor.Gestor):
    def ponEntreno(self, entreno):
        # Guarda el ultimo entrenamiento en el db de entrenos
        self.entreno = entreno

    def guardaPosicion(self, posEntreno):
        db = Util.DicSQL(self.configuracion.ficheroTrainings)
        data = db[self.entreno]
        if data is None:
            data = {}
        data["POSULTIMO"] = posEntreno
        db[self.entreno] = data
        db.close()

    def inicio(self, posEntreno, numEntrenos, titEntreno, liEntrenos, siTutorActivado=None, saltoAutomatico=False):
        if hasattr(self, "reiniciando"):
            if self.reiniciando:
                return
        self.reiniciando = True

        if siTutorActivado is None:
            siTutorActivado = (VarGen.dgtDispatch is None) and self.configuracion.tutorActivoPorDefecto

        self.posEntreno = posEntreno
        self.guardaPosicion(posEntreno)
        self.numEntrenos = numEntrenos
        self.titEntreno = titEntreno
        self.liEntrenos = liEntrenos
        self.saltoAutomatico = saltoAutomatico

        self.liHistorico = [self.posEntreno]

        self.ayudas = 99999

        fenInicial = self.liEntrenos[self.posEntreno - 1].strip()
        self.fenInicial = fenInicial

        self.rivalPensando = False

        self.dicEtiquetasPGN = None

        # Dirigido
        etiDirigido = ""
        self.siDirigido = False
        self.siDirigidoSeguir = None
        self.siDirigidoVariantes = False
        solucion = None
        siPartidaOriginal = False
        if "|" in fenInicial:
            li = fenInicial.split("|")

            fenInicial = li[0]
            if fenInicial.endswith(" 0"):
                fenInicial = fenInicial[:-1] + "1"

            nli = len(li)
            if nli >= 2:
                etiDirigido = li[1]

                # # Solucion
                if nli >= 3:
                    solucion = li[2]
                    if solucion:
                        self.dicDirigidoFen = PGN.leeEntDirigido(fenInicial, solucion)
                        self.siDirigido = len(self.dicDirigidoFen) > 0

                    # Partida original
                    if nli >= 4:
                        if nli > 4:
                            txt = "|".join(li[3:])
                        else:
                            txt = li[3]
                        txt = txt.replace("]", "]\n").replace(" [", "[")
                        pgn = PGN.UnPGN()
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
                            # if etiDirigido:
                            # etiDirigido += "<br>"
                            # for k, v in pgn.dic.iteritems():
                            # if k.upper() != "FEN":
                            # if etiDirigido:
                            # etiDirigido += "<br>"
                            # etiDirigido += "%s: <b>%s</b>"%(k,v)

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fenInicial)

        self.fen = fenInicial

        siBlancas = cp.siBlancas

        if not siPartidaOriginal:
            self.partida.reset(cp)
            if solucion:
                tmp_pgn = PGN.UnPGN()
                tmp_pgn.leeTexto('[FEN "%s"]\n%s' % (fenInicial, solucion))
                if tmp_pgn.partida.firstComment:
                    self.partida.setFirstComment(tmp_pgn.partida.firstComment, True)

        self.partida.pendienteApertura = False

        self.tipoJuego = kJugEntPos

        self.siJuegaHumano = False
        self.estado = kJugando
        self.siJuegaPorMi = True

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.liVariantes = []

        self.rmRival = None

        self.siTutorActivado = siTutorActivado
        self.pantalla.ponActivarTutor(self.siTutorActivado)

        self.ayudasPGN = 0

        liOpciones = [k_mainmenu, k_cambiar, k_reiniciar, k_atras]
        if self.dicEtiquetasPGN:
            liOpciones.append(k_pgnInformacion)
        liOpciones.extend((k_configurar, k_utilidades))
        if self.numEntrenos > 1:
            liOpciones.extend((k_anterior, k_siguiente))
        self.liOpcionesToolBar = liOpciones
        self.pantalla.ponToolBar(liOpciones)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(False, False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)
        titulo = "<b>%s</b>" % TrListas.dicTraining().get(self.titEntreno, self.titEntreno)
        if etiDirigido:
            titulo += "<br>%s" % etiDirigido
        self.ponRotulo1(titulo)
        self.ponRotulo2("%d / %d" % (posEntreno, numEntrenos))
        self.pgnRefresh(True)
        QTUtil.xrefreshGUI()

        if self.xrival is None:
            self.xrival = self.procesador.creaGestorMotor(self.configuracion.tutor, self.configuracion.tiempoTutor, self.configuracion.depthTutor)

        self.siAnalizadoTutor = False

        self.ponPosicionDGT()

        if siPartidaOriginal:
            # self.ponteAlFinal()
            self.repiteUltimaJugada()

        self.reiniciando = False
        self.rivalPensando = False
        self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_atras:
            self.atras()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_variantes:
            self.lanzaVariantes()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=True)

        elif clave == k_cambiar:
            self.ent_otro()

        elif clave == k_utilidades:
            if "/Tactics/" in self.entreno:
                liMasOpciones = []
            else:
                liMasOpciones = [("tactics", _("Create tactics training"), Iconos.Tacticas()),
                                 (None, None, None)]
            liMasOpciones.append(("play", _('Play current position'), Iconos.MoverJugar()))

            resp = self.utilidades(liMasOpciones)
            if resp == "tactics":
                self.createTactics()
            elif resp == "play":
                self.jugarPosicionActual()

        elif clave == k_pgnInformacion:
            self.pgnInformacionMenu(self.dicEtiquetasPGN)

        elif clave in (k_siguiente, k_anterior):
            self.ent_siguiente(clave)

        elif clave == k_peliculaSeguir:
            self.sigue()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def reiniciar(self):
        if self.rivalPensando:
            return
        self.inicio(self.posEntreno, self.numEntrenos, self.titEntreno, self.liEntrenos, self.siTutorActivado, self.saltoAutomatico)

    def ent_siguiente(self, tipo):
        if not (self.siJuegaHumano or self.estado == kFinJuego):
            return
        pos = self.posEntreno + (+1 if tipo == k_siguiente else -1)
        if pos > self.numEntrenos:
            pos = 1
        elif pos == 0:
            pos = self.numEntrenos
        self.inicio(pos, self.numEntrenos, self.titEntreno, self.liEntrenos, self.siTutorActivado, self.saltoAutomatico)

    def controlTeclado(self, nkey):
        if nkey in (Qt.Key_Plus, Qt.Key_PageDown):
            self.ent_siguiente(k_siguiente)
        elif nkey in (Qt.Key_Minus, Qt.Key_PageUp):
            self.ent_siguiente(k_anterior)
        elif nkey == Qt.Key_T:
            li = self.fenInicial.split("|")
            li[2] = self.partida.pgnBaseRAW()
            self.saveSelectedPosition("|".join(li))

    def listHelpTeclado(self):
        return [
            ("+/%s"%_("Page Down"), _("Next position")),
            ("-/%s"%_("Page Up"), _("Previous position")),
            ("T",  _("Save position in 'Selected positions' file")),
        ]

    def finPartida(self):
        self.procesador.inicio()

    def finalX(self):
        self.finPartida()
        return False

    def atras(self):
        if self.rivalPensando:
            return
        if self.partida.numJugadas():
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.ponteAlFinal()
            self.siAnalizadoTutor = False
            self.estado = kJugando
            self.refresh()
            self.siguienteJugada()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            if self.siDirigido and self.saltoAutomatico:
                self.ent_siguiente(k_siguiente)
            return

        self.siPiensaHumano = False

        self.compruebaComentarios()

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        if self.partida.numJugadas() > 0:
            jgUltima = self.partida.last_jg()
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

        if siRival:

            self.piensaRival()

        else:

            self.piensaHumano(siBlancas)

    def piensaHumano(self, siBlancas):
        fen = self.partida.ultPosicion.fen()
        if self.siDirigido and (fen in self.dicDirigidoFen) \
                and not self.dicDirigidoFen[fen] and self.siTutorActivado:
            self.lineaTerminadaOpciones()
            return

        self.siJuegaHumano = True
        self.activaColor(siBlancas)

    def piensaRival(self):
        self.rivalPensando = True
        pensarRival = True
        fen = self.partida.ultPosicion.fen()
        if self.siDirigido and self.siTutorActivado:
            my_last_fen = self.dicDirigidoFen.keys()[-1]
            if (fen in self.dicDirigidoFen) and (fen != my_last_fen):
                liOpciones = self.dicDirigidoFen[fen]
                if liOpciones:
                    liJugadas = []
                    for siMain, jg in liOpciones:
                        desde, hasta, coronacion = jg.desde, jg.hasta, jg.coronacion
                        if not self.siDirigidoVariantes:
                            if siMain:
                                liJugadas = []
                                break
                        rotulo = _("Main line") if siMain else ""
                        pgn = self.partida.ultPosicion.pgn(desde, hasta, coronacion)
                        liJugadas.append((desde, hasta, coronacion, rotulo, pgn))
                    if len(liJugadas) > 1:
                        desde, hasta, coronacion = PantallaGM.eligeJugada(self, liJugadas, False)
                    if len(liOpciones) > 1:
                        self.guardaVariantes()
                    pensarRival = False
            if pensarRival and self.siDirigidoSeguir is None:
                self.lineaTerminadaOpciones()
                self.rivalPensando = False
                return

        if pensarRival:
            self.pensando(True)
            self.desactivaTodas()

            self.rmRival = self.xrival.juega()

            self.pensando(False)
            desde, hasta, coronacion = self.rmRival.desde, self.rmRival.hasta, self.rmRival.coronacion

        if self.mueveRival(desde, hasta, coronacion):
            self.rivalPensando = False
            self.siguienteJugada()
        else:
            self.rivalPensando = False

    def sigue(self):
        self.estado = kJugando
        self.siDirigido = False
        self.siDirigidoSeguir = True
        if k_peliculaSeguir in self.liOpcionesToolBar:
            del self.liOpcionesToolBar[self.liOpcionesToolBar.index(k_peliculaSeguir)]
            self.pantalla.ponToolBar(self.liOpcionesToolBar)
        self.siguienteJugada()

    def lineaTerminadaOpciones(self):
        self.estado = kFinJuego
        if self.saltoAutomatico:
            self.ent_siguiente(k_siguiente)
            return False
        else:
            QTUtil2.mensajeTemporal(self.pantalla, _("This line training is completed."), 0.7)
            if not self.siTerminada():
                if k_peliculaSeguir not in self.liOpcionesToolBar:
                    self.liOpcionesToolBar.insert(4, k_peliculaSeguir)
                    self.pantalla.ponToolBar(self.liOpcionesToolBar)
            return False

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        movimiento = jg.movimiento()

        siMirarTutor = self.siTutorActivado

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        if siMirarTutor:
            fen = self.partida.ultPosicion.fen()
            if self.siDirigido and fen in self.dicDirigidoFen:
                liOpciones = self.dicDirigidoFen[fen]
                if len(liOpciones) > 1:
                    self.guardaVariantes()
                liMovs = []
                siEsta = False
                posMain = None
                for siMain, jg1 in liOpciones:
                    mv = jg1.movimiento()
                    if siMain:
                        posMain = mv[:2]

                    if mv.lower() == movimiento.lower():
                        if self.siDirigidoVariantes:
                            siEsta = True
                        else:
                            siEsta = siMain
                        if siEsta:
                            break
                    liMovs.append((jg1.desde, jg1.hasta, siMain))

                if not siEsta:
                    self.ponPosicion(self.partida.ultPosicion)
                    if posMain and posMain != movimiento[:2]:
                        self.tablero.markPosition(posMain)
                    else:
                        self.tablero.ponFlechasTmp(liMovs)
                    self.sigueHumano()
                    return False

            else:
                if not self.siAnalizadoTutor:
                    self.analizaTutor()
                if self.mrmTutor.mejorMovQue(movimiento):
                    if not jg.siJaqueMate:
                        tutor = Tutor.Tutor(self, self, jg, desde, hasta, False)

                        if tutor.elegir(True):
                            self.reponPieza(desde)
                            desde = tutor.desde
                            hasta = tutor.hasta
                            coronacion = tutor.coronacion
                            siBien, mens, jgTutor = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta,
                                                                      coronacion)
                            if siBien:
                                jg = jgTutor

                        del tutor
            self.mrmTutor = None

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""

        if self.siTutorActivado and self.siDirigido and (self.partida.ultPosicion.fen() not in self.dicDirigidoFen):
            self.lineaTerminadaOpciones()

        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):

        self.partida.append_jg(jg)
        self.partida.ultPosicion = jg.posicion

        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque
            self.estado = kFinJuego

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
            self.siAnalizadoTutor = False
            self.partida.ultPosicion = jg.posicion
            if self.siTutorActivado:
                if not self.siDirigido:
                    self.analizaTutor()  # Que analice antes de activar humano, para que no tenga que esperar
                    self.siAnalizadoTutor = True

            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)

            self.error = ""

            if self.siTutorActivado and self.siDirigido and ((self.partida.ultPosicion.fen() not in self.dicDirigidoFen)):
                self.lineaTerminadaOpciones()

            return True
        else:
            self.error = mens
            return False

    def ponResultado(self, quien):
        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False
        self.estado = kFinJuego

        if quien == kTablasRepeticion:
            self.resultado = kTablas

        elif quien == kTablas50:
            self.resultado = kTablas

        elif quien == kTablasFaltaMaterial:
            self.resultado = kTablas

        self.desactivaTodas()
        self.refresh()

    def ent_otro(self):
        pos = DatosNueva.numEntrenamiento(self.pantalla, self.titEntreno, self.numEntrenos, pos=self.posEntreno)
        if pos is not None:
            self.posEntreno = pos
            self.reiniciar()

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

        if len(self.liVariantes) == 1:
            if k_variantes not in self.liOpcionesToolBar:
                self.liOpcionesToolBar.append(k_variantes)
                self.pantalla.ponToolBar(self.liOpcionesToolBar)

    def lanzaVariantes(self):

        icoNegro = Iconos.PuntoNegro()
        icoVerde = Iconos.PuntoVerde()

        menu = QTVarios.LCMenu(self.pantalla)

        for n, (tit, txtp, siBlancas) in enumerate(self.liVariantes):
            menu.opcion(n, tit, icoVerde if siBlancas else icoNegro)
            menu.separador()

        resp = menu.lanza()
        if resp is not None:
            self.lanzaVariantesNumero(resp)

    def lanzaVariantesNumero(self, resp):

        if resp == -1:
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.fen)
            self.partida.reset(cp)
        else:
            self.partida.recuperaDeTexto(self.liVariantes[resp][1])
        self.estado = kJugando
        self.siDirigidoVariantes = True
        self.siDirigido = True
        self.ponteAlFinal()
        self.siguienteJugada()

    def compruebaComentarios(self):
        if not self.partida.liJugadas or not self.siDirigido:
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

    def createTactics(self):
        nameTactic = os.path.basename(self.entreno)[:-4]

        nomDir = os.path.join(self.configuracion.dirPersonalTraining, "Tactics", nameTactic)
        if os.path.isdir(nomDir):
            nom = nomDir + "-%d"
            n = 1
            while os.path.isdir(nom % n):
                n += 1
            nomDir = nom % n
        nomIni = os.path.join(nomDir, "Config.ini")
        nomTactic = "TACTIC1"
        nomDirTac = os.path.join(VarGen.configuracion.dirPersonalTraining, "Tactics")
        Util.creaCarpeta(nomDirTac)
        Util.creaCarpeta(nomDir)
        nomFNS = os.path.join(nomDir, "Puzzles.fns")

        # Se leen todos los fens
        f = open(self.entreno)
        liBase = []
        for linea in f:
            liBase.append(linea.strip())
        f.close()

        # Se crea el fichero con los puzzles
        f = codecs.open(nomFNS, "w", "utf-8", 'ignore')
        nregs = len(liBase)

        tmpBP = QTUtil2.BarraProgreso(self.pantalla, nameTactic, _("Working..."), nregs)
        tmpBP.mostrar()

        for n in range(nregs):

            if tmpBP.siCancelado():
                break

            tmpBP.pon(n + 1)

            linea = liBase[n]
            li = linea.split("|")
            fen = li[0]
            if len(li) < 3 or not li[2]:
                # tutor a trabajar
                mrm = self.xrival.analiza(fen)
                if not mrm.liMultiPV:
                    continue
                rm = mrm.liMultiPV[0]
                p = Partida.Partida(fen=fen)
                p.leerPV(rm.pv)
                pts = rm.puntosABS()
                jg = p.jugada(0)
                for pos, rm1 in enumerate(mrm.liMultiPV):
                    if pos:
                        if rm1.puntosABS() == pts:
                            p1 = Partida.Partida(fen=fen)
                            p1.leerPV(rm1.pv)
                            if pos > 1:
                                jg.variantes += "\n"
                            jg.variantes += p1.pgnBaseRAW()
                        else:
                            break

                jugadas = p.pgnBaseRAW()
                txt = fen + "||%s\n" % jugadas
            else:
                txt = linea

            f.write(txt)

        f.close()
        tmpBP.cerrar()

        # Se crea el fichero de control
        dicIni = {}
        dicIni[nomTactic] = d = {}
        d["MENU"] = nameTactic
        d["FILESW"] = "%s:100" % os.path.basename(nomFNS)

        Util.dic8ini(nomIni, dicIni)

        self.mensajeEnPGN(_X(_("Tactic training %1 created."), nomDir) + "<br>" +
                    _X(_("You can access this training from menu Trainings-Learn tactics by repetition-%1"), nomDir))

        self.procesador.entrenamientos.rehaz()
