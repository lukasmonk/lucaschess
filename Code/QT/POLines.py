import os.path

from PyQt4 import QtCore

from Code import Partida
from Code import OpeningLines
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaAperturas
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Delegados
from Code.QT import POLBoard
from Code.QT import POLAnalisis
from Code.QT import Voyager


class WOpeningLines(QTVarios.WDialogo):
    def __init__(self, procesador):

        QTVarios.WDialogo.__init__(self, procesador.pantalla,  _("Opening lines"), Iconos.OpeningLines(), "openingLines")

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.resultado = None

        self.listaOpenings = OpeningLines.ListaOpenings(self.configuracion)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("TITLE", _("Name"), 240)
        oColumnas.nueva("BASEPV", _("First moves"), 280)
        oColumnas.nueva("FILE", _("File"), 200)
        self.glista = Grid.Grid(self, oColumnas, siSelecFilas=True)
        self.glista.setMinimumWidth(self.glista.anchoColumnas() + 20)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Edit"), Iconos.Modificar(), self.modificar), None,
            (_("New"), Iconos.Nuevo(), self.new), None,
            (_("Up"), Iconos.Arriba(), self.arriba),
            (_("Down"), Iconos.Abajo(), self.abajo), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        tb = Controles.TBrutina(self, liAcciones)

        # Colocamos
        ly = Colocacion.V().control(tb).control(self.glista).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.glista)
        self.recuperarVideo()

        self.glista.gotop()

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
        while True:
            liGen = [(None, None)]
            liGen.append((_("Opening studio name") + ":", name))
            resultado = FormLayout.fedit(liGen, title=_("Opening studio name"), parent=self, icon=Iconos.OpeningLines(), anchoMinimo=460)
            if resultado:
                accion, liResp = resultado
                name = liResp[0]
                if name:
                    file = self.listaOpenings.select_filename(name)
                    break
                else:
                    return
            else:
                return

        self.listaOpenings.new(file, pv, name)
        self.glista.gobottom()
        self.glista.refresh()

    def borrar(self):
        li = self.glista.recnosSeleccionados()
        if len(li) > 0:
            mens = _("Do you want to delete all selected records?")
            mens += "\n"
            for num, fila in enumerate(li, 1):
                mens += "\n%d. %s" % (num, self.listaOpenings[fila][2])
            if QTUtil2.pregunta(self, mens):
                li.sort(reverse=True)
                for fila in li:
                    del self.listaOpenings[fila]
                self.glista.refresh()

    def gridNumDatos(self, grid):
        return len(self.listaOpenings)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        op = self.listaOpenings[fila]
        if col == "TITLE":
            return op[2]
        elif col == "FILE":
            return op[0]
        elif col == "BASEPV":
            pv = op[1]
            if pv:
                p = Partida.Partida()
                p.leerPV(op[1])
                return p.pgnBaseRAW()
            else:
                return ""

    def gridPonValor(self, grid, fila, columna, valor):
        valor = valor.strip()
        if valor:
            self.listaOpenings.change_title(fila, valor)

    def closeEvent(self, event):  # Cierre con X
        self.guardarVideo()

    def terminar(self):
        self.guardarVideo()
        self.reject()


