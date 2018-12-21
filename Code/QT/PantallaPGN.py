import codecs
import copy
import os

from PyQt4 import QtCore, QtGui

from Code import ControlPosicion
from Code import GM
from Code import PGN
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaBooks
from Code.QT import PantallaAnalisisParam
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import PantallaSavePGN
from Code import SQL
from Code import TrListas
from Code import Util
from Code import VarGen


class EstadoWpgn:
    def guarda(self, window, dbf):
        # dbf
        self.select = dbf.select
        self.condicion = dbf.condicion
        self.orden = dbf.orden

        # grid
        grid = window.grid
        self.recno = grid.recno()
        # self.dicVideoGrid = {}
        # grid.guardarVideo(self.dicVideoGrid)

        # ventana
        self.pos = window.pos()
        self.tam = window.size()

        self.liOrdenClaves = window.liOrdenClaves
        self.liFiltro = window.liFiltro

    def recuperaDBF(self, dbf):
        dbf.select = self.select
        dbf.condicion = self.condicion
        dbf.orden = self.orden
        dbf.leer()

    def recuperaWindow(self, window):
        window.resize(self.tam)
        window.move(self.pos)


class WElegir(QTVarios.WDialogo):
    def __init__(self, owner, dbf, dClaves, gestor, estado, siElegir):
        titulo = _("Choose a game to view") if siElegir else _("PGN viewer")
        icono = Iconos.PGN()
        extparam = "pgnelegir"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.siElegir = siElegir
        self.gestor = gestor
        self.dClaves = dClaves = copy.deepcopy(dClaves)
        self.seHaBorradoAlgo = False  # Para que se haga un touch antiguo al DB y lo regenere la proxima vez
        siRepite = estado is not None
        if siRepite:
            self.estado = estado
        else:
            self.estado = EstadoWpgn()
        self.dbf = dbf
        if siRepite:
            self.estado.recuperaDBF(dbf)
        else:
            self.dbf.leer()

        # Filtro
        self.liFiltro = self.estado.liFiltro if siRepite else []

        # Status bar-> antes que grid porque se actualiza en gridNumDatos
        self.status = QtGui.QStatusBar(self)
        self.status.setFixedHeight(22)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 50, siCentrado=True)

        def creaCol(clave, rotulo, siCentrado=True):
            tam = max(dClaves[clave], len(rotulo), 11)
            oColumnas.nueva(clave, rotulo, tam * 6, siCentrado=siCentrado)

        # # Claves segun orden estandar
        liBasic = ("EVENT", "SITE", "DATE", "ROUND", "WHITE", "BLACK", "RESULT", "ECO", "FEN", "WHITEELO", "BLACKELO")
        self.liOrdenClaves = []  # nos servira en el exterior, para paste pgn y para mostrar info
        for clave in liBasic:
            if clave in dClaves:
                rotulo = TrListas.pgnLabel(clave)
                creaCol(clave, rotulo, clave != "EVENT")
                self.liOrdenClaves.append(clave)
        for clave in dClaves:
            if clave.upper() not in liBasic:
                rotulo = TrListas.pgnLabel(clave)
                creaCol(clave.upper(), rotulo, clave != "EVENT")
                self.liOrdenClaves.append(clave.upper())

        # dicVideoGrid = self.estado.dicVideoGrid if siRepite else None
        # self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, dicVideo=dicVideoGrid, siSeleccionMultiple=True)
        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.registrarGrid(self.grid)

        if siRepite:
            self.grid.goto(self.estado.recno, 0)
        else:
            self.grid.gotop()

        # Toolbar
        if siElegir:
            liAcciones = [(_("Choose"), Iconos.Aceptar(), self.elegir), None,
                          (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                          (_("First"), Iconos.Inicio(), self.grid.gotop),
                          (_("Last"), Iconos.Final(), self.grid.gobottom), None,
                          (_("Filter"), Iconos.Filtrar(), self.filtrar), None,
                          ]
        else:
            liAcciones = [
                (_("Close"), Iconos.MainMenu(), self.cancelar), None,
                (_("View"), Iconos.Ver(), self.elegir), None,
                (_("Edit"), Iconos.Modificar(), self.editar), None,
                (_("Save"), Iconos.Grabar(), self.guardar), None,
                (_("First"), Iconos.Inicio(), self.grid.gotop),
                (_("Last"), Iconos.Final(), self.grid.gobottom), None,
                (_("Filter"), Iconos.Filtrar(), self.filtrar), None,
                (_("Remove"), Iconos.Borrar(), self.borrar), None,
                (_("Utilities"), Iconos.Utilidades(), self.utilidades), None
            ]
        tb = QTVarios.LCTB(self, liAcciones)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).control(self.status).margen(3)
        self.setLayout(layout)

        self.recuperarVideo(siTam=False)

        if siRepite:
            self.estado.recuperaWindow(self)
        else:
            n = self.grid.anchoColumnas() + 20
            w, h = QTUtil.tamEscritorio()
            self.resize(min(w * 9 / 10, n), h * 8 / 10)

        self.ponStatus()

    def ponStatus(self):
        txt = "%s: %d" % (_("Games"), self.dbf.reccount())
        if self.dbf.condicion:
            txt += " | %s: %s" % (_("Filter"), self.dbf.condicion)
        self.status.showMessage(txt, 0)

    def closeEvent(self, event):  # Cierre con X
        self.guardarVideo()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def editar(self):
        li = self.grid.recnosSeleccionados()
        if li:
            recno = li[0]
            # Se genera un PGN
            pgn = self.dbf.leeOtroCampo(recno, "PGN")
            if pgn:
                nuevoPGN, pv, dicPGN = self.gestor.procesador.gestorUnPGN(self, pgn)
                if nuevoPGN:
                    reg = self.dbf.baseRegistro()
                    for k in dicPGN:
                        setattr(reg, k.upper(), dicPGN[k])
                    reg.PGN = nuevoPGN
                    self.dbf.modificarReg(recno, reg)
                    self.grid.refresh()

    def elegir(self):
        self.guardarVideo()
        f, c = self.grid.posActualN()
        if 0 <= f < self.dbf.reccount():
            self.dbf.goto(f)
            self.estado.guarda(self, self.dbf)
            self.accept()

    def gridDobleClick(self, grid, fila, oColumna):
        self.elegir()

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave
        orden = self.dbf.orden
        if clave == "PLIES":
            siDesc = orden.endswith("DESC")
            siPrevio = orden.startswith("CAST")
            orden = "CAST(PLIES AS INTEGER)"
            if siPrevio:
                if siDesc:
                    orden = ""
                else:
                    orden += " DESC"
        elif orden.startswith(clave):
            siDesc = orden.endswith("DESC")
            if siDesc:
                orden = ""
            else:
                orden += " DESC"
        else:
            orden = clave
        self.dbf.orden = orden

        self.dbf.leer()
        self.grid.refresh()

    def gridNumDatos(self, grid):
        return self.dbf.reccount()

    def gridWheelEvent(self, quien, siAdelante):
        f, c = self.grid.posActualN()
        f += -1 if siAdelante else +1
        if 0 <= f < self.dbf.reccount():
            self.grid.goto(f, c)

    def gridDato(self, grid, fila, oColumna):
        self.dbf.goto(fila)
        clave = oColumna.clave
        if clave == "numero":
            return str(fila + 1)
        valor = getattr(self.dbf, oColumna.clave)
        if valor is None:
            valor = ""
        if clave.endswith("DATE"):
            valor = valor.replace(".??", "")
        if "?" in valor:
            valor = valor.replace("?", "")
        return valor

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        if k in (QtCore.Qt.Key_Enter, QtCore.Qt.Key_Return):
            self.elegir()

    def filtrar(self):
        w = WFiltrar(self, self.grid.oColumnas, self.liFiltro)
        if w.exec_():
            self.liFiltro = w.liFiltro
            self.dbf.condicion = w.where()
            self.dbf.leer()
            self.grid.refresh()
            self.grid.gotop()
            self.ponStatus()

    def guardar(self):
        nrecs = self.dbf.reccount()
        if nrecs == 0:
            return
        elif nrecs > 1:
            menu = QTVarios.LCMenu(self)
            menu.opcion("all", _("All games"), Iconos.PuntoNaranja())
            menu.separador()
            menu.opcion("selected", _("Selected games"), Iconos.PuntoAzul())
            resp = menu.lanza()
            if resp is None:
                return
            elif resp == "all":
                liSelected = range(nrecs)
            else:
                liSelected = self.grid.recnosSeleccionados()
        else:
            liSelected = [0]

        w = PantallaSavePGN.WSaveVarios(self, self.gestor.configuracion)
        if w.exec_():
            ws = PantallaSavePGN.FileSavePGN(self, w.dic_result)
            if ws.open():
                ws.um()

                antSelect = self.dbf.select
                nueSelect = antSelect + ",PGN"
                self.dbf.ponSelect(nueSelect)
                self.dbf.leer()
                self.dbf.gotop()

                for i in liSelected:
                    self.dbf.goto(i)
                    dic = self.dbf.dicValores()
                    pgn = dic["PGN"]
                    if i > 0 or not ws.is_new:
                        ws.write("\n\n")
                    ws.write(pgn)

                ws.close()
                self.dbf.ponSelect(antSelect)
                ws.um_final()

    def borrar(self):

        li = self.grid.recnosSeleccionados()
        if li:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.dbf.borrarLista(li)

                self.seHaBorradoAlgo = True  # Para que se haga un touch antiguo al DB y lo regenere
                self.dbf.leer()
                self.grid.refresh()

                self.grid.goto(li[-1], 0)
                self.ponStatus()

    def damePGNtemporal(self, wowner):
        fichTemporal = VarGen.configuracion.ficheroTemporal("pgn")
        fw = open(fichTemporal, "wb")
        dbf = self.dbf
        for i in range(dbf.reccount()):
            dbf.goto(i)
            dic = dbf.dicValores()
            result = dic["RESULT"]
            if result:
                result = result.replace(" ", "")
                if result in ("1-0", "0-1", "1/2-1/2"):
                    jugadas = dbf.leeOtroCampo(i, "PGN")
                    jg = jugadas.strip().replace("e.p.", "").replace("#+", "#")
                    if not jg.endswith(result):
                        jg += " " + result
                    fw.write(jg + "\n\n")
        fw.close()
        return fichTemporal

    def utilidades(self):
        menu = QTVarios.LCMenu(self)

        ico = Iconos.PuntoAzul()
        icoV = Iconos.PuntoVerde()
        icoN = Iconos.PuntoNaranja()
        icoT = Iconos.Tacticas()

        menu1 = menu.submenu(_("Polyglot book"), ico)

        menu1.opcion("crear", _("Create a new book"), ico)
        menu1.opcion("unir", _("Merge two books in one"), ico)

        menu.separador()
        menu.opcion("crearTactic", _("Create tactics training"),
                    icoT)  # Genera fichero de tacticas con todos los movimientos incluso desde el principio

        if "FEN" in self.liOrdenClaves:
            menu.separador()
            menu.opcion("crearFNS", _("List of FENs"), icoV)

        menu.separador()
        menu.opcion("masivo", _("Mass analysis"), icoN)

        menu.separador()
        eti = _("Play like a Grandmaster")
        menu.opcion("gm", _X(_('Create training to %1'), eti), Iconos.GranMaestro())

        resp = menu.lanza()
        if resp:
            if resp == "crear":
                PantallaBooks.polyglotCrear(self)
            elif resp == "unir":
                PantallaBooks.polyglotUnir(self)
            elif resp == "crearFNS":
                self.crearFNS()
            elif resp == "crearTactic":
                self.crearTactic()
            elif resp == "masivo":
                self.masivo()
            elif resp == "gm":
                self.gm()

    def gm(self):
        liSelec = self.grid.recnosSeleccionados()

        # Datos
        liGen = [(None, None)]

        liGen.append((_("Name") + ":", ""))

        liGen.append(("<div align=\"right\">" + _("Only player moves") + ":<br>%s</div>" % _(
                "(You can add multiple aliases separated by ; and wildcards with * )"), ""))

        liGen.append((_("Only selected games") + ":", len(liSelec) > 1))

        li = [1, (0, _("Both sides")), (1, _("Only the winning side")), (2, _("The winning side and both if drawn"))]

        liGen.append((_("Which side") + ":", li))

        eti = _("Play like a Grandmaster")
        eti = _X(_('Create training to %1'), eti)
        resultado = FormLayout.fedit(liGen, title=eti, parent=self, anchoMinimo=460, icon=Iconos.GranMaestro())

        if not resultado:
            return
        accion, liGen = resultado
        nombre = liGen[0]
        jugador = liGen[1]
        siSelec = liGen[2]
        result = liGen[3]

        if not nombre:
            return

        liJugadores = jugador.upper().split(";") if jugador else None

        # Se crea el objeto de ejecucion
        fgm = GM.FabGM(self.gestor.configuracion, nombre, liJugadores)

        # Se pasan todas las partidas
        if not siSelec:
            liSelec = range(self.dbf.reccount())

        nregs = len(liSelec)
        mensaje = _("Game") + "  %d/" + str(nregs)
        tmpBP = QTUtil2.BarraProgreso(self, eti, "", nregs).mostrar()

        for n, recno in enumerate(liSelec):

            if tmpBP.siCancelado():
                break

            self.dbf.goto(recno)

            if n:
                tmpBP.pon(n)
            tmpBP.mensaje(mensaje % (n + 1,))

            jugadas = self.dbf.leeOtroCampo(n, "PGN")

            pgn = PGN.UnPGN()
            pgn.leeTexto(jugadas)

            fgm.masMadera(pgn, pgn.partida, result)

        siCancelado = tmpBP.siCancelado()
        tmpBP.cerrar()

        if not siCancelado:
            # Se ejecuta
            siCreado = fgm.xprocesa()

            if siCreado:
                liCreados = [nombre]
                liNoCreados = None
            else:
                liNoCreados = [nombre]
                liCreados = None
            mensajeEntrenamientos(self, liCreados, liNoCreados)

    def masivo(self):

        liSeleccionadas = self.grid.recnosSeleccionados()
        nSeleccionadas = len(liSeleccionadas)

        alm = PantallaAnalisisParam.paramAnalisisMasivo(self, self.gestor.configuracion, nSeleccionadas > 1)
        if alm:

            if alm.siVariosSeleccionados:
                nregs = nSeleccionadas
            else:
                nregs = self.dbf.reccount()

            tmpBP = QTUtil2.BarraProgreso2(self, _("Mass analysis"), formato2="%p%")
            tmpBP.ponTotal(1, nregs)
            tmpBP.ponRotulo(1, _("Game"))
            tmpBP.ponRotulo(2, _("Moves"))
            tmpBP.mostrar()

            import Code.Analisis

            ap = Code.Analisis.AnalizaPartida(self.gestor.procesador, alm, True)

            for n in range(nregs):

                if tmpBP.siCancelado():
                    break

                tmpBP.pon(1, n + 1)

                if alm.siVariosSeleccionados:
                    n = liSeleccionadas[n]

                self.dbf.goto(n)
                self.grid.goto(n, 0)

                jugadas = self.dbf.leeOtroCampo(n, "PGN")

                pgn = PGN.UnPGN()
                pgn.leeTexto(jugadas)

                ap.xprocesa(pgn.dic, pgn.partida, tmpBP, jugadas)

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
                mensajeEntrenamientos(self, liCreados, liNoCreados)

            else:
                ap.terminar(False)

            tmpBP.cerrar()

    def crearFNS(self):

        configuracion = self.gestor.configuracion
        Util.creaCarpeta(configuracion.dirPersonalTraining)
        resp = QTUtil2.salvaFichero(self, _("File to save"), configuracion.dirPersonalTraining,
                                    _("List of FENs") + " (*.fns)", False)
        if not resp:
            return
        modo = "w"
        if Util.existeFichero(resp):
            yn = QTUtil2.preguntaCancelar(self, _X(_("The file %1 already exists, what do you want to do?"), resp),
                                          si=_("Append"), no=_("Overwrite"))
            if yn is None:
                return
            if yn:
                modo = "a"
        f = codecs.open(resp, modo, 'utf-8', 'ignore')

        liSeleccionadas = self.grid.recnosSeleccionados()
        nSeleccionadas = len(liSeleccionadas)
        siSeleccionadas = nSeleccionadas > 1

        if siSeleccionadas:
            nregs = nSeleccionadas
        else:
            nregs = self.dbf.reccount()

        tmpBP = QTUtil2.BarraProgreso(self, _("List of FENs"), _("Game"), nregs)
        tmpBP.mostrar()

        # eventAnt = ""

        for n in range(nregs):

            if tmpBP.siCancelado():
                break

            tmpBP.pon(n + 1)

            if siSeleccionadas:
                n = liSeleccionadas[n]

            self.dbf.goto(n)
            self.grid.goto(n, 0)

            pgn = self.dbf.leeOtroCampo(n, "PGN")
            li = []
            grabar = False
            for linea in pgn.split("\n"):
                if not grabar:
                    if not linea.startswith("["):
                        grabar = True
                if grabar:
                    li.append(linea.strip())
            jugadas = " ".join(li)
            while "  " in jugadas:
                jugadas = jugadas.replace("  ", " ")

            dic = self.dbf.dicValores()

            def xdic(k):
                x = dic.get(k, "")
                if x is None:
                    x = ""
                elif "?" in x:
                    x = x.replace(".?", "").replace("?", "")
                return x.strip()

            fen = dic["FEN"]
            if not fen:
                if not jugadas.strip("*"):
                    continue
                fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

            event = xdic("EVENT")
            site = xdic("SITE")
            date = xdic("DATE")
            if site == event:
                es = event
            else:
                es = event + " " + site
            es = es.strip()
            if date:
                if es:
                    es += " (%s)" % date
                else:
                    es = date
            white = xdic("WHITE")
            black = xdic("BLACK")
            wb = ("%s-%s" % (white, black)).strip("-")
            titulo = ""
            if es:
                titulo += "<br>%s" % es
            if wb:
                titulo += "<br>%s" % wb

            for other in ("TASK", "SOURCE"):
                v = xdic(other)
                if v:
                    titulo += "<br>%s" % v

            txt = fen + "|%s|%s\n" % (titulo, jugadas.strip())

            f.write(txt)

        f.close()
        tmpBP.cerrar()
        QTUtil2.mensaje(self, "%s" % _X(_("Saved to %1"), resp))
        self.gestor.procesador.entrenamientos.rehaz()

    def crearTactic(self):
        def rutinaDatos(recno):
            dic = {"PGN": self.dbf.leeOtroCampo(recno, "PGN").strip()}
            self.dbf.goto(recno)
            for clave in self.dClaves:
                clave = clave.upper()
                dic[clave] = getattr(self.dbf, clave)
            return dic

        liRegistros = self.grid.recnosSeleccionados()
        if len(liRegistros) < 2:
            liRegistros = range(self.dbf.reccount())

        crearTactic(self.gestor.procesador, self, liRegistros, rutinaDatos)


