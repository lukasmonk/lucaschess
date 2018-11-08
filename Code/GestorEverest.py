import copy
import time

from PyQt4 import QtCore

from Code import Apertura
from Code import Everest
from Code import Gestor
from Code.QT import PantallaJuicio
from Code.QT import QTUtil2
from Code.Constantes import *


class GestorEverest(Gestor.Gestor):
    def inicio(self, recno):

        self.expedition = Everest.Expedition(self.configuracion, recno)
        self.expedition.run()

        self.dic_analysis = {}

        self.siCompetitivo = True
        self.resultado = None
        self.siJuegaHumano = False
        self.analisis = None
        self.comentario = None
        self.siAnalizando = False
        self.siJugamosConBlancas = self.expedition.is_white
        self.siRivalConBlancas = not self.expedition.is_white
        self.partidaObj = self.expedition.partida
        self.numJugadasObj = self.partidaObj.numJugadas()
        self.posJugadaObj = 0
        self.nombreObj = self.expedition.nombre

        self.xanalyzer.maximizaMultiPV()

        self.puntos = 0
        self.tiempo = 0.0

        self.book = Apertura.AperturaPol(999)

        self.pantalla.ponToolBar((k_cancelar, k_reiniciar, k_configurar))

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, True)

        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.mostrarIndicador(True)
        self.ponRotulo1(self.expedition.label())
        self.ponRotulo2("")

        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()
        self.ponPosicionDGT()

        self.estado = kJugando
        self.siguienteJugada()

    def ponPuntos(self):
        self.ponRotulo2("%s : <b>%d</b>" % (_("Points"), self.puntos))

    def procesarAccion(self, clave):
        if clave == k_cancelar:
            self.cancelar()

        elif clave == k_reiniciar:
            if not QTUtil2.pregunta(self.pantalla, _("Restart the game?")):
                return
            change_game = self.restart(False)
            if change_game:
                self.terminar()
            else:
                self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidadesElo()

        elif clave == k_mainmenu:
            self.terminar()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        return self.cancelar()

    def cancelar(self):
        if self.posJugadaObj > 1 and self.estado == kJugando:
            self.restart(False)
        self.terminar()
        return False

    def terminar(self):
        self.analizaTerminar()
        self.terminaNoContinuo()
        self.procesador.inicio()
        self.procesador.showEverest(self.expedition.recno)

    def reiniciar(self):
        self.partida.reset()
        self.posJugadaObj = 0
        self.puntos = 0
        self.ponPuntos()
        self.tiempo = 0.0
        self.book = Apertura.AperturaPol(999)
        self.estado = kJugando
        self.tablero.ponPosicion(self.partida.iniPosicion)
        self.pgnRefresh(True)
        self.ponPosicionDGT()
        self.analizaFinal()
        self.terminaNoContinuo()

        self.ponRotulo1(self.expedition.label())
        self.ponPuntos()
        self.siguienteJugada()

    def restart(self, lost_points):
        self.terminaNoContinuo()
        change_game = self.expedition.add_try(False, self.tiempo, self.puntos)
        self.tiempo = 0.0
        licoment = []
        if lost_points:
            licoment.append(_("You have exceeded the limit of lost points."))

        if change_game:
            licoment.append(_("You have exceeded the limit of tries, you will fall back to the previous."))
        elif lost_points:
            licoment.append(_("You must repeat the game from beginning."))
        if licoment:
            comentario = "\n".join(licoment)
            w = PantallaJuicio.MensajeF(self.pantalla, comentario)
            w.mostrar()
        return change_game

    def analizaInicio(self):
        if not self.siTerminada():
            self.xanalyzer.ac_inicio(self.partida)
            self.siAnalizando = True

    def analizaMinimo(self, minTime):
        self.mrm = copy.deepcopy(self.xanalyzer.ac_minimo(minTime, False))
        return self.mrm

    def analizaEstado(self):
        self.xanalyzer.motor.ac_lee()
        self.mrm = copy.deepcopy(self.xanalyzer.ac_estado())
        return self.mrm

    def analizaFinal(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xanalyzer.ac_final(-1)

    def analizaTerminar(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xanalyzer.terminar()

    def analizaNoContinuo(self):
        self.tiempoNoContinuo += 500
        if self.tiempoNoContinuo >= 5000:
            self.analizaMinimo(5)
            self.analizaFinal()
            self.pendienteNoContinuo = False
        else:
            QtCore.QTimer.singleShot(500, self.analizaNoContinuo)

    def analizaNoContinuoFinal(self):
        if self.tiempoNoContinuo < 5000:
            um = QTUtil2.analizando(self.pantalla)
            self.analizaMinimo(5000)
            um.final()

    def terminaNoContinuo(self):
        if not self.continueTt:
            self.tiempoNoContinuo = 99999
            self.pendienteNoContinuo = False

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        if self.puntos < -self.expedition.tolerance:
            self.restart(True)
            self.estado = kFinJuego
            self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_configurar, k_utilidades))
            return
            # if change_game:
            #     self.terminar()
            #     return
            # self.reiniciar()

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()
        siBlancas = self.partida.ultPosicion.siBlancas

        numJugadas = self.partida.numJugadas()
        if numJugadas >= self.numJugadasObj:
            self.ponResultado()
            return

        siRival = siBlancas == self.siRivalConBlancas
        self.ponIndicador(siBlancas)

        self.refresh()

        if siRival:
            self.masJugada(False)
            self.siguienteJugada()

        else:
            self.siJuegaHumano = True
            self.pensando(True)
            self.analizaInicio()
            self.activaColor(siBlancas)
            self.pensando(False)
            self.iniTiempo = time.time()
            if not self.continueTt:
                QtCore.QTimer.singleShot(1000, self.analizaNoContinuo)
                self.tiempoNoContinuo = 0
                self.pendienteNoContinuo = True

    def mueveHumano(self, desde, hasta, coronacion=""):
        jgUsu = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgUsu:
            return False

        self.tiempo += time.time() - self.iniTiempo

        jgObj = self.partidaObj.jugada(self.posJugadaObj)
        fen = self.fenUltimo()

        siAnalizaJuez = True
        if self.book:
            siBookUsu = self.book.compruebaHumano(fen, desde, hasta)
            siBookObj = self.book.compruebaHumano(fen, jgObj.desde, jgObj.hasta)
            if siBookUsu and siBookObj:
                if jgObj.movimiento() != jgUsu.movimiento():
                    bmove = _("book move")
                    comentario = "%s: %s %s<br>%s: %s %s" % (self.nombreObj, jgObj.pgnSP(), bmove,
                                                             self.configuracion.jugador, jgUsu.pgnSP(), bmove)
                    w = PantallaJuicio.MensajeF(self.pantalla, comentario)
                    w.mostrar()
                siAnalizaJuez = False
            else:
                siAnalizaJuez = True
                if not siBookObj:
                    self.book = None

        analisis = None
        comentario = None

        if siAnalizaJuez:
            posicion = self.partida.ultPosicion
            saved = fen in self.dic_analysis
            if saved:
                rmObj, posObj, analisis, mrm = self.dic_analysis[fen]
            else:
                if self.continueTt:
                    um = QTUtil2.analizando(self.pantalla)
                    mrm = self.analizaMinimo(5000) if self.continueTt else self.mrm
                    um.final()
                else:
                    self.analizaNoContinuoFinal()
                    mrm = self.mrm
                rmObj, posObj = mrm.buscaRM(jgObj.movimiento())
                analisis = mrm, posObj
                self.dic_analysis[fen] = [rmObj, posObj, analisis, mrm]

            rmUsu, posUsu = mrm.buscaRM(jgUsu.movimiento())
            if rmUsu is None:
                um = QTUtil2.analizando(self.pantalla)
                self.analizaFinal()
                rmUsu = self.xanalyzer.valora(posicion, desde, hasta, coronacion)
                mrm.agregaRM(rmUsu)
                self.analizaInicio()
                um.final()

            w = PantallaJuicio.WJuicio(self, self.xanalyzer, self.nombreObj, posicion, mrm, rmObj, rmUsu, analisis,
                                       siCompetitivo=False)
            w.exec_()

            if not saved:
                analisis = w.analisis
                self.dic_analysis[fen][2] = analisis

            dpts = w.difPuntos()
            self.puntos += dpts
            self.ponPuntos()

            if posUsu != posObj:
                comentarioUsu = " %s" % (rmUsu.abrTexto())
                comentarioObj = " %s" % (rmObj.abrTexto())

                comentarioPuntos = "%s = %d %+d %+d = %d" % (_("Points"), self.puntos - dpts, rmUsu.puntosABS(),
                                                             -rmObj.puntosABS(), self.puntos)
                comentario = "%s: %s %s\n%s: %s %s\n%s" % (self.nombreObj, jgObj.pgnSP(), comentarioObj,
                                                           self.configuracion.jugador, jgUsu.pgnSP(), comentarioUsu,
                                                           comentarioPuntos)
        if not self.continueTt:
            self.terminaNoContinuo()

        self.analizaFinal()

        self.masJugada(True, analisis, comentario)

        self.siguienteJugada()
        return True

    def masJugada(self, siNuestra, analisis=None, comentario=None):
        jg = self.partidaObj.jugada(self.posJugadaObj)
        self.posJugadaObj += 1
        if analisis:
            jg.analisis = analisis
        if comentario:
            jg.comentario = comentario

        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()
        self.movimientosPiezas(jg.liMovs, True)
        self.tablero.ponPosicion(jg.posicion)
        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

    def ponResultado(self):
        self.analizaTerminar()
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.estado = kFinJuego

        mensaje = _("Congratulations you have passed this game.")
        self.expedition.add_try(True, self.tiempo, self.puntos)

        self.mensajeEnPGN(mensaje)

        self.terminar()
