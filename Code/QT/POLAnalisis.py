import os

import LCEngineV1 as LCEngine

from PyQt4 import QtGui, QtCore

from Code import Books
from Code import ControlPosicion
from Code import Partida
from Code import DBgames
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import Columnas
from Code.QT import Grid
from Code.QT import QTUtil2
from Code.QT import Delegados
from Code.QT import WBG_Summary


class TabEngine(QtGui.QWidget):
    def __init__(self, tabsAnalisis, procesador, configuracion):
        QtGui.QWidget.__init__(self)

        self.analyzing = False
        self.posicion = None
        self.li_analysis = []
        self.gestor_motor = None
        self.current_mrm = None

        self.dbop = tabsAnalisis.dbop

        self.procesador = procesador
        self.configuracion = configuracion
        self.siFigurines = configuracion.figurinesPGN

        self.tabsAnalisis = tabsAnalisis
        self.bt_start = Controles.PB(self, "", self.start).ponIcono(Iconos.Pelicula_Seguir(), 32)
        self.bt_stop = Controles.PB(self, "", self.stop).ponIcono(Iconos.Pelicula_Pausa(), 32)
        self.bt_stop.hide()

        self.lb_engine = Controles.LB(self, _("Engine") + ":")
        liMotores = configuracion.comboMotoresCompleto()  # (nombre, clave)
        default = configuracion.tutor.clave
        engine = self.dbop.getconfig("ENGINE", default)
        if len([clave for nombre,clave in liMotores if clave==engine]) == 0:
            engine = default
        self.cb_engine = Controles.CB(self, liMotores, engine).capturaCambiado(self.reset_motor)

        multipv = self.dbop.getconfig("ENGINE_MULTIPV", 10)
        lb_multipv = Controles.LB(self, _("Multi PV")+": ")
        self.sb_multipv = Controles.SB(self, multipv, 1, 500).tamMaximo(50)

        self.lb_analisis = Controles.LB(self, "").ponFondoN("#C9D2D7").ponTipoLetra(puntos=configuracion.puntosPGN)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("PDT", _("Evaluation"), 120, siCentrado=True)
        delegado = Delegados.EtiquetaPOS(True, siLineas=False) if self.siFigurines else None
        oColumnas.nueva("SOL", "", 100, siCentrado=True, edicion=delegado)
        oColumnas.nueva("PGN", _("Solution"), 860)

        self.grid_analysis = Grid.Grid(self, oColumnas, siSelecFilas=True, siCabeceraVisible=False)
        self.grid_analysis.tipoLetra(puntos=configuracion.puntosPGN)
        self.grid_analysis.ponAltoFila(configuracion.altoFilaPGN)
        # self.registrarGrid(self.grid_analysis)

        ly_lin1 = Colocacion.H().control(self.bt_start).control(self.bt_stop).control(self.lb_engine)
        ly_lin1.control(self.cb_engine)
        ly_lin1.espacio(50).control(lb_multipv).control(self.sb_multipv).relleno()
        ly = Colocacion.V().otro(ly_lin1).control(self.lb_analisis).control(self.grid_analysis).margen(3)

        self.setLayout(ly)

        self.reset_motor()

    def saveCurrent(self):
        if self.current_mrm:
            fenM2 = self.current_posicion.fenM2()
            dic = self.dbop.getfenvalue(fenM2)
            if "ANALISIS" in dic:
                mrm_ant = dic["ANALISIS"]
                if mrm_ant.getdepth0() > self.current_mrm.getdepth0():
                    return
            dic["ANALISIS"] = self.current_mrm
            self.dbop.setfenvalue(fenM2, dic)

    def setData(self, label, posicion):
        self.saveCurrent()
        self.posicion = posicion
        self.lb_analisis.ponTexto(label)
        if self.analyzing:
            self.analyzing = False
            self.gestor_motor.ac_final(0)
            partida = Partida.Partida(self.posicion)
            self.gestor_motor.ac_inicio(partida)
            self.analyzing = True
            QtCore.QTimer.singleShot(1000, self.lee_analisis)
        else:
            fenM2 = posicion.fenM2()
            dic = self.dbop.getfenvalue(fenM2)
            if "ANALISIS" in dic:
                self.show_analisis(dic["ANALISIS"])
            else:
                self.li_analysis = []
                self.grid_analysis.refresh()

    def start(self):
        self.current_mrm = None
        self.current_posicion = None
        self.sb_multipv.setDisabled(True)
        self.cb_engine.setDisabled(True)
        self.analyzing = True
        self.sb_multipv.setDisabled(True)
        self.show_stop()
        multipv = self.sb_multipv.valor()
        self.gestor_motor.actMultiPV(multipv)
        partida = Partida.Partida(self.posicion)
        self.gestor_motor.ac_inicio(partida)
        QtCore.QTimer.singleShot(1000, self.lee_analisis)

    def show_start(self):
        self.bt_stop.hide()
        self.bt_start.show()

    def show_stop(self):
        self.bt_start.hide()
        self.bt_stop.show()

    def show_analisis(self, mrm):
        self.current_mrm = mrm
        self.current_posicion = self.posicion
        li = []
        for rm in mrm.liMultiPV:
            partida = Partida.Partida(self.posicion)
            partida.leerPV(rm.pv)
            pgn = partida.pgnBaseRAW()
            lit = pgn.split(" ")
            siBlancas = self.posicion.siBlancas
            if siBlancas:
                pgn0 = lit[0].split(".")[-1]
                pgn1 = " ".join(lit[1:])
            else:
                pgn0 = lit[1]
                pgn1 = " ".join(lit[2:])

            if self.siFigurines:
                partida.ms_sol = pgn0, siBlancas, None, None, None, None, False, False
            else:
                partida.ms_sol = pgn0
            partida.ms_pgn = pgn1
            partida.ms_pdt = rm.abrTextoPDT()
            li.append(partida)
        self.li_analysis = li
        self.grid_analysis.refresh()

    def lee_analisis(self):
        if self.analyzing:
            mrm = self.gestor_motor.ac_estado()
            self.show_analisis(mrm)
            QtCore.QTimer.singleShot(2000, self.lee_analisis)

    def stop(self):
        self.saveCurrent()
        self.sb_multipv.setDisabled(False)
        self.cb_engine.setDisabled(False)
        self.analyzing = False
        self.show_start()
        if self.gestor_motor:
            self.gestor_motor.ac_final(0)

    def reset_motor(self):
        self.saveCurrent()
        clave = self.cb_engine.valor()
        if not clave:
            return
        self.analyzing = False
        if self.gestor_motor:
            self.gestor_motor.terminar()
        self.stop()
        conf_motor = self.configuracion.buscaRivalExt(clave)

        multipv = self.sb_multipv.valor()
        self.gestor_motor = self.procesador.creaGestorMotor(conf_motor, 0, 0, siMultiPV=multipv > 1)

    def gridNumDatos(self, grid):
        return len(self.li_analysis)

    def gridDato(self, grid, fila, oColumna):
        if oColumna.clave == "PDT":
            return self.li_analysis[fila].ms_pdt
        elif oColumna.clave == "SOL":
            return self.li_analysis[fila].ms_sol
        else:
            return self.li_analysis[fila].ms_pgn

    def saveConfig(self):
        self.dbop.setconfig("ENGINE", self.cb_engine.valor())
        self.dbop.setconfig("ENGINE_MULTIPV", self.sb_multipv.valor())


