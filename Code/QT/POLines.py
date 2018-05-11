import os.path
import copy

from PyQt4 import QtCore, QtGui

from Code import Util
from Code import Partida
from Code import Analisis
from Code import OpeningLines
from Code import Books
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
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
from Code.QT import FormLayout


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
        oColumnas.nueva("NUMLINES", _("Lines"), 80, siCentrado=True)
        oColumnas.nueva("FILE", _("File"), 200)
        self.glista = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)

        sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Edit"), Iconos.Modificar(), self.modificar), None,
            (_("New"), Iconos.Nuevo(), self.new), None,
            (_("Rename"), Iconos.Modificar(), self.renombrar), None,
            (_("Up"), Iconos.Arriba(), self.arriba),
            (_("Down"), Iconos.Abajo(), self.abajo), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Reinit"), Iconos.Reiniciar(), self.reiniciar), None,
        )
        tb = Controles.TBrutina(self, liAcciones)

        tb.setSizePolicy(sp)

        liAcciones = (
            (_("Sequential"), Iconos.TrainSequential(), self.tr_sequential), None,
            (_("Static"), Iconos.TrainStatic(), self.tr_static), None,
            (_("Positions"), Iconos.TrainPositions(), self.tr_positions),
        )
        tbtrain = Controles.TBrutina(self, liAcciones, siTexto=False)

        lbtrain = Controles.LB(self, _("Trainings")).alinCentrado().ponFondoN("lightgray")
        lytrain = Colocacion.V().control(lbtrain).control(tbtrain).margen(0)
        self.wtrain = QtGui.QWidget()
        self.wtrain.setLayout(lytrain)

        lytb = Colocacion.H().control(tb).control(self.wtrain).margen(0)
        wtb = QtGui.QWidget()
        wtb.setFixedHeight(62)
        wtb.setLayout(lytb)

        # Colocamos

        ly = Colocacion.V().control(wtb).control(self.glista).margen(4)

        self.setLayout(ly)

        self.registrarGrid(self.glista)
        self.recuperarVideo()

        self.wtrain.setVisible(False)
        self.glista.gotop()

    def tr_(self, tipo):
        recno = self.glista.recno()
        op = self.listaOpenings[recno]
        op["TRAIN"] = tipo
        self.resultado = op
        self.guardarVideo()
        self.accept()

    def tr_sequential(self):
        self.tr_("sequential")

    def tr_static(self):
        self.tr_("static")

    def tr_positions(self):
        self.tr_("positions")

    def reiniciar(self):
        self.listaOpenings.reiniciar()
        self.glista.refresh()
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

        name = self.get_nombre(name)
        if name:
            file = self.listaOpenings.select_filename(name)
            self.listaOpenings.new(file, pv, name)
            self.resultado = self.listaOpenings[-1]
            self.guardarVideo()
            self.accept()

    def get_nombre(self, name):
        liGen = [(None, None)]
        liGen.append((_("Opening studio name") + ":", name))
        resultado = FormLayout.fedit(liGen, title=_("Opening studio name"), parent=self, icon=Iconos.OpeningLines(), anchoMinimo=460)
        if resultado:
            accion, liResp = resultado
            name = liResp[0].strip()
            if name:
                return name
        return None

    def renombrar(self):
        fila = self.glista.recno()
        if fila >= 0:
            op = self.listaOpenings[fila]
            name = self.get_nombre(op["title"])
            if name:
                self.listaOpenings.change_title(fila, name)
                self.glista.refresh()

    def borrar(self):
        li = self.glista.recnosSeleccionados()
        if len(li) > 0:
            mens = _("Do you want to delete all selected records?")
            mens += "\n"
            for num, fila in enumerate(li, 1):
                mens += "\n%d. %s" % (num, self.listaOpenings[fila]["title"])
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
            return op["title"]
        elif col == "FILE":
            return op["file"]
        elif col == "NUMLINES":
            return op["lines"]
        elif col == "BASEPV":
            pv = op["pv"]
            if pv:
                p = Partida.Partida()
                p.leerPV(pv)
                return p.pgnBaseRAW()
            else:
                return ""

    def gridCambiadoRegistro(self, grid, fila, columna):
        ok = False
        if fila >= 0:
            op = self.listaOpenings[fila]
            ok = op["withtrainings"]

        self.wtrain.setVisible(ok)

    def closeEvent(self, event):  # Cierre con X
        self.guardarVideo()

    def terminar(self):
        self.guardarVideo()
        self.reject()


