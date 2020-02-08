import os.path
import random
import time

from Code import Analisis
from Code import ControlPosicion
from Code import Jugada
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaPotencia
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import Util


class WDailyTestBase(QTVarios.WDialogo):
    def __init__(self, procesador):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, _("Your daily test"), Iconos.DailyTest(), "nivelBase")

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.historico = Util.DicSQL(self.configuracion.ficheroDailyTest)
        self.calcListaHistorico()

        self.motor, self.segundos, self.pruebas, self.fns = self.leeParametros()

        # Historico
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("FECHA", _("Date"), 120, siCentrado=True)
        oColumnas.nueva("MPUNTOS", _("Points lost"), 100, siCentrado=True)
        oColumnas.nueva("MTIEMPOS", _("Time"), 80, siCentrado=True)
        oColumnas.nueva("MOTOR", _("Engine"), 120, siCentrado=True)
        oColumnas.nueva("SEGUNDOS", _("Second(s)"), 80, siCentrado=True)
        oColumnas.nueva("PRUEBAS", _("N. of tests"), 80, siCentrado=True)
        oColumnas.nueva("FNS", _("File"), 150, siCentrado=True)
        self.ghistorico = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.ghistorico.setMinimumWidth(self.ghistorico.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Start"), Iconos.Empezar(), self.empezar), None,
            (_("Configuration"), Iconos.Opciones(), self.configurar), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        tb = QTVarios.LCTB(self, liAcciones)

        # Colocamos
        ly = Colocacion.V().control(tb).control(self.ghistorico).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.ghistorico)
        self.recuperarVideo()

    def leeParametros(self):
        param = Util.DicSQL(self.configuracion.ficheroDailyTest, tabla="parametros")
        motor = param.get("MOTOR", "honey")
        segundos = param.get("SEGUNDOS", 7)
        pruebas = param.get("PRUEBAS", 5)
        fns = param.get("FNS", "")
        param.close()

        return motor, segundos, pruebas, fns

    def gridNumDatos(self, grid):
        return len(self.liHistorico)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        key = self.liHistorico[fila]
        reg = self.historico[key]
        if col == "FECHA":
            fecha = reg[col]
            return Util.localDate(fecha)
        elif col == "MPUNTOS":
            mpuntos = reg["MPUNTOS"]
            return "%0.2f" % mpuntos
        elif col == "MTIEMPOS":
            mtiempos = reg["MTIEMPOS"]
            return "%0.2f" % mtiempos
        elif col == "MOTOR":
            return reg["MOTOR"]
        elif col == "SEGUNDOS":
            tiempo = int(reg["TIEMPOJUGADA"] / 1000)
            return "%d" % tiempo
        elif col == "PRUEBAS":
            nfens = len(reg["LIFENS"])
            return "%d" % nfens
        elif col == "FNS":
            fns = reg.get("FNS", None)
            if fns:
                return os.path.basename(fns)
            else:
                return _("Default")

    def calcListaHistorico(self):
        self.liHistorico = self.historico.keys(siOrdenados=True, siReverse=True)

    def closeEvent(self, event):  # Cierre con X
        self.cerrar()

    def cerrar(self):
        self.guardarVideo()
        self.historico.close()

    def terminar(self):
        self.cerrar()
        self.reject()

    def configurar(self):
        # Datos
        liGen = [(None, None)]

        # # Motor
        mt = self.configuracion.tutorInicial if self.motor is None else self.motor

        liCombo = [mt]
        for nombre, clave in self.configuracion.comboMotoresMultiPV10():
            liCombo.append((clave, nombre))

        liGen.append((_("Engine") + ":", liCombo))

        # # Segundos a pensar el tutor
        config = FormLayout.Spinbox(_("Duration of engine analysis (secs)"), 1, 99, 50)
        liGen.append((config, self.segundos))

        # Pruebas
        config = FormLayout.Spinbox(_("N. of tests"), 1, 40, 40)
        liGen.append((config, self.pruebas))

        # Fichero
        config = FormLayout.Fichero(_("File"), "%s (*.fns);;%s PGN (*.pgn)" % (_("List of FENs"), _("File")), False,
                                    anchoMinimo=280)
        liGen.append((config, self.fns))

        # Editamos
        resultado = FormLayout.fedit(liGen, title=_("Configuration"), parent=self, icon=Iconos.Opciones())
        if resultado:
            accion, liResp = resultado
            self.motor = liResp[0]
            self.segundos = liResp[1]
            self.pruebas = liResp[2]
            self.fns = liResp[3]

            param = Util.DicSQL(self.configuracion.ficheroDailyTest, tabla="parametros")
            param["MOTOR"] = self.motor
            param["SEGUNDOS"] = self.segundos
            param["PRUEBAS"] = self.pruebas
            param["FNS"] = self.fns
            param.close()

    def borrar(self):
        li = self.ghistorico.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                um = QTUtil2.unMomento(self)
                for fila in li:
                    key = self.liHistorico[fila]
                    del self.historico[key]
                self.historico.pack()
                self.calcListaHistorico()
                um.final()
                self.ghistorico.refresh()

    def empezar(self):
        liR = []
        if self.fns and Util.existeFichero(self.fns):
            fns = self.fns.lower()
            li = []
            if fns.endswith(".pgn"):
                f = open(fns, "rb")
                for linea in f:
                    if linea.startswith("[FEN "):
                        li.append(linea[6:].split('"')[0])
                f.close()
            else:  # se supone que es un fichero de fens
                f = open(fns, "rb")
                for linea in f:
                    linea = linea.strip()
                    if linea[0].isalnum() and \
                            linea[-1].isdigit() and \
                            ((" w " in linea) or (" b " in linea)) and \
                                    linea.count("/") == 7:
                        li.append(linea)
                f.close()
            if len(li) >= self.pruebas:
                liR = random.sample(li, self.pruebas)
            else:
                self.fns = ""

        if not liR:
            liR = PantallaPotencia.lee_varias_lineas_mfn(self.pruebas)

        # liR = liFens
        w = WDailyTest(self, liR, self.motor, self.segundos, self.fns)
        w.exec_()
        self.calcListaHistorico()
        self.ghistorico.refresh()


