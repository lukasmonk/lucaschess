import atexit
import datetime
import random
import time

import LCEngine4 as LCEngine

from Code import ControlPosicion
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code.SQL import Base
from Code import Util


class HorsesHistorico:
    def __init__(self, fichero, test):
        self.fichero = fichero
        self.db = Base.DBBase(fichero)
        self.tabla = test

        if not self.db.existeTabla(self.tabla):
            self.creaTabla()

        self.dbf = self.db.dbf(self.tabla, "FECHA,MOVES,SECONDS,HINTS", orden="FECHA DESC")
        self.dbf.leer()

        self.orden = "FECHA", "DESC"

        atexit.register(self.close)

    def close(self):
        if self.dbf:
            self.dbf.cerrar()
            self.dbf = None
        self.db.cerrar()

    def creaTabla(self):
        tb = Base.TablaBase(self.tabla)
        tb.nuevoCampo("FECHA", "VARCHAR", notNull=True, primaryKey=True)
        tb.nuevoCampo("MOVES", "INTEGER")
        tb.nuevoCampo("SECONDS", "INTEGER")
        tb.nuevoCampo("HINTS", "INTEGER")
        self.db.generarTabla(tb)

    def __len__(self):
        return self.dbf.reccount()

    def goto(self, num):
        self.dbf.goto(num)

    def ponOrden(self, clave):
        nat, orden = self.orden
        if clave == nat:
            orden = "DESC" if orden == "ASC" else "ASC"
        else:
            nat = clave
            orden = "DESC" if clave == "FECHA" else "ASC"
        self.dbf.ponOrden(nat + " " + orden)
        self.orden = nat, orden

        self.dbf.leer()
        self.dbf.gotop()

    def fecha2txt(self, fecha):
        return "%4d%02d%02d%02d%02d%02d" % (fecha.year, fecha.month, fecha.day, fecha.hour, fecha.minute, fecha.second)

    def txt2fecha(self, txt):
        def x(d, h): return int(txt[d:h])

        year = x(0, 4)
        month = x(4, 6)
        day = x(6, 8)
        hour = x(8, 10)
        minute = x(10, 12)
        second = x(12, 14)
        fecha = datetime.datetime(year, month, day, hour, minute, second)
        return fecha

        self.dbf = self.db.dbf(self.tabla, "", orden="fecha desc")

    def append(self, fecha, moves, seconds, hints):
        br = self.dbf.baseRegistro()
        br.FECHA = self.fecha2txt(fecha)
        br.MOVES = moves
        br.SECONDS = seconds
        br.HINTS = hints
        self.dbf.insertar(br)

    def __getitem__(self, num):
        self.dbf.goto(num)
        reg = self.dbf.registroActual()
        reg.FECHA = self.txt2fecha(reg.FECHA)
        return reg

    def borrarLista(self, liNum):
        self.dbf.borrarLista(liNum)
        self.dbf.pack()
        self.dbf.leer()


class WHorsesBase(QTVarios.WDialogo):
    def __init__(self, procesador, test, titulo, tabla, icono):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, "horsesBase")

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.tabla = tabla
        self.icono = icono
        self.test = test
        self.titulo = titulo

        self.historico = HorsesHistorico(self.configuracion.ficheroHorses, tabla)

        # Historico
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("FECHA", _("Date"), 120, siCentrado=True)
        oColumnas.nueva("MOVES", _("Moves"), 100, siCentrado=True)
        oColumnas.nueva("SECONDS", _("Second(s)"), 80, siCentrado=True)
        oColumnas.nueva("HINTS", _("Hints"), 90, siCentrado=True)
        self.ghistorico = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.ghistorico.setMinimumWidth(self.ghistorico.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), "terminar"), None,
            (_("Start"), Iconos.Empezar(), "empezar"),
            (_("Remove"), Iconos.Borrar(), "borrar"), None,
        )
        self.tb = Controles.TB(self, liAcciones)
        self.ponToolBar(["terminar", "empezar", "borrar"])

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.ghistorico).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.ghistorico)
        self.recuperarVideo(siTam=False)

        self.ghistorico.gotop()

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave
        if clave in ("FECHA", "MOVES", "HINTS"):
            self.historico.ponOrden(clave)
            self.ghistorico.gotop()
            self.ghistorico.refresh()

    def gridNumDatos(self, grid):
        return len(self.historico)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.historico[fila]
        if col == "FECHA":
            return Util.localDateT(reg.FECHA)
        elif col == "MOVES":
            return "%d" % reg.MOVES
        elif col == "SECONDS":
            return "%d" % reg.SECONDS
        elif col == "HINTS":
            return "%d" % reg.HINTS

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "terminar":
            self.guardarVideo()
            self.historico.close()
            self.reject()

        elif accion == "empezar":
            self.empezar()

        elif accion == "borrar":
            self.borrar()

    def borrar(self):
        li = self.ghistorico.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.historico.borrarLista(li)
        self.ghistorico.gotop()
        self.ghistorico.refresh()

    def ponToolBar(self, liAcciones):

        self.tb.clear()
        for k in liAcciones:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])

        self.tb.liAcciones = liAcciones
        self.tb.update()

    def empezar(self):
        w = WHorses(self, self.test, self.procesador, self.titulo, self.icono)
        w.exec_()
        self.ghistorico.gotop()
        self.ghistorico.refresh()


