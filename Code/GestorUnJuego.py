import copy
import time

from Code import Apertura
from Code import Gestor
from Code.QT import PantallaJuicio
from Code.QT import QTUtil2
from Code.Constantes import *

class GestorUnJuego(Gestor.Gestor):
    def inicio(self, recno, partidaObj, nombreObj, siBlancas, rotulo):
        self.recno = recno
        self.siCompetitivo = False
        self.resultado = None
        self.siJuegaHumano = False
        self.analisis = None
        self.comentario = None
        self.siAnalizando = False
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas
        self.numJugadasObj = partidaObj.numJugadas()
        self.partidaObj = partidaObj
        self.posJugadaObj = 0
        self.nombreObj = nombreObj

        self.xanalyzer.maximizaMultiPV()

        self.puntos = 0
        self.tiempo = 0.0

        self.book = Apertura.AperturaPol(999)

        self.pantalla.ponToolBar((k_cancelar, k_reiniciar, k_configurar, k_utilidades))

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, True)

        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.mostrarIndicador(True)
        self.ponRotulo1(rotulo)
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
            self.reiniciar(True)

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidadesElo()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        return self.cancelar()

    def cancelar(self):
        self.puntos = -999
        self.analizaTerminar()
        self.procesador.inicio()
        return False

    def reiniciar(self, siPregunta):
        if siPregunta:
            if not QTUtil2.pregunta(self.pantalla, _("Restart the game?")):
                return

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

        self.siguienteJugada()

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

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

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

    def mueveHumano(self, desde, hasta, coronacion=""):
        jgUsu = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgUsu:
            return False

        self.tiempo += time.time() - self.iniTiempo

        jgObj = self.partidaObj.jugada(self.posJugadaObj)

        siAnalizaJuez = True
        if self.book:
            fen = self.fenUltimo()
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
            um = QTUtil2.analizando(self.pantalla)
            mrm = self.analizaMinimo(3000)
            posicion = self.partida.ultPosicion

            rmUsu, nada = mrm.buscaRM(jgUsu.movimiento())
            rmObj, posObj = mrm.buscaRM(jgObj.movimiento())

            analisis = mrm, posObj
            um.final()

            w = PantallaJuicio.WJuicio(self, self.xanalyzer, self.nombreObj, posicion, mrm, rmObj, rmUsu, analisis)
            w.exec_()

            analisis = w.analisis
            dpts = w.difPuntos()
            self.puntos += dpts
            self.ponPuntos()

            comentarioUsu = " %s" % (rmUsu.abrTexto())
            comentarioObj = " %s" % (rmObj.abrTexto())

            comentarioPuntos = "%s = %d %+d %+d = %d" % (_("Points"), self.puntos - dpts, rmUsu.puntosABS(),
                                                         -rmObj.puntosABS(), self.puntos)

            comentario = "%s: %s %s\n%s: %s %s\n%s" % (self.nombreObj, jgObj.pgnSP(), comentarioObj,
                                                       self.configuracion.jugador, jgUsu.pgnSP(), comentarioUsu,
                                                       comentarioPuntos)

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
            self.listaAperturasStd.asignaApertura(self.partida)
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

        if self.puntos < 0:
            mensaje = _("Unfortunately you have lost.")
            quien = kGanaRival
        else:
            mensaje = _("Congratulations you have won.")
            quien = kGanamos

        self.beepResultado(quien)

        QTUtil2.mensaje(self.pantalla, mensaje)
        self.ponFinJuego()
