import os.path
import time

from PyQt4 import QtCore

from Code import Analisis
from Code import BMT
from Code import ControlPGN
from Code import ControlPosicion
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
from Code.QT import Tablero
from Code import TrListas
from Code import Util
from Code.Constantes import *


class WHistorialBMT(QTVarios.WDialogo):
    def __init__(self, owner, dbf):

        # Variables
        self.procesador = owner.procesador
        self.configuracion = owner.configuracion

        # Datos ----------------------------------------------------------------
        self.dbf = dbf
        self.recnoActual = self.dbf.recno
        bmt_lista = Util.blob2var(dbf.leeOtroCampo(self.recnoActual, "BMT_LISTA"))
        self.liHistorial = Util.blob2var(dbf.leeOtroCampo(self.recnoActual, "HISTORIAL"))
        self.maxPuntos = dbf.MAXPUNTOS
        if bmt_lista.siTerminada():
            dic = {"FFINAL": dbf.FFINAL,
                   "ESTADO": dbf.ESTADO,
                   "PUNTOS": dbf.PUNTOS,
                   "SEGUNDOS": dbf.SEGUNDOS
                   }
            self.liHistorial.append(dic)

        # Dialogo ---------------------------------------------------------------
        icono = Iconos.Historial()
        titulo = _("Track record") + ": " + dbf.NOMBRE
        extparam = "bmthistorial"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        tb = QTVarios.LCTB(self)
        tb.new(_("Close"), Iconos.MainMenu(), self.terminar)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ESTADO", "", 26, edicion=Delegados.PmIconosBMT(), siCentrado=True)
        oColumnas.nueva("PUNTOS", _("Points"), 104, siCentrado=True)
        oColumnas.nueva("TIEMPO", _("Time"), 80, siCentrado=True)
        oColumnas.nueva("FFINAL", _("End date"), 90, siCentrado=True)

        self.grid = grid = Grid.Grid(self, oColumnas, xid=False, siEditable=True)
        # n = grid.anchoColumnas()
        # grid.setMinimumWidth( n + 20 )
        self.registrarGrid(grid)

        # Colocamos ---------------------------------------------------------------
        ly = Colocacion.V().control(tb).control(self.grid)

        self.setLayout(ly)

        self.recuperarVideo(siTam=True)

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def gridNumDatos(self, grid):
        return len(self.liHistorial)

    def gridDato(self, grid, fila, oColumna):
        dic = self.liHistorial[fila]
        col = oColumna.clave
        if col == "ESTADO":
            return dic["ESTADO"]

        elif col == "HECHOS":
            return "%d" % (dic["HECHOS"])

        elif col == "PUNTOS":
            p = dic["PUNTOS"]
            m = self.maxPuntos
            porc = p * 100 / m
            return "%d/%d=%d" % (p, m, porc) + "%"

        elif col == "FFINAL":
            f = dic["FFINAL"]
            return "%s-%s-%s" % (f[6:], f[4:6], f[:4]) if f else ""

        elif col == "TIEMPO":
            s = dic["SEGUNDOS"]
            if not s:
                s = 0
            m = s / 60
            s %= 60
            return "%d' %d\"" % (m, s) if m else "%d\"" % s


