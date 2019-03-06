import LCEngine4 as LCEngine

from PyQt4 import QtGui, QtCore

from Code import Partida
from Code import AperturasStd
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import FormLayout
from Code.QT import Delegados
from Code.QT import QTVarios

OPENINGS_WHITE, OPENINGS_BLACK, MOVES_WHITE, MOVES_BLACK = range(4)


class ToolbarMoves(QtGui.QWidget):
    def __init__(self, side, rutina):
        QtGui.QWidget.__init__(self)

        self.dispatch = rutina
        self.side = side
        self.setFont(Controles.TipoLetra())

        ancho = 54

        bt_all = Controles.PB(self, _("All"), self.run_all, plano=False).anchoFijo(ancho+16)
        bt_e4 = Controles.PB(self, "e4", self.run_e4, plano=False).anchoFijo(ancho)
        bt_d4 = Controles.PB(self, "d4", self.run_d4, plano=False).anchoFijo(ancho)
        bt_c4 = Controles.PB(self, "c4", self.run_c4, plano=False).anchoFijo(ancho)
        bt_nf3 = Controles.PB(self, "Nf3", self.run_nf3, plano=False).anchoFijo(ancho)
        bt_other = Controles.PB(self, _("Others"), self.run_other, plano=False).anchoFijo(ancho+16)

        ply1 = Controles.PB(self, _("%d ply") % 1, self.run_p1, plano=False).anchoFijo(ancho)
        ply2 = Controles.PB(self, _("%d ply") % 2, self.run_p2, plano=False).anchoFijo(ancho)
        ply3 = Controles.PB(self, _("%d ply") % 3, self.run_p3, plano=False).anchoFijo(ancho)
        ply4 = Controles.PB(self, _("%d ply") % 4, self.run_p4, plano=False).anchoFijo(ancho)
        ply5 = Controles.PB(self, _("%d ply") % 5, self.run_p5, plano=False).anchoFijo(ancho)

        self.sbply = Controles.SB(self, 0, 0, 100)
        self.sbply.capturaCambiado(self.run_p)
        lbply = Controles.LB(self, _("Plies"))

        layout = Colocacion.H().relleno(1).control(bt_all)
        layout.control(bt_e4).control(bt_d4).control(bt_c4).control(bt_nf3).control(bt_other).relleno(1)
        layout.control(ply1).control(ply2).control(ply3).control(ply4).control(ply5)
        layout.control(lbply).control(self.sbply).relleno(1).margen(0)

        self.setLayout(layout)

    def run_all(self):
        self.dispatch(self.side, "all")

    def run_e4(self):
        self.dispatch(self.side, "e2e4")

    def run_d4(self):
        self.dispatch(self.side, "d2d4")

    def run_c4(self):
        self.dispatch(self.side, "c2c4")

    def run_nf3(self):
        self.dispatch(self.side, "g1f3")

    def run_other(self):
        self.dispatch(self.side, "other")

    def run_p1(self):
        self.dispatch(self.side, "p1")

    def run_p2(self):
        self.dispatch(self.side, "p2")

    def run_p3(self):
        self.dispatch(self.side, "p3")

    def run_p4(self):
        self.dispatch(self.side, "p4")

    def run_p5(self):
        self.dispatch(self.side, "p5")

    def run_p(self):
        v = self.sbply.valor()
        self.dispatch(self.side, "p%d" % v)


