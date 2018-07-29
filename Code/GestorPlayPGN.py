import copy
import time

from Code import Util
from Code import Apertura
from Code import Partida
from Code import Gestor
from Code.QT import PantallaJuicio
from Code.QT import QTUtil2
from Code.Constantes import *
from Code.QT import PantallaPlayPGN


class GestorUnJuego(Gestor.Gestor):
    def inicio(self, recno, siBlancas):

        db = PantallaPlayPGN.PlayPGNs(self.configuracion.ficheroPlayPGN)
        reg = db.leeRegistro(recno)
        partidaObj = Partida.Partida()
        partidaObj.recuperaDeTexto(reg["PARTIDA"])
        nombreObj = reg.get("WHITE" if siBlancas else "BLACK", _("Player"))
        rotulo = db.rotulo(recno)
        db.close()

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

        self.siSave = False
        self.minTiempo = 5000

        self.xanalyzer.maximizaMultiPV()

        self.puntosMax = 0
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
        self.ponRotulo2("%s : <b>%d (%d)</b>" % (_("Points"), self.puntos, -self.puntosMax))

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.procesador.inicio()
            self.procesador.playPGNshow(self.recno)

        elif clave == k_cancelar:
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
        self.puntosMax = 0
        self.ponPuntos()
        self.tiempo = 0.0
        self.book = Apertura.AperturaPol(999)
        self.estado = kJugando
        self.tablero.ponPosicion(self.partida.iniPosicion)
        self.pgnRefresh(True)
        self.ponPosicionDGT()
        self.analizaFinal()

        self.siguienteJugada()

    def validoMRM(self, pvUsu, pvObj, mrmActual):
        jg = self.partidaObj.jugada(self.posJugadaObj)
        if jg.analisis:
            mrm, pos = jg.analisis
            msAnalisis = mrm.getTime()
            if msAnalisis > self.minTiempo:
                if mrmActual.getTime() > msAnalisis and mrmActual.contiene(pvUsu) and mrmActual.contiene(pvObj):
                    return None
                if mrm.contiene(pvObj) and mrm.contiene(pvUsu):
                    return mrm
        return None

    def analizaInicio(self):
        if not self.siTerminada():
            self.xanalyzer.ac_inicio(self.partida)
            self.siAnalizando = True

    def analizaMinimo(self, pvUsu, pvObj):
        mrmActual = self.xanalyzer.ac_estado()
        mrm = self.validoMRM(pvUsu, pvObj, mrmActual)
        if mrm:
            return mrm
        self.mrm = copy.deepcopy(self.xanalyzer.ac_minimo(self.minTiempo, False))
        return self.mrm

    def analizaEstado(self):
        self.xanalyzer.motor.ac_lee()
        self.mrm = copy.deepcopy(self.xanalyzer.ac_estado())
        return self.mrm

    def analizaFinal(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xanalyzer.ac_final(-1)
            self.siSave = True

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
            pvUsu = jgUsu.movimiento()
            pvObj = jgObj.movimiento()
            mrm = self.analizaMinimo(pvUsu, pvObj)
            posicion = self.partida.ultPosicion

            rmUsu, nada = mrm.buscaRM(pvUsu)
            rmObj, posObj = mrm.buscaRM(pvObj)

            analisis = mrm, posObj
            um.final()

            w = PantallaJuicio.WJuicio(self, self.xanalyzer, self.nombreObj, posicion, mrm, rmObj, rmUsu, analisis)
            w.exec_()

            analisis = w.analisis
            if w.siAnalisisCambiado:
                self.siSave = True
            dpts = w.difPuntos()
            self.puntos += dpts

            dptsMax = w.difPuntosMax()
            self.puntosMax += dptsMax

            comentarioUsu = " %s" % (rmUsu.abrTexto())
            comentarioObj = " %s" % (rmObj.abrTexto())

            comentarioPuntos = "%s = %d %+d %+d = %d" % (_("Points"), self.puntos - dpts, rmUsu.puntosABS(),
                                                         -rmObj.puntosABS(), self.puntos)
            comentario = "%s: %s %s\n%s: %s %s\n%s" % (self.nombreObj, jgObj.pgnSP(), comentarioObj,
                                                       self.configuracion.jugador, jgUsu.pgnSP(), comentarioUsu,
                                                       comentarioPuntos)
            self.ponPuntos()

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

        if self.puntos < 0:
            mensaje = _("Unfortunately you have lost.")
            quien = kGanaRival
        else:
            mensaje = _("Congratulations you have won.")
            quien = kGanamos

        self.beepResultado(quien)

        self.mensajeEnPGN(mensaje)
        self.ponFinJuego()
        self.guardar()

    def guardar(self):
        db = PantallaPlayPGN.PlayPGNs(self.configuracion.ficheroPlayPGN)
        reg = db.leeRegistro(self.recno)

        dicIntento = {
            "DATE": Util.hoy(),
            "COLOR": "w" if self.siJugamosConBlancas else "b",
            "POINTS": self.puntos,
            "POINTSMAX": self.puntosMax,
            "TIME": self.tiempo
        }

        if "LIINTENTOS" not in reg:
            reg["LIINTENTOS"] = []
        reg["LIINTENTOS"].insert(0, dicIntento)

        if self.siSave:
            reg["PARTIDA"] = self.partidaObj.guardaEnTexto()
            self.siSave = False

        db.cambiaRegistro(self.recno, reg)

        db.close()
