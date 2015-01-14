# -*- coding: latin-1 -*-

import os
import codecs

from PyQt4 import QtGui

import Code.Util as Util
import Code.Partida as Partida
import Code.TrListas as TrListas
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Colocacion as Colocacion
import Code.QT.QTVarios as QTVarios
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Grid as Grid
import Code.QT.Columnas as Columnas
import Code.QT.PantallaPGN as PantallaPGN
import Code.QT.PantallaBooks as PantallaBooks

class WGames(QtGui.QWidget):
    def __init__(self, procesador, winBookGuide, dbGames, wsummary, siMoves=True, siFEN=False):
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

        self.liFiltro = []
        self.siFiltro = False

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 70, siCentrado=True)
        liBasic = dbGames.liCamposBase
        ancho = 70
        for clave in liBasic:
            rotulo = TrListas.pgnLabel(clave)
            siCentrado = clave != "EVENT"
            if siFEN:
                ancho = 140 if clave == "FEN" else 70  # para que sirva con WBG_GamesFEN
            oColumnas.nueva(clave, rotulo, ancho, siCentrado=siCentrado)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True, id="wgames")

        # Status bar
        self.status = QtGui.QStatusBar(self)
        self.btLeerMas = Controles.PB(self, _("Show more games"), rutina=self.leerMas, plano=False)
        self.btLeerMas.setFixedHeight(18)
        self.status.addPermanentWidget(self.btLeerMas)
        self.status.setFixedHeight(22)

        # ToolBar
        liAccionesWork = [
            ( _("Close"), Iconos.MainMenu(), self.tw_terminar ), None,
            ( _("New"), Iconos.Nuevo(), self.tw_nuevo ), None,
            ( _("Edit"), Iconos.Modificar(), self.tw_editar ), None,
            ( _("First"), Iconos.Inicio(), self.tw_gotop ), None,
            ( _("Last"), Iconos.Final(), self.tw_gobottom ), None,
            ( _("Filter"), Iconos.Filtrar(), self.tw_filtrar ), None,
            ( _("Remove"), Iconos.Borrar(), self.tw_borrar ), None, None,
            ( _("Change"), Iconos.DatabaseF() if siFEN else Iconos.DatabaseC(), self.tg_change ), None,
            ( _("Import"), Iconos.MasDoc(), self.tg_importar ), None,
            ( _("Export"), Iconos.Grabar(), self.tg_exportar ), None,
        ]
        if not siFEN:
            liAccionesWork.extend([( _("Utilities"), Iconos.Utilidades(), self.tw_utilidades ), None])

        self.tbWork = Controles.TBrutina(self, liAccionesWork, tamIcon=24)

        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#4E5A65").ponTipoLetra(
            puntos=16)
        lyNT = Colocacion.H().control(self.lbName)
        if not siMoves:
            self.lbName.hide()

        lyTB = Colocacion.H().control(self.tbWork).relleno()  # .control(self.tbGen)

        layout = Colocacion.V().otro(lyNT).otro(lyTB).control(self.grid).control(self.status).margen(1)

        self.setLayout(layout)

        self.setNameToolBar()

    def limpiaColumnas(self):
        for col in self.grid.oColumnas.liColumnas:
            cab = col.cabecera
            if cab[-1] in "+-":
                col.cabecera = col.antigua

    def tw_utilidades(self):
        menu = QTVarios.LCMenu(self)

        ico = Iconos.PuntoAzul()
        icoT = Iconos.Tacticas()

        menu.separador()
        menu1 = menu.submenu(_("Polyglot book"), ico)

        menu1.opcion("crear", _("Create a new book"), ico)
        menu1.separador()
        menu1.opcion("unir", _("Merge two books in one"), ico)

        menu.separador()
        menu.opcion("crearTactic", _("Create tactics training"),
                    icoT)  # Genera fichero de tacticas con todos los movimientos incluso desde el principio

        # menu.separador()
        # menu.opcion( "masivo", _( "Mass analysis" ), icoN )

        # menu.separador()
        # eti  = _( "Play like a grandmaster" )
        # menu.opcion( "gm", _X( _('Create training to %1'), eti ), Iconos.GranMaestro() )

        resp = menu.lanza()
        if resp:
            if resp == "crear":
                PantallaBooks.polyglotCrear(self)
            elif resp == "unir":
                PantallaBooks.polyglotUnir(self)
            elif resp == "crearTactic":
                self.crearTactic()
                # elif resp == "masivo":
                # self.masivo()
                # elif resp == "gm":
                # self.gm()

    def setdbGames(self, dbGames):
        self.dbGames = dbGames

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove

    def setNameToolBar(self):
        nomFichero = self.dbGames.rotulo()
        self.lbName.ponTexto(nomFichero)
        self.wsummary.showActiveName(nomFichero)

    def leerMas(self):
        pb = QTUtil2.BarraProgreso1(self, _X(_("Reading %1"), "..."))
        pb.mostrar()
        total = self.summaryActivo["games"]
        pb.ponTotal(total)
        registros = self.dbGames.reccount()
        pb.pon(registros)
        self.updateStatus()

        while not pb.siCancelado():
            siPtes = self.dbGames.leerMasRegistros()
            registros = self.dbGames.reccount()
            pb.pon(registros)
            self.updateStatus()
            if not siPtes:
                break

        pb.cerrar()

    def updateStatus(self):
        if not self.summaryActivo:
            siPte = False
            txt = ""
        else:
            leidos = self.dbGames.reccount()
            siPte = self.dbGames.siFaltanRegistrosPorLeer()
            mv = self.summaryActivo["move"]
            txt = ""
            if mv:
                txt += mv + " | "
            if self.siFiltro:
                regs = "?" if siPte else str(leidos)
                tit = "%s(%s)" % (_("Games"), _("Filter")[0])
            else:
                regs = str(self.summaryActivo["games"] if siPte else leidos)
                tit = _("Games")

            txt += "%s : %s" % (tit, regs)

            if siPte:
                txt += " | %s : %d" % (_("Read"), leidos)
        self.status.showMessage(txt, 0)
        self.btLeerMas.setVisible(siPte)

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

    def tw_terminar(self):
        self.winBookGuide.terminar()

    def actualiza(self, siObligatorio=False):
        summaryActivo = self.wsummary.movActivo()
        if siObligatorio or self.summaryActivo != summaryActivo or self.liFiltro:
            self.siFiltro = False
            self.summaryActivo = summaryActivo

            pv = self.summaryActivo["pv"] if self.summaryActivo else ""
            self.dbGames.filterPV(pv)
            self.numJugada = pv.count(" ")
            self.grid.refresh()
            self.grid.gotop()
            self.updateStatus()
        recno = self.grid.recno()
        if recno >= 0:
            self.gridCambiadoRegistro(None, recno, None)
        if self.gridNumDatos(None) == 0:
            self.btLeerMas.setVisible(False)

    def gridCambiadoRegistro(self, grid, fila, oCol):
        pv = self.dbGames.damePV(fila)
        p = Partida.Partida()
        p.leerPV(pv)
        p.compruebaFinal()
        self.infoMove.modoPartida(p, self.numJugada)
        self.setFocus()
        self.grid.setFocus()

    def tw_gobottom(self):
        self.grid.gobottom()

    def tw_gotop(self):
        self.grid.gotop()

    def tw_nuevo(self):
        # Se genera un PGN
        pgn = self.dbGames.blankPGN()

        nuevoPGN, pv, dicPGN = self.procesador.gestorUnPGN(self, pgn)
        if nuevoPGN:
            if not self.dbGames.cambiarUno(None, nuevoPGN, pv, dicPGN):
                QTUtil2.mensError(self, _("This game already exists."))
            else:
                self.wsummary.reset()
                self.actualiza()
                self.grid.gobottom()

    def tw_editar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            recno = li[0]
            # Se genera un PGN
            pgn = self.dbGames.leePGNrecno(recno)

            nuevoPGN, pv, dicPGN = self.procesador.gestorUnPGN(self, pgn)
            if nuevoPGN:
                if not self.dbGames.cambiarUno(recno, nuevoPGN, pv, dicPGN):
                    QTUtil2.mensError(self, _("This game already exists."))
                else:
                    self.grid.refresh()
                    self.updateStatus()

    def tw_filtrar(self):
        w = PantallaPGN.WFiltrar(self, self.grid.oColumnas, self.liFiltro)
        if w.exec_():
            self.liFiltro = w.liFiltro
            self.siFiltro = True

            where = w.where()
            self.dbGames.filterPV(self.summaryActivo["pv"] if self.summaryActivo else None, where)
            self.grid.refresh()
            self.grid.gotop()
            self.updateStatus()
            self.siFiltro = len(where) > 0

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

            um.final()

    def tg_change(self):
        pathFich = QTUtil2.leeCreaFichero(self, os.path.dirname(self.configuracion.ficheroDBgames), "lcg",
                                          _("Database of complete games"))
        if pathFich:
            if not pathFich.lower().endswith(".lcg"):
                pathFich += ".lcg"
            path = os.path.dirname(pathFich)
            if os.path.isdir(path):
                self.changeDBgames(pathFich)

    def tg_exportar(self):
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

            # Fichero donde a?adir
            pathPGN = QTUtil2.salvaFichero(self, _("Export"), self.configuracion.dirSalvados,
                                           _("File") + " pgn (*.pgn)",
                                           False)
            if pathPGN:
                carpeta, nomf = os.path.split(pathPGN)
                if carpeta != self.configuracion.dirSalvados:
                    self.configuracion.dirSalvados = carpeta
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
                    QTUtil2.mensError(self, "%s : %s\n" % ( _("Unable to save"), pathPGN.replace("/", "\\") ))
                    return

                pb = QTUtil2.BarraProgreso1(self, _("Exporting..."))
                pb.mostrar()
                total = len(li)
                pb.ponTotal(total)

                if modo == "a":
                    fpgn.write("\n\n")
                for recno in li:
                    pgn = Util.blob2var(self.dbGames.field(recno, "PGN"))
                    pb.pon(recno + 1)
                    if pb.siCancelado():
                        break
                    fpgn.write(pgn)
                    fpgn.write("\n\n")

                fpgn.close()
                pb.cerrar()
                QTUtil2.mensaje(self, _X(_("Saved to %1"), pathPGN.replace("/", "\\")))

    def tg_importar(self):
        # Elegimos el fichero PGN
        path = QTUtil2.leeFichero(self, self.configuracion.dirPGN, "pgn")
        if not path:
            return None
        carpeta, fichero = os.path.split(path)
        if self.configuracion.dirPGN != carpeta:
            self.configuracion.dirPGN = carpeta
            self.configuracion.graba()

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
        fw = open(fichTemporal, "wb")
        dbf = self.dbGames.dbf
        for i in range(dbf.reccount()):
            dbf.goto(i)
            result = dbf.reg.RESULT
            if result:
                result = result.replace(" ", "")
                if result in ( "1-0", "0-1", "1/2-1/2" ):
                    jugadas = Util.blob2var(dbf.reg.PGN)
                    jg = jugadas.strip().replace("e.p.", "").replace("#+", "#")
                    if not jg.endswith(result):
                        jg += " " + result
                    fw.write(jg + "\n\n")
        fw.close()
        return fichTemporal

    def crearTactic(self):
        dbf = self.dbGames.dbf

        def rutinaDatos(recno):
            dic = {}
            dbf.goto(recno)
            for clave in dbf.liCampos:
                dic[clave] = getattr(dbf.reg, clave)
            dic["PGN"] = Util.blob2var(dic["PGN"])
            return dic

        liRegistros = self.grid.recnosSeleccionados()
        if len(liRegistros) < 2:
            liRegistros = range(dbf.reccount())

        PantallaPGN.crearTactic(self.procesador, self, liRegistros, rutinaDatos)
