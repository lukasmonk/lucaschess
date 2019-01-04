import atexit
import base64
import datetime
import random
import time

from PyQt4 import QtGui, QtCore

from Code import Analisis
from Code import ControlPosicion
from Code import Jugada
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code.SQL import Base
from Code import Util
from Code import VarGen


def lee_1_linea_mfn(linea):
    cabs, pv, jugada = linea.strip().split("||")
    dic = Util.SymbolDict()
    for x in cabs.split("|"):
        k, v = x.split(VarGen.XSEP)
        dic[k] = v
    p = Partida.Partida()
    p.leerPV(pv)
    event = dic["Event"]
    site = dic["Site"]
    if site and site != event:
        event += "-%s" % site
    date = dic["Date"].replace(".?", "").replace("?", "")
    white = dic["White"]
    black = dic["Black"]
    result = dic["Result"]
    info = "<b>%s - %s (%s)</b>    %s (%s) " % (white, black, result, event, date,)
    return p, dic, info, int(jugada), linea


def lee_linea_mfn():
    npos = random.randint(0, 9999)
    with open("./IntFiles/games.mfn") as f:
        for num, linea in enumerate(f):
            if num == npos:
                return lee_1_linea_mfn(linea)


def lee_varias_lineas_mfn(nlineas):  # PantallaDailyTest
    lipos = random.sample(range(0, 9999), nlineas)
    lifen = []
    with open("./IntFiles/games.mfn") as f:
        for num, linea in enumerate(f):
            if num in lipos:
                cabs, pv, jugada = linea.strip().split("||")
                p = Partida.Partida()
                p.leerPV(pv)
                fen = p.jugada(int(jugada)).posicion.fen()
                lifen.append(fen)
    return lifen


class PotenciaHistorico:
    def __init__(self, fichero):
        self.fichero = fichero
        self.db = Base.DBBase(fichero)
        self.tabla = "datos"

        if not self.db.existeTabla(self.tabla):
            self.crea_tabla()

        self.dbf = self.db.dbf(self.tabla, "REF,FECHA,SCORE,MOTOR,SEGUNDOS,MIN_MIN,MIN_MAX,LINE", orden="FECHA DESC")

        self.dbf.leer()

        self.orden = "FECHA", "DESC"

        atexit.register(self.close)

    def close(self):
        if self.dbf:
            self.dbf.cerrar()
            self.dbf = None
        self.db.cerrar()

    def crea_tabla(self):
        tb = Base.TablaBase(self.tabla)
        tb.nuevoCampo("FECHA", "VARCHAR", notNull=True, primaryKey=True)
        tb.nuevoCampo("REF", "INTEGER")
        tb.nuevoCampo("SCORE", "INTEGER")
        tb.nuevoCampo("MOTOR", "VARCHAR")
        tb.nuevoCampo("SEGUNDOS", "INTEGER")
        tb.nuevoCampo("MIN_MIN", "INTEGER")
        tb.nuevoCampo("MIN_MAX", "INTEGER")
        tb.nuevoCampo("LINE", "TEXT")
        tb.nuevoIndice("IND_SCORE", "SCORE")
        self.db.generarTabla(tb)

    def __len__(self):
        return self.dbf.reccount()

    def goto(self, num):
        self.dbf.goto(num)

    def pon_orden(self, clave):
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

    @staticmethod
    def fecha2txt(fecha):
        return "%4d%02d%02d%02d%02d%02d" % (fecha.year, fecha.month, fecha.day, fecha.hour, fecha.minute, fecha.second)

    @staticmethod
    def txt2fecha(txt):
        def x(d, h): return int(txt[d:h])

        year = x(0, 4)
        month = x(4, 6)
        day = x(6, 8)
        hour = x(8, 10)
        minute = x(10, 12)
        second = x(12, 14)
        fecha = datetime.datetime(year, month, day, hour, minute, second)
        return fecha

    def append(self, fecha, score, motor, segundos, min_min, min_max, linea, ref):

        br = self.dbf.baseRegistro()
        if ref is None:
            ref = self.dbf.maxCampo("REF")
            if not ref:
                ref = 1
            else:
                ref += 1
        br.REF = ref
        br.FECHA = self.fecha2txt(fecha)
        br.SCORE = score
        br.MOTOR = motor
        br.SEGUNDOS = segundos
        br.MIN_MIN = min_min
        br.MIN_MAX = min_max
        br.LINE = base64.encodestring(linea)
        self.dbf.insertar(br)

    def __getitem__(self, num):
        self.dbf.goto(num)
        reg = self.dbf.registroActual()
        reg.FECHA = self.txt2fecha(reg.FECHA)
        return reg

    def borrar_lista(self, linum):
        self.dbf.borrarLista(linum)
        self.dbf.pack()
        self.dbf.leer()


