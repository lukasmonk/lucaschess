
from Code.QT import QTVarios
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code import SingularMoves


class WSingularM(QTVarios.WDialogo):
    def __init__(self, owner, configuracion):
        self.configuracion = configuracion
        titulo = "%s: %s" % (_("Singular moves"), _("Calculate your strength"))
        icono = Iconos.Strength()
        extparam = "singularmoves"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.sm = SingularMoves.SingularMoves(configuracion.ficheroSingularMoves)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.cerrar), None,
            (_("New"), Iconos.Empezar(), self.nuevo), None,
            (_("Repeat"), Iconos.Pelicula_Repetir(), self.repetir), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        tb = Controles.TBrutina(self, liAcciones)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("N", _("N."), 60, siCentrado=True)
        oColumnas.nueva("DATE", _("Date"), 120, siCentrado=True)
        oColumnas.nueva("STRENGTH", _("Strength"), 80, siCentrado=True)
        oColumnas.nueva("REPETITIONS", _("Repetitions"), 80, siCentrado=True)
        oColumnas.nueva("BEST", _("Best strength"), 120, siCentrado=True)
        self.grid = grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        grid.coloresAlternados()
        self.registrarGrid(grid)

        ly = Colocacion.V().control(tb).control(grid).margen(3)

        self.setLayout(ly)

        grid.gotop()
        self.recuperarVideo(anchoDefecto=510, altoDefecto=640)

    def cerrar(self):
        self.guardarVideo()
        self.reject()

    def nuevo(self):
        self.guardarVideo()
        self.sm.current = -1
        self.sm.nuevo_bloque()
        self.accept()

    def repetir(self):
        fila = self.grid.recno()
        if fila >= 0:
            self.guardarVideo()
            self.sm.repite(fila)
            self.accept()

    def borrar(self):
        li = self.grid.recnosSeleccionados()
        if li and QTUtil2.pregunta(self, _("Are you sure?")):
            self.sm.borra_db(li)
            self.grid.refresh()
            self.grid.goto(li[0] if li[0] < self.sm.len_db() else 0, 0)

    def gridNumDatos(self, grid):
        return self.sm.len_db()

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "N":
            return "%d" % (fila+1,)
        if col == "DATE":
            key = self.sm.db_keys[fila]
            return "%s-%s-%s %s:%s" % (key[:4], key[4:6], key[6:8], key[8:10], key[10:12])
        registro = self.sm.reg_db(fila)
        if col == "STRENGTH":
            return "%0.2f" % registro.get("STRENGTH", 0.0)
        if col == "BEST":
            return "%0.2f" % registro.get("BEST", 0.0)
        if col == "REPETITIONS":
            rep = registro.get("REPETITIONS", [])
            return len(rep) if len(rep) else ""

    def gridDobleClick(self, grid, fila, columna):
        self.repetir()

