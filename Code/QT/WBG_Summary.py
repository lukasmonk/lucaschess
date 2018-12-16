import os

from PyQt4 import QtGui

from Code import AperturasStd
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import WBG_Comun


class WSummary(QtGui.QWidget):
    def __init__(self, procesador, winBookGuide, dbGames, siMoves=True):
        QtGui.QWidget.__init__(self)

        self.winBookGuide = winBookGuide

        self.dbGames = dbGames  # <--setdbGames
        self.bookGuide = winBookGuide.bookGuide
        self.infoMove = None  # <-- setInfoMove
        self.wmoves = None  # <-- setwmoves
        self.fenM2 = None
        self.liMoves = []
        self.analisisMRM = None
        self.siMoves = siMoves
        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.leeConfig()

        self.aperturasStd = AperturasStd.ap

        self.siFigurinesPGN = self.configuracion.figurinesPGN

        self.pvBase = ""

        self.orden = ["games", False]

        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#4E5A65").ponTipoLetra(
                puntos=10 if siMoves else 16)
        if not siMoves:
            self.lbName.hide()

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 35, siCentrado=True)
        self.delegadoMove = Delegados.EtiquetaPGN(True if self.siFigurinesPGN else None)
        oColumnas.nueva("move", _("Move"), 60, edicion=self.delegadoMove)
        dicTipos = {"t": Iconos.pmTransposition(),}
        oColumnas.nueva("trans", "", 28, edicion=Delegados.PmIconosBMT(dicIconos=dicTipos))
        oColumnas.nueva("analisis", _("Analysis"), 60, siDerecha=True)
        oColumnas.nueva("games", _("Games"), 70, siDerecha=True)
        oColumnas.nueva("pgames", "% " + _("Games"), 70, siDerecha=True)
        oColumnas.nueva("win", _("Win"), 70, siDerecha=True)
        oColumnas.nueva("draw", _("Draw"), 70, siDerecha=True)
        oColumnas.nueva("lost", _("Lost"), 70, siDerecha=True)
        oColumnas.nueva("pwin", "% " + _("Win"), 60, siDerecha=True)
        oColumnas.nueva("pdraw", "% " + _("Draw"), 60, siDerecha=True)
        oColumnas.nueva("plost", "% " + _("Lost"), 60, siDerecha=True)
        oColumnas.nueva("pdrawwin", "%% %s" % _("W+D"), 60, siDerecha=True)
        oColumnas.nueva("pdrawlost", "%% %s" % _("L+D"), 60, siDerecha=True)

        self.grid = Grid.Grid(self, oColumnas, xid="summary", siSelecFilas=True)
        self.grid.tipoLetra(puntos=self.configuracion.puntosPGN)
        self.grid.ponAltoFila(self.configuracion.altoFilaPGN)

        # ToolBar
        liAcciones = [(_("Close"), Iconos.MainMenu(), winBookGuide.terminar), None,
                      (_("Start position"), Iconos.Inicio(), self.inicio), None,
                      (_("Previous"), Iconos.AnteriorF(), self.anterior),
                      (_("Next"), Iconos.SiguienteF(), self.siguiente), None,
                      (_("Analyze"), Iconos.Analizar(), self.analizar), None,
                      (_("Rebuild"), Iconos.Reindexar(), self.reindexar), None,
                      (_("Config"), Iconos.Configurar(), self.config), None,
                      ]
        if siMoves:
            liAcciones.append((_("Create a new guide based in these games") if siMoves else _("Create"), Iconos.BookGuide(), self.createGuide))
            liAcciones.append(None)

        self.tb = QTVarios.LCTB(self, liAcciones, tamIcon=20, siTexto=not self.siMoves)
        if self.siMoves:
            self.tb.vertical()

        layout = Colocacion.V().control(self.lbName)
        if not self.siMoves:
            layout.control(self.tb)
        layout.control(self.grid)
        if self.siMoves:
            layout = Colocacion.H().control(self.tb).otro(layout)

        layout.margen(1)

        self.setLayout(layout)

        self.qtColor = (QTUtil.qtColorRGB(221, 255, 221), QTUtil.qtColorRGB(247, 247, 247), QTUtil.qtColorRGB(255, 217, 217))
        self.qtColorTotales = QTUtil.qtColorRGB(170, 170, 170)

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave

        if clave == "analisis":
            func = lambda dic: dic["analisis"].puntosABS() if dic["analisis"] else -9999999
        elif clave == "move":
            func = lambda dic: dic["move"].upper()
        else:
            func = lambda dic: dic[clave]
        tot = self.liMoves[-1]
        li = sorted(self.liMoves[:-1], key=func)

        orden, mas = self.orden
        if orden == clave:
            mas = not mas
        else:
            mas = clave == "move"
        if not mas:
            li.reverse()
        self.orden = clave, mas
        li.append(tot)
        self.liMoves = li
        self.grid.refresh()

    def setdbGames(self, dbGames):
        self.dbGames = dbGames

    def focusInEvent(self, event):
        self.winBookGuide.ultFocus = self

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove

    def setwmoves(self, wmoves):
        self.wmoves = wmoves

    def gridNumDatos(self, grid):
        return len(self.liMoves)

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        if k == 16777220:
            self.siguiente()

    def gridBotonDerecho(self, grid, fila, columna, modificadores):
        if self.siFilaTotales(fila):
            return
        alm = self.liMoves[fila]["alm"]
        if not alm or len(alm.LIALMS) < 2:
            return

        menu = QTVarios.LCMenu(self)
        rondo = QTVarios.rondoPuntos()
        for ralm in alm.LIALMS:
            menu.opcion(ralm, Partida.pv_pgn(None, ralm.PV), rondo.otro())
            menu.separador()
        resp = menu.lanza()
        if resp:
            self.ponPV(resp.PV)

    def gridDato(self, grid, nfila, ocol):
        clave = ocol.clave

        # Last=Totals
        if self.siFilaTotales(nfila):
            if clave in ("trans", "numero", "analisis", "pgames"):
                return ""
            elif clave == "move":
                return _("Total")

        if clave == "trans":
            alm = self.liMoves[nfila]["alm"]
            return "t" if (alm and len(alm.LIALMS) > 1) else "-"
        if self.liMoves[nfila]["games"] == 0 and clave not in ("numero", "analisis", "move"):
            return ""
        v = self.liMoves[nfila][clave]
        if clave.startswith("p"):
            return "%.01f %%" % v
        elif clave == "analisis":
            return v.abrTextoBase() if v else ""
        elif clave == "numero":
            if self.siFigurinesPGN:
                self.delegadoMove.setWhite("..." not in v)
            return v
        else:
            return str(v)

    def posicionFila(self, nfila):
        dic = self.liMoves[nfila]
        li = [[k, dic[k]] for k in ("win", "draw", "lost")]
        li = sorted(li, key=lambda x: x[1], reverse=True)
        d = {}
        prev = 0
        ant = li[0][1]
        total = 0
        for cl, v in li:
            if v < ant:
                prev += 1
            d[cl] = prev
            ant = v
            total += v
        if total == 0:
            d["win"] = d["draw"] = d["lost"] = -1
        return d

    def gridColorFondo(self, grid, nfila, ocol):
        clave = ocol.clave
        if self.siFilaTotales(nfila) and clave not in ("numero", "analisis"):
            return self.qtColorTotales
        if clave in ("pwin", "pdraw", "plost"):
            dic = self.posicionFila(nfila)
            n = dic[clave[1:]]
            if n > -1:
                return self.qtColor[n]

    def popPV(self, pv):
        if pv:
            rb = pv.rfind(" ")
            if rb == -1:
                pv = ""
            else:
                pv = pv[:rb]
        return pv

    def analizar(self):
        if not self.fenM2:
            return

        fila = self.grid.recno()
        wk = self.liMoves[fila]

        rmAnalisis = wk["analisis"]
        siShowMoves = rmAnalisis is not None

        def dispatch(nada):
            self.actualizaPV(self.pvBase)

        analisis = WBG_Comun.Analisis(self, self.bookGuide, dispatch, self.procesador)
        resp = analisis.menuAnalizar(self.fenM2, None, siShowMoves)
        if resp:
            partida = wk["partida"]
            fen = partida.last_jg().posicionBase.fen()
            pv = wk["pv"]
            analisis.exeAnalizar(self.fenM2, resp, None, fen, pv, rmAnalisis)

    def inicio(self):
        self.actualizaPV("")
        self.cambiaInfoMove()

    def anterior(self):
        if self.pvBase:
            pv = self.popPV(self.pvBase)

            self.actualizaPV(pv)
            self.cambiaInfoMove()

    def rehazActual(self):
        recno = self.grid.recno()
        if recno >= 0:
            dic = self.liMoves[recno]
            if "pv" in dic:
                pv = dic["pv"]
                if pv:
                    li = pv.split(" ")
                    pv = " ".join(li[:-1])
                self.actualizaPV(pv)
                self.cambiaInfoMove()

    def siguiente(self):
        recno = self.grid.recno()
        if recno >= 0:
            dic = self.liMoves[recno]
            if "pv" in dic:
                pv = dic["pv"]
                if pv.count(" ") > 0:
                    pv = "%s %s" % (self.pvBase, dic["pvmove"])  # transposition case
                self.actualizaPV(pv)
                self.cambiaInfoMove()

    def ponPV(self, pvMirar):
        if not pvMirar:
            self.actualizaPV(None)
        else:
            self.analisisMRM = None
            dicAnalisis = {}
            self.fenM2 = None
            p = Partida.Partida()
            if pvMirar:
                p.leerPV(pvMirar)
            self.fenM2 = p.ultPosicion.fenM2()
            self.analisisMRM = self.bookGuide.dbAnalisis.mrm(self.fenM2)
            if self.analisisMRM:
                for rm in self.analisisMRM.liMultiPV:
                    dicAnalisis[rm.movimiento()] = rm
            li = pvMirar.split(" ")
            self.pvBase = " ".join(li[:-1])
            busca = li[-1]
            self.liMoves = self.dbGames.getSummary(pvMirar, dicAnalisis, self.siFigurinesPGN)
            for fila, move in enumerate(self.liMoves):
                if move.get("pvmove") == busca:
                    self.grid.goto(fila, 0)
                    break
        self.cambiaInfoMove()

    def createGuide(self):
        name = os.path.basename(self.dbGames.nomFichero)[:-4]
        maxdepth = self.dbGames.depthStat()
        depth = maxdepth
        minGames = min(self.dbGames.all_reccount() * 10 / 100, 5)
        pointview = 2
        inicio = 0
        mov = self.movActivo()

        while True:
            liGen = [(None, None)]
            liGen.append((_("Name") + ":", name))

            liGen.append((FormLayout.Spinbox(_("Depth"), 2, maxdepth, 50), depth))

            liGen.append((None, None))

            liGen.append((FormLayout.Spinbox(_("Minimum games"), 1, 99, 50), minGames))

            liGen.append((None, None))

            liPointV = [pointview,
                        (0, _("White")),
                        (1, _("Black")),
                        (2, "%s + %s" % (_("White"), _("Draw"))),
                        (3, "%s + %s" % (_("Black"), _("Draw"))),
                        ]
            liGen.append((_("Point of view") + ":", liPointV))

            liGen.append((None, None))

            if mov:
                liInicio = [inicio,
                            (0, _("The beginning")),
                            (1, _("Current move")),
                            ]
                liGen.append((_("Start from") + ":", liInicio))

            resultado = FormLayout.fedit(liGen, title=_("Create new guide"), parent=self, anchoMinimo=460,
                                         icon=Iconos.TutorialesCrear())
            if resultado is None:
                return

            accion, liResp = resultado
            name = liResp[0].strip()
            if not name:
                return
            depth = liResp[1]
            minGames = liResp[2]
            pointview = liResp[3]
            if mov:
                inicio = liResp[4]

            ok = True
            for una in self.bookGuide.getTodas():
                if una.strip().upper() == name.upper():
                    QTUtil2.mensError(self, _("The name is repeated"))
                    ok = False
                    continue
            if ok:
                break

        # Grabamos
        um = QTUtil2.unMomento(self)

        # Read stats
        siWhite = pointview % 2 == 0
        siDraw = pointview > 1
        pv = ""
        if mov:
            if inicio > 0:
                pv = self.pvBase + " " + mov["pvmove"]
                pv = pv.strip()

        fichPVs = self.dbGames.flistAllpvs(depth, minGames, siWhite, siDraw, pv)

        # Write to bookGuide
        self.bookGuide.grabarFichSTAT(name, fichPVs)

        # BookGuide
        self.wmoves.inicializa()

        um.final()

    def reindexar(self):
        if not QTUtil2.pregunta(self, _("Do you want to rebuild stats?")):
            return

        # Select depth
        liGen = [(None, None)]
        liGen.append((None, _("Select the number of moves <br> for each game to be considered")))
        liGen.append((None, None))

        config = FormLayout.Spinbox(_("Depth"), 3, 255, 50)
        liGen.append((config, self.dbGames.depthStat()))

        resultado = FormLayout.fedit(liGen, title=_("Rebuild"), parent=self, icon=Iconos.Reindexar())
        if resultado is None:
            return None

        accion, liResp = resultado

        depth = liResp[0]

        self.RECCOUNT = 0

        bpTmp = QTUtil2.BarraProgreso1(self, _("Rebuilding"))
        bpTmp.mostrar()

        def dispatch(recno, reccount):
            if reccount != self.RECCOUNT:
                self.RECCOUNT = reccount
                bpTmp.ponTotal(reccount)
            bpTmp.pon(recno)
            return not bpTmp.siCancelado()

        self.dbGames.recrearSTAT(dispatch, depth)
        bpTmp.cerrar()
        self.inicio()

    def movActivo(self):
        recno = self.grid.recno()
        if recno >= 0:
            return self.liMoves[recno]
        else:
            return None

    def siFilaTotales(self, nfila):
        return nfila == len(self.liMoves)-1

    def noFilaTotales(self, nfila):
        return nfila < len(self.liMoves)-1

    def gridDobleClick(self, grid, fil, col):
        if self.noFilaTotales(fil):
            self.siguiente()

    def gridActualiza(self):
        nfila = self.grid.recno()
        if nfila > -1:
            self.gridCambiadoRegistro(None, nfila, None)

    def actualiza(self):
        movActual = self.infoMove.movActual
        # if movActual and movActual != self.bookGuide.root:
        pvBase = self.popPV(movActual.allPV())
        # else:
        # pvBase = None
        self.actualizaPV(pvBase)
        if movActual:
            pv = movActual.allPV()
            for n in range(len(self.liMoves)-1):
                if self.liMoves[n]["pv"] == pv:
                    self.grid.goto(n, 0)
                    return

    def actualizaPV(self, pvBase):
        self.pvBase = pvBase
        if not pvBase:
            pvMirar = ""
        else:
            pvMirar = self.pvBase

        self.analisisMRM = None
        dicAnalisis = {}
        self.fenM2 = None
        if pvMirar:
            p = Partida.Partida()
            if pvMirar:
                p.leerPV(pvMirar)
            self.fenM2 = p.ultPosicion.fenM2()
            self.analisisMRM = self.bookGuide.dbAnalisis.mrm(self.fenM2)
            if self.analisisMRM:
                for rm in self.analisisMRM.liMultiPV:
                    dicAnalisis[rm.movimiento()] = rm
        self.liMoves = self.dbGames.getSummary(pvMirar, dicAnalisis, self.siFigurinesPGN, self.allmoves)

        self.grid.refresh()
        self.grid.gotop()

    def reset(self):
        self.actualizaPV(None)
        self.grid.refresh()
        self.grid.gotop()

    def gridCambiadoRegistro(self, grid, fila, oCol):
        if self.grid.hasFocus() or self.hasFocus():
            self.cambiaInfoMove()

    def cambiaInfoMove(self):
        fila = self.grid.recno()
        if fila >= 0 and self.noFilaTotales(fila):
            pv = self.liMoves[fila]["pv"]
            p = Partida.Partida()
            p.leerPV(pv)
            p.siTerminada()
            p.asignaApertura()
            self.infoMove.modoPartida(p, 9999)
            self.setFocus()
            self.grid.setFocus()

    def showActiveName(self, nombre):
        # Llamado de WBG_Games -> setNameToolbar
        self.lbName.ponTexto(_("Summary of %s") % nombre)

    def leeConfig(self):
        dicConfig = self.configuracion.leeVariables("DBSUMMARY")
        if not dicConfig:
            dicConfig = { "allmoves":False }
        self.allmoves = dicConfig["allmoves"]
        return dicConfig

    def grabaConfig(self):
        dicConfig = {"allmoves": self.allmoves}
        self.configuracion.escVariables("DBSUMMARY", dicConfig)
        self.configuracion.graba()

    # def print_repertorio(self):
    #     siW = True
    #     basepv = "e2e4"
    #     k = [0, []]

    #     def haz(lipv):
    #         npv = len(lipv)
    #         liChildren = self.dbGames.dbSTAT.children(" ".join(lipv), False)
    #         if len(liChildren) == 0:
    #             return
    #         suma = 0
    #         for n, alm in enumerate(liChildren):
    #             alm.tt = alm.W + alm.B + alm.D + alm.O
    #             suma += alm.tt
    #         if (npv % 2 == 0) and siW:
    #             # buscamos la que tenga mas tt
    #             mx = 0
    #             mx_alm = None
    #             for alm in liChildren:
    #                 if alm.tt > mx:
    #                     mx_alm = alm
    #                     mx = alm.tt
    #             li = lipv[:]
    #             if mx_alm:
    #                 li.append(mx_alm.move)
    #                 haz(li)
    #         else:
    #             if suma < 2000 and npv < 20:
    #                 k[0] += 1
    #                 liLast = k[1]
    #                 nl = len(liLast)
    #                 ok = False
    #                 lip = []
    #                 for nr, pv in enumerate(lipv):
    #                     if nr < nl and not ok:
    #                         if pv == liLast[nr]:
    #                             lip.append("....")
    #                         else:
    #                             lip.append(pv)
    #                             ok = True
    #                     else:
    #                         lip.append(pv)
    #                 k[1] = lipv
    #                 # liW = []
    #                 # liB = []
    #                 # sitW = True
    #                 # for pv in lip:
    #                 #     if sitW:
    #                 #         liW.append(pv)
    #                 #     else:
    #                 #         liB.append(pv)
    #                 #     sitW = not sitW
    #                 #
    #                 # print " ".join(liW)
    #                 # print " ".join(liB)
    #                 print " ".join(lip)
    #                 return
    #             for alm in liChildren:
    #                 li = lipv[:]
    #                 li.append(alm.move)
    #                 haz(li)
    #     haz([basepv,])

    def config(self):
        # return self.print_repertorio()
        menu = QTVarios.LCMenu(self)
        menu.opcion("allmoves", _("Show all moves"), siChecked=self.allmoves)
        resp = menu.lanza()
        if resp is None:
            return
        self.allmoves = not self.allmoves

        self.actualizaPV(self.pvBase)