class WEntrenarBMT(QTVarios.WDialogo):
    def __init__(self, owner, dbf):

        # Variables
        self.procesador = owner.procesador
        self.configuracion = owner.configuracion

        self.iniTiempo = None
        self.antTxtSegundos = ""

        self.partida = Partida.Partida()
        self.siMostrarPGN = False

        self.posicion = ControlPosicion.ControlPosicion()
        self.actualP = 0  # Posicion actual

        self.controlPGN = ControlPGN.ControlPGN(self)

        self.estado = None  # compatibilidad con ControlPGN
        self.siJuegaHumano = False  # compatibilidad con ControlPGN

        # Datos ----------------------------------------------------------------
        self.dbf = dbf
        self.recnoActual = self.dbf.recno
        x = dbf.leeOtroCampo(self.recnoActual, "BMT_LISTA")
        self.bmt_lista = Util.blob2var(dbf.leeOtroCampo(self.recnoActual, "BMT_LISTA"))
        self.bmt_lista.compruebaColor()
        self.historial = Util.blob2var(dbf.leeOtroCampo(self.recnoActual, "HISTORIAL"))
        self.siTerminadaAntes = self.siTerminada = self.bmt_lista.siTerminada()
        self.timer = None
        self.datosInicio = self.bmt_lista.calculaTHPSE()

        # Dialogo ---------------------------------------------------------------
        icono = Iconos.BMT()
        titulo = dbf.NOMBRE
        extparam = "bmtentrenar"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Juegan ---------------------------------------------------------------
        self.lbJuegan = Controles.LB(self, "").ponColorFondoN("white", "black").alinCentrado()

        # Tablero ---------------------------------------------------------------
        confTablero = self.configuracion.confTablero("BMT", 32)

        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueveHumano)

        # Info -------------------------------------------------------------------
        colorFondo = QTUtil.qtColor(confTablero.colorNegras())
        self.trPuntos = "<big><b>" + _("Points") + "<br>%s</b></big>"
        self.trSegundos = "<big><b>" + _("Time") + "<br>%s</b></big>"
        self.lbPuntos = Controles.LB(self, "").ponFondo(colorFondo).alinCentrado().anchoMinimo(80)
        self.lbSegundos = Controles.LB(self, "").ponFondo(colorFondo).alinCentrado().anchoMinimo(80)
        self.lbPrimera = Controles.LB(self, _("* indicates actual move played in game"))
        f = Controles.TipoLetra(puntos=8)
        self.lbCondiciones = Controles.LB(self, "").ponFuente(f)

        # Grid-PGN ---------------------------------------------------------------
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 35, siCentrado=True)
        siFigurinesPGN = self.configuracion.figurinesPGN
        oColumnas.nueva("BLANCAS", _("White"), 100, edicion=Delegados.EtiquetaPGN(True if siFigurinesPGN else None))
        oColumnas.nueva("NEGRAS", _("Black"), 100, edicion=Delegados.EtiquetaPGN(False if siFigurinesPGN else None))
        self.pgn = Grid.Grid(self, oColumnas, siCabeceraMovible=False)
        nAnchoPgn = self.pgn.anchoColumnas() + 20
        self.pgn.setMinimumWidth(nAnchoPgn)

        self.pgn.setVisible(False)

        # BT posiciones ---------------------------------------------------------------
        self.liBT = []
        nSalto = (self.tablero.ancho + 34) / 26
        self.dicIconos = {0: Iconos.PuntoBlanco(),
                          1: Iconos.PuntoNegro(), 2: Iconos.PuntoAmarillo(),
                          3: Iconos.PuntoNaranja(), 4: Iconos.PuntoVerde(),
                          5: Iconos.PuntoAzul(), 6: Iconos.PuntoMagenta(),
                          7: Iconos.PuntoRojo(), 8: Iconos.PuntoEstrella()}
        nfila = 0
        ncolumna = 0
        lyBT = Colocacion.G()
        numero = 0
        nposic = len(self.bmt_lista)
        for x in range(nposic):
            bt = Controles.PB(self, str(x + 1), rutina=self.numero).anchoFijo(36).altoFijo(20)
            bt.numero = numero
            numero += 1
            estado = self.bmt_lista.estado(x)
            bt.ponIcono(self.dicIconos[estado])
            self.liBT.append(bt)

            lyBT.controlc(bt, nfila, ncolumna)
            nfila += 1
            if nfila == nSalto:
                ncolumna += 1
                nfila = 0
        # if ncolumna == 0:
        lyBT = Colocacion.V().otro(lyBT).relleno()

        gbBT = Controles.GB(self, _("Positions"), lyBT)

        # Lista de RM max 16 ---------------------------------------------------------------
        self.liBTrm = []
        nfila = 0
        ncolumna = 0
        lyRM = Colocacion.G()
        numero = 0
        for x in range(16):
            btRM = Controles.PB(self, "", rutina=self.pulsadoRM).anchoFijo(180).altoFijo(24).ponPlano(True)
            btRM.numero = numero
            btRM.setEnabled(False)
            numero += 1
            self.liBTrm.append(btRM)
            lyRM.controlc(btRM, nfila, ncolumna)
            ncolumna += 1
            if ncolumna == 2:
                nfila += 1
                ncolumna = 0

        self.gbRM = Controles.GB(self, _("Moves"), lyRM)

        # Tool bar ---------------------------------------------------------------
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), "terminar"),
            (_("Next"), Iconos.Siguiente(), "seguir"),
            (_("Repeat"), Iconos.Pelicula_Repetir(), "repetir"),
            (_("Resign"), Iconos.Abandonar(), "abandonar"),
            (_("Start"), Iconos.Empezar(), "empezar"),
            (_("Actual game"), Iconos.PartidaOriginal(), "original"),
        )
        self.tb = Controles.TB(self, liAcciones)

        self.recuperarVideo(siTam=False)

        # Colocamos ---------------------------------------------------------------
        lyPS = Colocacion.H().relleno().control(self.lbPuntos).relleno(2).control(self.lbSegundos).relleno()
        lyV = Colocacion.V().otro(lyPS).control(self.pgn).control(self.gbRM).control(self.lbPrimera)
        lyT = Colocacion.V().control(self.lbJuegan).control(self.tablero).control(self.lbCondiciones).relleno()
        lyTV = Colocacion.H().otro(lyT).otro(lyV).control(gbBT).margen(5)
        ly = Colocacion.V().control(self.tb).otro(lyTV).margen(2).relleno()

        self.setLayout(ly)

        if self.siTerminada:
            self.empezar()
        else:
            self.ponToolBar(["terminar", "empezar"])

        self.muestraControles(False)

    def muestraControles(self, si):
        for control in (
                self.lbJuegan, self.tablero, self.lbPuntos, self.lbSegundos, self.lbPrimera, self.lbCondiciones,
                self.pgn, self.gbRM):
            control.setVisible(si)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "terminar":
            self.terminar()
            self.accept()
        elif accion == "seguir":
            self.muestraControles(True)
            pos = self.actualP + 1
            if pos >= len(self.liBT):
                pos = 0
            self.buscaPrimero(pos)
        elif accion == "abandonar":
            self.bmt_uno.puntos = 0
            self.activaJugada(0)
            self.ponPuntos(0)
            self.ponToolBar()
        elif accion == "repetir":
            self.muestraControles(True)
            self.repetir()
        elif accion == "empezar":
            self.muestraControles(True)
            self.empezar()
        elif accion == "original":
            self.original()

    def closeEvent(self, event):
        self.terminar()

    def empezar(self):
        self.buscaPrimero(0)
        self.ponPuntos(0)
        self.ponSegundos()
        self.ponReloj()

    def terminar(self):
        self.finalizaTiempo()

        atotal, ahechos, at_puntos, at_segundos, at_estado = self.datosInicio

        total, hechos, t_puntos, t_segundos, t_estado = self.bmt_lista.calculaTHPSE()

        if (hechos != ahechos) or (t_puntos != at_puntos) or (t_segundos != at_segundos) or (t_estado != at_estado):

            reg = self.dbf.baseRegistro()
            reg.BMT_LISTA = Util.var2blob(self.bmt_lista)
            reg.HECHOS = hechos
            reg.SEGUNDOS = t_segundos
            reg.PUNTOS = t_puntos
            if self.historial:
                reg.HISTORIAL = Util.var2blob(self.historial)
                reg.REPE = len(reg.HISTORIAL)

            if self.siTerminada:
                if not self.siTerminadaAntes:
                    reg.ESTADO = str(t_estado / total)
                    reg.FFINAL = Util.dtos(Util.hoy())

            self.dbf.modificarReg(self.recnoActual, reg)

        self.guardarVideo()

    def repetir(self):
        if not QTUtil2.pregunta(self, _("Do you want to repeat this training?")):
            return

        self.quitaReloj()

        total, hechos, t_puntos, t_segundos, t_estado = self.bmt_lista.calculaTHPSE()

        dic = {}
        dic["FFINAL"] = self.dbf.FFINAL if self.siTerminadaAntes else Util.dtos(Util.hoy())
        dic["ESTADO"] = str(t_estado / total)
        dic["PUNTOS"] = t_puntos
        dic["SEGUNDOS"] = t_segundos

        self.historial.append(dic)

        self.bmt_lista.reiniciar()
        for bt in self.liBT:
            bt.ponIcono(self.dicIconos[0])

        self.siTerminadaAntes = self.siTerminada = False
        self.tablero.ponPosicion(ControlPosicion.ControlPosicion().logo())
        for bt in self.liBTrm:
            bt.ponTexto("")
        self.siMostrarPGN = False
        self.pgn.refresh()
        self.lbPuntos.ponTexto("")
        self.lbSegundos.ponTexto("")
        self.lbJuegan.ponTexto("")
        self.lbPrimera.setVisible(False)
        self.ponToolBar(["terminar", "empezar"])

    def ponRevision(self, siPoner):  # compatibilidad ControlPGN
        return

    def desactivaTodas(self):  # compatibilidad ControlPGN
        return

    def refresh(self):  # compatibilidad ControlPGN
        self.tablero.escena.update()
        self.update()
        QTUtil.refreshGUI()

    def ponPosicion(self, posicion):
        self.tablero.ponPosicion(posicion)

    def ponFlechaSC(self, desde, hasta, liVar=None):  # liVar incluido por compatibilidad
        self.tablero.ponFlechaSC(desde, hasta)

    def gridNumDatos(self, grid):
        if self.siMostrarPGN:
            return self.controlPGN.numDatos()
        else:
            return 0

    def ponteAlPrincipio(self):
        self.tablero.ponPosicion(self.partida.iniPosicion)
        self.pgn.goto(0, 0)
        self.pgn.refresh()

    def pgnMueveBase(self, fila, columna):
        if columna == "NUMERO":
            if fila == 0:
                self.ponteAlPrincipio()
                return
            else:
                fila -= 1
        self.controlPGN.mueve(fila, columna == "BLANCAS")

    def keyPressEvent(self, event):
        self.teclaPulsada("V", event.key())

    def tableroWheelEvent(self, nada, siAdelante):
        self.teclaPulsada("T", 16777234 if siAdelante else 16777236)

    def gridDato(self, grid, fila, oColumna):
        return self.controlPGN.dato(fila, oColumna.clave)

    def gridBotonIzquierdo(self, grid, fila, columna):
        self.pgnMueveBase(fila, columna.clave)

    def gridBotonDerecho(self, grid, fila, columna, modificadores):
        self.pgnMueveBase(fila, columna.clave)

    def gridDobleClick(self, grid, fila, columna):
        if columna.clave == "NUMERO":
            return
        self.analizaPosicion(fila, columna.clave)

    def gridTeclaControl(self, grid, k, siShift, siControl, siAlt):
        self.teclaPulsada("G", k)

    def gridWheelEvent(self, ogrid, siAdelante):
        self.teclaPulsada("T", 16777236 if not siAdelante else 16777234)

    def teclaPulsada(self, tipo, tecla):
        if self.siMostrarPGN:
            dic = QTUtil2.dicTeclas()
            if tecla in dic:
                self.mueveJugada(dic[tecla])

    def mueveJugada(self, tipo):
        partida = self.partida
        fila, columna = self.pgn.posActual()

        clave = columna.clave
        if clave == "NUMERO":
            siBlancas = tipo == kMoverAtras
            fila -= 1
        else:
            siBlancas = clave != "NEGRAS"

        siEmpiezaConNegras = partida.siEmpiezaConNegras

        lj = partida.numJugadas()
        if siEmpiezaConNegras:
            lj += 1
        ultFila = (lj - 1) / 2
        siUltBlancas = lj % 2 == 1

        if tipo == kMoverAtras:
            if siBlancas:
                fila -= 1
            siBlancas = not siBlancas
            pos = fila * 2
            if not siBlancas:
                pos += 1
            if fila < 0 or (fila == 0 and pos == 0 and siEmpiezaConNegras):
                self.ponteAlPrincipio()
                return
        elif tipo == kMoverAdelante:
            if not siBlancas:
                fila += 1
            siBlancas = not siBlancas
        elif tipo == kMoverInicio:  # Inicio
            self.ponteAlPrincipio()
            return
        elif tipo == kMoverFinal:
            fila = ultFila
            siBlancas = not partida.ultPosicion.siBlancas

        if fila == ultFila:
            if siUltBlancas and not siBlancas:
                return

        if fila < 0 or fila > ultFila:
            self.refresh()
            return
        if fila == 0 and siBlancas and siEmpiezaConNegras:
            siBlancas = False

        self.pgnColocate(fila, siBlancas)
        self.pgnMueveBase(fila, "BLANCAS" if siBlancas else "NEGRAS")

    def pgnColocate(self, fil, siBlancas):
        col = 1 if siBlancas else 2
        self.pgn.goto(fil, col)

    def numero(self):
        bt = self.sender()
        self.activaPosicion(bt.numero)
        return 0

    def pulsadoRM(self):
        if self.siMostrarPGN:
            bt = self.sender()
            self.muestra(bt.numero)

    def ponToolBar(self, li=None):
        if not li:
            li = ["terminar", "seguir"]

            if not self.bmt_uno.siTerminado:
                li.append("abandonar")
            else:
                if self.siTerminada:
                    li.append("repetir")
                if self.bmt_uno.clpartida:
                    li.append("original")
        self.tb.clear()
        for k in li:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])

        self.tb.liAcciones = li
        self.tb.update()

    def ponPuntos(self, descontar):
        self.bmt_uno.puntos -= descontar
        if self.bmt_uno.puntos < 0:
            self.bmt_uno.puntos = 0
        self.bmt_uno.actualizaEstado()

        eti = "%d/%d" % (self.bmt_uno.puntos, self.bmt_uno.maxPuntos)
        self.lbPuntos.ponTexto(self.trPuntos % eti)

    def ponSegundos(self):
        segundos = self.bmt_uno.segundos
        if self.iniTiempo:
            segundos += int(time.time() - self.iniTiempo)
        minutos = segundos / 60
        segundos -= minutos * 60

        if minutos:
            eti = "%d'%d\"" % (minutos, segundos)
        else:
            eti = "%d\"" % (segundos,)
        eti = self.trSegundos % eti

        if eti != self.antTxtSegundos:
            self.antTxtSegundos = eti
            self.lbSegundos.ponTexto(eti)

    def buscaPrimero(self, desde):
        # Buscamos el primero que no se ha terminado
        n = len(self.bmt_lista)
        for x in range(n):
            t = desde + x
            if t >= n:
                t = 0
            if not self.bmt_lista.siTerminado(t):
                self.activaPosicion(t)
                return

        self.activaPosicion(desde)

    def activaJugada1(self, num):
        rm = self.bmt_uno.mrm.liMultiPV[num]
        partida = Partida.Partida()
        partida.recuperaDeTexto(rm.txtPartida)

        bt = self.liBTrm[num]
        txt = "%d: %s = %s" % (rm.nivelBMT + 1, partida.jugada(0).pgnSP(), rm.abrTexto())
        if rm.siPrimero:
            txt = "%s *" % txt
            self.lbPrimera.setVisible(True)

        bt.ponTexto(txt)
        bt.setEnabled(True)
        bt.ponPlano(False)

    def activaJugada(self, num):
        rm = self.bmt_uno.mrm.liMultiPV[num]
        if rm.nivelBMT == 0:
            self.finalizaTiempo()
            for n in range(len(self.bmt_uno.mrm.liMultiPV)):
                self.activaJugada1(n)
            self.bmt_uno.siTerminado = True
            self.muestra(num)
            self.ponPuntos(0)
            bt = self.liBT[self.actualP]
            bt.ponIcono(self.dicIconos[self.bmt_uno.estado])

            self.siTerminada = self.bmt_lista.siTerminada()

            self.ponToolBar()

        else:
            self.activaJugada1(num)

    def activaPosicion(self, num):

        self.finalizaTiempo()  # Para que guarde el tiempo, si no es el primero

        self.bmt_uno = bmt_uno = self.bmt_lista.dameUno(num)
        mrm = bmt_uno.mrm
        tm = mrm.maxTiempo
        dp = mrm.maxProfundidad
        if tm > 0:
            txt = " %d %s" % (tm / 1000, _("Second(s)"))
        elif dp > 0:
            txt = " %s %d" % (_("depth"), dp)
        else:
            txt = ""

        self.posicion.leeFen(bmt_uno.fen)

        mens = ""
        if self.posicion.enroques:
            color, colorR = _("White"), _("Black")
            cK, cQ, cKR, cQR = "K", "Q", "k", "q"

            def menr(ck, cq):
                enr = ""
                if ck in self.posicion.enroques:
                    enr += "O-O"
                if cq in self.posicion.enroques:
                    if enr:
                        enr += "  +  "
                    enr += "O-O-O"
                return enr

            enr = menr(cK, cQ)
            if enr:
                mens += "  %s : %s" % (color, enr)
            enr = menr(cKR, cQR)
            if enr:
                mens += " %s : %s" % (colorR, enr)
        if self.posicion.alPaso != "-":
            mens += "     %s : %s" % (_("En passant"), self.posicion.alPaso)

        if mens:
            txt += "  - " + mens

        self.lbCondiciones.ponTexto(mrm.nombre + txt )

        self.tablero.ponPosicion(self.posicion)

        self.liBT[self.actualP].ponPlano(True)
        self.liBT[num].ponPlano(False)
        self.actualP = num

        nliRM = len(mrm.liMultiPV)
        partida = Partida.Partida()
        for x in range(16):
            bt = self.liBTrm[x]
            if x < nliRM:
                rm = mrm.liMultiPV[x]
                bt.setVisible(True)
                bt.ponPlano(not rm.siElegida)
                baseTxt = str(rm.nivelBMT + 1)
                if rm.siElegida:
                    partida.reset(self.posicion)
                    partida.leerPV(rm.pv)
                    baseTxt += " - " + partida.jugada(0).pgnSP()
                bt.ponTexto(baseTxt)
            else:
                bt.setVisible(False)

        self.ponPuntos(0)
        self.ponSegundos()

        self.ponToolBar()
        if bmt_uno.siTerminado:
            self.activaJugada(0)
            self.muestra(0)
        else:
            self.lbPrimera.setVisible(False)
            self.iniciaTiempo()
            self.sigueHumano()

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.paraHumano()

        movimiento = desde + hasta

        # Peon coronando
        if not coronacion and self.posicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.posicion.siBlancas)
            if coronacion is None:
                self.sigueHumano()
                return False
        if coronacion:
            movimiento += coronacion

        nElegido = None
        puntosDescontar = self.bmt_uno.mrm.liMultiPV[-1].nivelBMT
        for n, rm in enumerate(self.bmt_uno.mrm.liMultiPV):
            if rm.pv.lower().startswith(movimiento.lower()):
                nElegido = n
                puntosDescontar = rm.nivelBMT
                break

        self.ponPuntos(puntosDescontar)

        if nElegido is not None:
            self.activaJugada(nElegido)

        if not self.bmt_uno.siTerminado:
            self.sigueHumano()
        return True

    def paraHumano(self):
        self.tablero.desactivaTodas()

    def sigueHumano(self):
        self.siMostrarPGN = False
        self.pgn.refresh()
        siW = self.posicion.siBlancas
        self.tablero.ponPosicion(self.posicion)
        self.tablero.ponerPiezasAbajo(siW)
        self.tablero.ponIndicador(siW)
        self.tablero.activaColor(siW)
        self.lbJuegan.ponTexto(_("White to play") if siW else _("Black to play"))

    def ponReloj(self):
        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.enlaceReloj)
        self.timer.start(500)

    def quitaReloj(self):
        if self.timer:
            self.timer.stop()
            self.timer = None

    def enlaceReloj(self):
        self.ponSegundos()

    def original(self):
        self.siMostrarPGN = True
        self.lbJuegan.ponTexto(_("Actual game"))
        txtPartida = self.bmt_lista.dicPartidas[self.bmt_uno.clpartida]
        self.partida.recuperaDeTexto(txtPartida)

        siW = self.posicion.siBlancas
        fen = self.posicion.fen()
        fila = 0
        for jg in self.partida.liJugadas:
            if jg.posicionBase.fen() == fen:
                break
            if not jg.posicionBase.siBlancas:
                fila += 1
        self.pgnMueveBase(fila, "BLANCAS" if siW else "NEGRAS")
        self.pgn.goto(fila, 1 if siW else 2)

        self.tablero.ponerPiezasAbajo(siW)

        self.pgn.refresh()

    def muestra(self, num):

        for n, bt in enumerate(self.liBTrm):
            f = bt.font()
            siBold = f.bold()
            if (num == n and not siBold) or (num != n and siBold):
                f.setBold(not siBold)
                bt.setFont(f)
            bt.setAutoDefault(num == n)
            bt.setDefault(num == n)

        self.siMostrarPGN = True
        self.lbJuegan.ponTexto(self.liBTrm[num].text())
        self.partida.reset(self.posicion)
        rm = self.bmt_uno.mrm.liMultiPV[num]
        self.partida.recuperaDeTexto(rm.txtPartida)

        siW = self.posicion.siBlancas
        self.pgnMueveBase(0, "BLANCAS" if siW else "NEGRAS")
        self.pgn.goto(0, 1 if siW else 2)

        self.tablero.ponerPiezasAbajo(siW)

        self.pgn.refresh()

    def iniciaTiempo(self):
        self.iniTiempo = time.time()
        if not self.timer:
            self.ponReloj()

    def finalizaTiempo(self):
        if self.iniTiempo:
            tiempo = time.time() - self.iniTiempo
            self.bmt_uno.segundos += int(tiempo)
        self.iniTiempo = None
        self.quitaReloj()

    def dameJugadaEn(self, fila, clave):
        siBlancas = clave != "NEGRAS"

        pos = fila * 2
        if not siBlancas:
            pos += 1
        if self.partida.siEmpiezaConNegras:
            pos -= 1
        tam_lj = self.partida.numJugadas()
        if tam_lj == 0:
            return
        siUltimo = (pos + 1) >= tam_lj

        jg = self.partida.jugada(pos)
        return jg, siBlancas, siUltimo, tam_lj, pos

    def analizaPosicion(self, fila, clave):

        if fila < 0:
            return

        jg, siBlancas, siUltimo, tam_lj, pos = self.dameJugadaEn(fila, clave)
        if jg.siJaqueMate:
            return

        maxRecursion = 9999
        Analisis.muestraAnalisis(self.procesador, self.procesador.XTutor(), jg, siBlancas, maxRecursion, pos,
                                 pantalla=self)