class EDCelda(Controles.ED):
    def focusOutEvent(self, event):
        self.parent.focusOut(self)
        Controles.ED.focusOutEvent(self, event)


class WEdMove(QtGui.QWidget):
    def __init__(self, owner, conj_piezas, si_blancas):
        QtGui.QWidget.__init__(self)

        self.owner = owner

        self.conj_piezas = conj_piezas

        self.filaPromocion = (7, 8) if si_blancas else (2, 1)

        self.menuPromocion = self.creaMenuPiezas("QRBN ", si_blancas)

        self.promocion = " "

        self.origen = EDCelda(self, "").caracteres(2).controlrx("(|[a-h][1-8])").anchoFijo(
                24).alinCentrado().capturaCambiado(self.miraPromocion)

        self.flecha = flecha = Controles.LB(self).ponImagen(Iconos.pmMover())

        self.destino = EDCelda(self, "").caracteres(2).controlrx("(|[a-h][1-8])").anchoFijo(
                24).alinCentrado().capturaCambiado(self.miraPromocion)

        self.pbPromocion = Controles.PB(self, "", self.pulsadoPromocion, plano=False).anchoFijo(24)

        ly = Colocacion.H().relleno().control(self.origen).espacio(2).control(flecha).espacio(2).control(
                self.destino).control(self.pbPromocion).margen(0).relleno()
        self.setLayout(ly)

        self.miraPromocion()

    def focusOut(self, quien):
        self.owner.ponUltimaCelda(quien)

    def activa(self):
        self.setFocus()
        self.origen.setFocus()

    def activaDestino(self):
        self.setFocus()
        self.destino.setFocus()

    def resultado(self):
        desde = hasta = ""

        desde = self.origen.texto()
        if len(desde) != 2:
            desde = ""

        hasta = self.destino.texto()
        if len(hasta) != 2:
            desde = ""

        return desde, hasta, self.promocion.strip()

    def deshabilita(self):
        self.origen.deshabilitado(True)
        self.destino.deshabilitado(True)
        self.pbPromocion.setEnabled(False)
        if not self.origen.texto() or not self.destino.texto():
            self.origen.hide()
            self.destino.hide()
            self.pbPromocion.hide()
            self.flecha.hide()

    def miraPromocion(self):
        show = True
        ori, dest = self.filaPromocion
        txtO = self.origen.texto()
        if len(txtO) < 2 or int(txtO[-1]) != ori:
            show = False
        if show:
            txtD = self.destino.texto()
            if len(txtD) < 2 or int(txtD[-1]) != dest:
                show = False
        self.pbPromocion.setVisible(show)
        return show

    def pulsadoPromocion(self):
        if not self.miraPromocion():
            return
        resp = self.menuPromocion.exec_(QtGui.QCursor.pos())
        if resp is not None:
            icono = self.conj_piezas.icono(resp.clave) if resp.clave else QtGui.QIcon()
            self.pbPromocion.ponIcono(icono)
            self.promocion = resp.clave

    def creaMenuPiezas(self, lista, siBlancas):
        menu = QtGui.QMenu(self)

        dic = {"K": _("King"), "Q": _("Queen"), "R": _("Rook"), "B": _("Bishop"), "N": _("Knight"), "P": _("Pawn")}

        for pz in lista:
            if pz == " ":
                icono = QtGui.QIcon()
                txt = _("Remove")
            else:
                txt = dic[pz]
                if not siBlancas:
                    pz = pz.lower()
                icono = self.conj_piezas.icono(pz)

            accion = QtGui.QAction(icono, txt, menu)

            accion.clave = pz.strip()
            menu.addAction(accion)

        return menu


