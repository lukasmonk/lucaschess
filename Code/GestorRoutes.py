import random
import time

import LCEngine4 as LCEngine

from Code import Util
from Code import Books
from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code import LibChess
from Code import PGN
from Code import Partida
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code import Routes
from Code import VarGen
from Code.Constantes import *


class GR_Engine:
    def __init__(self, procesador, nlevel):
        self._label = "%s - %s %d" % (_("Engine"), _("Level"), nlevel)
        self.configuracion = procesador.configuracion
        self.level = nlevel
        if nlevel == 0:
            self.gestor = None
            self._nombre = self._label
        else:
            dEngines = self.elos()
            x = +1 if nlevel < 6 else -1
            while True:
                if len(dEngines[nlevel]) > 0:
                    nom_engine, depth, elo = random.choice(dEngines[nlevel])
                    break
                else:
                    nlevel += x
                    if nlevel > 6:
                        nlevel = 1
            rival = self.configuracion.buscaRival(nom_engine)
            self.gestor = procesador.creaGestorMotor(rival, None, depth)
            self._nombre = "%s %s %d" % (rival.nombre, _("Depth"), depth)
            self._label += "\n%s\n%s: %d" % (self._nombre, _("Estimated elo"), elo)

    def close(self):
        if self.gestor and self.gestor != self:
            self.gestor.terminar()
            self.gestor = None

    @property
    def label(self):
        return self._label

    @property
    def nombre(self):
        return self._nombre

    def play(self, fen):
        if self.gestor:
            mrm = self.gestor.analiza(fen)
            return mrm.rmBest().movimiento()
        else:
            return LCEngine.runFen(fen, 1, 0, 2)

    def elos(self):
        x = """stockfish 1284 1377 1377 1496
alaric 1154 1381 1813 2117
amyan 1096 1334 1502 1678
bikjump 1123 1218 1489 1572
cheng 1137 1360 1662 1714
chispa 1109 1180 1407 1711
clarabit 1119 1143 1172 1414
critter 1194 1614 1814 1897
discocheck 1138 1380 1608 1812
fruit 1373 1391 1869 1932
gaia 1096 1115 1350 1611
cyrano 1154 1391 1879 2123
garbochess 1146 1382 1655 1892
gaviota 1152 1396 1564 1879
greko 1158 1218 1390 1742
hamsters 1143 1382 1649 1899
komodo 1204 1406 1674 1891
lime 1143 1206 1493 1721
pawny 1096 1121 1333 1508
rhetoric 1131 1360 1604 1820
roce 1122 1150 1206 1497
rodent 1103 1140 1375 1712
umko 1120 1384 1816 1930
rybka 1881 2060 2141 2284
simplex 1118 1166 1411 1814
ufim 1189 1383 1928 2134
texel 1154 1387 1653 1874
toga 1236 1495 1928 2132"""
        d = {1: [], 2: [], 3: [], 4: [], 5: [], 6: []}

        def mas(engine, depth, celo):
            elo = int(celo)
            if elo < 1100:
                tp = 1
            elif elo < 1200:
                tp = 2
            elif elo < 1400:
                tp = 3
            elif elo < 1700:
                tp = 4
            elif elo < 2000:
                tp = 5
            elif elo < 2200:
                tp = 6
            else:
                return
            if engine in self.configuracion.dicRivales:
                d[tp].append((engine, depth, elo))

        for line in x.split("\n"):
            engine, d1, d2, d3, d4 = line.split(" ")
            mas(engine, 1, d1)
            mas(engine, 2, d2)
            mas(engine, 3, d3)
            mas(engine, 4, d4)
        return d


class GestorRoutes(Gestor.Gestor):
    def inicio(self, route):
        self.route = route
        if not hasattr(self, "time_start"):
            self.time_start = time.time()
        self.state = route.state

        self.tipoJuego = kJugRoute

    def finPartida(self):
        self.procesador.inicio()
        self.route.add_time(time.time() - self.time_start, self.state)

    def finalX(self):
        self.finPartida()
        return False

    def masJugada(self, jg, siNuestra):
        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        resp = self.partida.si3repetidas()
        if resp:
            jg.siTablasRepeticion = True

        if self.partida.ultPosicion.movPeonCap >= 100:
            jg.siTablas50 = True

        if self.partida.ultPosicion.siFaltaMaterial():
            jg.siTablasFaltaMaterial = True

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()


