import os

from PyQt4 import QtGui, QtCore

from Code import Analisis
from Code import Partida
from Code import DBgames
from Code import TrListas
from Code import Util
from Code import AperturasStd
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaBooks
from Code.QT import PantallaPGN
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import PantallaSolo
from Code.QT import PantallaSavePGN
from Code.QT import PantallaAnalisisParam
from Code.QT import PantallaPlayPGN


class WGames(QtGui.QWidget):
    def __init__(self, procesador, winBookGuide, dbGames, wsummary, siMoves=True):
        QtGui.QWidget.__init__(self)

        self.winBookGuide = winBookGuide
        self.dbGames = dbGames  # <--setdbGames
        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.siMoves = siMoves

        self.wsummary = wsummary
        self.infoMove = None  # <-- setInfoMove
        self.summaryActivo = None  # movimiento activo en summary
        self.numJugada = 0  # Se usa para indicarla al mostrar el pgn en infoMove

        self.terminado = False # singleShot

        self.ap = AperturasStd.ap

        self.liFiltro = []
        self.where = None

        self.last_opening = None

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 70, siCentrado=True)
        liBasic = dbGames.liCamposBase
        ancho = 70
        for clave in liBasic:
            rotulo = TrListas.pgnLabel(clave)
            oColumnas.nueva(clave, rotulo, ancho, siCentrado=True)
        oColumnas.nueva("rowid", _("Row ID"), 70, siCentrado=True)
        oColumnas.nueva("opening", _("Opening"), 140)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True, xid="wgames")

        # Status bar
        self.status = QtGui.QStatusBar(self)
        self.status.setFixedHeight(22)

        # ToolBar
        liAccionesWork = [
            (_("Close"), Iconos.MainMenu(), self.tw_terminar), None,
            (_("File"), Iconos.File(), self.tg_file), None,
            (_("New"), Iconos.Nuevo(), self.tw_nuevo, _("Add a new game")), None,
            (_("Edit"), Iconos.Modificar(), self.tw_editar), None,
            (_("First"), Iconos.Inicio(), self.tw_gotop), None,
            (_("Last"), Iconos.Final(), self.tw_gobottom), None,
            (_("Filter"), Iconos.Filtrar(), self.tw_filtrar), None,
            (_("Remove"), Iconos.Borrar(), self.tw_borrar),None,
            (_("Up"), Iconos.Arriba(), self.tw_up), None,
            (_("Down"), Iconos.Abajo(), self.tw_down), None,
            (_("Config"), Iconos.Configurar(), self.tw_configure), None,
            (_("Utilities"), Iconos.Utilidades(), self.tw_utilities), None,
        ]

        self.tbWork = QTVarios.LCTB(self, liAccionesWork, tamIcon=24, puntos=12)

        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#4E5A65").ponTipoLetra(puntos=16)
        lyNT = Colocacion.H().control(self.lbName)
        if not siMoves:
            self.lbName.hide()

        lyTB = Colocacion.H().control(self.tbWork)

        layout = Colocacion.V().otro(lyNT).otro(lyTB).control(self.grid).control(self.status).margen(1)

        self.setLayout(layout)

        self.setNameToolBar()

        self.recuperaOrden()

    def recuperaOrden(self):
        liOrden = self.dbGames.recuperaOrden()
        if liOrden:
            for clave, tipo in liOrden:
                col = self.grid.buscaCabecera(clave)
                if col:
                    col.antigua = col.cabecera
                    col.cabecera = col.antigua + ("+" if tipo == "ASC" else "-")
            self.grid.refresh()

    def limpiaColumnas(self):
        for col in self.grid.oColumnas.liColumnas:
            cab = col.cabecera
            if cab[-1] in "+-":
                col.cabecera = col.antigua

    def setdbGames(self, dbGames):
        self.dbGames = dbGames

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove
        self.graphicBoardReset()

    def setNameToolBar(self):
        nomFichero = self.dbGames.rotulo()
        self.lbName.ponTexto(nomFichero)
        self.wsummary.showActiveName(nomFichero)

    def updateStatus(self):
        if self.terminado:
            return
        if not self.summaryActivo:
            txt = ""
        else:
            partida = self.summaryActivo.get("partida", Partida.PartidaCompleta())
            nj = partida.numJugadas()
            if nj > 1:
                p = partida.copia(nj-2)
                txt = "%s | " % p.pgnBaseRAW()
            else:
                txt = ""
            siPte = self.dbGames.siFaltanRegistrosPorLeer()
            if not siPte:
                recs = self.dbGames.reccount()
                if recs:
                    txt += "%s: %d" % (_("Games"), recs)
            if self.where:
                txt += " | %s: %s" % (_("Filter"), self.where)
            if siPte:
                QtCore.QTimer.singleShot(1000, self.updateStatus)

        self.status.showMessage(txt, 0)

    def gridNumDatos(self, grid):
        return self.dbGames.reccount()

    def gridDato(self, grid, nfila, ocol):
        clave = ocol.clave
        if clave == "numero":
            return str(nfila + 1)
        elif clave == "rowid":
            return str(self.dbGames.getROWID(nfila))
        elif clave == "opening":
            xpv = self.dbGames.field(nfila, "XPV")
            return self.ap.XPV(xpv)
        return self.dbGames.field(nfila, clave)

    def gridDobleClick(self, grid, fil, col):
        self.tw_editar()

    def gridDobleClickCabecera(self, grid, col):
        liOrden = self.dbGames.dameOrden()
        clave = col.clave
        if clave in ("numero"):
            return
        if clave == "opening":
            clave = "XPV"
        siEsta = False
        for n, (cl, tp) in enumerate(liOrden):
            if cl == clave:
                siEsta = True
                if tp == "ASC":
                    liOrden[n] = (clave, "DESC")
                    col.cabecera = col.antigua + "-"
                    if n:
                        del liOrden[n]
                        liOrden.insert(0, (clave, "DESC"))

                elif tp == "DESC":
                    del liOrden[n]
                    col.cabecera = col.cabecera[:-1]
                break
        if not siEsta:
            liOrden.insert(0, (clave, "ASC"))
            col.antigua = col.cabecera
            col.cabecera = col.antigua + "+"
        self.dbGames.ponOrden(liOrden)
        self.grid.refresh()
        self.updateStatus()

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        if k in (QtCore.Qt.Key_Enter, QtCore.Qt.Key_Return):
            self.tw_editar()

    def closeEvent(self):
        self.tw_terminar()

    def tw_terminar(self):
        self.terminado = True
        self.dbGames.close()
        self.dbGames.guardaOrden()
        self.winBookGuide.terminar()

    def actualiza(self, siObligatorio=False):
        def pvSummary(summary):
            if summary is None:
                return ""
            lipv = summary.get("pv", "").split(" ")
            return " ".join(lipv[:-1])
        summaryActivo = self.wsummary.movActivo()
        if siObligatorio or pvSummary(self.summaryActivo) != pvSummary(summaryActivo) or self.liFiltro:
            self.where = None
            self.summaryActivo = summaryActivo
            pv = ""
            if self.summaryActivo:
                pv = self.summaryActivo.get("pv")
                if pv:
                    lipv = pv.split(" ")
                    pv = " ".join(lipv[:-1])
                else:
                    pv = ""
            self.dbGames.filterPV(pv)
            self.updateStatus()
            self.numJugada = pv.count(" ")
            self.grid.refresh()
            self.grid.gotop()
        recno = self.grid.recno()
        if recno >= 0:
            self.gridCambiadoRegistro(None, recno, None)

    def gridCambiadoRegistro(self, grid, fila, oCol):
        if self.gridNumDatos(grid) > fila >= 0:
            pv = self.dbGames.damePV(fila)
            p = Partida.Partida()
            p.leerPV(pv)
            p.siTerminada()
            self.infoMove.modoPartida(p, self.numJugada)
            self.setFocus()
            self.grid.setFocus()

    def tw_gobottom(self):
        self.grid.gobottom()

    def tw_gotop(self):
        self.grid.gotop()

    def tw_up(self):
        fila = self.grid.recno()
        filaNueva = self.dbGames.intercambia(fila, True)
        if filaNueva is not None:
            self.grid.goto(filaNueva, 0)
            self.grid.refresh()

    def tw_down(self):
        fila = self.grid.recno()
        filaNueva = self.dbGames.intercambia(fila, False)
        if filaNueva is not None:
            self.grid.goto(filaNueva, 0)
            self.grid.refresh()

    def editar(self, recno, partidaCompleta):
        partidaCompleta = self.procesador.gestorPartida(self, partidaCompleta, True, self.infoMove.tablero)
        if partidaCompleta:
            if not self.dbGames.guardaPartidaRecno(recno, partidaCompleta):
                QTUtil2.mensError(self, _("This game already exists."))
            else:
                self.actualiza(True)
                if recno is None:
                    self.grid.gobottom()
                else:
                    self.grid.goto(recno, 0)
                    self.gridCambiadoRegistro(self, recno, None)

                self.wsummary.rehazActual()

    def tw_nuevo(self):
        recno = None
        pc = self.dbGames.blankPartida()
        self.editar(recno, pc)

    def tw_editar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            recno = li[0]
            partidaCompleta = self.dbGames.leePartidaRecno(recno)
            if partidaCompleta:
                self.editar(recno, partidaCompleta)
            else:
                QTUtil2.mensaje(self, _("This game is wrong and can not be edited"))

    def tw_filtrar(self):
        xpv = None
        if self.summaryActivo and "pv" in self.summaryActivo:
            li = self.summaryActivo["pv"].split(" ")
            if len(li) > 1:
                xpv = " ".join(li[:-1])

        def refresh():
            self.grid.refresh()
            self.grid.gotop()
            self.updateStatus()
            self.gridCambiadoRegistro(None, 0, 0)

        def standard():
            w = PantallaPGN.WFiltrar(self, self.grid.oColumnas, self.liFiltro, self.dbGames.nomFichero)
            if w.exec_():
                self.liFiltro = w.liFiltro

                self.where = w.where()
                self.dbGames.filterPV(xpv, self.where)
                refresh()

        def raw_sql():
            w = PantallaPGN.WFiltrarRaw(self, self.grid.oColumnas, self.where)
            if w.exec_():
                self.where = w.where
                self.dbGames.filterPV(xpv, self.where)
                refresh()

        def opening():
            me = QTUtil2.unMomento(self)
            import Code.QT.PantallaAperturas as PantallaAperturas
            w = PantallaAperturas.WAperturas(self, self.configuracion, self.last_opening)
            me.final()
            if w.exec_():
                self.last_opening = ap = w.resultado()
                pv = getattr(ap, "a1h8", "")
                self.dbGames.filterPV(pv)
                self.numJugada = pv.count(" ")
                refresh()

        def remove():
            self.dbGames.filterPV("")
            self.where = None
            refresh()

        menu = QTVarios.LCMenu(self)
        menu.opcion(standard, _("Standard"), Iconos.Filtrar())
        menu.separador()
        menu.opcion(raw_sql, _("Advanced"), Iconos.SQL_RAW())
        menu.separador()
        menu.opcion(opening, _("Opening"), Iconos.Apertura())
        if self.dbGames.filter is not None and self.dbGames.filter:
            menu.separador()
            menu.opcion(remove, _("Remove filter"), Iconos.Cancelar())
        # menu.opcion(vopening, _("Various openings"), Iconos.Apertura())
        # menu.separador()
        # menu.opcion(sample, _("Select a sample"), Iconos.Apertura())
        # menu.separador()

        resp = menu.lanza()
        if resp:
            resp()

    def tw_borrar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            if not QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                return

            um = QTUtil2.unMomento(self)

            self.dbGames.borrarLista(li)
            if self.siMoves:
                self.summaryActivo["games"] -= len(li)
            self.grid.refresh()
            self.updateStatus()

            self.wsummary.reset()

            um.final()

    def tg_file(self):
        menu = QTVarios.LCMenu(self)
        lista = QTVarios.listaDB(self.configuracion, False)
        if lista:
            smenu = menu.submenu( _("Open another database"), Iconos.DatabaseC())
            rp = QTVarios.rondoPuntos()
            for fich in lista:
                smenu.opcion(os.path.join(self.configuracion.carpetaGames, fich), _F(fich[:-4]), rp.otro())
                smenu.separador()
            menu.separador()

        menu.opcion(self.tg_create,_("Create a new database"), Iconos.NuevaDB())
        menu.separador()

        submenu = menu.submenu(_("Import from"), Iconos.DatabaseCNew())
        submenu.opcion(self.tg_importar_PGN, _("A PGN file"), Iconos.FichPGN())
        submenu.separador()
        submenu.opcion(self.tg_importar_DB, _("Other database"), Iconos.DatabaseC())
        submenu.separador()
        submenu.opcion(self.tg_importar_pks, _("A PKS file"), Iconos.JuegaSolo())
        menu.separador()

        submenu = menu.submenu(_("Export to"), Iconos.DatabaseMas())
        submenu.opcion(self.tg_exportar_PGN, _("A PGN file"), Iconos.FichPGN())
        submenu.separador()
        submenu.opcion(self.tg_exportar_DB, _("Other database"), Iconos.DatabaseC())
        menu.separador()

        menu.opcion(self.tg_pack, _("Pack database"), Iconos.Pack())
        menu.separador()

        resp = menu.lanza()
        if resp:
            if type(resp) == str:
                self.changeDBgames(resp)
            else:
                resp()

    def tw_configure(self):
        siShow = self.dbGames.recuperaConfig("GRAPHICS_SHOW_ALLWAYS", False)
        siGraphicsSpecific = self.dbGames.recuperaConfig("GRAPHICS_SPECIFIC", False)
        menu = QTVarios.LCMenu(self)
        dico = {True: Iconos.Aceptar(), False: Iconos.PuntoAmarillo()}
        menu1 = menu.submenu(_("Graphic elements (Director)"), Iconos.Script())
        menu2 = menu1.submenu(_("Show allways"), Iconos.PuntoAzul())
        menu2.opcion(self.tw_dir_show_yes, _("Yes"), dico[siShow])
        menu2.separador()
        menu2.opcion(self.tw_dir_show_no, _("No"), dico[not siShow])
        menu1.separador()
        menu2 = menu1.submenu(_("Specific to this database"), Iconos.PuntoAzul())
        menu2.opcion(self.tw_locale_yes, _("Yes"), dico[siGraphicsSpecific])
        menu2.separador()
        menu2.opcion(self.tw_locale_no, _("No"), dico[not siGraphicsSpecific])
        resp = menu.lanza()
        if resp:
            resp()

    def readVarsConfig(self):
        showAllways = self.dbGames.recuperaConfig("GRAPHICS_SHOW_ALLWAYS")
        specific = self.dbGames.recuperaConfig("GRAPHICS_SPECIFIC")
        return showAllways, specific

    def graphicBoardReset(self):
        showAllways, specific = self.readVarsConfig()
        fichGraphic = self.dbGames.nomFichero if specific else None
        self.infoMove.tablero.dbVisual_setFichero(fichGraphic)
        self.infoMove.tablero.dbVisual_setShowAllways(showAllways)

    def tw_dir_show_yes(self):
        self.dbGames.guardaConfig("GRAPHICS_SHOW_ALLWAYS", True)
        self.graphicBoardReset()

    def tw_dir_show_no(self):
        self.dbGames.guardaConfig("GRAPHICS_SHOW_ALLWAYS", False)
        self.graphicBoardReset()

    def tw_locale_yes(self):
        self.dbGames.guardaConfig("GRAPHICS_SPECIFIC", True)
        self.graphicBoardReset()

    def tw_locale_no(self):
        self.dbGames.guardaConfig("GRAPHICS_SPECIFIC", False)
        self.graphicBoardReset()

    def tw_utilities(self):
        menu = QTVarios.LCMenu(self)
        ico = Iconos.PuntoAzul()
        menu1 = menu.submenu(_("Polyglot book"), ico)
        menu1.opcion(self.tw_uti_pcreate, _("Create a new book"), ico)
        menu1.separador()
        menu1.opcion(self.tw_uti_pmerge, _("Merge two books in one"), ico)
        menu.separador()
        menu.opcion(self.tw_massive_change_tags, _("Massive change of tags"), Iconos.PGN())
        menu.separador()
        menu.opcion(self.tw_massive_analysis, _("Mass analysis"), Iconos.Analizar())
        menu.separador()
        menu.opcion(self.tw_play_against, _("Play against a game"), Iconos.Law())
        resp = menu.lanza()
        if resp:
            resp()

    def tw_play_against(self):
        li = self.grid.recnosSeleccionados()
        if li:
            recno = li[0]
            raw = self.dbGames.leeAllRecno(recno)
            xpv = raw["XPV"]
            partidaCompleta = self.dbGames.leePartidaRaw(raw)
            h = hash(xpv)
            dbPlay = PantallaPlayPGN.PlayPGNs(self.configuracion.ficheroPlayPGN)
            recplay = dbPlay.recnoHash(h)
            if recplay is None:
                dic = Util.SymbolDict()
                for tag, value in partidaCompleta.liTags:
                    dic[tag] = value
                dic["PARTIDA"] = partidaCompleta.guardaEnTexto()
                dbPlay.appendHash(h, dic)
                recplay = dbPlay.recnoHash(h)
            dbPlay.close()

            self.tw_terminar()
            self.procesador.playPGNshow(recplay)

    def tg_importar_pks(self):
        path_pks = QTUtil2.leeFichero(self, self.configuracion.dirJS, "pks")
        if path_pks:
            direc = os.path.dirname(path_pks)
            if direc != self.configuracion.dirJS:
                self.configuracion.dirJS = direc
                self.configuracion.graba()

            mens_error = self.dbGames.insert_pks(path_pks)
            if mens_error:
                QTUtil2.mensError(self, mens_error)
                return
            self.actualiza(True)
            self.grid.gobottom(0)

    def tg_pack(self):
        um = QTUtil2.unMomento(self)
        self.dbGames.pack()
        um.final()

    def tw_massive_change_tags(self):
        resp = PantallaSolo.massive_change_tags(self, self.configuracion, len(self.grid.recnosSeleccionados()), True)
        if resp:
            recno = self.grid.recno()
            liTags, remove, overwrite, si_all, set_extend = resp
            liRegistros = range(self.dbGames.reccount()) if si_all else self.grid.recnosSeleccionados()
            nRegistros = len(liRegistros)
            if (nRegistros == 1 or
                ((nRegistros > 1) and
                 QTUtil2.pregunta(self, _("Are you sure do you want to change the %d registers?" % nRegistros)))):
                um = QTUtil2.unMomento(self)
                self.dbGames.massive_change_tags(liTags, liRegistros, remove, overwrite, set_extend)
                self.grid.refresh()
                self.grid.goto(recno, 0)
                um.final()

    def tw_massive_analysis(self):
        liSeleccionadas = self.grid.recnosSeleccionados()
        nSeleccionadas = len(liSeleccionadas)

        alm = PantallaAnalisisParam.paramAnalisisMasivo(self, self.configuracion, nSeleccionadas > 1, siDatabase=True)
        if alm:

            if alm.siVariosSeleccionados:
                nregs = nSeleccionadas
            else:
                nregs = self.dbGames.reccount()

            tmpBP = QTUtil2.BarraProgreso2(self, _("Mass analysis"), formato2="%p%")
            tmpBP.ponTotal(1, nregs)
            tmpBP.ponRotulo(1, _("Game"))
            tmpBP.ponRotulo(2, _("Moves"))
            tmpBP.mostrar()

            ap = Analisis.AnalizaPartida(self.procesador, alm, True)

            for n in range(nregs):

                if tmpBP.siCancelado():
                    break

                tmpBP.pon(1, n + 1)

                if alm.siVariosSeleccionados:
                    n = liSeleccionadas[n]

                partida = self.dbGames.leePartidaRecno(n)
                self.grid.goto(n, 0)

                ap.xprocesa(partida.dicTags(), partida, tmpBP, partida.pgn())

                self.dbGames.guardaPartidaRecno(n, partida)

            if not tmpBP.siCancelado():
                ap.terminar(True)

                liCreados = []
                liNoCreados = []

                if alm.tacticblunders:
                    if ap.siTacticBlunders:
                        liCreados.append(alm.tacticblunders)
                    else:
                        liNoCreados.append(alm.tacticblunders)

                for x in (alm.pgnblunders, alm.fnsbrilliancies, alm.pgnbrilliancies):
                    if x:
                        if Util.existeFichero(x):
                            liCreados.append(x)
                        else:
                            liNoCreados.append(x)

                if alm.bmtblunders:
                    if ap.siBMTblunders:
                        liCreados.append(alm.bmtblunders)
                    else:
                        liNoCreados.append(alm.bmtblunders)
                if alm.bmtbrilliancies:
                    if ap.siBMTbrilliancies:
                        liCreados.append(alm.bmtbrilliancies)
                    else:
                        liNoCreados.append(alm.bmtbrilliancies)
                if liCreados:
                    PantallaPGN.mensajeEntrenamientos(self, liCreados, liNoCreados)

            else:
                ap.terminar(False)

            tmpBP.cerrar()

    def tw_uti_pcreate(self):
        PantallaBooks.polyglotCrear(self)

    def tw_uti_pmerge(self):
        PantallaBooks.polyglotUnir(self)

    def tg_change(self):
        database = QTVarios.selectDB(self, self.configuracion, False)
        if database:
            path = os.path.dirname(database)
            if os.path.isdir(path):
                self.changeDBgames(database)

    def listaSelected(self, no1=False):
        li = self.grid.recnosSeleccionados()
        if not li:
            return None
        if no1 and len(li) == 1:
            siTodos = True
        else:
            menu = QTVarios.LCMenu(self)
            menu.opcion(True, _("All read"), Iconos.PuntoVerde())
            menu.separador()
            menu.opcion(False, "%s [%d]"%(_("Only selected"),len(li)), Iconos.PuntoAzul())
            siTodos = menu.lanza()
            if siTodos is None:
                return None
        if siTodos:
            li = range(self.dbGames.reccount())
        return li

    def tg_exportar(self, ext):
        li = self.listaSelected()
        if li is None:
            return

        # Fichero donde a?adir
        if ext == "lcg":
            carpeta = self.configuracion.carpetaGames
        else:
            carpeta = self.configuracion.dirSalvados
        path = QTUtil2.salvaFichero(self, _("Export"), carpeta, _("File") + " %s (*.%s)"%(ext, ext), False)
        if path:
            if not path.lower().endswith(".%s"%ext):
                path += ".%s"%ext
            carpeta, nomf = os.path.split(path)
            if ext != "lcg" and carpeta != self.configuracion.dirSalvados:
                self.configuracion.dirSalvados = carpeta
                self.configuracion.graba()

            # Grabamos
            modo = "w"
            if Util.existeFichero(path):
                yn = QTUtil2.preguntaCancelar(self, _X(_("The file %1 already exists, what do you want to do?"), path),
                                              si=_("Append"), no=_("Overwrite"))
                if yn is None:
                    return None
                if yn:
                    modo = "a"
            return li, modo, path
        else:
            return None

    def tg_exportar_DB(self):
        resp = self.tg_exportar("lcg")
        if not resp:
            return
        li, modo, path = resp

        if modo == "w" and Util.existeFichero(path):
            Util.borraFichero(path)
            Util.borraFichero(path+"_s1")

        dlTmp = QTVarios.ImportarFicheroDB(self)
        dlTmp.show()

        dbn = DBgames.DBgames(path)
        dbn.appendDB(self.dbGames, li, dlTmp)

    def tg_create(self):
        database = QTVarios.createDB(self, self.configuracion, False)
        if database:
            self.changeDBgames(database)

    def tg_exportar_PGN(self):
        li = self.listaSelected()
        if li:
            w = PantallaSavePGN.WSaveVarios(self, self.configuracion)
            if w.exec_():
                ws = PantallaSavePGN.FileSavePGN(self, w.dic_result)
                if ws.open():
                    pb = QTUtil2.BarraProgreso1(self, _("Saving..."), formato1="%p%")
                    pb.mostrar()
                    pb.ponTotal(len(li))
                    for n, recno in enumerate(li):
                        pb.pon(n)
                        pgn, result = self.dbGames.leePGNRecno(recno)
                        if pb.siCancelado():
                            break
                        if n > 0 or not ws.is_new:
                            ws.write("\n\n")
                        pgn = pgn.strip().replace("e.p.", "").replace("#+", "#")
                        if result in ( "*", "1-0", "0-1", "1/2-1/2"):
                            if not pgn.endswith(result):
                                pgn += " " + result
                        ws.write(pgn + "\n\n")

                    pb.close()
                    ws.close()

    def tg_importar_PGN(self):
        files = QTVarios.select_pgns(self)
        if not files:
            return None

        dlTmp = QTVarios.ImportarFicheroPGN(self)
        dlTmp.show()
        self.dbGames.leerPGNs(files, dlTmp)

        self.actualiza(True)
        self.wsummary.reset()

    def tg_importar_DB(self):
        path = QTVarios.select_ext(self, "lcg")
        if not path:
            return None

        dlTmp = QTVarios.ImportarFicheroDB(self)
        dlTmp.show()

        dbn = DBgames.DBgames(path)
        self.dbGames.appendDB(dbn, range(dbn.all_reccount()), dlTmp)

        self.actualiza(True)
        self.wsummary.reset()

    def changeDBgames(self, pathFich):
        self.configuracion.ficheroDBgames = pathFich
        self.configuracion.graba()
        self.winBookGuide.cambiaDBgames(pathFich)
        self.setNameToolBar()
        self.limpiaColumnas()
        self.actualiza(True)

    def damePGNtemporal(self, wowner):
        total = self.dbGames.reccount()
        if total < 1:
            return None
        fichTemporalPGN = self.configuracion.ficheroTemporal("pgn")
        pb = QTUtil2.BarraProgreso1(wowner, _("Creating temporary PGN file..."), formato1="%p%")
        pb.mostrar()
        pb.ponTotal(total)

        with open(fichTemporalPGN, "wb") as q:
            for recno in range(self.dbGames.reccount()):
                pb.pon(recno)
                if pb.siCancelado():
                    fichTemporalPGN = None
                    break
                pgn, result = self.dbGames.leePGNRecno(recno)
                if result:
                    result = result.replace(" ", "")
                    if result in ("1-0", "0-1", "1/2-1/2"):
                        jg = pgn.strip().replace("e.p.", "").replace("#+", "#")
                        if not jg.endswith(result):
                            jg += " " + result
                        q.write(jg + "\n\n")
            pb.close()
            return fichTemporalPGN
