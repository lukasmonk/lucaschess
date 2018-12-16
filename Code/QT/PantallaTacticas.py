from PyQt4 import QtGui

from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Util


def consultaHistorico(pantalla, tactica, icono):
    w = WHistoricoTacticas(pantalla, tactica, icono)
    return w.resultado if w.exec_() else None


class WHistoricoTacticas(QTVarios.WDialogo):
    def __init__(self, pantalla, tactica, icono):

        QTVarios.WDialogo.__init__(self, pantalla, tactica.titulo, icono, "histoTacticas")

        self.liHistorico = tactica.historico()
        self.tactica = tactica
        self.resultado = None

        # Historico
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("REFERENCE", _("Reference"), 120, siCentrado=True)
        oColumnas.nueva("FINICIAL", _("Start date"), 120, siCentrado=True)
        oColumnas.nueva("FFINAL", _("End date"), 120, siCentrado=True)
        oColumnas.nueva("TIEMPO", "%s - %s:%s" % (_("Days"), _("Hours"), _("Minutes")), 120, siCentrado=True)
        oColumnas.nueva("POSICIONES", _("Num. puzzles"), 100, siCentrado=True)
        oColumnas.nueva("SECONDS", _("Working time"), 100, siCentrado=True)
        oColumnas.nueva("ERRORS", _("Errors"), 100, siCentrado=True)
        self.ghistorico = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.ghistorico.setMinimumWidth(self.ghistorico.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), "terminar"),
            (_("Train"), Iconos.Empezar(), "entrenar"),
            (_("New"), Iconos.Nuevo(), "nuevo"),
            (_("Remove"), Iconos.Borrar(), "borrar"),
        )
        self.tb = Controles.TB(self, liAcciones)
        accion = "nuevo" if tactica.terminada() else "entrenar"
        self.ponToolBar("terminar", accion, "borrar")

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.ghistorico).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.ghistorico)
        self.recuperarVideo(siTam=False)

        self.ghistorico.gotop()

    def gridNumDatos(self, grid):
        return len(self.liHistorico)

    def gridDobleClick(self, grid, fila, oColumna):
        if fila == 0 and not self.tactica.terminada():
            self.entrenar()

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.liHistorico[fila]
        if col == "FINICIAL":
            fecha = reg["FINICIAL"]
            return Util.localDateT(fecha)
        elif col == "FFINAL":
            fecha = reg["FFINAL"]
            if fecha:
                return Util.localDateT(fecha)
            else:
                return "..."
        elif col == "TIEMPO":
            fi = reg["FINICIAL"]
            ff = reg["FFINAL"]
            if not ff:
                ff = Util.hoy()
            dif = ff - fi
            t = int(dif.total_seconds())
            h = t // 3600
            m = (t - h * 3600) // 60
            d = h // 24
            h -= d*24
            return "%d - %d:%02d" % (d, h, m)
        elif col == "POSICIONES":
            if "POS" in reg:
                posiciones = reg["POS"]
                if fila == 0:
                    posActual = self.tactica.posActual()
                    if posActual is not None and posActual < posiciones:
                        return "%d/%d" % (posActual, posiciones)
                    else:
                        return str(posiciones)
                else:
                    return str(posiciones)
            return "-"
        elif col == "SECONDS":
            seconds = reg.get("SECONDS", None)
            if fila == 0 and not seconds:
                seconds = self.tactica.segundosActivo()
            if seconds:
                hours = int(seconds / 3600)
                seconds -= hours*3600
                minutes = int(seconds / 60)
                seconds -= minutes * 60
                return "%02d:%02d:%02d" % (hours, minutes, int(seconds))
            else:
                return "-"

        elif col == "ERRORS":
            if fila == 0 and not self.tactica.terminada():
                errors = self.tactica.erroresActivo()
            else:
                errors = reg.get("ERRORS", None)
            if errors is None:
                return "-"
            else:
                return "%d" % errors

        elif col == "REFERENCE":
            if fila == 0 and not self.tactica.terminada():
                reference = self.tactica.referenciaActivo()
            else:
                reference = reg.get("REFERENCE", "")
            return reference

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def terminar(self):
        self.guardarVideo()
        self.reject()

    def nuevo(self):
        self.entrenar()

    def entrenar(self):
        if self.tactica.terminada():
            menu = QTVarios.LCMenu(self)
            menu.opcion("auto", _("Default settings"), Iconos.PuntoAzul())
            menu.separador()
            menu.opcion("manual", _("Manual configuration"), Iconos.PuntoRojo())

            n = self.ghistorico.recno()
            if n >= 0:
                reg = self.liHistorico[n]
                if "PUZZLES" in reg:
                    menu.separador()
                    menu.opcion("copia%d" % n, _("Copy configuration from current register"), Iconos.PuntoVerde())

            resp = menu.lanza()
            if not resp:
                return
            self.resultado = resp
        else:
            self.resultado = "seguir"
        self.guardarVideo()
        self.accept()

    def borrar(self):
        li = self.ghistorico.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.tactica.borraListaHistorico(li)
                self.liHistorico = self.tactica.historico()
        self.ghistorico.gotop()
        self.ghistorico.refresh()
        accion = "nuevo" if self.tactica.terminada() else "entrenar"
        self.ponToolBar("terminar", accion, "borrar")

    def ponToolBar(self, *liAcciones):

        self.tb.clear()
        for k in liAcciones:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])
            self.tb.addSeparator()

        self.tb.liAcciones = liAcciones
        self.tb.update()