class WBlqMove(QtGui.QWidget):
    def __init__(self, owner, conj_piezas, siBlancas, posicion):
        QtGui.QWidget.__init__(self)

        self.owner = owner
        self.wm = WEdMove(self, conj_piezas, siBlancas)
        self.ms = Controles.LB(self, "")
        self.an = Controles.PB(self, "?", self.analizarUno, plano=False).anchoFijo(18)
        self.cancelar = Controles.LB(self, "").ponImagen(Iconos.pmCancelarPeque())
        self.aceptar = Controles.LB(self, "").ponImagen(Iconos.pmAceptarPeque())
        ly = Colocacion.H().control(self.aceptar).control(self.cancelar).control(self.wm).control(self.an).control(
                self.ms).relleno().margen(0)
        self.setLayout(ly)

        self.ms.hide()
        self.an.hide()
        self.aceptar.hide()
        self.cancelar.hide()

        self.posicion = posicion

    def ponUltimaCelda(self, quien):
        self.owner.ponUltimaCelda(quien)

    def activa(self):
        self.setFocus()
        self.wm.activa()

    def analizarUno(self):
        self.owner.analizar(self.posicion)

    def deshabilita(self):
        self.wm.deshabilita()
        self.an.hide()
        self.ms.hide()
        self.aceptar.hide()
        self.cancelar.hide()

    def resultado(self):
        return self.wm.resultado()

    def ponPuntos(self, puntos):
        self.ms.ponTexto("%s: %d/100" % (_("Points"), puntos))
        self.ms.show()
        self.an.show()

    def ponError(self, mensaje):
        self.ms.ponTexto(mensaje)
        self.ms.show()

    def siCorrecto(self, correcto):
        if correcto:
            self.aceptar.show()
        else:
            self.cancelar.show()


