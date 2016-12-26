import time

from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code import TurnOnLights
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.Constantes import *

class GestorTurnOnLights(Gestor.Gestor):
    def inicio(self, num_theme, num_block, tol):

        if hasattr(self, "reiniciando"):
            if self.reiniciando:
                return
        self.reiniciando = True

        self.num_theme = num_theme
        self.num_block = num_block
        self.tol = tol
        self.block = self.tol.get_block(self.num_theme, self.num_block)
        self.block.shuffle()

        self.av_seconds = self.block.av_seconds()
        if self.av_seconds:
            cat, ico = self.block.cqualification()
            self.lb_previous = "%s - %0.2f\"" % (cat, self.av_seconds)
        else:
            self.lb_previous = None
        self.num_line = 0
        self.num_lines = len(self.block)

        self.total_time_used = 0.0
        self.ayudas = 0
        self.errores = 0

        self.tipoJuego = kJugEntLight

        self.siJuegaHumano = False

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(False)
        self.ayudasPGN = 0

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True, True)
        self.ponMensajero(self.mueveHumano)
        self.mostrarIndicador(True)

        self.reiniciando = False

        self.next_line_run()

    def next_line(self):
        if self.num_line < self.num_lines:
            self.line = self.block.line(self.num_line)
            self.num_move = -1
            self.time_used = 0.0

            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.line.fen)
            self.partida.reset(cp)

            siBlancas = cp.siBlancas
            self.siJugamosConBlancas = siBlancas
            self.siRivalConBlancas = not siBlancas
            self.ponPosicion(self.partida.ultPosicion)
            self.ponPiezasAbajo(siBlancas)
            r1 = self.line.label
            if self.lb_previous:
                r1 += "<br><b>%s</b>" % self.lb_previous
            self.ponRotulo1(r1)
            r2 = "<b>%d/%d</b>" % (self.num_line+1, self.num_lines)
            self.ponRotulo2(r2)
            self.pgnRefresh(True)

            self.partida.pendienteApertura = False

    def next_line_run(self):

        liOpciones = [k_mainmenu, k_ayuda, k_reiniciar]
        self.pantalla.ponToolBar(liOpciones)

        self.next_line()

        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        self.estado = kJugando

        self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_ayuda:
            self.ayuda()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=False)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_siguiente:
            self.next_line_run()

    def reiniciar(self):
        self.inicio(self.num_theme, self.num_block, self.tol)

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        self.num_move += 1
        if self.num_move >= self.line.total_moves():
            self.finLinea()
            return

        if siRival:
            pv = self.line.get_move(self.num_move)
            desde, hasta, coronacion = pv[:2], pv[2:4], pv[4:]
            self.mueveRival(desde, hasta, coronacion)
            self.siguienteJugada()

        else:
            self.siJuegaHumano = True
            self.ini_time = time.time()
            self.activaColor(siBlancas)

    def finLinea(self):
        self.num_line += 1
        islast_line = self.num_line == self.num_lines
        if islast_line:

            #Previous
            ant_tm = self.block.av_seconds()
            ant_done = self.tol.done_level()
            ant_cat_level, nada = self.tol.cat_num_level()
            ant_cat_global = self.tol.cat_global()

            num_moves = self.block.num_moves()
            ta = self.total_time_used + self.errores*5.0 + self.ayudas*10.0
            tm = ta/num_moves
            self.block.new_result(tm)
            TurnOnLights.write_tol(self.tol)
            cat_block, ico = TurnOnLights.qualification(tm)
            cat_level, ico = self.tol.cat_num_level()
            cat_global = self.tol.cat_global()

            txt_more_time = ""
            txt_more_cat = ""
            txt_more_line = ""
            txt_more_global = ""
            if ant_tm is None or tm < ant_tm:
                txt_more_time = '<span style="color:red">%s</span>' % _("New record")
                done = self.tol.done_level()
                if done and (not ant_done):
                    if not self.tol.islast_level():
                        txt_more_line = "%s<hr>" % _("Open the next level")
                if cat_level != ant_cat_level:
                    txt_more_cat = '<span style="color:red">%s</span>' % _("New")
                if cat_global != ant_cat_global:
                    txt_more_global = '<span style="color:red">%s</span>' % _("New")

            cErrores = '<tr><td align=right> %s </td><td> %d (x5"=%d")</td></tr>' % (_('Errors'), self.errores, self.errores*5) if self.errores else ""
            cAyudas = '<tr><td align=right> %s </td><td> %d (x10"=%d")</td></tr>' % (_('Hints'), self.ayudas, self.ayudas*10) if self.ayudas else ""
            mens = ('<hr><center><big>'+_('You have finished this block of positions') +
                    '<hr><table>' +
                    '<tr><td align=right> %s </td><td> %0.2f"</td></tr>' % (_('Time used'), self.total_time_used) +
                     cErrores +
                     cAyudas +
                    '<tr><td align=right> %s: </td><td> %0.2f" %s</td></tr>' % (_('Time assigned'), ta, txt_more_time) +
                    '<tr><td align=right> %s: </td><td> %d</td></tr>' % (_('Total moves'), num_moves) +
                    '<tr><td align=right> %s: </td><td> %0.2f"</td></tr>' % (_('Average time'), tm) +
                    '<tr><td align=right> %s: </td><td> %s</td></tr>' % (_('Block qualification'), cat_block) +
                    '<tr><td align=right> %s: </td><td> %s %s</td></tr>' % (_('Level qualification'), cat_level, txt_more_cat) +
                    '<tr><td align=right> %s: </td><td> %s %s</td></tr>' % (_('Global qualification'), cat_global, txt_more_global) +
                    '</table></center></big><hr>' +
                    txt_more_line
                    )
            QTUtil2.mensaje(self.pantalla, mens, _("Result of training"))

        else:
            if self.tol.go_fast:
                self.next_line_run()
                return
            QTUtil2.mensajeTemporal(self.pantalla, _("This line training is completed."), 1.3)

        self.estado = kFinJuego
        self.desactivaTodas()

        liOpciones = [k_mainmenu, k_reiniciar, k_configurar, k_utilidades]

        if not islast_line:
            liOpciones.append(k_siguiente)

        self.pantalla.ponToolBar(liOpciones)

    def mueveHumano(self, desde, hasta, coronacion=None):
        end_time = time.time()
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            self.errores += 1
            return False

        movimiento = jg.movimiento().lower()
        if movimiento == self.line.get_move(self.num_move).lower():
            self.movimientosPiezas(jg.liMovs)
            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, True)
            self.error = ""
            self.total_time_used += (end_time - self.ini_time)
            self.siguienteJugada()
            return True

        self.errores += 1
        self.sigueHumano()
        return False

    def masJugada(self, jg, siNuestra):
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
        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, False)
        self.movimientosPiezas(jg.liMovs, True)
        self.error = ""

    def ayuda(self):
        self.ayudas += 1
        mov = self.line.get_move(self.num_move).lower()
        self.tablero.markPosition(mov[:2])

    def finPartida(self):
        self.procesador.inicio()
        self.procesador.showTurnOnLigths(self.tol.name)

    def finalX(self):
        self.procesador.inicio()
        return False

    def actualPGN(self):
        resp = '[Event "%s"]\n' % _("Turn on the lights")
        resp += '[Site "%s"]\n' % self.line.label.replace("<br>", " ").strip()
        resp += '[FEN "%s"\n' % self.partida.iniPosicion.fen()

        resp += "\n" + self.partida.pgnBase()

        return resp
