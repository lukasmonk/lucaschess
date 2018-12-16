from PyQt4 import QtCore, QtGui

from Code import VarGen
from Code import Partida
from Code import XRun
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaAnalisisParam
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code.QT import Histogram
from Code.QT import WCapturas


class WAnalisisGraph(QTVarios.WDialogo):
    def __init__(self, wowner, gestor, alm, muestraAnalisis):
        titulo = _("Result of analysis")
        icono = Iconos.Estadisticas()
        extparam = "estadisticasv1"
        QTVarios.WDialogo.__init__(self, wowner, titulo, icono, extparam)
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint)

        self.alm = alm
        self.procesador = gestor.procesador
        self.gestor = gestor
        self.configuracion = gestor.configuracion
        self.siPawns = not gestor.procesador.configuracion.centipawns
        self.muestraAnalisis = muestraAnalisis
        self.colorWhite = QTUtil.qtColorRGB(231, 244, 254)

        def xcol():
            oColumnas = Columnas.ListaColumnas()
            oColumnas.nueva("NUM", _("N."), 50, siCentrado=True)
            oColumnas.nueva("MOVE", _("Move"), 120, siCentrado=True, edicion=Delegados.EtiquetaPGN(True, True, True))
            oColumnas.nueva("BEST", _("Best move"), 120, siCentrado=True, edicion=Delegados.EtiquetaPGN(True, True, True))
            oColumnas.nueva("DIF", _("Difference"), 80, siCentrado=True)
            oColumnas.nueva("PORC", "%", 80, siCentrado=True)
            oColumnas.nueva("ELO", _("Elo"), 80, siCentrado=True)
            return oColumnas

        self.dicLiJG = {"A": self.alm.lijg, "W": self.alm.lijgW, "B": self.alm.lijgB}
        gridAll = Grid.Grid(self, xcol(), siSelecFilas=True, xid="A")
        anchoGrid = gridAll.fixMinWidth()
        self.registrarGrid(gridAll)
        gridW = Grid.Grid(self, xcol(), siSelecFilas=True, xid="W")
        anchoGrid = max(gridW.fixMinWidth(), anchoGrid)
        self.registrarGrid(gridW)
        gridB = Grid.Grid(self, xcol(), siSelecFilas=True, xid="B")
        anchoGrid = max(gridB.fixMinWidth(), anchoGrid)
        self.registrarGrid(gridB)

        self.emIndexes = Controles.EM(self, alm.indexesHTML).soloLectura()
        pbSave = Controles.PB(self, _("Save to game comments"), self.saveIndexes, plano=False)
        pbSave.ponIcono(Iconos.Grabar())
        ly0 = Colocacion.H().control(pbSave).relleno()
        ly = Colocacion.V().control(self.emIndexes).otro(ly0).relleno()
        wIdx = QtGui.QWidget()
        wIdx.setLayout(ly)

        self.tabGrid = tabGrid = Controles.Tab()
        tabGrid.nuevaTab(gridAll, _("All moves"))
        tabGrid.nuevaTab(gridW, _("White"))
        tabGrid.nuevaTab(gridB, _("Black"))
        tabGrid.nuevaTab(wIdx, _("Indexes"))
        tabGrid.dispatchChange(self.tabChanged)
        self.tabActive = 0

        confTablero = VarGen.configuracion.confTablero("ANALISISGRAPH", 48)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(alm.siBlancasAbajo)
        self.tablero.dispatchSize(self.tableroSizeChanged)

        self.capturas = WCapturas.CapturaLista(self, self.tablero)
        ly_tc = Colocacion.H().control(self.tablero).control(self.capturas)

        self.rbShowValues = Controles.RB(self, _("Values"), rutina=self.cambiadoShow).activa(True)
        self.rbShowElo = Controles.RB(self, _("Elo performance"), rutina=self.cambiadoShow)
        self.chbShowLostPoints = Controles.CHB(self, _("Show lost points"), self.getShowLostPoints()).capturaCambiado(self, self.showLostPointsChanged)
        ly_rb = Colocacion.H().espacio(40).control(self.rbShowValues).espacio(20).control(self.rbShowElo).espacio(30).control(self.chbShowLostPoints).relleno(1)

        layout = Colocacion.G()
        layout.controlc(tabGrid, 0, 0)
        layout.otroc(ly_rb, 1, 0)
        layout.otroc(ly_tc, 0, 1, numFilas=2)

        Controles.Tab().ponPosicion("W")
        ancho = self.tablero.width() + anchoGrid
        self.htotal = [ Histogram.Histogram(self, alm.hgame, gridAll, ancho, True),
                        Histogram.Histogram(self, alm.hwhite, gridW, ancho, True),
                        Histogram.Histogram(self, alm.hblack, gridB, ancho, True),
                        Histogram.Histogram(self, alm.hgame, gridAll, ancho, False, alm.eloT),
                        Histogram.Histogram(self, alm.hwhite, gridW, ancho, False, alm.eloW),
                        Histogram.Histogram(self, alm.hblack, gridB, ancho, False, alm.eloB)
        ]
        lh = Colocacion.V()
        for x in range(6):
            lh.control(self.htotal[x])
            if x:
                self.htotal[x].hide()

        layout.otroc(lh, 2, 0, 1, 3)
        self.setLayout(layout)

        self.recuperarVideo()

        gridAll.gotop()
        gridB.gotop()
        gridW.gotop()
        self.gridBotonIzquierdo(gridAll, 0, None)
        th = self.tablero.height()
        self.tabGrid.setFixedHeight(th)
        self.adjustSize()
        self.emIndexes.setFixedHeight(th - 72)

    def valorShowLostPoints(self):
        # Llamada desde histogram
        return self.chbShowLostPoints.valor()

    def showLostPointsChanged(self):
        dic = {"SHOWLOSTPOINTS":self.valorShowLostPoints()}
        self.configuracion.escVariables("ANALISIS_GRAPH", dic)
        self.cambiadoShow()

    def getShowLostPoints(self):
        dic = self.configuracion.leeVariables("ANALISIS_GRAPH")
        return dic.get("SHOWLOSTPOINTS", True) if dic else True

    def cambiadoShow(self):
        self.tabChanged(self.tabGrid.currentIndex())

    def tableroSizeChanged(self):
        th = self.tablero.height()
        self.tabGrid.setFixedHeight(th)
        self.emIndexes.setFixedHeight(th - 72)
        self.adjustSize()
        self.cambiadoShow()

    def tabChanged(self, ntab):
        QtGui.QApplication.processEvents()
        tab_vis = 0 if ntab == 3 else ntab
        if self.rbShowElo.isChecked():
            tab_vis += 3
        for n in range(6):
            self.htotal[n].setVisible(False)
        self.htotal[tab_vis].setVisible(True)
        self.adjustSize()
        self.tabActive = ntab

    def gridCambiadoRegistro(self, grid, fila, columna):
        self.gridBotonIzquierdo(grid, fila, columna)

    def saveIndexes(self):
        self.gestor.partida.setFirstComment(self.alm.indexesRAW)
        QTUtil2.mensajeTemporal(self, _("Saved"), 1.8)

    def gridBotonIzquierdo(self, grid, fila, columna):
        self.tablero.quitaFlechas()
        jg = self.dicLiJG[grid.id][fila]
        self.tablero.ponPosicion(jg.posicion)
        mrm, pos = jg.analisis
        rm = mrm.liMultiPV[pos]
        self.tablero.ponFlechaSC(rm.desde, rm.hasta)
        rm = mrm.liMultiPV[0]
        self.tablero.creaFlechaMulti(rm.movimiento(), False)
        grid.setFocus()
        ta = self.tabActive if self.tabActive < 3 else 0
        self.htotal[ta].setPointActive(fila)
        self.htotal[ta+3].setPointActive(fila)

        dic, siBlancas = jg.posicion.capturas()
        self.capturas.pon(dic)

    def gridDobleClick(self, grid, fila, columna):
        jg = self.dicLiJG[grid.id][fila]
        mrm, pos = jg.analisis
        self.muestraAnalisis(self.procesador, self.procesador.xtutor, jg, self.tablero.siBlancasAbajo, 999999, pos,
                             pantalla=self, siGrabar=False)

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        nrecno = grid.recno()
        if k in (QtCore.Qt.Key_Enter, QtCore.Qt.Key_Return):
            self.gridDobleClick(grid, nrecno, None)
        elif k == QtCore.Qt.Key_Right:
            if nrecno+1 < self.gridNumDatos(grid):
                grid.goto(nrecno+1, 0)
        elif k == QtCore.Qt.Key_Left:
            if nrecno > 0:
                grid.goto(nrecno-1, 0)

    def gridColorFondo(self, grid, fila, oColumna):
        if grid.id == "A":
            jg = self.alm.lijg[fila]
            return self.colorWhite if jg.xsiW else None
        return None

    def gridAlineacion(self, grid, fila, oColumna):
        if grid.id == "A":
            jg = self.alm.lijg[fila]
            return "i" if jg.xsiW else "d"
        return None

    def gridNumDatos(self, grid):
        return len(self.dicLiJG[grid.id])

    def gridDato(self, grid, fila, oColumna):
        columna = oColumna.clave
        jg = self.dicLiJG[grid.id][fila]
        if columna == "NUM":
            return " %s " % jg.xnum
        elif columna in ("MOVE", "BEST"):
            mrm, pos = jg.analisis
            rm = mrm.liMultiPV[pos if columna == "MOVE" else 0]
            pv1 = rm.pv.split(" ")[0]
            desde = pv1[:2]
            hasta = pv1[2:4]
            coronacion = pv1[4] if len(pv1) == 5 else None
            txt = rm.abrTextoBase()
            if txt:
                txt = " (%s)" % txt
            return jg.posicionBase.pgn(desde, hasta, coronacion) + txt
        elif columna == "DIF":
            mrm, pos = jg.analisis
            rm = mrm.liMultiPV[0]
            rm1 = mrm.liMultiPV[pos]
            pts = rm.puntosABS_5() - rm1.puntosABS_5()
            if self.siPawns:
                pts /= 100.0
                return "%0.2f" % pts
            else:
                return "%d" % pts
        elif columna == "PORC":
            return "%3d%%" % jg.porcentaje
        elif columna == "ELO":
            return "%3d" % jg.elo

    def closeEvent(self, event):
        self.guardarVideo()


