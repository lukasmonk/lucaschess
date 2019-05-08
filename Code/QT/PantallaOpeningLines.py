import os
import os.path

from PyQt4 import QtGui

from Code import VarGen
from Code import Partida
from Code import OpeningLines
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaAperturas
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Delegados
from Code.QT import FormLayout


class WOpeningLines(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.resultado = None
        self.listaOpenings = OpeningLines.ListaOpenings(self.configuracion)

        QTVarios.WDialogo.__init__(self, procesador.pantalla,  self.getTitulo(), Iconos.OpeningLines(), "openingLines")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("TITLE", _("Name"), 240)
        oColumnas.nueva("BASEPV", _("First moves"), 280)
        oColumnas.nueva("NUMLINES", _("Lines"), 80, siCentrado=True)
        oColumnas.nueva("FILE", _("File"), 200)
        self.glista = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)

        sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Edit"), Iconos.Modificar(), self.modificar), None,
            (_("New"), Iconos.Nuevo(), self.new), None,
            (_("Copy"), Iconos.Copiar(), self.copy), None,
            (_("Rename"), Iconos.Modificar(), self.renombrar), None,
            (_("Up"), Iconos.Arriba(), self.arriba),
            (_("Down"), Iconos.Abajo(), self.abajo), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Reinit"), Iconos.Reiniciar(), self.reiniciar), None,
            (_("Folder"), Iconos.File(), self.changeFolder), None,
        )
        tb = QTVarios.LCTB(self, liAcciones)

        tb.setSizePolicy(sp)

        liAcciones = (
            (_("Sequential"), Iconos.TrainSequential(), self.tr_sequential), None,
            (_("Static"), Iconos.TrainStatic(), self.tr_static), None,
            (_("Positions"), Iconos.TrainPositions(), self.tr_positions), None,
            (_("With engines"), Iconos.TrainEngines(), self.tr_engines),
        )
        self.tbtrain = tbtrain = Controles.TBrutina(self, liAcciones, siTexto=False)

        lbtrain = Controles.LB(self, _("Trainings")).alinCentrado().ponFondoN("lightgray")
        lytrain = Colocacion.V().control(lbtrain).control(tbtrain).margen(0)
        self.wtrain = QtGui.QWidget()
        self.wtrain.setLayout(lytrain)

        lytb = Colocacion.H().control(tb).control(self.wtrain).margen(0)
        wtb = QtGui.QWidget()
        wtb.setFixedHeight(62)
        wtb.setLayout(lytb)

        # Colocamos

        ly = Colocacion.V().control(wtb).control(self.glista).margen(4)

        self.setLayout(ly)

        self.registrarGrid(self.glista)
        self.recuperarVideo(anchoDefecto=self.glista.anchoColumnas()+20)

        self.wtrain.setVisible(False)
        self.glista.gotop()

    def getTitulo(self):
        return "%s [%s]" % (_("Opening lines"), os.path.relpath(self.listaOpenings.folder))

    def tr_(self, tipo):
        recno = self.glista.recno()
        op = self.listaOpenings[recno]
        op["TRAIN"] = tipo
        self.resultado = op
        self.terminar()

    def tr_sequential(self):
        self.tr_("sequential")

    def tr_static(self):
        self.tr_("static")

    def tr_positions(self):
        self.tr_("positions")

    def tr_engines(self):
        self.tr_("engines")

    def reiniciar(self):
        self.listaOpenings.reiniciar()
        self.glista.refresh()
        self.glista.gotop()
        if len(self.listaOpenings) == 0:
            self.wtrain.setVisible(False)

    def changeFolder(self):
        nof = _("New opening folder")
        base = self.configuracion.folderBaseOpenings
        li = [x for x in os.listdir(base) if os.path.isdir(os.path.join(base, x))]
        menu = QTVarios.LCMenu(self)
        rondo = QTVarios.rondoPuntos()
        menu.opcion("", _("Home folder"), Iconos.Home())
        menu.separador()
        for x in li:
            menu.opcion(x, x, rondo.otro())
        menu.separador()
        menu.opcion(":n", nof, Iconos.Nuevo())
        if VarGen.isWindows:
            menu.separador()
            menu.opcion(":m", _("Direct maintenance"), Iconos.Configurar())

        resp = menu.lanza()
        if resp is not None:
            if resp == ":m":
                os.startfile(base)
                return

            elif resp == ":n":
                name = ""
                error = ""
                while True:
                    liGen = [FormLayout.separador]
                    liGen.append((nof + ":", name))
                    if error:
                        liGen.append(FormLayout.separador)
                        liGen.append((None, error))

                    resultado = FormLayout.fedit(liGen, title=nof, parent=self,
                                                 icon=Iconos.OpeningLines(), anchoMinimo=460)
                    if resultado:
                        accion, liResp = resultado
                        name = liResp[0].strip()
                        if name:
                            path = os.path.join(base, name)
                            try:
                                os.mkdir(path)
                            except:
                                error = _("Unable to create this folder")
                                continue
                            if not os.path.isdir(path):
                                continue
                            break
                    else:
                        return
            else:
                path = os.path.join(base, resp)

            path = os.path.relpath(path)
            self.configuracion.folderOpenings = path
            self.configuracion.graba()
            self.listaOpenings = OpeningLines.ListaOpenings(self.configuracion)
            self.glista.refresh()
            self.glista.gotop()
            if len(self.listaOpenings) == 0:
                self.wtrain.setVisible(False)
            self.setWindowTitle(self.getTitulo())

    def arriba(self):
        fila = self.glista.recno()
        if self.listaOpenings.arriba(fila):
            self.glista.goto(fila - 1, 0)
            self.glista.refresh()

    def abajo(self):
        fila = self.glista.recno()
        if self.listaOpenings.abajo(fila):
            self.glista.goto(fila + 1, 0)
            self.glista.refresh()

    def modificar(self):
        recno = self.glista.recno()
        if recno >= 0:
            self.resultado = self.listaOpenings[recno]
        else:
            self.resultado = None
        self.guardarVideo()
        self.accept()

    def gridDobleClick(self, grid, fila, oColumna):
        recno = self.glista.recno()
        if recno >= 0:
            self.modificar()

    def new(self):
        si_expl = len(self.listaOpenings) < 4
        if si_expl:
            QTUtil2.mensaje(self, _("First you must select the initial moves."))
        w = PantallaAperturas.WAperturas(self, self.configuracion, None)
        if w.exec_():
            ap = w.resultado()
            pv = ap.a1h8 if ap else ""
            name = ap.nombre if ap else ""
        else:
            return

        if si_expl:
            QTUtil2.mensaje(self, _("Secondly you have to choose a name for this opening studio."))

        name = self.get_nombre(name)
        if name:
            file = self.listaOpenings.select_filename(name)
            self.listaOpenings.new(file, pv, name)
            self.resultado = self.listaOpenings[-1]
            self.guardarVideo()
            self.accept()

    def copy(self):
        recno = self.glista.recno()
        if recno >= 0:
            self.listaOpenings.copy(recno)
            self.glista.refresh()

    def get_nombre(self, name):
        liGen = [(None, None)]
        liGen.append((_("Opening studio name") + ":", name))
        resultado = FormLayout.fedit(liGen, title=_("Opening studio name"), parent=self, icon=Iconos.OpeningLines(), anchoMinimo=460)
        if resultado:
            accion, liResp = resultado
            name = liResp[0].strip()
            if name:
                return name
        return None

    def renombrar(self):
        fila = self.glista.recno()
        if fila >= 0:
            op = self.listaOpenings[fila]
            name = self.get_nombre(op["title"])
            if name:
                self.listaOpenings.change_title(fila, name)
                self.glista.refresh()

    def borrar(self):
        li = self.glista.recnosSeleccionados()
        if len(li) > 0:
            mens = _("Do you want to delete all selected records?")
            mens += "\n"
            for num, fila in enumerate(li, 1):
                mens += "\n%d. %s" % (num, self.listaOpenings[fila]["title"])
            if QTUtil2.pregunta(self, mens):
                li.sort(reverse=True)
                for fila in li:
                    del self.listaOpenings[fila]
                recno = self.glista.recno()
                self.glista.refresh()
                if recno >= len(self.listaOpenings):
                    self.glista.gobottom()

    def gridNumDatos(self, grid):
        return len(self.listaOpenings)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        op = self.listaOpenings[fila]
        if col == "TITLE":
            return op["title"]
        elif col == "FILE":
            return op["file"]
        elif col == "NUMLINES":
            return op["lines"]
        elif col == "BASEPV":
            pv = op["pv"]
            if pv:
                p = Partida.Partida()
                p.leerPV(pv)
                return p.pgnBaseRAW()
            else:
                return ""

    def gridCambiadoRegistro(self, grid, fila, columna):
        ok_ssp = False
        ok_eng = False
        if fila >= 0:
            op = self.listaOpenings[fila]
            ok_ssp = op.get("withtrainings", False)
            ok_eng = op.get("withtrainings_engines", False)

        if ok_ssp or ok_eng:
            self.wtrain.setVisible(True)
            self.tbtrain.setAccionVisible(self.tr_sequential, ok_ssp)
            self.tbtrain.setAccionVisible(self.tr_static, ok_ssp)
            self.tbtrain.setAccionVisible(self.tr_positions, ok_ssp)
            self.tbtrain.setAccionVisible(self.tr_engines, ok_eng)
        else:
            self.wtrain.setVisible(False)

    def closeEvent(self, event):  # Cierre con X
        self.guardarVideo()

    def terminar(self):
        self.guardarVideo()
        self.accept()


