from PyQt4 import QtCore, QtGui

import Code.TrListas as TrListas
import Code.Util as Util
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Grid as Grid
import Code.QT.Controles as Controles
import Code.QT.Columnas as Columnas
import Code.QT.Delegados as Delegados

class WEtiquetasPGN(QtGui.QDialog):
    def __init__(self, procesador, liPGN):
        wParent = procesador.pantalla
        self.procesador = procesador
        self.creaLista(liPGN)

        super(WEtiquetasPGN, self).__init__(wParent)

        self.setWindowTitle(_("Edit PGN labels"))
        self.setWindowIcon(Iconos.PGN())
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint)

        # Toolbar
        liAccionesWork = (
            ( _("Accept"), Iconos.Aceptar(), self.accept ), None,
            ( _("Cancel"), Iconos.Cancelar(), self.reject ), None,
            ( _("Up"), Iconos.Arriba(), self.arriba ), None,
            ( _("Down"), Iconos.Abajo(), self.abajo ), None,
        )
        tbWork = Controles.TBrutina(self, liAccionesWork, tamIcon=24)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ETIQUETA", _("Label"), 150, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("VALOR", _("Value"), 400, edicion=Delegados.LineaTextoUTF8())

        self.grid = Grid.Grid(self, oColumnas, siEditable=True)
        n = self.grid.anchoColumnas()
        self.grid.setFixedWidth(n + 20)

        # Layout
        layout = Colocacion.V().control(tbWork).control(self.grid).margen(3)
        self.setLayout(layout)

    def creaLista(self, liPGN):
        sd = Util.SymbolDict()
        for eti, val in liPGN:
            sd[eti] = val

        li = []
        for eti in (
                "Event", "Site", "Date", "Round", "White", "Black", "Result", "ECO", "FEN", "WhiteElo", "BlackElo" ):
            if eti in sd:
                li.append([eti, sd[eti]])
            else:
                li.append([eti, ""])
        while len(li) < 30:
            li.append(["", ""])
        self.liPGN = li

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "aceptar":
            self.accept()
        elif accion == "cancelar":
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