class GestorRoutesPlay(GestorRoutes):
    def inicio(self, route):
        GestorRoutes.inicio(self, route)

        line = route.get_line()

        opening = line.opening
        siBlancas = opening.is_white if opening.is_white is not None else random.choice([True,False])
        self.liPVopening = opening.pv.split(" ")
        self.posOpening = 0
        self.is_opening = len(opening.pv) > 0
        self.book = Books.Libro("P", VarGen.tbookI, VarGen.tbookI, True)
        self.book.polyglot()

        self.engine = GR_Engine(self.procesador, line.engine)
        self.must_win = route.must_win()
        self.rivalPensando = False

        self.siJuegaHumano = False
        self.estado = kJugando

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.pantalla.ponActivarTutor(False)

        self.ayudasPGN = 0

        liOpciones = [k_mainmenu, k_configurar, k_reiniciar]
        self.pantalla.ponToolBar(liOpciones)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True)

        self.ponRotulo1(self.engine.label)
        if self.must_win:
            self.ponRotulo2(_("You must win to pass this game"))

        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)

        self.pgnRefresh(True)
        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        self.siguienteJugada()

    def finPartida(self):
        self.engine.close()
        GestorRoutes.finPartida(self)

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()
            self.procesador.showRoute()

        elif clave == k_reiniciar:
            self.partida.reset()
            self.inicio(self.route)

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        if self.partida.siTerminada():
            self.lineaTerminada()
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas
        if siRival:
            self.juegaRival()
        else:
            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def juegaRival(self):
        if self.is_opening:
            pv = self.liPVopening[self.posOpening]
            self.posOpening += 1
            if self.posOpening == len(self.liPVopening):
                self.is_opening = False
        else:
            fen = self.partida.ultPosicion.fen()
            pv = None
            if self.book:
                pv = self.book.eligeJugadaTipo(fen, "au")
                if not pv:
                    self.book = None
            if not pv:
                if self.partida.ultPosicion.totalPiezas() <= 4:
                    t4 = LibChess.T4()
                    pv = t4.best_move(fen)
                    t4.close()
                if not pv:
                    pv = self.engine.play(fen)

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, pv[:2], pv[2:4], pv[4:])
        self.masJugada(jg, False)
        self.movimientosPiezas(jg.liMovs, True)
        self.siguienteJugada()

    def mueveHumano(self, desde, hasta, coronacion=None):
        jgSel = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgSel:
            return False

        fen = self.partida.ultPosicion.fen()
        pv = jgSel.movimiento().lower()
        if self.is_opening:
            op_pv = self.liPVopening[self.posOpening]
            if pv != op_pv:
                if self.must_win:
                    QTUtil2.mensajeTemporal(self.pantalla, _("Wrong move"), 2)
                    self.procesarAccion(k_reiniciar)
                else:
                    QTUtil2.mensError(self.pantalla, "%s\n%s" % (_("Wrong move"), _("Right move: %s") % Partida.pv_san(fen, op_pv)))
                    self.sigueHumano()
                return False
            self.posOpening += 1
            if self.posOpening == len(self.liPVopening):
                self.is_opening = False

        self.movimientosPiezas(jgSel.liMovs)

        self.partida.ultPosicion = jgSel.posicion
        self.masJugada(jgSel, True)
        self.error = ""

        self.siguienteJugada()
        return True

    def lineaTerminada(self):
        self.desactivaTodas()
        self.siJuegaHumano = False
        self.estado = kFinJuego
        self.refresh()
        liOpciones = [k_mainmenu, k_utilidades]
        self.pantalla.ponToolBar(liOpciones)
        jgUlt = self.partida.last_jg()

        siwin = (jgUlt.siBlancas() == self.siJugamosConBlancas) and not jgUlt.siTablas()

        self.guardarGanados(siwin)

        if siwin:
            if self.route.end_playing():
                mensaje = _("Congratulations, you have completed the game.")
            else:
                mensaje = _("Well done")
            self.mensajeEnPGN(mensaje)
        else:
            if self.must_win:
                QTUtil2.mensError(self.pantalla, _("You must win to pass this step."))
                liOpciones = [k_mainmenu, k_configurar, k_utilidades, k_reiniciar]
                self.pantalla.ponToolBar(liOpciones)
            else:
                self.route.end_playing()

    def actualPGN(self):
        resp = '[Event "%s"]\n' % _("Transsiberian Railway")
        hoy = Util.hoy()
        resp += '[Date "%d-%d-%d"]\n' % (hoy.year, hoy.month, hoy.day)

        lbe = self.engine.nombre

        white, black = self.configuracion.jugador, lbe
        if not self.siJugamosConBlancas:
            white, black = black, white

        resp += '[White "%s"]\n' % white
        resp += '[Black "%s"]\n' % black

        last_jg = self.partida.last_jg()
        result = last_jg.resultado()
        if last_jg.siTablas():
            result = "1/2-1/2"
        else:
            result = "1-0" if last_jg.siBlancas() else "0-1"

        resp += '[Result "%s"]\n' % result

        ap = self.partida.apertura
        resp += '[ECO "%s"]\n' % ap.eco
        resp += '[Opening "%s"]\n' % ap.trNombre

        resp += "\n" + self.partida.pgnBase() + " " + result

        return resp