class WFiltrar(QtGui.QDialog):
    def __init__(self, wParent, oColumnas, liFiltro, dbSaveNom=None):
        super(WFiltrar, self).__init__()

        if dbSaveNom is None:
            dbSaveNom = VarGen.configuracion.ficheroFiltrosPGN

        self.setWindowTitle(_("Filter"))
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
        self.setWindowIcon(Iconos.Filtrar())

        self.liFiltro = liFiltro
        nFiltro = len(liFiltro)
        self.dbSaveNom = dbSaveNom

        liCampos = [(x.cabecera, x.clave) for x in oColumnas.liColumnas if x.clave != "numero"]
        liCampos.insert(0, ("", None))
        liCondicion = [("", None),
                       (_("Equal"), "="),
                       (_("Not equal"), "<>"),
                       (_("Greater than"), ">"),
                       (_("Less than"), "<"),
                       (_("Greater than or equal"), ">="),
                       (_("Less than or equal"), "<="),
                       (_("Like (wildcard = *)"), "LIKE"),
                       (_("Not like (wildcard = *)"), "NOT LIKE")]

        liUnion = [("", None),
                   (_("AND"), "AND"),
                   (_("OR"), "OR")]

        f = Controles.TipoLetra(puntos=12)  # 0, peso=75 )

        lbCol = Controles.LB(self, _("Column")).ponFuente(f)
        lbPar0 = Controles.LB(self, "(").ponFuente(f)
        lbPar1 = Controles.LB(self, ")").ponFuente(f)
        lbCon = Controles.LB(self, _("Condition")).ponFuente(f)
        lbVal = Controles.LB(self, _("Value")).ponFuente(f)
        lbUni = Controles.LB(self, "+").ponFuente(f)

        ly = Colocacion.G()
        ly.controlc(lbUni, 0, 0).controlc(lbPar0, 0, 1).controlc(lbCol, 0, 2).controlc(lbCon, 0, 3).controlc(lbVal, 0,
                                                                                                             4).controlc(
                lbPar1, 0, 5)

        self.numC = 8
        liC = []

        union, par0, campo, condicion, valor, par1 = None, False, None, None, "", False
        for i in range(self.numC):
            if i > 0:
                c_union = Controles.CB(self, liUnion, union)
                ly.controlc(c_union, i + 1, 0)
            else:
                c_union = None

            c_par0 = Controles.CHB(self, "", par0).anchoFijo(20)
            ly.controlc(c_par0, i + 1, 1)
            c_campo = Controles.CB(self, liCampos, campo)
            ly.controlc(c_campo, i + 1, 2)
            c_condicion = Controles.CB(self, liCondicion, condicion)
            ly.controlc(c_condicion, i + 1, 3)
            c_valor = Controles.ED(self, valor)
            ly.controlc(c_valor, i + 1, 4)
            c_par1 = Controles.CHB(self, "", par1).anchoFijo(20)
            ly.controlc(c_par1, i + 1, 5)

            liC.append((c_union, c_par0, c_campo, c_condicion, c_valor, c_par1))

        self.liC = liC

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.reject), None,
                      (_("Reinit"), Iconos.Reiniciar(), self.reiniciar), None,
                      (_("Save/Restore"), Iconos.Grabar(), self.grabar), None
                      ]

        tb = QTVarios.LCTB(self, liAcciones)

        # Layout
        layout = Colocacion.V().control(tb).otro(ly).margen(3)
        self.setLayout(layout)

        liC[0][2].setFocus()

        if nFiltro > 0:
            self.lee_filtro(self.liFiltro)

    def grabar(self):
        if not self.lee_filtro_actual():
            return
        with Util.DicSQL(self.dbSaveNom, tabla="Filters") as dbc:
            liConf = dbc.keys(siOrdenados=True)
            if len(liConf) == 0 and len(self.liFiltro) == 0:
                return
            menu = Controles.Menu(self)
            SELECCIONA, BORRA, GRABA = range(3)
            for x in liConf:
                menu.opcion((SELECCIONA, x), x, Iconos.PuntoAzul())
            menu.separador()

            if len(self.liFiltro) > 0:
                submenu = menu.submenu(_("Save current"), Iconos.Mas())
                if liConf:
                    for x in liConf:
                        submenu.opcion((GRABA, x), x, Iconos.PuntoAmarillo())
                submenu.separador()
                submenu.opcion((GRABA, None), _("New"), Iconos.NuevoMas())

            if liConf:
                menu.separador()
                submenu = menu.submenu(_("Remove"), Iconos.Delete())
                for x in liConf:
                    submenu.opcion((BORRA, x), x, Iconos.PuntoRojo())
            resp = menu.lanza()

            if resp:
                op, nombre = resp

                if op == SELECCIONA:
                    liFiltro = dbc[nombre]
                    self.lee_filtro(liFiltro)
                elif op == BORRA:
                    if QTUtil2.pregunta(self, _X(_("Delete %1 ?"), nombre)):
                        del dbc[nombre]
                elif op == GRABA:
                    if self.lee_filtro_actual():
                        if nombre is None:
                            liGen = [FormLayout.separador]
                            liGen.append((_("Name") + ":", ""))

                            resultado = FormLayout.fedit(liGen, title=_("Filter"), parent=self, icon=Iconos.Libre())
                            if resultado:
                                accion, liGen = resultado

                                nombre = liGen[0].strip()
                                if nombre:
                                    dbc[nombre] = self.liFiltro
                        else:
                            dbc[nombre] = self.liFiltro

    def lee_filtro(self, liFiltro):
        self.liFiltro = liFiltro
        nFiltro = len(liFiltro)

        for i in range(self.numC):
            if nFiltro > i:
                union, par0, campo, condicion, valor, par1 = liFiltro[i]
            else:
                union, par0, campo, condicion, valor, par1 = None, False, None, None, "", False
            c_union, c_par0, c_campo, c_condicion, c_valor, c_par1 = self.liC[i]
            if c_union:
                c_union.ponValor(union)
            c_par0.ponValor(par0)
            c_campo.ponValor(campo)
            c_condicion.ponValor(condicion)
            c_valor.ponTexto(valor)
            c_par1.ponValor(par1)

    def reiniciar(self):
        for i in range(self.numC):
            self.liC[i][1].ponValor(False)
            self.liC[i][2].setCurrentIndex(0)
            self.liC[i][3].setCurrentIndex(0)
            self.liC[i][4].ponTexto("")
            self.liC[i][5].ponValor(False)
            if i > 0:
                self.liC[i][0].setCurrentIndex(0)
        self.aceptar()

    def lee_filtro_actual(self):
        self.liFiltro = []

        npar = 0

        for i in range(self.numC):
            par0 = self.liC[i][1].valor()
            campo = self.liC[i][2].valor()
            condicion = self.liC[i][3].valor()
            valor = self.liC[i][4].texto().rstrip()
            par1 = self.liC[i][5].valor()

            if campo and condicion:
                if campo == "PLIES":
                    valor = valor.strip()
                    if valor.isdigit():
                        valor = "%d" % int(valor)  # fonkap patch %3d -> %d
                if par0:
                    npar += 1
                if par1:
                    npar -= 1
                if npar < 0:
                    break
                if i > 0:
                    union = self.liC[i][0].valor()
                    if union:
                        self.liFiltro.append([union, par0, campo, condicion, valor, par1])
                else:
                    self.liFiltro.append([None, par0, campo, condicion, valor, par1])
            else:
                break
        if npar:
            QTUtil2.mensError(self, _("The parentheses are unbalanced."))
            return False
        return True

    def aceptar(self):
        if self.lee_filtro_actual():
            self.accept()

    def where(self):
        where = ""
        for union, par0, campo, condicion, valor, par1 in self.liFiltro:
            valor = valor.upper()
            if condicion in ("LIKE", "NOT LIKE"):
                valor = valor.replace("*", "%")
                if "%" not in valor:
                    valor = "%" + valor + "%"

            if union:
                where += " %s " % union
            if par0:
                where += "("
            if condicion in ("=", "<>") and not valor:
                where += "(( %s %s ) OR (%s %s ''))" % (
                    campo, "IS NULL" if condicion == "=" else "IS NOT NULL", campo, condicion)
            else:
                valor = valor.upper()
                if valor.isupper():
                    where += "UPPER(%s) %s '%s'" % (campo, condicion, valor)  # fonkap patch
                elif valor.isdigit():  # fonkap patch
                    where += "CAST(%s as decimal) %s %s" % (campo, condicion, valor)  # fonkap patch
                else:
                    where += "%s %s '%s'" % (campo, condicion, valor)  # fonkap patch
            if par1:
                where += ")"
        return where


