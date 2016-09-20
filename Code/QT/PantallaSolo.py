from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTVarios
from Code import TrListas
from Code import Util

class WEtiquetasPGN(QTVarios.WDialogo):
    def __init__(self, procesador, liPGN):
        titulo = _("Edit PGN labels")
        icono = Iconos.PGN()
        extparam = "editlabels"

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)
        self.procesador = procesador
        self.creaLista(liPGN)

        # Toolbar
        liAccionesWork = (
            (_("Accept"), Iconos.Aceptar(), self.aceptar), None,
            (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
            (_("Up"), Iconos.Arriba(), self.arriba), None,
            (_("Down"), Iconos.Abajo(), self.abajo), None,
        )
        tbWork = Controles.TBrutina(self, liAccionesWork, tamIcon=24)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ETIQUETA", _("Label"), 150, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("VALOR", _("Value"), 400, edicion=Delegados.LineaTextoUTF8())

        self.grid = Grid.Grid(self, oColumnas, siEditable=True)
        n = self.grid.anchoColumnas()
        self.grid.setFixedWidth(n + 20)
        self.registrarGrid(self.grid)

        # Layout
        layout = Colocacion.V().control(tbWork).control(self.grid).margen(3)
        self.setLayout(layout)

        self.recuperarVideo()

    def creaLista(self, liPGN):
        sd = Util.SymbolDict()
        for eti, val in liPGN:
            sd[eti] = val

        li = []
        listandard = ("Event", "Site", "Date", "Round", "White", "Black", "Result", "ECO", "WhiteElo", "BlackElo")
        for eti in listandard:
            li.append([eti, sd.get(eti,"")])
        for eti, val in liPGN:
            if eti not in listandard:
                li.append([eti,val])
        while len(li) < 30:
            li.append(["", ""])
        self.liPGN = li

    def aceptar(self):
        self.guardarVideo()
        self.accept()

    def closeEvent(self):
        self.guardarVideo()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def gridNumDatos(self, grid):
        return len(self.liPGN)

    def gridPonValor(self, grid, fila, oColumna, valor):
        col = 0 if oColumna.clave == "ETIQUETA" else 1
        self.liPGN[fila][col] = valor

    def gridDato(self, grid, fila, oColumna):
        if oColumna.clave == "ETIQUETA":
            lb = self.liPGN[fila][0]
            ctra = lb.upper()
            trad = TrListas.pgnLabel(lb)
            if trad != ctra:
                clave = trad
            else:
                if lb:
                    clave = lb  # [0].upper()+lb[1:].lower()
                else:
                    clave = ""
            return clave
        return self.liPGN[fila][1]

    def arriba(self):
        recno = self.grid.recno()
        if recno:
            self.liPGN[recno], self.liPGN[recno - 1] = self.liPGN[recno - 1], self.liPGN[recno]
            self.grid.goto(recno - 1, 0)
            self.grid.refresh()

    def abajo(self):
        n0 = self.grid.recno()
        if n0 < len(self.liPGN) - 1:
            n1 = n0 + 1
            self.liPGN[n0], self.liPGN[n1] = self.liPGN[n1], self.liPGN[n0]
            self.grid.goto(n1, 0)
            self.grid.refresh()

def editarEtiquetasPGN(procesador, liPGN):
    w = WEtiquetasPGN(procesador, liPGN)
    if w.exec_():
        li = []
        for eti, valor in w.liPGN:
            if (len(eti.strip()) > 0) and (len(valor.strip()) > 0):
                li.append([eti, valor])
        return li
    else:
        return None