class GestorRoutesEndings(GestorRoutes):
    def inicio(self, route):
        GestorRoutes.inicio(self, route)

        ending = self.route.get_ending()
        if "|" in ending:
            self.is_guided = True
            self.t4 = None
            self.fen, label, pv = ending.split("|")
            self.liPV = pv.split(" ")
            self.posPV = 0
        else:
            self.is_guided = False
            self.t4 = LibChess.T4()
            self.fen = ending + " - - 0 1"

        self.rivalPensando = False

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.fen)

        siBlancas = cp.siBlancas

        self.partida.reset(cp)
        self.partida.pendienteApertura = False

        self.warnings = 0
        self.max_warnings = 5

        self.siJuegaHumano = False
        self.estado = kJugando

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.pantalla.ponActivarTutor(False)
        self.quitaAyudas(True)

        self.ayudasPGN = 0

        liOpciones = [k_mainmenu, k_ayuda]
        self.pantalla.ponToolBar(liOpciones)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)

        self.ponWarnings()

        self.pgnRefresh(True)
        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        if self.is_guided:
            self.ponRotulo1("<b>%s</b>" % label)

        self.siguienteJugada()

    def ponWarnings(self):
        if self.warnings <= self.max_warnings:
            self.ponRotulo2(_("Warnings: %d/%d" % (self.warnings, self.max_warnings)))
        else:
            self.ponRotulo2(_("You must repeat the puzzle."))

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()
            self.procesador.showRoute()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=True)

        elif clave == k_ayuda:
            self.ayuda()

        elif clave == k_siguiente:
            if self.route.km_pending():
                self.inicio(self.route)
            else:
                self.finPartida()
                self.procesador.showRoute()

        elif clave == k_utilidades:
            self.utilidades()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finPartida(self):
        if self.t4:
            self.t4.close()
        GestorRoutes.finPartida(self)

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        if self.partida.siTerminada():
            self.lineaTerminada()
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas
        if siRival:
            if self.is_guided:
                pv = self.liPV[self.posPV].split("-")[0]
                self.posPV += 1
            else:
                fen = self.partida.ultPosicion.fen()
                pv = self.t4.best_move(fen)
            self.mueveRival(pv[:2], pv[2:4], pv[4:])
            self.siguienteJugada()
        else:
            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def show_error(self, mens):
        QTUtil2.mensajeTemporal(self.pantalla, mens, 4, background="#FF9B00", posicion="tb")

    def show_mens(self, mens):
        QTUtil2.mensajeTemporal(self.pantalla, mens, 4, posicion="tb", background="#C3D6E8")

    def mueveHumano(self, desde, hasta, coronacion=None):
        jgSel = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgSel:
            return False

        if self.is_guided:
            pvSel = jgSel.movimiento().lower()
            pvObj = self.liPV[self.posPV]
            li = pvObj.split("-")
            if li[0] != pvSel:
                if pvSel in li:
                    pgn = Partida.pv_pgn(jgSel.posicionBase.fen(), pvObj)
                    self.show_mens(_("You have selected one correct move, but the line use %s") % pgn)
                    self.ponFlechaSC(pvObj[:2], pvObj[2:4])
                    self.ayuda(False)
                else:
                    self.show_error(_("Wrong move"))
                    self.warnings += 1
                    self.ponWarnings()
                self.sigueHumano()
                return False
            self.posPV += 1
        else:
            fen = self.partida.ultPosicion.fen()
            pv = jgSel.movimiento().lower()
            b_wdl, b_dtz = self.t4.wdl_dtz(fen)
            m_wdl, m_dtz = self.t4.wd_move(fen, pv)
            if b_wdl != m_wdl:
                self.show_error(_("Wrong move"))
                self.warnings += 1
                self.ponWarnings()
                self.ponPosicion(self.partida.ultPosicion)
                self.sigueHumano()
                return False
            if b_wdl == 2:
                numjug = self.partida.ultPosicion.jugadas
                if b_dtz % 2 == 1:
                    b_dtz += 1
                numjug += b_dtz / 2
                if numjug >= 50:
                    mens = "%s\n%s" % (_("Wrong move"), _("Draw according to the 50 move rule"))
                    self.show_error(mens)
                    QTUtil2.mensajeTemporal(self.pantalla,mens, 2)
                    self.warnings += 1
                    self.ponWarnings()
                    self.ponPosicion(self.partida.ultPosicion)
                    self.sigueHumano()
                    return False

        self.movimientosPiezas(jgSel.liMovs)

        self.masJugada(jgSel, True)
        self.error = ""

        self.siguienteJugada()
        return True

    def mueveRival(self, desde, hasta, coronacion):
        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        self.masJugada(jg, False)
        self.movimientosPiezas(jg.liMovs, True)
        return True

    def ayuda(self, siWarning = True):
        pvObj = self.liPV[self.posPV]
        li = pvObj.split("-")
        liMovs = [(pv[:2], pv[2:4], n == 0) for n, pv in enumerate(li)]
        self.tablero.ponFlechasTmp(liMovs)
        if siWarning:
            self.warnings += self.max_warnings
            self.ponWarnings()

    def lineaTerminada(self):
        self.desactivaTodas()
        self.siJuegaHumano = False
        self.estado = kFinJuego
        self.refresh()

        jgUlt = self.partida.last_jg()
        if jgUlt.siTablas():
            mensaje = "%s<br>%s" % (_("Draw"), _("You must repeat the puzzle."))
            self.mensajeEnPGN(mensaje)
            self.inicio(self.route)
        elif self.warnings <= self.max_warnings:
            self.pantalla.ponToolBar([k_mainmenu, k_utilidades])
            self.mensajeEnPGN(_("Done"))
            self.route.end_ending()
        else:
            QTUtil2.mensaje(self.pantalla, )
            mensaje = "%s<br>%s" % (_("Done with errors."), _("You must repeat the puzzle."))
            self.mensajeEnPGN(mensaje)
            self.inicio(self.route)

    def actualPGN(self):
        resp = '[Event "%s"]\n' % _("Transsiberian Railway")
        resp += '[FEN "%s"\n' % self.partida.iniPosicion.fen()

        resp += "\n" + self.partida.pgnBase()

        return resp