class WDailyTest(QTVarios.WDialogo):
    def __init__(self, owner, liFens, motor, segundos, fns):

        super(WDailyTest, self).__init__(owner, _("Your daily test"), Iconos.DailyTest(), "nivel")

        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion

        if motor.startswith("*"):
            motor = motor[1:]
        confMotor = self.configuracion.buscaTutor(motor, "honey")
        self.xtutor = self.procesador.creaGestorMotor(confMotor, segundos * 1000, None)
        self.xtutor.maximizaMultiPV()

        self.historico = owner.historico

        # Tablero
        confTablero = self.configuracion.confTablero("NIVEL", 48)

        self.liFens = liFens
        self.nFens = len(self.liFens)
        self.juego = 0
        self.liPuntos = []
        self.liPV = []
        self.liTiempos = []
        self.fns = fns

        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueveHumano)

        # Rotulos informacion
        self.lbColor = Controles.LB(self, "").ponWrap().anchoMinimo(200)
        self.lbJuego = Controles.LB(self, "").alinCentrado()

        # Tool bar
        liAcciones = (
            # ( _( "Start" ), Iconos.Empezar(), "empezar" ),
            (_("Analysis"), Iconos.Tutor(), "analizar"),
            (_("Cancel"), Iconos.Cancelar(), "cancelar"),
            (_("Continue"), Iconos.Pelicula_Seguir(), "seguir"),
            (_("Resign"), Iconos.Abandonar(), "abandonar"),
        )
        self.tb = Controles.TB(self, liAcciones)

        lyT = Colocacion.V().control(self.tablero).relleno()
        lyV = Colocacion.V().control(self.lbJuego).relleno().control(self.lbColor).relleno(2)
        lyTV = Colocacion.H().otro(lyT).otro(lyV)
        ly = Colocacion.V().control(self.tb).otro(lyTV)

        self.setLayout(ly)

        self.posicion = ControlPosicion.ControlPosicion()
        self.recuperarVideo()

        self.siguienteJugada()

    def terminar(self):
        self.xtutor.terminar()
        self.guardarVideo()
        self.reject()

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "abandonar":
            if QTUtil2.pregunta(self, _("Are you sure you want to resign?")):
                self.terminar()
        elif accion == "cancelar":
            if QTUtil2.pregunta(self, _("Are you sure you want to cancel?")):
                self.terminar()
        elif accion in "terminar":
            self.terminar()
        elif accion == "empezar":
            self.siguienteJugada()
        elif accion == "seguir":
            self.siguienteJugada()
        elif accion == "analizar":
            self.analizar()

    def ponToolBar(self, liAcciones):
        self.tb.clear()
        for k in liAcciones:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])

        self.tb.liAcciones = liAcciones
        self.tb.update()

    def siguienteJugada(self):
        self.ponToolBar(["abandonar"])

        if self.juego == self.nFens:
            self.terminarTest()
            return

        fen = self.liFens[self.juego]
        self.juego += 1

        self.lbJuego.ponTexto("<h2>%d/%d<h2>" % (self.juego, self.nFens))

        cp = self.posicion

        cp.leeFen(fen)

        siW = cp.siBlancas
        color, colorR = _("White"), _("Black")
        cK, cQ, cKR, cQR = "K", "Q", "k", "q"
        if not siW:
            color, colorR = colorR, color
            cK, cQ, cKR, cQR = cKR, cQR, cK, cQ

        mens = "<h1><center>%s</center></h1><br>" % color

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

        self.lbColor.ponTexto(mens)

        self.sigueHumano()
        self.iniTiempo = time.time()

    def terminarTest(self):
        self.paraHumano()
        self.xtutor.terminar()

        t = 0
        for x in self.liPuntos:
            t += x
        mpuntos = t * 1.0 / self.nFens

        t = 0.0
        for x in self.liTiempos:
            t += x
        mtiempos = t * 1.0 / self.nFens

        hoy = Util.hoy()
        fecha = "%d%02d%02d" % (hoy.year, hoy.month, hoy.day)
        datos = {}
        datos["FECHA"] = hoy
        datos["MOTOR"] = self.xtutor.clave
        datos["TIEMPOJUGADA"] = self.xtutor.motorTiempoJugada
        datos["LIFENS"] = self.liFens
        datos["LIPV"] = self.liPV
        datos["MPUNTOS"] = mpuntos
        datos["MTIEMPOS"] = mtiempos
        datos["FNS"] = self.fns

        self.historico[fecha] = datos

        self.lbColor.ponTexto("")
        self.lbJuego.ponTexto("")

        mens = "<h3>%s : %0.2f</h3><h3>%s : %0.2f</h3>" % (_("Points lost"), mpuntos, _("Time in seconds"), mtiempos)
        QTUtil2.mensaje(self, mens, _("Result"), siResalta=False)

        self.accept()

    def paraHumano(self):
        self.tablero.desactivaTodas()

    def sigueHumano(self):
        siW = self.posicion.siBlancas
        self.tablero.ponPosicion(self.posicion)
        self.tablero.ponerPiezasAbajo(siW)
        self.tablero.ponIndicador(siW)
        self.tablero.activaColor(siW)

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

        siBien, mens, self.jg = Jugada.dameJugada(self.posicion, desde, hasta, coronacion)
        if siBien:
            self.tablero.ponPosicion(self.jg.posicion)
            self.tablero.ponFlechaSC(desde, hasta)
            self.calculaTiempoPuntos()
        else:
            self.sigueHumano()

    def calculaTiempoPuntos(self):
        tiempo = time.time() - self.iniTiempo

        um = QTUtil2.analizando(self)
        self.rmr, pos = self.xtutor.analizaJugada(self.jg, self.xtutor.motorTiempoJugada)
        self.jg.analisis = self.rmr, pos
        um.final()
        pv = self.jg.movimiento()
        li = []
        pv = pv.lower()

        minimo = self.rmr.liMultiPV[0].puntosABS()
        actual = None
        mens = "<h2>%d/%d</h2><center><table>" % (self.juego, self.nFens)
        li = []
        for rm in self.rmr.liMultiPV:
            pts = rm.puntosABS()
            ptsc = minimo - pts
            mv = rm.movimiento().lower()
            if mv == pv:
                actual = ptsc
            pgn = self.posicion.pgnSP(mv[:2], mv[2:4], mv[4:])
            li.append((mv == pv, pgn, pts, ptsc))

        if actual is None:
            actual = ptsc

        for siPV, pgn, pts, ptsc in li:
            dosp = "&nbsp;:&nbsp;"
            dosi = "&nbsp;=&nbsp;"
            cpts = "%d" % pts
            cptsc = "%d" % ptsc
            if siPV:
                ini = "<b>"
                fin = "</b>"
                pgn = ini + pgn + fin
                dosp = ini + dosp + fin
                dosi = ini + dosi + fin
                cpts = ini + cpts + fin
                cptsc = ini + cptsc + fin

            mens += "<tr><td>%s</td><td>%s</td><td align=\"right\">%s</td><td>%s</td><td align=\"right\">%s</td></tr>" % (
                pgn, dosp, cpts, dosi, cptsc)
        mens += "</table></center>"

        self.liPV.append(pv)
        self.liPuntos.append(actual)
        self.liTiempos.append(tiempo)

        self.lbJuego.ponTexto(mens)
        self.lbColor.ponTexto("")
        self.ponToolBar(["seguir", "cancelar", "analizar"])

    def analizar(self):
        Analisis.muestraAnalisis(self.procesador, self.xtutor, self.jg, self.posicion.siBlancas, 9999999, 1,
                                 pantalla=self, siGrabar=False)


def dailyTest(procesador):
    w = WDailyTestBase(procesador)
    w.exec_()