class TabBook(QtGui.QWidget):
    def __init__(self, tabsanalisis, book, configuracion):
        QtGui.QWidget.__init__(self)

        self.tabsanalisis = tabsanalisis
        self.posicion = None
        self.leido = False

        self.book = book
        book.polyglot()
        self.li_moves = []

        self.siFigurines = configuracion.figurinesPGN

        oColumnas = Columnas.ListaColumnas()
        delegado = Delegados.EtiquetaPOS(True, siLineas=False) if self.siFigurines else None
        for x in range(20):
            oColumnas.nueva(x, "", 80, siCentrado=True, edicion = delegado)
        self.grid_moves = Grid.Grid(self, oColumnas, siSelecFilas=True, siCabeceraMovible=False, siCabeceraVisible=False)
        self.grid_moves.tipoLetra(puntos=configuracion.puntosPGN)
        self.grid_moves.ponAltoFila(configuracion.altoFilaPGN)

        ly = Colocacion.V().control(self.grid_moves).margen(3)

        self.setLayout(ly)

    def gridNumDatos(self, grid):
        return len(self.li_moves)

    def gridDato(self, grid, fila, oColumna):
        mv = self.li_moves[fila]
        li = mv.dato
        key = int(oColumna.clave)
        pgn = li[key]
        if self.siFigurines:
            siBlancas = " w " in mv.fen
            return pgn, siBlancas, None, None, None, None, False, True
        else:
            return pgn

    def gridDobleClick(self, grid, fila, oColumna):
        self.lee_subnivel(fila)
        self.grid_moves.refresh()

    def gridBotonDerecho(self, grid, fila, columna, modificadores):
        self.borra_subnivel(fila)
        self.grid_moves.refresh()

    def setData(self, posicion):
        self.posicion = posicion
        self.start()

    def borra_subnivel(self, fila):
        alm = self.li_moves[fila]
        nv = alm.nivel
        if nv == 0:
            return
        li = []
        for x in range(fila, 0, -1):
            alm1 = self.li_moves[x]
            if alm1.nivel < nv:
                break
            li.append(x)
        for x in range(fila+1, len(self.li_moves)):
            alm1 = self.li_moves[x]
            if alm1.nivel < nv:
                break
            li.append(x)
        li.sort(reverse=True)
        for x in li:
            del self.li_moves[x]

    def lee_subnivel(self, fila):
        alm_base = self.li_moves[fila]
        if alm_base.nivel >= 17:
            return
        LCEngine.setFen(alm_base.fen)
        if LCEngine.movePV(alm_base.desde, alm_base.hasta, alm_base.coronacion):
            fen = LCEngine.getFen()
            for alm in self.book.almListaJugadas(fen):
                nv = alm.nivel = alm_base.nivel + 1
                alm.dato = [""] * 20
                alm.dato[nv] = alm.pgn
                alm.dato[nv+1] = alm.porc
                alm.dato[nv+2] = "%d" % alm.weight
                fila += 1
                self.li_moves.insert(fila, alm)

    def lee(self):
        if not self.leido and self.posicion:
            fen = self.posicion.fen()
            self.li_moves = self.book.almListaJugadas(fen)
            for alm in self.li_moves:
                alm.nivel = 0
                alm.dato = [""]*20
                alm.dato[0] = alm.pgn
                alm.dato[1] = alm.porc
                alm.dato[2] = "%d" % alm.weight
            self.leido = True

    def start(self):
        self.leido = False
        self.lee()
        self.grid_moves.refresh()

    def stop(self):
        pass