class WPotenciaBase(QTVarios.WDialogo):
    def __init__(self, procesador):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, _("Determine your calculating power"), Iconos.Potencia(),
                                   "potenciaBase")

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.historico = PotenciaHistorico(self.configuracion.ficheroPotencia)

        self.motor, self.segundos, self.min_min, self.min_max = self.leeParametros()

        # Historico
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("REF", _("N."), 35, siCentrado=True)
        oColumnas.nueva("FECHA", _("Date"), 120, siCentrado=True)
        oColumnas.nueva("SCORE", _("Score"), 100, siCentrado=True)
        oColumnas.nueva("MOTOR", _("Engine"), 120, siCentrado=True)
        oColumnas.nueva("SEGUNDOS", _("Second(s)"), 80, siCentrado=True)
        oColumnas.nueva("MIN_MIN", _("Minimum minutes"), 90, siCentrado=True)
        oColumnas.nueva("MIN_MAX", _("Maximum minutes"), 90, siCentrado=True)
        self.ghistorico = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.ghistorico.setMinimumWidth(self.ghistorico.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Start"), Iconos.Empezar(), self.empezar),
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Configuration"), Iconos.Opciones(), self.configurar), None,
            (_("Repeat"), Iconos.Pelicula_Repetir(), self.repetir), None,
        )
        self.tb = Controles.TBrutina(self, liAcciones)
        # self.ponToolBar([self.terminar, self.empezar, self.repetir, self.configurar, self.borrar])

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.ghistorico).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.ghistorico)
        self.recuperarVideo(siTam=False)

        self.ghistorico.gotop()

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave
        if clave in ("FECHA", "SCORE", "REF"):
            self.historico.pon_orden(clave)
            self.ghistorico.gotop()
            self.ghistorico.refresh()

    def gridDobleClick(self, grid, fil, col):
        self.repetir(fil)

    def repetir(self, fil=None):
        if fil is None:
            fil = self.ghistorico.recno()
            if fil < 0:
                return
        reg = self.historico[fil]
        linea = reg.LINE
        if linea:
            linea = base64.decodestring(linea)
            w = WPotencia(self, self.motor, self.segundos, self.min_min, self.min_max, linea, reg.REF)
            w.exec_()
            self.ghistorico.gotop()
            self.ghistorico.refresh()

    def leeParametros(self):
        param = Util.DicSQL(self.configuracion.ficheroPotencia, tabla="parametros")
        motor = param.get("MOTOR", "stockfish")
        segundos = param.get("SEGUNDOS", 5)
        min_min = param.get("MIN_MIN", 1)
        min_max = param.get("MIN_MAX", 5)
        param.close()

        return motor, segundos, min_min, min_max

    def gridNumDatos(self, grid):
        return len(self.historico)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.historico[fila]
        if col == "FECHA":
            return Util.localDateT(reg.FECHA)
        elif col == "SCORE":
            return str(reg.SCORE)
        elif col == "MOTOR":
            return reg.MOTOR
        elif col == "SEGUNDOS":
            return str(reg.SEGUNDOS)
        elif col == "MIN_MIN":
            return str(reg.MIN_MIN)
        elif col == "MIN_MAX":
            return str(reg.MIN_MAX)
        elif col == "REF":
            return str(reg.REF)

    def terminar(self):
        self.guardarVideo()
        self.historico.close()
        self.reject()

    def borrar(self):
        li = self.ghistorico.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.historico.borrar_lista(li)
        self.ghistorico.gotop()
        self.ghistorico.refresh()

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

        # Minutos
        config = FormLayout.Spinbox(_("Minimum minutes"), 0, 99, 50)
        liGen.append((config, self.min_min))

        config = FormLayout.Spinbox(_("Maximum minutes"), 0, 99, 50)
        liGen.append((config, self.min_max))

        # Editamos
        resultado = FormLayout.fedit(liGen, title=_("Configuration"), parent=self, icon=Iconos.Opciones())
        if resultado:
            accion, liResp = resultado
            self.motor = liResp[0]
            self.segundos = liResp[1]
            self.min_min = liResp[2]
            self.min_max = liResp[3]

            param = Util.DicSQL(self.configuracion.ficheroPotencia, tabla="parametros")
            param["MOTOR"] = self.motor
            param["SEGUNDOS"] = self.segundos
            param["MIN_MIN"] = self.min_min
            param["MIN_MAX"] = self.min_max
            param.close()

            # def ponToolBar(self, liAcciones):

            # self.tb.clear()
            # for k in liAcciones:
            # self.tb.dicTB[k].setVisible(True)
            # self.tb.dicTB[k].setEnabled(True)
            # self.tb.addAction(self.tb.dicTB[k])

            # self.tb.liAcciones = liAcciones
            # self.tb.update()

    def empezar(self):
        w = WPotencia(self, self.motor, self.segundos, self.min_min, self.min_max)
        w.exec_()
        self.ghistorico.gotop()
        self.ghistorico.refresh()


