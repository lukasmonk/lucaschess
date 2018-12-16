from PyQt4 import QtGui, QtSvg

from Code import Analisis
from Code import ControlPosicion
from Code import Jugada
from Code import Partida
from Code import TrListas
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import WorkMap


class WMap(QTVarios.WDialogo):
    def __init__(self, procesador, mapa):
        self.workmap = WorkMap.WorkMap(mapa)
        dic = TrListas.maps()
        titulo = dic[mapa]
        icono = getattr(Iconos, mapa)()

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, mapa)

        self.procesador = procesador

        self.playCurrent = None

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("TIPO", "", 24, edicion=Delegados.PmIconosBMT(), siCentrado=True)
        oColumnas.nueva("SELECT", _("Select one to play"), 150)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, xid="W")

        self.registrarGrid(self.grid)

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Play"), Iconos.Empezar(), self.play), None,
        )
        tbWork = QTVarios.LCTB(self, liAcciones, tamIcon=24)

        self.lbInfo = Controles.LB(self)

        self.wsvg = wsvg = QtSvg.QSvgWidget()
        p = wsvg.palette()
        p.setColor(wsvg.backgroundRole(), QtGui.QColor("#F5F5F5"))
        wsvg.setPalette(p)

        ly = Colocacion.V().control(tbWork).control(self.lbInfo).control(self.grid)
        w = QtGui.QWidget()
        w.setLayout(ly)

        splitter = QtGui.QSplitter(self)
        splitter.addWidget(w)
        splitter.addWidget(wsvg)
        self.registrarSplitter(splitter, "splitter")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ACTIVE", _("Active"), 80, siCentrado=True)
        oColumnas.nueva("TIPO", _("Type"), 110, siCentrado=True)
        oColumnas.nueva("DCREATION", _("Creation date"), 110, siCentrado=True)
        oColumnas.nueva("DONE", _("Done"), 110, siCentrado=True)
        oColumnas.nueva("DEND", _("Ending date"), 110, siCentrado=True)
        oColumnas.nueva("RESULT", _("Result"), 110, siCentrado=True)

        self.gridData = Grid.Grid(self, oColumnas, siSelecFilas=True, xid="H")
        self.registrarGrid(self.gridData)

        liAcciones = ((_("Close"), Iconos.MainMenu(), self.terminar), None,
                      (_("Select"), Iconos.Seleccionar(), self.data_select), None,
                      (_("New"), Iconos.NuevoMas(), self.data_new), None,
                      (_("Remove"), Iconos.Borrar(), self.data_remove), None,
                      )
        tb = QTVarios.LCTB(self, liAcciones, tamIcon=24)

        ly = Colocacion.V().control(tb).control(self.gridData)
        w = QtGui.QWidget()
        w.setLayout(ly)

        self.tab = Controles.Tab()
        self.tab.ponPosicion("W")
        self.tab.nuevaTab(splitter, _("Map"))
        self.tab.nuevaTab(w, _("Data"))

        ly = Colocacion.H().control(self.tab).margen(0)
        self.setLayout(ly)

        self.recuperarVideo(siTam=True, anchoDefecto=960, altoDefecto=600)

        self.workmap.setWidget(wsvg)
        self.workmap.resetWidget()
        self.grid.gotop()
        self.gridData.gotop()

        self.informacion()

    def data_new(self):
        menu = QTVarios.LCMenu(self)

        menu1 = menu.submenu(_("Checkmates in GM games"), Iconos.GranMaestro())
        menu1.opcion("mate_basic", _("Basic"), Iconos.PuntoAzul())
        menu1.separador()
        menu1.opcion("mate_easy", _("Easy"), Iconos.PuntoAmarillo())
        menu1.opcion("mate_medium", _("Medium"), Iconos.PuntoNaranja())
        menu1.opcion("mate_hard", _("Hard"), Iconos.PuntoRojo())

        menu.separador()
        menu.opcion("sts_basic", _("STS: Strategic Test Suite"), Iconos.STS())

        resp = menu.lanza()
        if resp:
            tipo, model = resp.split("_")
            if tipo == "sts":
                liGen = [(None, None)]
                liR = [(str(x), x) for x in range(1, 100)]
                config = FormLayout.Combobox(_("Model"), liR)
                liGen.append((config, "1"))
                resultado = FormLayout.fedit(liGen, title=_("STS: Strategic Test Suite"), parent=self, anchoMinimo=160, icon=Iconos.Maps())
                if resultado is None:
                    return
                accion, liResp = resultado
                model = liResp[0]
            self.workmap.nuevo(tipo, model)
            self.activaWorkmap()

    def doWork(self, fila):
        tipo = self.workmap.TIPO
        if tipo == "mate":
            self.playCurrent = self.workmap
            self.guardarVideo()
            self.accept()

        elif tipo == "sts":
            w = WUnSTSMap(self)
            w.exec_()
            self.gridData.refresh()
            self.workmap.resetWidget()
            self.informacion()
            self.grid.refresh()

    def data_select(self):
        fila = self.gridData.recno()
        self.workmap.activaRowID(fila)
        self.activaWorkmap(siGoTop=False)

    def activaWorkmap(self, siGoTop=True):
        self.workmap.setWidget(self.wsvg)
        self.workmap.resetWidget()
        self.grid.refresh()
        self.gridData.refresh()

        self.grid.gotop()
        if siGoTop:
            self.gridData.gotop()

        self.informacion()

    def data_remove(self):
        raw = self.workmap.db.listaRaws[self.gridData.recno()]
        if raw["ACTIVE"] != "X":
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), _("this work"))):
                self.workmap.db.borra(raw["ROWID"])
                self.gridData.refresh()

    def informacion(self):
        current = self.workmap.nameCurrent()
        total = self.workmap.total()
        hechos, total = self.workmap.done()
        info = self.workmap.info()
        tipo = self.workmap.db.getTipo()
        txt = '<b><span style="color:#C156F8">%s: %s</span>' % (_("Active"), current) if current else ""
        txt += '<br><span style="color:brown">%s: %s</span></b>' % (_("Type"), tipo) + \
               '<br><span style="color:teal">%s: %d/%d</span></b>' % (_("Done"), hechos, total) + \
               '<br><span style="color:blue">%s: %s</span></b>' % (_("Result"), info)
        self.lbInfo.ponTexto(txt)

    def lanza(self, fila):
        siHecho = self.workmap.setAimFila(fila)
        if siHecho:
            self.workmap.resetWidget()
            self.informacion()
            self.grid.gotop()
            self.grid.refresh()
        else:
            self.doWork(fila)

    def gridDobleClick(self, grid, fila, columna):
        if grid == self.grid:
            self.lanza(fila)
        else:
            self.data_select()

    def play(self):
        fila = self.grid.recno()
        self.lanza(fila)

    def terminar(self):
        self.guardarVideo()
        self.reject()

    def gridNumDatos(self, grid):
        return self.workmap.numDatos() if grid.id == "W" else self.workmap.db.numDatos()

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        return self.workmap.dato(fila, clave) if grid.id == "W" else self.workmap.db.dato(fila, clave)