class WHorses(QTVarios.WDialogo):
    def __init__(self, owner, test, procesador, titulo, icono):

        QTVarios.WDialogo.__init__(self, owner, titulo, icono, "horses")

        self.historico = owner.historico
        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion

        self.test = test

        # Tablero
        confTablero = self.configuracion.confTablero("HORSES", 48)

        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueveHumano)

        # Rotulo tiempo
        self.lbInformacion = Controles.LB(self, _("Goal: to capture the king up to the square a8")).alinCentrado()
        self.lbMoves = Controles.LB(self, "")

        # Tool bar
        liAcciones = (
            (_("Cancel"), Iconos.Cancelar(), "cancelar"),
            (_("Reinit"), Iconos.Reiniciar(), "reiniciar"),
            (_("Help"), Iconos.AyudaGR(), "ayuda"),
        )
        self.tb = Controles.TB(self, liAcciones)

        # Layout
        lyInfo = Colocacion.H().control(self.lbInformacion).relleno().control(self.lbMoves)
        lyT = Colocacion.V().relleno().control(self.tablero).otro(lyInfo).relleno().margen(10)

        ly = Colocacion.V().control(self.tb).otro(lyT).relleno().margen(0)

        self.setLayout(ly)

        self.recuperarVideo()
        self.adjustSize()

        liTB = ["cancelar", "reiniciar", "ayuda"]
        self.ponToolBar(liTB)

        self.reset()

    def reset(self):
        self.preparaTest()
        self.tablero.ponerPiezasAbajo(True)
        self.tablero.ponPosicion(self.cpInicial)
        self.tablero.quitaFlechas()
        self.ponSiguiente()
        self.timer = time.time()
        self.moves = 0
        self.hints = 0
        self.nayuda = 0  # para que haga un rondo

    def ponNumMoves(self):
        color = "red" if self.numMoves <= self.movesParcial else "green"
        self.lbMoves.ponTexto('<font color="%s">%d/%d</font>' % (color, self.movesParcial, self.numMoves))

    def ponSiguiente(self):

        posDesde = self.camino[0 if self.baseUnica else self.posActual]
        posHasta = self.camino[self.posActual + 1]
        tlist = LCEngine.liNMinimo(posDesde, posHasta, self.celdas_ocupadas)
        self.numMoves = len(tlist[0]) - 1
        self.movesParcial = 0

        cp = self.cpInicial.copia()

        self.posTemporal = posDesde
        ca = LCEngine.posA1(posDesde)
        cp.casillas[ca] = "N" if self.siBlancas else "n"
        cs = LCEngine.posA1(posHasta)
        cp.casillas[cs] = "k" if self.siBlancas else "K"

        self.cpActivo = cp

        self.tablero.ponPosicion(cp)
        self.tablero.activaColor(self.siBlancas)

        self.ponNumMoves()

    def avanza(self):
        self.tablero.quitaFlechas()
        self.posActual += 1
        if self.posActual == len(self.camino) - 1:
            self.final()
            return
        self.ponSiguiente()

    def final(self):
        seconds = int(time.time() - self.timer)
        self.historico.append(Util.hoy(), self.moves, seconds, self.hints)

        QTUtil2.mensaje(self, "<b>%s<b><ul><li>%s: <b>%d</b></li><li>%s: <b>%d</b></li><li>%s: <b>%d</b></li></ul>" % (
            _("Congratulations goal achieved"), _("Moves"), self.moves, _("Second(s)"), seconds, _("Hints"),
            self.hints))

        self.guardarVideo()
        self.accept()

    def mueveHumano(self, desde, hasta, coronacion=""):
        p0 = LCEngine.a1Pos(desde)
        p1 = LCEngine.a1Pos(hasta)
        if p1 in LCEngine.dicN[p0]:
            self.moves += 1
            self.movesParcial += 1
            self.ponNumMoves()
            if p1 not in self.camino:
                return False
            self.cpActivo.casillas[desde] = None
            self.cpActivo.casillas[hasta] = "N" if self.siBlancas else "n"
            self.tablero.ponPosicion(self.cpActivo)
            self.tablero.activaColor(self.siBlancas)
            self.posTemporal = p1
            if p1 == self.camino[self.posActual + 1]:
                self.avanza()
                return True
            return True
        return False

    def preparaTest(self):
        self.cpInicial = ControlPosicion.ControlPosicion()
        self.cpInicial.leeFen("8/8/8/8/8/8/8/8 w - - 0 1")
        casillas = self.cpInicial.casillas
        self.baseUnica = self.test > 3
        self.siBlancas = random.randint(1, 2) == 1

        if self.test in (1, 4, 5):
            celdas_ocupadas = []
        elif self.test == 2:  # 4 peones
            celdas_ocupadas = [18, 21, 25, 27, 28, 30, 42, 45, 49, 51, 52, 54]
            for a1 in ("c3", "c6", "f3", "f6"):
                casillas[a1] = "p" if self.siBlancas else "P"
        elif self.test == 3:  # levitt
            ch = celdas_ocupadas = [27]
            for li in LCEngine.dicQ[27]:
                for x in li:
                    ch.append(x)

            casillas["d4"] = "q" if self.siBlancas else "Q"

        self.camino = []
        p, f, s = 0, 7, 1
        for x in range(8):
            li = range(p, f + s, s)
            for t in range(7, -1, -1):
                if li[t] in celdas_ocupadas:
                    del li[t]
            self.camino.extend(li)
            if s == 1:
                s = -1
                p += 15
                f += 1
            else:
                s = +1
                p += 1
                f += 15

        if self.test == 5:  # empieza en e4
            for n, x in enumerate(self.camino):
                if x == 28:
                    del self.camino[n]
                    self.camino.insert(0, 28)
                    break

        self.posActual = 0
        self.celdas_ocupadas = celdas_ocupadas

    def closeEvent(self, event):
        self.guardarVideo()
        event.accept()

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "cancelar":
            self.guardarVideo()
            self.reject()
        elif accion == "ayuda":
            self.ayuda()
        elif accion == "reiniciar":
            self.reiniciar()

    def reiniciar(self):
        # Si no esta en la posicion actual, le lleva a la misma
        pa = self.posTemporal
        pi = self.camino[0 if self.baseUnica else self.posActual]

        if pa == pi:
            self.reset()
        else:
            self.ponSiguiente()

    def ayuda(self):
        self.hints += 1
        self.tablero.quitaFlechas()
        self.ponSiguiente()
        pa = self.camino[0 if self.baseUnica else self.posActual]
        ps = self.camino[self.posActual + 1]
        tlist = LCEngine.liNMinimo(pa, ps, self.celdas_ocupadas)
        if self.nayuda >= len(tlist):
            self.nayuda = 0

        li = tlist[self.nayuda]
        for x in range(len(li) - 1):
            d = LCEngine.posA1(li[x])
            h = LCEngine.posA1(li[x + 1])
            self.tablero.creaFlechaMov(d, h, "2")
        self.nayuda += 1

    def ponToolBar(self, liAcciones):
        self.tb.clear()
        for k in liAcciones:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])

        self.tb.liAcciones = liAcciones
        self.tb.update()


def pantallaHorses(procesador, test, titulo, icono):
    tabla = "TEST%d" % test
    w = WHorsesBase(procesador, test, titulo, tabla, icono)
    w.exec_()