class WStudy(QTVarios.WDialogo):
    def __init__(self, procesador, dbop):

        self.dbop = dbop
        title = dbop.gettitle()

        QTVarios.WDialogo.__init__(self, procesador.pantalla, title, Iconos.Study(), "studyOpening")

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.partidabase = self.dbop.getpartidabase()
        self.num_jg_inicial = self.partidabase.numJugadas()
        self.partida = None
        siFigurinesPGN = self.configuracion.figurinesPGN

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            # (_("New"), Iconos.Nuevo(), self.new), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            # (_("Train"), Iconos.Study(), self.train), None,
            (_("Voyager 2"), Iconos.Voyager1(), self.voyager2), None,
            (_("Import"), Iconos.Mezclar(), self.importar), None
        )
        self.tb = tb = Controles.TBrutina(self, liAcciones)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("LINE", _("Line"), 35, edicion=Delegados.EtiquetaPOS(False, True))
        inicio = self.partidabase.numJugadas()/2+1
        ancho_col = ((self.configuracion.anchoPGN - 35 - 20) / 2)*80//100
        for x in range(inicio, 75):
            oColumnas.nueva(str(x), str(x), ancho_col, edicion=Delegados.EtiquetaPOS(siFigurinesPGN, True))
        self.glines = Grid.Grid(self, oColumnas, siCabeceraMovible=False)
        self.glines.setAlternatingRowColors(False)
        self.glines.tipoLetra(puntos=self.configuracion.puntosPGN)
        self.glines.ponAltoFila(self.configuracion.altoFilaPGN)

        self.pboard = POLBoard.BoardLines(self, self.configuracion)
        self.pboard.ponPartidaBase(self.partidabase)
        self.pboard.ponPartida(self.partidabase)

        self.tabsanalisis = POLAnalisis.TabsAnalisis(self, self.procesador, self.configuracion)
        lyg = Colocacion.H().control(self.tabsanalisis).margen(3)
        self.gb_analysis = Controles.GB(self, _("Analysis"), lyg)
        self.gb_analysis.conectar(self.analisis)
        self.gb_analysis.setChecked(True)

        lyLV = Colocacion.V().control(self.glines).control(self.gb_analysis)
        ly1 = Colocacion.H().control(self.pboard).otro(lyLV)
        layout = Colocacion.V().control(tb).otro(ly1)
        self.setLayout(layout)

        self.colorPar = QTUtil.qtColor("#DBDAD9")
        self.colorNon = QTUtil.qtColor("#F1EFE9")
        self.colorLine = QTUtil.qtColor("#CDCCCB")

        self.analisis()
        self.partida = self.partidabase
        self.pboard.MoverFinal()

        self.recuperarVideo()

    def voyager2(self):
        partida = Partida.Partida()
        partida.leeOtra(self.partidabase)
        ptxt = Voyager.voyagerPartida(self.owner, partida)
        if ptxt:
            partida = Partida.Partida()
            partida.recuperaDeTexto(ptxt)
            if self.partidabase.pv() in partida.pv():
                siNueva, num_linea, siAppend = self.dbop.posPartida(partida)
                if siNueva:
                    self.dbop.append(partida)
                else:
                    if siAppend:
                        self.dbop[num_linea] = partida
                self.glines.refresh()
            else:
                QTUtil2.mensError(self, _X("New line must begin with %1", self.partidabase.pgnSP()))

    def importar(self):
        menu = QTVarios.LCMenu(self)

        liOp = self.dbop.getOtras(self.configuracion)
        if liOp:
            otra = menu.submenu(_("Other line openings"), Iconos.OpeningLines())
            for fichero, titulo in liOp:
                otra.opcion(("ol", fichero), titulo, Iconos.PuntoVerde())
            menu.separador()
        menu.opcion(("pgn", None), _("PGN with variants"), Iconos.Tablero())
        menu.separador()
        menu.opcion(("polyglot", None), _("Polyglot book"), Iconos.Libros())
        menu.separador()
        menu.opcion(("summary", None), _("Database summary"), Iconos.DatabaseC())
        menu.separador()
        resp = menu.lanza()
        if resp is None:
            return
        tipo, arg = resp
        if tipo == "pgn" :
            self.importarPGN()
        elif tipo == "polyglot":
            self.importarPolyglot()
        elif tipo == "summary":
            pass
        elif tipo == "ol":
            pass

    def importarPolyglot(self):
        previo = self.configuracion.leeVariables("WBG_MOVES")
        carpeta = previo.get("CARPETABIN", "")

        ficheroBIN = QTUtil2.leeFichero(self, carpeta, "%s (*.bin)" % _("Polyglot book"), titulo=_("File to import"))
        if not ficheroBIN:
            return
        previo["CARPETABIN"] = os.path.dirname(ficheroBIN)
        self.configuracion.escVariables("WBG_MOVES", previo)

        liGen = [(None, None)]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))

        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 99, 50), 30))
        liGen.append((None, None))

        liGen.append((_("Only white best moves"), False))
        liGen.append((None, None))

        liGen.append((_("Only black best moves"), False))
        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=os.path.basename(ficheroBIN), parent=self, anchoMinimo=360,
                                     icon=Iconos.PuntoNaranja())

        if resultado:
            accion, liResp = resultado
            depth, whiteBest, blackBest = liResp
            self.dbop.grabarPolyglot(self, ficheroBIN, depth, whiteBest, blackBest)
            self.glines.refresh()
            self.glines.gotop()

    def importarPGN(self):
        previo = self.configuracion.leeVariables("OPENINGLINES")
        carpeta = previo.get("CARPETAPGN", "")

        ficheroPGN = QTUtil2.leeFichero(self, carpeta, "%s (*.pgn)" % _("PGN Format"), titulo=_("File to import"))
        if not ficheroPGN:
            return
        previo["CARPETAPGN"] = os.path.dirname(ficheroPGN)
        self.configuracion.escVariables("OPENINGLINES", previo)

        liGen = [(None, None)]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))

        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 999, 50), 30))
        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=os.path.basename(ficheroPGN), parent=self, anchoMinimo=460,
                                     icon=Iconos.PuntoNaranja())

        if resultado:
            accion, liResp = resultado
            depth = liResp[0]

            self.dbop.grabarPGN(self, ficheroPGN, depth)
            self.glines.refresh()
            self.glines.gotop()

    def analisis(self):
        x = self.gb_analysis.isChecked()
        if not x:
            self.tabsanalisis.hide()
        else:
            self.tabsanalisis.show()

    def gridColorFondo(self, grid, fila, oColumna):
        col = oColumna.clave
        linea = fila//2
        if col == "LINE":
            return self.colorLine
        else:
            return self.colorPar if linea % 2 == 0 else self.colorNon

    def gridCambiadoRegistro(self, grid, fila, oColumna):
        col = oColumna.clave
        linea = fila//2
        self.partida = self.dbop[linea]
        iswhite = fila % 2 == 0
        if col.isdigit():
            njug = (int(col)-1)*2
            if not iswhite:
                njug += 1
        else:
            njug = self.partida.numJugadas()-1
        self.pboard.ponPartida(self.partida)
        self.pboard.colocatePartida(njug)
        self.glines.setFocus()

    def setJugada(self, njug):
        """Recibimos informacion del panel del tablero"""
        if njug >= 0:
            self.tabsanalisis.setPosicion(self.partida, njug)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        linea = fila//2
        iswhite = (fila%2) == 0
        color = None
        info = None
        indicadorInicial = None
        liNAGs = []
        siLine = False
        agrisar = False

        if col == "LINE":
            pgn = str(linea+1) if iswhite else ""
            siLine = True

        else:
            njug = (int(col)-1)*2
            if not iswhite:
                njug += 1
            partida = self.dbop[linea]
            if self.num_jg_inicial <= njug < partida.numJugadas():
                jg = partida.jugada(njug)
                pgn = jg.pgnFigurinesSP()
                if linea:
                    partida_ant = self.dbop[linea-1]
                    if partida_ant.pv_hasta(njug) == partida.pv_hasta(njug):
                        agrisar = True
                dic = self.dbop.getfenvalue(jg.posicion.fenM2())
                if dic:
                    if "COMENTARIO" in dic:
                        v = dic["COMENTARIO"]
                        if v:
                            indicadorInicial = "V"
                    if "VALORACION" in dic:
                        v = dic["VALORACION"]
                        if v:
                            liNAGs.append(str(v))
                    if "VENTAJA" in dic:
                        v = dic["VENTAJA"]
                        if v:
                            liNAGs.append(str(v))
            else:
                pgn = ""

        return pgn, iswhite, color, info, indicadorInicial, liNAGs, agrisar, siLine

    def gridNumDatos(self, grid):
        return len(self.dbop)*2

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        if k == QtCore.Qt.Key_Delete:
            self.borrar_move()

    def borrar_move(self):
        fila, col = self.glines.posActual()
        linea = fila // 2
        if 0 <= linea < len(self.dbop):
            partida = self.dbop[linea]
            njug = (int(col.clave) - 1) * 2
            if fila % 2 == 1:
                njug += 1
            if linea:
                partida_ant = self.dbop[linea-1]
                if partida_ant.pv_hasta(njug-1) == partida.pv_hasta(njug-1):
                    return self.borrar()
            if linea < len(self.dbop)-1:
                partida_sig = self.dbop[linea+1]
                if partida_sig.pv_hasta(njug-1) == partida.pv_hasta(njug-1):
                    return self.borrar()

            if njug == self.num_jg_inicial:
                return self.borrar()

            if QTUtil2.pregunta(self, _("Do you want to eliminate this move?")):
                partida.liJugadas = partida.liJugadas[:njug]
                partida.ultPosicion = partida.jugada(-1).posicion
                self.dbop[linea] = partida

                self.goto_finlinea()

    def borrar(self):
        fila = self.glines.recno()
        linea = fila//2
        if 0 <= linea < len(self.dbop):
            if QTUtil2.pregunta(self, _X("Do you want to remove line %1", str(linea+1))):
                del self.dbop[linea]
                self.goto_finlinea()

    def goto_finlinea(self):
        nlines = len(self.dbop)
        if nlines == 0:
            return

        linea = self.glines.recno() // 2
        if linea >= nlines:
            linea = nlines-1

        partida = self.dbop[linea]

        fila = linea*2
        njug = partida.numJugadas()
        if njug % 2 == 0:
            fila += 1

        ncol = njug//2
        if njug %2 == 1:
            ncol += 1

        ncol -= self.num_jg_inicial//2
        self.glines.goto(fila, ncol)
        self.glines.refresh()

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def mueveHumano(self, partida):
        # Estamos en la misma linea ?
        # recno = self.glines.recno()
        # Buscamos en las lineas si hay alguna que el pv sea parcial o totalmente igual
        siNueva, num_linea, siAppend = self.dbop.posPartida(partida)
        siBlancas = partida.jugada(-1).siBlancas()
        ncol = (partida.numJugadas() - self.num_jg_inicial) // 2 + 1
        if siNueva:
            self.dbop.append(partida)
        else:
            if siAppend:
                self.dbop[num_linea] = partida
        if not siAppend:
            siNueva, num_linea, siAppend = self.dbop.posPartida(partida)

        fila = num_linea*2
        if not siBlancas:
            fila += 1
        self.glines.goto(fila, ncol)

        self.glines.refresh()


def openingLines(procesador):
    w = WOpeningLines(procesador)
    return w.resultado if w.exec_() else None


def study(procesador, fichero):
    with QTUtil.EscondeWindow(procesador.pantalla):
        dbop = OpeningLines.Opening(os.path.join(procesador.configuracion.folderOpenings, fichero))
        w = WStudy(procesador, dbop)
        w.exec_()
        dbop.close()