class GestorRoutesTactics(GestorRoutes):
    def inicio(self, route):
        GestorRoutes.inicio(self, route)

        tactica = self.route.get_tactic()

        self.partida_objetivo = PGN.pgn_partida(tactica.fen, tactica.pgn)

        self.rivalPensando = False

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(tactica.fen)

        self.fen = tactica.fen

        siBlancas = cp.siBlancas

        self.partida.reset(cp)
        self.partida.pendienteApertura = False

        self.siJuegaHumano = False
        self.estado = kJugando

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.pantalla.ponActivarTutor(False)

        self.ayudasPGN = 0

        liOpciones = [k_mainmenu, k_ayuda]
        self.pantalla.ponToolBar(liOpciones)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.pantalla.quitaAyudas(True)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(siBlancas)
        # self.ponRotulo1("<b>%s</b>" % tactica.label)
        self.ponRotulo2(route.mens_tactic(False))
        self.pgnRefresh(True)
        QTUtil.xrefreshGUI()

        self.ponPosicionDGT()

        self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()
            self.procesador.showRoute()

        elif clave == k_configurar:
            self.configurar(siSonidos=True, siCambioTutor=True)

        elif clave == k_ayuda:
            self.ayuda()

        elif clave == k_siguiente:
            if self.route.km_pending():
                self.inicio(self.route)
            else:
                self.finPartida()
                self.procesador.showRoute()

        elif clave == k_utilidades:
            self.utilidades()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def jugadaObjetivo(self):
        return self.partida_objetivo.jugada(self.partida.numJugadas())

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        if self.partida.numJugadas() == self.partida_objetivo.numJugadas():
            self.lineaTerminada()
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas
        if siRival:
            jg = self.jugadaObjetivo()
            self.mueveRival(jg.desde, jg.hasta, jg.coronacion)
            self.siguienteJugada()
        else:
            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jgSel = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgSel:
            return False

        jgObj = self.jugadaObjetivo()
        if jgObj.movimiento() != jgSel.movimiento():
            for pvar in jgObj.pvariantes:
                jgObjV = pvar.jugada(0)
                if jgObjV.movimiento() == jgSel.movimiento():

                    QTUtil2.mensajeTemporal(self.pantalla, _("You have selected one correct move, but the line use %s") % jgObj.pgnSP(), 3, posicion="ad")
                    self.ayuda(False)
                    self.sigueHumano()
                    return False
            QTUtil2.mensajeTemporal(self.pantalla, _("Wrong move"), 0.8, posicion="ad")
            self.route.error_tactic(self.partida_objetivo.numJugadas())
            self.ponRotulo2(self.route.mens_tactic(False))
            self.sigueHumano()
            return False

        self.movimientosPiezas(jgSel.liMovs)

        self.partida.ultPosicion = jgSel.posicion
        self.masJugada(jgSel, True)
        self.error = ""

        self.siguienteJugada()
        return True

    def mueveRival(self, desde, hasta, coronacion):
        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        self.masJugada(jg, False)
        self.movimientosPiezas(jg.liMovs, True)
        return True

    def ayuda(self, siQuitarPuntos=True):
        jgObj = self.jugadaObjetivo()
        liMovs = [(jgObj.desde, jgObj.hasta, True)]
        for pvar in jgObj.pvariantes:
            jg0 = pvar.jugada(0)
            liMovs.append((jg0.desde, jg0.hasta, False))
        self.tablero.ponFlechasTmp(liMovs)
        if siQuitarPuntos:
            self.route.error_tactic(self.partida_objetivo.numJugadas())
            self.ponRotulo2(self.route.mens_tactic(False))

    def lineaTerminada(self):
        self.desactivaTodas()
        self.refresh()
        km = self.route.end_tactic()
        if not self.route.go_fast:
            mensaje = "%s<br>%s" % (_("Done"), _("You have traveled %s") % Routes.km_mi(km, self.route.is_miles))
            self.mensajeEnPGN(mensaje)
        self.siJuegaHumano = False
        self.estado = kFinJuego
        if self.route.go_fast:
            self.procesarAccion(k_siguiente)
        else:
            liOpciones = [k_mainmenu, k_utilidades, k_siguiente]
            self.pantalla.ponToolBar(liOpciones)
            self.ponRotulo2(self.route.mens_tactic(True))

    def actualPGN(self):
        resp = '[Event "%s"]\n' % _("Transsiberian Railway")
        resp += '[FEN "%s"\n' % self.partida.iniPosicion.fen()

        resp += "\n" + self.partida.pgnBase()

        return resp
