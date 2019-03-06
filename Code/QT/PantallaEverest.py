import os.path
import random

import LCEngine4 as LCEngine
from PyQt4 import QtSvg, QtCore

from Code import Everest
from Code import PGN
import Code.PGNreader as PGNReader
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
import Code.SQL.Base as SQLBase
from Code import Util


class WNewExpedition(QTVarios.WDialogo):
    def __init__(self, owner, fichero):
        self.litourneys = Everest.str_file(fichero)
        self.configuracion = owner.configuracion
        titulo = _("New expedition")
        icono = Iconos.Trekking()
        extparam = "newexpedition"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.selected = None

        # Torneo
        li = [("%s (%d)" % (_F(tourney["TOURNEY"]), len(tourney["GAMES"])), tourney) for tourney in self.litourneys]
        li.sort(key=lambda x:x[0])
        self.cbtourney, lbtourney = QTUtil2.comboBoxLB(self, li, li[0], _("Expedition"))
        btmas = Controles.PB(self, "", self.mas).ponIcono(Iconos.Mas22())
        lytourney = Colocacion.H().control(lbtourney).control(self.cbtourney).control(btmas).relleno(1)

        # tolerance
        self.sbtolerance_min, lbtolerance_min = QTUtil2.spinBoxLB(self, 20, 0, 99999, _("From"))
        self.sbtolerance_min.capturaCambiado(self.tolerance_changed)
        self.sbtolerance_max, lbtolerance_max = QTUtil2.spinBoxLB(self, 1000, 0, 99999, _("To"))
        lbexplanation = Controles.LB(self, _("Maximum lost points for having to repeat active game"))
        ly = Colocacion.H().relleno(2).control(lbtolerance_min).control(self.sbtolerance_min).relleno(1)
        ly.control(lbtolerance_max).control(self.sbtolerance_max).relleno(2)
        layout = Colocacion.V().otro(ly).control(lbexplanation)
        gbtolerance = Controles.GB(self, _("Tolerance"), layout)

        # tries
        self.sbtries_min, lbtries_min = QTUtil2.spinBoxLB(self, 2, 1, 99999, _("From"))
        self.sbtries_min.capturaCambiado(self.tries_changed)
        self.sbtries_max, lbtries_max = QTUtil2.spinBoxLB(self, 15, 1, 99999, _("To"))
        lbexplanation = Controles.LB(self, _("Maximum repetitions to return to the previous game"))
        ly = Colocacion.H().relleno(2).control(lbtries_min).control(self.sbtries_min).relleno(1)
        ly.control(lbtries_max).control(self.sbtries_max).relleno(2)
        layout = Colocacion.V().otro(ly).control(lbexplanation)
        gbtries = Controles.GB(self, _("Tries"), layout)

        # color
        liColors = ((_("Default"), "D"), (_("White"), "W"), (_("Black"), "B"))
        self.cbcolor = Controles.CB(self, liColors, "D")
        layout = Colocacion.H().relleno(1).control(self.cbcolor).relleno(1)
        gbcolor = Controles.GB(self, _("Color"), layout)

        tb = QTVarios.LCTB(self)
        tb.new(_("Accept"), Iconos.Aceptar(), self.aceptar)
        tb.new(_("Cancel"), Iconos.Cancelar(), self.cancelar)

        layout = Colocacion.V().control(tb).otro(lytourney).control(gbtolerance).control(gbtries).control(gbcolor)

        self.setLayout(layout)

    def aceptar(self):
        self.selected = alm = Util.Almacen()
        alm.tourney = self.cbtourney.valor()
        alm.tolerance_min = self.sbtolerance_min.valor()
        alm.tolerance_max = self.sbtolerance_max.valor()
        alm.tries_min = self.sbtries_min.valor()
        alm.tries_max = self.sbtries_max.valor()
        alm.color = self.cbcolor.valor()
        self.accept()

    def cancelar(self):
        self.reject()

    def tolerance_changed(self):
        tolerance_min = self.sbtolerance_min.valor()
        self.sbtolerance_max.setMinimum(tolerance_min)
        if self.sbtolerance_max.valor() < tolerance_min:
            self.sbtolerance_max.ponValor(tolerance_min)

    def tries_changed(self):
        tries_min = self.sbtries_min.valor()
        self.sbtries_max.setMinimum(tries_min)
        if self.sbtries_max.valor() < tries_min:
            self.sbtries_max.ponValor(tries_min)

    def mas(self):
        path = QTVarios.select_pgn(self)
        if not path:
            return

        fpgn = PGN.PGN(self.configuracion)

        dicDB = fpgn.leeFichero(self, path)
        if dicDB is None:
            return

        bd = SQLBase.DBBase(dicDB["PATHDB"])

        dClavesTam = dicDB["DCLAVES"]
        dbf = bd.dbf("GAMES", ",".join(dClavesTam.keys()))
        dbf.leer()

        nreccount = dbf.reccount()

        plant = ""
        shuffle = False
        reverse = False
        todos = range(1, nreccount + 1)
        li_regs = None
        while True:
            sep = FormLayout.separador
            liGen = []
            liGen.append((None, "%s: %d" % (_("Total games"), nreccount)))
            liGen.append(sep)
            config = FormLayout.Editbox(_("Select games") + "<br>" +
                                        _("By example:") + " -5,7-9,14,19-" + "<br>" +
                                        _("Empty means all games"),
                                        rx="[0-9,\-,\,]*")
            liGen.append((config, plant))

            liGen.append(sep)

            liGen.append((_("Shuffle") + ":", shuffle))

            liGen.append(sep)

            liGen.append((_("Reverse") + ":", reverse))

            liGen.append(sep)

            config = FormLayout.Spinbox(_("Max moves"), 0, 999, 50)
            liGen.append((config, 0))

            resultado = FormLayout.fedit(liGen, title=_("Select games"), parent=self, anchoMinimo=200,
                                         icon=Iconos.Opciones())
            if resultado:
                accion, liResp = resultado
                plant, shuffle, reverse, max_moves = liResp
                if plant:
                    ln = Util.ListaNumerosImpresion(plant)
                    li_regs = ln.selected(todos)
                else:
                    li_regs = todos
                nregs = len(li_regs)
                if 12 <= nregs <= 500:
                    break
                else:
                    QTUtil2.mensError(self, "%s (%d)" % (_("Number of games must be in range 12-500"), nregs))
                    li_regs = None
            else:
                break

        if li_regs:
            if shuffle:
                random.shuffle(li_regs)
            if reverse:
                li_regs.sort(reverse=True)
            li_regs = [x - 1 for x in li_regs]  # 0 init

            dic = {}
            dic["TOURNEY"] = os.path.basename(path)[:-4]
            games = dic["GAMES"] = []

            for recno in li_regs:
                pgn = dbf.leeOtroCampo(recno, "PGN")
                g = PGNReader.read1Game(pgn)
                pv = g.pv()
                if max_moves:
                    lipv = pv.strip().split(" ")
                    if len(lipv) > max_moves:
                        pv = " ".join(lipv[:max_moves])
                dt = {
                    "LABELS": [(k, v) for k, v in g.labels.iteritems()],
                    "XPV": LCEngine.pv2xpv(pv)
                }
                games.append(dt)

            self.litourneys.append(dic)

            li = [("%s (%d)" % (tourney["TOURNEY"], len(tourney["GAMES"])), tourney) for tourney in self.litourneys]
            self.cbtourney.rehacer(li, dic)

        dbf.cerrar()
        bd.cerrar()


