import os
import shutil

from PyQt4 import QtSvg, QtCore

from Code import Util
from Code import Washing
from Code import DBgames
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import QTUtil2
from Code.QT import FormLayout
from Code.QT import PantallaSavePGN


class WWashing(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.siPlay = False
        self.wreload = False

        self.dbwashing = Washing.DBWashing(procesador.configuracion)
        self.washing = self.dbwashing.washing
        eng = self.washing.lastEngine(procesador.configuracion)
        siTerminado = eng is None

        owner = procesador.pantalla
        titulo = _("The Washing Machine")
        extparam = "washing"
        icono = Iconos.WashingMachine()
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        tb = QTVarios.LCTB(self)
        tb.new(_("Close"), Iconos.MainMenu(), self.terminar)
        tb.new(_("File"), Iconos.Recuperar(), self.file)
        if not siTerminado:
            tb.new(_("Play"), Iconos.Play(), self.play)

        # Tab current
        ia = self.washing.index_average()

        c_create = "#f9e7e7"
        c_tactics = "#df8f87"
        c_reinit = "#8aa678"
        c_des = "#e4e4e4"
        c_hab = "#9dde67"
        c_act = "#dd2a2b"
        c_ia = "#cccdea"

        li_ia = ("#ffed00", "#ff8e00", "#29b41b", "#1174ff", "#bc01d9", "#eb0000")
        d_ia = li_ia[int(eng.index()/(100.0/6.0))%6]
        state = eng.state
        wsvg = QtSvg.QSvgWidget()
        with open("IntFiles/washing-machine.svg") as f:
            svg = f.read()
            d_create = c_des
            d_tactics = c_des
            d_reinit = c_des
            ctac = ""
            if state == Washing.CREATING:
                d_create = c_act
            elif state == Washing.REPLAY:
                d_create = c_hab
                d_tactics = c_hab
                d_reinit = c_act
            elif state == Washing.TACTICS:
                d_create = c_hab
                d_tactics = c_act
                ctac = str(eng.numTactics())
            svg = svg.replace(c_create, d_create)
            svg = svg.replace(c_tactics, d_tactics)
            svg = svg.replace(c_reinit, d_reinit)
            svg = svg.replace("TAC", ctac)
            svg = svg.replace(c_ia, d_ia)
            wsvg.load(QtCore.QByteArray(svg))
        p = 1.0
        wsvg.setFixedSize(287.79*p, 398.83*p)

        if siTerminado:
            plant = "<tr><td align=\"right\">%s:</td><td><b>%s</b></td></tr>"
            hints, times, games = self.washing.totals()
            nEngines = self.washing.numEngines()
            html = '<h2><center>%s: %d %s</center></h2><br><table cellpadding="4">'% ( _("Finished"), nEngines, _("engines") )
            for x in range(3):
                html += plant
            html += "</table>"

            html = html % ( _("Hints"), "%d (%0.02f)" % (hints, hints*1.0/nEngines),
                            _("Games"), "%d (%0.02f)" % (games, games*1.0/nEngines),
                            _("Time"), "%s (%s)" % (Util.secs2str(times), Util.secs2str(int(times/nEngines))),
                        )

        else:
            plant = "<tr><td align=\"right\">%s:</td><td><b>%s</b></td></tr>"
            plantverde = "<tr><td align=\"right\">%s:</td><td style=\"color:green;\"><b>%s</b></td></tr>"
            html = "<h2><center>%s %d/%d</center></h2><br><table cellpadding=\"4\">"
            for x in range(8):
                html += plantverde if x == 2 else plant
            html += "</table>"
            html = html % ( _("Washing"), self.washing.numEngines(), self.washing.totalEngines(procesador.configuracion),
                            _("Engine"), eng.nombre,
                            _("Elo"), eng.elo,
                            "<b>%s</b>"% _("Task"), eng.lbState(),
                            _("Color"), _("White") if eng.color else _("Black"),
                            _("Hints"), "%d/%d"%(eng.hints_current, eng.hints),
                            _("Games"), eng.games,
                            _("Time"), eng.lbTime(),
                            _("Index"), eng.cindex(),
                        )

        lbTxt = Controles.LB(self, html).ponTipoLetra(puntos=12)
        lbIdx = Controles.LB(self, "%0.2f%%" % ia).alinCentrado().ponTipoLetra(puntos=36, peso=700)

        ly0 = Colocacion.V().control(wsvg).relleno(1)
        ly1 = Colocacion.V().espacio(20).control(lbTxt).espacio(20).control(lbIdx).relleno(1)
        ly2 = Colocacion.H().otro(ly0).otro(ly1)
        gbCurrent = Controles.GB(self, "", ly2)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("STEP", _("Washing"), 50, siCentrado=True)
        oColumnas.nueva("ENGINE", _("Engine"), 170, siCentrado=True)
        oColumnas.nueva("ELO", _("Elo"), 50, siCentrado=True)
        oColumnas.nueva("COLOR", _("Color"), 70, siCentrado=True)
        oColumnas.nueva("STATE", _("State"), 90, siCentrado=True)
        oColumnas.nueva("HINTS", _("Hints"), 60, siCentrado=True)
        oColumnas.nueva("GAMES", _("Games"), 60, siCentrado=True)
        oColumnas.nueva("TIME", _("Time"), 60, siCentrado=True)
        oColumnas.nueva("DATE", _("Date"), 120, siCentrado=True)
        oColumnas.nueva("INDEX", _("Index"), 60, siCentrado=True)

        self.grid = grid = Grid.Grid(self, oColumnas, siSelecFilas=True)
        nAnchoPgn = self.grid.anchoColumnas() + 20
        self.grid.setMinimumWidth(nAnchoPgn)
        self.registrarGrid(grid)

        ly0 = Colocacion.V().control(self.grid)
        gbDatos = Controles.GB(self, "", ly0)

        self.tab = Controles.Tab()
        self.tab.nuevaTab(gbCurrent, _("Current"))
        self.tab.nuevaTab(gbDatos, _("Data"))

        # Colocamos ---------------------------------------------------------------
        ly = Colocacion.V().control(tb).control(self.tab)
        self.setLayout(ly)

        self.recuperarVideo(siTam=True, anchoDefecto=nAnchoPgn)

    def terminar(self):
        self.guardarVideo()
        self.reject()

    def file(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion("saveas", _("Save a copy"), Iconos.GrabarComo())
        menu.separador()
        menu.opcion("restorefrom", _("Restore from"), Iconos.Recuperar())
        menu.separador()
        submenu = menu.submenu(_("Create new"), Iconos.Nuevo())
        submenu.opcion("new_UNED", _("UNED chess school"), Iconos.Uned())
        submenu.separador()
        submenu.opcion("new_UWE", _("Uwe Auerswald"), Iconos.Uwe())
        submenu.separador()
        submenu.opcion("new_SM", _("Singular moves"), Iconos.Singular())
        menu.separador()
        submenu = menu.submenu(_("Export to"), Iconos.DatabaseMas())
        submenu.opcion("save_pgn", _("A PGN file"), Iconos.FichPGN())
        submenu.separador()
        submenu.opcion("save_db", _("Database"), Iconos.DatabaseC())

        resp = menu.lanza()
        if resp is None:
            return
        if resp == "saveas":
            liGen = [(None, None)]
            config = FormLayout.Editbox(_("Name"), ancho=160)
            liGen.append((config, ""))

            resultado = FormLayout.fedit(liGen, title=_("Name"), parent=self, icon=Iconos.GrabarComo())
            if resultado:
                accion, liResp = resultado
                fich = nombre = liResp[0]
                if nombre.lower()[-4:] != ".wsm":
                    fich += ".wsm"
                path = os.path.join(self.configuracion.carpeta, fich)
                ok = True
                if Util.existeFichero(path):
                    ok = QTUtil2.pregunta(self, _X(_("The file %1 already exists, what do you want to do?"), fich),
                                                  etiSi=_("Overwrite"), etiNo=_("Cancel"))
                if ok:
                    shutil.copy(self.dbwashing.file, path)
        elif resp == "restorefrom":
            li = []
            for fich in os.listdir(self.configuracion.carpeta):
                if fich.endswith(".wsm") and fich != self.dbwashing.filename:
                    li.append(fich[:-4])
            if not li:
                QTUtil2.mensaje(self, _("There is no file") )
                return
            menu = QTVarios.LCMenu(self)
            for fich in li:
                menu.opcion(fich, fich, Iconos.PuntoRojo())
            resp = menu.lanza()
            if resp:
                if QTUtil2.pregunta(self, "%s\n%s" % (  _("Current data will be removed and overwritten."),
                                                        _("Are you sure?")) ):
                    shutil.copy(os.path.join(self.configuracion.carpeta, resp+".wsm"), self.dbwashing.file)
                    self.wreload = True
                    self.guardarVideo()
                    self.accept()
        elif resp.startswith("new_"):
            tactic = resp[4:]
            if QTUtil2.pregunta(self, "%s\n%s" % (  _("Current data will be removed and overwritten."),
                                                    _("Are you sure?")) ):
                self.dbwashing.new(tactic)
                self.wreload = True
                self.guardarVideo()
                self.accept()

        elif resp.startswith("save_"):
            def other_pc():
                for engine in self.washing.liEngines:
                    if engine.state == Washing.ENDED:
                        game = self.dbwashing.restoreGame(engine)
                        pc = Partida.PartidaCompleta()
                        pc.leeOtra(game)
                        dt = engine.date if engine.date else Util.hoy()
                        if engine.color:
                            white = self.configuracion.jugador
                            black = engine.nombre
                            result = "1-0"
                            whiteelo = str(self.configuracion.elo)
                            blackelo = engine.elo
                        else:
                            black = self.configuracion.jugador
                            white = engine.nombre
                            result = "0-1"
                            blackelo = str(self.configuracion.elo)
                            whiteelo = engine.elo
                        tags = [
                            ["Site", "Lucas Chess"],
                            ["Event", _("The Washing Machine")],
                            ["Date", "%d-%d-%d" % (dt.year, dt.month, dt.day)],
                            ["White", white],
                            ["Black", black],
                            ["WhiteElo", whiteelo],
                            ["BlackElo", blackelo],
                            ["Result", result],
                        ]
                        ap = game.apertura
                        if ap:
                            tags.append( ["ECO", ap.eco] )
                            tags.append( ["Opening", ap.trNombre] )
                        pc.setTags(tags)
                        yield pc

            if resp == "save_db":
                ext = "lcg"
                path = QTUtil2.salvaFichero(self, _("Database of complete games"), self.configuracion.ficheroDBgames,
                                            _("File") + " %s (*.%s)" % (ext, ext),
                                            False)
                if path:
                    if not path.lower().endswith(".lcg"):
                        path += ".lcg"
                    me = QTUtil2.mensEspera.inicio(self, _("Saving..."))
                    dbn = DBgames.DBgames(path)
                    for pc in other_pc():
                        dbn.inserta(pc)
                    me.final()
                    QTUtil2.mensaje(self, _X(_("Saved to %1"), path))
            else:
                w = PantallaSavePGN.WSaveVarios(self, self.configuracion)
                if w.exec_():
                    ws = PantallaSavePGN.FileSavePGN(self, w.dic_result)
                    if ws.open():
                        ws.um()
                        for n, pc in enumerate(other_pc()):
                            if n or not ws.is_new:
                                ws.write("\n\n")
                        ws.write(pc.pgn())
                    ws.close()
                    ws.um_final()

    def gridNumDatos(self, grid):
        return self.washing.numEngines()

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "STEP":
            return str(fila+1)
        engine = self.washing.liEngines[fila]
        if col == "ENGINE":
            return engine.nombre
        if col == "ELO":
            return str(engine.elo)
        if col == "STATE":
            return engine.lbState()
        if col == "COLOR":
            return _("White") if engine.color else _("Black")
        if col == "HINTS":
            return str(engine.hints)
        if col == "GAMES":
            return str(engine.games)
        if col == "TIME":
            return engine.lbTime()
        if col == "DATE":
            return engine.cdate()
        if col == "INDEX":
            return engine.cindex()

    def play(self):
        self.siPlay = True
        self.guardarVideo()
        self.accept()


def pantallaWashing(procesador):
    while True:
        w = WWashing(procesador)
        if w.exec_():
            if w.wreload:
                continue
            return w.siPlay
        return False