class WUnSTSMap(QTVarios.WDialogo):
    def __init__(self, owner):

        self.workmap = owner.workmap
        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion
        self.alm = self.workmap.getAim()

        QTVarios.WDialogo.__init__(self, owner, _("STS: Strategic Test Suite"), Iconos.STS(), "stsmap")

        # Tablero
        confTablero = self.configuracion.confTablero("STSMAP", 48)

        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueveHumano)

        # Rotulos informacion
        self.lbJuego = Controles.LB(self).ponWrap().anchoMinimo(200)

        # Tool bar
        self.liAcciones = (
            (_("Continue"), Iconos.Pelicula_Seguir(), self.seguir),
            (_("Cancel"), Iconos.Cancelar(), self.cancelar),
            (_("Analysis"), Iconos.Tutor(), self.analizar),
        )
        self.tb = QTVarios.LCTB(self, self.liAcciones)

        lyT = Colocacion.V().control(self.tablero).relleno()
        lyV = Colocacion.V().relleno().control(self.lbJuego).relleno(2)
        lyTV = Colocacion.H().otro(lyT).otro(lyV)
        ly = Colocacion.V().control(self.tb).otro(lyTV)

        self.setLayout(ly)

        self.recuperarVideo()

        self.ponToolbar(self.cancelar)
        self.ponJuego()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def seguir(self):
        self.cancelar()

    def ponToolbar(self, *liCurrent):
        for txt, ico, rut in self.liAcciones:
            self.tb.setAccionVisible(rut, rut in liCurrent)

    def ponJuego(self):
        self.ponToolbar(self.cancelar)

        self.posicion = cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.alm.fen)

        mens = "<h2>%s</h2><br>" % self.alm.name

        siW = cp.siBlancas
        color, colorR = _("White"), _("Black")
        cK, cQ, cKR, cQR = "K", "Q", "k", "q"
        if not siW:
            color, colorR = colorR, color
            cK, cQ, cKR, cQR = cKR, cQR, cK, cQ

        if cp.enroques:
            def menr(ck, cq):
                enr = ""
                if ck in cp.enroques:
                    enr += "O-O"
                if cq in cp.enroques:
                    if enr:
                        enr += "  +  "
                    enr += "O-O-O"
                return enr

            enr = menr(cK, cQ)
            if enr:
                mens += "<br>%s : %s" % (color, enr)
            enr = menr(cKR, cQR)
            if enr:
                mens += "<br>%s : %s" % (colorR, enr)
        if cp.alPaso != "-":
            mens += "<br>     %s : %s" % (_("En passant"), cp.alPaso)
        self.lbJuego.ponTexto(mens)

        siW = cp.siBlancas
        self.tablero.ponPosicion(cp)
        self.tablero.ponerPiezasAbajo(siW)
        self.tablero.ponIndicador(siW)
        self.tablero.activaColor(siW)

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.tablero.desactivaTodas()

        # Peon coronando
        if not coronacion and self.posicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.posicion.siBlancas)
            if coronacion is None:
                self.ponJuego()
                return False

        siBien, mens, jg = Jugada.dameJugada(self.posicion, desde, hasta, coronacion)
        if siBien:
            self.tablero.ponPosicion(jg.posicion)
            self.tablero.ponFlechaSC(desde, hasta)
            self.hechaJugada(jg)
        else:
            self.ponJuego()
            return False
        return True

    def hechaJugada(self, jg):
        self.tablero.desactivaTodas()
        self.jg = jg

        self.ponToolbar(self.seguir, self.analizar)

        donePV = jg.movimiento().lower()
        dicResults = self.alm.dicResults

        mens = "<h2>%s</h2><br>" % self.alm.name

        mens += '<table><tr><th>%s</th><th>%s</th></tr>' % (_("Move"), _("Points"))
        mx = 0
        ok = False
        stylePV = ' style="color:red;"'
        for pv, points in dicResults.iteritems():
            if donePV == pv.lower():
                ok = True
                mas = stylePV
            else:
                mas = ""
            san = Partida.pv_san(self.alm.fen, pv)
            mens += '<tr%s><td align="center">%s</td><td align="right">%d</td></tr>' % (mas, san, points)
            if points > mx:
                mx = points
        if not ok:
            san = Partida.pv_san(self.alm.fen, donePV)
            mens += '<tr%s><td align="center">%s</td><td align="right">%d</td></tr>' % (stylePV, san, 0)
        mens += "</table>"

        self.alm.donePV = donePV
        self.alm.puntos = dicResults.get(donePV, 0)
        self.alm.total = mx

        mens += "<br><h2>%s: %d/%d</h2>" % (_("Points"), self.alm.puntos, self.alm.total)
        self.lbJuego.ponTexto(mens)

        self.workmap.winAim(donePV)

    def analizar(self):
        xtutor = self.procesador.XTutor()
        Analisis.muestraAnalisis(self.procesador, xtutor, self.jg, self.posicion.siBlancas,
                                 9999999, 1, pantalla=self, siGrabar=False)


def train_map(procesador, mapa):
    w = WMap(procesador, mapa)
    w.exec_()
    return w.playCurrent