class WLines(QTVarios.WDialogo):
    def __init__(self, procesador, dbop):
        self.dbop = dbop
        title = dbop.gettitle()

        QTVarios.WDialogo.__init__(self, procesador.pantalla, title, Iconos.OpeningLines(), "studyOpening")

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.partidabase = self.dbop.getpartidabase()
        self.num_jg_inicial = self.partidabase.numJugadas()
        self.num_jg_actual = None
        self.partida = None

        self.resultado = None
        siFigurinesPGN = self.configuracion.figurinesPGN

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Import"), Iconos.Mezclar(), self.importar), None,
            (_("Utilities"), Iconos.Utilidades(), self.utilidades), None,
            (_("Train"), Iconos.Study(), self.train), None,
        )
        self.tb = Controles.TBrutina(self, liAcciones)

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

        self.tabsanalisis = POLAnalisis.TabsAnalisis(self, self.procesador, self.configuracion)

        splitter = QtGui.QSplitter(self)
        splitter.setOrientation(QtCore.Qt.Vertical)
        splitter.addWidget(self.glines)
        splitter.addWidget(self.tabsanalisis)

        sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        splitter.setSizePolicy(sp)

        self.registrarSplitter(splitter, "SPLITTER")

        lyLV = Colocacion.V().control(splitter)
        lyTB = Colocacion.V().control(self.tb).control(self.pboard)
        layout = Colocacion.H().otro(lyTB).otro(lyLV).margen(3)
        self.setLayout(layout)

        self.colorPar = QTUtil.qtColor("#DBDAD9")
        self.colorNon = QTUtil.qtColor("#F1EFE9")
        self.colorLine = QTUtil.qtColor("#CDCCCB")

        self.partida = self.partidabase

        self.pboard.MoverFinal()

        self.recuperarVideo()

    def utilidades(self):
        menu = QTVarios.LCMenu(self)
        submenu = menu.submenu(_("Analysis"), Iconos.Analizar())
        submenu.opcion(self.ta_massive, _("Mass analysis"), Iconos.Analizar())
        submenu.separador()
        submenu.opcion(self.ta_remove, _("Delete all previous analysis"), Iconos.Delete())
        menu.separador()
        resp = menu.lanza()
        if resp:
            resp()

    def ta_massive(self):
        dicVar = self.configuracion.leeVariables("MASSIVE_OLINES")

        liGen = [FormLayout.separador]

        config = FormLayout.Combobox(_("Engine"), self.configuracion.comboMotoresMultiPV10(4))
        liGen.append((config, dicVar.get("ENGINE", self.configuracion.tutor)))

        liGen.append((_("Duration of engine analysis (secs)") + ":", dicVar.get("SEGUNDOS", float(self.configuracion.tiempoTutor / 1000.0))))
        liDepths = [("--", 0)]
        for x in range(1, 51):
            liDepths.append((str(x), x))
        config = FormLayout.Combobox(_("Depth"), liDepths)
        liGen.append((config, dicVar.get("DEPTH", self.configuracion.depthTutor)))

        li = [(_("Maximum"), 0)]
        for x in (1, 3, 5, 10, 15, 20, 30, 40, 50, 75, 100, 150, 200):
            li.append((str(x), x))
        config = FormLayout.Combobox(_("Number of moves evaluated by engine(MultiPV)"), li)
        liGen.append((config, dicVar.get("MULTIPV", self.configuracion.tutorMultiPV)))

        liGen.append(FormLayout.separador)
        liGen.append((_("Redo any existing prior analyses (if they exist)") + ":", dicVar.get("REDO", False)))

        resultado = FormLayout.fedit(liGen, title=_("Mass analysis"), parent=self, anchoMinimo=460,
                                     icon=Iconos.Analizar())
        if resultado is None:
            return

        claveMotor, tiempo, depth, multiPV, redo = resultado[1]
        ms = int(tiempo * 1000)
        if ms == 0 and depth == 0:
            return

        dicVar["ENGINE"] = claveMotor
        dicVar["SEGUNDOS"] = tiempo
        dicVar["DEPTH"] = depth
        dicVar["MULTIPV"] = multiPV
        dicVar["REDO"] = redo
        self.configuracion.escVariables("MASSIVE_OLINES", dicVar)

        um = QTUtil2.unMomento(self)
        stFensM2 = self.dbop.getAllFen()
        if redo == False:
            liBorrar = []
            for fenM2 in stFensM2:
                dic = self.dbop.getfenvalue(fenM2)
                if "ANALISIS" in dic:
                    liBorrar.append(fenM2)
            for fenM2 in liBorrar:
                stFensM2.remove(fenM2)

        conf_engine = copy.deepcopy(self.configuracion.buscaMotor(claveMotor))
        conf_engine.actMultiPV(multiPV)
        xgestor = self.procesador.creaGestorMotor(conf_engine, ms, depth, True)

        um.final()

        mensaje = _("Move") + "  %d/" + str(len(stFensM2))
        tmpBP = QTUtil2.BarraProgreso(self, _("Mass analysis"), "", len(stFensM2))

        done = 0

        for n, fenM2 in enumerate(stFensM2, 1):

            if tmpBP.siCancelado():
                break

            tmpBP.inc()
            tmpBP.mensaje(mensaje % n)

            mrm = xgestor.analiza(fenM2 + " 0 1")
            dic = self.dbop.getfenvalue(fenM2)
            dic["ANALISIS"] = mrm
            self.dbop.setfenvalue(fenM2, dic)
            done += 1

        tmpBP.cerrar()

    def ta_remove(self):
        if QTUtil2.pregunta(self, _("Are you sure?")):
            total = len(self.dbop.db_fenvalues)
            mensaje = _("Move") + "  %d/" + str(total)
            tmpBP = QTUtil2.BarraProgreso(self, "", "", total)
            self.dbop.removeAnalisis(tmpBP, mensaje)
            tmpBP.cerrar()
            self.glines.refresh()


    def train(self):
        if self.train_test():
            menu = QTVarios.LCMenu(self)
            menu.opcion("tr_sequential", _("Sequential"), Iconos.TrainSequential())
            menu.separador()
            menu.opcion("tr_static", _("Static"), Iconos.TrainStatic())
            menu.separador()
            menu.opcion("tr_positions", _("Positions"), Iconos.TrainPositions())
            menu.separador()
            submenu = menu.submenu(_("Configuration"), Iconos.Configurar())
            submenu.opcion("update", _("Update current trainings"), Iconos.Reindexar())
            submenu.separador()
            submenu.opcion("new", _("Re-create all trainings"), Iconos.Modificar())
            resp = menu.lanza()
            if resp is None:
                return
            if resp.startswith("tr_"):
                self.resultado = resp
                self.accept()
            elif resp == "new":
                self.trainNew()
            elif resp == "update":
                self.trainUpdate()

    def train_test(self):
        if len(self.dbop) == 0:
            return False
        training = self.dbop.training()
        if training is None:
            return self.trainNew()
        return True

    def trainNew(self):
        training = self.dbop.training()
        if training is None:
            color = "WHITE"
            random_order = False
            max_moves = 0
        else:
            color = training["COLOR"]
            random_order = training["RANDOM"]
            max_moves = training["MAXMOVES"]

        liGen = [(None, None)]

        liJ = [(_("White"), "WHITE"), (_("Black"), "BLACK")]
        config = FormLayout.Combobox(_("Play with"), liJ)
        liGen.append((config, color))

        liGen.append((None, None))
        liGen.append((_("Random order"), random_order))

        liGen.append((None, None))
        liGen.append((_("Maximum number of moves (0=all)"), max_moves))

        resultado = FormLayout.fedit(liGen, title=_("New training"), parent=self, anchoMinimo=360, icon=Iconos.Study())
        if resultado is None:
            return

        accion, liResp = resultado

        reg = {}

        reg["COLOR"], reg["RANDOM"], reg["MAXMOVES"] = liResp

        self.dbop.createTraining(reg, self.procesador)

        QTUtil2.mensaje(self, _("The trainings of this opening has been created"))

    def trainUpdate(self):
        self.dbop.updateTraining()
        QTUtil2.mensaje(self, _("The trainings have been updated"))

    def addPartida(self, partida):
        if partida.pv().startswith(self.partidabase.pv()):
            siNueva, num_linea, siAppend = self.dbop.posPartida(partida)
            if siNueva:
                self.dbop.append(partida)
            else:
                if siAppend:
                    self.dbop[num_linea] = partida
            self.glines.refresh()
        else:
            QTUtil2.mensError(self, _X("New line must begin with %1", self.partidabase.pgnSP()))

    def partidaActual(self):
        partida = Partida.Partida()
        numcol = self.glines.posActualN()[1]
        partida.leeOtra(self.partida if self.partida and numcol> 0 else self.partidabase)
        if self.num_jg_actual is not None \
            and self.num_jg_inicial <= self.num_jg_actual < len(partida):
            partida.liJugadas = partida.liJugadas[:self.num_jg_actual+1]
            partida.ultPosicion = partida.jugada(-1).posicion
        return partida

    def voyager2(self, partida):
        ptxt = Voyager.voyagerPartida(self, partida)
        if ptxt:
            partida = Partida.Partida()
            partida.recuperaDeTexto(ptxt)
            self.addPartida(partida)

    def importar(self):
        menu = QTVarios.LCMenu(self)

        def haz_menu(frommenu, part):
            liOp = self.dbop.getOtras(self.configuracion, part)
            if liOp:
                otra = frommenu.submenu(_("Other opening lines"), Iconos.OpeningLines())
                for fichero, titulo in liOp:
                    otra.opcion(("ol", (fichero, part)), titulo, Iconos.PuntoVerde())
                frommenu.separador()
            frommenu.opcion(("pgn", part), _("PGN with variants"), Iconos.Tablero())
            frommenu.separador()
            frommenu.opcion(("polyglot", part), _("Polyglot book"), Iconos.Libros())
            frommenu.separador()
            frommenu.opcion(("summary", part), _("Database summary"), Iconos.DatabaseC())
            frommenu.separador()
            frommenu.opcion(("voyager2", part), _("Voyager 2"), Iconos.Voyager1())
            frommenu.separador()
            frommenu.opcion(("opening", part), _("Opening"), Iconos.Apertura())

        partida = self.partidaActual()
        if len(partida) > len(self.partidabase):
            sub1 = menu.submenu(_("From current position"), Iconos.MoverLibre())
            haz_menu(sub1, partida)
            menu.separador()
            sub2 = menu.submenu(_("From base position"), Iconos.MoverInicio())
            haz_menu(sub2, self.partidabase)
        else:
            haz_menu(menu, self.partidabase)

        resp = menu.lanza()
        if resp is None:
            return
        tipo, partida = resp
        if tipo == "pgn" :
            self.importarPGN(partida)
        elif tipo == "polyglot":
            self.importarPolyglot(partida)
        elif tipo == "summary":
            self.importarSummary(partida)
        elif tipo == "voyager2":
            self.voyager2(partida)
        elif tipo == "opening":
            self.importarApertura(partida)
        elif tipo == "ol":
            fichero, partida = partida
            self.importarOtra(fichero, partida)

    def importarOtra(self, fichero, partida):
        um = QTUtil2.unMomento(self)
        pathFichero = os.path.join(self.configuracion.folderOpenings, fichero)
        self.dbop.importarOtra(pathFichero, partida)
        um.final()
        self.glines.refresh()
        self.glines.gotop()

    def importarApertura(self, partida):
        partida.asignaApertura()
        w = PantallaAperturas.WAperturas(self, self.configuracion, partida.apertura)
        if w.exec_():
            ap = w.resultado()
            partida = Partida.Partida()
            partida.leerPV(ap.a1h8)
            self.addPartida(partida)

    def importarLeeParam(self, titulo, dicData):
        liGen = [FormLayout.separador]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))
        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 99, 50), dicData.get("DEPTH", 30)))
        liGen.append(FormLayout.separador)

        li = [(_("Only white best moves"), True), (_("Only black best moves"), False)]
        config = FormLayout.Combobox(_("Moves"), li)
        liGen.append((config, dicData.get("SIWHITE", True)))
        liGen.append(FormLayout.separador)

        liGen.append((FormLayout.Spinbox(_("Minimum moves must have each line"), 0, 99, 50), dicData.get("MINMOVES", 0)))

        resultado = FormLayout.fedit(liGen, title=titulo, parent=self, anchoMinimo=360, icon=Iconos.PuntoNaranja())
        if resultado:
            accion, liResp = resultado
            depth, siWhite, minMoves = liResp
            dicData["DEPTH"] = depth
            dicData["SIWHITE"] = siWhite
            dicData["MINMOVES"] = minMoves
            self.configuracion.escVariables("WBG_MOVES", dicData)
            return dicData
        return None

    def importarSummary(self, partida):
        nomfichgames = QTVarios.selectDB(self, self.configuracion, False, True)
        if nomfichgames:
            previo = self.configuracion.leeVariables("OPENINGLINES")
            dicData = self.importarLeeParam(_("Database summary"), previo)
            if dicData:
                ficheroSummary = nomfichgames + "_s1"
                depth, siWhite, minMoves = dicData["DEPTH"], dicData["SIWHITE"], dicData["MINMOVES"]
                self.dbop.importarSummary(self, partida, ficheroSummary, depth, siWhite, minMoves)
                self.glines.refresh()
                self.glines.gotop()

    def importarPolyglot(self, partida):
        listaLibros = Books.ListaLibros()
        listaLibros.recuperaVar(self.configuracion.ficheroBooks)
        listaLibros.comprueba()

        liGen = [FormLayout.separador]

        li = [(book.nombre, book) for book in listaLibros.lista]
        config = FormLayout.Combobox(_("Book that plays white side"), li)
        liGen.append((config, listaLibros.lista[0]))
        liGen.append(FormLayout.separador)
        config = FormLayout.Combobox(_("Book that plays black side"), li)
        liGen.append((config, listaLibros.lista[0]))
        liGen.append(FormLayout.separador)

        resultado = FormLayout.fedit(liGen, title=_("Polyglot book"), parent=self, anchoMinimo=360, icon=Iconos.Libros())
        if resultado:
            accion, liResp = resultado
            bookW, bookB = liResp
        else:
            return

        bookW.polyglot()
        bookB.polyglot()

        titulo = bookW.nombre if bookW==bookB else "%s/%s" % (bookW.nombre, bookB.nombre)
        dicData = self.configuracion.leeVariables("OPENINGLINES")
        dicData = self.importarLeeParam(titulo, dicData)
        if dicData:
            depth, siWhite, minMoves = dicData["DEPTH"], dicData["SIWHITE"], dicData["MINMOVES"]
            self.dbop.importarPolyglot(self, partida, bookW, bookB, titulo, depth, siWhite, minMoves)
            self.glines.refresh()
            self.glines.gotop()

    def importarPGN(self, partida):
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

            self.dbop.importarPGN(self, partida, ficheroPGN, depth)
            self.glines.refresh()
            self.glines.gotop()

    def gridColorFondo(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "LINE":
            return self.colorLine
        else:
            linea = fila // 2
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
            njug = None
        self.num_jg_actual = njug
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
        if k in (QtCore.Qt.Key_Delete, QtCore.Qt.Key_Backspace):
            fila, col = self.glines.posActual()
            if col.clave == "LINE":
                self.borrar()
            else:
                self.borrar_move()

    def gridDobleClick(self, grid, fila, oColumna):
        partida = self.partidaActual()
        if partida:
            self.procesador.cambiaXAnalyzer()
            xanalyzer = self.procesador.xanalyzer
            jg = partida.jugada(-1)
            fenM2 = jg.posicionBase.fenM2()
            dic = self.dbop.getfenvalue(fenM2)
            if "ANALISIS" in dic:
                mrm = dic["ANALISIS"]
                jg.analisis = mrm, 0
            else:
                me = QTUtil2.mensEspera.inicio(self, _("Analyzing the move...."), posicion="ad")

                jg.analisis = xanalyzer.analizaJugadaPartida(partida, len(partida)-1, xanalyzer.motorTiempoJugada,
                                                               xanalyzer.motorProfundidad)
                me.final()
            Analisis.muestraAnalisis(self.procesador, xanalyzer, jg, self.pboard.tablero.siBlancasAbajo, 9999,
                                     len(partida)-1, pantalla=self)

            dic = self.dbop.getfenvalue(fenM2)
            dic["ANALISIS"] = jg.analisis[0]
            self.dbop.setfenvalue(fenM2, dic)

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

            siUltimo = njug == len(partida)-1  # si es el ultimo no se pregunta
            if siUltimo or QTUtil2.pregunta(self, _("Do you want to eliminate this move?")):
                partida.liJugadas = partida.liJugadas[:njug]
                partida.ultPosicion = partida.jugada(-1).posicion
                self.dbop[linea] = partida

                self.goto_finlinea()

    def borrar(self):
        tam_dbop = len(self.dbop)
        if tam_dbop == 0:
            return
        current = self.glines.recno()//2
        li = []
        if 0 <= current < tam_dbop:
            li.append(["current", _("Remove line %d") % (current+1,), Iconos.Mover()])
        if tam_dbop > 1:
            li.append(["lines", _("Remove a list of lines"), Iconos.MoverLibre()])

        if len(li) > 0:
            menu = QTVarios.LCMenu(self)
            for key, title, ico in li:
                menu.opcion(key, title, ico)
                menu.separador()
            resp = menu.lanza()

            if resp == "current":
                del self.dbop[current]
                self.goto_inilinea()

            else:
                liGen = [FormLayout.separador]
                config = FormLayout.Editbox("<div align=\"right\">" + _("Lines") + "<br>" +
                                            _("By example:") + " -5,8-12,14,19-",
                                            rx="[0-9,\-,\,]*")
                liGen.append((config, ""))
                resultado = FormLayout.fedit(liGen, title=_("Remove a list of lines"), parent=self, anchoMinimo=460, icon=Iconos.OpeningLines())
                if resultado:
                    accion, liResp = resultado
                    clista = liResp[0]
                    if clista:
                        ln = Util.ListaNumerosImpresion(clista)
                        li = ln.selected(range(1, tam_dbop+1))
                        sli = []
                        cad = ""
                        for num in li:
                            if cad:
                                cad += "," + str(num)
                            else:
                                cad = str(num)
                            if len(cad) > 80:
                                sli.append(cad)
                                cad = ""
                        if cad:
                            sli.append(cad)
                        cli = "\n".join(sli)
                        if QTUtil2.pregunta(self, _("Do you want to remove the next lines?") + "\n\n" + cli):
                            li.sort(reverse=True)
                            um = QTUtil2.unMomento(self, _("Working..."))
                            for num in li:
                                del self.dbop[num-1]
                            self.glines.refresh()
                            self.goto_inilinea()
                            um.final()

    def goto_inilinea(self):
        nlines = len(self.dbop)
        if nlines == 0:
            return

        linea = self.glines.recno() // 2
        if linea >= nlines:
            linea = nlines-1

        fila = linea*2
        ncol = 0
        self.glines.goto(fila, ncol)
        self.glines.refresh()

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
        self.tabsanalisis.saveConfig()
        self.guardarVideo()
        self.accept()

    def mueveHumano(self, partida):
        # Estamos en la misma linea ?
        # recno = self.glines.recno()
        # Buscamos en las lineas si hay alguna que el pv sea parcial o totalmente igual
        partida.pendienteApertura = True
        siNueva, num_linea, siAppend = self.dbop.posPartida(partida)
        siBlancas = partida.jugada(-1).siBlancas()
        ncol = (partida.numJugadas() - self.num_jg_inicial + 1) // 2
        if self.num_jg_inicial%2 == 1 and siBlancas:
             ncol += 1
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

        self.glines.refresh()
        self.glines.goto(fila, ncol)


class WStaticTraining(QTVarios.WDialogo):
    def __init__(self, procesador, dbop):
        self.training = dbop.training()
        self.ligames = self.training["LIGAMES_STATIC"]
        self.num_games = len(self.ligames)
        self.elems_fila = 10
        if self.num_games < self.elems_fila:
            self.elems_fila = self.num_games
        self.num_filas = (self.num_games-1) / self.elems_fila + 1
        self.seleccionado = None

        titulo = "%s - %s" % (_("Opening lines"), _("Static training"))

        extparam = "openlines_static_%s" % dbop.nomFichero

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, Iconos.TrainStatic(), extparam)

        lb = Controles.LB(self,  dbop.gettitle())
        lb.ponFondoN("#BDDBE8").alinCentrado().ponTipoLetra(puntos=14)

        # Toolbar
        tb = Controles.TBrutina(self)
        tb.new(_("Close"), Iconos.MainMenu(), self.terminar)

        # Lista
        ancho = 42
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("FILA", "", 36, siCentrado=True)
        for x in range(self.elems_fila):
            oColumnas.nueva("COL%d" % x, "%d" % (x+1,), ancho, siCentrado=True, edicion=Delegados.PmIconosWeather())

        self.grid = Grid.Grid(self, oColumnas, altoFila=ancho, background="white")
        self.grid.setAlternatingRowColors(False)
        self.grid.tipoLetra(puntos=10, peso=500)
        nAnchoPgn = self.grid.anchoColumnas() + 20
        self.grid.setMinimumWidth(nAnchoPgn)

        ly = Colocacion.V().control(lb).control(tb).control(self.grid)
        self.setLayout(ly)

        alto = self.num_filas*ancho + 146
        self.recuperarVideo(siTam=True, altoDefecto=alto, anchoDefecto=nAnchoPgn)

    def terminar(self):

        self.guardarVideo()
        self.reject()

    def gridNumDatos(self, grid):
        return self.num_filas

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "FILA":
            return "%d" % fila
        elif col.startswith("COL"):
            num = fila*self.elems_fila + int(col[3:])
            if num >= self.num_games:
                return None
            game = self.ligames[num]
            sinerror = game["NOERROR"]
            return str(sinerror) if sinerror < 4 else "4"

    def gridDobleClick(self, grid, fila, oColumna):
        col = oColumna.clave
        if col.startswith("COL"):
            num = fila*self.elems_fila + int(col[3:])
            if num >= self.num_games:
                return
            self.seleccionado = num
            self.guardarVideo()
            self.accept()


def selectLine(procesador, dbop):
    w = WStaticTraining(procesador, dbop)
    w.exec_()
    return w.seleccionado


def openingLines(procesador):
    w = WOpeningLines(procesador)
    return w.resultado if w.exec_() else None


def study(procesador, fichero):
    with QTUtil.EscondeWindow(procesador.pantalla):
        dbop = OpeningLines.Opening(os.path.join(procesador.configuracion.folderOpenings, fichero))
        w = WLines(procesador, dbop)
        w.exec_()
        dbop.close()
        return w.resultado

