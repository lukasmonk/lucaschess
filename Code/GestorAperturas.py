# -*- coding: latin-1 -*-

import random

from Code.Constantes import *
import Code.Util as Util
import Code.Jugada as Jugada
import Code.ControlPosicion as ControlPosicion
import Code.XMotorRespuesta as XMotorRespuesta
import Code.Gestor as Gestor
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.QTVarios as QTVarios
import Code.QT.Iconos as Iconos

class GestorAperturas(Gestor.Gestor):
    def inicio(self, listaAperturasStd, ficheroDatos, lista, fila, jugamos, repeticiones, rep_actual):

        self.tipoJuego = kJugAperturas

        self.ayudas = 9999  # Para que analice sin problemas

        self.siRepeticionValida = True

        self.listaAperturasStd, self.ficheroDatos, self.lista, self.fila, self.jugamos, self.repeticiones, self.rep_actual = \
            listaAperturasStd, ficheroDatos, lista, fila, jugamos, repeticiones, rep_actual
        self.lee()

        self.siJugamosConBlancas = jugamos != "NEGRAS"
        self.siRivalConBlancas = None if jugamos == "AMBOS" else not self.siJugamosConBlancas

        self.pantalla.ponToolBar(( k_mainmenu, k_ayuda, k_reiniciar, k_configurar, k_utilidades, k_siguiente ))
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.quitaAyudas()
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.ponRotulo1(lista[fila]["NOMBRE"])
        self.ponRotulo2(self.txtAciertos())
        self.pgnRefresh(True)

        self.ponCapInfoPorDefecto()

        self.estado = kJugando

        self.ponPosicionDGT()

        self.quitaInformacion()

        self.siguienteJugada()

    def menuAyuda(self):

        self.siRepeticionValida = False

        menu = QTVarios.LCMenu(self.pantalla)

        for n, (pv, desde, hasta, coronacion, pgn) in enumerate(self.listaJugadas):
            menu.opcion(n, pgn + " - " + self.dicActual[pv]["NOMBRE"], Iconos.PuntoVerde())
            menu.separador()

        menu.lanza()

    def lee(self):
        lili = self.lista[self.fila]["LISTA"]

        dic = {}
        for x in lili:
            td = dic
            lx = x.split(" ")
            for a1h8 in lx:
                if a1h8 in td:
                    td = td[a1h8]["HIJOS"]
                else:
                    td[a1h8] = {}
                    td = td[a1h8]
                    td["NOMBRE"] = self.listaAperturasStd.dic[x].nombre
                    td["HIJOS"] = {}
                    td = td["HIJOS"]

        self.dicActual = dic
        self.dicBase = dic

    def procesarAccion(self, clave):

        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_siguiente:
            self.finPartida()
            self.procesador.entrenamientos.aperturas()

        elif clave == k_ayuda:
            self.menuAyuda()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        return self.finPartida()

    def finPartida(self):
        self.procesador.inicio()
        return False

    def reiniciar(self):
        self.partida.reset()
        self.inicio(self.listaAperturasStd, self.ficheroDatos, self.lista, self.fila, self.jugamos, self.repeticiones,
                    self.rep_actual)

    def siguienteJugada(self):

        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        fen = self.partida.ultPosicion.fen()
        self.listaJugadas = self.miraListaJugadas(fen)

        if not self.listaJugadas:
            self.ponResultado()
            return

        siRival = siBlancas == self.siRivalConBlancas

        if siRival:
            self.desactivaTodas()

            nli = len(self.listaJugadas)
            if nli > 1:
                pos = random.randint(0, nli - 1)
            else:
                pos = 0

            pv, desde, hasta, coronacion, pgn = self.listaJugadas[pos]
            self.ultPV = pv

            self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
            self.rmRival.desde = desde
            self.rmRival.hasta = hasta
            self.rmRival.coronacion = coronacion

            self.mueveRival(self.rmRival)
            self.siguienteJugada()

        else:

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def miraListaJugadas(self, fen):
        posicion = ControlPosicion.ControlPosicion()
        posicion.leeFen(fen)

        listaJugadas = []
        for pv in self.dicActual.keys():
            desde, hasta, coronacion = pv[:2], pv[2:4], pv[4:]
            pgn = posicion.pgnSP(desde, hasta, coronacion)
            listaJugadas.append(( pv, desde, hasta, coronacion, pgn ))
        return listaJugadas

    def mueveHumano(self, desde, hasta, coronacion=None):

        if self.siJuegaHumano:
            self.paraHumano()
        else:
            self.sigueHumano()
            return False

        # Peón coronando
        if not coronacion and self.partida.ultPosicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.partida.ultPosicion.siBlancas)
            if coronacion is None:
                self.sigueHumano()
                return False

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)

        if not siBien:
            self.sigueHumano()
            self.error = mens
            return False

        siEncontrado = False
        if coronacion is None:
            coronacion = ""
        for pv, jdesde, jhasta, jcoronacion, jpgn in self.listaJugadas:
            if desde == jdesde and hasta == jhasta and coronacion == jcoronacion:
                siEncontrado = True
                break

        if not siEncontrado:
            self.menuAyuda()
            self.sigueHumano()
            return False

        self.ultPV = desde + hasta + coronacion
        self.ponRotulo2(self.txtAciertos())

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):

        # Preguntamos al motor si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.liJugadas.append(jg)
        if self.partida.pendienteApertura:
            self.listaAperturasStd.asignaApertura(self.partida)

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.dicActual = self.dicActual[self.ultPV]["HIJOS"]

        self.ponPosicionDGT()

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

    def txtAciertos(self):
        if self.repeticiones == 0:
            txt = _("Repetitions") + " : %d" % self.rep_actual
        else:
            txt = _X(_("%1 repetitions remain"), str(self.repeticiones - self.rep_actual))
        return txt

    def ponResultado(self):
        self.estado = kFinJuego
        self.tablero.desactivaTodas()
        if self.siRepeticionValida:
            self.rep_actual += 1
            self.lista[self.fila][self.jugamos] += 1

            Util.guardaVar(self.ficheroDatos, self.lista)
        self.ponRotulo2(self.txtAciertos())

        if self.rep_actual == self.repeticiones:
            self.finPartida()
            self.procesador.entrenamientos.aperturas()
            return

        mensaje = "%s\n\n%s" % ( self.txtAciertos(), _("Do you want to continue?"))

        resp = QTUtil2.pregunta(self.pantalla, mensaje)
        if resp:
            self.reiniciar()

