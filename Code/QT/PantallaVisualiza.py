import os.path
import time

import LCEngine4 as LCEngine
from PyQt4 import QtCore

from Code import ControlPosicion
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import Util


class WControl(QTVarios.WDialogo):
    def __init__(self, procesador, path_bloque):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, _("The board at a glance"), Iconos.Gafas(),
                                   "visualizaBase")

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.path_bloque = path_bloque

        fichero = os.path.join(self.configuracion.carpeta, os.path.basename(path_bloque) + "db")
        self.historico = Util.DicSQL(fichero)
        self.liHistorico = self.calcListaHistorico()

        # Historico
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("SITE", _("Site"), 100, siCentrado=True)
        oColumnas.nueva("DATE", _("Date"), 100, siCentrado=True)
        oColumnas.nueva("LEVEL", _("Level"), 80, siCentrado=True)
        oColumnas.nueva("TIME", _("Time used"), 80, siCentrado=True)
        oColumnas.nueva("ERRORS", _("Errors"), 80, siCentrado=True)
        oColumnas.nueva("INTERVAL", _("Interval"), 100, siCentrado=True)
        oColumnas.nueva("POSITION", _("Position"), 80, siCentrado=True)
        oColumnas.nueva("COLOR", _("Square color"), 80, siCentrado=True)
        oColumnas.nueva("ISATTACKED", _("Is attacked?"), 80, siCentrado=True)
        oColumnas.nueva("ISATTACKING", _("Is attacking?"), 80, siCentrado=True)
        self.ghistorico = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.ghistorico.setMinimumWidth(self.ghistorico.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Play"), Iconos.Empezar(), self.play), None,
            (_("New"), Iconos.Nuevo(), self.new), None,
            (_("Remove"), Iconos.Borrar(), self.remove), None,
        )
        self.tb = Controles.TBrutina(self, liAcciones)

        # Colocamos
        ly = Colocacion.V().control(self.tb).control(self.ghistorico).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.ghistorico)
        self.recuperarVideo()
        self.ghistorico.gotop()

    def gridNumDatos(self, grid):
        return len(self.liHistorico)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        key = self.liHistorico[fila]
        reg = self.historico[key]
        v = reg[col]
        if col == "DATE":
            v = Util.localDate(reg[col])
        elif col == "ERRORS":
            v = "%d" % v
        elif col == "TIME":
            if v > 60:
                m = v / 60
                s = v % 60
                v = "%d\'%d\"" % (m, s)
            else:
                v = "%d\"" % v
        elif col == "INTERVAL":
            v = '%d"' % v
            if reg["INTERVALPIECE"]:
                v = "x %s" % v
        elif col == "LEVEL":
            v = _("Finished") if v == 0 else str(v)
        elif col in ("POSITION", "COLOR", "ISATTACKED", "ISATTACKING"):
            v = _("Yes") if v else _("No")
        return v

    def calcListaHistorico(self):
        return self.historico.keys(siOrdenados=True, siReverse=True)

    def terminar(self):
        self.guardarVideo()
        self.historico.close()
        self.reject()

    def play(self):
        if not self.liHistorico:
            return self.new()

        recno = self.ghistorico.recno()
        if recno >= 0:
            key = self.liHistorico[recno]
            reg = self.historico[key]
            if reg["LEVEL"] > 0:
                return self.work(recno)

    def new(self):
        recno = self.ghistorico.recno()
        if recno >= 0:
            key = self.liHistorico[recno]
            reg = self.historico[key]
            sitePre = reg["SITE"]
            intervaloPre = reg["INTERVAL"]
            intervaloPorPiezaPre = reg["INTERVALPIECE"]
            esatacadaPre = reg["ISATTACKED"]
            esatacantePre = reg["ISATTACKING"]
            posicionPre = reg["POSITION"]
            colorPre = reg["COLOR"]
        else:
            recno = 0
            sitePre = None
            intervaloPre = 3
            intervaloPorPiezaPre = True
            esatacadaPre = False
            esatacantePre = False
            posicionPre = False
            colorPre = False

        # Datos
        liGen = [(None, None)]

        # # Site
        f = open(self.path_bloque)
        liData = [x.split("|") for x in f.read().split("\n")]
        f.close()
        liSites = []
        sitePreNum = -1
        for n, uno in enumerate(liData):
            site = uno[0]
            if site:
                if sitePre and site == sitePre:
                    sitePreNum = n
                liSites.append((site, n))
        liSites = sorted(liSites, key=lambda st: st[0])
        config = FormLayout.Combobox(_("Site"), liSites)
        if sitePreNum == -1:
            sitePreNum = liSites[0][0]
        liGen.append((config, sitePreNum))

        liGen.append((None, None))

        # # Intervals
        liGen.append((None, _("Seconds of every glance") + ":"))
        liGen.append((FormLayout.Spinbox(_("Seconds"), 1, 100, 50), intervaloPre))

        liTypes = ((_("By piece"), True), (_("Fixed"), False))
        config = FormLayout.Combobox(_("Type"), liTypes)
        liGen.append((config, intervaloPorPiezaPre))

        liGen.append((None, None))

        liGen.append((None, _("Ask for") + ":"))
        liGen.append((_("Position") + ":", posicionPre))
        liGen.append((_("Square color") + ":", colorPre))
        liGen.append((_("Is attacked?") + ":", esatacadaPre))
        liGen.append((_("Is attacking?") + ":", esatacantePre))

        resultado = FormLayout.fedit(liGen, title=_("Configuration"), parent=self, icon=Iconos.Gafas(), anchoMinimo=360)
        if resultado:
            accion, liGen = resultado

            siteNum, \
            intervalo, intervaloPorPieza, \
            posicion, color, esatacada, esatacante = liGen

            dicdatos = {}
            f = dicdatos["DATE"] = Util.hoy()
            dicdatos["FENS"] = liData[siteNum][1:]
            dicdatos["SITE"] = liData[siteNum][0]
            dicdatos["INTERVAL"] = intervalo
            dicdatos["INTERVALPIECE"] = intervaloPorPieza
            dicdatos["ISATTACKED"] = esatacada
            dicdatos["ISATTACKING"] = esatacante
            dicdatos["POSITION"] = posicion
            dicdatos["COLOR"] = color
            dicdatos["ERRORS"] = 0
            dicdatos["TIME"] = 0
            dicdatos["LEVEL"] = 1

            key = Util.dtosext(f)
            self.historico[key] = dicdatos
            self.liHistorico.insert(0, key)
            self.ghistorico.refresh()
            self.ghistorico.gotop()

            self.work(recno)

    def work(self, recno):
        key = self.liHistorico[recno]
        dicdatos = self.historico[key]

        w = WPlay(self, dicdatos)
        w.exec_()

        self.historico[key] = dicdatos
        self.ghistorico.refresh()

    def gridDobleClick(self, grid, fila, columna):
        key = self.liHistorico[fila]
        dicdatos = self.historico[key]
        if dicdatos["LEVEL"]:
            self.work(fila)

    def remove(self):
        li = self.ghistorico.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                um = QTUtil2.unMomento(self)
                for fila in li:
                    key = self.liHistorico[fila]
                    del self.historico[key]
                self.historico.pack()
                self.liHistorico = self.calcListaHistorico()
                um.final()
                self.ghistorico.refresh()


