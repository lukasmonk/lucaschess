from Code import TurnOnLights
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import QTUtil
from Code.QT import QTUtil2

class WTurnOnLights(QTVarios.WDialogo):
    def __init__(self, owner, name, title, icono, folder, li_tam_blocks):

        self.tol = TurnOnLights.read_tol(name, title, folder, li_tam_blocks)
        self.reinit = False

        titulo = _("Turn on the lights") + ": " + title
        extparam = "tol%s-%d" % (name, self.tol.work_level)
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.colorTheme = QTUtil.qtColor("#F0F0F0")

        # Toolbar
        tb = Controles.TBrutina(self)
        tb.new(_("Close"), Iconos.MainMenu(), self.terminar)
        anterior, siguiente = self.tol.prev_next()
        if anterior:
            tb.new(_("Previous"), Iconos.Anterior(), self.goto_previous)
        if siguiente:
            tb.new(_("Next"), Iconos.Siguiente(), self.goto_next)
        tb.new(_("Config"), Iconos.Configurar(), self.config)
        tb.new(_("Information"), Iconos.Informacion(), self.colors)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        work_level = self.tol.work_level + 1
        oColumnas.nueva("THEME", _("Level %d/%d") % (work_level, self.tol.num_levels), 175)
        for x in range(self.tol.num_blocks):
            oColumnas.nueva("BLOCK%d" % x, "%d" % (x+1,), 42, siCentrado=True, edicion=Delegados.PmIconosColor())

        self.grid = grid = Grid.Grid(self, oColumnas, altoFila=42, background="white")
        self.grid.setAlternatingRowColors(False)
        self.grid.tipoLetra(puntos=10, peso=500)
        nAnchoPgn = self.grid.anchoColumnas() + 20
        self.grid.setMinimumWidth(nAnchoPgn)
        self.registrarGrid(grid)

        # Colocamos ---------------------------------------------------------------
        ly = Colocacion.V().control(tb).control(self.grid)

        self.setLayout(ly)

        alto = self.tol.num_themes*42 + 116
        self.recuperarVideo(siTam=True, altoDefecto=alto, anchoDefecto=nAnchoPgn)

    def terminar(self):
        self.guardarVideo()
        self.reject()

    def gridNumDatos(self, grid):
        return self.tol.num_themes

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "THEME":
            return "  " + self.tol.nom_theme(fila)
        elif col.startswith("BLOCK"):
            block = int(col[5:])
            return self.tol.val_block(fila, block)

    def gridColorFondo(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "THEME":
            return self.colorTheme
        return None

    def gridDobleClick(self, grid, fila, oColumna):
        col = oColumna.clave
        if col.startswith("BLOCK"):
            block = int(col[5:])
            self.num_theme = fila
            self.num_block = block
            self.guardarVideo()
            self.accept()

    def goto_previous(self):
        self.tol.work_level -= 1
        TurnOnLights.write_tol(self.tol)
        self.reinit = True
        self.guardarVideo()
        self.accept()

    def goto_next(self):
        self.tol.work_level += 1
        TurnOnLights.write_tol(self.tol)
        self.reinit = True
        self.guardarVideo()
        self.accept()

    def config(self):
        menu = QTVarios.LCMenu(self)
        smenu = menu.submenu(_("Tactics"), Iconos.Tacticas())
        go_fast = self.tol.go_fast
        dico = {True: Iconos.Aceptar(), False: Iconos.PuntoAmarillo()}
        smenu.opcion("t_false", _("Stop after solve"), dico[not go_fast], siDeshabilitado=not go_fast)
        smenu.opcion("t_true", _("Jump to the next after solve"), dico[go_fast], siDeshabilitado=go_fast)
        menu.separador()
        menu.opcion("remove", _("Remove all results of all levels"), Iconos.Cancelar())
        resp = menu.lanza()
        if resp:
            if resp.startswith("t_"):
                self.tol.go_fast = resp == "t_true"
                TurnOnLights.write_tol(self.tol)
            elif resp == "remove":
                if not QTUtil2.pregunta(self, _("Are you sure you want to delete all results of all levels and start again from scratch?")):
                    return
                TurnOnLights.remove_tol(self.tol)
                self.reinit = True
                self.guardarVideo()
                self.accept()

    def colors(self):
        menu = QTVarios.LCMenu(self)
        d = {"0": Iconos.Gris32(),
             "1": Iconos.Amarillo32(),
             "2": Iconos.Naranja32(),
             "3": Iconos.Verde32(),
             "4": Iconos.Azul32(),
             "5": Iconos.Magenta32(),
             "6": Iconos.Rojo32(),
             "7": Iconos.Light32()
             }
        for txt, key, secs in TurnOnLights.QUALIFICATIONS:
            menu.opcion(None, "%s < %0.2f\"" % (_F(txt), secs), d[key])
            menu.separador()
        menu.lanza()

def pantallaTurnOnLigths(procesador, name, title, icono, folder, li_tam_blocks):
    while True:
        w = WTurnOnLights(procesador.pantalla, name, title, icono, folder, li_tam_blocks)
        if w.exec_():
            if not w.reinit:
                return w.num_theme, w.num_block, w.tol
        else:
            break
    return None

