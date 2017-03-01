from Code import PGN
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import VarGen


def texto2partida(owner, texto):
    pgn = PGN.UnPGN()
    pgn.leeTexto(texto)
    if pgn.siError:
        QTUtil2.mensError(owner, _("This is not a valid PGN file"))
        return None
    return pgn.partida


class W_EligeMovimientos(QTVarios.WDialogo):
    def __init__(self, owner, partida):

        titulo = _("Choose moves")
        icono = Iconos.Camara()
        extparam = "tabvpart"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.owner = owner
        self.partida = partida
        self.siEmpiezaConNegras = partida.siEmpiezaConNegras
        siTodos = True
        self.liElegidos = [siTodos] * len(partida)

        liAcciones = [(_("Accept"), Iconos.Aceptar(), "aceptar"), None,
                      (_("Cancel"), Iconos.Cancelar(), "reject"), None,
                      (_("Mark"), Iconos.Marcar(), "marcar"), None,
                      ]
        tb = Controles.TB(self, liAcciones)

        # Tablero
        confTablero = VarGen.configuracion.confTablero("ELIGEMOVS", 24)
        self.tablero = Tablero.TableroVisual(self, confTablero)
        self.tablero.crea()
        self.tablero.desactivaTodas()

        # Pgn
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ELEGIDO", "", 30, siCentrado=True, siChecked=True)
        oColumnas.nueva("NUMERO", _("N."), 35, siCentrado=True)
        oColumnas.nueva("BLANCAS", _("White"), 100, siCentrado=True)
        oColumnas.nueva("NEGRAS", _("Black"), 100, siCentrado=True)
        self.pgn = Grid.Grid(self, oColumnas, siCabeceraMovible=False, siSelecFilas=True)
        nAnchoPgn = self.pgn.anchoColumnas() + 20
        self.pgn.setMinimumWidth(nAnchoPgn)
        self.pgn.gotop()

        ly = Colocacion.H().control(self.tablero).control(self.pgn)

        layout = Colocacion.V().control(tb).otro(ly)
        self.setLayout(layout)
        self.pgn.setFocus()

        self.resultado = []

    def procesarTB(self):
        accion = self.sender().clave
        eval("self.%s()" % accion)

    def aceptar(self):
        li = []
        for n, siElegido in enumerate(self.liElegidos):
            if siElegido:
                li.append(self.partida.liJugadas[n])
        self.resultado = li
        self.accept()

    def gridNumDatos(self, grid):
        return len(self.partida)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        if clave == "ELEGIDO":
            return self.liElegidos[fila]
        elif clave == "NUMERO":
            if self.siEmpiezaConNegras:
                fila += 1
            return str(fila / 2 + 1)
        else:
            valor = self.partida.jugada(fila).pgnSP()
            siBlancas = self.partida.jugada(fila).posicionBase.siBlancas
            if clave == "BLANCAS":
                return valor if siBlancas else ""
            else:
                return valor if not siBlancas else ""

    def gridPonValor(self, grid, fila, oColumna, valor):
        self.liElegidos[fila] = valor

    def marcar(self):
        menu = QTVarios.LCMenu(self)
        f = Controles.TipoLetra(puntos=8, peso=75)
        menu.ponFuente(f)
        menu.opcion(1, _("All"), Iconos.PuntoVerde())
        menu.opcion(2, _("None"), Iconos.PuntoNaranja())
        resp = menu.lanza()
        if resp:
            for n in range(len(self.liElegidos)):
                self.liElegidos[n] = resp == 1
            self.pgn.refresh()

    def gridCambiadoRegistro(self, grid, fil, columna):
        jg = self.partida.jugada(fil)
        self.tablero.ponPosicion(jg.posicion)
        self.tablero.ponFlechaSC(jg.desde, jg.hasta)
        self.pgn.setFocus()
