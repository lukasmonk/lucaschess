import os
import time
import codecs
import random
import sys
import subprocess

from Code.Constantes import *
import Code.VarGen as VarGen
import Code.Util as Util
import Code.AnalisisGraph as AnalisisGraph
import Code.AnalisisIndexes as AnalisisIndexes
import Code.QT.PantallaAnalisis as PantallaAnalisis
import Code.Partida as Partida
import Code.Jugada as Jugada
import Code.ControlPGN as ControlPGN
import Code.Analisis as Analisis
import Code.MotorInterno as MotorInterno
import Code.SAK as SAK
import Code.AperturasStd as AperturasStd
import Code.XKibitzers as XKibitzers
import Code.DGT as DGT
import Code.QT.QTUtil as QTUtil
import Code.QT.QTVarios as QTVarios
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Iconos as Iconos
import Code.QT.PantallaTutor as PantallaTutor
import Code.QT.Pelicula as Pelicula
import Code.QT.PantallaColores as PantallaColores
import Code.QT.PantallaArbol as PantallaArbol
import Code.QT.PantallaArbolBook as PantallaArbolBook
import Code.QT.WBGuide as WBGuide

class Gestor():
    def __init__(self, procesador):

        self.fen = None

        self.procesador = procesador
        self.pantalla = procesador.pantalla
        self.tablero = procesador.tablero
        # self.tablero.mostrarPiezas(True, True)
        self.configuracion = procesador.configuracion
        self.runSound = VarGen.runSound

        self.liKibitzersActivas = procesador.liKibitzersActivas

        self.estado = kFinJuego  # Para que siempre este definido

        self.tipoJuego = None
        self.ayudas = None

        self.resultado = kDesconocido

        self.categoria = None

        self.ml = MotorInterno.MotorInterno()
        self.sak = SAK.sak

        self.pantalla.ponGestor(self)

        self.partida = Partida.Partida()

        self.listaAperturasStd = AperturasStd.ListaAperturasStd(self.configuracion, False, False)

        self.teclaPanico = procesador.teclaPanico
        self.siTeclaPanico = False

        self.siJuegaHumano = False

        self.pgn = ControlPGN.ControlPGN(self)

        self.xtutor = procesador.XTutor()
        self.xrival = None

        self.teclaPanico = 32

        self.siJuegaPorMi = False

        self.unMomento = self.procesador.unMomento
        self.um = None

        self.xRutinaAccionDef = None

        self.xpelicula = None

        self.pantalla.ajustaTam()

        self.tablero.exePulsadoNum = self.exePulsadoNum

        self.siRevision = True  # controla si hay mostrar el rotulo de revisando

        # Capturas
        self.capturasActivable = False

        # Informacion
        self.informacionActivable = True

        # x Control del tutor
        #  asi sabemos si ha habido intento de analisis previo (por ejemplo el usuario mientras piensa decide activar el tutor)
        self.siIniAnalizaTutor = False

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

    def atajosRatonReset(self):
        self.atajosRatonDestino = None
        self.atajosRatonOrigen = None

    def otherCandidates(self, liMoves, posicion, liC):
        liPlayer = []
        for mov in liMoves:
            if mov.mate():
                liPlayer.append(( mov.hasta(), "P#" ))
            elif mov.jaque():
                liPlayer.append(( mov.hasta(), "P+" ))
            elif mov.captura():
                liPlayer.append(( mov.hasta(), "Px" ))
        fen = posicion.fen()
        if "w" in fen:
            fen = fen.replace(" w ", " b ")
        else:
            fen = fen.replace(" b ", " w ")
        siJaque = self.sak.isCheck()
        self.sak.setFEN(fen)
        liO = self.sak.getExMoves()
        liRival = []
        for mov in liO:
            if not siJaque:
                if mov.mate():
                    liRival.append(( mov.hasta(), "R#" ))
                elif mov.jaque():
                    liRival.append(( mov.hasta(), "R+" ))
                elif mov.captura():
                    liPlayer.append(( mov.hasta(), "Rx" ))
            elif mov.captura():
                liPlayer.append(( mov.hasta(), "Rx" ))

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

        self.sak.setFEN(posicion.fen())
        li = self.sak.getExMoves()
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

            if (siO and siD) or ( (siO is None) and siD ) or ( (siD is None) and siO ):
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
        # Que hay en esa casilla:
        # vacia o pieza contraria = destino
        # pieza nuestra = origen

        numJugadas, nj, fila, siBlancas = self.jugadaActual()
        if nj < numJugadas - 1:
            self.atajosRatonReset()
            liC = self.colectCandidates(a1h8)
            self.tablero.showCandidates(liC)
            return

        posicion = self.partida.ultPosicion
        self.sak.setFEN(posicion.fen())
        li = self.sak.getExMoves()
        if not li:
            return

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
        if not (siOrigen or siDestino):
            self.atajosRatonReset()
            return

        siPredictivo = self.configuracion.siAtajosRaton

        pieza = posicion.casillas.get(a1h8, None)
        if pieza is None:
            self.atajosRatonDestino = a1h8
        elif self.siJugamosConBlancas:
            if pieza.isupper():
                self.atajosRatonOrigen = a1h8
            else:
                self.atajosRatonDestino = a1h8
        else:
            if pieza.isupper():
                self.atajosRatonDestino = a1h8
            else:
                self.atajosRatonOrigen = a1h8

        if not siPredictivo:
            if self.atajosRatonOrigen and self.atajosRatonDestino:
                # self.tablero.muevePiezaTemporal(self.atajosRatonOrigen, self.atajosRatonDestino)
                self.tablero.mensajero(self.atajosRatonOrigen, self.atajosRatonDestino)
            elif not self.atajosRatonOrigen:
                self.atajosRatonReset()
            elif self.configuracion.showCandidates:
                liC = []
                for mov in li:
                    a1 = mov.desde()
                    h8 = mov.hasta()
                    if a1 == self.atajosRatonOrigen:
                        liC.append((h8, "C"))
                self.otherCandidates(li, posicion, liC)
                self.tablero.showCandidates(liC)
            return

        # else tipo = predictivo
        # Si no es posible el movimiento -> y estan los dos -> reset + nuevo intento
        # Miramos todos los movimientos que cumplan

        liC = []
        for mov in li:
            a1 = mov.desde()
            h8 = mov.hasta()
            siO = (self.atajosRatonOrigen == a1) if self.atajosRatonOrigen else None
            siD = (self.atajosRatonDestino == h8) if self.atajosRatonDestino else None

            if (siO and siD) or ( (siO is None) and siD ) or ( (siD is None) and siO ):
                t = (a1, h8)
                if not (t in liC):
                    liC.append(t)

        nlc = len(liC)
        if nlc == 0:
            if (self.atajosRatonDestino == a1h8) and self.atajosRatonOrigen and self.atajosRatonOrigen != a1h8:
                self.atajosRatonOrigen = None
                self.atajosRaton(a1h8)
        elif nlc == 1:
            desde, hasta = liC[0]
            self.tablero.muevePiezaTemporal(desde, hasta)
            self.tablero.mensajero(desde, hasta)
        elif self.configuracion.showCandidates:

            #-CONTROL-
            if hasattr(self.pgn, "jugada"):  # gestor60 no tiene por ejemplo
                if self.atajosRatonOrigen:
                    liC = [(hasta, "C") for desde, hasta in liC]
                else:
                    liC = [(desde, "C") for desde, hasta in liC]
                self.otherCandidates(li, posicion, liC)
                self.tablero.showCandidates(liC)

    def repiteUltimaJugada(self):
        # Gestor ent tac + ent pos si hay partida
        if self.partida.numJugadas():
            jg = self.partida.jugada(-1)
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
        """
        Hace los movimientos de piezas en el tablero
        """
        # if siMovTemporizado and self.configuracion.efectosVisuales:

            # rapidez = self.configuracion.rapidezMovPiezas * 1.0 / 100.0
            # cpu = self.procesador.cpu
            # cpu.reset()
            # segundos = None

            # # primero los movimientos
            # for movim in liMovs:
                # if movim[0] == "m":
                    # if segundos is None:
                        # desde, hasta = movim[1], movim[2]
                        # dc = ord(desde[0]) - ord(hasta[0])
                        # df = int(desde[1]) - int(hasta[1])
                        # # Maxima distancia = 9.9 ( 9,89... sqrt(7**2+7**2)) = 4 segundos
                        # dist = ( dc ** 2 + df ** 2 ) ** 0.5
                        # segundos = 4.0 * dist / (9.9 * rapidez)

                    # cpu.muevePieza(movim[1], movim[2], siExclusiva=False, segundos=segundos)

            # if segundos is None:
                # segundos = 1.0

            # # segundo los borrados
            # for movim in liMovs:
                # if movim[0] == "b":
                    # n = cpu.duerme(segundos * 0.80 / rapidez)
                    # cpu.borraPieza(movim[1], padre=n)

            # # tercero los cambios
            # for movim in liMovs:
                # if movim[0] == "c":
                    # cpu.cambiaPieza(movim[1], movim[2], siExclusiva=True)

            # cpu.runLineal()

        # else:
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
        if (self.pantalla.siCapturas or self.pantalla.siInformacionPGN or self.liKibitzersActivas):
            if not hasattr(self.pgn, "jugada"):  # gestor60 por ejemplo
                return
            fila, columna = self.pantalla.pgnPosActual()
            posJugada, jg = self.pgn.jugada(fila, columna.clave)
            if jg:
                dic, siBlancas = jg.posicion.capturas()
            else:
                dic, siBlancas = self.partida.ultPosicion.capturas()

            nomApertura = ""
            apertura = self.partida.apertura
            if apertura:
                nomApertura = apertura.trNombre
            if self.pantalla.siCapturas:
                self.pantalla.ponCapturas(dic, jg, nomApertura)
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
        return (self.estado == kFinJuego) or \
               self.tipoJuego in (
                   kJugEntPos, kJugPGN, kJugEntMaq, kJugEntTac, kJugGM, kJugSolo, kJugBooks, kJugAperturas,
                   kJugXFCC ) or \
               (self.tipoJuego in (kJugElo, kJugMicElo) and not self.siCompetitivo)

    def miraKibitzers(self, jg, columnaClave):
        if jg:
            fen = jg.posicionBase.fen() if columnaClave == "NUMERO" else jg.posicion.fen()
        else:
            fen = self.partida.ultPosicion.fen()
        liApagadas = []
        for n, xkibitzer in enumerate(self.liKibitzersActivas):
            if xkibitzer.siActiva():
                xkibitzer.ponFen(fen)
            else:
                liApagadas.append(n)
        if liApagadas:
            for x in range(len(liApagadas) - 1, -1, -1):
                kibitzer = self.liKibitzersActivas[x]
                kibitzer.close()
                del self.liKibitzersActivas[x]

    def paraKibitzers(self):
        for n, xkibitzer in enumerate(self.liKibitzersActivas):
            xkibitzer.ponFen(None)

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
        # def pensando( self, siPensando ):
        # if siPensando:
        # if self.um:
        # self.um.final()
        # self.um = self.unMomento()
        # else:
        # if self.um:
        # self.um.final()
        # self.um = None

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
                jg = self.partida.liJugadas[-1]
                self.runSound.playLista(jg.listaSonidos())
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
            jg = self.partida.liJugadas[0]
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

    def pgnInformacion(self, fila, clave):
        if self.informacionActivable:
            self.pantalla.activaInformacionPGN()
            self.ponVista()

    def quitaInformacion(self, siActivable=False):
        self.pantalla.activaInformacionPGN(False)
        self.informacionActivable = siActivable

        # def vista( self ):
        # menu = QTVarios.LCMenu()

        # menu.opcion( "pgn", _( "PGN information" ), Iconos.InformacionPGNUno() )
        # menu.separador()
        # menu.opcion( "capturas", _( "Captured material" ), Iconos.Capturas() )

        # resp = menu.lanza()
        # if resp == "pgn":
        # self.pantalla.activaInformacionPGN()
        # elif resp == "capturas":
        # self.pantalla.activaCapturas()
        # self.ponVista()

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
                    "It is saved in the clipboard to paste it wherever you want.") ))

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
            self.pantalla.activaInformacionPGN()
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
        return pos, self.partida.liJugadas[pos] if tam_lj else None

    def fenActivo(self):
        pos, jg = self.jugadaActiva()
        return jg.posicion.fen() if jg else self.fenUltimo()

    def fenActivoConInicio(self):
        """
        Incluye la posicion inicial
        """

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
        tam_lj = self.partida.numJugadas()
        if 0 <= pos < tam_lj:
            return self.partida.liJugadas[pos].posicionBase.fen()
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
                self.analizaTutorInicio()

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

        jg = self.partida.liJugadas[pos]
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

        jg, siBlancas, siUltimo, tam_lj, pos = self.dameJugadaEn(fila, clave)

        if self.estado == kFinJuego:
            maxRecursion = 9999
        else:
            if not (self.tipoJuego in [kJugEntPos, kJugPGN, kJugEntMaq, kJugGM, kJugSolo, kJugBooks, kJugAperturas,
                                       kJugEntTac, kJugXFCC] or \
                            (self.tipoJuego in [kJugElo, kJugMicElo] and not self.siCompetitivo)):
                if siUltimo or self.ayudas == 0:
                    return
                maxRecursion = tam_lj - pos - 3  # %#
            else:
                maxRecursion = 9999

        if not (hasattr(jg, "analisis") and jg.analisis):
            me = QTUtil2.mensEspera.inicio(self.pantalla, _("Analyzing the move...."), posicion="ad")
            mrm, pos = self.xtutor.analizaJugada(jg, self.xtutor.motorTiempoJugada, self.xtutor.motorProfundidad)
            jg.analisis = mrm, pos
            me.final()

        Analisis.muestraAnalisis(self.procesador, self.xtutor, jg, self.tablero.siBlancasAbajo, maxRecursion, pos)
        self.ponVista()

    def analizar(self):
        Analisis.analizaPartida(self)
        self.refresh()

    def cambiaRival(self, nuevo):
        self.procesador.cambiaRival(nuevo)

    def pelicula(self):
        resp = Pelicula.paramPelicula(self.pantalla)
        if resp is None:
            return

        segundos, siInicio = resp

        self.xpelicula = Pelicula.Pelicula(self, segundos, siInicio)

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

    def ponFlechas(self, fen, siMB, si2):
        self.ml.ponFen(fen)
        self.ml.calculaEstado()
        if si2:
            li = self.ml.listaCapturas2(siMB, False)
        else:
            li = self.ml.listaCapturas(siMB)

        for m in li:
            if si2:
                m1, m2 = m
                pv = m1.pv()
                d = pv[:2]
                h = pv[2:4]
                self.tablero.creaFlechaMov(d, h, "b")
                pv = m2.pv()
                d = pv[:2]
                h = pv[2:4]
                self.tablero.creaFlechaMov(d, h, "c")
            else:
                pv = m.pv()
                d = pv[:2]
                h = pv[2:4]
                self.tablero.creaFlechaMov(d, h, "c")

    def exePulsadoNum(self, siActivar, numero):
        if numero in [1, 2, 7, 8]:
            if siActivar:
                # Que jugada esta en el tablero
                fen = self.fenActivoConInicio()
                siBlancas = " w " in fen
                if siBlancas:
                    siMB = numero in [1, 2]
                else:
                    siMB = numero in [7, 8]
                si2 = numero in [2, 7]
                self.tablero.quitaFlechas()
                if self.tablero.flechaSC:
                    self.tablero.flechaSC.hide()
                self.ponFlechas(fen, siMB, si2)
            else:
                self.tablero.quitaFlechas()
                if self.tablero.flechaSC:
                    self.tablero.flechaSC.show()

    def trasteros(self, orden):

        if orden == "nuevo":
            resp = QTUtil2.salvaFichero(self.pantalla, _("Boxrooms PGN"), self.configuracion.dirSalvados,
                                        _("File") + " pgn (*.pgn)", False)
            if resp:
                carpeta, trastero = os.path.split(resp)
                if carpeta != self.configuracion.dirSalvados:
                    self.configuracion.dirSalvados = carpeta
                    self.configuracion.graba()

                orden = None
                for n, (carpeta1, trastero1) in enumerate(self.configuracion.liTrasteros):
                    if carpeta1.lower() == carpeta.lower() and trastero1.lower() == trastero.lower():
                        orden = len(self.configuracion.liTrasteros) - 1
                        break

                if orden is None:
                    self.configuracion.liTrasteros.append((carpeta, trastero))
                    self.configuracion.graba()
                    orden = len(self.configuracion.liTrasteros) - 1
                self.trasteros(str(orden))  # para que grabe

        elif orden.startswith("quitar"):
            nquitar = int(orden[7:])
            del self.configuracion.liTrasteros[nquitar]
            self.configuracion.graba()

        else:
            carpeta, trastero = self.configuracion.liTrasteros[int(orden)]
            fichero = os.path.join(carpeta, trastero)
            dato = self.listado("pgn")
            if os.path.isfile(fichero):
                dato = "\n" * 2 + dato
            try:
                f = codecs.open(fichero, "a", 'utf-8', 'ignore')
                f.write(dato)
                f.close()
                QTUtil2.mensaje(self.pantalla, _X(_("Saved to %1"), fichero.replace("/", "\\")))
            except:
                QTUtil2.mensError(self.pantalla, "%s : %s\n" % ( _("Unable to save"), fichero.replace("/", "\\") ))

    def kibitzers(self, liKibitzers, orden):

        if orden == "nueva":
            liKibitzers = XKibitzers.nuevaKibitzer(self.pantalla, self.configuracion)
            if liKibitzers:
                self.kibitzers(liKibitzers, str(len(liKibitzers) - 1))

        elif orden.startswith("quitar"):
            nquitar = int(orden[7:])
            del liKibitzers[nquitar]
            XKibitzers.listaKibitzersGrabar(self.configuracion, liKibitzers)

        else:
            nkibitzer = int(orden)
            kibitzer = liKibitzers[nkibitzer]
            xkibitzer = XKibitzers.XKibitzer(self, kibitzer)
            self.liKibitzersActivas.append(xkibitzer)
            self.ponVista()

    def paraHumano(self):
        self.siJuegaHumano = False
        self.desactivaTodas()

    def sigueHumano(self):
        self.siTeclaPanico = False
        self.siJuegaHumano = True
        self.activaColor(self.partida.ultPosicion.siBlancas)

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
                    self.ayudas or self.tipoJuego in (kJugEntMaq, kJugSolo, kJugEntPos, kJugEntTac) ):
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

        icoNegro = Iconos.PuntoNegro()
        icoVerde = Iconos.PuntoVerde()

        menu = QTVarios.LCMenu(self.pantalla)

        # Vista
        menuVista = menu.submenu(_("View"), Iconos.Vista())
        menuVista.opcion("vista_pgn", _("PGN information"), Iconos.InformacionPGNUno())
        menuVista.separador()
        menuVista.opcion("vista_capturas", _("Captured material"), Iconos.Capturas())
        menuVista.separador()
        menuVista.opcion("vista_colorpgn", _("Colors") + " - " + _("PGN"), Iconos.Temas())
        menu.separador()

        # DGT
        if self.configuracion.siDGT:
            menu.opcion("dgt", _("Disable %1") if DGT.siON() else _("Enable %1"), _("DGT board"), Iconos.DGT())
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
            menuSonido = menu.submenu(_("Sounds"), Iconos.S_Play())

            def ico_tit(si):
                return (icoNegro, _("Sound off in :")) if not si else (icoVerde, _("Sound on in :"))

            ico, tit = ico_tit(self.configuracion.siSuenaBeep)
            menuSonido.opcion("sonido_beep", tit + " " + _("Beep after opponent's move"), ico)

            menuSonido.separador()

            ico, tit = ico_tit(self.configuracion.siSuenaResultados)
            menuSonido.opcion("sonido_resultado", tit + " " + _("Results"), ico)

            menuSonido.separador()

            ico, tit = ico_tit(self.configuracion.siSuenaJugada)
            menuSonido.opcion("sonido_jugada", tit + " " + _("Rival moves"), ico)

            menuSonido.separador()

            ico, tit = ico_tit(self.configuracion.siSuenaNuestro)
            menuSonido.opcion("sonido_jugador", tit + " " + _("Activate sounds with our moves"), ico)

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

            if resp.startswith("vista_"):
                resp = resp[6:]
                if resp == "pgn":
                    self.pantalla.activaInformacionPGN()
                    self.ponVista()
                elif resp == "capturas":
                    self.pantalla.activaCapturas()
                    self.ponVista()
                elif resp == "colorpgn":
                    PantallaColores.cambiaColoresPGN(self.pantalla, self.configuracion)
                    self.pantalla.base.pgnRefresh()

            elif resp == "dgt":
                DGT.cambiarON_OFF()
                self.compruebaDGT()

            elif resp.startswith("trastero"):
                self.trasteros(resp[9:])

            elif resp.startswith("sonido_"):
                if resp.endswith("jugada"):
                    self.configuracion.siSuenaJugada = not self.configuracion.siSuenaJugada
                elif resp.endswith("resultado"):
                    self.configuracion.siSuenaResultados = not self.configuracion.siSuenaResultados
                elif resp.endswith("beep"):
                    self.configuracion.siSuenaBeep = not self.configuracion.siSuenaBeep
                elif resp.endswith("jugador"):
                    self.configuracion.siSuenaNuestro = not self.configuracion.siSuenaNuestro
                self.configuracion.graba()

            elif resp == "tutor":
                self.cambioTutor()

            elif resp == "motores":
                self.procesador.motoresExternos()

            elif resp == "ontop":
                self.pantalla.onTopWindow()

            elif resp.startswith("cg_"):
                orden = resp[3:]
                if orden == "pgn":
                    self.pgn.siMostrar = not self.pgn.siMostrar
                    self.pantalla.base.pgnRefresh()
                elif orden == "change":
                    self.tablero.blindfoldChange()

                elif orden == "conf":
                    self.tablero.blindfoldConfig()

        return None

    def utilidades(self, liMasOpciones=None, siArbol=True):

        menu = QTVarios.LCMenu(self.pantalla)

        siJugadas = self.partida.numJugadas() > 0

        # Grabar
        icoGrabar = Iconos.Grabar()
        icoFichero = Iconos.GrabarFichero()
        icoCamara = Iconos.Camara()
        icoClip = Iconos.Clip()
        icoAzul = Iconos.PuntoAzul()
        icoTras = Iconos.Trastero()
        icoNegro = Iconos.PuntoNegro()
        icoVerde = Iconos.PuntoVerde()
        icoNaranja = Iconos.PuntoNaranja()
        icoMagenta = Iconos.PuntoMagenta()
        icoRojo = Iconos.PuntoRojo()

        trFichero = _("Save to a file")
        trPortapapeles = _("Copy to clipboard")

        menuGR = menu.submenu(_("Save"), icoGrabar)

        menuPGN = menuGR.submenu(_("PGN Format"), icoAzul)
        menuPGN.opcion("pgnfichero", trFichero, icoFichero)
        menuPGN.opcion("pgnportapapeles", trPortapapeles, icoClip)

        menuGR.separador()

        menuFEN = menuGR.submenu(_("FEN Format"), icoAzul)
        menuFEN.opcion("fenfichero", trFichero, icoFichero)
        menuFEN.opcion("fenportapapeles", trPortapapeles, icoClip)

        menuGR.separador()

        menuFNS = menuGR.submenu(_("List of FENs"), icoAzul)
        menuFNS.opcion("fnsfichero", trFichero, icoFichero)
        menuFNS.opcion("fnsportapapeles", trPortapapeles, icoClip)

        menuGR.separador()

        menuGR.opcion("pksfichero", "%s -> %s" % (_("PKS Format"), _("Create your own game")),
                      Iconos.JuegaSolo())

        menuGR.separador()

        # menuGR.opcion("jsonfichero", _("JSON Format"), Iconos.JuegaSolo())

        # menuGR.separador()

        menuV = menuGR.submenu(_("Board -> Image"), icoCamara)
        menuV.opcion("volfichero", trFichero, icoFichero)
        menuV.opcion("volportapapeles", trPortapapeles, icoClip)

        menuGR.separador()

        liTras = self.configuracion.liTrasteros
        menuTras = menuGR.submenu(_("Boxrooms PGN"), Iconos.Trasteros())
        for ntras, uno in enumerate(liTras):
            carpeta, trastero = uno
            menuTras.opcion("trastero_%d" % ntras, "%s  (%s)" % (trastero, carpeta), icoTras)
        menuTras.separador()
        menuTras.opcion("trastero_nuevo", _("New boxroom"), Iconos.Trastero_Nuevo())
        if liTras:
            icoQuitar = Iconos.Trastero_Quitar()
            menuTrasQ = menuTras.submenu(_("Remove boxroom from the list (not deleting it)"), icoQuitar)
            for ntras, uno in enumerate(liTras):
                carpeta, trastero = uno
                menuTrasQ.opcion("trastero_quitar_%d" % ntras, "%s  (%s)" % (trastero, carpeta), icoQuitar)

        menu.separador()

        # Analizar
        if siJugadas:
            if not ( self.tipoJuego in ( kJugElo, kJugMicElo) and self.siCompetitivo and self.estado == kJugando ):
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

        # Pelicula
        if siJugadas:
            menu.opcion("pelicula", _("Replay game"), Iconos.Pelicula())
            menu.separador()

        # Kibitzers
        if self.siMiraKibitzers():
            menu.separador()
            menuKibitzers = menu.submenu(_("Kibitzers"), Iconos.Kibitzer())

            liKibitzers = XKibitzers.listaKibitzersRecuperar(self.configuracion)

            dico = {"S": icoVerde, "C": icoAzul, "J": icoNaranja, "I": icoNegro, "L": icoMagenta, "M": icoRojo}
            for nkibitzer, kibitzer in enumerate(liKibitzers):
                menuKibitzers.opcion("kibitzer_%d" % nkibitzer, kibitzer["NOMBRE"], dico[kibitzer["TIPO"]])

            menuKibitzers.separador()
            menuKibitzers.opcion("kibitzer_nueva", _("New"), Iconos.Nuevo())
            if liKibitzers:
                menuKibitzers.separador()
                menuKibitzersBorrar = menuKibitzers.submenu(_("Remove"), Iconos.Borrar())
                for nkibitzer, kibitzer in enumerate(liKibitzers):
                    menuKibitzersBorrar.opcion("kibitzer_quitar_%d" % nkibitzer, kibitzer["NOMBRE"], icoNegro)

        # Juega por mi
        if self.siJuegaPorMi and self.estado == kJugando and (
                    self.ayudas or self.tipoJuego in (kJugEntMaq, kJugSolo, kJugEntPos, kJugEntTac) ):
            menu.separador()
            menu.opcion("juegapormi", _("Plays instead of me") + "  [^1]", Iconos.JuegaPorMi()),

        # Arbol de movimientos
        if siArbol:
            menu.separador()
            menu.opcion("arbol", _("Moves tree"), Iconos.Arbol())

        # Mas Opciones
        if liMasOpciones:
            menu.separador()
            for clave, rotulo, icono in liMasOpciones:
                if rotulo is None:
                    menu.separador()
                else:
                    menu.opcion(clave, rotulo, icono)

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

        elif resp == "pelicula":
            self.pelicula()

        elif resp.startswith("trastero"):
            self.trasteros(resp[9:])

        elif resp.startswith("kibitzer_"):
            self.kibitzers(liKibitzers, resp[9:])

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

            # elif resp == "jsonfichero":
            # self.salvaJSON()

        else:
            extension = resp[:3]
            siFichero = resp.endswith("fichero")
            self.salvaFEN_PGN(extension, siFichero)
        return None

    def showAnalisis(self):
        um = self.procesador.unMomento()
        alm = AnalisisGraph.genSVG(self.partida)
        alm.indexesHTML, alm.indexesRAW = AnalisisIndexes.genIndexes(self.partida)
        um.final()
        PantallaAnalisis.showGraph(self.pantalla, self, alm, Analisis.muestraAnalisis)

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

            # def salvaJSON(self):
            # pgn = self.listado("pgn")
            # dic = self.procesador.saveAsJSON(self.estado, self.partida, pgn)
            # import json
            # data = json.dumps(dic, indent=4)
            # extension = "json"
            # fichero = self.configuracion.dirJS
            # while True:
            # fichero = QTUtil2.salvaFichero(self.pantalla, _("File to save"), fichero,
            # _("File") + " %s (*.%s)" % (extension, extension),
            # siConfirmarSobreescritura=True)
            # if fichero:
            # fichero = str(fichero)
            # if os.path.isfile(fichero):
            # yn = QTUtil2.preguntaCancelar(self.pantalla,
            # _X(_("The file %1 already exists, what do you want to do?"), fichero),
            # si=_("Overwrite"), no=_("Choose another"))
            # if yn is None:
            # break
            # if not yn:
            # continue
            # direc = os.path.dirname(fichero)
            # if direc != self.configuracion.dirJS:
            # self.configuracion.dirJS = direc
            # self.configuracion.graba()

            # f = open(fichero, "wb")
            # f.write(data)
            # f.close()

            # nombre = os.path.basename(fichero)
            # QTUtil2.mensajeTemporal(self.pantalla, _X(_("Saved to %1"), nombre), 0.8)
            # return

            # break

    def salvaFEN_PGN(self, extension, siFichero):
        dato = self.listado(extension)
        if siFichero:
            if extension == "pgn":
                QTVarios.savePGN( self.pantalla, self.pgn.actual())
                return

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
                        _("Unable to save"), resp, _("It is saved in the clipboard to paste it wherever you want.") ))

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

            txt = "%s||%s|%s\n" % ( fen, siguientes, pgn )
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

        w = WBGuide.WBGuide(self.pantalla, self, fenM2inicial=fenM2, pvInicial=move)
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
            self.partida.liJugadas[-1].siTablasAcuerdo = True
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
                self.partida.liJugadas[-1].siTablasAcuerdo = True
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
                ( "libros", _("Consult a book"), Iconos.Libros() ),
                ( None, None, None ),
                ( "bookguide", _("Personal Opening Guide"), Iconos.BookGuide() ),
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
        f = open(self.configuracion.ficheroSelectedPositions, "ab")
        f.write(lineaTraining + "\n")
        f.close()
        QTUtil2.mensajeTemporal(self.pantalla, _('Position saved in "Selected positions" file.'), 2)
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
                fen = jg.posicionBase.fen()
            li = []
            if sys.argv[0].endswith(".py"):
                li.append("pythonw.exe" if VarGen.isWindows else "python")
                li.append("Lucas.py")
            else:
                li.append("Lucas.exe" if VarGen.isWindows else "Lucas")
            li.extend(["-play", fen])
            subprocess.Popen(li)

    def showPV(self, pv, nArrows):
        if not pv:
            return True
        self.tablero.quitaFlechas()
        tipo = "m1"
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

    # def savePGN( self ):
        # QTVarios.savePGN( self.pantalla, self.pgn.actual())

        # dicVariables = self.configuracion.leeVariables("SAVEPGN")

        # liGen = [(None, None)]

        # liHistorico = dicVariables.get("LIHISTORICO")

        # config = FormLayout.Fichero(_("File to save"), "pgn", True, liHistorico=liHistorico, anchoMinimo=300)
        # liGen.append(( config, "" ))

        # #Codec
        # liCodecs = [k for k in set(v for k,v in aliases.iteritems())]
        # liCodecs.sort()
        # liCodecs = [(k,k) for k in liCodecs]
        # liCodecs.insert( 0, (_("Same as file"), "file" ) )
        # liCodecs.insert( 0, ("%s: UTF-8"%_("By default"), "default" ) )
        # config = FormLayout.Combobox(_("Write with the codec"), liCodecs)
        # codec = dicVariables.get("CODEC", "default")
        # liGen.append(( config, codec ))

        # #Overwrite
        # liGen.append( ( _("Overwrite"), dicVariables.get("OVERWRITE", False)) )

        # #Remove comments
        # liGen.append( ( _("Remove comments and variations"), dicVariables.get("REMCOMMENTSVAR", False)) )

        # # Editamos
        # resultado = FormLayout.fedit(liGen, title=_("Save PGN"), parent=self.pantalla, icon=Iconos.PGN())
        # if resultado is None:
            # return

        # accion, liResp = resultado
        # fichero, codec, overwrite, remcommentsvar = liResp
        # if not fichero:
            # return
        # if not liHistorico:
            # liHistorico = []
        # if fichero in liHistorico:
            # del liHistorico[liHistorico.index(fichero)]
        # liHistorico.insert(0,fichero)

        # dicVariables["LIHISTORICO"] = liHistorico[:20]
        # dicVariables["CODEC"] = codec
        # dicVariables["OVERWRITE"] = overwrite
        # dicVariables["REMCOMMENTSVAR"] = remcommentsvar

        # self.configuracion.escVariables("SAVEPGN",dicVariables)
        # carpeta, name = os.path.split(fichero)
        # if carpeta != self.configuracion.dirSalvados:
            # self.configuracion.dirSalvados = carpeta
            # self.configuracion.graba()

        # if remcommentsvar:
            # p = partida.copia()
            # p.borraCV( )
            # x = self.partida
            # self.partida = p
            # pgn = self.pgn.actual()
            # self.partida = x
        # else:
            # pgn = self.pgn.actual()
        # pgn = pgn.replace( "\n", "\r\n" )

        # modo = "w" if overwrite else "a"
        # if not overwrite:
            # if not Util.existeFichero(fichero):
                # modo = "w"
        # if codec == "default":
            # codec = "utf-8"
        # elif code == "file":
            # codec = "utf-8"
            # if Util.existeFichero(fichero):
                # f = open(fich)
                # u = chardet.universaldetector.UniversalDetector()
                # for n, x in enumerate(f):
                    # u.feed(x)
                    # if n == 1000:
                        # break
                # f.close()
                # u.close()
                # codec = u.result.get("encoding", "utf-8")

        # try:
            # f = codecs.open( fichero, modo, codec, 'ignore' )
            # if modo == "a":
                # f.write( "\r\n\r\n" )
            # f.write(pgn)
            # f.close()
            # QTUtil2.mensajeTemporal( self.pantalla, _( "Saved" ), 1.2 )
        # except:
            # QTUtil.ponPortapapeles(pgn)
            # QTUtil2.mensError(self.pantalla, "%s : %s\n\n%s" % (_("Unable to save"), fichero, _("It is saved in the clipboard to paste it wherever you want.") ))