class TabDatabase(QtGui.QWidget):
    def __init__(self, tabsanalisis, procesador, dbstat):
        QtGui.QWidget.__init__(self)

        self.tabsanalisis = tabsanalisis

        self.pv = None

        self.bookGuide = self
        self.dbstat = dbstat

        self.wsummary = WBG_Summary.WSummaryBase(procesador, dbstat)

        layout = Colocacion.H().control(self.wsummary)
        self.setLayout(layout)

    def setData(self, pv):
        self.pv = pv
        self.wsummary.actualizaPV(self.pv)

    def start(self):
        self.wsummary.actualizaPV(self.pv)

    def stop(self):
        self.dbstat.close()


class TabsAnalisis(QtGui.QWidget):
    def __init__(self, panelOpening, procesador, configuracion):
        QtGui.QWidget.__init__(self)

        self.panelOpening = panelOpening
        self.dbop = panelOpening.dbop

        self.procesador = procesador
        self.configuracion = configuracion
        self.partida = None
        self.njg = None

        self.tabengine = TabEngine(self, procesador, configuracion)
        self.liTabs = [("engine", self.tabengine),]
        self.tabActive = 0

        self.tabs = Controles.Tab(panelOpening)
        self.tabs.ponTipoLetra(puntos=self.configuracion.puntosPGN)
        self.tabs.nuevaTab(self.tabengine, _("Engine"))
        self.tabs.setTabIcon(0, Iconos.Motor())
        self.tabs.dispatchChange(self.tabChanged)

        tabButton = QtGui.QToolButton(self)
        tabButton.setIcon(Iconos.Nuevo())
        tabButton.clicked.connect(self.creaTab)
        li = [(_("Analysis of next move"), True), (_("Analysis of current move"), False)]
        self.cb_nextmove = Controles.CB(self, li, True).capturaCambiado(self.changedNextMove)

        corner_widget = QtGui.QWidget(self)
        lyCorner = Colocacion.H().control(self.cb_nextmove).control(tabButton).margen(0)
        corner_widget.setLayout(lyCorner)

        self.tabs.setCornerWidget(corner_widget)
        self.tabs.setTabsClosable(True)
        self.tabs.tabCloseRequested.connect(self.tabCloseRequested)

        layout = Colocacion.V()
        layout.control(self.tabs).margen(0)
        self.setLayout(layout)

    def changedNextMove(self):
        if self.partida:
            self.setPosicion(self.partida, self.njg)

    def tabChanged(self, ntab):
        self.tabActive = ntab
        if ntab:
            tipo, wtab = self.liTabs[ntab]
            wtab.start()

    def tabCloseRequested(self, ntab):
        tipo, wtab = self.liTabs[ntab]
        wtab.stop()
        if ntab:
            del self.liTabs[ntab]
            self.tabs.removeTab(ntab)
            del wtab

    def creaTab(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion("book", _("Polyglot book"), Iconos.Libros())
        menu.separador()
        menu.opcion("dbase", _("Database"), Iconos.Database())
        resp = menu.lanza()
        pos = 0
        if resp == "book":
            book = self.seleccionaLibro()
            if book:
                tabbook = TabBook(self, book, self.configuracion)
                self.liTabs.append((resp, tabbook))
                pos = len(self.liTabs)-1
                self.tabs.nuevaTab(tabbook, book.nombre, pos)
                self.tabs.setTabIcon(pos, Iconos.Libros())
                self.setPosicion(self.partida, self.njg, pos)
        elif resp == "dbase":
            nomfichgames = QTVarios.selectDB(self, self.configuracion, False, True)
            if nomfichgames:
                dbSTAT = DBgames.TreeSTAT(nomfichgames + "_s1")
                tabdb = TabDatabase(self, self.procesador, dbSTAT)
                self.liTabs.append((resp, tabdb))
                pos = len(self.liTabs) - 1
                self.setPosicion(self.partida, self.njg, pos)
                nombre = os.path.basename(nomfichgames)[:-4]
                self.tabs.nuevaTab(tabdb, nombre, pos)
                self.tabs.setTabIcon(pos, Iconos.Database())
        self.tabs.activa(pos)

    def setPosicion(self, partida, njg, numTab=None):
        if partida is None:
            return
        jg = partida.jugada(njg)
        self.partida = partida
        self.njg = njg
        next = self.cb_nextmove.valor()
        if jg:
            if njg == 0:
                pv = partida.pv_hasta(njg) if next else ""
            else:
                pv = partida.pv_hasta(njg if next else njg - 1)
            posicion = jg.posicion if next else jg.posicionBase
        else:
            posicion = ControlPosicion.ControlPosicion().posInicial()
            pv = ""

        for ntab, (tipo, tab) in enumerate(self.liTabs):
            if ntab == 0:
                p = Partida.Partida()
                p.leerPV(pv)
                tab.setData(p.pgnHTML(siFigurines=self.configuracion.figurinesPGN), posicion)
            else:
                data = pv if tipo == "dbase" else posicion
                if numTab is not None:
                    if ntab != numTab:
                        continue
                tab.setData(data)
                tab.start()

    def seleccionaLibro(self):
        listaLibros = Books.ListaLibros()
        listaLibros.recuperaVar(self.configuracion.ficheroBooks)
        listaLibros.comprueba()
        menu = QTVarios.LCMenu(self)
        rondo = QTVarios.rondoPuntos()
        for book in listaLibros.lista:
            menu.opcion(("x", book), book.nombre, rondo.otro())
            menu.separador()
        menu.opcion(("n", None), _("Install new book"), Iconos.Nuevo())
        resp = menu.lanza()
        if resp:
            orden, book = resp
            if orden == "x":
                pass
            elif orden == "n":
                fbin = QTUtil2.leeFichero(self, listaLibros.path, "bin", titulo=_("Polyglot book"))
                if fbin:
                    listaLibros.path = os.path.dirname(fbin)
                    nombre = os.path.basename(fbin)[:-4]
                    book = Books.Libro("P", nombre, fbin, True)
                    listaLibros.nuevo(book)
                    listaLibros.guardaVar(self.configuracion.ficheroBooks)
        else:
            book = None
        return book

    def saveConfig(self):
        for tipo, wtab in self.liTabs:
            if tipo == "engine":
                wtab.saveConfig()

