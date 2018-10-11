from Code import Util
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios


class WAnotar(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.resultado = None
        self.db = Util.DicSQL(self.configuracion.ficheroAnotar)
        self.lista = self.db.keys(True, True)
        self.resultado = None

        QTVarios.WDialogo.__init__(self, procesador.pantalla,  _("Writing down moves of a game"), Iconos.Write(), "annotateagame")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("DATE", _("Date"), 110)
        oColumnas.nueva("COLOR", _("Color"), 80, siCentrado=True)
        oColumnas.nueva("GAME", _("Game"), 280)
        oColumnas.nueva("MOVES", _("Moves"), 80, siCentrado=True)
        oColumnas.nueva("TIME", _("Avg time"), 80, siCentrado=True)
        oColumnas.nueva("ERRORS", _("Errors"), 80, siCentrado=True)
        oColumnas.nueva("HINTS", _("Hints"), 80, siCentrado=True)
        self.glista = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("New"), Iconos.Nuevo(), self.new), None,
            (_("Repeat"), Iconos.Copiar(), self.repetir), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        tb = Controles.TBrutina(self, liAcciones)

        ly = Colocacion.V().control(tb).control(self.glista).margen(4)

        self.setLayout(ly)

        self.registrarGrid(self.glista)
        self.recuperarVideo(anchoDefecto=self.glista.anchoColumnas()+20)

    def gridDobleClick(self, grid, fila, oColumna):
        self.repetir()

    def repetir(self):
        recno = self.glista.recno()
        if recno >= 0:
            registro = self.db[self.lista[recno]]
            self.haz(registro["PC"])

    def new(self):
        self.haz(None)

    def haz(self, que):
        siblancasabajo = QTVarios.blancasNegras(self)
        if siblancasabajo is None:
            return
        self.resultado = que, siblancasabajo
        self.guardarVideo()
        self.db.close()
        self.accept()

    def borrar(self):
        li = self.glista.recnosSeleccionados()
        if len(li) > 0:
            mens = _("Do you want to delete all selected records?")
            if QTUtil2.pregunta(self, mens):
                for fila in li:
                    del self.db[self.lista[fila]]
                recno = self.glista.recno()
                self.glista.refresh()
                self.lista = self.db.keys(True, True)
                if recno >= len(self.lista):
                    self.glista.gobottom()

    def gridNumDatos(self, grid):
        return len(self.lista)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.db[self.lista[fila]]
        if not reg:
            return ""
        if col == "DATE":
            return self.lista[fila]
        elif col == "GAME":
            return reg["PC"].titulo(("DATE", "EVENT", "WHITE", "BLACK", "RESULT"))
        elif col == "MOVES":
            total = len(reg["PC"])
            moves = reg["MOVES"]
            if total == moves:
                return str(total)
            else:
                return "%d/%d" % (moves, total)
        elif col == "TIME":
            return "%0.2f\"" % reg["TIME"]
        elif col == "HINTS":
            return str(reg["HINTS"])
        elif col == "ERRORS":
            return str(reg["ERRORS"])
        elif col == "COLOR":
            return _("White") if reg["COLOR"] else _("Black")

    def closeEvent(self, event):  # Cierre con X
        self.db.close()
        self.guardarVideo()

    def terminar(self):
        self.db.close()
        self.guardarVideo()
        self.reject()