class WConfTactics(QtGui.QWidget):
    def __init__(self, owner, tactica, ncopia=None):
        QtGui.QWidget.__init__(self)

        self.owner = owner
        self.tacticaINI = tactica
        if ncopia is not None:
            regHistorico = tactica.historico()[ncopia]
        else:
            regHistorico = None

        # Total por ficheros
        self.liFTOTAL = tactica.calculaTotales()
        total = sum(self.liFTOTAL)

        # N. puzzles
        if regHistorico:
            num = regHistorico["PUZZLES"]
        else:
            num = tactica.PUZZLES
        if not num or num > total:
            num = total

        lbPuzzles = Controles.LB(self, _("Max number of puzzles in each block") + ": ")
        self.sbPuzzles = Controles.SB(self, num, 1, total)

        # Reference
        lbReference = Controles.LB(self, _("Reference") + ": ")
        self.edReference = Controles.ED(self)

        # Iconos
        icoMas = Iconos.Add()
        icoMenos = Iconos.Delete()
        icoCancel = Iconos.CancelarPeque()
        icoReset = Iconos.MoverAtras()

        def tbGen(prev):
            liAcciones = ((_("Add"), icoMas, "%s_add" % prev),
                          (_("Delete"), icoMenos, "%s_delete" % prev), None,
                          (_("Delete all"), icoCancel, "%s_delete_all" % prev), None,
                          (_("Reset"), icoReset, "%s_reset" % prev), None,
                          )
            tb = Controles.TB(self, liAcciones, tamIcon=16, siTexto=False)
            return tb

        f = Controles.TipoLetra(peso=75)

        # Repeticiones de cada puzzle
        if regHistorico:
            self.liJUMPS = regHistorico["JUMPS"][:]
        else:
            self.liJUMPS = tactica.JUMPS[:]
        tb = tbGen("jumps")
        oCol = Columnas.ListaColumnas()
        oCol.nueva("NUMERO", _("Repetition"), 80, siCentrado=True)
        oCol.nueva("JUMPS_SEPARATION", _("Separation"), 80, siCentrado=True,
                   edicion=Delegados.LineaTexto(siEntero=True))
        self.grid_jumps = Grid.Grid(self, oCol, siSelecFilas=True, siEditable=True, xid="j")
        self.grid_jumps.setMinimumWidth(self.grid_jumps.anchoColumnas() + 20)
        ly = Colocacion.V().control(tb).control(self.grid_jumps)
        gbJumps = Controles.GB(self, _("Repetitions of each puzzle"), ly).ponFuente(f)
        self.grid_jumps.gotop()

        # Repeticion del bloque
        if regHistorico:
            self.liREPEAT = regHistorico["REPEAT"][:]
        else:
            self.liREPEAT = tactica.REPEAT[:]
        tb = tbGen("repeat")
        oCol = Columnas.ListaColumnas()
        oCol.nueva("NUMERO", _("Block"), 40, siCentrado=True)
        self.liREPEATtxt = (_("Original"), _("Random"), _("Previous"))
        oCol.nueva("REPEAT_ORDER", _("Order"), 100, siCentrado=True, edicion=Delegados.ComboBox(self.liREPEATtxt))
        self.grid_repeat = Grid.Grid(self, oCol, siSelecFilas=True, siEditable=True, xid="r")
        self.grid_repeat.setMinimumWidth(self.grid_repeat.anchoColumnas() + 20)
        ly = Colocacion.V().control(tb).control(self.grid_repeat)
        gbRepeat = Controles.GB(self, _("Blocks"), ly).ponFuente(f)
        self.grid_repeat.gotop()

        # Penalizaciones
        if regHistorico:
            self.liPENAL = regHistorico["PENALIZATION"][:]
        else:
            self.liPENAL = tactica.PENALIZATION[:]
        tb = tbGen("penal")
        oCol = Columnas.ListaColumnas()
        oCol.nueva("NUMERO", _("N."), 20, siCentrado=True)
        oCol.nueva("PENAL_POSITIONS", _("Positions"), 100, siCentrado=True, edicion=Delegados.LineaTexto(siEntero=True))
        oCol.nueva("PENAL_%", _("Affected"), 100, siCentrado=True)
        self.grid_penal = Grid.Grid(self, oCol, siSelecFilas=True, siEditable=True, xid="p")
        self.grid_penal.setMinimumWidth(self.grid_penal.anchoColumnas() + 20)
        ly = Colocacion.V().control(tb).control(self.grid_penal)
        gbPenal = Controles.GB(self, _("Penalties"), ly).ponFuente(f)
        self.grid_penal.gotop()

        # ShowText
        if regHistorico:
            self.liSHOWTEXT = regHistorico["SHOWTEXT"][:]
        else:
            self.liSHOWTEXT = tactica.SHOWTEXT[:]
        tb = tbGen("show")
        oCol = Columnas.ListaColumnas()
        self.liSHOWTEXTtxt = (_("No"), _("Yes"))
        oCol.nueva("NUMERO", _("N."), 20, siCentrado=True)
        oCol.nueva("SHOW_VISIBLE", _("Visible"), 100, siCentrado=True, edicion=Delegados.ComboBox(self.liSHOWTEXTtxt))
        oCol.nueva("SHOW_%", _("Affected"), 100, siCentrado=True)
        self.grid_show = Grid.Grid(self, oCol, siSelecFilas=True, siEditable=True, xid="s")
        self.grid_show.setMinimumWidth(self.grid_show.anchoColumnas() + 20)
        ly = Colocacion.V().control(tb).control(self.grid_show)
        gbShow = Controles.GB(self, _("Show text associated with each puzzle"), ly).ponFuente(f)
        self.grid_show.gotop()

        # Files
        if regHistorico:
            self.liFILES = regHistorico["FILESW"][:]
        else:
            self.liFILES = []
            for num, (fich, w, d, h) in enumerate(tactica.filesw):
                if not d or d < 1:
                    d = 1
                if not h or h > self.liFTOTAL[num] or h < 1:
                    h = self.liFTOTAL[num]
                if d > h:
                    d, h = h, d
                self.liFILES.append([fich, w, d, h])
        oCol = Columnas.ListaColumnas()
        oCol.nueva("FILE", _("File"), 220, siCentrado=True)
        oCol.nueva("WEIGHT", _("Weight"), 100, siCentrado=True, edicion=Delegados.LineaTexto(siEntero=True))
        oCol.nueva("TOTAL", _("Total"), 100, siCentrado=True)
        oCol.nueva("FROM", _("From"), 100, siCentrado=True, edicion=Delegados.LineaTexto(siEntero=True))
        oCol.nueva("TO", _("To"), 100, siCentrado=True, edicion=Delegados.LineaTexto(siEntero=True))
        self.grid_files = Grid.Grid(self, oCol, siSelecFilas=True, siEditable=True, xid="f")
        self.grid_files.setMinimumWidth(self.grid_files.anchoColumnas() + 20)
        ly = Colocacion.V().control(self.grid_files)
        gbFiles = Controles.GB(self, _("FNS files"), ly).ponFuente(f)
        self.grid_files.gotop()

        # Layout
        lyReference = Colocacion.H().control(lbReference).control(self.edReference)
        lyPuzzles = Colocacion.H().control(lbPuzzles).control(self.sbPuzzles)
        ly = Colocacion.G()
        ly.otro(lyPuzzles, 0, 0).otro(lyReference, 0, 1)
        ly.filaVacia(1, 5)
        ly.controld(gbJumps, 2, 0).control(gbPenal, 2, 1)
        ly.filaVacia(3, 5)
        ly.controld(gbRepeat, 4, 0)
        ly.control(gbShow, 4, 1)
        ly.filaVacia(5, 5)
        ly.control(gbFiles, 6, 0, 1, 2)

        layout = Colocacion.V().espacio(10).otro(ly)

        self.setLayout(layout)

        self.grid_repeat.gotop()

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def gridNumDatos(self, grid):
        xid = grid.id
        if xid == "j":
            return len(self.liJUMPS)
        if xid == "r":
            return len(self.liREPEAT)
        if xid == "p":
            return len(self.liPENAL)
        if xid == "s":
            return len(self.liSHOWTEXT)
        if xid == "f":
            return len(self.liFILES)

    def etiPorc(self, fila, numFilas):
        if numFilas == 0:
            return "100%"
        p = 100.0 / numFilas
        de = p * fila
        a = p * (fila + 1)
        return "%d%%  -  %d%%" % (int(de), int(a))

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "NUMERO":
            return str(fila + 1)
        if col == "JUMPS_SEPARATION":
            return str(self.liJUMPS[fila])
        elif col == "REPEAT_ORDER":
            n = self.liREPEAT[fila]
            if fila == 0:
                if n == 2:
                    self.liREPEAT[0] = 0
                    n = 0
            return self.liREPEATtxt[n]
        elif col == "PENAL_POSITIONS":
            return str(self.liPENAL[fila])
        elif col == "PENAL_%":
            return self.etiPorc(fila, len(self.liPENAL))
        elif col == "SHOW_VISIBLE":
            n = self.liSHOWTEXT[fila]
            return self.liSHOWTEXTtxt[n]
        elif col == "SHOW_%":
            return self.etiPorc(fila, len(self.liSHOWTEXT))
        elif col == "FILE":
            return self.liFILES[fila][0]
        elif col == "WEIGHT":
            return str(self.liFILES[fila][1])
        elif col == "TOTAL":
            return str(self.liFTOTAL[fila])
        elif col == "FROM":
            return str(self.liFILES[fila][2])
        elif col == "TO":
            return str(self.liFILES[fila][3])

    def gridPonValor(self, grid, fila, oColumna, valor):
        xid = grid.id
        if xid == "j":
            self.liJUMPS[fila] = int(valor)
        elif xid == "r":
            self.liREPEAT[fila] = self.liREPEATtxt.index(valor)
        elif xid == "p":
            self.liPENAL[fila] = int(valor)
        elif xid == "s":
            self.liSHOWTEXT[fila] = self.liSHOWTEXTtxt.index(valor)
        elif xid == "f":
            col = oColumna.clave
            n = int(valor)
            if col == "WEIGHT":
                if n > 0:
                    self.liFILES[fila][1] = n
            elif 0 < n <= self.liFTOTAL[fila]:
                if col == "FROM":
                    if n <= self.liFILES[fila][3]:
                        self.liFILES[fila][2] = n
                elif col == "TO":
                    if n >= self.liFILES[fila][2]:
                        self.liFILES[fila][3] = n

    def resultado(self):

        tactica = self.tacticaINI
        tactica.PUZZLES = int(self.sbPuzzles.valor())
        tactica.REFERENCE = self.edReference.texto().strip()
        tactica.JUMPS = self.liJUMPS
        tactica.REPEAT = self.liREPEAT
        tactica.PENALIZATION = self.liPENAL
        tactica.SHOWTEXT = self.liSHOWTEXT
        tactica.filesw = self.liFILES

        return tactica

    def jumps_add(self):
        n = len(self.liJUMPS)
        if n == 0:
            x = 3
        else:
            x = self.liJUMPS[-1] * 2
        self.liJUMPS.append(x)
        self.grid_jumps.refresh()
        self.grid_jumps.goto(n, 0)

    def jumps_delete(self):
        x = self.grid_jumps.recno()
        if x >= 0:
            del self.liJUMPS[x]
            self.grid_jumps.refresh()
            n = len(self.liJUMPS)
            if n:
                self.grid_jumps.goto(x if x < n else n - 1, 0)
                self.grid_jumps.refresh()

    def jumps_delete_all(self):
        self.liJUMPS = []
        self.grid_jumps.refresh()

    def jumps_reset(self):
        self.liJUMPS = self.tacticaINI.JUMPS[:]
        self.grid_jumps.gotop()
        self.grid_jumps.refresh()

    def repeat_add(self):
        n = len(self.liREPEAT)
        self.liREPEAT.append(0)
        self.grid_repeat.goto(n, 0)

    def repeat_delete(self):
        x = self.grid_repeat.recno()
        n = len(self.liREPEAT)
        if x >= 0 and n > 1:
            del self.liREPEAT[x]
            self.grid_repeat.refresh()
            x = x if x < n else n - 1
            self.grid_repeat.goto(x, 0)
            self.grid_repeat.refresh()

    def repeat_delete_all(self):
        self.liREPEAT = [0, ]
        self.grid_repeat.refresh()

    def repeat_reset(self):
        self.liREPEAT = self.tacticaINI.REPEAT[:]
        self.grid_repeat.gotop()
        self.grid_repeat.refresh()

    def penal_add(self):
        n = len(self.liPENAL)
        if n == 0:
            x = 1
        else:
            x = self.liPENAL[-1] + 1
        self.liPENAL.append(x)
        self.grid_penal.refresh()
        self.grid_penal.goto(n, 0)

    def penal_delete(self):
        x = self.grid_penal.recno()
        if x >= 0:
            del self.liPENAL[x]
            self.grid_penal.refresh()
            n = len(self.liPENAL)
            if n:
                self.grid_penal.goto(x if x < n else n - 1, 0)
                self.grid_penal.refresh()

    def penal_delete_all(self):
        self.liPENAL = []
        self.grid_penal.refresh()

    def penal_reset(self):
        self.liPENAL = self.tacticaINI.PENALIZATION[:]
        self.grid_penal.gotop()
        self.grid_penal.refresh()

    def show_add(self):
        n = len(self.liSHOWTEXT)
        self.liSHOWTEXT.append(1)
        self.grid_show.goto(n, 0)

    def show_delete(self):
        x = self.grid_show.recno()
        n = len(self.liSHOWTEXT)
        if x >= 0 and n > 1:
            del self.liSHOWTEXT[x]
            self.grid_show.refresh()
            x = x if x < n else n - 1
            self.grid_show.goto(x, 0)
            self.grid_show.refresh()

    def show_delete_all(self):
        self.liSHOWTEXT = [1, ]
        self.grid_show.refresh()

    def show_reset(self):
        self.liSHOWTEXT = self.tacticaINI.SHOWTEXT[:]
        self.grid_show.gotop()
        self.grid_show.refresh()


