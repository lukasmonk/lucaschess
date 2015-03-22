import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.QTVarios as QTVarios
import Code.QT.Columnas as Columnas
import Code.QT.Grid as Grid

import Code.QT.FormLayout as FormLayout

class WBoxing(QTVarios.WDialogo):
    def __init__(self, owner, boxing):

        self.boxing = boxing

        # Dialogo ---------------------------------------------------------------
        icono = Iconos.Resistencia()
        titulo = _("Resistance Test")
        tipo = boxing.tipo
        if tipo:
            titulo += "-" + _("Blindfold chess")
            if tipo == "p1":
                titulo += "-" + _("Hide only our pieces")
            elif tipo == "p2":
                titulo += "-" + _("Hide only opponent pieces")
        extparam = "boxing"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)
        # self.setStyleSheet("QWidget { background: #AFC3D7 }")

        # Tool bar ---------------------------------------------------------------
        liAcciones = [( _("Quit"), Iconos.MainMenu(), "terminar" ), None,
                      ( _("Remove data"), Iconos.Borrar(), "borrar" ), None,
                      ( _("Config"), Iconos.Configurar(), "configurar" ),
        ]
        tb = Controles.TB(self, liAcciones, background="#AFC3D7")

        # Texto explicativo ----------------------------------------------------
        self.lb = Controles.LB(self)
        self.ponTextoAyuda()
        self.lb.ponFondoN("#F5F0CF")

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ENGINE", _("Engine"), 198)
        oColumnas.nueva("BLANCAS", _("White"), 200, siCentrado=True)
        oColumnas.nueva("NEGRAS", _("Black"), 200, siCentrado=True)

        self.grid = grid = Grid.Grid(self, oColumnas, siSelecFilas=True, background=None)
        self.grid.coloresAlternados()
        self.registrarGrid(grid)

        # Layout
        lyB = Colocacion.V().controlc(self.lb).control(self.grid).margen(3)
        layout = Colocacion.V().control(tb).otro(lyB).margen(0)
        self.setLayout(layout)

        self.recuperarVideo(siTam=True, anchoDefecto=677, altoDefecto=562)

        self.grid.gotop()

        self.grid.setFocus()
        self.resultado = None

    def ponTextoAyuda(self):
        segundos, puntos = self.boxing.actual()
        self.lb.ponTexto('<center><b>%s<br><font color="red">%s</red></b></center>' % \
                         (_X(_(
                             "Target %1/%2: withstand maximum moves against an engine,<br>        that thinks %1 second(s), without losing more than %2 points."),
                             str(segundos), str(puntos)), \
                          _("Double click in any cell to begin to play") ))

    def gridNumDatos(self, grid):
        return self.boxing.numEngines()

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        if clave == "ENGINE":
            return self.boxing.dameEtiEngine(fila)
        else:
            return self.boxing.dameEtiRecord(clave, fila)

    def gridDobleClick(self, grid, fila, columna):
        clave = columna.clave
        if clave != "ENGINE":
            self.play(clave)

    def play(self, clave):
        self.guardarVideo()
        self.resultado = self.grid.recno(), clave
        self.accept()

    def borrar(self):
        numEngine = self.grid.recno()
        if QTUtil2.pregunta(self, _X(_("Remove data from %1 ?"), self.boxing.dameEtiEngine(numEngine))):
            self.boxing.borraRegistros(numEngine)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "terminar":
            self.guardarVideo()
            self.accept()
        elif accion == "play":
            self.play()

        elif accion == "borrar":
            self.borrar()

        elif accion == "configurar":
            self.configurar()

    def configurar(self):
        segundos, puntos = self.boxing.actual()

        liGen = [(None, None)]

        config = FormLayout.Spinbox(_("Time in seconds"), 1, 99999, 80)
        liGen.append(( config, segundos ))

        config = FormLayout.Spinbox(_("Points"), 10, 99999, 80)
        liGen.append(( config, puntos ))

        resultado = FormLayout.fedit(liGen, title=_("Config"), parent=self, icon=Iconos.Configurar())
        if resultado:
            accion, liResp = resultado
            segundos = liResp[0]
            puntos = liResp[1]
            self.boxing.cambiaConfiguracion(segundos, puntos)
            self.ponTextoAyuda()
            self.grid.refresh()
            return liResp[0]

def pantallaBoxing(window, boxing):
    w = WBoxing(window, boxing)
    if w.exec_():
        return w.resultado
    else:
        return None

