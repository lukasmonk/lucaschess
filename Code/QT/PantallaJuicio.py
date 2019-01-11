from PyQt4 import QtCore, QtGui

from Code import Analisis
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import Util


class WJuicio(QTVarios.WDialogo):
    def __init__(self, gestor, xmotor, nombreOP, posicion, mrm, rmOP, rmUsu, analisis, siCompetitivo=None):
        self.siCompetitivo = gestor.siCompetitivo if siCompetitivo is None else siCompetitivo
        self.nombreOP = nombreOP
        self.posicion = posicion
        self.rmOP = rmOP
        self.rmUsu = rmUsu
        self.mrm = mrm
        self.analisis = analisis
        self.siAnalisisCambiado = False
        self.xmotor = xmotor
        self.gestor = gestor

        self.listaRM, self.posOP = self.hazListaRM()

        titulo = _("Analysis")
        icono = Iconos.Analizar()
        extparam = "jzgm"
        QTVarios.WDialogo.__init__(self, gestor.pantalla, titulo, icono, extparam)

        self.colorNegativo = QTUtil.qtColorRGB(255, 0, 0)
        self.colorImpares = QTUtil.qtColorRGB(231, 244, 254)

        self.lbComentario = Controles.LB(self, "").ponTipoLetra(puntos=10).alinCentrado()

        confTablero = gestor.configuracion.confTablero("JUICIO", 32)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(posicion.siBlancas)

        self.lbMotor = Controles.LB(self).alinCentrado()
        self.lbTiempo = Controles.LB(self).alinCentrado()

        liMas = ((_("Close"), "close", Iconos.AceptarPeque()),)
        lyBM, tbBM = QTVarios.lyBotonesMovimiento(self, "", siLibre=False, tamIcon=24, siMas=gestor.continueTt, liMasAcciones=liMas)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("POSREAL", "#", 40, siCentrado=True)
        oColumnas.nueva("JUGADAS", "%d %s" % (len(self.listaRM), _("Moves")), 120, siCentrado=True)
        oColumnas.nueva("PLAYER", _("Player"), 120)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)

        lyT = Colocacion.V().control(self.tablero).otro(lyBM).control(self.lbComentario)

        # Layout
        layout = Colocacion.H().otro(lyT).control(self.grid)

        self.setLayout(layout)

        self.grid.setFocus()

        self.grid.goto(self.posOP, 0)
        self.siMoviendoTiempo = False

        self.ponPuntos()

    def difPuntos(self):
        return self.rmUsu.puntosABS_5() - self.rmOP.puntosABS_5()

    def difPuntosMax(self):
        return self.mrm.mejorMov().puntosABS_5() - self.rmUsu.puntosABS_5()

    def ponPuntos(self):
        pts = self.difPuntos()
        if pts > 0:
            txt = _("Points won %d") % pts
            color = "green"
        elif pts < 0:
            txt = _("Lost points %d") % (-pts,)
            color = "red"
        else:
            txt = ""
            color = "black"
        self.lbComentario.ponTexto(txt)
        self.lbComentario.ponColorN(color)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "close":
            self.siMueveTiempo = False
            self.accept()
        elif accion == "MoverAdelante":
            self.mueve(nSaltar=1)
        elif accion == "MoverAtras":
            self.mueve(nSaltar=-1)
        elif accion == "MoverInicio":
            self.mueve(siBase=True)
        elif accion == "MoverFinal":
            self.mueve(siFinal=True)
        elif accion == "MoverTiempo":
            self.mueveTiempo()
        elif accion == "MoverMas":
            self.mueveMas()
        elif accion == "MoverLibre":
            self.mueveLibre()

    def gridNumDatos(self, grid):
        return len(self.listaRM)

    def hazListaRM(self):
        li = []
        posOP = 0
        nombrePlayer = _("You")
        posReal = 0
        ultPts = -99999999
        for pos, rm in enumerate(self.mrm.liMultiPV):
            pv1 = rm.pv.split(" ")[0]
            desde = pv1[:2]
            hasta = pv1[2:4]
            coronacion = pv1[4] if len(pv1) == 5 else None

            pgn = self.posicion.pgnSP(desde, hasta, coronacion)
            a = Util.Almacen()
            a.rm = rm
            a.texto = "%s (%s)" % (pgn, rm.abrTextoBase())
            p = a.puntosABS = rm.puntosABS()
            if p != ultPts:
                ultPts = p
                posReal += 1

            siOP = rm.pv == self.rmOP.pv
            siUsu = rm.pv == self.rmUsu.pv
            if siOP and siUsu:
                txt = _("Both")
                posOP = pos
            elif siOP:
                txt = self.nombreOP
                posOP = pos
            elif siUsu:
                txt = nombrePlayer
            else:
                txt = ""
            a.player = txt

            a.siElegido = siOP or siUsu
            if a.siElegido or not self.siCompetitivo:
                if siOP:
                    posOP = len(li)
                a.posReal = posReal
                li.append(a)

        return li, posOP

    def gridBold(self, grid, fila, columna):
        return self.listaRM[fila].siElegido

    def gridDato(self, grid, fila, oColumna):
        if oColumna.clave == "PLAYER":
            return self.listaRM[fila].player
        elif oColumna.clave == "POSREAL":
            return self.listaRM[fila].posReal
        else:
            return self.listaRM[fila].texto

    def gridColorTexto(self, grid, fila, oColumna):
        return None if self.listaRM[fila].puntosABS >= 0 else self.colorNegativo

    def gridColorFondo(self, grid, fila, oColumna):
        if fila % 2 == 1:
            return self.colorImpares
        else:
            return None

    def gridCambiadoRegistro(self, grid, fila, columna):
        self.partida = Partida.Partida(self.posicion)
        self.partida.leerPV(self.listaRM[fila].rm.pv)
        self.maxMoves = self.partida.numJugadas()
        self.mueve(siInicio=True)

        self.grid.setFocus()

    def mueve(self, siInicio=False, nSaltar=0, siFinal=False, siBase=False):
        if nSaltar:
            pos = self.posMueve + nSaltar
            if 0 <= pos < self.maxMoves:
                self.posMueve = pos
            else:
                return False
        elif siInicio:
            self.posMueve = 0
        elif siBase:
            self.posMueve = -1
        elif siFinal:
            self.posMueve = self.maxMoves - 1
        if len(self.partida):
            jg = self.partida.jugada(self.posMueve if self.posMueve > -1 else 0)
            if siBase:
                self.tablero.ponPosicion(jg.posicionBase)
            else:
                self.tablero.ponPosicion(jg.posicion)
                self.tablero.ponFlechaSC(jg.desde, jg.hasta)
        return True

    def mueveTiempo(self):
        if self.siMoviendoTiempo:
            self.siMoviendoTiempo = False
            return
        self.siMoviendoTiempo = True
        self.mueve(siBase=True)
        self.mueveTiempoWork()

    def mueveTiempoWork(self):
        if self.siMoviendoTiempo:
            if not self.mueve(nSaltar=1):
                self.siMoviendoTiempo = False
                return
            QtCore.QTimer.singleShot(1000, self.mueveTiempoWork)

    def mueveMas(self):
        mrm = self.gestor.analizaEstado()

        rmUsuN, pos = mrm.buscaRM(self.rmUsu.movimiento())
        if rmUsuN is None:
            um = QTUtil2.analizando(self)
            self.gestor.analizaFinal()
            rmUsuN = self.xmotor.valora(self.posicion, self.rmUsu.desde, self.rmUsu.hasta, self.rmUsu.coronacion)
            mrm.agregaRM(rmUsuN)
            self.gestor.analizaInicio()
            um.final()

        self.rmUsu = rmUsuN

        rmOPN, pos = mrm.buscaRM(self.rmOP.movimiento())
        if rmOPN is None:
            um = QTUtil2.analizando(self)
            self.gestor.analizaFinal()
            rmOPN = self.xmotor.valora(self.posicion, self.rmOP.desde, self.rmOP.hasta, self.rmOP.coronacion)
            pos = mrm.agregaRM(rmOPN)
            self.gestor.analizaInicio()
            um.final()

        self.rmOP = rmOPN
        self.analisis = self.mrm, pos
        self.siAnalisisCambiado = True

        self.mrm = mrm

        self.ponPuntos()
        self.listaRM, self.posOP = self.hazListaRM()
        self.grid.refresh()

    def mueveLibre(self):
        jg = self.partida.jugada(self.posMueve)
        pts = self.listaRM[self.grid.recno()].rm.texto()
        Analisis.AnalisisVariantes(self, self.xmotor, jg, self.posicion.siBlancas, pts)


class MensajeF(QtGui.QDialog):
    def __init__(self, parent, mens):
        QtGui.QDialog.__init__(self, parent)

        self.setWindowTitle(_("Result"))
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
        self.setWindowIcon(Iconos.Fide())
        self.setStyleSheet("QDialog, QLabel { background: #E9E9E9 }")

        lbm = Controles.LB(self, "<big><b>%s</b></big>" % mens)
        self.bt = Controles.PB(self, _("Continue"), rutina=self.accept, plano=False)

        ly = Colocacion.G().control(lbm, 0, 0).controlc(self.bt, 1, 0)

        ly.margen(20)

        self.setLayout(ly)

        w = parent.base.pgn
        self.move(w.x() + w.width() / 2 - self.width() / 2, w.y() + w.height() / 2 - self.height() / 2)

    def mostrar(self):
        QTUtil.refreshGUI()
        self.exec_()
        QTUtil.refreshGUI()
