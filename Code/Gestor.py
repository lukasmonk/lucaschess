import codecs
import os
import random
import time

import LCEngine4 as LCEngine

from Code import Analisis
from Code import AnalisisIndexes
from Code import AperturasStd
from Code import ControlPGN
from Code import ControlPosicion
from Code import DGT
from Code import Jugada
from Code import Partida
from Code import DBgames
from Code import DBgamesFEN
from Code.QT import Histogram
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import PantallaAnalisis
from Code.QT import PantallaArbol
from Code.QT import PantallaArbolBook
from Code.QT import PantallaSavePGN
from Code.QT import PantallaTutor
from Code.QT import PantallaKibitzers
from Code.QT import Pelicula
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import WOpeningGuide
from Code.QT import TabTipos
from Code import Util
from Code import VarGen
from Code import Kibitzers
from Code import XRun
from Code.Constantes import *


class Gestor:
    def __init__(self, procesador):

        self.fen = None

        self.procesador = procesador
        self.pantalla = procesador.pantalla
        self.tablero = procesador.tablero
        self.tablero.setAcceptDropPGNs(None)
        # self.tablero.mostrarPiezas(True, True)
        self.configuracion = procesador.configuracion
        self.runSound = VarGen.runSound

        self.liKibitzersActivas = procesador.liKibitzersActivas

        self.estado = kFinJuego  # Para que siempre este definido

        self.tipoJuego = None
        self.ayudas = None
        self.ayudasPGN = 0

        self.siCompetitivo = False

        self.resultado = kDesconocido

        self.categoria = None

        self.pantalla.ponGestor(self)

        self.partida = Partida.Partida()

        self.listaAperturasStd = AperturasStd.ap

        self.teclaPanico = procesador.teclaPanico
        self.siTeclaPanico = False

        self.siJuegaHumano = False

        self.pgn = ControlPGN.ControlPGN(self)

        self.xtutor = procesador.XTutor()
        self.xanalyzer = procesador.XAnalyzer()
        self.xrival = None

        self.teclaPanico = 32

        self.siJuegaPorMi = False

        self.unMomento = self.procesador.unMomento
        self.um = None

        self.xRutinaAccionDef = None

        self.xpelicula = None

        self.pantalla.ajustaTam()

        self.tablero.exePulsadoNum = self.exePulsadoNum
        self.tablero.exePulsadaLetra = self.exePulsadaLetra

        self.siRevision = True  # controla si hay mostrar el rotulo de revisando

        # Capturas
        self.capturasActivable = False

        # Informacion
        self.informacionActivable = True

        self.nonDistract = None

        # x Control del tutor
        #  asi sabemos si ha habido intento de analisis previo (por ejemplo el usuario mientras piensa decide activar el tutor)
        self.siIniAnalizaTutor = False

        self.continueTt = not self.configuracion.notbackground

        # Atajos raton:
        self.atajosRatonDestino = None
        self.atajosRatonOrigen = None

        # DGT
        self.compruebaDGT()

    def ponFinJuego(self):
        if self.partida.numJugadas():
            self.estado = kFinJuego
            self.desactivaTodas()
            liOpciones = [k_mainmenu]
            if hasattr(self, "reiniciar"):
                liOpciones.append(k_reiniciar)
            liOpciones.append(k_configurar)
            liOpciones.append(k_utilidades)
            self.pantalla.ponToolBar(liOpciones)
            self.quitaAyudas()
        else:
            self.procesador.reset()

    def finGestor(self):
        # se llama desde procesador.inicio, antes de borrar el gestor
        self.tablero.atajosRaton = None
        if self.nonDistract:
            self.pantalla.base.tb.setVisible(True)

    def atajosRatonReset(self):
        self.atajosRatonDestino = None
        self.atajosRatonOrigen = None

    def otherCandidates(self, liMoves, posicion, liC):
        liPlayer = []
        for mov in liMoves:
            if mov.mate():
                liPlayer.append((mov.hasta(), "P#"))
            elif mov.jaque():
                liPlayer.append((mov.hasta(), "P+"))
            elif mov.captura():
                liPlayer.append((mov.hasta(), "Px"))
        oposic = posicion.copia()
        oposic.siBlancas = not posicion.siBlancas
        oposic.alPaso = ""
        siJaque = LCEngine.isCheck()
        LCEngine.setFen(oposic.fen())
        liO = LCEngine.getExMoves()
        liRival = []
        for n, mov in enumerate(liO):
            if not siJaque:
                if mov.mate():
                    liRival.append((mov.hasta(), "R#"))
                elif mov.jaque():
                    liRival.append((mov.hasta(), "R+"))
                elif mov.captura():
                    liPlayer.append((mov.hasta(), "Rx"))
            elif mov.captura():
                liPlayer.append((mov.hasta(), "Rx"))

        liC.extend(liRival)
        liC.extend(liPlayer)

    def colectCandidates(self, a1h8):
        if not hasattr(self.pgn, "jugada"):  # gestor60 por ejemplo
            return None
        fila, columna = self.pantalla.pgnPosActual()
        posJugada, jg = self.pgn.jugada(fila, columna.clave)
        if jg:
            posicion = jg.posicion
        else:
            posicion = self.partida.iniPosicion

        LCEngine.setFen(posicion.fen())
        li = LCEngine.getExMoves()
        if not li:
            return None

        # Se comprueba si algun movimiento puede empezar o terminar ahi
        siOrigen = siDestino = False
        for mov in li:
            desde = mov.desde()
            hasta = mov.hasta()
            if a1h8 == desde:
                siOrigen = True
                break
            if a1h8 == hasta:
                siDestino = True
                break
        origen = destino = None
        if siOrigen or siDestino:
            pieza = posicion.casillas.get(a1h8, None)
            if pieza is None:
                destino = a1h8
            elif posicion.siBlancas:
                if pieza.isupper():
                    origen = a1h8
                else:
                    destino = a1h8
            else:
                if pieza.isupper():
                    destino = a1h8
                else:
                    origen = a1h8

        liC = []
        for mov in li:
            a1 = mov.desde()
            h8 = mov.hasta()
            siO = (origen == a1) if origen else None
            siD = (destino == h8) if destino else None

            if (siO and siD) or ((siO is None) and siD) or ((siD is None) and siO):
                t = (a1, h8)
                if not (t in liC):
                    liC.append(t)

        if origen:
            liC = [(dh[1], "C") for dh in liC]
        else:
            liC = [(dh[0], "C") for dh in liC]
        self.otherCandidates(li, posicion, liC)
        return liC

    def atajosRaton(self, a1h8):
        if a1h8 is None or not self.tablero.siActivasPiezas:
            self.atajosRatonReset()
            return

        numJugadas, nj, fila, siBlancas = self.jugadaActual()
        if nj < numJugadas - 1:
            self.atajosRatonReset()
            liC = self.colectCandidates(a1h8)
            self.tablero.showCandidates(liC)
            return

        posicion = self.partida.ultPosicion
        LCEngine.setFen(posicion.fen())
        li_moves = LCEngine.getExMoves()
        if not li_moves:
            return

        # Se comprueba si algun movimiento puede empezar o terminar ahi
        li_destinos = []
        li_origenes = []
        for mov in li_moves:
            desde = mov.desde()
            hasta = mov.hasta()
            if a1h8 == desde:
                li_destinos.append(hasta)
            if a1h8 == hasta:
                li_origenes.append(desde)
        if not (li_destinos or li_origenes):
            self.atajosRatonReset()
            return

        def mueve():
            self.tablero.muevePiezaTemporal(self.atajosRatonOrigen, self.atajosRatonDestino)
            if (not self.tablero.mensajero(self.atajosRatonOrigen, self.atajosRatonDestino)) and self.atajosRatonOrigen:
                self.tablero.reponPieza(self.atajosRatonOrigen)
            self.atajosRatonReset()

        def showCandidates():
            if self.configuracion.showCandidates:
                liC = []
                for mov in li_moves:
                    a1 = mov.desde()
                    h8 = mov.hasta()
                    if a1 == self.atajosRatonOrigen:
                        liC.append((h8, "C"))
                self.otherCandidates(li_moves, posicion, liC)
                self.tablero.showCandidates(liC)

        if not self.configuracion.siAtajosRaton:
            if li_destinos:
                self.atajosRatonOrigen = a1h8
                self.atajosRatonDestino = None
                # if self.atajosRatonDestino and self.atajosRatonDestino in li_destinos:
                #     mueve()
                # else:
                #     self.atajosRatonDestino = None
                showCandidates()
                return
            elif li_origenes:
                self.atajosRatonDestino = a1h8
                if self.atajosRatonOrigen and self.atajosRatonOrigen in li_origenes:
                    mueve()
                else:
                    self.atajosRatonOrigen = None
                    self.atajosRatonDestino = None
                    showCandidates()
            return

        if li_origenes:
            self.atajosRatonDestino = a1h8
            if self.atajosRatonOrigen and self.atajosRatonOrigen in li_origenes:
                mueve()
            elif len(li_origenes) == 1:
                self.atajosRatonOrigen = li_origenes[0]
                mueve()
            else:
                showCandidates()
            return

        if li_destinos:
            self.atajosRatonOrigen = a1h8
            if self.atajosRatonDestino and self.atajosRatonDestino in li_destinos:
                mueve()
            elif len(li_destinos) == 1:
                self.atajosRatonDestino = li_destinos[0]
                mueve()
            else:
                showCandidates()
            return

        # if self.atajosRatonOrigen and self.atajosRatonDestino:
        #     elif not self.atajosRatonOrigen:
        #         self.atajosRatonReset()
        #     elif self.configuracion.showCandidates:
        #         liC = []
        #         for mov in li_moves:
        #             a1 = mov.desde()
        #             h8 = mov.hasta()
        #             if a1 == self.atajosRatonOrigen:
        #                 liC.append((h8, "C"))
        #         self.otherCandidates(li_moves, posicion, liC)
        #         self.tablero.showCandidates(liC)
        #     return

        # # else tipo = predictivo
        # # Si no es posible el movimiento -> y estan los dos -> reset + nuevo intento
        # # Miramos todos los movimientos que cumplan
        # liC = []
        # for mov in li_moves:
        #     a1 = mov.desde()
        #     h8 = mov.hasta()
        #     siO = (self.atajosRatonOrigen == a1) if self.atajosRatonOrigen else None
        #     siD = (self.atajosRatonDestino == h8) if self.atajosRatonDestino else None

        #     if (siO and siD) or ((siO is None) and siD) or ((siD is None) and siO):
        #         t = (a1, h8)
        #         if not (t in liC):
        #             liC.append(t)

        # nlc = len(liC)
        # if nlc == 0:
        #     if (self.atajosRatonDestino == a1h8) and self.atajosRatonOrigen and self.atajosRatonOrigen != a1h8:
        #         self.atajosRatonOrigen = None
        #         self.atajosRaton(a1h8)
        # elif nlc == 1:
        #     desde, hasta = liC[0]
        #     self.tablero.muevePiezaTemporal(desde, hasta)
        #     if not self.tablero.mensajero(desde, hasta):
        #             self.tablero.reponPieza(desde)
        #     self.atajosRatonReset()
        # elif self.configuracion.showCandidates:

        #     # -CONTROL-
        #     if hasattr(self.pgn, "jugada"):  # gestor60 no tiene por ejemplo
        #         if self.atajosRatonOrigen:
        #             liC = [(hasta, "C") for desde, hasta in liC]
        #         else:
        #             liC = [(desde, "C") for desde, hasta in liC]
        #         self.otherCandidates(li_moves, posicion, liC)
        #         self.tablero.showCandidates(liC)

    def repiteUltimaJugada(self):
        # Gestor ent tac + ent pos si hay partida
        if self.partida.numJugadas():
            jg = self.partida.last_jg()
            self.tablero.ponPosicion(jg.posicionBase)
            self.tablero.ponFlechaSC(jg.desde, jg.hasta)
            QTUtil.refreshGUI()
            time.sleep(0.6)
            ant = self.configuracion.efectosVisuales
            self.configuracion.efectosVisuales = True
            self.movimientosPiezas(jg.liMovs, True)
            self.configuracion.efectosVisuales = ant
            self.tablero.ponPosicion(jg.posicion)

    def movimientosPiezas(self, liMovs, siMovTemporizado=False):
        if siMovTemporizado and self.configuracion.efectosVisuales:

            rapidez = self.configuracion.rapidezMovPiezas * 1.0 / 100.0
            cpu = self.procesador.cpu
            cpu.reset()
            segundos = None

            # primero los movimientos
            for movim in liMovs:
                if movim[0] == "m":
                    if segundos is None:
                        desde, hasta = movim[1], movim[2]
                        dc = ord(desde[0]) - ord(hasta[0])
                        df = int(desde[1]) - int(hasta[1])
                        # Maxima distancia = 9.9 ( 9,89... sqrt(7**2+7**2)) = 4 segundos
                        dist = (dc ** 2 + df ** 2) ** 0.5
                        segundos = 4.0 * dist / (9.9 * rapidez)
                    if self.procesador.gestor:
                        cpu.muevePieza(movim[1], movim[2], siExclusiva=False, segundos=segundos)
                    else:
                        return

            if segundos is None:
                segundos = 1.0

            # segundo los borrados
            for movim in liMovs:
                if movim[0] == "b":
                    if self.procesador.gestor:
                        n = cpu.duerme(segundos * 0.80 / rapidez)
                        cpu.borraPieza(movim[1], padre=n)
                    else:
                        return

            # tercero los cambios
            for movim in liMovs:
                if movim[0] == "c":
                    if self.procesador.gestor:
                        cpu.cambiaPieza(movim[1], movim[2], siExclusiva=True)
                    else:
                        return

            if self.procesador.gestor:
                cpu.runLineal()

        else:
            for movim in liMovs:
                if movim[0] == "b":
                    self.tablero.borraPieza(movim[1])
                elif movim[0] == "m":
                    self.tablero.muevePieza(movim[1], movim[2])
                elif movim[0] == "c":
                    self.tablero.cambiaPieza(movim[1], movim[2])

        # Aprovechamos que esta operacion se hace en cada jugada
        self.atajosRatonReset()

    def numDatos(self):
        return self.pgn.numDatos()

    def ponVista(self):
        if not hasattr(self.pgn, "jugada"):  # gestor60 por ejemplo
            return
        fila, columna = self.pantalla.pgnPosActual()
        posJugada, jg = self.pgn.jugada(fila, columna.clave)

        # if jg:
        #     posicion = jg.posicionBase if columna.clave == "NUMERO" else jg.posicion
        # else:
        #     posicion = self.partida.iniPosicion
        # self.tablero.setUltPosicion(posicion)

        if self.pantalla.siCapturas or self.pantalla.siInformacionPGN or self.liKibitzersActivas:
            if jg:
                dic, siBlancas = jg.posicion.capturas()
                if hasattr(jg, "analisis") and jg.analisis:
                    mrm, pos = jg.analisis
                    if pos:  # no se muestra la mejor jugada si es la realizada
                        rm0 = mrm.mejorMov()
                        self.tablero.ponFlechaSCvar([(rm0.desde, rm0.hasta),])

            else:
                dic, siBlancas = self.partida.ultPosicion.capturas()

            nomApertura = ""
            apertura = self.partida.apertura
            if apertura:
                nomApertura = apertura.trNombre
            if self.pantalla.siCapturas:
                self.pantalla.ponCapturas(dic)
            if self.pantalla.siInformacionPGN:
                if (fila == 0 and columna.clave == "NUMERO") or fila < 0:
                    self.pantalla.ponInformacionPGN(self.partida, None, nomApertura)
                else:
                    self.pantalla.ponInformacionPGN(None, jg, nomApertura)

            if self.liKibitzersActivas:
                if self.siMiraKibitzers():
                    self.miraKibitzers(jg, columna.clave)
                else:
                    self.paraKibitzers()

    def siMiraKibitzers(self):
        return ((self.estado == kFinJuego) or
               self.tipoJuego in (kJugEntPos, kJugPGN, kJugEntMaq, kJugEntTac, kJugGM, kJugSolo, kJugBooks, kJugAperturas) or
               (self.tipoJuego in (kJugElo, kJugMicElo) and not self.siCompetitivo))

    def miraKibitzers(self, jg, columnaClave, soloNuevo=False):
        if jg:
            fenBase = jg.posicionBase.fen()
            fen = fenBase if columnaClave == "NUMERO" else jg.posicion.fen()
        else:
            fen = self.partida.ultPosicion.fen()
            fenBase = fen
        liApagadas = []
        last = len(self.liKibitzersActivas) - 1
        for n, xkibitzer in enumerate(self.liKibitzersActivas):
            if xkibitzer.siActiva():
                if soloNuevo and n != last:
                    continue
                xkibitzer.ponFen(fen, fenBase)
            else:
                liApagadas.append(n)
        if liApagadas:
            for x in range(len(liApagadas) - 1, -1, -1):
                kibitzer = self.liKibitzersActivas[x]
                kibitzer.close()
                del self.liKibitzersActivas[x]

    def paraKibitzers(self):
        for n, xkibitzer in enumerate(self.liKibitzersActivas):
            xkibitzer.terminar() #ponFen(None)
        self.procesador.quitaKibitzers()
        self.liKibitzersActivas = []

    def ponPiezasAbajo(self, siBlancas):
        self.tablero.ponerPiezasAbajo(siBlancas)

    def quitaAyudas(self, siTambienTutorAtras=True, siQuitarAtras=True):
        self.pantalla.quitaAyudas(siTambienTutorAtras, siQuitarAtras)
        self.siTutorActivado = False
        self.ponActivarTutor(False)

    def ponAyudas(self, ayudas, siQuitarAtras=True):
        self.pantalla.ponAyudas(ayudas, siQuitarAtras)

    def pensando(self, siPensando):
        self.pantalla.pensando(siPensando)

    def ponActivarTutor(self, siActivar):
        self.pantalla.ponActivarTutor(siActivar)
        self.siTutorActivado = siActivar

    def cambiaActivarTutor(self):
        self.siTutorActivado = not self.siTutorActivado
        self.ponActivarTutor(self.siTutorActivado)

    def desactivaTodas(self):
        self.tablero.desactivaTodas()

    def activaColor(self, siBlancas):
        self.tablero.activaColor(siBlancas)

    def mostrarIndicador(self, siPoner):
        self.tablero.indicadorSC.setVisible(siPoner)

    def ponIndicador(self, siBlancas):
        self.tablero.ponIndicador(siBlancas)

    def ponMensajero(self, funcMensajero):
        self.tablero.ponMensajero(funcMensajero, self.atajosRaton)

    def ponFlechaSC(self, desde, hasta, lipvvar=None):
        self.tablero.ponFlechaSC(desde, hasta)
        self.tablero.quitaFlechas()
        if lipvvar:
            self.tablero.ponFlechaSCvar(lipvvar)

    def reponPieza(self, posic):
        self.tablero.reponPieza(posic)

    def ponRotulo1(self, mensaje):
        return self.pantalla.ponRotulo1(mensaje)

    def ponRotulo2(self, mensaje):
        return self.pantalla.ponRotulo2(mensaje)

    def ponRotulo3(self, mensaje):
        return self.pantalla.ponRotulo3(mensaje)

    def quitaRotulo3(self):
        return self.pantalla.ponRotulo3(None)

    def alturaRotulo3(self, px):
        return self.pantalla.alturaRotulo3(px)

    def ponRevision(self, siPoner):
        if not self.siRevision:
            siPoner = False
        self.pantalla.ponRevision(siPoner)

    def beepExtendido(self, siNuestro=False):
        if siNuestro:
            if not self.configuracion.siSuenaNuestro:
                return
        if self.configuracion.siSuenaJugada:
            if self.partida.numJugadas():
                jg = self.partida.jugada(-1)
                self.runSound.playLista(jg.listaSonidos(), siEsperar=True)
        elif self.configuracion.siSuenaBeep:
            self.runSound.playBeep()

    def beepZeitnot(self):
        self.runSound.playZeitnot()

    def beepResultado(self, resfinal):
        if not self.configuracion.siSuenaResultados:
            return
        dic = {
            kGanamos: "GANAMOS",
            kGanaRival: "GANARIVAL",
            kTablas: "TABLAS",
            kTablasRepeticion: "TABLASREPETICION",
            kTablas50: "TABLAS50",
            kTablasFaltaMaterial: "TABLASFALTAMATERIAL",
            kGanamosTiempo: "GANAMOSTIEMPO",
            kGanaRivalTiempo: "GANARIVALTIEMPO"
        }
        if resfinal in dic:
            self.runSound.playClave(dic[resfinal])

    def pgnRefresh(self, siBlancas):
        self.pantalla.pgnRefresh(siBlancas)

    def refresh(self):
        self.tablero.escena.update()
        self.pantalla.update()
        QTUtil.refreshGUI()

    def mueveJugada(self, tipo):
        partida = self.partida
        if not partida.numJugadas():
            return
        fila, columna = self.pantalla.pgnPosActual()

        clave = columna.clave
        if clave == "NUMERO":
            siBlancas = tipo == "-"
            fila -= 1
        else:
            siBlancas = clave != "NEGRAS"

        siEmpiezaConNegras = partida.siEmpiezaConNegras

        lj = partida.numJugadas()
        if siEmpiezaConNegras:
            lj += 1
        ultFila = (lj - 1) / 2
        siUltBlancas = lj % 2 == 1

        if tipo == kMoverAtras:
            if siBlancas:
                fila -= 1
            siBlancas = not siBlancas
            pos = fila * 2
            if not siBlancas:
                pos += 1
            if fila < 0 or (fila == 0 and pos == 0 and siEmpiezaConNegras):
                self.ponteAlPrincipio()
                return
        elif tipo == kMoverAdelante:
            if not siBlancas:
                fila += 1
            siBlancas = not siBlancas
        elif tipo == kMoverInicio:
            self.ponteAlPrincipio()
            return
        elif tipo == kMoverFinal:
            fila = ultFila
            siBlancas = not partida.ultPosicion.siBlancas

        if fila == ultFila:
            if siUltBlancas and not siBlancas:
                return

        if fila < 0 or fila > ultFila:
            self.refresh()
            return
        if fila == 0 and siBlancas and siEmpiezaConNegras:
            siBlancas = False

        self.pantalla.pgnColocate(fila, siBlancas)
        self.pgnMueve(fila, siBlancas)

    def ponteEnJugada(self, numJugada):
        fila = (numJugada + 1) / 2 if self.partida.siEmpiezaConNegras else numJugada / 2
        jg = self.partida.jugada(numJugada)
        siBlancas = jg.posicionBase.siBlancas
        self.pantalla.pgnColocate(fila, siBlancas)
        self.pgnMueve(fila, siBlancas)

    def ponteAlPrincipio(self):
        self.ponPosicion(self.partida.iniPosicion)
        self.pantalla.base.pgn.goto(0, 0)
        self.pantalla.base.pgnRefresh()  # No se puede usar pgnRefresh, ya que se usa con gobottom en otros lados y aqui eso no funciona
        self.ponVista()

    def ponteAlPrincipioColor(self):
        if self.partida.liJugadas:
            jg = self.partida.jugada(0)
            self.ponPosicion(jg.posicion)
            self.pantalla.base.pgn.goto(0, 2 if jg.posicion.siBlancas else 1)
            self.tablero.ponFlechaSC(jg.desde, jg.hasta)
            self.pantalla.base.pgnRefresh()  # No se puede usar pgnRefresh, ya que se usa con gobottom en otros lados y aqui eso no funciona
            self.ponVista()
        else:
            self.ponteAlPrincipio()

    def pgnMueve(self, fila, siBlancas):
        self.pgn.mueve(fila, siBlancas)
        self.ponVista()

    def pgnMueveBase(self, fila, columna):
        if columna == "NUMERO":
            if fila == 0:
                self.ponteAlPrincipio()
                return
            else:
                fila -= 1
        self.pgn.mueve(fila, columna == "BLANCAS")
        self.ponVista()

    def ponteAlFinal(self):
        if self.partida.numJugadas():
            self.mueveJugada(kMoverFinal)
        else:
            self.ponPosicion(self.partida.iniPosicion)
            self.pantalla.base.pgnRefresh()  # No se puede usar pgnRefresh, ya que se usa con gobottom en otros lados y aqui eso no funciona
        self.ponVista()

    def jugadaActual(self):
        partida = self.partida
        fila, columna = self.pantalla.pgnPosActual()
        siBlancas = columna.clave != "NEGRAS"
        siEmpiezaConNegras = partida.siEmpiezaConNegras

        numJugadas = partida.numJugadas()
        if numJugadas == 0:
            return 0, -1, -1, partida.iniPosicion.siBlancas
        nj = fila * 2
        if not siBlancas:
            nj += 1
        if siEmpiezaConNegras:
            nj -= 1
        return numJugadas, nj, fila, siBlancas

    def pgnInformacion(self):
        if self.informacionActivable:
            self.pantalla.activaInformacionPGN()
            self.ponVista()

    def quitaInformacion(self, siActivable=False):
        self.pantalla.activaInformacionPGN(False)
        self.informacionActivable = siActivable

    def guardarPGN(self):
        conf = self.configuracion

        if conf.salvarFichero:

            try:
                f = codecs.open(conf.salvarFichero, "a", 'utf-8', 'ignore')
                dato = self.pgn.actual() + "\n\n"
                f.write(dato.replace("\n", "\r\n"))
                f.close()
            except:
                QTUtil.ponPortapapeles(self.pgn.actual())
                QTUtil2.mensError(self.pantalla, "%s : %s\n\n%s" % (_("Unable to save"), conf.salvarFichero, _(
                        "It is saved in the clipboard to paste it wherever you want.")))

    def guardarGanados(self, siGanado):
        conf = self.configuracion

        if siGanado:
            siSalvar = conf.salvarGanados
        else:
            siSalvar = conf.salvarPerdidos

        if siSalvar:
            self.guardarPGN()

    def guardarNoTerminados(self):
        if self.partida.numJugadas() < 2:
            return

        conf = self.configuracion

        if conf.salvarAbandonados:
            if not QTUtil2.pregunta(self.pantalla, _("Do you want to save this game?")):
                return
            self.guardarPGN()

    def ponCapPorDefecto(self):
        self.capturasActivable = True
        if self.configuracion.siActivarCapturas:
            self.pantalla.activaCapturas(True)
            self.ponVista()

    def ponInfoPorDefecto(self):
        self.informacionActivable = True
        if self.configuracion.siActivarInformacion:
            self.pantalla.activaInformacionPGN(True)
            self.ponVista()

    def ponCapInfoPorDefecto(self):
        self.ponCapPorDefecto()
        self.ponInfoPorDefecto()

    def capturas(self):
        if self.capturasActivable:
            self.pantalla.activaCapturas()
            self.ponVista()

    def quitaCapturas(self):
        self.pantalla.activaCapturas(False)
        self.ponVista()

    def nonDistractMode(self):
        self.nonDistract = self.pantalla.base.nonDistractMode(self.nonDistract)
        self.pantalla.ajustaTam()

    def boardRightMouse(self, siShift, siControl, siAlt):
        self.tablero.lanzaDirector()

    def gridRightMouse(self, siShift, siControl, siAlt):
        if siControl:
            self.capturas()
        elif siAlt:
            self.nonDistract = self.pantalla.base.nonDistractMode(self.nonDistract)
        else:
            self.pgnInformacion()
        self.pantalla.ajustaTam()

    def listado(self, tipo):
        if tipo == "pgn":
            return self.pgn.actual()
        elif tipo == "fen":
            return self.fenActivoConInicio()
        elif tipo == "fns":
            return self.partida.fensActual()

    def jugadaActiva(self):
        fila, columna = self.pantalla.pgnPosActual()
        siBlancas = columna.clave != "NEGRAS"
        pos = fila * 2
        if not siBlancas:
            pos += 1
        if self.partida.siEmpiezaConNegras:
            pos -= 1
        tam_lj = self.partida.numJugadas()
        siUltimo = (pos + 1) >= tam_lj
        if siUltimo:
            pos = tam_lj - 1
        return pos, self.partida.jugada(pos) if tam_lj else None

    def fenActivo(self):
        pos, jg = self.jugadaActiva()
        return jg.posicion.fen() if jg else self.fenUltimo()

    def fenActivoConInicio(self):
        pos, jg = self.jugadaActiva()
        if pos == 0:
            fila, columna = self.pantalla.pgnPosActual()
            if columna.clave == "NUMERO":
                return self.partida.iniPosicion.fen()
        return jg.posicion.fen() if jg else self.fenUltimo()

    def fenUltimo(self):
        return self.partida.fenUltimo()

    def fenPrevio(self):
        fila, columna = self.pantalla.pgnPosActual()
        siBlancas = columna.clave != "NEGRAS"
        pos = fila * 2
        if not siBlancas:
            pos += 1
        if self.partida.siEmpiezaConNegras:
            pos -= 1
        tam_lj = len(self.partida)
        if 0 <= pos < tam_lj:
            return self.partida.jugada(pos).posicionBase.fen()
        else:
            return self.partida.iniPosicion.fen()

    def analizaTutor(self):
        self.pensando(True)
        fen = self.partida.ultPosicion.fen()
        if not self.siTerminada():
            self.mrmTutor = self.xtutor.analiza(fen)
        else:
            self.mrmTutor = None
        self.pensando(False)
        return self.mrmTutor

    def cambioTutor(self):
        if PantallaTutor.cambioTutor(self.pantalla, self.configuracion):
            self.procesador.cambiaXTutor()
            self.xtutor = self.procesador.XTutor()
            self.ponRotulo2(_("Tutor") + ": <b>" + self.xtutor.nombre)
            self.siAnalizadoTutor = False

            if self.tipoJuego == kJugEntMaq:
                self.analizaInicio()

    def siTerminada(self):
        return self.partida.ultPosicion.siTerminada()

    def dameJugadaEn(self, fila, clave):
        siBlancas = clave != "NEGRAS"

        pos = fila * 2
        if not siBlancas:
            pos += 1
        if self.partida.siEmpiezaConNegras:
            pos -= 1
        tam_lj = self.partida.numJugadas()
        if tam_lj == 0:
            return
        siUltimo = (pos + 1) >= tam_lj

        jg = self.partida.jugada(pos)
        return jg, siBlancas, siUltimo, tam_lj, pos

    def ayudaMover(self, maxRecursion):
        if not self.siTerminada():
            jg = Jugada.Jugada()
            jg.posicionBase = self.partida.ultPosicion.copia()
            jg.desde = jg.hasta = jg.coronacion = ""
            Analisis.muestraAnalisis(self.procesador, self.xtutor, jg, self.tablero.siBlancasAbajo, maxRecursion, 0,
                                     siGrabar=False)

    def analizaPosicion(self, fila, clave):
        if fila < 0:
            return

        # siShift, siControl, siAlt = QTUtil.kbdPulsado() # Antes de que analice
        jg, siBlancas, siUltimo, tam_lj, pos_jg = self.dameJugadaEn(fila, clave)
        if not jg:
            return

        if self.estado == kFinJuego:
            maxRecursion = 9999
        else:
            if not (self.tipoJuego in [kJugEntPos, kJugPGN, kJugEntMaq, kJugGM, kJugSolo, kJugBooks, kJugAperturas, kJugEntTac] or
                        (self.tipoJuego in [kJugElo, kJugMicElo] and not self.siCompetitivo)):
                if siUltimo or self.ayudas == 0:
                    return
                maxRecursion = tam_lj - pos_jg - 3  # %#
            else:
                maxRecursion = 9999

        if not (hasattr(jg, "analisis") and jg.analisis):
            siCancelar = self.xtutor.motorTiempoJugada > 5000 or self.xtutor.motorProfundidad > 7
            me = QTUtil2.mensEspera.inicio(self.pantalla, _("Analyzing the move...."), posicion="ad", siCancelar = siCancelar)
            if siCancelar:
                def test_me(txt):
                    return not me.cancelado()
                self.xanalyzer.ponGuiDispatch(test_me)
            mrm, pos = self.xanalyzer.analizaJugadaPartida(self.partida, pos_jg, self.xtutor.motorTiempoJugada, self.xtutor.motorProfundidad)
            if siCancelar:
                if me.cancelado():
                    me.final()
                    return
            jg.analisis = mrm, pos
            me.final()

        Analisis.muestraAnalisis(self.procesador, self.xtutor, jg, self.tablero.siBlancasAbajo, maxRecursion, pos_jg)
        self.ponVista()

    def analizar(self):
        Analisis.analizaPartida(self)
        self.refresh()

    def borrar(self):
        separador = FormLayout.separador
        li_del = [separador]
        li_del.append(separador)
        li_del.append((_("Variants") + ":", False))
        li_del.append(separador)
        li_del.append((_("Ratings") + ":", False))
        li_del.append(separador)
        li_del.append((_("Comments") + ":", False))
        li_del.append(separador)
        li_del.append((_("Analysis") + ":", False))
        li_del.append(separador)
        li_del.append((_("All") + ":", False))
        resultado = FormLayout.fedit(li_del, title=_("Remove"), parent=self.pantalla, icon=Iconos.Delete())
        if resultado:
            variants, ratings, comments, analysis, all = resultado[1]
            if all:
                variants, ratings, comments, analysis = True, True, True, True
            for jg in self.partida.liJugadas:
                if variants:
                    jg.variantes = ""
                if ratings:
                    jg.critica = ""
                    jg.criticaDirecta = ""
                if comments:
                    jg.comentario = ""
                if analysis:
                    jg.analisis = None
            self.refresh()

    def cambiaRival(self, nuevo):
        self.procesador.cambiaRival(nuevo)

    def pelicula(self):
        resp = Pelicula.paramPelicula(self.configuracion, self.pantalla)
        if resp is None:
            return

        segundos, siInicio, siPGN = resp

        self.xpelicula = Pelicula.Pelicula(self, segundos, siInicio, siPGN)

    def ponRutinaAccionDef(self, rutina):
        self.xRutinaAccionDef = rutina

    def rutinaAccionDef(self, clave):
        if self.xRutinaAccionDef:
            self.xRutinaAccionDef(clave)
        elif clave == k_mainmenu:
            self.procesador.reset()
        else:
            self.procesador.procesarAccion(clave)

    def finalX0(self):
        # Se llama desde la pantalla al pulsar X
        # Se comprueba si estamos en la pelicula
        if self.xpelicula:
            self.xpelicula.terminar()
            return False
        return self.finalX()

    def exePulsadoNum(self, siActivar, numero):
        if numero in [1, 8]:
            if siActivar:
                # Que jugada esta en el tablero
                fen = self.fenActivoConInicio()
                siBlancas = " w " in fen
                if numero == 1:
                    siMB = siBlancas
                else:
                    siMB = not siBlancas
                self.tablero.quitaFlechas()
                if self.tablero.flechaSC:
                    self.tablero.flechaSC.hide()
                li = LCEngine.getCaptures(fen, siMB)
                for m in li:
                    d = m.desde()
                    h = m.hasta()
                    self.tablero.creaFlechaMov(d, h, "c")
            else:
                self.tablero.quitaFlechas()
                if self.tablero.flechaSC:
                    self.tablero.flechaSC.show()

        elif numero in [2, 7]:
            if siActivar:
                # Que jugada esta en el tablero
                fen = self.fenActivoConInicio()
                siBlancas = " w " in fen
                if numero == 2:
                    siMB = siBlancas
                else:
                    siMB = not siBlancas
                if siMB != siBlancas:
                    fen = LCEngine.fenOB(fen)
                cp = ControlPosicion.ControlPosicion()
                cp.leeFen(fen)
                liMovs = cp.aura()

                self.liMarcosTmp = []
                regMarco = TabTipos.Marco()
                color = self.tablero.confTablero.flechaActivoDefecto().colorinterior
                if color == -1:
                    color = self.tablero.confTablero.flechaActivoDefecto().color

                st = set()
                for h8 in liMovs:
                    if h8 not in st:
                        regMarco.a1h8 = h8 + h8
                        regMarco.siMovible = True
                        regMarco.color = color
                        regMarco.colorinterior = color
                        regMarco.opacidad = 0.5
                        marco = self.tablero.creaMarco(regMarco)
                        self.liMarcosTmp.append(marco)
                        st.add(h8)

            else:
                for marco in self.liMarcosTmp:
                    self.tablero.xremoveItem(marco)
                self.liMarcosTmp = []

    def exePulsadaLetra(self, siActivar, letra):
        if siActivar:
            dic = { 'a':kMoverInicio,
                    'b':kMoverAtras, 'c':kMoverAtras, 'd':kMoverAtras,
                    'e':kMoverAdelante, 'f':kMoverAdelante, 'g':kMoverAdelante,
                    'h':kMoverFinal }
            self.mueveJugada(dic[letra])

    def kibitzers(self, orden):
        if orden == "edit":
            w = PantallaKibitzers.WKibitzers(self.pantalla, self.procesador)
            w.exec_()

        else:
            nkibitzer = int(orden)
            xkibitzer = Kibitzers.IPCKibitzer(self, nkibitzer)
            self.liKibitzersActivas.append(xkibitzer)
            fila, columna = self.pantalla.pgnPosActual()
            posJugada, jg = self.pgn.jugada(fila, columna.clave)
            self.miraKibitzers(jg, columna.clave, True)

    def paraHumano(self):
        self.siJuegaHumano = False
        self.desactivaTodas()

    def sigueHumano(self):
        self.siTeclaPanico = False
        self.siJuegaHumano = True
        self.activaColor(self.partida.ultPosicion.siBlancas)

    def checkMueveHumano(self, desde, hasta, coronacion):
        if self.siJuegaHumano:
            self.paraHumano()
        else:
            self.sigueHumano()
            return None

        movimiento = desde + hasta

        # Peon coronando
        if not coronacion and self.partida.ultPosicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.partida.ultPosicion.siBlancas)
            if coronacion is None:
                self.sigueHumano()
                return None
        if coronacion:
            movimiento += coronacion

        if self.siTeclaPanico:
            self.sigueHumano()
            return None

        siBien, self.error, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        if siBien:
            return jg
        else:
            self.sigueHumano()
            return None

    def librosConsulta(self, siEnVivo):
        w = PantallaArbolBook.PantallaArbolBook(self, siEnVivo)
        if w.exec_():
            return w.resultado
        else:
            return None

    def compruebaDGT(self):
        if self.configuracion.siDGT:
            if not DGT.activarSegunON_OFF(self.dgt):  # Error
                QTUtil2.mensError(self.pantalla, _("Error, could not detect the DGT board driver."))

    def dgt(self, quien, a1h8):
        if self.tablero.mensajero and self.tablero.siActivasPiezas:
            if quien == "whiteMove":
                if not self.tablero.siActivasPiezasColor:
                    return 0
            elif quien == "blackMove":
                if self.tablero.siActivasPiezasColor:
                    return 0
            elif quien == "scan":
                return 1
            else:
                return 1

            if self.tablero.mensajero(a1h8[:2], a1h8[2:4], a1h8[4:]):
                return 1
            return 0

        return 1

    def ponPosicion(self, posicion):
        self.tablero.ponPosicion(posicion)

    def ponPosicionDGT(self):
        DGT.ponPosicion(self.partida)

    def juegaPorMi(self):
        if self.siJuegaPorMi and self.estado == kJugando and (
                    self.ayudas or self.tipoJuego in (kJugEntMaq, kJugSolo, kJugEntPos, kJugEntTac)):
            if not self.siTerminada():
                mrm = self.analizaTutor()
                rm = mrm.mejorMov()
                if rm.desde:
                    self.siAnalizadoTutor = True
                    self.mueveHumano(rm.desde, rm.hasta, rm.coronacion)
                    if self.ayudas:
                        self.ayudas -= 1
                        if self.ayudas:
                            self.ponAyudas(self.ayudas)
                        else:
                            self.quitaAyudas()

    def control1(self):
        self.juegaPorMi()

    def configurar(self, liMasOpciones=None, siCambioTutor=False, siSonidos=False, siBlinfold=True):
        menu = QTVarios.LCMenu(self.pantalla)

        # Vista
        menuVista = menu.submenu(_("View"), Iconos.Vista())
        menuVista.opcion("vista_pgn", _("PGN information"), Iconos.InformacionPGNUno())
        menuVista.separador()
        menuVista.opcion("vista_capturas", _("Captured material"), Iconos.Capturas())
        menu.separador()

        # DGT
        if self.configuracion.siDGT:
            menu.opcion("dgt", (_("Disable %1") if DGT.siON() else _("Enable %1")).replace("%1", _("DGT board")), Iconos.DGT())
            menu.separador()

        # Ciega - Mostrar todas - Ocultar blancas - Ocultar negras
        if siBlinfold:
            menuCG = menu.submenu(_("Blindfold chess"), Iconos.Ojo())

            si = self.tablero.blindfold
            if si:
                ico = Iconos.Naranja()
                tit = _("Deactivate")
            else:
                ico = Iconos.Verde()
                tit = _("Activate")
            menuCG.opcion("cg_change", tit, ico)
            menuCG.separador()
            menuCG.opcion("cg_conf", _("Configuration"), Iconos.Opciones())
            menuCG.separador()
            menuCG.opcion("cg_pgn", "%s: %s" % (_("PGN"), _("Hide") if self.pgn.siMostrar else _("Show")), Iconos.PGN())

        # Sonidos
        if siSonidos:
            menu.separador()
            menu.opcion("sonido", _("Sounds"), Iconos.S_Play())

        # Cambio de tutor
        if siCambioTutor:
            menu.separador()
            menu.opcion("tutor", _("Tutor change"), Iconos.Tutor())

        menu.separador()
        menu.opcion("motores", _("External engines"), Iconos.Motores())

        # On top
        menu.separador()
        rotulo = _("Disable") if self.pantalla.onTop else _("Enable")
        menu.opcion("ontop", "%s: %s" % (rotulo, _("window on top")),
                    Iconos.Bottom() if self.pantalla.onTop else Iconos.Top())

        # Right mouse
        menu.separador()
        rotulo = _("Disable") if self.configuracion.directGraphics else _("Enable")
        menu.opcion("mouseGraphics", "%s: %s" % (rotulo, _("Live graphics with the right mouse button") ),
                    Iconos.RightMouse())

        # Logs of engines
        listaGMotores = VarGen.listaGestoresMotor.listaActivos() if VarGen.listaGestoresMotor else []
        menu.separador()
        smenu = menu.submenu(_("Save engines log"), Iconos.Grabar())
        if len(listaGMotores) > 0:
            for pos, gmotor in enumerate(listaGMotores):
                ico = Iconos.Cancelar() if gmotor.ficheroLog else Iconos.PuntoVerde()
                smenu.opcion("log_%d"%pos, gmotor.nombre, ico)

        smenu.separador()
        if self.configuracion.siLogEngines:
            smenu.opcion("log_noall", _("Always deactivated for all engines"), Iconos.Cancelar())
        else:
            smenu.opcion("log_yesall", _("Always activated for all engines"), Iconos.Aceptar())

        menu.separador()

        # Mas Opciones
        if liMasOpciones:
            menu.separador()
            for clave, rotulo, icono in liMasOpciones:
                if rotulo is None:
                    menu.separador()
                else:
                    menu.opcion(clave, rotulo, icono)

        resp = menu.lanza()
        if resp:

            if liMasOpciones:
                for clave, rotulo, icono in liMasOpciones:
                    if resp == clave:
                        return resp

            if resp.startswith("log_"):
                resp = resp[4:]
                self.log_engines(resp)

            if resp.startswith("vista_"):
                resp = resp[6:]
                if resp == "pgn":
                    self.pantalla.activaInformacionPGN()
                    self.ponVista()
                elif resp == "capturas":
                    self.pantalla.activaCapturas()
                    self.ponVista()

            elif resp == "dgt":
                DGT.cambiarON_OFF()
                self.compruebaDGT()

            elif resp == "sonido":
                self.config_sonido()

            elif resp == "tutor":
                self.cambioTutor()

            elif resp == "motores":
                self.procesador.motoresExternos()

            elif resp == "ontop":
                self.pantalla.onTopWindow()

            elif resp == "mouseGraphics":
                self.configuracion.directGraphics = not self.configuracion.directGraphics
                self.configuracion.graba()

            elif resp.startswith("cg_"):
                orden = resp[3:]
                if orden == "pgn":
                    self.pgn.siMostrar = not self.pgn.siMostrar
                    self.pantalla.base.pgnRefresh()
                elif orden == "change":
                    x = str(self)
                    modoPosicionBlind = False
                    for tipo in ("GestorEntPos",):
                        if tipo in x:
                            modoPosicionBlind = True
                    self.tablero.blindfoldChange(modoPosicionBlind)

                elif orden == "conf":
                    self.tablero.blindfoldConfig()

        return None

    def log_engines(self, resp):
        if resp.isdigit():
            resp = int(resp)
            motor = VarGen.listaGestoresMotor.listaActivos()[resp]
            if motor.ficheroLog:
                motor.log_close()
            else:
                motor.log_open()
        else:
            listaGMotores = VarGen.listaGestoresMotor.listaActivos()
            if resp == "yesall":
                self.configuracion.siLogEngines = True
            else:
                self.configuracion.siLogEngines = False
            for gmotor in listaGMotores:
                if resp == "yesall":
                    gmotor.log_open()
                else:
                    gmotor.log_close()
            self.configuracion.graba()

    def config_sonido(self):
        separador = FormLayout.separador
        liSon = [separador]
        liSon.append(separador)
        liSon.append((_("Beep after opponent's move") + ":", self.configuracion.siSuenaBeep))
        liSon.append(separador)
        liSon.append((None, _("Sound on in") + ":"))
        liSon.append((_("Results") + ":", self.configuracion.siSuenaResultados))
        liSon.append((_("Rival moves") + ":", self.configuracion.siSuenaJugada))
        liSon.append(separador)
        liSon.append((_("Activate sounds with our moves") + ":", self.configuracion.siSuenaNuestro))
        liSon.append(separador)
        resultado = FormLayout.fedit(liSon, title=_("Sounds"), parent=self.pantalla, anchoMinimo=250,
                                     icon=Iconos.S_Play())
        if resultado:
            self.configuracion.siSuenaBeep, self.configuracion.siSuenaResultados, \
            self.configuracion.siSuenaJugada, self.configuracion.siSuenaNuestro = resultado[1]
            self.configuracion.graba()

    def utilidades(self, liMasOpciones=None, siArbol=True):

        menu = QTVarios.LCMenu(self.pantalla)

        siJugadas = self.partida.numJugadas() > 0

        # Grabar
        icoGrabar = Iconos.Grabar()
        icoFichero = Iconos.GrabarFichero()
        icoCamara = Iconos.Camara()
        icoClip = Iconos.Clip()

        trFichero = _("Save to a file")
        trPortapapeles = _("Copy to clipboard")

        menuSave = menu.submenu(_("Save"), icoGrabar)

        menuSave.opcion("pgnfichero", _("PGN Format"), Iconos.PGN())

        menuSave.separador()

        menuFEN = menuSave.submenu(_("FEN Format"), Iconos.Naranja())
        menuFEN.opcion("fenfichero", trFichero, icoFichero)
        menuFEN.opcion("fenportapapeles", trPortapapeles, icoClip)

        menuSave.separador()

        menuFNS = menuSave.submenu(_("List of FENs"), Iconos.InformacionPGNUno())
        menuFNS.opcion("fnsfichero", trFichero, icoFichero)
        menuFNS.opcion("fnsportapapeles", trPortapapeles, icoClip)

        menuSave.separador()

        menuSave.opcion("pksfichero", "%s -> %s" % (_("PKS Format"), _("Create your own game")),
                      Iconos.JuegaSolo())

        menuSave.separador()

        menuDB = menuSave.submenu(_("Database"), Iconos.DatabaseCNew())
        siFen = not self.partida.siFenInicial()
        QTVarios.menuDB(menuDB, self.configuracion, siFen, True)
        menuSave.separador()

        menuV = menuSave.submenu(_("Board -> Image"), icoCamara)
        menuV.opcion("volfichero", trFichero, icoFichero)
        menuV.opcion("volportapapeles", trPortapapeles, icoClip)

        menu.separador()

        # Analizar
        if siJugadas:
            if not (self.tipoJuego in (kJugElo, kJugMicElo) and self.siCompetitivo and self.estado == kJugando):
                nAnalisis = 0
                for jg in self.partida.liJugadas:
                    if jg.analisis:
                        nAnalisis += 1
                if nAnalisis > 4:
                    submenu = menu.submenu(_("Analysis"), Iconos.Analizar())
                else:
                    submenu = menu
                submenu.opcion("analizar", _("Analyze"), Iconos.Analizar())
                if nAnalisis > 4:
                    submenu.separador()
                    submenu.opcion("analizar_grafico", _("Show graphics"), Iconos.Estadisticas())
                menu.separador()

                menu.opcion("borrar", _("Remove"), Iconos.Delete())
                menu.separador()

        # Pelicula
        if siJugadas:
            menu.opcion("pelicula", _("Replay game"), Iconos.Pelicula())
            menu.separador()

        # Kibitzers
        if self.siMiraKibitzers():
            menu.separador()
            menuKibitzers = menu.submenu(_("Kibitzers"), Iconos.Kibitzer())

            kibitzers = Kibitzers.Kibitzers()
            for num, (nombre, ico) in enumerate(kibitzers.lista_menu()):
                menuKibitzers.opcion("kibitzer_%d" % num, nombre, ico)
            menuKibitzers.separador()
            menuKibitzers.opcion("kibitzer_edit", _("Edition"), Iconos.ModificarP())

        # Juega por mi
        if self.siJuegaPorMi and self.estado == kJugando and \
                (self.ayudas or self.tipoJuego in (kJugEntMaq, kJugSolo, kJugEntPos, kJugEntTac)):
            menu.separador()
            menu.opcion("juegapormi", _("Plays instead of me") + "  [^1]", Iconos.JuegaPorMi()),

        # Arbol de movimientos
        if siArbol:
            menu.separador()
            menu.opcion("arbol", _("Moves tree"), Iconos.Arbol())

        # Mas Opciones
        if liMasOpciones:
            menu.separador()
            submenu = menu
            for clave, rotulo, icono in liMasOpciones:
                if rotulo is None:
                    if icono is None:
                        # liMasOpciones.append((None, None, None))
                        submenu.separador()
                    else:
                        # liMasOpciones.append((None, None, True))  # Para salir del submenu
                        submenu = menu
                elif clave is None:
                    # liMasOpciones.append((None, titulo, icono))
                    submenu = menu.submenu(rotulo, icono)

                else:
                    # liMasOpciones.append((clave, titulo, icono))
                    submenu.opcion(clave, rotulo, icono)

        resp = menu.lanza()

        if not resp:
            return

        if liMasOpciones:
            for clave, rotulo, icono in liMasOpciones:
                if resp == clave:
                    return resp

        if resp == "juegapormi":
            self.juegaPorMi()

        elif resp == "analizar":
            self.analizar()

        elif resp == "analizar_grafico":
            self.showAnalisis()

        elif resp == "borrar":
            self.borrar()

        elif resp == "pelicula":
            self.pelicula()

        elif resp.startswith("kibitzer_"):
            self.kibitzers(resp[9:])

        elif resp == "arbol":
            self.arbol()

        elif resp.startswith("vol"):
            accion = resp[3:]
            if accion == "fichero":
                resp = QTUtil2.salvaFichero(self.pantalla, _("File to save"), self.configuracion.dirSalvados,
                                            "%s PNG (*.png)" % _("File"), False)
                if resp:
                    self.tablero.salvaEnImagen(resp, "png")

            else:
                self.tablero.salvaEnImagen()

        elif resp == "pksfichero":
            self.salvaPKS()

        elif resp == "pgnfichero":
            self.salvaPGN()

        elif resp.startswith("dbf_"):
            self.salvaDB(resp[4:])

        elif resp.startswith("fen") or resp.startswith("fns"):
            extension = resp[:3]
            si_fichero = resp.endswith("fichero")
            self.salvaFEN_FNS(extension, si_fichero)

        return None

    def mensajeEnPGN(self, mens, titulo=None):
        p0 = self.pantalla.base.pgn.pos()
        p = self.pantalla.mapToGlobal(p0)
        QTUtil2.mensajeEnPunto(self.pantalla, mens, titulo, p)

    def showAnalisis(self):
        um = self.procesador.unMomento()
        elos = self.partida.calc_elos(self.configuracion)
        elosFORM = self.partida.calc_elosFORM(self.configuracion)
        alm = Histogram.genHistograms(self.partida, self.configuracion.centipawns)
        alm.indexesHTML, alm.indexesRAW, alm.eloW, alm.eloB, alm.eloT = AnalisisIndexes.genIndexes(self.partida, elos, elosFORM, alm)
        alm.siBlancasAbajo = self.tablero.siBlancasAbajo
        um.final()
        PantallaAnalisis.showGraph(self.pantalla, self, alm, Analisis.muestraAnalisis)

    def salvaDB(self, database):
        pgn = self.listado("pgn")
        liTags = []
        for linea in pgn.split("\n"):
            if linea.startswith("["):
                ti = linea.split('"')
                if len(ti) == 3:
                    clave = ti[0][1:].strip()
                    valor = ti[1].strip()
                    liTags.append([clave, valor])
            else:
                break

        pc = Partida.PartidaCompleta(liTags=liTags)
        pc.leeOtra(self.partida)

        siFen = not self.partida.siFenInicial()
        db = DBgamesFEN.DBgamesFEN(database) if siFen else DBgames.DBgames(database)
        resp = db.inserta(pc)
        db.close()
        if resp:
            QTUtil2.mensaje(self.pantalla, _("Saved"))
        else:
            QTUtil2.mensError(self.pantalla, _("This game already exists."))

    def salvaPKS(self):
        pgn = self.listado("pgn")
        dic = self.procesador.saveAsPKS(self.estado, self.partida, pgn)
        extension = "pks"
        fichero = self.configuracion.dirJS
        while True:
            fichero = QTUtil2.salvaFichero(self.pantalla, _("File to save"), fichero,
                                           _("File") + " %s (*.%s)" % (extension, extension),
                                           siConfirmarSobreescritura=True)
            if fichero:
                fichero = str(fichero)
                if os.path.isfile(fichero):
                    yn = QTUtil2.preguntaCancelar(self.pantalla,
                                                  _X(_("The file %1 already exists, what do you want to do?"), fichero),
                                                  si=_("Overwrite"), no=_("Choose another"))
                    if yn is None:
                        break
                    if not yn:
                        continue
                direc = os.path.dirname(fichero)
                if direc != self.configuracion.dirJS:
                    self.configuracion.dirJS = direc
                    self.configuracion.graba()

                f = open(fichero, "wb")
                f.write(Util.dic2txt(dic))
                f.close()

                nombre = os.path.basename(fichero)
                QTUtil2.mensajeTemporal(self.pantalla, _X(_("Saved to %1"), nombre), 0.8)
                return

            break

    def salvaPGN(self):
        w = PantallaSavePGN.WSave(self.pantalla, self.pgn.actual(), self.configuracion)
        w.exec_()

    def salvaFEN_FNS(self, extension, siFichero):
        dato = self.listado(extension)
        if siFichero:

            resp = QTUtil2.salvaFichero(self.pantalla, _("File to save"), self.configuracion.dirSalvados,
                                        _("File") + " %s (*.%s)" % (extension, extension), False)
            if resp:
                try:

                    modo = "w"
                    if Util.existeFichero(resp):
                        yn = QTUtil2.preguntaCancelar(self.pantalla,
                                                      _X(_("The file %1 already exists, what do you want to do?"),
                                                         resp), si=_("Append"), no=_("Overwrite"))
                        if yn is None:
                            return
                        if yn:
                            modo = "a"
                            dato = "\n" * 2 + dato
                    f = codecs.open(resp, modo, 'utf-8', 'ignore')
                    f.write(dato.replace("\n", "\r\n"))
                    f.close()
                    QTUtil2.mensaje(self.pantalla, _X(_("Saved to %1"), resp))
                    direc = os.path.dirname(resp)
                    if direc != self.configuracion.dirSalvados:
                        self.configuracion.dirSalvados = direc
                        self.configuracion.graba()
                except:
                    QTUtil.ponPortapapeles(dato)
                    QTUtil2.mensError(self.pantalla, "%s : %s\n\n%s" % (
                        _("Unable to save"), resp, _("It is saved in the clipboard to paste it wherever you want.")))

        else:
            QTUtil.ponPortapapeles(dato)

    def arbol(self):
        numJugadas, nj, fila, siBlancas = self.jugadaActual()
        w = PantallaArbol.PantallaArbol(self.pantalla, self.partida, nj, self.procesador)
        w.exec_()

    def control0(self):
        fila, columna = self.pantalla.pgnPosActual()
        numJugadas, nj, fila, siBlancas = self.jugadaActual()
        if numJugadas:
            self.partida.siTerminada()
            if fila == 0 and columna.clave == "NUMERO":
                fen = self.partida.iniPosicion.fen()
                nj = -1
            else:
                jg = self.partida.jugada(nj)
                fen = jg.posicion.fen()

            pgnActual = self.pgn.actual()
            pgn = ""
            for linea in pgnActual.split("\n"):
                if linea.startswith("["):
                    pgn += linea.strip()
                else:
                    break

            p = self.partida.copia(nj)
            pgn += p.pgnBaseRAW()
            pgn = pgn.replace("|", "-")

            siguientes = ""
            if nj < self.partida.numJugadas() - 1:
                p = self.partida.copiaDesde(nj + 1)
                siguientes = p.pgnBaseRAW(p.iniPosicion.jugadas).replace("|", "-")

            txt = "%s||%s|%s\n" % (fen, siguientes, pgn)
            QTUtil.ponPortapapeles(txt)
            QTUtil2.mensajeTemporal(self.pantalla, _("It is saved in the clipboard to paste it wherever you want."), 2)

    def bookGuide(self):
        numJugadas, nj, fila, siBlancas = self.jugadaActual()
        if nj >= 0:
            jg = self.partida.jugada(nj)
            fenM2 = jg.posicion.fenM2()
            move = jg.movimiento()
        else:
            fenM2 = self.partida.iniPosicion.fenM2()
            move = None

        w = WOpeningGuide.WOpeningGuide(self.pantalla, self, fenM2inicial=fenM2, pvInicial=move)
        w.exec_()

    # Para elo games + entmaq
    def tablasPlayer(self):
        siAcepta = False
        njug = self.partida.numJugadas()
        if len(self.liRMrival) >= 4 and njug > 40:
            if njug > 100:
                limite = -100
            elif njug > 60:
                limite = -150
            else:
                limite = -200
            siAcepta = True
            for rm in self.liRMrival[-4:]:
                if rm.puntosABS() > limite:
                    siAcepta = False
        if siAcepta:
            self.partida.last_jg().siTablasAcuerdo = True
            self.ponResultado(kTablas)
        else:
            QTUtil2.mensaje(self.pantalla, _("Sorry, but the engine doesn't accept a draw right now."))
        self.noMolestar = 5
        return siAcepta

    def valoraRMrival(self, siRivalConBlancas):

        if self.partida.numJugadas() < 50 \
                or len(self.liRMrival) <= 5:
            return True
        if self.noMolestar:
            self.noMolestar -= 1
            return True

        b = random.random() ** 0.33

        # Resign
        siResign = True
        for n, rm in enumerate(self.liRMrival[-5:]):
            if int(rm.puntosABS() * b) > self.resignPTS:
                siResign = False
                break
        if siResign:
            resp = QTUtil2.pregunta(self.pantalla, _X(_("%1 wants to resign, do you accept it?"), self.xrival.nombre))
            if resp:
                self.partida.abandona(siRivalConBlancas)
                self.partida.abandona(siRivalConBlancas)
                self.ponResultado(kGanamos)
                return False
            else:
                self.noMolestar = 9
                return True

        # # Draw
        siDraw = True
        for rm in self.liRMrival[-5:]:
            pts = rm.puntosABS()
            if (not (-250 < int(pts * b) < -100)) or pts < -250:
                siDraw = False
                break
        if siDraw:
            resp = QTUtil2.pregunta(self.pantalla, _X(_("%1 proposes draw, do you accept it?"), self.xrival.nombre))
            if resp:
                self.partida.last_jg().siTablasAcuerdo = True
                self.ponResultado(kTablas)
                return False
            else:
                self.noMolestar = 9
                return True

        return True

    def utilidadesElo(self):
        if self.siCompetitivo:
            self.utilidades(siArbol=False)
        else:
            liMasOpciones = (
                ("libros", _("Consult a book"), Iconos.Libros()),
                (None, None, None),
                ("bookguide", _("Personal Opening Guide"), Iconos.BookGuide()),
            )

            resp = self.utilidades(liMasOpciones, siArbol=True)
            if resp == "libros":
                liMovs = self.librosConsulta(True)
                if liMovs:
                    for x in range(len(liMovs) - 1, -1, -1):
                        desde, hasta, coronacion = liMovs[x]
                        self.mueveHumano(desde, hasta, coronacion)
            elif resp == "bookguide":
                self.bookGuide()

    def pgnInformacionMenu(self, dicEtiquetasPGN):
        menu = QTVarios.LCMenu(self.pantalla)

        for clave, valor in dicEtiquetasPGN.iteritems():
            siFecha = clave.upper().endswith("DATE")
            if clave.upper() == "FEN":
                continue
            if siFecha:
                valor = valor.replace(".??", "").replace(".?", "")
            valor = valor.strip("?")
            if valor:
                menu.opcion(clave, "%s : %s" % (clave, valor), Iconos.PuntoAzul())

        menu.lanza()

    def saveSelectedPosition(self, lineaTraining):
        # Llamado desde GestorEnPos and GestorEntTac, para salvar la posicion tras pulsar una P
        f = codecs.open(self.configuracion.ficheroSelectedPositions, "ab", "utf-8")
        f.write(lineaTraining + "\n")
        f.close()
        QTUtil2.mensajeTemporal(self.pantalla, _('Position saved in "%s" file.'%self.configuracion.ficheroSelectedPositions), 2)
        self.procesador.entrenamientos.menu = None

    def jugarPosicionActual(self):
        fila, columna = self.pantalla.pgnPosActual()
        numJugadas, nj, fila, siBlancas = self.jugadaActual()
        if numJugadas:
            self.partida.siTerminada()
            if fila == 0 and columna.clave == "NUMERO":
                fen = self.partida.iniPosicion.fen()
            else:
                jg = self.partida.jugada(nj)
                fen = jg.posicion.fen()

            XRun.run_lucas("-play", fen)

    def showPV(self, pv, nArrows):
        if not pv:
            return True
        self.tablero.quitaFlechas()
        tipo = "mt"
        opacidad = 100
        pv = pv.strip()
        while "  " in pv:
            pv = pv.replace("  ", " ")
        lipv = pv.split(" ")
        npv = len(lipv)
        nbloques = min(npv, nArrows)
        salto = (80 - 15) * 2 / (nbloques - 1) if nbloques > 1 else 0
        cambio = max(30, salto)

        for n in range(nbloques):
            pv = lipv[n]
            self.tablero.creaFlechaMov(pv[:2], pv[2:4], tipo + str(opacidad))
            if n % 2 == 1:
                opacidad -= cambio
                cambio = salto
            tipo = "ms" if tipo == "mt" else "mt"
        return True

    def ponFlechasTutor(self, mrm, nArrows):
        self.tablero.quitaFlechas()
        if self.tablero.flechaSC:
            self.tablero.flechaSC.hide()

        rm_mejor = mrm.mejorMov()
        if not rm_mejor:
            return
        rm_peor = mrm.liMultiPV[-1]
        peso0 = rm_mejor.puntosABS()
        rango = peso0 - rm_peor.puntosABS()
        for n, rm in enumerate(mrm.liMultiPV, 1):
            peso = rm.puntosABS()
            factor = 1.0 - (peso0-peso)*1.0/rango
            self.tablero.creaFlechaTutor(rm.desde, rm.hasta, factor)
            if n >= nArrows:
                return