class WStaticTraining(QTVarios.WDialogo):
    def __init__(self, procesador, dbop):
        self.training = dbop.training()
        self.ligames = self.training["LIGAMES_STATIC"]
        self.num_games = len(self.ligames)
        self.elems_fila = 10
        if self.num_games < self.elems_fila:
            self.elems_fila = self.num_games
        self.num_filas = (self.num_games-1) / self.elems_fila + 1
        self.seleccionado = None

        titulo = "%s - %s" % (_("Opening lines"), _("Static training"))

        extparam = "openlines_static_%s" % dbop.nomFichero

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, Iconos.TrainStatic(), extparam)

        lb = Controles.LB(self,  dbop.gettitle())
        lb.ponFondoN("#BDDBE8").alinCentrado().ponTipoLetra(puntos=14)

        # Toolbar
        tb = QTVarios.LCTB(self)
        tb.new(_("Close"), Iconos.MainMenu(), self.terminar)

        # Lista
        ancho = 42
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("FILA", "", 36, siCentrado=True)
        for x in range(self.elems_fila):
            oColumnas.nueva("COL%d" % x, "%d" % (x+1,), ancho, siCentrado=True, edicion=Delegados.PmIconosWeather())

        self.grid = Grid.Grid(self, oColumnas, altoFila=ancho, background="white")
        self.grid.setAlternatingRowColors(False)
        self.grid.tipoLetra(puntos=10, peso=500)
        nAnchoPgn = self.grid.anchoColumnas() + 20
        self.grid.setMinimumWidth(nAnchoPgn)

        ly = Colocacion.V().control(lb).control(tb).control(self.grid)
        self.setLayout(ly)

        alto = self.num_filas*ancho + 146
        self.recuperarVideo(siTam=True, altoDefecto=alto, anchoDefecto=nAnchoPgn)

    def terminar(self):

        self.guardarVideo()
        self.reject()

    def gridNumDatos(self, grid):
        return self.num_filas

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "FILA":
            return "%d" % fila
        elif col.startswith("COL"):
            num = fila*self.elems_fila + int(col[3:])
            if num >= self.num_games:
                return None
            game = self.ligames[num]
            sinerror = game["NOERROR"]
            return str(sinerror) if sinerror < 4 else "4"

    def gridDobleClick(self, grid, fila, oColumna):
        col = oColumna.clave
        if col.startswith("COL"):
            num = fila*self.elems_fila + int(col[3:])
            if num >= self.num_games:
                return
            self.seleccionado = num
            self.guardarVideo()
            self.accept()


def selectLine(procesador, dbop):
    w = WStaticTraining(procesador, dbop)
    w.exec_()
    return w.seleccionado


def openingLines(procesador):
    w = WOpeningLines(procesador)
    return w.resultado if w.exec_() else None

