# -*- coding: latin-1 -*-

import os

from PyQt4 import QtGui

import Code.VarGen as VarGen
import Code.Partida as Partida
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Colocacion as Colocacion
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.FormLayout as FormLayout
import Code.QT.Grid as Grid
import Code.QT.Columnas as Columnas
import Code.QT.WBG_Comun as WBG_Comun

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

        self.pvBase = ""

        self.orden = ["games", False]

        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#4E5A65").ponTipoLetra(
            puntos=10 if siMoves else 16)
        if not siMoves:
            self.lbName.hide()

        # Grid
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("move", _("Move"), 60)
        oColumnas.nueva("analisis", _("Analysis"), 60, siDerecha=True)
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

        self.grid = Grid.Grid(self, oColumnas, id="summary", siSelecFilas=True)

        # ToolBar
        liAcciones = [( _("Previous"), Iconos.AnteriorF(), "anterior" ),
                      ( _("Next"), Iconos.SiguienteF(), "siguiente" ), None,
                      ( _("Analyze"), Iconos.Analizar(), "analizar" ), None,
                      ( _("Rebuild"), Iconos.Reindexar(), "reindexar" ), None,
                      (_("Summary filtering by player") if siMoves else _("By player"), Iconos.Player(),
                       "reindexarPlayer" ), None,
        ]
        if siMoves:
            liAcciones.append((
                _("Create a new guide based in these games") if siMoves else _("Create"), Iconos.BookGuide(),
                "create" ))
            liAcciones.append(None)

        self.tb = Controles.TB(self, liAcciones, tamIcon=20, siTexto=not self.siMoves)
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

        self.qtColor = (
            QTUtil.qtColorRGB(221, 255, 221), QTUtil.qtColorRGB(247, 247, 247), QTUtil.qtColorRGB(255, 217, 217) )

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave

        if clave == "analisis":
            func = lambda dic: dic["analisis"].puntosABS() if dic["analisis"] else -9999999
        elif clave == "move":
            func = lambda dic: dic["move"].upper()
        else:
            func = lambda dic: dic[clave]
        li = sorted(self.liMoves, key=func)

        orden, mas = self.orden
        if orden == clave:
            mas = not mas
        else:
            mas = clave == "move"
        if not mas:
            li.reverse()
        self.orden = clave, mas
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

    def gridDato(self, grid, nfila, ocol):
        clave = ocol.clave
        v = self.liMoves[nfila][clave]
        if clave == "move":
            return v
        elif clave.startswith("p"):
            return "%.01f %%" % v
        elif clave == "analisis":
            return v.abrTextoBase() if v else ""
        else:
            return str(v)

    def posicionFila(self, nfila):
        dic = self.liMoves[nfila]
        li = [[k, dic[k]] for k in ("win", "draw", "lost")]
        li = sorted(li, key=lambda x: x[1], reverse=True)
        d = {}
        prev = 0
        ant = li[0][1]
        for cl, v in li:
            if v < ant:
                prev += 1
            d[cl] = prev
            ant = v
        return d

    def gridColorFondo(self, grid, nfila, ocol):
        clave = ocol.clave
        if clave in ( "pwin", "pdraw", "plost" ):
            dic = self.posicionFila(nfila)
            n = dic[clave[1:]]
            return self.qtColor[n]

        return None

    def procesarTB(self):
        getattr(self, self.sender().clave)()

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
        pv = wk["pv"]

        rmAnalisis = wk["analisis"]
        siShowMoves = rmAnalisis is not None

        def dispatch(nada):
            self.actualizaPV(self.pvBase)

        analisis = WBG_Comun.Analisis(self, self.bookGuide, dispatch, self.procesador)
        resp = analisis.menuAnalizar(self.fenM2, None, siShowMoves)
        if resp:
            partida = wk["partida"]
            fen = partida.jugada(-1).posicionBase.fen()
            pv = wk["pv"]
            analisis.exeAnalizar(self.fenM2, resp, None, fen, pv, rmAnalisis)

    def anterior(self):
        if self.pvBase:
            pv = self.popPV(self.pvBase)
        else:
            pv = None
        self.actualizaPV(pv)
        self.cambiaInfoMove()

    def siguiente(self):
        recno = self.grid.recno()
        if recno >= 0:
            pv = self.liMoves[recno]["pv"]
            self.actualizaPV(pv)
            self.cambiaInfoMove()

    def create(self):
        name = os.path.basename(self.dbGames.nomFichero)[:-4]
        maxdepth = self.dbGames.depthStat()
        depth = maxdepth
        minGames = min(self.dbGames.reccountTotal() * 10 / 100, 5)
        pointview = 2
        inicio = 0
        mov = self.movActivo()

        while True:
            liGen = [(None, None)]
            liGen.append(( _("Name") + ":", name ))

            liGen.append((FormLayout.Spinbox(_("Depth"), 2, maxdepth, 50), depth))

            liGen.append((None, None))

            liGen.append((FormLayout.Spinbox(_("Minimum games"), 1, 99, 50), minGames))

            liGen.append((None, None))

            liPointV = [pointview,
                        ( 0, _("White") ),
                        ( 1, _("Black") ),
                        ( 2, "%s + %s" % (_("White"), _("Draw") ) ),
                        ( 3, "%s + %s" % (_("Black"), _("Draw") ) ),
            ]
            liGen.append((_("Point of view") + ":", liPointV))

            liGen.append((None, None))

            if mov:
                liInicio = [inicio,
                            ( 0, _("The beginning") ),
                            ( 1, _("Current move") ),
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
        liGen.append(( None, _("Select the number of moves <br> for each game to be considered") ))
        liGen.append((None, None))

        li = [(str(n), n) for n in range(3, 255)]
        config = FormLayout.Combobox(_("Depth"), li)
        liGen.append(( config, self.dbGames.depthStat() ))

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
        self.grid.refresh()

    def reindexarPlayer(self):
        dic = VarGen.configuracion.leeVariables("reindexplayer")

        # Select depth
        liGen = [(None, None)]
        liGen.append(( _("Player (wildcards=*)"), dic.get("player", "")))
        liGen.append((None, None))
        liGen.append(( None, _("Select the number of moves <br> for each game to be considered") ))
        liGen.append((None, None))

        li = [(str(n), n) for n in range(3, 255)]
        config = FormLayout.Combobox(_("Depth"), li)
        liGen.append(( config, dic.get("depth", 30) ))

        resultado = FormLayout.fedit(liGen, title=_("Summary filtering by player"), parent=self,
                                     icon=Iconos.Reindexar())
        if resultado is None:
            return None

        accion, liResp = resultado

        dic["player"] = player = liResp[0]
        if not player:
            return
        dic["depth"] = depth = liResp[1]

        VarGen.configuracion.escVariables("reindexplayer", dic)

        class CLT:
            pass

        clt = CLT()
        clt.RECCOUNT = 0
        clt.SICANCELADO = False

        bpTmp = QTUtil2.BarraProgreso1(self, _("Rebuilding"))
        bpTmp.mostrar()

        def dispatch(recno, reccount):
            if reccount != clt.RECCOUNT:
                clt.RECCOUNT = reccount
                bpTmp.ponTotal(reccount)
            bpTmp.pon(recno)
            if bpTmp.siCancelado():
                clt.SICANCELADO = True

            return not clt.SICANCELADO

        self.dbGames.recrearSTATplayer(dispatch, depth, player)
        bpTmp.cerrar()
        if clt.SICANCELADO:
            self.dbGames.ponSTATbase()
        self.grid.refresh()

    def movActivo(self):
        recno = self.grid.recno()
        if recno >= 0:
            return self.liMoves[recno]
        else:
            return None

    def gridDobleClick(self, grid, fil, col):
        self.siguiente()

    def actualiza(self):
        movActual = self.infoMove.movActual
        # if movActual and movActual != self.bookGuide.root:
        pvBase = self.popPV(movActual.allPV())
        # else:
        # pvBase = None
        self.actualizaPV(pvBase)
        if movActual:
            pv = movActual.allPV()
            for n, uno in enumerate(self.liMoves):
                if uno["pv"] == pv:
                    self.grid.goto(n, 0)
                    return

    def actualizaPV(self, pvBase):
        self.pvBase = pvBase
        if pvBase is None:
            pvMirar = None
        else:
            pvMirar = self.pvBase

        self.analisisMRM = None
        dicAnalisis = {}
        self.fenM2 = None
        if pvMirar is not None:
            p = Partida.Partida()
            if pvMirar:
                p.leerPV(pvMirar)
            self.fenM2 = p.ultPosicion.fenM2()
            self.analisisMRM = self.bookGuide.dbAnalisis.mrm(self.fenM2)
            if self.analisisMRM:
                for rm in self.analisisMRM.liMultiPV:
                    dicAnalisis[rm.movimiento()] = rm
        self.liMoves = self.dbGames.getSummary(pvMirar, dicAnalisis)

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
        if fila >= 0:
            pv = self.liMoves[fila]["pv"]
            p = Partida.Partida()
            p.leerPV(pv)
            p.siTerminada()
            self.infoMove.modoPartida(p, 9999)
            self.setFocus()
            self.grid.setFocus()

    def showActiveName(self, nombre):
        # Llamado de WBG_Games -> setNameToolbar
        self.lbName.ponTexto(_("Summary of %s") % nombre)