class WSummaryBase(QtGui.QWidget):
    def __init__(self, procesador, dbSTAT):
        QtGui.QWidget.__init__(self)

        self.dbSTAT = dbSTAT
        self.liMoves = []
        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.siFigurinesPGN = self.configuracion.figurinesPGN

        self.orden = ["games", False]

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("numero", _("N."), 35, siCentrado=True)
        self.delegadoMove = Delegados.EtiquetaPGN(True if self.siFigurinesPGN else None)
        oColumnas.nueva("move", _("Move"), 60, edicion=self.delegadoMove)
        dicTipos = {"t": Iconos.pmTransposition(),}
        oColumnas.nueva("trans", "", 28, edicion=Delegados.PmIconosBMT(dicIconos=dicTipos))
        oColumnas.nueva("games", _("Games"), 70, siDerecha=True)
        oColumnas.nueva("pgames", "% " + _("Games"), 70, siDerecha=True, siCentrado=True)
        oColumnas.nueva("win", _("Win"), 70, siDerecha=True)
        oColumnas.nueva("draw", _("Draw"), 70, siDerecha=True)
        oColumnas.nueva("lost", _("Lost"), 70, siDerecha=True)
        oColumnas.nueva("pwin", "% " + _("Win"), 60, siDerecha=True)
        oColumnas.nueva("pdraw", "% " + _("Draw"), 60, siDerecha=True)
        oColumnas.nueva("plost", "% " + _("Lost"), 60, siDerecha=True)
        oColumnas.nueva("pdrawwin", "%% %s" % _("W+D"), 60, siDerecha=True)
        oColumnas.nueva("pdrawlost", "%% %s" % _("L+D"), 60, siDerecha=True)

        self.grid = Grid.Grid(self, oColumnas, xid="summarybase", siSelecFilas=True)
        self.grid.tipoLetra(puntos=self.configuracion.puntosPGN)
        self.grid.ponAltoFila(self.configuracion.altoFilaPGN)

        layout = Colocacion.V()
        layout.control(self.grid)
        layout.margen(1)

        self.setLayout(layout)

        self.qtColor = (QTUtil.qtColorRGB(221, 255, 221), QTUtil.qtColorRGB(247, 247, 247), QTUtil.qtColorRGB(255, 217, 217))
        self.qtColorTotales = QTUtil.qtColorRGB(170, 170, 170)

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave

        if clave == "move":
            func = lambda dic: dic["move"].upper()
        else:
            func = lambda dic: dic[clave]
        tot = self.liMoves[-1]
        li = sorted(self.liMoves[:-1], key=func)

        orden, mas = self.orden
        if orden == clave:
            mas = not mas
        else:
            mas = clave == "move"
        if not mas:
            li.reverse()
        self.orden = clave, mas
        li.append(tot)
        self.liMoves = li
        self.grid.refresh()

    def gridNumDatos(self, grid):
        return len(self.liMoves)

    def gridDato(self, grid, nfila, ocol):
        clave = ocol.clave

        # Last=Totals
        if self.siFilaTotales(nfila):
            if clave in ("trans", "numero", "pgames"):
                return ""
            elif clave == "move":
                return _("Total")

        if clave == "trans":
            alm = self.liMoves[nfila]["alm"]
            return "t" if len(alm.LIALMS) > 1 else "-"
        if self.liMoves[nfila]["games"] == 0 and clave not in ("numero", "move"):
            return ""
        v = self.liMoves[nfila][clave]
        if clave.startswith("p"):
            return "%.01f %%" % v
        elif clave == "numero":
            if self.siFigurinesPGN:
                self.delegadoMove.setWhite("..." not in v)
            return v
        else:
            return str(v)

    def posicionFila(self, nfila):
        dic = self.liMoves[nfila]
        li = [[k, dic[k]] for k in ("win", "draw", "lost")]
        li = sorted(li, key=lambda x: x[1], reverse=True)
        d = {}
        prev = 0
        ant = li[0][1]
        total = 0
        for cl, v in li:
            if v < ant:
                prev += 1
            d[cl] = prev
            ant = v
            total += v
        if total == 0:
            d["win"] = d["draw"] = d["lost"] = -1
        return d

    def gridColorFondo(self, grid, nfila, ocol):
        clave = ocol.clave
        if self.siFilaTotales(nfila) and clave not in ("numero", "analisis"):
            return self.qtColorTotales
        if clave in ("pwin", "pdraw", "plost"):
            dic = self.posicionFila(nfila)
            n = dic[clave[1:]]
            if n > -1:
                return self.qtColor[n]

    def siFilaTotales(self, nfila):
        return nfila == len(self.liMoves)-1

    def noFilaTotales(self, nfila):
        return nfila < len(self.liMoves)-1

    def actualizaPV(self, pvBase):
        self.pvBase = pvBase
        if not pvBase:
            pvMirar = ""
        else:
            pvMirar = self.pvBase

        self.liMoves = self.dbSTAT.getSummary(pvMirar, {}, self.siFigurinesPGN, False)

        self.grid.refresh()
        self.grid.gotop()

    def gridBotonDerecho(self, grid, fila, columna, modificadores):
        if self.siFilaTotales(fila):
            return
        alm = self.liMoves[fila]["alm"]
        if not alm or len(alm.LIALMS) < 2:
            return

        menu = QTVarios.LCMenu(self)
        rondo = QTVarios.rondoPuntos()
        for ralm in alm.LIALMS:
            menu.opcion(ralm, Partida.pv_pgn(None, ralm.PV), rondo.otro())
            menu.separador()
        resp = menu.lanza()
        if resp:
            self.actualizaPV(resp.PV)

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        if k == 16777220:
            self.siguiente()

    def gridDobleClick(self, grid, fil, col):
        self.siguiente()

    def siguiente(self):
        recno = self.grid.recno()
        if recno >= 0 and self.noFilaTotales(recno):
            dic = self.liMoves[recno]
            if "pv" in dic:
                pv = dic["pv"]
                if pv.count(" ") > 0:
                    pv = "%s %s" % (self.pvBase, dic["pvmove"])  # transposition case
                self.actualizaPV(pv)