class EM_SQL(Controles.EM):
    def __init__(self, owner, where, liCampos):
        self.liCampos = liCampos
        Controles.EM.__init__(self, owner, where, siHTML=False)

    def mousePressEvent(self, event):
        Controles.EM.mousePressEvent(self, event)
        if event.button() == QtCore.Qt.RightButton:
            menu = QTVarios.LCMenu(self)
            rondo = QTVarios.rondoPuntos()
            for txt, key in self.liCampos:
                menu.opcion(key, txt, rondo.otro())
            resp = menu.lanza()
            if resp:
                self.insertarTexto(resp)


class WFiltrarRaw(QTVarios.WDialogo):
    def __init__(self, wParent, oColumnas, where):
        QtGui.QDialog.__init__(self, wParent)

        QTVarios.WDialogo.__init__(self, wParent, _("Filter"), Iconos.Filtrar(), "rawfilter")

        self.where = ""
        liCampos = [(x.cabecera, x.clave) for x in oColumnas.liColumnas if x.clave != "numero"]

        f = Controles.TipoLetra(puntos=12)  # 0, peso=75 )

        lbRaw = Controles.LB(self, "%s:"%_("Raw SQL")).ponFuente(f)
        self.edRaw = EM_SQL(self, where, liCampos).altoFijo(72).anchoMinimo(512).ponFuente(f)

        lbHelp = Controles.LB(self, _("Right button to select a column of database")).ponFuente(f)
        lyHelp = Colocacion.H().relleno().control(lbHelp).relleno()

        ly = Colocacion.H().control(lbRaw).control(self.edRaw)

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.reject), None,
                      ]
        tb = QTVarios.LCTB(self, liAcciones)

        # Layout
        layout = Colocacion.V().control(tb).otro(ly).otro(lyHelp).margen(3)
        self.setLayout(layout)

        self.edRaw.setFocus()

        self.recuperarVideo(siTam=False)

    def aceptar(self):
        self.where = self.edRaw.texto()
        self.guardarVideo()
        self.accept()