class WBMT(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.configuracion.compruebaBMT()

        self.bmt = BMT.BMT(self.configuracion.ficheroBMT)
        self.leerDBF()

        owner = procesador.pantalla
        icono = Iconos.BMT()
        titulo = self.titulo()
        extparam = "bmt"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        liAcciones = [(_("Close"), Iconos.MainMenu(), self.terminar), None,
                      (_("Play"), Iconos.Empezar(), self.entrenar), None,
                      (_("New"), Iconos.Nuevo(), self.nuevo), None,
                      (_("Modify"), Iconos.Modificar(), self.modificar), None,
                      (_("Remove"), Iconos.Borrar(), self.borrar), None,
                      (_("Track record"), Iconos.Historial(), self.historial), None,
                      (_("Utilities"), Iconos.Utilidades(), self.utilidades),
                      ]
        tb = QTVarios.LCTB(self, liAcciones)

        self.tab = tab = Controles.Tab()

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NOMBRE", _("Name"), 274, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("EXTRA", _("Extra info."), 64, siCentrado=True)
        oColumnas.nueva("HECHOS", _("Made"), 84, siCentrado=True)
        oColumnas.nueva("PUNTOS", _("Points"), 84, siCentrado=True)
        oColumnas.nueva("TIEMPO", _("Time"), 80, siCentrado=True)
        oColumnas.nueva("REPETICIONES", _("Rep."), 50, siCentrado=True)
        oColumnas.nueva("ORDEN", _("Order"), 70, siCentrado=True)

        self.grid = grid = Grid.Grid(self, oColumnas, xid="P", siEditable=False, siSelecFilas=True,
                                     siSeleccionMultiple=True)
        self.registrarGrid(grid)
        tab.nuevaTab(grid, _("Pending"))

        # Terminados
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ESTADO", "", 26, edicion=Delegados.PmIconosBMT(), siCentrado=True)
        oColumnas.nueva("NOMBRE", _("Name"), 240)
        oColumnas.nueva("EXTRA", _("Extra info."), 64, siCentrado=True)
        oColumnas.nueva("HECHOS", _("Positions"), 64, siCentrado=True)
        oColumnas.nueva("PUNTOS", _("Points"), 84, siCentrado=True)
        oColumnas.nueva("FFINAL", _("End date"), 90, siCentrado=True)
        oColumnas.nueva("TIEMPO", _("Time"), 80, siCentrado=True)
        oColumnas.nueva("REPETICIONES", _("Rep."), 50, siCentrado=True)
        oColumnas.nueva("ORDEN", _("Order"), 70, siCentrado=True)

        self.gridT = gridT = Grid.Grid(self, oColumnas, xid="T", siEditable=True, siSelecFilas=True,
                                       siSeleccionMultiple=True)
        self.registrarGrid(gridT)
        tab.nuevaTab(gridT, _("Finished"))

        self.dicReverse = {}

        # Layout
        layout = Colocacion.V().control(tb).control(tab).margen(8)
        self.setLayout(layout)

        self.recuperarVideo(siTam=True, anchoDefecto=760)

        self.grid.gotop()
        self.gridT.gotop()

        self.grid.setFocus()

    def titulo(self):
        fdir, fnam = os.path.split(self.configuracion.ficheroBMT)
        return "%s : %s (%s)" % (_("Find best move"), fnam, fdir)

    def terminar(self):
        self.bmt.cerrar()
        self.guardarVideo()
        self.reject()
        return

    def actual(self):
        if self.tab.posActual() == 0:
            grid = self.grid
            dbf = self.dbf
        else:
            grid = self.gridT
            dbf = self.dbfT
        recno = grid.recno()
        if recno >= 0:
            dbf.goto(recno)

        return grid, dbf, recno

    def historial(self):
        grid, dbf, recno = self.actual()
        if recno >= 0:
            if dbf.REPE:
                w = WHistorialBMT(self, dbf)
                w.exec_()

    def utilidades(self):
        menu = QTVarios.LCMenu(self)

        menu.opcion("cambiar", _("Select/create another file of training"), Iconos.BMT())

        menu.separador()
        menu1 = menu.submenu(_("Import") + "/" + _("Export"), Iconos.PuntoMagenta())
        menu1.opcion("exportar", _("Export the current training"), Iconos.PuntoVerde())
        menu1.separador()
        menu1.opcion("exportarLimpio", _("Export Current training with no history"), Iconos.PuntoAzul())
        menu1.separador()
        menu1.opcion("importar", _("Import a training"), Iconos.PuntoNaranja())

        menu.separador()
        menu2 = menu.submenu(_("Generate new trainings"), Iconos.PuntoRojo())
        menu2.opcion("dividir", _("Dividing the active training"), Iconos.PuntoVerde())
        menu2.separador()
        menu2.opcion("extraer", _("Extract a range of positions"), Iconos.PuntoAzul())
        menu2.separador()
        menu2.opcion("juntar", _("Joining selected training"), Iconos.PuntoNaranja())
        menu2.separador()
        menu2.opcion("rehacer", _("Analyze again"), Iconos.PuntoAmarillo())

        resp = menu.lanza()
        if resp:
            if resp == "cambiar":
                self.cambiar()
            elif resp == "importar":
                self.importar()
            elif resp.startswith("exportar"):
                self.exportar(resp == "exportarLimpio")
            elif resp == "dividir":
                self.dividir()
            elif resp == "extraer":
                self.extraer()
            elif resp == "juntar":
                self.juntar()
            elif resp == "pack":
                self.pack()
            elif resp == "rehacer":
                self.rehacer()

    def pack(self):
        um = QTUtil2.unMomento(self)
        self.dbf.pack()
        self.releer()
        um.final()

    def rehacer(self):
        grid, dbf, recno = self.actual()
        if recno < 0:
            return
        nombre = dbf.NOMBRE
        extra = dbf.EXTRA
        bmt_lista = Util.blob2var(dbf.leeOtroCampo(recno, "BMT_LISTA"))

        # Motor y tiempo, cogemos los estandars de analisis
        fichero = self.configuracion.ficheroAnalisis
        dic = Util.recuperaVar(fichero)
        if dic:
            motor = dic["MOTOR"]
            tiempo = dic["TIEMPO"]
        else:
            motor = self.configuracion.tutor.clave
            tiempo = self.configuracion.tiempoTutor

        # Bucle para control de errores
        liGen = [(None, None)]

        # # Nombre del entrenamiento
        liGen.append((_("Name") + ":", nombre))
        liGen.append((_("Extra info.") + ":", extra))

        # # Tutor
        li = self.configuracion.ayudaCambioTutor()
        li[0] = motor
        liGen.append((_("Engine") + ":", li))

        # Decimas de segundo a pensar el tutor
        liGen.append((_("Duration of engine analysis (secs)") + ":", tiempo / 1000.0))

        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=nombre, parent=self, anchoMinimo=560, icon=Iconos.Opciones())
        if not resultado:
            return
        accion, liGen = resultado

        nombre = liGen[0]
        extra = liGen[1]
        motor = liGen[2]
        tiempo = int(liGen[3] * 1000)

        if not tiempo or not nombre:
            return

        dic = {"MOTOR": motor, "TIEMPO": tiempo}
        Util.guardaVar(fichero, dic)

        # Analizamos todos, creamos las partidas, y lo salvamos
        confMotor = self.configuracion.buscaMotor(motor)
        confMotor.multiPV = 16
        xgestor = self.procesador.creaGestorMotor(confMotor, tiempo, None, True)

        tamLista = len(bmt_lista.liBMT_Uno)

        mensaje = _("Analyzing the move....")
        tmpBP = QTUtil2.BarraProgreso(self.procesador.pantalla, nombre, mensaje, tamLista).mostrar()

        cp = ControlPosicion.ControlPosicion()
        siCancelado = False

        partida = Partida.Partida()

        for pos in range(tamLista):

            uno = bmt_lista.dameUno(pos)

            fen = uno.fen
            ant_movimiento = ""
            for rm in uno.mrm.liMultiPV:
                if rm.siPrimero:
                    ant_movimiento = rm.movimiento()
                    break

            tmpBP.mensaje(mensaje + " %d/%d" % (pos, tamLista))
            tmpBP.pon(pos)
            if tmpBP.siCancelado():
                siCancelado = True
                break

            mrm = xgestor.analiza(fen)

            cp.leeFen(fen)

            previa = 999999999
            nprevia = -1
            tniv = 0

            for rm in mrm.liMultiPV:
                if tmpBP.siCancelado():
                    siCancelado = True
                    break
                pts = rm.puntosABS()
                if pts != previa:
                    previa = pts
                    nprevia += 1
                tniv += nprevia
                rm.nivelBMT = nprevia
                rm.siElegida = False
                rm.siPrimero = rm.movimiento() == ant_movimiento
                partida.reset(cp)
                partida.leerPV(rm.pv)
                rm.txtPartida = partida.guardaEnTexto()

            if siCancelado:
                break

            uno.mrm = mrm  # lo cambiamos y ya esta

        xgestor.terminar()

        if not siCancelado:
            # Grabamos

            bmt_lista.reiniciar()

            reg = self.dbf.baseRegistro()
            reg.ESTADO = "0"
            reg.NOMBRE = nombre
            reg.EXTRA = extra
            reg.TOTAL = len(bmt_lista)
            reg.HECHOS = 0
            reg.PUNTOS = 0
            reg.MAXPUNTOS = bmt_lista.maxPuntos()
            reg.FINICIAL = Util.dtos(Util.hoy())
            reg.FFINAL = ""
            reg.SEGUNDOS = 0
            reg.BMT_LISTA = Util.var2blob(bmt_lista)
            reg.HISTORIAL = Util.var2blob([])
            reg.REPE = 0

            reg.ORDEN = 0

            self.dbf.insertarReg(reg, siReleer=True)

        tmpBP.cerrar()
        self.grid.refresh()

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave
        if clave != "NOMBRE":
            return

        grid, dbf, recno = self.actual()

        li = []
        for x in range(dbf.reccount()):
            dbf.goto(x)
            li.append((dbf.NOMBRE, x))

        li.sort(key=lambda x: x[0])

        siReverse = self.dicReverse.get(grid.id, False)
        self.dicReverse[grid.id] = not siReverse

        if siReverse:
            li.reverse()

        order = 0
        reg = dbf.baseRegistro()
        for nom, recno in li:
            reg.ORDEN = order
            dbf.modificarReg(recno, reg)
            order += 1
        dbf.commit()
        dbf.leer()
        grid.refresh()
        grid.gotop()

    def dividir(self):
        grid, dbf, recno = self.actual()
        if recno < 0:
            return
        reg = dbf.registroActual()  # Importante ya que dbf puede cambiarse mientras se edita

        liGen = [(None, None)]

        mx = dbf.TOTAL
        if mx <= 1:
            return
        bl = mx / 2

        liGen.append((FormLayout.Spinbox(_("Block Size"), 1, mx - 1, 50), bl))

        resultado = FormLayout.fedit(liGen, title="%s %s" % (reg.NOMBRE, reg.EXTRA), parent=self,
                                     icon=Iconos.Opciones())

        if resultado:
            accion, liGen = resultado
            bl = liGen[0]

            um = QTUtil2.unMomento(self)
            bmt_lista = Util.blob2var(dbf.leeOtroCampo(recno, "BMT_LISTA"))

            desde = 0
            pos = 1
            extra = reg.EXTRA
            while desde < mx:
                hasta = desde + bl
                if hasta >= mx:
                    hasta = mx
                bmt_listaNV = bmt_lista.extrae(desde, hasta)
                reg.TOTAL = hasta - desde
                reg.BMT_LISTA = Util.var2blob(bmt_listaNV)
                reg.HISTORIAL = Util.var2blob([])
                reg.REPE = 0
                reg.ESTADO = "0"
                reg.EXTRA = (extra + " (%d)" % pos).strip()
                pos += 1
                reg.HECHOS = 0
                reg.PUNTOS = 0
                reg.MAXPUNTOS = bmt_listaNV.maxPuntos()
                reg.FFINAL = ""
                reg.SEGUNDOS = 0

                dbf.insertarReg(reg, siReleer=False)

                desde = hasta

            self.releer()
            um.final()

    def extraer(self):
        grid, dbf, recno = self.actual()
        if recno < 0:
            return
        reg = dbf.registroActual()  # Importante ya que dbf puede cambiarse mientras se edita
        liGen = [(None, None)]
        config = FormLayout.Editbox("<div align=\"right\">" + _("List of positions") + "<br>" +
                                    _("By example:") + " -5,7-9,14,19-",
                                    rx="[0-9,\-,\,]*")
        liGen.append((config, ""))

        resultado = FormLayout.fedit(liGen, title=reg.NOMBRE, parent=self, anchoMinimo=200, icon=Iconos.Opciones())

        if resultado:
            accion, liGen = resultado

            bmt_lista = Util.blob2var(dbf.leeOtroCampo(recno, "BMT_LISTA"))
            clista = liGen[0]
            if clista:
                lni = Util.ListaNumerosImpresion(clista)
                bmt_listaNV = bmt_lista.extraeLista(lni)

                reg.TOTAL = len(bmt_listaNV)
                reg.BMT_LISTA = Util.var2blob(bmt_listaNV)
                reg.HISTORIAL = Util.var2blob([])
                reg.REPE = 0
                reg.ESTADO = "0"
                reg.EXTRA = clista
                reg.HECHOS = 0
                reg.PUNTOS = 0
                reg.MAXPUNTOS = bmt_listaNV.maxPuntos()
                reg.FFINAL = ""
                reg.SEGUNDOS = 0

                um = QTUtil2.unMomento(self)
                dbf.insertarReg(reg, siReleer=False)

                self.releer()
                um.final()

    def juntar(self):
        grid, dbf, recno = self.actual()
        orden = dbf.ORDEN
        nombre = dbf.NOMBRE
        extra = dbf.EXTRA

        # Lista de recnos
        li = grid.recnosSeleccionados()

        if len(li) <= 1:
            return

        # Se pide nombre y extra
        liGen = [(None, None)]

        # # Nombre del entrenamiento
        liGen.append((_("Name") + ":", nombre))

        liGen.append((_("Extra info.") + ":", extra))

        liGen.append((FormLayout.Editbox(_("Order"), tipo=int, ancho=50), orden))

        titulo = "%s (%d)" % (_("Joining selected training"), len(li))
        resultado = FormLayout.fedit(liGen, title=titulo, parent=self, anchoMinimo=560, icon=Iconos.Opciones())
        if not resultado:
            return

        um = QTUtil2.unMomento(self)

        accion, liGen = resultado
        nombre = liGen[0].strip()
        extra = liGen[1]
        orden = liGen[2]

        # Se crea una bmt_lista, suma de todas
        bmt_lista = BMT.BMT_Lista()

        for recno in li:
            bmt_lista1 = Util.blob2var(dbf.leeOtroCampo(recno, "BMT_LISTA"))
            for uno in bmt_lista1.liBMT_Uno:
                bmt_lista.nuevo(uno)
                if uno.clpartida:
                    bmt_lista.compruebaPartida(uno.clpartida, bmt_lista1.dicPartidas[uno.clpartida])

        bmt_lista.reiniciar()

        # Se graba el registro
        reg = dbf.baseRegistro()
        reg.ESTADO = "0"
        reg.NOMBRE = nombre
        reg.EXTRA = extra
        reg.TOTAL = len(bmt_lista)
        reg.HECHOS = 0
        reg.PUNTOS = 0
        reg.MAXPUNTOS = bmt_lista.maxPuntos()
        reg.FINICIAL = Util.dtos(Util.hoy())
        reg.FFINAL = ""
        reg.SEGUNDOS = 0
        reg.BMT_LISTA = Util.var2blob(bmt_lista)
        reg.HISTORIAL = Util.var2blob([])
        reg.REPE = 0

        reg.ORDEN = orden

        dbf.insertarReg(reg, siReleer=False)

        self.releer()

        um.final()

    def cambiar(self):
        fbmt = QTUtil2.salvaFichero(self, _("Select/create another file of training"), self.configuracion.ficheroBMT,
                                    _("File") + " bmt (*.bmt)", siConfirmarSobreescritura=False)
        if fbmt:
            fbmt = Util.dirRelativo(fbmt)
            abmt = self.bmt
            try:
                self.bmt = BMT.BMT(fbmt)
            except:
                QTUtil2.mensError(self, _X(_("Unable to read file %1"), fbmt))
                return
            abmt.cerrar()
            self.leerDBF()
            self.configuracion.ficheroBMT = fbmt
            self.configuracion.graba()
            self.setWindowTitle(self.titulo())
            self.grid.refresh()
            self.gridT.refresh()

    def exportar(self, siLimpiar):
        grid, dbf, recno = self.actual()

        if recno >= 0:
            regActual = dbf.registroActual()
            carpeta = os.path.dirname(self.configuracion.ficheroBMT)
            filtro = _("File") + " bm1 (*.bm1)"
            fbm1 = QTUtil2.salvaFichero(self, _("Export the current training"), carpeta, filtro,
                                        siConfirmarSobreescritura=True)
            if fbm1:
                if siLimpiar:
                    regActual.ESTADO = "0"
                    regActual.HECHOS = 0
                    regActual.PUNTOS = 0
                    regActual.FFINAL = ""
                    regActual.SEGUNDOS = 0
                    bmt_lista = Util.blob2var(dbf.leeOtroCampo(recno, "BMT_LISTA"))
                    bmt_lista.reiniciar()
                    regActual.BMT_LISTA = bmt_lista
                    regActual.HISTORIAL = []
                    regActual.REPE = 0
                else:
                    regActual.BMT_LISTA = Util.blob2var(dbf.leeOtroCampo(recno, "BMT_LISTA"))
                    regActual.HISTORIAL = Util.blob2var(dbf.leeOtroCampo(recno, "HISTORIAL"))

                Util.guardaVar(fbm1, regActual)

    def modificar(self):
        grid, dbf, recno = self.actual()

        if recno >= 0:
            dbf.goto(recno)

            nombre = dbf.NOMBRE
            extra = dbf.EXTRA
            orden = dbf.ORDEN

            liGen = [(None, None)]

            # # Nombre del entrenamiento
            liGen.append((_("Name") + ":", nombre))

            liGen.append((_("Extra info.") + ":", extra))

            liGen.append((FormLayout.Editbox(_("Order"), tipo=int, ancho=50), orden))

            resultado = FormLayout.fedit(liGen, title=nombre, parent=self, anchoMinimo=560, icon=Iconos.Opciones())

            if resultado:
                accion, liGen = resultado
                liCamposValor = (("NOMBRE", liGen[0].strip()), ("EXTRA", liGen[1]), ("ORDEN", liGen[2]))
                self.grabaCampos(grid, recno, liCamposValor)

    def releer(self):
        self.dbf.leer()
        self.dbfT.leer()
        self.grid.refresh()
        self.gridT.refresh()
        QTUtil.refreshGUI()

    def importar(self):
        carpeta = os.path.dirname(self.configuracion.ficheroBMT)
        filtro = _("File") + " bm1 (*.bm1)"
        fbm1 = QTUtil2.leeFichero(self, carpeta, filtro, titulo=_("Import a training"))
        if fbm1:

            reg = Util.recuperaVar(fbm1)
            if hasattr(reg, "BMT_LISTA"):
                reg.BMT_LISTA = Util.var2blob(reg.BMT_LISTA)
                reg.HISTORIAL = Util.var2blob(reg.HISTORIAL)
                self.dbf.insertarReg(reg, siReleer=False)
                self.releer()
            else:
                QTUtil2.mensError(self, _X(_("Unable to read file %1"), fbm1))

    def entrenar(self):
        grid, dbf, recno = self.actual()
        if recno >= 0:
            w = WEntrenarBMT(self, dbf)
            w.exec_()
            self.releer()

    def borrar(self):
        grid, dbf, recno = self.actual()
        li = grid.recnosSeleccionados()
        if len(li) > 0:
            tit = "<br><ul>"
            for x in li:
                dbf.goto(x)
                tit += "<li>%s %s</li>" % (dbf.NOMBRE, dbf.EXTRA)
            base = _("the following training")
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), base) + tit):
                um = QTUtil2.unMomento(self)
                dbf.borrarLista(li)
                dbf.pack()
                self.releer()
                um.final()

    def grabaCampos(self, grid, fila, liCamposValor):
        dbf = self.dbfT if grid.id == "T" else self.dbf
        reg = dbf.baseRegistro()
        for campo, valor in liCamposValor:
            setattr(reg, campo, valor)
        dbf.modificarReg(fila, reg)
        dbf.commit()
        dbf.leer()
        grid.refresh()

    def gridPonValor(self, grid, fila, oColumna, valor):  # ? necesario al haber delegados
        pass

    def gridNumDatos(self, grid):
        dbf = self.dbfT if grid.id == "T" else self.dbf
        return dbf.reccount()

    def gridDobleClick(self, grid, fila, columna):
        self.entrenar()

    def gridDato(self, grid, fila, oColumna):
        dbf = self.dbfT if grid.id == "T" else self.dbf
        col = oColumna.clave

        dbf.goto(fila)

        if col == "NOMBRE":
            return dbf.NOMBRE

        elif col == "ORDEN":
            return dbf.ORDEN if dbf.ORDEN else 0

        elif col == "ESTADO":
            return dbf.ESTADO

        elif col == "HECHOS":
            if grid.id == "T":
                return "%d" % dbf.TOTAL
            else:
                return "%d/%d" % (dbf.HECHOS, dbf.TOTAL)

        elif col == "PUNTOS":
            p = dbf.PUNTOS
            m = dbf.MAXPUNTOS
            if grid.id == "T":
                porc = p * 100 / m
                return "%d/%d=%d" % (p, m, porc) + "%"
            else:
                return "%d/%d" % (p, m)

        elif col == "EXTRA":
            return dbf.EXTRA

        elif col == "FFINAL":
            f = dbf.FFINAL
            return "%s-%s-%s" % (f[6:], f[4:6], f[:4]) if f else ""

        elif col == "TIEMPO":
            s = dbf.SEGUNDOS
            if not s:
                s = 0
            m = s / 60
            s %= 60
            return "%d' %d\"" % (m, s) if m else "%d\"" % s

        elif col == "REPETICIONES":
            return str(dbf.REPE)

    def leerDBF(self):
        self.dbf = self.bmt.leerDBF(False)
        self.dbfT = self.bmt.leerDBF(True)

    def nuevo(self):

        tpirat = Controles.TipoLetra("Chess Diagramm Pirat", self.configuracion.puntosMenu+4)

        def xopcion(menu, clave, texto, icono, siDeshabilitado=False):
            if "KP" in texto:
                d = {"K":"r", "P":"w", "k":chr(126), "p":chr(134)}
                k2 = texto.index("K", 2)
                texto = texto[:k2] + texto[k2:].lower()
                texton = ""
                for c in texto:
                    texton += d[c]
                menu.opcion(clave, texton, icono, siDeshabilitado, tipoLetra=tpirat)
            else:
                menu.opcion(clave, texto, icono, siDeshabilitado)

        # Elegimos el entrenamiento
        menu = QTVarios.LCMenu(self)
        self.procesador.entrenamientos.menuFNS(menu, _("Select the training positions you want to use as a base"),
                                               xopcion)
        resp = menu.lanza()
        if resp is None:
            return

        fns = resp[3:]
        f = open(fns, "rb")
        liFEN = []
        for linea in f:
            linea = linea.strip()
            if linea:
                if "|" in linea:
                    linea = linea.split("|")[0]
                liFEN.append(linea)
        nFEN = len(liFEN)
        if not nFEN:
            return

        nombre = os.path.basename(fns)[:-4]
        nombre = TrListas.dicTraining().get(nombre, nombre)

        # Motor y tiempo, cogemos los estandars de analisis
        fichero = self.configuracion.ficheroAnalisis
        dic = Util.recuperaVar(fichero)
        if dic:
            motor = dic["MOTOR"]
            tiempo = dic["TIEMPO"]
        else:
            motor = self.configuracion.tutor.clave
            tiempo = self.configuracion.tiempoTutor

        if not tiempo:
            tiempo = 3.0

        # Bucle para control de errores
        while True:
            # Datos
            liGen = [(None, None)]

            # # Nombre del entrenamiento
            liGen.append((_("Name") + ":", nombre))

            # # Tutor
            li = self.configuracion.ayudaCambioTutor()
            li[0] = motor
            liGen.append((_("Engine") + ":", li))

            # Decimas de segundo a pensar el tutor
            liGen.append((_("Duration of engine analysis (secs)") + ":", tiempo / 1000.0))

            liGen.append((None, None))

            liGen.append((FormLayout.Spinbox(_("From number"), 1, nFEN, 50), 1))
            liGen.append((FormLayout.Spinbox(_("To number"), 1, nFEN, 50), nFEN if nFEN < 20 else 20))

            resultado = FormLayout.fedit(liGen, title=nombre, parent=self, anchoMinimo=560, icon=Iconos.Opciones())

            if resultado:
                accion, liGen = resultado

                nombre = liGen[0]
                motor = liGen[1]
                tiempo = int(liGen[2] * 1000)

                if not tiempo or not nombre:
                    return

                dic = {"MOTOR": motor, "TIEMPO": tiempo}
                Util.guardaVar(fichero, dic)

                desde = liGen[3]
                hasta = liGen[4]
                nDH = hasta - desde + 1
                if nDH <= 0:
                    return
                break

            else:
                return

        # Analizamos todos, creamos las partidas, y lo salvamos
        confMotor = self.configuracion.buscaMotor(motor)
        confMotor.multiPV = 16
        xgestor = self.procesador.creaGestorMotor(confMotor, tiempo, None, True)

        mensaje = _("Analyzing the move....")
        tmpBP = QTUtil2.BarraProgreso(self.procesador.pantalla, nombre, mensaje, nDH).mostrar()

        cp = ControlPosicion.ControlPosicion()
        siCancelado = False

        bmt_lista = BMT.BMT_Lista()

        partida = Partida.Partida()

        for n in range(desde - 1, hasta):

            fen = liFEN[n]

            tmpBP.mensaje(mensaje + " %d/%d" % (n + 2 - desde, nDH))
            tmpBP.pon(n + 2 - desde)
            if tmpBP.siCancelado():
                siCancelado = True
                break

            mrm = xgestor.analiza(fen)

            cp.leeFen(fen)

            previa = 999999999
            nprevia = -1
            tniv = 0

            for rm in mrm.liMultiPV:
                if tmpBP.siCancelado():
                    siCancelado = True
                    break
                pts = rm.puntosABS()
                if pts != previa:
                    previa = pts
                    nprevia += 1
                tniv += nprevia
                rm.nivelBMT = nprevia
                rm.siElegida = False
                rm.siPrimero = False
                partida.reset(cp)
                partida.leerPV(rm.pv)
                partida.siTerminada()
                rm.txtPartida = partida.guardaEnTexto()

            if siCancelado:
                break

            bmt_uno = BMT.BMT_Uno(fen, mrm, tniv, None)

            bmt_lista.nuevo(bmt_uno)

        xgestor.terminar()

        if not siCancelado:
            # Grabamos

            reg = self.dbf.baseRegistro()
            reg.ESTADO = "0"
            reg.NOMBRE = nombre
            reg.EXTRA = "%d-%d" % (desde, hasta)
            reg.TOTAL = len(bmt_lista)
            reg.HECHOS = 0
            reg.PUNTOS = 0
            reg.MAXPUNTOS = bmt_lista.maxPuntos()
            reg.FINICIAL = Util.dtos(Util.hoy())
            reg.FFINAL = ""
            reg.SEGUNDOS = 0
            reg.BMT_LISTA = Util.var2blob(bmt_lista)
            reg.HISTORIAL = Util.var2blob([])
            reg.REPE = 0

            reg.ORDEN = 0

            self.dbf.insertarReg(reg, siReleer=True)

        self.releer()
        tmpBP.cerrar()


def pantallaBMT(procesador):
    w = WBMT(procesador)
    w.exec_()
