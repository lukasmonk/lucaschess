import codecs
import os

from PyQt4 import QtGui, QtCore

from Code import Partida
from Code import DBgamesFEN
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaPGN
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Voyager
from Code.QT import PantallaSolo
from Code import TrListas
from Code import Util


class WGamesFEN(QtGui.QWidget):
    def __init__(self, procesador, winBookGuide, dbGamesFEN):
        QtGui.QWidget.__init__(self)

        self.winBookGuide = winBookGuide
        self.dbGamesFEN = dbGamesFEN
        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.infoMove = None  # <-- setInfoMove
        self.numJugada = 0  # Se usa para indicarla al mostrar el pgn en infoMove

        self.terminado = False # singleShot

        self.liFiltro = []
        self.where = None

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 70, siCentrado=True)
        liBasic = dbGamesFEN.liCamposBase
        for clave in liBasic:
            rotulo = TrListas.pgnLabel(clave)
            siCentrado = clave != "EVENT"

            ancho = 140 if clave == "FEN" else 70  # para que sirva con WBG_GamesFEN
            oColumnas.nueva(clave, rotulo, ancho, siCentrado=siCentrado)
        oColumnas.nueva("rowid", _("Row ID"), 70, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True, xid="wgamesfen")

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

        self.tbWork = QTVarios.LCTB(self, liAccionesWork, tamIcon=24, puntos=procesador.configuracion.puntosTB)

        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#4E5A65").ponTipoLetra(puntos=16)
        lyNT = Colocacion.H().control(self.lbName)

        lyTB = Colocacion.H().control(self.tbWork)

        layout = Colocacion.V().otro(lyNT).otro(lyTB).control(self.grid).control(self.status).margen(1)

        self.setLayout(layout)

        self.setNameToolBar()

    def reread(self):
        self.dbGamesFEN.lee_rowids()
        self.grid.refresh()

    def limpiaColumnas(self):
        for col in self.grid.oColumnas.liColumnas:
            cab = col.cabecera
            if cab[-1] in "+-":
                col.cabecera = col.antigua

    def setdbGames(self, dbGamesFEN):
        self.dbGamesFEN = dbGamesFEN

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove
        self.graphicBoardReset()

    def setNameToolBar(self):
        nomFichero = self.dbGamesFEN.rotulo()
        self.lbName.ponTexto(nomFichero)

    def updateStatus(self):
        if self.terminado:
            return
        self.dbGamesFEN.setFilter(self.where)

    def gridNumDatos(self, grid):
        return self.dbGamesFEN.reccount()

    def gridDato(self, grid, nfila, ocol):
        clave = ocol.clave
        if clave == "numero":
            return str(nfila + 1)
        elif clave == "rowid":
            return str(self.dbGamesFEN.getROWID(nfila))
        return self.dbGamesFEN.field(nfila, clave)

    def gridDobleClick(self, grid, fil, col):
        self.tw_editar()

    def gridDobleClickCabecera(self, grid, col):
        liOrden = self.dbGamesFEN.dameOrden()
        clave = col.clave
        if clave == "numero":
            return
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
        self.dbGamesFEN.ponOrden(liOrden)
        self.grid.refresh()
        self.updateStatus()

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        if k in (QtCore.Qt.Key_Enter, QtCore.Qt.Key_Return):
            self.tw_editar()

    def closeEvent(self, event):
        self.tw_terminar()

    def tw_terminar(self):
        self.terminado = True
        self.dbGamesFEN.close()
        self.winBookGuide.terminar()

    def actualiza(self, siObligatorio=False):
        if siObligatorio or self.liFiltro:
            self.where = None
            self.updateStatus()
            self.grid.refresh()
            self.grid.gotop()
        recno = self.grid.recno()
        if recno >= 0:
            self.gridCambiadoRegistro(None, recno, None)

    def gridCambiadoRegistro(self, grid, fila, oCol):
        fen, pv = self.dbGamesFEN.dameFEN_PV(fila)
        p = Partida.Partida(fen=fen)
        p.leerPV(pv)
        p.siTerminada()
        self.infoMove.modoFEN(p, fen, -1)
        self.setFocus()
        self.grid.setFocus()

    def tw_filtrar(self):
        w = PantallaPGN.WFiltrar(self, self.grid.oColumnas, self.liFiltro)
        if w.exec_():
            self.liFiltro = w.liFiltro

            self.where = w.where()
            self.dbGamesFEN.setFilter(self.where)
            self.grid.refresh()
            self.grid.gotop()
            self.updateStatus()

    def tw_gobottom(self):
        self.grid.gobottom()

    def tw_gotop(self):
        self.grid.gotop()

    def tw_up(self):
        fila = self.grid.recno()
        filaNueva = self.dbGamesFEN.intercambia(fila, True)
        if filaNueva is not None:
            self.grid.goto(filaNueva, 0)
            self.grid.refresh()

    def tw_down(self):
        fila = self.grid.recno()
        filaNueva = self.dbGamesFEN.intercambia(fila, False)
        if filaNueva is not None:
            self.grid.goto(filaNueva, 0)
            self.grid.refresh()

    def editar(self, recno, partidaCompleta):
        partidaCompleta = self.procesador.gestorPartida(self, partidaCompleta, False, self.infoMove.tablero)
        if partidaCompleta is not None:
            if not self.dbGamesFEN.guardaPartidaRecno(recno, partidaCompleta):
                QTUtil2.mensError(self, _("This game already exists."))
            else:
                self.actualiza(True)
                if recno is None:
                    self.grid.gobottom()
                else:
                    self.grid.goto(recno, 0)
                    self.gridCambiadoRegistro(self, recno, None)

    def tw_nuevo(self):
        fen = Voyager.voyagerFEN(self, "", False)
        if fen is not None:
            if self.dbGamesFEN.si_existe_fen(fen):
                QTUtil2.mensError(self, _("This position already exists."))
                return
            hoy = Util.hoy()
            liTags=[['Date', "%d.%02d.%02d" % (hoy.year, hoy.month, hoy.day)],['FEN', fen]]
            pc = Partida.PartidaCompleta(fen=fen, liTags=liTags)
            self.editar(None, pc)

    def tw_editar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            recno = li[0]
            partidaCompleta = self.dbGamesFEN.leePartidaRecno(recno)

            if partidaCompleta is not None:
                self.editar(recno, partidaCompleta)
            else:
                QTUtil2.mensaje(self, _("This game is wrong and can not be edited"))

    def tw_borrar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            if not QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                return

            um = QTUtil2.unMomento(self)

            self.dbGamesFEN.borrarLista(li)
            self.grid.refresh()
            self.updateStatus()

            um.final()

    def tw_configure(self):
        siShow = self.dbGamesFEN.recuperaConfig("GRAPHICS_SHOW_ALLWAYS", False)
        siGraphicsSpecific = self.dbGamesFEN.recuperaConfig("GRAPHICS_SPECIFIC", False)
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
        showAllways = self.dbGamesFEN.recuperaConfig("GRAPHICS_SHOW_ALLWAYS")
        specific = self.dbGamesFEN.recuperaConfig("GRAPHICS_SPECIFIC")
        return showAllways, specific

    def graphicBoardReset(self):
        showAllways, specific = self.readVarsConfig()
        fichGraphic = self.dbGamesFEN.nomFichero if specific else None
        self.infoMove.tablero.dbVisual_setFichero(fichGraphic)
        self.infoMove.tablero.dbVisual_setShowAllways(showAllways)

    def tw_dir_show_yes(self):
        self.dbGamesFEN.guardaConfig("GRAPHICS_SHOW_ALLWAYS", True)
        self.graphicBoardReset()

    def tw_dir_show_no(self):
        self.dbGamesFEN.guardaConfig("GRAPHICS_SHOW_ALLWAYS", False)
        self.graphicBoardReset()

    def tw_locale_yes(self):
        self.dbGamesFEN.guardaConfig("GRAPHICS_SPECIFIC", True)
        self.graphicBoardReset()

    def tw_locale_no(self):
        self.dbGamesFEN.guardaConfig("GRAPHICS_SPECIFIC", False)
        self.graphicBoardReset()

    def tw_utilities(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion(self.tw_massive_change_tags, _("Massive change of tags"), Iconos.PGN())
        menu.separador()
        menu.opcion(self.tw_uti_tactic, _("Create tactics training"), Iconos.Tacticas())
        resp = menu.lanza()
        if resp:
            resp()

    def tw_uti_tactic(self):
        def rutinaDatos(recno):
            dic = {}
            for clave in self.dbGamesFEN.liCamposBase:
                dic[clave] = self.dbGamesFEN.field(recno, clave)
            p = self.dbGamesFEN.leePartidaRecno(recno)
            dic["PGN"] = p.pgn()
            return dic

        liRegistros = self.grid.recnosSeleccionados()
        if len(liRegistros) < 2:
            liRegistros = range(self.dbGamesFEN.reccount())

        PantallaPGN.crearTactic(self.procesador, self, liRegistros, rutinaDatos)

    def tg_file(self):
        menu = QTVarios.LCMenu(self)

        lista = QTVarios.listaDB(self.configuracion, True)
        if lista:
            smenu = menu.submenu( _("Open another database"), Iconos.DatabaseC())
            rp = QTVarios.rondoPuntos()
            for fich in lista:
                smenu.opcion(os.path.join(self.configuracion.carpetaPositions, fich), _F(fich[:-4]), rp.otro())
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

        resp = menu.lanza()
        if resp:
            if type(resp) == str:
                self.changeDBgames(resp)
            else:
                resp()

    def tg_importar_pks(self):
        path_pks = QTUtil2.leeFichero(self, self.configuracion.dirJS, "pks")
        if path_pks:
            direc = os.path.dirname(path_pks)
            if direc != self.configuracion.dirJS:
                self.configuracion.dirJS = direc
                self.configuracion.graba()

            mens_error = self.dbGamesFEN.insert_pks(path_pks)
            if mens_error:
                QTUtil2.mensError(self, mens_error)
                return
            self.actualiza(True)
            self.grid.gobottom(0)

    def tw_massive_change_tags(self):
        resp = PantallaSolo.massive_change_tags(self, self.configuracion, len(self.grid.recnosSeleccionados()), False)
        if resp:
            recno = self.grid.recno()
            liTags, remove, overwrite, si_all = resp
            liRegistros = range(self.dbGamesFEN.reccount()) if si_all else self.grid.recnosSeleccionados()
            nRegistros = len(liRegistros)
            if (nRegistros == 1 or
                ((nRegistros > 1) and
                 QTUtil2.pregunta(self, _("Are you sure do you want to change the %d registers?" % nRegistros)))):
                um = QTUtil2.unMomento(self)
                self.dbGamesFEN.massive_change_tags(liTags, liRegistros, remove, overwrite)
                self.reread()
                self.grid.goto(recno, 0)
                um.final()

    def tg_create(self):
        database = QTVarios.createDB(self, self.configuracion, True)
        if database:
            self.changeDBgames(database)

    def tg_exportar(self, ext):
        li = self.grid.recnosSeleccionados()
        if not li:
            return None
        menu = QTVarios.LCMenu(self)
        menu.opcion(True, _("All read"), Iconos.PuntoVerde())
        menu.separador()
        menu.opcion(False, "%s [%d]"%(_("Only selected"),len(li)), Iconos.PuntoAzul())
        siTodos = menu.lanza()
        if siTodos is None:
            return None

        if siTodos:
            li = range(self.dbGamesFEN.reccount())

        # Fichero donde a?adir
        path = QTUtil2.salvaFichero(self, _("Export"), self.configuracion.dirSalvados,
                                      _("File") + " %s (*.%s)"%(ext, ext),
                                      False)
        if path:
            if not path.lower().endswith(".%s"%ext):
                path += ".%s"%ext
            carpeta, nomf = os.path.split(path)
            if carpeta != self.configuracion.dirSalvados:
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
        resp = self.tg_exportar("lcf")
        if not resp:
            return
        li, modo, path = resp

        if modo == "w" and Util.existeFichero(path):
            Util.borraFichero(path)

        dlTmp = QTVarios.ImportarFicheroDB(self)
        dlTmp.ponExportados()
        dlTmp.show()

        dbn = DBgamesFEN.DBgamesFEN(path)
        dbn.appendDB(self.dbGamesFEN, li, dlTmp)

    def tg_exportar_PGN(self):
        resp = self.tg_exportar("pgn")
        if not resp:
            return
        li, modo, path = resp

        try:
            fpgn = codecs.open(path, modo, 'utf-8', 'ignore')
        except:
            QTUtil2.mensError(self, "%s : %s\n" % (_("Unable to save"), path))
            return

        pb = QTUtil2.BarraProgreso1(self, _("Exporting..."))
        pb.mostrar()
        total = len(li)
        pb.ponTotal(total)

        if modo == "a":
            fpgn.write("\n\n")
        for n, recno in enumerate(li):
            p = self.dbGamesFEN.leePartidaRecno(recno)
            pb.pon(n + 1)
            if pb.siCancelado():
                break
            fpgn.write(p.pgn())
            fpgn.write("\n\n")

        fpgn.close()
        pb.cerrar()
        QTUtil2.mensaje(self, _X(_("Saved to %1"), path))

    def tg_importar_PGN(self):
        path = QTVarios.select_pgn(self)
        if not path:
            return None

        dlTmp = QTVarios.ImportarFicheroPGN(self)
        dlTmp.show()
        self.dbGamesFEN.leerPGN(path, dlTmp)

        self.actualiza(True)

    def tg_importar_DB(self):
        path = QTVarios.select_ext(self, "lcf")
        if path:
            dlTmp = QTVarios.ImportarFicheroDB(self)
            dlTmp.show()

            dbn = DBgamesFEN.DBgamesFEN(path)
            dbn.lee_rowids()
            liRecnos = range(dbn.reccount())
            self.dbGamesFEN.appendDB(dbn, liRecnos, dlTmp)

            self.actualiza(True)

    def changeDBgames(self, pathFich):
        self.configuracion.ficheroDBgamesFEN = pathFich
        self.configuracion.graba()
        self.winBookGuide.cambiaDBgames(pathFich)
        self.setNameToolBar()
        self.limpiaColumnas()
        self.actualiza(True)