def elegirPGN(owner, dbf, dClaves, gestor, estado, siElegir=False):
    w = WElegir(owner, dbf, dClaves, gestor, estado, siElegir)
    if w.exec_():
        return True, w.estado, w.seHaBorradoAlgo
    return False, None, w.seHaBorradoAlgo


def eligePartida(ventana, path=None):
    configuracion = VarGen.configuracion
    # Elegimos el fichero PGN
    if path is None:
        path = QTUtil2.leeFichero(ventana, configuracion.dirPGN, "pgn")
        if not path:
            return None
    carpeta, fichero = os.path.split(path)
    if configuracion.dirPGN != carpeta:
        configuracion.dirPGN = carpeta
        configuracion.graba()

    # Lo importamos
    fpgn = PGN.PGN(configuracion)

    dicDB = fpgn.leeFichero(ventana, path)
    if dicDB is None:
        return None

    # Los datos los tenemos en una BD.sql
    dClaves = dicDB["DCLAVES"]

    bd = SQL.Base.DBBase(dicDB["PATHDB"])

    dbf = bd.dbf("GAMES", ",".join(dClaves.keys()) + ",PGN")

    siSeguir, estadoWpgn, siSeHaBorradoAlgo = elegirPGN(ventana, dbf, dClaves, None, None, siElegir=True)
    if siSeguir:
        reg = dbf.dicValores()

    dbf.cerrar()

    bd.cerrar()

    if not siSeguir:
        return None

    unpgn = PGN.UnPGN()
    unpgn.leeTexto(reg["PGN"])

    return unpgn