class WPlay(QTVarios.WDialogo):
    def __init__(self, owner, dicdatos):

        self.dicdatos = dicdatos

        site = dicdatos["SITE"]
        self.level = dicdatos["LEVEL"]
        self.intervalo = dicdatos["INTERVAL"]
        self.intervaloPorPieza = dicdatos["INTERVALPIECE"]
        self.esatacada = dicdatos["ISATTACKED"]
        self.esatacante = dicdatos["ISATTACKING"]
        self.posicion = dicdatos["POSITION"]
        self.color = dicdatos["COLOR"]
        self.errors = dicdatos["ERRORS"]
        self.time = dicdatos["TIME"]
        self.liFENs = dicdatos["FENS"]

        mas = "x" if self.intervaloPorPieza else ""
        titulo = "%s (%s%d\")" % (site, mas, self.intervalo)

        super(WPlay, self).__init__(owner, titulo, Iconos.Gafas(), "visualplay")

        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion

        # Tiempo en tablero
        intervaloMax = self.intervalo
        if self.intervaloPorPieza:
            intervaloMax *= 32

        # Tablero
        confTablero = self.configuracion.confTablero("VISUALPLAY", 48)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()

        lyT = Colocacion.V().control(self.tablero)

        self.gbTablero = Controles.GB(self, "", lyT)

        # entradas
        ly = Colocacion.G()

        self.posPosicion = None
        self.posColor = None
        self.posIsAttacked = None
        self.posIsAttacking = None

        lista = [_("Piece")]
        if self.posicion:
            lista.append(_("Position"))
        if self.color:
            lista.append(_("Square color"))
        if self.esatacada:
            lista.append(_("Is attacked?"))
        if self.esatacante:
            lista.append(_("Is attacking?"))
        self.liLB2 = []
        for col, eti in enumerate(lista):
            ly.control(Controles.LB(self, eti), 0, col + 1)
            lb2 = Controles.LB(self, eti)
            ly.control(lb2, 0, col + len(lista) + 2)
            self.liLB2.append(lb2)
        elementos = len(lista) + 1

        liComboPieces = []
        for c in "PNBRQKpnbrqk":
            liComboPieces.append(("", c, self.tablero.piezas.icono(c)))

        self.pmBien = Iconos.pmAceptarPeque()
        self.pmMal = Iconos.pmCancelarPeque()
        self.pmNada = Iconos.pmPuntoAmarillo()

        self.liBloques = []
        for x in range(32):
            fila = x % 16 + 1
            colPos = elementos if x > 15 else 0

            unBloque = []
            self.liBloques.append(unBloque)

            # # Solucion
            lb = Controles.LB(self, "").ponImagen(self.pmNada)
            ly.control(lb, fila, colPos)
            unBloque.append(lb)

            # # Piezas
            colPos += 1
            cb = Controles.CB(self, liComboPieces, "P")
            # cb.setStyleSheet("* { min-height:32px }")
            cb.setIconSize(QtCore.QSize(20, 20))

            ly.control(cb, fila, colPos)
            unBloque.append(cb)

            if self.posicion:
                ec = Controles.ED(self, "").caracteres(2).controlrx("(|[a-h][1-8])").anchoFijo(24).alinCentrado()
                colPos += 1
                ly.controlc(ec, fila, colPos)
                unBloque.append(ec)

            if self.color:
                cl = QTVarios.TwoImages(Iconos.pmBlancas(), Iconos.pmNegras())
                colPos += 1
                ly.controlc(cl, fila, colPos)
                unBloque.append(cl)

            if self.esatacada:
                isat = QTVarios.TwoImages(Iconos.pmAtacada().scaledToWidth(24), Iconos.pmPuntoNegro())
                colPos += 1
                ly.controlc(isat, fila, colPos)
                unBloque.append(isat)

            if self.esatacante:
                at = QTVarios.TwoImages(Iconos.pmAtacante().scaledToWidth(24), Iconos.pmPuntoNegro())
                colPos += 1
                ly.controlc(at, fila, colPos)
                unBloque.append(at)

        ly1 = Colocacion.H().otro(ly).relleno()
        ly2 = Colocacion.V().otro(ly1).relleno()
        self.gbSolucion = Controles.GB(self, "", ly2)

        f = Controles.TipoLetra("", 11, 80, False, False, False, None)

        bt = Controles.PB(self, _("End game"), self.terminar, plano=False).ponIcono(Iconos.FinPartida()).ponFuente(f)
        self.btTablero = Controles.PB(self, _("Go to board"), self.activaTablero, plano=False).ponIcono(
                Iconos.Tablero()).ponFuente(f)
        self.btComprueba = Controles.PB(self, _("Test the solution"), self.compruebaSolucion, plano=False).ponIcono(
                Iconos.Check()).ponFuente(f)
        self.btGotoNextLevel = Controles.PB(self, _("Go to next level"), self.gotoNextLevel, plano=False).ponIcono(
                Iconos.GoToNext()).ponFuente(f)
        ly0 = Colocacion.H().control(bt).relleno().control(self.btTablero).control(self.btComprueba).control(
                self.btGotoNextLevel)

        lyBase = Colocacion.H().control(self.gbTablero).control(self.gbSolucion)

        layout = Colocacion.V().otro(ly0).otro(lyBase)

        self.setLayout(layout)

        self.recuperarVideo()

        self.gotoNextLevel()

    def miraTiempo(self):
        if self.iniTime:
            t = int(time.time() - self.iniTime)
            self.iniTime = None
            self.dicdatos["TIME"] += t

    def terminar(self):
        self.miraTiempo()
        self.accept()

    def closeEvent(self, event):
        self.miraTiempo()

    def gotoNextLevel(self):
        rotulo = _("Level %d") % self.level
        self.gbSolucion.setTitle(rotulo)

        fen = self.liFENs[self.level - 1]

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fen)
        cp.legal()
        self.tablero.ponerPiezasAbajo(cp.siBlancas)
        self.tablero.ponPosicion(cp)

        mens = ""
        if cp.enroques:
            if ("K" if cp.siBlancas else "k") in cp.enroques:
                mens = "O-O"
            if ("Q" if cp.siBlancas else "q") in cp.enroques:
                if mens:
                    mens += " + "
                mens += "O-O-O"
            if mens:
                mens = _("Castling moves possible") + ": " + mens
        if cp.alPaso != "-":
            mens += " " + _("En passant") + ": " + cp.alPaso
        self.rotuloTablero = _("White") if cp.siBlancas else _("Black")
        if mens:
            self.rotuloTablero += " " + mens

        self.cp = cp

        self.intervaloMax = self.intervalo
        if self.intervaloPorPieza:
            self.intervaloMax *= self.level + 2
        self.ponTiempo(self.intervaloMax)

        for x in range(32):
            bloque = self.liBloques[x]
            siVisible = x < self.level + 2
            for elem in bloque:
                elem.setVisible(siVisible)
            if siVisible:
                bloque[0].ponImagen(self.pmNada)
                pz = "K" if x == 0 else ("k" if x == 1 else "P")
                bloque[1].ponValor(pz)
                pos = 1
                if self.posicion:
                    pos += 1
                    bloque[pos].ponTexto("")
                if self.color:
                    pos += 1
                    bloque[pos].valor(True)
                if self.esatacada:
                    pos += 1
                    bloque[pos].valor(False)
                if self.esatacante:
                    pos += 1
                    bloque[pos].valor(False)

        for lb in self.liLB2:
            lb.setVisible(self.level >= 16)

        self.activaTablero()
        QtCore.QTimer.singleShot(1000, self.compruebaTiempo)
        self.iniTime = time.time()

    def compruebaTiempo(self):
        t = round(time.time() - self.iniTimeTablero, 0)
        r = self.intervaloMax - int(t)

        if r <= 0:
            self.activaSolucion()
        else:
            self.ponTiempo(r)
            QtCore.QTimer.singleShot(1000, self.compruebaTiempo)

    def activaTablero(self):
        self.iniTimeTablero = time.time()
        self.gbSolucion.hide()
        self.btTablero.hide()
        self.btComprueba.hide()
        self.btGotoNextLevel.hide()
        self.compruebaTiempo()
        self.gbTablero.show()
        self.gbTablero.adjustSize()
        self.adjustSize()

    def activaSolucion(self):
        self.gbTablero.hide()
        self.btTablero.show()
        self.btComprueba.show()
        self.btGotoNextLevel.hide()
        self.gbSolucion.show()
        self.adjustSize()

    def compruebaSolucion(self):
        liSolucion = self.calculaSolucion()
        nErrores = 0
        for x in range(self.level + 2):
            bloque = self.liBloques[x]

            pieza = bloque[1].valor()
            pos = 1
            if self.posicion:
                pos += 1
                posicion = bloque[pos].texto()
            if self.color:
                pos += 1
                color = bloque[pos].valor()
            if self.esatacada:
                pos += 1
                atacada = bloque[pos].valor()
            if self.esatacante:
                pos += 1
                atacante = bloque[pos].valor()

            correcta = False
            for rsol in liSolucion:
                if rsol.comprobada:
                    continue
                if rsol.pieza != pieza:
                    continue
                if self.posicion:
                    if rsol.posicion != posicion:
                        continue
                if self.color:
                    if rsol.color != color:
                        continue
                if self.esatacada:
                    if rsol.atacada != atacada:
                        continue
                if self.esatacante:
                    if rsol.atacante != atacante:
                        continue
                correcta = True
                rsol.comprobada = True
                break

            bloque[0].ponImagen(self.pmBien if correcta else self.pmMal)
            if not correcta:
                nErrores += 1
        if nErrores == 0:
            self.miraTiempo()
            self.gbSolucion.show()
            self.ponTiempo(0)
            self.gbTablero.show()
            self.btComprueba.hide()
            self.btTablero.hide()
            self.level += 1
            if self.level > 32:
                self.level = 0
            else:
                self.btGotoNextLevel.show()
            self.dicdatos["LEVEL"] = self.level
        else:
            self.dicdatos["ERRORS"] += nErrores

    def ponTiempo(self, num):
        titulo = self.rotuloTablero
        if num:
            titulo += "[ %s ]" % num
        self.gbTablero.setTitle(titulo)

    def calculaSolucion(self):
        fenMB = self.cp.fen()
        fenOB = fenMB.replace(" w ", " b ") if "w" in fenMB else fenMB.replace(" b ", " w ")
        stAttacKing = set()
        stAttacKed = set()
        for fen in (fenMB, fenOB):
            LCEngine.setFen(fen)
            liMV = LCEngine.getExMoves()
            for mv in liMV:
                if mv.captura():
                    stAttacKing.add(mv.desde())
                    stAttacKed.add(mv.hasta())

        liSolucion = []
        for posicion, pieza in self.cp.casillas.iteritems():
            if pieza:
                reg = Util.Almacen()
                reg.pieza = pieza
                reg.posicion = posicion

                lt = posicion[0]
                nm = int(posicion[1])
                iswhite = nm % 2 == 0
                if lt in "bdfh":
                    iswhite = not iswhite
                reg.color = iswhite

                reg.atacante = posicion in stAttacKing
                reg.atacada = posicion in stAttacKed
                reg.comprobada = False
                liSolucion.append(reg)
        return liSolucion


def pantallaVisualiza(procesador):
    w = WControl(procesador, "./IntFiles/Visual/R50-01.vis")
    w.exec_()