class WExpedition(QTVarios.WDialogo):
    def __init__(self, wowner, configuracion, recno):
        expedition = Everest.Expedition(configuracion, recno)
        self.li_routes, self.current, svg, rotulo = expedition.gen_routes()

        titulo = _("Everest")
        icono = Iconos.Trekking()
        extparam = "expedition"
        QTVarios.WDialogo.__init__(self, wowner, titulo, icono, extparam)

        self.selected = False

        wsvg = QtSvg.QSvgWidget()
        wsvg.load(QtCore.QByteArray(svg))
        wsvg.setFixedSize(762, 762.0 * 520.0 / 1172.0)
        lySVG = Colocacion.H().relleno(1).control(wsvg).relleno(1)

        liAcciones = (
            (_("Climb"), Iconos.Empezar(), self.climb), None,
            (_("Close"), Iconos.MainMenu(), self.cancel), None,
        )
        tb = Controles.TBrutina(self, liAcciones).vertical()
        if self.current is None:
            tb.setAccionVisible(self.climb, False)

        lyRot = Colocacion.H()
        for elem in rotulo:
            lb_rotulo = Controles.LB(self, elem).alinCentrado()
            lb_rotulo.setStyleSheet("QWidget { border-style: groove; border-width: 2px; border-color: LightSlateGray ;}")
            lb_rotulo.ponTipoLetra(puntos=12, peso=700)
            lyRot.control(lb_rotulo)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ROUTE", _("Route"), 240, siCentrado=True)
        oColumnas.nueva("GAMES", _("Games"), 80, siCentrado=True)
        oColumnas.nueva("DONE", _("Done"), 80, siCentrado=True)
        oColumnas.nueva("TIME", _("Time"), 80, siCentrado=True)
        oColumnas.nueva("MTIME", _("Average time"), 80, siCentrado=True)
        oColumnas.nueva("MPOINTS", _("Av. lost points"), 80, siCentrado=True)
        oColumnas.nueva("TRIES", _("Max tries"), 80, siCentrado=True)
        oColumnas.nueva("TOLERANCE", _("Tolerance"), 80, siCentrado=True)
        grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=False)
        grid.setMinimumWidth(grid.anchoColumnas() + 20)
        grid.coloresAlternados()

        lyG = Colocacion.V().otro(lyRot).control(grid).margen(0)

        lyR = Colocacion.H().control(tb).otro(lyG).margen(0)

        ly = Colocacion.V().otro(lySVG).otro(lyR).margen(3)

        self.setLayout(ly)

        self.recuperarVideo(siTam=True, anchoDefecto=784, altoDefecto=670)

    def gridNumDatos(self, grid):
        return 12

    def gridDato(self, grid, fila, oColumna):
        return self.li_routes[fila][oColumna.clave]

    def gridBold(self, grid, fila, oColumna):
        return self.current is not None and fila == self.current

    def gridDobleClick(self, grid, fila, oColumna):
        if self.current is not None and fila == self.current:
            self.climb()

    def gen_routes(self, ev, li_distribution, done_game, tries, tolerances, times):
        li_p = ev.li_points
        li_routes = []
        xgame = done_game + 1
        xcurrent = None
        for x in range(12):
            d = {}
            d["ROUTE"] = "%s - %s" % (li_p[x][4], li_p[x + 1][4])
            xc = li_distribution[x]
            d["GAMES"] = str(xc)
            done = xgame if xc >= xgame else xc
            xgame -= xc
            if xcurrent is None and xgame < 0:
                xcurrent = x

            d["DONE"] = str(done if done > 0 else "0")
            d["TRIES"] = str(tries[x])
            d["TOLERANCE"] = str(tolerances[x])
            seconds = times[x]
            d["TIME"] = "%d' %d\"" % (seconds / 60, seconds % 60)
            mseconds = seconds / done if done > 0 else 0
            d["MTIME"] = "%d' %d\"" % (mseconds / 60, mseconds % 60)
            li_routes.append(d)

        return li_routes, xcurrent

    def climb(self):
        self.guardarVideo()
        self.selected = True
        self.accept()

    def cancel(self):
        self.guardarVideo()
        self.reject()