class WEditaTactica(QTVarios.WDialogo):
    def __init__(self, owner, tactica, ncopia):

        QTVarios.WDialogo.__init__(self, owner, _X(_("Configuration of %1"), tactica.titulo), Iconos.Tacticas(),
                                   "editaTactica")

        self.tactica = tactica

        liAcciones = ((_("Accept"), Iconos.Aceptar(), "aceptar"), None,
                      (_("Cancel"), Iconos.Cancelar(), "cancelar"), None,
                      (_("Help"), Iconos.AyudaGR(), "ayuda"), None,
                      )
        tb = Controles.TB(self, liAcciones)

        self.wtactic = WConfTactics(self, tactica, ncopia)

        layout = Colocacion.V().control(tb).control(self.wtactic)
        self.setLayout(layout)
        # self.recuperarVideo()

    def procesarTB(self):
        self.guardarVideo()
        accion = self.sender().clave
        if accion == "aceptar":
            self.accept()

        elif accion == "cancelar":
            self.reject()

        elif accion == "ayuda":
            self.ayuda()

    def ayuda(self):
        menu = QTVarios.LCMenu(self)

        nico = QTVarios.rondoColores()

        for opcion, txt in (
                (self.borraJUMPS, _("Without repetitions of each puzzle")),
                (self.borraREPEAT, _("Without repetitions of block")),
                (self.borraPENALIZATION, _("Without penalties")),
        ):
            menu.opcion(opcion, txt, nico.otro())
            menu.separador()

        resp = menu.lanza()
        if resp:
            resp()

    def borraJUMPS(self):
        self.wtactic.jumps_delete_all()

    def borraREPEAT(self):
        self.wtactic.repeat_delete_all()

    def borraPENALIZATION(self):
        self.wtactic.penal_delete_all()


def edita1tactica(owner, tactica, ncopia):
    w = WEditaTactica(owner, tactica, ncopia)
    if w.exec_():
        tresp = w.wtactic.resultado()

        tactica.PUZZLES = tresp.PUZZLES
        tactica.JUMPS = tresp.JUMPS
        tactica.REPEAT = tresp.REPEAT
        tactica.PENALIZATION = tresp.PENALIZATION
        tactica.SHOWTEXT = tresp.SHOWTEXT

        return True
    else:
        return False
