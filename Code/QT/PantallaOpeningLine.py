import os
import os.path
import copy

import LCEngine4 as LCEngine

from PyQt4 import QtCore, QtGui

from Code import Util
from Code import Partida
from Code import Analisis
from Code import OpeningLines
from Code import Books
from Code import EnginesBunch
from Code.QT import Colocacion
from Code.QT import Columnas
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
from Code.QT import PantallaSavePGN


class WLines(QTVarios.WDialogo):
    def __init__(self, procesador, dbop):
        self.dbop = dbop
        self.title = dbop.gettitle()

        QTVarios.WDialogo.__init__(self, procesador.pantalla, self.title, Iconos.OpeningLines(), "studyOpening")

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
            (_("Import"), Iconos.Import8(), self.importar), None,
            (_("Export"), Iconos.Export8(), self.exportar), None,
            (_("Utilities"), Iconos.Utilidades(), self.utilidades), None,
            (_("Train"), Iconos.Study(), self.train), None,
        )
        self.tb = QTVarios.LCTB(self, liAcciones)

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

        self.last_numlines = 0
        self.show_lines()

    def show_lines(self):
        numlines = len(self.dbop)
        if numlines != self.last_numlines:
            self.setWindowTitle( "%s [%d]" %(self.title, numlines))
            self.last_numlines = numlines

    def exportar(self):
        menu = QTVarios.LCMenu(self)
        submenu = menu.submenu(_("PGN Format"), Iconos.PGN())
        r = "%s %%s" % _("Result")
        submenu.opcion("1-0", r % "1-0", Iconos.Blancas8())
        submenu.opcion("0-1", r % "0-1", Iconos.Negras8())
        submenu.opcion("1/2-1/2", r % "1/2-1/2", Iconos.Tablas8())
        submenu.opcion("", _("Without Result"), Iconos.Gris())
        resp = menu.lanza()
        if resp is not None:
            w = PantallaSavePGN.WSaveVarios(self, self.configuracion)
            if w.exec_():
                ws = PantallaSavePGN.FileSavePGN(self, w.dic_result)
                if ws.open():
                    self.dbop.exportarPGN(ws, resp)
                    ws.close()
                    ws.um_final()

    def utilidades(self):
        menu = QTVarios.LCMenu(self)
        submenu = menu.submenu(_("Analysis"), Iconos.Analizar())
        submenu.opcion(self.ta_massive, _("Mass analysis"), Iconos.Analizar())
        submenu.separador()
        submenu.opcion(self.ta_remove, _("Delete all previous analysis"), Iconos.Delete())
        menu.separador()
        lihistory = self.dbop.lihistory()
        if lihistory:
            submenu = menu.submenu(_("Backups"), Iconos.Copiar())
            rondo = QTVarios.rondoPuntos()
            for history in lihistory[:30]:
                h = history
                if len(h) > 70:
                    h = h[:70] + "..."
                submenu.opcion(history, h, rondo.otro())
                submenu.separador()

        # submenu = menu.submenu(_("History of this session"), Iconos.Copiar())
        resp = menu.lanza()
        if resp:
            if isinstance(resp, (str, unicode)):
                if QTUtil2.pregunta(self, _("Are you sure you want to restore backup %s ?" % ("\n%s" % resp))):
                    um = QTUtil2.unMomento(self, _("Working..."))
                    self.dbop.rechistory(resp)
                    self.glines.refresh()
                    self.glines.gotop()
                    um.final()
            else:
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
        menu = QTVarios.LCMenu(self)
        trSSP, trEng = self.train_test()
        if trSSP:
            menu.opcion("tr_sequential", _("Sequential"), Iconos.TrainSequential())
            menu.separador()
            menu.opcion("tr_static", _("Static"), Iconos.TrainStatic())
            menu.separador()
            menu.opcion("tr_positions", _("Positions"), Iconos.TrainPositions())
            menu.separador()
        if trEng:
            menu.opcion("tr_engines", _("With engines"), Iconos.TrainEngines())
            menu.separador()
        submenu = menu.submenu(_("Configuration"), Iconos.Configurar())
        if trEng or trSSP:
            submenu.opcion("update", _("Update current trainings"), Iconos.Reindexar())
            submenu.separador()
        submenu1 = submenu.submenu(_("Create trainings"), Iconos.Modificar())
        submenu1.opcion("new_ssp", "%s - %s - %s" %(_("Sequential"), _("Static"), _("Positions")), Iconos.TrainSequential())
        submenu1.opcion("new_eng", "With engines", Iconos.TrainEngines())

        resp = menu.lanza()
        if resp is None:
            return
        if resp.startswith("tr_"):
            self.resultado = resp
            self.accept()
        elif resp == "new_ssp":
            self.trainNewSSP()
        elif resp == "new_eng":
            self.trainNewEngines()
        elif resp == "update":
            self.trainUpdateAll()

    def train_test(self):
        if len(self.dbop) == 0:
            return False, False
        training = self.dbop.training()
        trainingEng = self.dbop.trainingEngines()
        return training is not None, trainingEng is not None

    def trainNewSSP(self):
        training = self.dbop.training()
        color = "WHITE"
        random_order = False
        max_moves = 0

        if training is not None:
            color = training["COLOR"]
            random_order = training["RANDOM"]
            max_moves = training["MAXMOVES"]

        separador = FormLayout.separador
        liGen = [separador]

        liJ = [(_("White"), "WHITE"), (_("Black"), "BLACK")]
        config = FormLayout.Combobox(_("Play with"), liJ)
        liGen.append((config, color))

        liGen.append(separador)
        liGen.append((_("Random order"), random_order))

        liGen.append(separador)
        liGen.append((_("Maximum number of moves (0=all)"), max_moves))

        resultado = FormLayout.fedit(liGen, title=_("New training"), parent=self, anchoMinimo=360, icon=Iconos.Study())
        if resultado is None:
            return

        accion, liResp = resultado

        reg = {}

        reg["COLOR"], reg["RANDOM"], reg["MAXMOVES"] = liResp

        self.dbop.createTrainingSSP(reg, self.procesador)

        QTUtil2.mensaje(self, _("The trainings of this opening has been created"))

    def trainNewEngines(self):
        training = self.dbop.trainingEngines()
        color = "WHITE"
        basepv = self.dbop.basePV
        mandatory = basepv.count(" ")+1 if len(basepv)>0 else 0
        control = 10
        lost_points = 20
        engine_control = self.configuracion.tutor.clave
        engine_time = 5.0
        num_engines = 20
        key_engine = "alaric"
        ext_engines = []
        auto_analysis = True
        ask_movesdifferent = False
        times = [500, 1000, 2000, 4000, 8000]
        books = ["", "", "", "", ""]

        if training is not None:
            color = training["COLOR"]
            mandatory = training.get("MANDATORY", mandatory)
            control = training.get("CONTROL", control)
            lost_points = training.get("LOST_POINTS", lost_points)
            engine_control = training.get("ENGINE_CONTROL", engine_control)
            engine_time = training.get("ENGINE_TIME", engine_time)
            num_engines = training.get("NUM_ENGINES", num_engines)
            key_engine = training.get("KEY_ENGINE", key_engine)
            ext_engines = training.get("EXT_ENGINES", ext_engines)
            auto_analysis = training.get("AUTO_ANALYSIS", auto_analysis)
            ask_movesdifferent = training.get("ASK_MOVESDIFFERENT", ask_movesdifferent)
            times = training.get("TIMES", times)
            books = training.get("BOOKS", books)

        separador = FormLayout.separador
        liGen = [separador]

        liJ = [(_("White"), "WHITE"), (_("Black"), "BLACK")]
        config = FormLayout.Combobox(_("Play with"), liJ)
        liGen.append((config, color))

        liGen.append((_("Mandatory moves") + ":", mandatory))
        liGen.append(separador)
        liGen.append((_("Moves until the control") + ":", control))
        liGen.append(separador)
        liGen.append((_("Maximum number of lost centipawns to pass control") + ":", lost_points))
        liGen.append(separador)

        dicRivales = self.configuracion.dicRivales
        dicRivales = EnginesBunch.filtra(dicRivales)
        config = FormLayout.Spinbox(_("Number of engines"), 0, len(dicRivales), 50)
        liGen.append((config, num_engines))

        likeys = [(dicRivales[x].nombre, x) for x in dicRivales]
        likeys.sort(key=lambda x: x[1])
        config = FormLayout.Combobox(_("Bunch of engines"), likeys)
        liGen.append((config, key_engine))
        liGen.append(separador)

        config = FormLayout.Combobox(_("Engine that does the control"), self.configuracion.comboMotoresCompleto())
        liGen.append((config, engine_control))
        liGen.append((_("Duration of analysis (secs)") + ":", float(engine_time)))

        liGen.append(separador)

        liGen.append((_("Automatic analysis") + ":", auto_analysis))

        liGen.append(separador)

        liGen.append((_("Ask when the moves are different from the line") + ":", ask_movesdifferent))

        liMotoresExternos = self.configuracion.listaMotoresExternos(ordenados=False)
        if liMotoresExternos:
            liExt = [separador]
            for cm in liMotoresExternos:
                liExt.append((cm.nombre, cm.alias in ext_engines))
        else:
            liExt = None

        liLevels = [separador]
        listaLibros = Books.ListaLibros()
        listaLibros.recuperaVar(self.configuracion.ficheroBooks)
        listaLibros.comprueba()
        libooks = [(bookx.nombre, bookx) for bookx in listaLibros.lista]
        libooks.insert(0, ("--", None))
        for level in range(5):
            n = level+1
            title = "%s %d" % (_("Level"), n)
            liLevels.append((None, title))
            tm = times[level]/1000.0 if len(times) > level else 0.0
            liLevels.append((_("Time engines think in seconds") + ":", tm))

            bk = books[level] if len(books) > level else ""
            book = listaLibros.buscaLibro(bk) if bk else None
            config = FormLayout.Combobox(_("Book"), libooks)
            liLevels.append((config, book))

        lista = []
        lista.append((liGen, _("Basic data"), ""))
        if liExt:
            lista.append((liExt, _("External engines"), ""))
        lista.append((liLevels, _("Levels"), ""))

        resultado = FormLayout.fedit(lista, title=_("With engines"), parent=self, anchoMinimo=360, icon=Iconos.Study())
        if resultado is None:
            return

        accion, liResp = resultado

        selMotoresExt = []
        if liMotoresExternos:
            liGen, liExt, liLevels = liResp

            for x in range(len(liMotoresExternos)):
                if liExt[x]:
                    selMotoresExt.append(liMotoresExternos[x].alias)
        else:
            liGen, liLevels = liResp

        reg = {}

        (reg["COLOR"], reg["MANDATORY"], reg["CONTROL"], reg["LOST_POINTS"], reg["NUM_ENGINES"], reg["KEY_ENGINE"],
            reg["ENGINE_CONTROL"], reg["ENGINE_TIME"], reg["AUTO_ANALYSIS"], reg["ASK_MOVESDIFFERENT"] ) = liGen
        reg["EXT_ENGINES"] = selMotoresExt

        if (len(selMotoresExt) + reg["NUM_ENGINES"]) == 0:
            reg["NUM_ENGINES"] = 1

        times = []
        books = []
        for x in range(5):
            tm = int(liLevels[x*2]*1000)
            bk = liLevels[x*2+1]
            if tm:
                times.append(tm)
                books.append(bk.nombre if bk else "")
        if len(times) == 0:
            times.append(500)
            books.append(None)
        reg["TIMES"] = times
        reg["BOOKS"] = books

        self.dbop.createTrainingEngines(reg, self.procesador)

        QTUtil2.mensaje(self, _("Created"))

    def trainUpdateAll(self):
        self.dbop.updateTraining(self.procesador)
        self.dbop.updateTrainingEngines()
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
        self.show_lines()

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
            self.show_lines()

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
            frommenu.separador()
            frommenu.opcion(("pgo", part), _("Personal Opening Guide"), Iconos.BookGuide())

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
        if tipo == "pgn":
            self.importarPGN(partida)
        elif tipo == "polyglot":
            self.importarPolyglot(partida)
        elif tipo == "pgo":
            self.importarPGO(partida)
        elif tipo == "summary":
            self.importarSummary(partida)
        elif tipo == "voyager2":
            self.voyager2(partida)
        elif tipo == "opening":
            self.importarApertura(partida)
        elif tipo == "ol":
            fichero, partida = partida
            self.importarOtra(fichero, partida)
        self.show_lines()

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

    def importarLeeParam(self, titulo):
        dicData = self.dbop.getconfig("IMPORTAR_LEEPARAM")
        if not dicData:
            dicData = {}
        liGen = [FormLayout.separador]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))
        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 99, 50), dicData.get("DEPTH", 30)))
        liGen.append(FormLayout.separador)

        li = [(_("Only white best moves"), True), (_("Only black best moves"), False)]
        config = FormLayout.Combobox(_("Moves"), li)
        liGen.append((config, dicData.get("SIWHITE", True)))
        liGen.append(FormLayout.separador)

        li = [(_("Only one best move"), True), (_("All best moves"), False)]
        config = FormLayout.Combobox(_("Best move"), li)
        liGen.append((config, dicData.get("ONLYONE", True)))
        liGen.append(FormLayout.separador)

        liGen.append((FormLayout.Spinbox(_("Minimum moves must have each line"), 0, 99, 50), dicData.get("MINMOVES", 0)))

        resultado = FormLayout.fedit(liGen, title=titulo, parent=self, anchoMinimo=360, icon=Iconos.PuntoNaranja())
        if resultado:
            accion, liResp = resultado
            depth, siWhite, onlyone, minMoves = liResp
            dicData["DEPTH"] = depth
            dicData["SIWHITE"] = siWhite
            dicData["ONLYONE"] = onlyone
            dicData["MINMOVES"] = minMoves
            self.dbop.setconfig("IMPORTAR_LEEPARAM", dicData)
            self.configuracion.escVariables("WBG_MOVES", dicData)
            return dicData
        return None

    def importarSummary(self, partida):
        nomfichgames = QTVarios.selectDB(self, self.configuracion, False, True)
        if nomfichgames:
            dicData = self.importarLeeParam(_("Database summary"))
            if dicData:
                ficheroSummary = nomfichgames + "_s1"
                depth, siWhite, onlyone, minMoves = dicData["DEPTH"], dicData["SIWHITE"], dicData["ONLYONE"], dicData["MINMOVES"]
                self.dbop.importarSummary(self, partida, ficheroSummary, depth, siWhite, onlyone, minMoves)
                self.glines.refresh()
                self.glines.gotop()

    def importarPolyglot(self, partida):
        listaLibros = Books.ListaLibros()
        listaLibros.recuperaVar(self.configuracion.ficheroBooks)
        listaLibros.comprueba()

        dicData = self.dbop.getconfig("IMPORT_POLYGLOT")
        bookW = listaLibros.lista[0]
        bookB = listaLibros.lista[0]
        if dicData:
            book = listaLibros.buscaLibro(dicData["BOOKW"])
            if book:
                bookW = book
            book = listaLibros.buscaLibro(dicData["BOOKB"])
            if book:
                bookB = book

        liGen = [FormLayout.separador]

        li = [(bookx.nombre, bookx) for bookx in listaLibros.lista]
        config = FormLayout.Combobox(_("Book that plays white side"), li)
        liGen.append((config, bookW))
        liGen.append(FormLayout.separador)
        config = FormLayout.Combobox(_("Book that plays black side"), li)
        liGen.append((config, bookB))
        liGen.append(FormLayout.separador)

        resultado = FormLayout.fedit(liGen, title=_("Polyglot book"), parent=self, anchoMinimo=360, icon=Iconos.Libros())
        if resultado:
            accion, liResp = resultado
            bookW, bookB = liResp
            dicData = {"BOOKW":bookW.nombre, "BOOKB":bookB.nombre}
            self.dbop.setconfig("IMPORT_POLYGLOT", dicData)
        else:
            return

        bookW.polyglot()
        bookB.polyglot()

        titulo = bookW.nombre if bookW==bookB else "%s/%s" % (bookW.nombre, bookB.nombre)
        dicData = self.importarLeeParam(titulo)
        if dicData:
            depth, siWhite, onlyone, minMoves = dicData["DEPTH"], dicData["SIWHITE"], dicData["ONLYONE"], dicData["MINMOVES"]
            self.dbop.importarPolyglot(self, partida, bookW, bookB, titulo, depth, siWhite, onlyone, minMoves)
            self.glines.refresh()
            self.glines.gotop()

    def importarPGN(self, partida):
        previo = self.configuracion.leeVariables("OPENINGLINES")
        carpeta = previo.get("CARPETAPGN", "")

        ficheroPGN = QTUtil2.leeFichero(self, carpeta, "%s (*.pgn)" % _("PGN Format"), titulo=_("File to import"))
        if not ficheroPGN:
            return
        previo["CARPETAPGN"] = os.path.dirname(ficheroPGN)

        liGen = [(None, None)]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))

        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 999, 50), previo.get("IPGN_DEPTH", 30)))
        liGen.append((None, None))

        liVariations = (
            (_("All"), "A"),
            (_("None"), "N"),
            (_("White"), "W"),
            (_("Black"), "B")
        )
        config = FormLayout.Combobox(_("Include variations"), liVariations)
        liGen.append((config, previo.get("IPGN_VARIATIONSMODE", "A")))
        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=os.path.basename(ficheroPGN), parent=self, anchoMinimo=460,
                                     icon=Iconos.PuntoNaranja())

        if resultado:
            accion, liResp = resultado
            previo["IPGN_DEPTH"] = depth = liResp[0]
            previo["IPGN_VARIATIONSMODE"] = variations = liResp[1]

            self.dbop.importarPGN(self, partida, ficheroPGN, depth, variations)
            self.glines.refresh()
            self.glines.gotop()
            self.configuracion.escVariables("OPENINGLINES", previo)

    def importarPGO(self, partida):
        li = [(uno.name[:-4], uno.path) for uno in Util.listdir(self.configuracion.carpeta) if uno.name.endswith(".pgo")]
        if not li:
            QTUtil2.mensError(self, _("There is no one"))
            return

        liGen = [(None, None)]

        config = FormLayout.Combobox(_("Personal Opening Guide"), li)
        liGen.append((config, li[0][0]))

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))

        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 999, 50), 30))
        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=_("Personal Opening Guide"), parent=self, anchoMinimo=460,
                                     icon=Iconos.BookGuide())

        if resultado:
            accion, liResp = resultado
            pgo, depth = liResp

            self.dbop.importarPGO(partida, pgo, depth)
            self.glines.refresh()
            self.glines.gotop()

            QTUtil2.mensaje(self, _("Imported"))

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
        self.show_lines()

    def borrar(self):
        tam_dbop = len(self.dbop)
        if tam_dbop == 0:
            return
        menu = QTVarios.LCMenu(self)
        current = self.glines.recno()//2
        if 0 <= current < tam_dbop:
            menu.opcion("current", _("Remove line %d") % (current+1,), Iconos.Mover())
            menu.separador()
        if tam_dbop > 1:
            menu.opcion("lines", _("Remove a list of lines"), Iconos.MoverLibre())
            menu.separador()
            menu.opcion("worst", _("Remove worst lines"), Iconos.Borrar())
            menu.separador()
        submenu = menu.submenu(_("Remove last move if the line ends with"), Iconos.Final())
        submenu.opcion("last_white", _("White"), Iconos.Blancas())
        submenu.separador()
        submenu.opcion("last_black", _("Black"), Iconos.Negras())

        resp = menu.lanza()

        if resp == "current":
            self.dbop.saveHistory(_("Remove line %d") % (current+1,))
            del self.dbop[current]
            self.goto_inilinea()

        elif resp == "lines":
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
                            if len(sli) == 4:
                                sli.append("...")
                            elif len(sli) < 4:
                                sli.append(cad)
                            cad = ""
                    if cad:
                        sli.append(cad)
                    cli = "\n".join(sli)
                    if QTUtil2.pregunta(self, _("Do you want to remove the next lines?") + "\n\n" + cli):
                        um = QTUtil2.unMomento(self, _("Working..."))
                        self.dbop.removeLines([x-1 for x in li], cli)
                        self.glines.refresh()
                        self.goto_inilinea()
                        um.final()
        elif resp == "worst":
            self.remove_worst()
        elif resp == "last_white":
            self.remove_lastmove(True)
        elif resp == "last_black":
            self.remove_lastmove(False)
        self.show_lines()

    def remove_lastmove(self, iswhite):
        um = QTUtil2.unMomento(self, _("Working..."))
        self.dbop.remove_lastmove(iswhite, "%s %s" % (_("Remove last move if the line ends with"), _("White") if iswhite else _("Black")))
        um.final()

    def remove_worst(self):
        # color + time
        liGen = [FormLayout.separador]
        liJ = [(_("White"), "WHITE"), (_("Black"), "BLACK")]
        config = FormLayout.Combobox(_("Side"), liJ)
        liGen.append((config, "WHITE"))
        liGen.append((_("Duration of engine analysis (secs)") + ":", float(self.configuracion.tiempoTutor / 1000.0)))
        resultado = FormLayout.fedit(liGen, title=_("Remove worst lines"), parent=self, icon=Iconos.OpeningLines())
        if resultado:
            color, segs = resultado[1]
            ms = int(segs * 1000)
            if ms == 0:
                return
            si_white = color == "WHITE"
            dic = self.dbop.dicRepeFen(si_white)
            mensaje = _("Move") + "  %d/" + str(len(dic))
            tmpBP = QTUtil2.BarraProgreso(self, _("Remove worst lines"), "", len(dic))

            xgestor = self.procesador.creaGestorMotor(self.configuracion.tutor, ms, 0, siMultiPV=False)

            st_borrar = set()

            ok = True

            for n, fen in enumerate(dic, 1):

                if tmpBP.siCancelado():
                    ok = False
                    break

                tmpBP.inc()
                tmpBP.mensaje(mensaje % n)

                max_puntos = -999999
                max_pv = None
                dicPV = dic[fen]
                for pv in dicPV:
                    if tmpBP.siCancelado():
                        ok = False
                        break
                    LCEngine.setFen(fen)
                    LCEngine.movePV(pv[:2], pv[2:4], pv[4:])
                    mrm = xgestor.analiza(fen)
                    rm = mrm.mejorMov()
                    pts = rm.puntosABS()
                    if not si_white:
                        pts = -pts
                    if pts > max_puntos:
                        max_puntos = pts
                        if max_pv:
                            for nl in dicPV[max_pv]:
                                st_borrar.add(nl)
                        max_pv = pv
                    else:
                        for nl in dicPV[pv]:
                            st_borrar.add(nl)

            tmpBP.cerrar()

            xgestor.terminar()

            if ok:
                li_borrar = list(st_borrar)
                n = len(li_borrar)
                if n:
                    self.dbop.removeLines(li_borrar, _("Remove worst lines"))
                    QTUtil2.mensaje(self, _("Removed %d lines") % n)
                else:
                    QTUtil2.mensaje(self, _("Done"))

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

    def goto_next_lipv(self, lipv):
        li = self.dbop.getNumLinesPV(lipv, base=0)
        linea_actual = self.glines.recno() // 2

        if linea_actual in li:
            linea = linea_actual
        else:
            li.sort()
            linea = None
            for l in li:
                if l > linea_actual:
                    linea = l
                    break
            if linea is None:
                linea = li[0]

        njug = len(lipv)

        fila = linea*2
        if njug % 2 == 0:
            fila += 1

        ncol = njug//2
        if njug %2 == 1:
            ncol += 1

        ncol -= self.num_jg_inicial//2
        self.glines.goto(fila, ncol)
        self.glines.refresh()

    def procesosFinales(self):
        tablero = self.pboard.tablero
        tablero.dbVisual.saveMoviblesTablero(tablero)
        self.dbop.setconfig("WHITEBOTTOM", tablero.siBlancasAbajo)
        self.tabsanalisis.saveConfig()
        self.dbop.close()
        self.guardarVideo()

    def terminar(self):
        self.procesosFinales()
        self.accept()

    def closeEvent(self, event):
        self.procesosFinales()

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
        self.show_lines()


def study(procesador, fichero):
    with QTUtil.EscondeWindow(procesador.pantalla):
        dbop = OpeningLines.Opening(os.path.join(procesador.configuracion.folderOpenings, fichero))
        w = WLines(procesador, dbop)
        w.exec_()
        dbop.close()
        return w.resultado

