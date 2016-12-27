import codecs
import os

from PyQt4 import QtGui, QtCore

from Code import Partida
from Code import DBgames
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaBooks
from Code.QT import PantallaPGN
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import TrListas
from Code import Util

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

        self.liFiltro = []
        self.where = None

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 70, siCentrado=True)
        liBasic = dbGames.liCamposBase
        ancho = 70
        for clave in liBasic:
            rotulo = TrListas.pgnLabel(clave)
            siCentrado = clave != "EVENT"
            oColumnas.nueva(clave, rotulo, ancho, siCentrado=siCentrado)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True, xid="wgames")

        # Status bar
        self.status = QtGui.QStatusBar(self)
        self.status.setFixedHeight(22)

        # ToolBar
        liAccionesWork = [
            (_("Close"), Iconos.MainMenu(), self.tw_terminar), None,
            (_("Database"), Iconos.DatabaseC(), self.tg_file), None,
            (_("New"), Iconos.Nuevo(), self.tw_nuevo, _("Add a new game")), None,
            (_("Edit"), Iconos.Modificar(), self.tw_editar), None,
            (_("First"), Iconos.Inicio(), self.tw_gotop), None,
            (_("Last"), Iconos.Final(), self.tw_gobottom), None,
            (_("Filter"), Iconos.Filtrar(), self.tw_filtrar), None,
            (_("Remove"), Iconos.Borrar(), self.tw_borrar),None,
        ]

        self.tbWork = Controles.TBrutina(self, liAccionesWork, tamIcon=24)

        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#4E5A65").ponTipoLetra(puntos=16)
        lyNT = Colocacion.H().control(self.lbName)
        if not siMoves:
            self.lbName.hide()

        lyTB = Colocacion.H().control(self.tbWork)

        layout = Colocacion.V().otro(lyNT).otro(lyTB).control(self.grid).control(self.status).margen(1)

        self.setLayout(layout)

        self.setNameToolBar()

    def limpiaColumnas(self):
        for col in self.grid.oColumnas.liColumnas:
            cab = col.cabecera
            if cab[-1] in "+-":
                col.cabecera = col.antigua

    def setdbGames(self, dbGames):
        self.dbGames = dbGames

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove

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
        return self.dbGames.field(nfila, clave)

    def gridDobleClick(self, grid, fil, col):
        self.tw_editar()

    def gridDobleClickCabecera(self, grid, col):
        liOrden = self.dbGames.dameOrden()
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
        self.dbGames.ponOrden(liOrden)
        self.grid.refresh()
        self.updateStatus()

    def closeEvent(self):
        self.tw_terminar()

    def tw_terminar(self):
        self.terminado = True
        self.dbGames.close()
        self.winBookGuide.terminar()

    def actualiza(self, siObligatorio=False):
        summaryActivo = self.wsummary.movActivo()
        if siObligatorio or self.summaryActivo != summaryActivo or self.liFiltro:
            self.where = None
            self.summaryActivo = summaryActivo
            pv = ""
            if self.summaryActivo:
                alm = self.summaryActivo.get("alm")
                if alm:
                    if len(alm.LIALMS) > 1:
                        pv = [ralm.PV for ralm in alm.LIALMS]
                    else:
                        pv = self.summaryActivo.get("pv")
                        if pv:
                            lipv = pv.split(" ")
                            pv = " ".join(lipv[:-1])
            self.dbGames.filterPV(pv)
            self.updateStatus()
            self.numJugada = pv.count(" ")
            self.grid.refresh()
            self.grid.gotop()
        recno = self.grid.recno()
        if recno >= 0:
            self.gridCambiadoRegistro(None, recno, None)

    def gridCambiadoRegistro(self, grid, fila, oCol):
        if fila >= 0:
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

    def editar(self, recno, partidaCompleta):
        partidaCompleta = self.procesador.gestorPartida(self, partidaCompleta, True)
        if partidaCompleta:
            if not self.dbGames.guardaPartidaRecno(recno, partidaCompleta):
                QTUtil2.mensError(self, _("This game already exists."))
            else:
                self.actualiza(True)

    def tw_nuevo(self):
        recno = None
        pc = self.dbGames.blankPartida()
        self.editar(recno, pc)

    def tw_editar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            recno = li[0]
            partidaCompleta = self.dbGames.leePartidaRecno(recno)
            self.editar(recno, partidaCompleta)

    def tw_filtrar(self):
        def standard():
            w = PantallaPGN.WFiltrar(self, self.grid.oColumnas, self.liFiltro)
            if w.exec_():
                self.liFiltro = w.liFiltro

                self.where = w.where()
                self.dbGames.filterPV(self.summaryActivo["pv"] if self.summaryActivo else None, self.where)
                self.grid.refresh()
                self.grid.gotop()
                self.updateStatus()

        def raw_sql():
            w = PantallaPGN.WFiltrarRaw(self, self.grid.oColumnas, self.where)
            if w.exec_():
                self.where = w.where
                self.dbGames.filterPV(self.summaryActivo["pv"] if self.summaryActivo else None, self.where)
                self.grid.refresh()
                self.grid.gotop()
                self.updateStatus()

        def opening():
            me = QTUtil2.unMomento(self)
            import Code.QT.PantallaAperturas as PantallaAperturas
            w = PantallaAperturas.WAperturas(self, self.configuracion, "")
            me.final()
            if w.exec_():
                ap = w.resultado()
                pv = getattr(ap, "a1h8", "")
                self.dbGames.filterPV(pv)
                self.numJugada = pv.count(" ")
                self.grid.gotop()
                self.grid.refresh()
                self.updateStatus()
                self.gridCambiadoRegistro(None, 0, 0)

        menu = QTVarios.LCMenu(self)
        menu.opcion(standard, _("Standard"), Iconos.Filtrar())
        menu.separador()
        menu.opcion(raw_sql, _("Advanced"), Iconos.SQL_RAW())
        menu.separador()
        menu.opcion(opening, _("Opening"), Iconos.Apertura())
        menu.separador()
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

        menu.opcion(self.tg_createDB, _("Create a new database"), Iconos.NuevaDB())
        menu.separador()
        menu.opcion(self.tg_change, _("Open another database"), Iconos.DatabaseC())
        menu.separador()

        submenu = menu.submenu(_("Import from"), Iconos.DatabaseCNew())
        submenu.opcion(self.tg_importar_PGN, _("A PGN file"), Iconos.FichPGN())
        submenu.separador()
        submenu.opcion(self.tg_importar_DB, _("Other database"), Iconos.DatabaseC())
        menu.separador()

        submenu = menu.submenu(_("Export to"), Iconos.DatabaseMas())
        submenu.opcion(self.tg_exportar_PGN, _("A PGN file"), Iconos.FichPGN())
        submenu.separador()
        submenu.opcion(self.tg_exportar_DB, _("Other database"), Iconos.DatabaseC())
        menu.separador()

        submenu = menu.submenu(_("Utilities"), Iconos.Utilidades())
        ico = Iconos.PuntoAzul()
        icoT = Iconos.Tacticas()
        menu1 = submenu.submenu(_("Polyglot book"), ico)
        menu1.opcion(self.tw_uti_pcreate, _("Create a new book"), ico)
        menu1.separador()
        menu1.opcion(self.tw_uti_pmerge, _("Merge two books in one"), ico)
        submenu.separador()
        submenu.opcion(self.tw_uti_tactic, _("Create tactics training"), icoT)

        resp = menu.lanza()
        if resp:
            resp()

    def tw_uti_pcreate(self):
        PantallaBooks.polyglotCrear(self)

    def tw_uti_pmerge(self):
        PantallaBooks.polyglotUnir(self)

    def tw_uti_tactic(self):
        def rutinaDatos(recno):
            dic = {}
            for clave in self.dbGames.liCamposBase:
                dic[clave] = self.dbGames.field(recno, clave)
            dic["PGN"] = self.dbGames.leePGNrecno(recno)
            return dic

        liRegistros = self.grid.recnosSeleccionados()
        if len(liRegistros) < 2:
            liRegistros = range(self.dbGames.reccount())

        PantallaPGN.crearTactic(self.procesador, self, liRegistros, rutinaDatos)

    def tg_change(self):
        pathFich = QTUtil2.leeFichero(self, os.path.dirname(self.configuracion.ficheroDBgames), "lcg",
                                          _("Database of complete games"))
        if pathFich:
            if not pathFich.lower().endswith(".lcg"):
                pathFich += ".lcg"
            path = os.path.dirname(pathFich)
            if os.path.isdir(path):
                self.changeDBgames(pathFich)

    def tg_createDB(self):
        pathFich = QTUtil2.creaFichero(self, os.path.dirname(self.configuracion.ficheroDBgames), "lcg",
                                          _("Database of complete games"))
        if pathFich:
            if not pathFich.lower().endswith(".lcg"):
                pathFich += ".lcg"
            path = os.path.dirname(pathFich)
            if os.path.isdir(path):
                Util.borraFichero(pathFich)
                Util.borraFichero(pathFich+"-st1")
                self.changeDBgames(pathFich)

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
            li = range(self.dbGames.reccount())

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

    def tg_exportar_PGN(self):
        li = self.grid.recnosSeleccionados()
        if li:
            if len(li) > 1:
                menu = QTVarios.LCMenu(self)
                menu.opcion(True, _("All read"), Iconos.PuntoVerde())
                menu.separador()
                menu.opcion(False, _("Only selected"), Iconos.PuntoAzul())
                resp = menu.lanza()
                if resp is None:
                    return
                siTodos = resp
            else:
                siTodos = True

            if siTodos:
                li = range(self.dbGames.reccount())

            # Fichero donde aadir
            pathPGN = QTUtil2.salvaFichero(self, _("Export"), self.configuracion.dirPGN,
                                           _("File") + " pgn (*.pgn)",
                                           False)
            if pathPGN:
                carpeta, nomf = os.path.split(pathPGN)
                if carpeta != self.configuracion.dirPGN:
                    self.configuracion.dirPGN = carpeta
                    self.configuracion.graba()

                # Grabamos
                modo = "w"
                if Util.existeFichero(pathPGN):
                    yn = QTUtil2.preguntaCancelar(self,
                                                  _X(_("The file %1 already exists, what do you want to do?"), pathPGN),
                                                  si=_("Append"), no=_("Overwrite"))
                    if yn is None:
                        return
                    if yn:
                        modo = "a"
                try:
                    fpgn = codecs.open(pathPGN, modo, 'utf-8', 'ignore')
                except:
                    QTUtil2.mensError(self, "%s : %s\n" % (_("Unable to save"), pathPGN.replace("/", "\\")))
                    return

                pb = QTUtil2.BarraProgreso1(self, _("Exporting..."))
                pb.mostrar()
                total = len(li)
                pb.ponTotal(total)

                if modo == "a":
                    fpgn.write("\n\n")
                for n, recno in enumerate(li):
                    pgn = self.dbGames.leePGNRecno(recno)
                    pb.pon(n + 1)
                    if pb.siCancelado():
                        break
                    fpgn.write(pgn)
                    fpgn.write("\n\n")

                fpgn.close()
                pb.cerrar()
                QTUtil2.mensaje(self, _X(_("Saved to %1"), pathPGN.replace("/", "\\")))

    def tg_importar_PGN(self):
        path = QTVarios.select_pgn(self)
        if not path:
            return None

        dlTmp = QTVarios.ImportarFicheroPGN(self)
        dlTmp.show()
        self.dbGames.leerPGN(path, dlTmp)

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

    def tg_importarFEN(self):
        # Elegimos el fichero PGN
        path = QTVarios.select_pgn(self)
        if not path:
            return None

        dlTmp = QTVarios.ImportarFicheroPGN(self)
        dlTmp.show()
        self.dbGames.leerPGN(path, dlTmp)

        self.actualiza(True)

    def changeDBgames(self, pathFich):
        self.configuracion.ficheroDBgames = pathFich
        self.configuracion.graba()
        self.winBookGuide.cambiaDBgames(pathFich)
        self.setNameToolBar()
        self.limpiaColumnas()
        self.actualiza(True)

    def tg_create(self):
        resp = self.tg_nombre_depth(_("New"))

        if resp:
            nombre, depth = resp
            self.changeDBgames(nombre)

    def damePGNtemporal(self):
        # Llamado desde PantallaBooks al crear el poliglot
        fichTemporal = self.configuracion.ficheroTemporal("pgn")
        with open(fichTemporal, "wb") as q:
            for recno in range(self.dbGames.reccount()):
                result = self.dbGames.field(recno, "RESULT")
                if result:
                    result = result.replace(" ", "")
                    if result in ("1-0", "0-1", "1/2-1/2"):
                        pgn = self.dbGames.leePGNRecno(recno)
                        jg = pgn.strip().replace("e.p.", "").replace("#+", "#")
                        if not jg.endswith(result):
                            jg += " " + result
                        q.write(jg + "\n\n")
        return fichTemporal

