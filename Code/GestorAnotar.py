import time

from Code import Util
from Code import Gestor
from Code import Partida
from Code.QT import QTVarios
from Code.Constantes import *


class GestorAnotar(Gestor.Gestor):
    def inicio(self, partidaObjetivo, siBlancaAbajo):

        self.partida = Partida.Partida()
        self.tipoJuego = kJugAnotar
        self.partidaObjetivo = partidaObjetivo
        self.jugadaActual = -1
        self.totalJugadas = len(self.partidaObjetivo)
        self.tablero.showCoordenadas(False)

        self.ayudasRecibidas = 0
        self.errores = 0
        self.cancelado = False

        self.pantalla.ponActivarTutor(False)
        self.siBlancasAbajo = siBlancaAbajo
        self.ponPiezasAbajo(self.siBlancasAbajo)
        self.mostrarIndicador(True)
        self.siTerminar = False
        self.pantalla.ponToolBar((k_mainmenu,))
        self.pantalla.habilitaToolbar(k_mainmenu, False)
        self.informacionActivable = False
        self.pantalla.activaInformacionPGN(False)
        self.pantalla.activaJuego(False, False, siAyudas=False)
        self.ponActivarTutor(False)
        self.quitaAyudas()
        self.ponVista()
        self.ponRotulo1("")
        self.ponRotulo2("")

        self.estado = kJugando

        self.desactivaTodas()

        self.tiempo = 0.0

        self.siguienteJugada()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return False

        self.estado = kJugando

        self.jugadaActual += 1
        if self.jugadaActual >= self.totalJugadas:
            self.finalizar()
            return False

        self.ponPiezasAbajo(self.siBlancasAbajo)

        self.ponPosicion(self.partida.ultPosicion)

        siBlancas = self.partida.siBlancas()
        self.colorJugando = siBlancas

        self.ponIndicador(siBlancas)
        jg = self.partidaObjetivo.jugada(self.jugadaActual)
        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()
        self.movimientosPiezas(jg.liMovs, True)
        self.tablero.ponFlechaSC(jg.desde, jg.hasta)

        tm = time.time()

        w = QTVarios.ReadAnnotation(self.pantalla, jg.pgnSP())
        if not w.exec_():
            self.cancelado = True
            self.finalizar()
            return False

        self.tiempo += time.time() - tm
        conAyuda, errores = w.resultado
        if conAyuda:
            self.ayudasRecibidas += 1
        self.errores += errores

        self.refresh()

        return self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_reiniciar:
            self.inicio(self.partidaObjetivo, self.siBlancasAbajo)

        elif clave in (k_cancelar, k_mainmenu):
            self.tablero.showCoordenadas(True)
            self.procesador.inicio()
            self.procesador.show_anotar()

        elif clave == k_configurar:
            self.configurar()

        elif clave == k_utilidades:
            self.utilidades()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        self.tablero.showCoordenadas(True)
        return True

    def finalizar(self):
        self.informacionActivable = True
        self.tablero.showCoordenadas(True)
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas()
        self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_configurar, k_utilidades))
        if self.cancelado:
            self.partida = self.partidaObjetivo
        self.ponteAlFinal()
        blancas, negras, fecha, event, result = "", "", "", "", ""
        for key, value in self.partidaObjetivo.liTags:
            key = key.upper()
            if key == "WHITE":
                blancas = value
            elif key == "BLACK":
                negras = value
            elif key == "DATE":
                fecha = value
            elif key == "EVENT":
                event = value
            elif key == "RESULT":
                result = value

        self.ponRotulo1("%s - %s<br> %s: <b>%s</b><br>%s: <b>%s</b><br>%s: <b>%s</b>" % (
            fecha, event,
            _("White"), blancas,
            _("Black"), negras,
            _("Result"), result))
        numjug = self.jugadaActual
        if numjug > 0:
            self.ponRotulo2("%s: <b>%d</b><br>%s: %0.2f\"<br>%s: <b>%d</b><br>%s: <b>%d</b>" % (
                        _("Moves"), numjug,
                        _("Average time"), self.tiempo/numjug,
                        _("Errors"), self.errores,
                        _("Hints"), self.ayudasRecibidas
            ))
            if numjug > 2:
                db = Util.DicSQL(self.configuracion.ficheroAnotar)
                f = Util.hoy()
                key = "%04d-%02d-%02d %02d:%02d:%02d" % (f.year, f.month, f.day, f.hour, f.minute, f.second)
                db[key] = {
                    "PC": self.partidaObjetivo,
                    "MOVES": numjug,
                    "TIME": self.tiempo/numjug,
                    "HINTS": self.ayudasRecibidas,
                    "ERRORS": self.errores,
                    "COLOR": self.siBlancasAbajo,
                }
                db.close()

    def actualPGN(self):
        return self.partidaObjetivo.pgn()