def mensajeEntrenamientos(owner, liCreados, liNoCreados):
    txt = ""
    if liCreados:
        txt += _("Created the following trainings") + ":"
        txt += "<ul>"
        for x in liCreados:
            txt += "<li>%s</li>" % os.path.basename(x)
        txt += "</ul>"
    if liNoCreados:
        txt += _("No trainings created due to lack of data") + ":"
        txt += "<ul>"
        for x in liNoCreados:
            txt += "<li>%s</li>" % os.path.basename(x)
        txt += "</ul>"
    QTUtil2.mensaje(owner, txt)


def crearTactic(procesador, wowner, liRegistros, rutinaDatos):
    # Se pide el nombre de la carpeta
    liGen = [(None, None)]

    liGen.append((_("Name") + ":", ""))

    liGen.append((None, None))

    liJ = [(_("Default"), 0), (_("White"), 1), (_("Black"), 2)]
    config = FormLayout.Combobox(_("Point of view"), liJ)
    liGen.append((config, 0))

    eti = _("Create tactics training")
    resultado = FormLayout.fedit(liGen, title=eti, parent=wowner, anchoMinimo=460, icon=Iconos.Tacticas())

    if not resultado:
        return
    accion, liGen = resultado
    menuname = liGen[0].strip()
    if not menuname:
        return
    pointview = str(liGen[1])

    restDir = Util.validNomFichero(menuname)
    nomDir = os.path.join(VarGen.configuracion.dirPersonalTraining, "Tactics", restDir)
    nomIni = os.path.join(nomDir, "Config.ini")
    if os.path.isfile(nomIni):
        dicIni = Util.ini8dic(nomIni)
        n = 1
        while True:
            if "TACTIC%d" % n in dicIni:
                if "MENU" in dicIni["TACTIC%d" % n]:
                    if dicIni["TACTIC%d" % n]["MENU"].upper() == menuname.upper():
                        break
                else:
                    break
                n += 1
            else:
                break
        nomTactic = "TACTIC%d" % n
    else:
        nomDirTac = os.path.join(VarGen.configuracion.dirPersonalTraining, "Tactics")
        Util.creaCarpeta(nomDirTac)
        Util.creaCarpeta(nomDir)
        nomTactic = "TACTIC1"
        dicIni = {}
    nomFNS = os.path.join(nomDir, "Puzzles.fns")
    if os.path.isfile(nomFNS):
        n = 1
        nomFNS = os.path.join(nomDir, "Puzzles-%d.fns")
        while os.path.isfile(nomFNS % n):
            n += 1
        nomFNS = nomFNS % n

    # Se crea el fichero con los puzzles
    f = codecs.open(nomFNS, "w", "utf-8", 'ignore')

    nregs = len(liRegistros)
    tmpBP = QTUtil2.BarraProgreso(wowner, menuname, _("Game"), nregs)
    tmpBP.mostrar()

    fen0 = ControlPosicion.FEN_INICIAL

    for n in range(nregs):

        if tmpBP.siCancelado():
            break

        tmpBP.pon(n + 1)

        recno = liRegistros[n]

        dicValores = rutinaDatos(recno)
        plies = dicValores["PLIES"]
        if plies == 0:
            continue

        pgn = dicValores["PGN"]
        li = pgn.split("\n")
        if len(li) == 1:
            li = pgn.split("\r")
        li = [linea for linea in li if not linea.strip().startswith("[")]
        jugadas = " ".join(li).replace("\r", "").replace("\n", "")
        if not jugadas.strip("*"):
            continue

        def xdic(k):
            x = dicValores.get(k, "")
            if x is None:
                x = ""
            elif "?" in x:
                x = x.replace(".?", "").replace("?", "")
            return x.strip()

        fen = dicValores.get("FEN")
        if not fen:
            fen = fen0

        event = xdic("EVENT")
        site = xdic("SITE")
        date = xdic("DATE")
        if site == event:
            es = event
        else:
            es = event + " " + site
        es = es.strip()
        if date:
            if es:
                es += " (%s)" % date
            else:
                es = date
        white = xdic("WHITE")
        black = xdic("BLACK")
        wb = ("%s-%s" % (white, black)).strip("-")
        titulo = ""
        if es:
            titulo += "<br>%s" % es
        if wb:
            titulo += "<br>%s" % wb

        for other in ("TASK", "SOURCE"):
            v = xdic(other)
            if v:
                titulo += "<br>%s" % v

        txt = fen + "|%s|%s\n" % (titulo, jugadas)

        f.write(txt)

    f.close()
    tmpBP.cerrar()

    # Se crea el fichero de control
    dicIni[nomTactic] = d = {}
    d["MENU"] = menuname
    d["FILESW"] = "%s:100" % os.path.basename(nomFNS)
    d["POINTVIEW"] = pointview

    Util.dic8ini(nomIni, dicIni)

    QTUtil2.mensaje(wowner, _X(_("Tactic training %1 created."), menuname) + "<br>" +
                    _X(_("You can access this training from menu Trainings-Learn tactics by repetition-%1"),
                       restDir))

    procesador.entrenamientos.rehaz()