class WPotencia(QTVarios.WDialogo):
    def __init__(self, owner, motor, segundos, min_min, min_max, linea=None, ref=None):

        super(WPotencia, self).__init__(owner, _("Determine your calculating power"), Iconos.Potencia(), "potencia")

        self.partida, self.dicPGN, info, self.jugadaInicial, self.linea = lee_1_linea_mfn(
                linea) if linea else lee_linea_mfn()
        self.fen = self.partida.jugada(self.jugadaInicial).posicion.fen()
        self.ref = ref

        self.historico = owner.historico
        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion

        if motor.startswith("*"):
            motor = motor[1:]
        confMotor = self.configuracion.buscaTutor(motor, "stockfish")
        self.xtutor = self.procesador.creaGestorMotor(confMotor, segundos * 1000, None)
        self.xtutor.maximizaMultiPV()

        # Tablero
        confTablero = self.configuracion.confTablero("POTENCIA", 48)

        self.min_min = min_min
        self.min_max = min_max

        cp = self.partida.jugada(self.jugadaInicial).posicion

        self.tablero = Tablero.TableroEstatico(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(cp.siBlancas)
        self.tablero.ponPosicion(cp)

        # Rotulo informacion
        self.lbInformacion = self.creaLBInformacion(info, cp)

        # Consultar la partida
        self.btConsultar = Controles.PB(self, _("Show game"), self.consultar, plano=False)

        # Rotulo tiempo
        self.lbTiempo = Controles.LB(self, "").alinCentrado()

        self.liwm = []
        conj_piezas = self.tablero.piezas
        siBlancas = cp.siBlancas
        for i in range(12):
            wm = WBlqMove(self, conj_piezas, siBlancas, i)
            self.liwm.append(wm)
            siBlancas = not siBlancas

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar),
            (_("Cancel"), Iconos.Cancelar(), self.cancelar),
            (_("Check"), Iconos.Check(), self.comprobar),
        )
        self.tb = Controles.TBrutina(self, liAcciones)

        # Layout
        lyInfo = Colocacion.H().relleno().control(self.lbInformacion).control(self.btConsultar).relleno()
        lyT = Colocacion.V().relleno().control(self.tablero).otro(lyInfo).controlc(self.lbTiempo).relleno()

        lyV = Colocacion.V()
        for wm in self.liwm:
            lyV.control(wm)
        lyV.relleno()
        f = Controles.TipoLetra(puntos=10, peso=75)
        self.gbMovs = Controles.GB(self, _("Next moves"), lyV).ponFuente(f)

        lyTV = Colocacion.H().otro(lyT).control(self.gbMovs).relleno()

        ly = Colocacion.V().control(self.tb).otro(lyTV).relleno()

        self.setLayout(ly)

        self.recuperarVideo()
        self.adjustSize()

        liTB = [self.cancelar]

        # Tiempo
        self.timer = None
        if min_min or min_max:
            self.baseTiempo = time.time()
            if min_min:
                self.gbMovs.hide()
                self.iniciaReloj(self.pensandoHastaMin)
            else:
                liTB.insert(0, self.comprobar)
                self.iniciaReloj(self.pensandoHastaMax)

        self.ponToolBar(liTB)

        self.liwm[0].activa()

        self.btConsultar.hide()

        self.ultimaCelda = None

    def consultar(self):
        pgn = ""
        for k, v in self.dicPGN.iteritems():
            pgn += '[%s "%s"]\n' % (k, v)
        pgn += "\n" + self.partida.pgnBaseRAW()

        nuevoPGN, pv, dicPGN = self.procesador.gestorUnPGN(self, pgn, self.jugadaInicial + 1, False)

    def pulsadaCelda(self, celda):
        if self.ultimaCelda:
            self.ultimaCelda.ponTexto(celda)

            ucld = self.ultimaCelda
            for num, blq in enumerate(self.liwm):
                wm = blq.wm
                if wm.origen == ucld:
                    wm.miraPromocion()
                    wm.activaDestino()
                    self.ultimaCelda = wm.destino
                    return
                elif wm.destino == ucld:
                    wm.miraPromocion()
                    if num < (len(self.liwm) - 1):
                        x = num + 1
                    else:
                        x = 0
                    blq = self.liwm[x]
                    wm = blq.wm
                    wm.activa()
                    self.ultimaCelda = wm.origen
                    return

    def ponUltimaCelda(self, wmcelda):
        self.ultimaCelda = wmcelda

    def pensandoHastaMin(self):
        dif = self.min_min * 60 - int(time.time() - self.baseTiempo)
        if dif <= 0:
            self.ponToolBar([self.comprobar, self.cancelar])
            self.paraReloj()
            if self.min_max:
                self.gbMovs.show()
                self.liwm[0].activa()
                self.baseTiempo = time.time()
                self.iniciaReloj(self.pensandoHastaMax)
        else:
            self.lbTiempo.ponTexto(_X(_("%1 seconds remain to think moves before you can indicate them"), str(dif)))

    def pensandoHastaMax(self):
        dif = (self.min_max - self.min_min) * 60 - int(time.time() - self.baseTiempo)
        if dif <= 0:
            self.paraReloj()
            self.comprobar()
        else:
            self.lbTiempo.ponTexto(_X(_("%1 seconds remain to indicate moves"), str(dif)))

    def iniciaReloj(self, enlace, transicion=1000):
        if self.timer is not None:
            self.timer.stop()

        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), enlace)
        self.timer.start(transicion)

    def paraReloj(self):
        self.lbTiempo.ponTexto("")
        if self.timer is not None:
            self.timer.stop()
            del self.timer
            self.timer = None

    def closeEvent(self, event):
        self.paraReloj()
        self.guardarVideo()
        event.accept()

    def terminar(self):
        self.paraReloj()
        self.guardarVideo()
        self.reject()

    def cancelar(self):
        self.terminar()

    def comprobar(self):
        self.paraReloj()
        self.ponToolBar([self.cancelar])
        for wm in self.liwm:
            wm.deshabilita()

        um = QTUtil2.analizando(self)

        self.liAnalisis = []
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.fen)
        siError = False
        totalPuntos = 0
        factor = 1
        previo = 100
        for wm in self.liwm:
            desde, hasta, coronacion = wm.resultado()
            if desde:
                cpNue = cp.copia()
                siBien, mensaje = cpNue.mover(desde, hasta, coronacion)
                wm.siCorrecto(siBien)
                if not siBien:
                    wm.ponError(_("Invalid move"))
                    siError = True
                    break
                jg = Jugada.Jugada()
                jg.ponDatos(cp, cpNue, desde, hasta, coronacion)
                mrm, pos = self.xtutor.analizaJugada(jg, self.xtutor.motorTiempoJugada)
                jg.analisis = mrm, pos

                self.liAnalisis.append(jg)

                rm = mrm.liMultiPV[pos]
                rj = mrm.liMultiPV[0]
                dif = rj.puntosABS() - rm.puntosABS()
                if dif >= 100:
                    puntos = 0
                else:
                    puntos = 100 - dif
                wm.ponPuntos(puntos)
                cp = cpNue
                totalPuntos += int(puntos * factor * previo / 100)
                previo = puntos * previo / 100
                factor *= 2
            else:
                break

        um.final()
        self.btConsultar.show()

        if not siError:
            self.lbTiempo.ponTexto("<h2>%s: %d %s</h2>" % (_("Result"), totalPuntos, _("pts")))

            self.historico.append(Util.hoy(), totalPuntos, self.xtutor.clave, int(self.xtutor.motorTiempoJugada / 1000),
                                  self.min_min, self.min_max, self.linea, self.ref)

            self.ponToolBar([self.terminar])

    def ponToolBar(self, liAcciones):

        self.tb.clear()
        for k in liAcciones:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])

        self.tb.liAcciones = liAcciones
        self.tb.update()

    def creaLBInformacion(self, info, cp):

        color, colorR = _("White"), _("Black")
        cK, cQ, cKR, cQR = "K", "Q", "k", "q"

        mens = ""

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
                mens += "  %s : %s" % (color, enr)
            enr = menr(cKR, cQR)
            if enr:
                mens += " %s : %s" % (colorR, enr)
        if cp.alPaso != "-":
            mens += "     %s : %s" % (_("En passant"), cp.alPaso)

        if mens:
            mens = "<b>%s</b><br>" % mens
        mens += info

        mens = "<center>%s</center>" % mens

        return Controles.LB(self, mens)

    def analizar(self, posicion):

        jg = self.liAnalisis[posicion]
        siBlancas = jg.posicionBase.siBlancas
        Analisis.muestraAnalisis(self.procesador, self.xtutor, jg, siBlancas, 9999999, 1, pantalla=self, siGrabar=False)


def pantallaPotencia(procesador):
    w = WPotenciaBase(procesador)
    w.exec_()