class WEverest(QTVarios.WDialogo):
    def __init__(self, procesador):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, _("Expeditions to the Everest"), Iconos.Trekking(),
                                   "everestBase")

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.db = Everest.Expeditions(self.configuracion)

        self.selected = None

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NAME", _("Expedition"), 120, siCentrado=True)
        oColumnas.nueva("DATE_INIT", _("Start date"), 120, siCentrado=True)
        oColumnas.nueva("DATE_END", _("Final date"), 100, siCentrado=True)
        oColumnas.nueva("NUM_GAMES", _("Games"), 80, siCentrado=True)
        oColumnas.nueva("TIMES", _("Time"), 120, siCentrado=True)
        oColumnas.nueva("TOLERANCE", _("Tolerance"), 90, siCentrado=True)
        oColumnas.nueva("TRIES", _("Tries"), 90, siCentrado=True)
        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Start"), Iconos.Empezar(), self.start), None,
            (_("New"), Iconos.Nuevo(), self.nuevo), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        self.tb = QTVarios.LCTB(self, liAcciones)

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.grid).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.grid)
        self.recuperarVideo(siTam=False)

        self.grid.gotop()

    def gridDobleClick(self, grid, fil, col):
        if fil >= 0:
            self.start()

    def gridNumDatos(self, grid):
        return self.db.reccount()

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        v = self.db.field(fila, col)

        # if col in ("NAME", "TOLERANCE", "TRIES"):return v
        if col in ("DATE_INIT", "DATE_END"):
            d = Util.stodext(v)
            v = Util.localDateT(d) if d else ""

        elif col == "TIMES":
            li = eval(v)
            seconds = sum(x for x, p in li)
            done_games = self.db.field(fila, "NEXT_GAME")  # next_game is 0 based
            mseconds = seconds / done_games if done_games > 0 else 0
            v = "%d' %d\" / %d' %d\"" % (mseconds / 60, mseconds % 60, seconds / 60, seconds % 60)

        elif col == "NUM_GAMES":
            next_game = self.db.field(fila, "NEXT_GAME")
            v = "%d / %d" % (next_game, v)

        return v

    def terminar(self):
        self.guardarVideo()
        self.db.close()
        self.reject()

    def borrar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.db.borrar_lista(li)
                self.grid.gotop()
                self.grid.refresh()

    def start(self):
        recno = self.grid.recno()
        if recno >= 0:
            self.guardarVideo()
            self.db.close()
            self.selected = recno
            self.accept()

    def nuevo(self):
        menu = QTVarios.LCMenu(self)

        menu.opcion("tourneys", _("Tourneys"), Iconos.Torneos())
        menu.separador()
        menu.opcion("fide_openings", _("Openings from progressive elo games"), Iconos.Apertura())
        menu.separador()
        menu.opcion("gm_openings", _("Openings from GM games"), Iconos.GranMaestro())

        resp = menu.lanza()
        if not resp:
            return
        fichero = "IntFiles/Everest/%s.str"%resp
        w = WNewExpedition(self, fichero)
        if w.exec_():
            reg = w.selected
            self.db.new(reg)
            self.grid.refresh()


def everest(procesador):
    w = WEverest(procesador)
    if w.exec_():
        procesador.showEverest(w.selected)


def show_expedition(wowner, configuracion, recno):
    wexp = WExpedition(wowner, configuracion, recno)
    return wexp.exec_()