class WPlayer(QtGui.QWidget):
    def __init__(self, procesador, winBookGuide, dbGames):
        QtGui.QWidget.__init__(self)

        self.winBookGuide = winBookGuide
        self.procesador = procesador
        self.data = [[], [], [], []]
        self.movesWhite = []
        self.movesBlack = []
        self.lastFilterMoves = {"white":"", "black":""}
        self.configuracion = procesador.configuracion

        self.infoMove = None  # <-- setInfoMove

        self.ap = AperturasStd.ap

        self.gridOpeningWhite = self.gridOpeningBlack = self.gridMovesWhite = self.gridMovesBlack = 0

        # GridOpening
        ancho = 54
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("opening", _("Opening"), 200)
        oColumnas.nueva("games", _("Games"), ancho, siDerecha=True)
        oColumnas.nueva("pgames", "% " + _("Games"), 70, siDerecha=True)
        oColumnas.nueva("win", _("Win"), ancho, siDerecha=True)
        oColumnas.nueva("draw", _("Draw"), ancho, siDerecha=True)
        oColumnas.nueva("lost", _("Loss"), ancho, siDerecha=True)
        oColumnas.nueva("pwin", "% " + _("Win"), ancho, siDerecha=True)
        oColumnas.nueva("pdraw", "% " + _("Draw"), ancho, siDerecha=True)
        oColumnas.nueva("plost", "% " + _("Loss"), ancho, siDerecha=True)
        oColumnas.nueva("pdrawwin", "%% %s" % _("W+D"), ancho, siDerecha=True)
        oColumnas.nueva("pdrawlost", "%% %s" % _("L+D"), ancho, siDerecha=True)

        self.gridOpeningWhite = Grid.Grid(self, oColumnas, siSelecFilas=True)
        self.gridOpeningBlack = Grid.Grid(self, oColumnas, siSelecFilas=True)

        # GridWhite/GridBlack
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("games", _("Games"), ancho, siDerecha=True)
        oColumnas.nueva("win", _("Win"), ancho, siDerecha=True)
        oColumnas.nueva("draw", _("Draw"), ancho, siDerecha=True)
        oColumnas.nueva("lost", _("Loss"), ancho, siDerecha=True)
        oColumnas.nueva("pwin", "% " + _("Win"), ancho, siDerecha=True)
        oColumnas.nueva("pdraw", "% " + _("Draw"), ancho, siDerecha=True)
        oColumnas.nueva("plost", "% " + _("Loss"), ancho, siDerecha=True)

        ancho_col = 40
        siFigurinesPGN = self.configuracion.figurinesPGN
        for x in range(1, 50):
            num = (x-1)*2
            oColumnas.nueva(str(num), "%d." % x, ancho_col, siCentrado=True, edicion=Delegados.EtiquetaPOS(siFigurinesPGN, siLineas=False))
            oColumnas.nueva(str(num+1), "...", ancho_col, siCentrado=True, edicion=Delegados.EtiquetaPOS(siFigurinesPGN, siLineas=False))

        self.gridMovesWhite = Grid.Grid(self, oColumnas, siSelecFilas=True)
        self.gridMovesBlack = Grid.Grid(self, oColumnas, siSelecFilas=True)

        wWhite = QtGui.QWidget(self)
        tbmovesw = ToolbarMoves("white", self.dispatchMoves)
        ly = Colocacion.V().control(tbmovesw).control(self.gridMovesWhite).margen(3)
        wWhite.setLayout(ly)

        wblack = QtGui.QWidget(self)
        tbmovesb = ToolbarMoves("black", self.dispatchMoves)
        ly = Colocacion.V().control(tbmovesb).control(self.gridMovesBlack).margen(3)
        wblack.setLayout(ly)

        tabs = Controles.Tab(self)
        tabs.nuevaTab(self.gridOpeningWhite, _("White openings"))
        tabs.nuevaTab(self.gridOpeningBlack, _("Black openings"))
        tabs.nuevaTab(wWhite, _("White moves"))
        tabs.nuevaTab(wblack, _("Black moves"))

        # ToolBar
        liAccionesWork = [
            (_("Close"), Iconos.MainMenu(), self.tw_terminar), None,
            ("", Iconos.Usuarios(), self.tw_changeplayer), None,
            (_("Rebuild"), Iconos.Reindexar(), self.tw_rebuild), None,
        ]

        self.tbWork = Controles.TBrutina(self, liAccionesWork, tamIcon=24, puntos=12)
        self.tbWork.setToolButtonStyle(QtCore.Qt.ToolButtonTextBesideIcon)

        lyTB = Colocacion.H().control(self.tbWork)
        layout = Colocacion.V().otro(lyTB).control(tabs).margen(1)

        self.setLayout(layout)
        self.qtColor = (QTUtil.qtColorRGB(221, 255, 221), QTUtil.qtColorRGB(247, 247, 247), QTUtil.qtColorRGB(255, 217, 217))

        self.setdbGames(dbGames)
        self.setPlayer(self.leeVariable("PLAYER", ""))

    def dispatchMoves(self, side, opcion):
        dataSide = self.data[MOVES_WHITE if side == "white" else MOVES_BLACK]

        if opcion == "all":
            showData = range(len(dataSide))

        elif opcion in ("e2e4", "d2d4", "c2c4", "g1f3"):
            showData = [n for n in range(len(dataSide)) if dataSide[n]["pv"].startswith(opcion)]

        elif opcion == "other":
            showData = [n for n in range(len(dataSide))
                            if not dataSide[n]["pv"].startswith("e2e4") and
                                not dataSide[n]["pv"].startswith("d2d4") and
                                not dataSide[n]["pv"].startswith("c2c4") and
                                not dataSide[n]["pv"].startswith("g1f3")
                        ]

        else:  #if opcion.startswith("p"):
            num = int(opcion[1:])
            if num == 0:
                return self.dispatchMoves(side, "all")
            if self.lastFilterMoves[side].startswith("p"):
                showDataPrevio = range(len(dataSide))
            else:
                showDataPrevio = self.movesWhite if side == "white" else self.movesBlack
            showData = [n for n in showDataPrevio if dataSide[n]["pv"].count(" ") < num]

        if side == "white":
            self.movesWhite = showData
            self.gridMovesWhite.refresh()

        else:
            self.movesBlack = showData
            self.gridMovesBlack.refresh()

        self.lastFilterMoves[side] = opcion

    def setdbGames(self, dbGames):
        self.dbGames = dbGames
        self.setPlayer(self.leeVariable("PLAYER", ""))

    def setPlayer(self, player):
        self.player = player
        self.data = [[], [], [], []]
        accion = self.tbWork.liAcciones[1]
        accion.setIconText(self.player if self.player else _("Player"))

        self.gridOpeningWhite.refresh()
        self.gridOpeningBlack.refresh()
        self.gridMovesWhite.refresh()
        self.gridMovesBlack.refresh()

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove

    def dataGrid(self, grid):
        if grid == self.gridOpeningWhite:
            return self.data[OPENINGS_WHITE]
        elif grid == self.gridOpeningBlack:
            return self.data[OPENINGS_BLACK]
        elif grid == self.gridMovesWhite:
            return self.data[MOVES_WHITE]
        elif grid == self.gridMovesBlack:
            return self.data[MOVES_BLACK]

    def gridNumDatos(self, grid):
        if grid == self.gridOpeningWhite:
            return len(self.data[OPENINGS_WHITE])
        elif grid == self.gridOpeningBlack:
            return len(self.data[OPENINGS_BLACK])
        elif grid == self.gridMovesWhite:
            return len(self.movesWhite)
        elif grid == self.gridMovesBlack:
            return len(self.movesBlack)
        else:
            return 0

    def gridDato(self, grid, nfila, ocol):
        clave = ocol.clave
        dt = self.dataGrid(grid)
        if grid == self.gridMovesWhite:
            nfila = self.movesWhite[nfila]
        elif grid == self.gridMovesBlack:
            nfila = self.movesBlack[nfila]
        return dt[nfila][clave]

    def gridCambiadoRegistro(self, grid, nfila, oCol):
        dt = self.dataGrid(grid)
        if grid == self.gridMovesWhite:
            nfila = self.movesWhite[nfila]
        elif grid == self.gridMovesBlack:
            nfila = self.movesBlack[nfila]
        if len(dt) > nfila >= 0:
            partida = dt[nfila]["partida"]
            if partida is None:
                pv = dt[nfila]["pv"]
                partida = Partida.Partida()
                partida.leerPV(pv)
            self.infoMove.modoPartida(partida, partida.numJugadas()-1)
            self.setFocus()

    def gridColorFondo(self, grid, nfila, ocol):
        dt = self.dataGrid(grid)
        if not dt:
            return
        if grid == self.gridMovesWhite:
            nfila = self.movesWhite[nfila]
        elif grid == self.gridMovesBlack:
            nfila = self.movesBlack[nfila]
        clave = ocol.clave + "c"
        color = dt[nfila].get(clave)
        return self.qtColor[color] if color is not None else None

    def leeVariable(self, var, default=None):
        return self.dbGames.recuperaConfig(var, default)

    def escVariable(self, var, valor):
        self.dbGames.guardaConfig(var, valor)

    def listaPlayers(self):
        return self.leeVariable("LISTA_PLAYERS", [])

    def rereadPlayers(self):
        um = QTUtil2.unMomento(self)
        lista = self.dbGames.players()
        self.escVariable("LISTA_PLAYERS", lista)
        um.final()

    def changePlayer(self, lp):
        liGen = []
        lista = [(player, player) for player in lp]
        config = FormLayout.Combobox(_("Name"), lista)
        liGen.append((config, self.leeVariable("PLAYER", "")))

        listaAlias = lista[:]
        listaAlias.insert(0, ("--", ""))
        for nalias in range(1,4):
            liGen.append(FormLayout.separador)
            config = FormLayout.Combobox("%s %d" % (_("Alias"), nalias), listaAlias)
            liGen.append((config, self.leeVariable("ALIAS%d" % nalias, "")))

        resultado = FormLayout.fedit(liGen, title=_("Player"), parent=self, anchoMinimo=200, icon=Iconos.Player())
        if resultado is None:
            return
        accion, liGen = resultado
        nombre, alias1, alias2, alias3 = liGen
        self.escVariable("PLAYER", nombre)
        self.escVariable("ALIAS1", alias1)
        self.escVariable("ALIAS2", alias2)
        self.escVariable("ALIAS3", alias3)
        self.setPlayer(nombre)
        self.tw_rebuild()

    def tw_terminar(self):
        self.terminado = True
        self.dbGames.close()
        self.dbGames.guardaOrden()
        self.winBookGuide.terminar()

    def tw_changeplayer(self):
        lp = self.listaPlayers()
        if len(lp) == 0:
            self.rereadPlayers()
        lp = self.listaPlayers()
        if len(lp) == 0:
            return None

        menu = QTVarios.LCMenu(self)
        menu.opcion("change", _("Change"), Iconos.ModificarP())
        menu.separador()
        menu.opcion("reread", _("Reread the players list"), Iconos.Reindexar())

        resp = menu.lanza()
        if resp == "change":
            self.changePlayer(lp)

        elif resp == "reread":
            self.rereadPlayers()

    def tw_rebuild(self):
        pb = QTUtil2.BarraProgreso1(self, _("Working..."), formato1="%p%")
        pb.mostrar()
        liFields = ["RESULT", "XPV", "WHITE", "BLACK"]
        dicOpenings = {"white":{}, "black":{}}
        dicMoves = {"white":{}, "black":{}}
        dic_hap = {}
        nombre = self.player
        alias1 = self.leeVariable("ALIAS1")
        alias2 = self.leeVariable("ALIAS2")
        alias3 = self.leeVariable("ALIAS3")

        liplayer = (nombre, alias1, alias2, alias3)

        filtro = "WHITE = '%s' or BLACK = '%s'" %(nombre, nombre)
        for alias in (alias1, alias2, alias3):
            if alias:
                filtro += "or WHITE = '%s' or BLACK = '%s'" %(alias, alias)
        pb.ponTotal(self.dbGames.countData(filtro))

        for n, alm in enumerate(self.dbGames.yieldData(liFields, filtro)):
            pb.pon(n)
            if pb.siCancelado():
                return
            result = alm.RESULT
            if result in ("1-0", "0-1", "1/2-1/2"):
                white = alm.WHITE
                black = alm.BLACK

                resultw = "win" if result == "1-0" else ("lost" if result == "0-1" else "draw")
                resultb = "win" if result == "0-1" else ("lost" if result == "1-0" else "draw")

                if white in liplayer:
                    side = "white"
                    result = resultw
                elif black in liplayer:
                    side = "black"
                    result = resultb
                else:
                    continue
                xpv = alm.XPV
                if not xpv:
                    continue

                # openings
                ap = self.ap.baseXPV(xpv)
                hap = hash(ap)
                dco = dicOpenings[side]
                if hap not in dic_hap:
                    dic_hap[hap] = ap
                if hap not in dco:
                    dco[hap] = {"win":0, "draw":0, "lost":0}
                dco[hap][result] += 1

                # moves
                listapvs = LCEngine.xpv2pv(xpv).split(" ")
                dcm = dicMoves[side]
                pvt = ""
                for pv in listapvs:
                    if pvt:
                        pvt += " " + pv
                    else:
                        pvt = pv
                    if pvt not in dcm:
                        dcm[pvt] = {"win": 0, "draw": 0, "lost": 0, "games":0}
                    dcm[pvt][result] += 1
                    dcm[pvt]["games"] += 1

        pb.close()

        um = QTUtil2.unMomento(self, _("Working..."))

        def color3(x, y, z):
            if x > y and x > z:
                return 0
            if x < y and x < z:
                return 2
            return 1

        def color2(x, y):
            if x > y:
                return 0
            if x < y:
                return 2
            return 1

        def z(x):
            return "%0.2f" % x

        color = None
        info = None
        indicadorInicial = None
        liNAGs = []
        siLine = False

        data = [[], [], [], []]
        for side in ("white", "black"):
            dtemp = []
            tt = 0
            for hap in dicOpenings[side]:
                dt = dicOpenings[side][hap]
                w, d, l = dt["win"], dt["draw"], dt["lost"]
                t = w + d + l
                tt += t
                ap = dic_hap[hap]
                dic = {
                    "opening": ap.trNombre,
                    "opening_obj": ap,
                    "games": t,
                    "win": w,
                    "draw": d,
                    "lost": l,
                    "pwin": z(w*100.0/t),
                    "pdraw": z(d * 100.0 / t),
                    "plost": z(l * 100.0 / t),
                    "pdrawlost": z((d+l) * 100.0 / t),
                    "pdrawwin": z((w+d) * 100.0 / t),
                }
                dic["winc"] = dic["pwinc"] = color3(w, d, l)
                dic["drawc"] = dic["pdrawc"] = color3(d, w, l)
                dic["lostc"] = dic["plostc"] = color3(l, w, d)
                dic["pdrawlostc"] = color2(d+l, d+w)
                dic["pdrawwinc"] = color2(d+w, d+l)
                p = Partida.Partida()
                p.leerPV(ap.a1h8)
                dic["partida"] = p
                dtemp.append(dic)

            for d in dtemp:
                d["pgames"] = z(d["games"]*100.0/tt)
            dtemp.sort(key=lambda x: "%5d%s" % (99999-x["games"], x["opening"]), reverse=False)
            if side == "white":
                data[OPENINGS_WHITE] = dtemp
            else:
                data[OPENINGS_BLACK] = dtemp

            # moves
            dtemp = []
            dc = dicMoves[side]
            st_rem = set()

            listapvs = dicMoves[side].keys()
            listapvs.sort()

            sipar = 1 if side == "white" else 0

            for pv in listapvs:
                if dc[pv]["games"] == 1:
                    lipv = pv.split(" ")
                    nlipv = len(lipv)
                    if nlipv > 1:
                        pvant = " ".join(lipv[:-1])
                        if pvant in st_rem or dc[pvant]["games"] == 1 and (nlipv)%2 == sipar:
                            st_rem.add(pv)

            for pv in st_rem:
                del dc[pv]

            listapvs = dicMoves[side].keys()
            listapvs.sort()
            antlipv = []
            for npv, pv in enumerate(listapvs):
                dt = dicMoves[side][pv]
                w, d, l = dt["win"], dt["draw"], dt["lost"]
                t = w + d + l
                tt += t
                lipv = pv.split(" ")
                nli = len(lipv)
                dic = {
                    "pv": pv,
                    "games": t,
                    "win": w,
                    "draw": d,
                    "lost": l,
                    "pwin": z(w*100.0/t),
                    "pdraw": z(d * 100.0 / t),
                    "plost": z(l * 100.0 / t),
                    "pdrawlost": z((d+l) * 100.0 / t),
                    "pdrawwin": z((w+d) * 100.0 / t),
                    "nivel": nli,
                }
                # p = Partida.Partida()
                # p.leerPV(pv)
                dic["partida"] = None
                li_pgn = Partida.lipv_lipgn(lipv)
                nliant = len(antlipv)
                agrisar = True
                for x in range(100):
                    iswhite = (x % 2) == 0
                    pgn = li_pgn[x] if x < nli else ""
                    # pgn = p.jugada(x).pgnBaseSP() if x < nli else ""
                    if agrisar:
                        if x >= nliant:
                            agrisar = False
                        elif x < nli:
                            if lipv[x] != antlipv[x]:
                                agrisar = False
                    dic[str(x)] = pgn, iswhite, color, info, indicadorInicial, liNAGs, agrisar, siLine
                antlipv = lipv
                dic["winc"] = dic["pwinc"] = color3(w, d, l)
                dic["drawc"] = dic["pdrawc"] = color3(d, w, l)
                dic["lostc"] = dic["plostc"] = color3(l, w, d)
                dic["pdrawlostc"] = color2(d+l, d+w)
                dic["pdrawwinc"] = color2(d+w, d+l)
                dtemp.append(dic)

            liorder = []
            def ordena(empieza, nivel):
                li = []
                for n, uno in enumerate(dtemp):
                    if uno["nivel"] == nivel and uno["pv"].startswith(empieza):
                        li.append(uno)
                li.sort(key=lambda x:"%5d%5d" % (x["games"], x["win"]), reverse=True)
                for uno in li:
                    liorder.append(uno)
                    ordena(uno["pv"], nivel+1)

            ordena("", 1)
            if side == "white":
                data[MOVES_WHITE] = liorder
                self.movesWhite = range(len(liorder))
            else:
                data[MOVES_BLACK] = liorder
                self.movesBlack = range(len(liorder))

        um.final()

        self.data = data
        self.gridOpeningWhite.refresh()
        self.gridOpeningBlack.refresh()
        self.gridMovesWhite.refresh()
        self.gridMovesBlack.refresh()

