from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTVarios

class WFavoritos(QTVarios.WDialogo):
    def __init__(self, entrenamientos):

        entrenamientos.comprueba()
        self.entrenamientos = entrenamientos
        self.procesador = entrenamientos.procesador
        self.liFavoritos = self.procesador.configuracion.liFavoritos

        QTVarios.WDialogo.__init__(self, self.procesador.pantalla, _("Training favorites"), Iconos.Corazon(),
                                   "favoritos")

        # Toolbar
        liAcciones = [
            (_("Close"), Iconos.MainMenu(), "terminar"), None,
            (_("New"), Iconos.Nuevo(), "nuevo"), None,
            (_("Remove"), Iconos.Borrar(), "borrar"), None,
            (_("Up"), Iconos.Arriba(), "arriba"), None,
            (_("Down"), Iconos.Abajo(), "abajo"), None,
        ]
        tb = Controles.TB(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("OPCION", _("Training"), 400)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)
        f = Controles.TipoLetra(puntos=10, peso=75)
        self.grid.setFont(f)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).relleno()
        self.setLayout(layout)

        self.recuperarVideo(siTam=True)

        self.grid.gotop()

    def procesarTB(self):
        self.guardarVideo()
        accion = self.sender().clave
        if accion == "terminar":
            self.accept()

        elif accion == "nuevo":
            self.nuevo()

        elif accion == "borrar":
            self.borrar()

        elif accion == "arriba":
            self.arriba()

        elif accion == "abajo":
            self.abajo()

    def gridNumDatos(self, grid):
        return len(self.liFavoritos)

    def gridDato(self, grid, fila, oColumna):
        clave = self.liFavoritos[fila]
        dic = self.entrenamientos.dicMenu
        dato = dic.get(clave, None)
        if dato:
            return dato[1]
        else:
            return "???"

    def graba(self, fila):
        self.procesador.configuracion.liFavoritos = self.liFavoritos
        self.procesador.configuracion.graba()
        self.grid.refresh()
        if fila >= len(self.liFavoritos):
            fila = len(self.liFavoritos) - 1
        self.grid.goto(fila, 0)

    def nuevo(self):
        fila = self.grid.recno()
        resp = self.entrenamientos.menu.lanza()
        if resp:
            tam = len(self.liFavoritos)
            if fila < tam - 1:
                fila += 1
                self.liFavoritos.insert(fila, resp)
            else:
                self.liFavoritos.append(resp)
                fila = len(self.liFavoritos) - 1
            self.graba(fila)

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            del self.liFavoritos[fila]
            self.graba(fila)

    def arriba(self):
        fila = self.grid.recno()
        if fila >= 1:
            self.liFavoritos[fila], self.liFavoritos[fila - 1] = self.liFavoritos[fila - 1], self.liFavoritos[fila]
            self.graba(fila - 1)

    def abajo(self):
        fila = self.grid.recno()
        if fila < len(self.liFavoritos) - 1:
            self.liFavoritos[fila], self.liFavoritos[fila + 1] = self.liFavoritos[fila + 1], self.liFavoritos[fila]
            self.graba(fila + 1)

def miraFavoritos(entrenamientos):
    w = WFavoritos(entrenamientos)
    w.exec_()