def showGraph(wowner, gestor, alm, muestraAnalisis):
    w = WAnalisisGraph(wowner, gestor, alm, muestraAnalisis)
    w.exec_()


class WMuestra(QtGui.QWidget):
    def __init__(self, owner, um):
        super(WMuestra, self).__init__(owner)

        self.um = um
        self.owner = owner

        self.etiquetaMotor = um.etiquetaMotor()
        self.etiquetaTiempo = um.etiquetaTiempo()

        self.tablero = owner.tablero

        self.lbMotorM = Controles.LB(self, self.etiquetaMotor).alinCentrado().ponTipoLetra(puntos=9, peso=75)
        self.lbTiempoM = Controles.LB(self, self.etiquetaTiempo).alinCentrado().ponTipoLetra(puntos=9, peso=75)
        self.dicFonts = {True: "blue", False: "grey"}

        self.btCancelar = Controles.PB(self, "", self.cancelar).ponIcono(Iconos.X())

        self.lbPuntuacion = owner.lbPuntuacion
        self.lbMotor = owner.lbMotor
        self.lbTiempo = owner.lbTiempo
        self.lbPGN = owner.lbPGN

        self.listaRM = um.listaRM
        self.siTiempoActivo = False

        self.colorNegativo = QTUtil.qtColorRGB(255, 0, 0)
        self.colorImpares = QTUtil.qtColorRGB(231, 244, 254)
        oColumnas = Columnas.ListaColumnas()
        self.siFigurinesPGN = VarGen.configuracion.figurinesPGN
        oColumnas.nueva("JUGADAS", "%d %s" % (len(self.listaRM), _("Moves")), 120, siCentrado=True,
                        edicion=Delegados.EtiquetaPGN(um.jg.siBlancas() if self.siFigurinesPGN else None))
        self.wrm = Grid.Grid(self, oColumnas, siLineas=False)

        self.wrm.tipoLetra(puntos=VarGen.configuracion.puntosPGN)
        nAncho = self.wrm.anchoColumnas() + 20
        self.wrm.setFixedWidth(nAncho)
        self.wrm.goto(self.um.posElegida, 0)

        # Layout
        ly2 = Colocacion.H().relleno().control(self.lbTiempoM).relleno().control(self.btCancelar)
        layout = Colocacion.V().control(self.lbMotorM).otro(ly2).control(self.wrm)

        self.setLayout(layout)

        self.wrm.setFocus()

    def activa(self, siActivar):
        color = self.dicFonts[siActivar]
        self.lbMotorM.ponColorN(color)
        self.lbTiempoM.ponColorN(color)
        self.btCancelar.setEnabled(not siActivar)
        self.siTiempoActivo = False

        if siActivar:
            self.lbMotor.ponTexto(self.etiquetaMotor)
            self.lbTiempo.ponTexto(self.etiquetaTiempo)

    def cancelar(self):
        self.owner.borrarMuestra(self.um)

    def cambiadoRM(self, fila):
        self.um.ponPosRMactual(fila)
        self.lbPuntuacion.ponTexto(self.um.puntuacionActual())

        self.lbPGN.ponTexto(self.um.pgnActual())

        self.ponTablero()
        self.owner.adjustSize()
        QTUtil.refreshGUI()

    def ponTablero(self):
        posicion, desde, hasta = self.um.posicionActual()
        self.tablero.ponPosicion(posicion)
        if desde:
            self.tablero.ponFlechaSC(desde, hasta)

    def gridNumDatos(self, grid):
        return len(self.listaRM)

    def gridBotonIzquierdo(self, grid, fila, columna):
        self.cambiadoRM(fila)
        self.owner.activaMuestra(self.um)

    def gridBotonDerecho(self, grid, fila, columna, modificadores):
        self.cambiadoRM(fila)

    def gridBold(self, grid, fila, columna):
        return self.um.siElegido(fila)

    def gridDato(self, grid, fila, oColumna):
        return self.listaRM[fila][1]

    def gridColorTexto(self, grid, fila, oColumna):
        rm = self.listaRM[fila][0]
        return None if rm.puntosABS() >= 0 else self.colorNegativo

    def gridColorFondo(self, grid, fila, oColumna):
        if fila % 2 == 1:
            return self.colorImpares
        else:
            return None

    def situate(self, recno):
        if 0 <= recno < len(self.listaRM):
            self.wrm.goto(recno, 0)
            self.cambiadoRM(recno)
            self.owner.activaMuestra(self.um)

    def abajo(self):
        self.situate(self.wrm.recno() + 1)

    def primero(self):
        self.situate(0)

    def arriba(self):
        self.situate(self.wrm.recno() - 1)

    def ultimo(self):
        self.situate(len(self.listaRM) - 1)

    def procesarTB(self, accion):
        accion = accion[5:]
        if accion in ("Adelante", "Atras", "Inicio", "Final"):
            self.um.cambiaMovActual(accion)
            self.ponTablero()
        elif accion == "Libre":
            self.um.analizaExterior(self.owner, self.owner.siBlancas)
        elif accion == "Tiempo":
            self.lanzaTiempo()
        elif accion == "Grabar":
            self.grabar()
        elif accion == "GrabarTodos":
            self.grabarTodos()
        elif accion == "Jugar":
            self.jugarPosicion()
        elif accion == "FEN":
            QTUtil.ponPortapapeles(self.um.fenActual())
            QTUtil2.mensaje(self, _("FEN is in clipboard"))

    def jugarPosicion(self):
        posicion, desde, hasta = self.um.posicionBaseActual()
        fen = posicion.fen()

        XRun.run_lucas("-play", fen)

    def lanzaTiempo(self):
        self.siTiempoActivo = not self.siTiempoActivo
        if self.siTiempoActivo:
            self.um.cambiaMovActual("Inicio")
            self.ponTablero()
            QtCore.QTimer.singleShot(400, self.siguienteTiempo)

    def siguienteTiempo(self):
        if self.siTiempoActivo:
            self.um.cambiaMovActual("Adelante")
            self.ponTablero()
            if self.um.siPosFinal():
                self.siTiempoActivo = False
            else:
                QtCore.QTimer.singleShot(1400, self.siguienteTiempo)

    def grabar(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion(True, _("Complete variant"), Iconos.PuntoVerde())
        menu.separador()
        menu.opcion(False, _("Only the first move"), Iconos.PuntoRojo())
        resp = menu.lanza()
        if resp is None:
            return
        self.um.grabarBase(self.um.partida, self.um.rm, resp)
        self.um.ponVistaGestor()

    def grabarTodos(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion(True, _("Complete variants"), Iconos.PuntoVerde())
        menu.separador()
        menu.opcion(False, _("Only the first move of each variant"), Iconos.PuntoRojo())
        resp = menu.lanza()
        if resp:
            for pos, tp in enumerate(self.um.listaRM):
                rm = tp[0]
                partida = Partida.Partida(self.um.jg.posicionBase)
                partida.leerPV(rm.pv)
                self.um.grabarBase(partida, rm, resp)
            self.um.ponVistaGestor()


class WAnalisis(QTVarios.WDialogo):
    def __init__(self, mAnalisis, ventana, siBlancas, siLibre, siGrabar, muestraInicial):
        titulo = _("Analysis")
        icono = Iconos.Tutor()
        extparam = "analysis"

        QTVarios.WDialogo.__init__(self, ventana, titulo, icono, extparam)

        self.mAnalisis = mAnalisis
        self.muestraActual = None

        configuracion = VarGen.configuracion
        confTablero = configuracion.confTablero("ANALISIS", 48)
        self.siLibre = siLibre
        self.siGrabar = siGrabar
        self.siBlancas = siBlancas

        tbWork = QTVarios.LCTB(self, tamIcon=24)
        tbWork.new(_("Close"), Iconos.MainMenu(), self.terminar)
        tbWork.new(_("New"), Iconos.NuevoMas(), self.crear)

        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(siBlancas)

        self.lbMotor = Controles.LB(self).alinCentrado()
        self.lbTiempo = Controles.LB(self).alinCentrado()
        self.lbPuntuacion = Controles.LB(self).alinCentrado().ponTipoLetra(puntos=configuracion.puntosPGN, peso=75)
        self.lbPGN = Controles.LB(self).ponWrap().ponTipoLetra(puntos=configuracion.puntosPGN)

        self.setStyleSheet("QStatusBar::item { border-style: outset; border-width: 1px; border-color: LightSlateGray ;}")

        liMasAcciones = (("FEN:%s" % _("Copy to clipboard"), "MoverFEN", Iconos.Clip()),)
        lytb, self.tb = QTVarios.lyBotonesMovimiento(self, "", siLibre=siLibre,
                                                     siGrabar=siGrabar,
                                                     siGrabarTodos=siGrabar,
                                                     siJugar=mAnalisis.maxRecursion > 10,
                                                     liMasAcciones=liMasAcciones)

        lyTabl = Colocacion.H().relleno().control(self.tablero).relleno()

        lyMotor = Colocacion.H().control(self.lbPuntuacion).relleno().control(self.lbMotor).control(self.lbTiempo)

        lyV = Colocacion.V()
        lyV.control(tbWork)
        lyV.otro(lyTabl)
        lyV.otro(lytb)
        lyV.otro(lyMotor)
        lyV.control(self.lbPGN)
        lyV.relleno()

        wm = WMuestra(self, muestraInicial)
        muestraInicial.wmu = wm

        # Layout
        self.ly = Colocacion.H().margen(10)
        self.ly.otro(lyV)
        self.ly.control(wm)

        lyM = Colocacion.H().margen(0).otro(self.ly).relleno()

        layout = Colocacion.V()
        layout.otro(lyM)
        layout.margen(3)
        layout.setSpacing(1)
        self.setLayout(layout)

        self.recuperarVideo(siTam=False)
        self.show()
        wm.cambiadoRM(muestraInicial.posElegida)
        self.activaMuestra(muestraInicial)

    def keyPressEvent(self, event):
        k = event.key()

        if k == 16777237:  # abajo
            self.muestraActual.wmu.abajo()
        elif k == 16777235:  # arriba
            self.muestraActual.wmu.arriba()
        elif k == 16777234:  # izda
            self.muestraActual.wmu.procesarTB("MoverAtras")
        elif k == 16777236:  # dcha
            self.muestraActual.wmu.procesarTB("MoverAdelante")
        elif k == 16777232:  # inicio
            self.muestraActual.wmu.procesarTB("MoverInicio")
        elif k == 16777233:  # final
            self.muestraActual.wmu.procesarTB("MoverFinal")
        elif k == 16777238:  # avpag
            self.muestraActual.wmu.primero()
        elif k == 16777239:  # dnpag
            self.muestraActual.wmu.ultimo()
        elif k == 16777220:  # enter
            self.muestraActual.wmu.procesarTB("MoverLibre")
        elif k == 16777216:  # esc
            self.terminar()

    def closeEvent(self, event):  # Cierre con X
        self.terminar(False)

    def terminar(self, siAccept=True):
        for una in self.mAnalisis.liMuestras:
            una.wmu.siTiempoActivo = False
        self.guardarVideo()
        if siAccept:
            self.accept()

    def activaMuestra(self, um):
        self.muestraActual = um
        for una in self.mAnalisis.liMuestras:
            if hasattr(una, "wmu"):
                una.wmu.activa(una == um)

    def crearMuestra(self, um):
        wm = WMuestra(self, um)
        self.ly.control(wm)
        wm.show()

        um.ponWMU(wm)

        self.activaMuestra(um)

        wm.gridBotonIzquierdo(wm.wrm, um.posRMactual, 0)

        return wm

    def borrarMuestra(self, um):
        um.desactiva()
        self.adjustSize()
        QTUtil.refreshGUI()

    def procesarTB(self):
        clave = self.sender().clave
        if clave == "terminar":
            self.terminar()
            self.accept()
        elif clave == "crear":
            self.crear()
        else:
            self.muestraActual.wmu.procesarTB(clave)

    def iniciaReloj(self, funcion):
        if not hasattr(self, "timer"):
            self.timer = QtCore.QTimer(self)
            self.connect(self.timer, QtCore.SIGNAL("timeout()"), funcion)
        self.timer.start(1000)

    def paraReloj(self):
        if hasattr(self, "timer"):
            self.timer.stop()
            delattr(self, "timer")

    def crear(self):
        alm = PantallaAnalisisParam.paramAnalisis(self, VarGen.configuracion, False, siTodosMotores=True)
        if alm:
            um = self.mAnalisis.creaMuestra(self, alm)
            self.crearMuestra(um)


class WAnalisisVariantes(QtGui.QDialog):
    def __init__(self, oBase, ventana, segundosPensando, siBlancas, cPuntos, maxRecursion):
        super(WAnalisisVariantes, self).__init__(ventana)

        self.oBase = oBase

        # Creamos los controles
        self.setWindowTitle(_("Variants"))

        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowMinimizeButtonHint)
        self.setWindowIcon(Iconos.Tutor())

        f = Controles.TipoLetra(puntos=12, peso=75)
        flb = Controles.TipoLetra(puntos=10)

        lbPuntuacionAnterior = Controles.LB(self, cPuntos).alinCentrado().ponFuente(flb)
        self.lbPuntuacionNueva = Controles.LB(self).alinCentrado().ponFuente(flb)

        confTablero = VarGen.configuracion.confTablero("ANALISISVARIANTES", 32)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(siBlancas)

        self.tableroT = Tablero.Tablero(self, confTablero)
        self.tableroT.crea()
        self.tableroT.ponerPiezasAbajo(siBlancas)

        btTerminar = Controles.PB(self, _("Close"), self.close).ponPlano(False)
        btReset = Controles.PB(self, _("Another change"), oBase.reset).ponIcono(Iconos.MoverLibre()).ponPlano(False)
        liMasAcciones = (("FEN:%s" % _("Copy to clipboard"), "MoverFEN", Iconos.Clip()),)
        lytbTutor, self.tb = QTVarios.lyBotonesMovimiento(self, "", siLibre=maxRecursion > 0,
                                                          liMasAcciones=liMasAcciones)
        self.maxRecursion = maxRecursion - 1

        self.segundos, lbSegundos = QTUtil2.spinBoxLB(self, segundosPensando, 1, 999, maxTam=40, etiqueta=_("Second(s)"))

        # Creamos los layouts

        lyVariacion = Colocacion.V().control(lbPuntuacionAnterior).control(self.tablero)
        gbVariacion = Controles.GB(self, _("Proposed change"), lyVariacion).ponFuente(f).alinCentrado()

        lyTutor = Colocacion.V().control(self.lbPuntuacionNueva).control(self.tableroT)
        gbTutor = Controles.GB(self, _("Tutor's prediction"), lyTutor).ponFuente(f).alinCentrado()

        lyBT = Colocacion.H().control(btTerminar).control(btReset).relleno().control(lbSegundos).control(self.segundos)

        layout = Colocacion.G().control(gbVariacion, 0, 0).control(gbTutor, 0, 1)
        layout.otro(lyBT, 1, 0).otro(lytbTutor, 1, 1)

        self.setLayout(layout)

        self.move(ventana.x() + 20, ventana.y() + 20)

    def dameSegundos(self):
        return int(self.segundos.value())

    def ponPuntuacion(self, pts):
        self.lbPuntuacionNueva.ponTexto(pts)

    def procesarTB(self):
        self.oBase.procesarTB(self.sender().clave, self.maxRecursion)

    def iniciaReloj(self, funcion):
        if not hasattr(self, "timer"):
            self.timer = QtCore.QTimer(self)
            self.connect(self.timer, QtCore.SIGNAL("timeout()"), funcion)
        self.timer.start(1000)

    def paraReloj(self):
        if hasattr(self, "timer"):
            self.timer.stop()
            delattr(self, "timer")

    def closeEvent(self, event):  # Cierre con X
        self.paraReloj()

    def keyPressEvent(self, event):
        k = event.key()
        if k == 16777237:  # abajo
            clave = "MoverAtras"
        elif k == 16777235:  # arriba
            clave = "MoverAdelante"
        elif k == 16777234:  # izda
            clave = "MoverAtras"
        elif k == 16777236:  # dcha
            clave = "MoverAdelante"
        elif k == 16777232:  # inicio
            clave = "MoverInicio"
        elif k == 16777233:  # final
            clave = "MoverFinal"
        elif k == 16777216:  # esc
            self.paraReloj()
            self.accept()
        else:
            return
        self.oBase.procesarTB(clave, self.maxRecursion)
