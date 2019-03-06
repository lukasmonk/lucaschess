import os
import sys
import time

import LCEngine4 as LCEngine
from PyQt4 import QtCore, QtGui

import struct
import psutil

from Code import AperturasStd
from Code import AnalisisIndexes
from Code import ControlPosicion
from Code import Configuracion
from Code import Partida
from Code import EngineThread
from Code import Util
from Code import VarGen
from Code import XMotorRespuesta
from Code import Kibitzers
from Code import Books
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import Piezas
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code.QT import Delegados
from Code.QT import PantallaKibitzers

CONFIGURACION = "C"
FEN = "F"
TERMINAR = "T"
COPYCLIPBOARD = "P"


class VentanaPolyglot(QtGui.QDialog):
    def __init__(self, cpu):
        QtGui.QDialog.__init__(self)

        self.cpu = cpu

        dicVideo = self.cpu.dic_video
        if not dicVideo:
            dicVideo = {}

        self.siTop = dicVideo.get("SITOP", True)
        self.siShowTablero = dicVideo.get("SHOW_TABLERO", True)
        self.posicion = ControlPosicion.ControlPosicion()

        self.fen = ""
        self.siPlay = True

        self.setWindowTitle(cpu.titulo)
        self.setWindowIcon(Iconos.Book())

        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint)

        self.setBackgroundRole(QtGui.QPalette.Light)

        configuracion = VarGen.configuracion = cpu.configuracion

        VarGen.todasPiezas = Piezas.TodasPiezas()
        confTablero = cpu.configuracion.confTablero("kib" + cpu.kibitzer.huella, 24)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        Delegados.generaPM(self.tablero.piezas)

        book = Books.Libro("P", cpu.kibitzer.nombre, cpu.kibitzer.exe, True)
        self.book = book
        book.polyglot()
        self.li_moves = []

        self.siFigurines = configuracion.figurinesPGN

        oColumnas = Columnas.ListaColumnas()
        delegado = Delegados.EtiquetaPOS(True, siLineas=False) if self.siFigurines else None
        for x in range(20):
            oColumnas.nueva(x, "", 80, siCentrado=True, edicion = delegado)
        self.grid_moves = Grid.Grid(self, oColumnas, siSelecFilas=True, siCabeceraMovible=False, siCabeceraVisible=False)
        self.grid_moves.tipoLetra(puntos=configuracion.puntosPGN)
        self.grid_moves.ponAltoFila(configuracion.altoFilaPGN)

        liAcciones = (
            (_("Quit"), Iconos.Kibitzer_Terminar(), self.terminar),
            (_("Continue"), Iconos.Kibitzer_Continuar(), self.play),
            (_("Pause"), Iconos.Kibitzer_Pausa(), self.pause),
            (_("Board"), Iconos.Tablero(), self.confTablero),
            ("%s: %s" % (_("Enable"), _("window on top")), Iconos.Top(), self.windowTop),
            ("%s: %s" % (_("Disable"), _("window on top")), Iconos.Bottom(), self.windowBottom),
        )
        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)
        self.tb.setAccionVisible(self.play, False)

        ly1 = Colocacion.H().control(self.tb)
        ly2 = Colocacion.V().otro(ly1).control(self.grid_moves)

        layout = Colocacion.H().control(self.tablero).otro(ly2)
        self.setLayout(layout)

        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), cpu.compruebaInput)
        self.timer.start(200)

        if not self.siShowTablero:
            self.tablero.hide()
        self.recuperarVideo(dicVideo)
        self.ponFlags()

    def ponFen(self, fen):
        self.posicion = ControlPosicion.ControlPosicion()
        self.posicion.leeFen(fen)
        self.fen = fen

        if self.siPlay:
            self.siW = self.posicion.siBlancas
            self.centipawns = self.cpu.configuracion.centipawns

            self.tablero.ponPosicion(self.posicion)
            self.li_moves = self.book.almListaJugadas(fen)
            for alm in self.li_moves:
                alm.nivel = 0
                alm.dato = [""] * 20
                alm.dato[0] = alm.pgn
                alm.dato[1] = alm.porc
                alm.dato[2] = "%d" % alm.weight
            self.grid_moves.gotop()
            self.grid_moves.refresh()
            self.ponFlecha(0)

    def ponFlags(self):
        if self.siTop:
            flags = self.windowFlags() | QtCore.Qt.WindowStaysOnTopHint
        else:
            flags = self.windowFlags() & ~QtCore.Qt.WindowStaysOnTopHint
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | flags)
        self.tb.setAccionVisible(self.windowTop, not self.siTop)
        self.tb.setAccionVisible(self.windowBottom, self.siTop)
        self.show()

    def windowTop(self):
        self.siTop = True
        self.ponFlags()

    def windowBottom(self):
        self.siTop = False
        self.ponFlags()

    def terminar(self):
        self.finalizar()
        self.accept()

    def pause(self):
        self.siPlay = False
        self.tb.setPosVisible(1, True)
        self.tb.setPosVisible(2, False)

    def play(self):
        self.siPlay = True
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.ponFen(self.fen)

    def siAnalizar(self):
        siW = " w " in self.fen
        if not self.siPlay or \
                (siW and (not self.siBlancas)) or \
                ((not siW) and (not self.siNegras)):
            return False
        return True

    def color(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion("blancas", _("White"), Iconos.PuntoNaranja())
        menu.opcion("negras", _("Black"), Iconos.PuntoNegro())
        menu.opcion("blancasnegras", "%s + %s" % (_("White"), _("Black")), Iconos.PuntoVerde())
        resp = menu.lanza()
        if resp:
            self.siNegras = True
            self.siBlancas = True
            if resp == "blancas":
                self.siNegras = False
            elif resp == "negras":
                self.siBlancas = False
            if self.siAnalizar():
                self.ponFen(self.fen)

    def closeEvent(self, event):
        self.finalizar()

    def finalizar(self):
        self.guardarVideo()

    def guardarVideo(self):
        dic = {}

        pos = self.pos()
        dic["_POSICION_"] = "%d,%d" % (pos.x(), pos.y())

        tam = self.size()
        dic["_SIZE_"] = "%d,%d" % (tam.width(), tam.height())

        # dic["SHOW_TABLERO"] = self.siShowTablero
        # dic["NARROWS"] = self.nArrows

        dic["SITOP"] = self.siTop

        self.cpu.save_video(dic)

    def recuperarVideo(self, dicVideo):
        if dicVideo:
            wE, hE = QTUtil.tamEscritorio()
            x, y = dicVideo["_POSICION_"].split(",")
            x = int(x)
            y = int(y)
            if not (0 <= x <= (wE - 50)):
                x = 0
            if not (0 <= y <= (hE - 50)):
                y = 0
            self.move(x, y)
            if "_SIZE_" not in dicVideo:
                w, h = self.width(), self.height()
                for k in dicVideo:
                    if k.startswith("_TAMA"):
                        w, h = dicVideo[k].split(",")
            else:
                w, h = dicVideo["_SIZE_"].split(",")
            w = int(w)
            h = int(h)
            if w > wE:
                w = wE
            elif w < 20:
                w = 20
            if h > hE:
                h = hE
            elif h < 20:
                h = 20
            self.resize(w, h)

    def gridNumDatos(self, grid):
        return len(self.li_moves)

    def gridDato(self, grid, fila, oColumna):
        mv = self.li_moves[fila]
        li = mv.dato
        key = int(oColumna.clave)
        pgn = li[key]
        if self.siFigurines:
            siBlancas = " w " in mv.fen
            return pgn, siBlancas, None, None, None, None, False, True
        else:
            return pgn

    def gridDobleClick(self, grid, fila, oColumna):
        self.lee_subnivel(fila)
        self.grid_moves.refresh()

    def gridBotonDerecho(self, grid, fila, columna, modificadores):
        self.borra_subnivel(fila)
        self.grid_moves.refresh()

    def gridCambiadoRegistro(self, grid, fila, oColumna):
        self.ponFlecha(fila)

    def ponFlecha(self, fila):
        if fila < len(self.li_moves):
            alm = self.li_moves[fila]
            self.tablero.ponFlechaSC(alm.desde, alm.hasta)

    def borra_subnivel(self, fila):
        alm = self.li_moves[fila]
        nv = alm.nivel
        if nv == 0:
            return
        li = []
        for x in range(fila, 0, -1):
            alm1 = self.li_moves[x]
            if alm1.nivel < nv:
                break
            li.append(x)
        for x in range(fila+1, len(self.li_moves)):
            alm1 = self.li_moves[x]
            if alm1.nivel < nv:
                break
            li.append(x)
        li.sort(reverse=True)
        for x in li:
            del self.li_moves[x]

    def lee_subnivel(self, fila):
        alm_base = self.li_moves[fila]
        if alm_base.nivel >= 17:
            return
        LCEngine.setFen(alm_base.fen)
        if LCEngine.movePV(alm_base.desde, alm_base.hasta, alm_base.coronacion):
            fen = LCEngine.getFen()
            for alm in self.book.almListaJugadas(fen):
                nv = alm.nivel = alm_base.nivel + 1
                alm.dato = [""] * 20
                alm.dato[nv] = alm.pgn
                alm.dato[nv+1] = alm.porc
                alm.dato[nv+2] = "%d" % alm.weight
                fila += 1
                self.li_moves.insert(fila, alm)

    def confTablero(self):
        self.pause()
        menu = QTVarios.LCMenu(self)
        if self.siShowTablero:
            menu.opcion("hide", _("Hide"), Iconos.PuntoNaranja())
        else:
            menu.opcion("show", _("Show"), Iconos.PuntoNaranja())
        resp = menu.lanza()
        if resp:
            if resp == "hide":
                self.siShowTablero = False
                self.tablero.hide()
            elif resp == "show":
                self.siShowTablero = True
                self.tablero.show()
            self.guardarVideo()
        self.play()


class VentanaMultiPV(QtGui.QDialog):
    def __init__(self, cpu):
        QtGui.QDialog.__init__(self)

        self.cpu = cpu

        dicVideo = self.cpu.dic_video
        if not dicVideo:
            dicVideo = {}

        self.siTop = dicVideo.get("SITOP", True)
        self.siShowTablero = dicVideo.get("SHOW_TABLERO", True)
        self.nArrows = dicVideo.get("NARROWS", 7)

        self.fen = ""

        self.liData = []
        self.dicLineas = {}

        self.setWindowTitle(cpu.titulo)
        self.setWindowIcon(Iconos.Motor())

        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint)

        self.setBackgroundRole(QtGui.QPalette.Light)

        VarGen.configuracion = cpu.configuracion

        VarGen.todasPiezas = Piezas.TodasPiezas()
        confTablero = cpu.configuracion.confTablero("kib" + cpu.kibitzer.huella, 24)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("BESTMOVE", _("Alternatives"), 80, siCentrado=True)
        oColumnas.nueva("EVALUATION", _("Evaluation"), 85, siCentrado=True)
        oColumnas.nueva("MAINLINE", _("Main line"), 300)
        self.grid = Grid.Grid(self, oColumnas, dicVideo=dicVideo, siSelecFilas=True)

        self.lbDepth = Controles.LB(self)

        liAcciones = (
            (_("Quit"), Iconos.Kibitzer_Terminar(), self.terminar),
            (_("Continue"), Iconos.Kibitzer_Continuar(), self.play),
            (_("Pause"), Iconos.Kibitzer_Pausa(), self.pause),
            (_("The line selected is saved to the clipboard"), Iconos.MoverGrabar(), self.portapapelesJugSelected),
            (_("Analyze only color"), Iconos.P_16c(), self.color),
            (_("Board"), Iconos.Tablero(), self.confTablero),
            ("%s: %s" % (_("Enable"), _("window on top")), Iconos.Top(), self.windowTop),
            ("%s: %s" % (_("Disable"), _("window on top")), Iconos.Bottom(), self.windowBottom),
            ( _("Options"), Iconos.Opciones(), self.changeOptions),
        )
        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)
        self.tb.setAccionVisible(self.play, False)

        ly1 = Colocacion.H().control(self.tb).relleno().control(self.lbDepth)
        ly2 = Colocacion.V().otro(ly1).control(self.grid)

        layout = Colocacion.H().control(self.tablero).otro(ly2)
        self.setLayout(layout)

        self.siPlay = True
        self.siBlancas = True
        self.siNegras = True

        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), cpu.compruebaInput)
        self.timer.start(200)

        if not self.siShowTablero:
            self.tablero.hide()
        self.recuperarVideo(dicVideo)
        self.ponFlags()

        self.motor = None
        self.lanzaMotor()

    def changeOptions(self):
        self.pause()
        w = PantallaKibitzers.WKibitzerLive(self, self.cpu.configuracion, self.cpu.numkibitzer)
        if w.exec_():
            xprioridad = w.result_xprioridad
            if xprioridad is not None:
                pid = self.motor.pid()
                if VarGen.isWindows:
                    hp, ht, pid, dt = struct.unpack("PPII", pid.asstring(16))
                p = psutil.Process(pid)
                p.nice(xprioridad)
            if w.result_posicionBase is not None:
                self.cpu.posicionBase = w.result_posicionBase
                self.fen = self.cpu.fenBase if self.cpu.posicionBase else self.cpu.fen
            if w.result_opciones:
                for opcion, valor in w.result_opciones:
                    if valor is None:
                        orden = "setoption name %s" % opcion
                    else:
                        if type(valor) == bool:
                            valor = str(valor).lower()
                        orden = "setoption name %s value %s" % (opcion, valor)
                    self.ready_ok(orden)
        self.play()
        self.runOrdenes()

    def ponFlags(self):
        if self.siTop:
            flags = self.windowFlags() | QtCore.Qt.WindowStaysOnTopHint
        else:
            flags = self.windowFlags() & ~QtCore.Qt.WindowStaysOnTopHint
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | flags)
        self.tb.setAccionVisible(self.windowTop, not self.siTop)
        self.tb.setAccionVisible(self.windowBottom, self.siTop)
        self.show()

    def windowTop(self):
        self.siTop = True
        self.ponFlags()

    def windowBottom(self):
        self.siTop = False
        self.ponFlags()

    def terminar(self):
        self.finalizar()
        self.accept()

    def pause(self):
        self.siPlay = False
        self.tb.setPosVisible(1, True)
        self.tb.setPosVisible(2, False)
        self.ready_ok("stop")
        self.runOrdenes()

    def play(self):
        self.siPlay = True
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.ponFen(self.fen)

    def gridNumDatos(self, grid):
        return len(self.liData)

    def gridDato(self, grid, fila, oColumna):
        rm = self.liData[fila]
        clave = oColumna.clave
        if clave == "EVALUATION":
            return rm.abrTexto()
        elif clave == "BESTMOVE":
            p = Partida.Partida(fen=self.fen)
            p.leerPV(rm.pv)
            li = p.pgnSP().split(" ")
            resp = ""
            if li:
                if ".." in li[0]:
                    if len(li) > 1:
                        resp = li[1]
                else:
                    resp = li[0].lstrip("1234567890.")
            return resp

        else:
            p = Partida.Partida(fen=self.fen)
            p.leerPV(rm.pv)
            li = p.pgnSP().split(" ")
            if ".." in li[0]:
                li = li[1:]
            return " ".join(li[1:])

    def gridBold(self, grid, fila, oColumna):
        return oColumna.clave in ("EVALUATION", "BESTMOVE")

    def lanzaMotor(self):
        self.motor = QtCore.QProcess()

        self.buffer = ""

        self.liOrdenes = []

        self.esperaOK = None

        self.lock = False

        configMotor = self.cpu.configMotor
        self.nom_engine = configMotor.nombre
        exe = configMotor.ejecutable()
        args = configMotor.argumentos()
        absexe = os.path.abspath(exe)
        direxe = os.path.abspath(os.path.dirname(exe))

        self.motor.setWorkingDirectory(direxe)
        self.motor.start(absexe, args, mode=QtCore.QIODevice.ReadWrite)
        self.motor.waitForStarted()
        self.connect(self.motor, QtCore.SIGNAL("readyReadStandardOutput()"), self.readOutput)

        if self.cpu.prioridad:
            pid = self.motor.pid()
            if VarGen.isWindows:
                hp, ht, pid, dt = struct.unpack("PPII", pid.asstring(16))
            p = psutil.Process(pid)
            p.nice(self.cpu.prioridad)

        self.numMultiPV = configMotor.multiPV
        if self.numMultiPV <= 1:
            self.numMultiPV = min(configMotor.maxMultiPV, 20)

        configMotor.liUCI.append(("MultiPV", str(self.numMultiPV)))

        self.orden_ok("uci", "uciok")

        for opcion, valor in configMotor.liUCI:
            if valor is None:
                orden = "setoption name %s" % opcion
            else:
                if type(valor) == bool:
                    valor = str(valor).lower()
                orden = "setoption name %s value %s" % (opcion, valor)
            self.ready_ok(orden)

        self.runOrdenes()

    def closeEvent(self, event):
        self.finalizar()

    def siAnalizar(self):
        siW = " w " in self.fen
        if not self.siPlay or \
                (siW and (not self.siBlancas)) or \
                ((not siW) and (not self.siNegras)):
            return False
        return True

    def color(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion("blancas", _("White"), Iconos.PuntoNaranja())
        menu.opcion("negras", _("Black"), Iconos.PuntoNegro())
        menu.opcion("blancasnegras", "%s + %s" % (_("White"), _("Black")), Iconos.PuntoVerde())
        resp = menu.lanza()
        if resp:
            self.siNegras = True
            self.siBlancas = True
            if resp == "blancas":
                self.siNegras = False
            elif resp == "negras":
                self.siBlancas = False
            if self.siAnalizar():
                self.ponFen(self.fen)

    def finalizar(self):
        self.guardarVideo()
        if self.motor:
            self.motor.close()
            self.motor = None

    def portapapelesJugSelected(self):
        if self.liData:
            n = self.grid.recno()
            if n < 0 or n >= len(self.liData):
                n = 0
            rm = self.liData[n]
            p = Partida.Partida(fen=self.fen)
            p.leerPV(rm.pv)
            jg0 = p.jugada(0)
            jg0.comentario = rm.abrTextoPDT() + " " + self.nom_engine
            pgn = p.pgnBaseRAW()
            resp = '["FEN", "%s"]\n\n%s' % (self.fen, pgn)
            QTUtil.ponPortapapeles(resp)
            QTUtil2.mensajeTemporal(self, _("The line selected is saved to the clipboard"), 0.7)

    def guardarVideo(self):
        dic = {}

        pos = self.pos()
        dic["_POSICION_"] = "%d,%d" % (pos.x(), pos.y())

        tam = self.size()
        dic["_SIZE_"] = "%d,%d" % (tam.width(), tam.height())

        dic["SHOW_TABLERO"] = self.siShowTablero
        dic["NARROWS"] = self.nArrows

        dic["SITOP"] = self.siTop

        self.grid.guardarVideo(dic)

        self.cpu.save_video(dic)

    def recuperarVideo(self, dicVideo):
        if dicVideo:
            wE, hE = QTUtil.tamEscritorio()
            x, y = dicVideo["_POSICION_"].split(",")
            x = int(x)
            y = int(y)
            if not (0 <= x <= (wE - 50)):
                x = 0
            if not (0 <= y <= (hE - 50)):
                y = 0
            self.move(x, y)
            if "_SIZE_" not in dicVideo:
                w, h = self.width(), self.height()
                for k in dicVideo:
                    if k.startswith("_TAMA"):
                        w, h = dicVideo[k].split(",")
            else:
                w, h = dicVideo["_SIZE_"].split(",")
            w = int(w)
            h = int(h)
            if w > wE:
                w = wE
            elif w < 20:
                w = 20
            if h > hE:
                h = hE
            elif h < 20:
                h = 20
            self.resize(w, h)

    def miraBuffer(self):
        if self.lock:
            return
        self.lock = True

        li = self.buffer.split("\n")
        if self.buffer.endswith("\n"):
            self.buffer = ""
        else:
            self.buffer = li[-1]
            li = li[:-1]

        for linea in li:
            if "multipv" in linea:
                self.mrm.dispatch(linea)
                self.si_read_multipv = True
            else:
                if self.si_read_multipv:
                    self.si_read_multipv = False
                    self.mrm.ordena()
                    self.liData = self.mrm.liMultiPV
                    if len(self.liData) == 0:
                        continue
                    rm = self.mrm.liMultiPV[0]
                    self.lbDepth.ponTexto("%s: %d" % (_("Depth"), rm.depth))
                    partida = Partida.Partida(fen=self.fen)
                    partida.leerPV(rm.pv)
                    if partida.numJugadas():
                        self.tablero.quitaFlechas()
                        jg0 = partida.jugada(0)
                        self.tablero.ponPosicion(jg0.posicion)
                        tipo = "mt"
                        opacidad = 100
                        salto = (80 - 15) * 2 / (self.nArrows - 1) if self.nArrows > 1 else 1
                        cambio = max(30, salto)

                        for njg in range(min(partida.numJugadas(), self.nArrows)):
                            tipo = "ms" if tipo == "mt" else "mt"
                            jg = partida.jugada(njg)
                            self.tablero.creaFlechaMov(jg.desde, jg.hasta, tipo + str(opacidad))
                            if njg % 2 == 1:
                                opacidad -= cambio
                                cambio = salto

                    self.grid.refresh()
                    self.mrm = XMotorRespuesta.MRespuestaMotor(self.cpu.titulo, self.siW)

        self.lock = False
        QTUtil.refreshGUI()

    def readOutput(self):
        if not self.lock:
            txt = self.motor.readAllStandardOutput()
            if txt:
                self.buffer += str(txt)
                if self.esperaOK:
                    if self.esperaOK in self.buffer and self.buffer.endswith("\n"):
                        self.buffer = ""
                else:
                    self.miraBuffer()
            self.runOrdenes()
        else:
            time.sleep(0.1)

    def orden_ok(self, orden, ok=None):
        self.liOrdenes.append((orden, ok))

    def ready_ok(self, orden):
        self.orden_ok(orden)
        self.orden_ok("isready", "readyok")

    def orden_fen(self, fen):
        posicionInicial = ControlPosicion.ControlPosicion()
        posicionInicial.leeFen(fen)

        self.siW = posicionInicial.siBlancas
        self.centipawns = self.cpu.configuracion.centipawns
        self.mrm = XMotorRespuesta.MRespuestaMotor(self.cpu.titulo, self.siW)
        self.si_read_multipv = False

        self.tablero.ponPosicion(posicionInicial)

        self.ready_ok("stop")

        self.ready_ok("ucinewgame")
        self.ready_ok("position fen %s" % fen)

        self.orden_ok("go infinite", None)

    def runOrdenes(self):
        if self.liOrdenes:
            self.siOrden = True
            orden, ok = self.liOrdenes[0]
            del self.liOrdenes[0]
            self.escribe(orden)
            if ok:
                self.esperaOK = ok
            else:
                self.esperaOK = None
                self.runOrdenes()
        else:
            self.esperaOK = None

    def escribe(self, linea):
        self.motor.write(str(linea) + "\n")

    def confTablero(self):
        self.pause()
        menu = QTVarios.LCMenu(self)
        if self.siShowTablero:
            menu.opcion("hide", _("Hide"), Iconos.PuntoNaranja())
        else:
            menu.opcion("show", _("Show"), Iconos.PuntoNaranja())
        menu.separador()
        menu1 = menu.submenu(_("Arrows"), Iconos.PuntoNegro())
        for x in range(1, 31):
            menu1.opcion(x, str(x), Iconos.PuntoVerde(), siDeshabilitado=x == self.nArrows)
        resp = menu.lanza()
        if resp:
            if resp == "hide":
                self.siShowTablero = False
                self.tablero.hide()
            elif resp == "show":
                self.siShowTablero = True
                self.tablero.show()
            else:
                self.nArrows = resp
            self.guardarVideo()
        self.play()

    def ponFen(self, fen):
        self.dicLineas = {}
        self.liData = []
        self.lbDepth.ponTexto("-")
        if fen:
            self.fen = fen
            self.numMovesDepth = min(self.numMultiPV, LCEngine.setFen(fen))
            if self.siAnalizar():
                self.orden_fen(fen)
                self.runOrdenes()
            else:
                self.ready_ok("stop")
                self.runOrdenes()

        else:
            self.ready_ok("stop")
            self.runOrdenes()
        self.grid.refresh()


class Ventana(QtGui.QDialog):
    def __init__(self, cpu, siWidgets=True, flags=None):
        QtGui.QDialog.__init__(self)

        self.cpu = cpu
        self.motor = None

        self.liData = []
        self.almFEN = {}

        self.fen = ""

        self.setWindowTitle(cpu.titulo)
        self.setWindowIcon(Iconos.Motor())

        if not flags:
            flags = QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint | QtCore.Qt.WindowMaximizeButtonHint | QtCore.Qt.WindowStaysOnTopHint
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | flags)

        self.setBackgroundRole(QtGui.QPalette.Light)
        self.setStyleSheet("QTextEdit { background-color: rgb( 250,250,250); }")

        VarGen.configuracion = cpu.configuracion
        VarGen.todasPiezas = Piezas.TodasPiezas()
        confTablero = cpu.configuracion.confTablero("kib" + cpu.kibitzer.huella, 24)

        self.siWidgets = siWidgets
        if siWidgets:
            self.tablero = Tablero.Tablero(self, confTablero)
            self.tablero.crea()

        self.siShowTablero = siWidgets
        self.nArrows = 7

        self.siTop = True

        if siWidgets:
            self.em = Controles.EM(self).soloLectura()
            liAcciones = (
                (_("Quit"), Iconos.Kibitzer_Terminar(), self.terminar),
                (_("Continue"), Iconos.Kibitzer_Continuar(), self.play),
                (_("Pause"), Iconos.Kibitzer_Pausa(), self.pause),
                (_("The best solution found by the engine is saved to the clipboard"), Iconos.MoverGrabar(), self.portapapelesUltJug),
                (_("Analyze only color"), Iconos.P_16c(), self.color),
                (_("Board"), Iconos.Tablero(), self.confTablero),
                ("%s: %s" % (_("Enable"), _("window on top")), Iconos.Top(), self.windowTop),
                ("%s: %s" % (_("Disable"), _("window on top")), Iconos.Bottom(), self.windowBottom),
                ( _("Options"), Iconos.Opciones(), self.changeOptions),
            )
            self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)

            self.layoutDT = Colocacion.H().control(self.tablero).control(self.em)

        self.siPlay = True
        self.siBlancas = True
        self.siNegras = True

        self.creaRestoControles()

        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), cpu.compruebaInput)
        self.timer.start(200)

        self.recuperarVideo()
        self.ponFlags()

    def pause(self):
        pass

    def play(self):
        pass

    def changeOptions(self):
        self.pause()
        w = PantallaKibitzers.WKibitzerLive(self, self.cpu.configuracion, self.cpu.numkibitzer)
        if w.exec_():
            xprioridad = w.result_xprioridad
            if xprioridad is not None:
                pid = self.motor.pid()
                if VarGen.isWindows:
                    hp, ht, pid, dt = struct.unpack("PPII", pid.asstring(16))
                p = psutil.Process(pid)
                p.nice(xprioridad)
            if w.result_posicionBase is not None:
                self.cpu.posicionBase = w.result_posicionBase
                self.fen = self.cpu.fenBase if self.cpu.posicionBase else self.cpu.fen
            if w.result_opciones:
                for opcion, valor in w.result_opciones:
                    if valor is None:
                        orden = "setoption name %s" % opcion
                    else:
                        if type(valor) == bool:
                            valor = str(valor).lower()
                        orden = "setoption name %s value %s" % (opcion, valor)
                    self.ready_ok(orden)
        self.play()
        self.runOrdenes()

    def ponFlags(self):
        if self.siTop:
            flags = self.windowFlags() | QtCore.Qt.WindowStaysOnTopHint
        else:
            flags = self.windowFlags() & ~QtCore.Qt.WindowStaysOnTopHint
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | flags)
        self.tb.setAccionVisible(self.windowTop, not self.siTop)
        self.tb.setAccionVisible(self.windowBottom, self.siTop)
        self.show()

    def windowTop(self):
        self.siTop = True
        self.ponFlags()

    def windowBottom(self):
        self.siTop = False
        self.ponFlags()

    def closeEvent(self, event):
        self.finalizar()

    def tableroPosicion(self, posicion):
        if self.siWidgets:
            self.tablero.ponPosicion(posicion)

    def siAnalizar(self):
        siW = " w " in self.fen
        if not self.siPlay or \
                (siW and (not self.siBlancas)) or \
                ((not siW) and (not self.siNegras)):
            return False
        return True

    def color(self):
        menu = QTVarios.LCMenu(self)
        menu.opcion("blancas", _("White"), Iconos.PuntoNaranja())
        menu.opcion("negras", _("Black"), Iconos.PuntoNegro())
        menu.opcion("blancasnegras", "%s + %s" % (_("White"), _("Black")), Iconos.PuntoVerde())
        resp = menu.lanza()
        if resp:
            self.siNegras = True
            self.siBlancas = True
            if resp == "blancas":
                self.siNegras = False
            elif resp == "negras":
                self.siBlancas = False
            if self.siAnalizar():
                self.ponFen(self.fen)

    def finalizar(self):
        self.guardarVideo()
        if self.motor:
            self.escribe("quit")
            self.motor.close()
            self.motor = None

    def comentario(self, una):
        puntuacion = una["score"].replace("cp", "").strip()
        depth = una["depth"].strip()
        return " {%s D%s} " % (puntuacion, depth)

    def portapapelesUltJug(self):
        if self.liData and self.siAnalizar():
            una = self.liData[-1]
            pgn = una["pgn"]
            resp = '["FEN", "%s"]\n\n%s\n' % (self.fen, pgn)
            QTUtil.ponPortapapeles(resp)
            # li = pgn.split(" ")
            # n = 2 if "..." in pgn else 1
            # resp = " ".join(li[0:n])
            # # resp += self.comentario(una)
            # if len(li) > n:
            #     resp += " ".join(li[n:])

    def guardarVideo(self):
        dic = {}

        pos = self.pos()
        dic["_POSICION_"] = "%d,%d" % (pos.x(), pos.y())

        tam = self.size()
        dic["_SIZE_"] = "%d,%d" % (tam.width(), tam.height())

        dic["SHOW_TABLERO"] = self.siShowTablero
        dic["NARROWS"] = self.nArrows

        dic["SITOP"] = self.siTop

        self.cpu.save_video(dic)

    def recuperarVideo(self):
        dic = self.cpu.dic_video
        if dic:
            wE, hE = QTUtil.tamEscritorio()
            x, y = dic["_POSICION_"].split(",")
            x = int(x)
            y = int(y)
            if not (0 <= x <= (wE - 50)):
                x = 0
            if not (0 <= y <= (hE - 50)):
                y = 0
            self.move(x, y)
            if "_SIZE_" not in dic:
                w, h = self.width(), self.height()
                for k in dic:
                    if k.startswith("_TAMA"):
                        w, h = dic[k].split(",")
            else:
                w, h = dic["_SIZE_"].split(",")
            w = int(w)
            h = int(h)
            if w > wE:
                w = wE
            elif w < 20:
                w = 20
            if h > hE:
                h = hE
            elif h < 20:
                h = 20
            self.resize(w, h)
            self.siShowTablero = dic.get("SHOW_TABLERO", self.siShowTablero)
            self.nArrows = dic.get("NARROWS", self.nArrows)
            self.siTop = dic.get("SITOP", self.siTop)

            if not self.siShowTablero:
                if self.siWidgets:
                    self.tablero.hide()

    def lanzaMotor(self, siMultiPV=False):
        self.motor = QtCore.QProcess()

        self.buffer = ""

        self.liOrdenes = []

        self.esperaOK = None

        self.lock = False

        configMotor = self.cpu.configMotor
        exe = configMotor.ejecutable()
        args = configMotor.argumentos()

        self.motor.setWorkingDirectory(os.path.abspath(os.path.dirname(exe)))
        self.motor.start(exe, args, mode=QtCore.QIODevice.Unbuffered | QtCore.QIODevice.ReadWrite)
        self.motor.waitForStarted()
        self.connect(self.motor, QtCore.SIGNAL("readyReadStandardOutput()"), self.readOutput)
        if self.cpu.prioridad:
            pid = self.motor.pid()
            if VarGen.isWindows:
                hp, ht, pid, dt = struct.unpack("PPII", pid.asstring(16))
            p = psutil.Process(pid)
            p.nice(self.cpu.prioridad)

        if siMultiPV and configMotor.maxMultiPV:
            configMotor.liUCI.append(("MultiPV", str(configMotor.maxMultiPV)))

        self.orden_ok("uci", "uciok")

        for opcion, valor in configMotor.liUCI:
            if valor is None:
                orden = "setoption name %s" % opcion
            else:
                if opcion.upper() == "MULTIPV" and not siMultiPV:
                    continue
                if type(valor) == bool:
                    valor = str(valor).lower()
                orden = "setoption name %s value %s" % (opcion, valor)
            self.ready_ok(orden)

        self.runOrdenes()

    def miraBuffer(self):
        self.lock = True
        li = self.buffer.split("\n")

        siInfo = False

        for linea in li:
            if linea.startswith("info ") and " pv " in linea and " score " in linea:
                dClaves = self.miraClaves(linea, (
                    "multipv", "depth", "seldepth", "score", "time", "nodes", "pv", "hashfull", "tbhits", "nps"))
                pv = dClaves["pv"]
                if not pv:
                    continue
                dClaves["pgn"] = self.pgn(dClaves["pv"])

                score = dClaves["score"].strip()
                if score.startswith("cp "):
                    dClaves["puntos"] = int(score.split(" ")[1])
                    dClaves["mate"] = 0
                elif score.startswith("mate "):
                    dClaves["puntos"] = 0
                    dClaves["mate"] = int(score.split(" ")[1])

                self.liData.append(dClaves)
                siInfo = True
        if not siInfo:
            if "bestmove" in self.buffer:
                for linea in li:
                    if linea.startswith("bestmove "):
                        self.em.ponHtml(linea)
                        self.buffer = ""
                        self.lock = False
                        return

        resto = li[-1]
        if self.buffer.endswith("\n"):
            resto += "\n"
        self.buffer = resto

        if siInfo:
            txt = '<table border="1" cellpadding="4" cellspacing="0" style="border-color: #ACA899">'
            ndata = len(self.liData)

            if self.partida.numJugadas():
                self.tablero.quitaFlechas()
                jg0 = self.partida.jugada(0)
                self.tablero.ponPosicion(jg0.posicion)
                tipo = "mt"
                opacidad = 100
                salto = (80 - 15) * 2 / (self.nArrows - 1) if self.nArrows > 1 else 1
                cambio = max(30, salto)

                for njg in range(min(self.partida.numJugadas(), self.nArrows)):
                    tipo = "ms" if tipo == "mt" else "mt"
                    jg = self.partida.jugada(njg)
                    self.tablero.creaFlechaMov(jg.desde, jg.hasta, tipo + str(opacidad))
                    if njg % 2 == 1:
                        opacidad -= cambio
                        cambio = salto

            if ndata:
                txt += "<tr>"
                txt += "<th>%s</th>" % _("Depth")
                txt += '<th><center>%s</center></th>' % _("Evaluation")
                txt += '<th><center>%s</center></th>' % _("Best move")
                txt += '<th><center>%s</center></th>' % _("Main line")
                txt += "</tr>"
                for n in range(ndata - 1, -1, -1):
                    if len(self.liData) <= n:
                        break
                    una = self.liData[n]
                    if una:
                        txt += "<tr>"
                        txt += '<td align="center">%s</td>' % una["depth"]

                        txt += '<td align="center"><b>%s</b></td>' % self.confTextoBase(una)
                        li = una["pgn"].split(" ")
                        if "..." in li[0]:
                            del li[0]
                        nli = len(li)
                        if nli:
                            txt += '<td align="center"><b>%s</b></td>' % li[0]
                        if nli > 1:
                            txt += "<td>%s</td>" % (" ".join(li[1:]),)
                        txt += "</tr>"

            txt += "</table>"
            if self.siAnalizar():
                self.em.ponHtml(txt)

        self.lock = False

    def pgn(self, pv):
        self.partida.liJugadas = []
        self.partida.ultPosicion = self.partida.iniPosicion.copia()
        self.partida.leerPV(pv)

        return self.partida.pgnSP()

    def miraClaves(self, mensaje, liClaves):
        dClaves = {}
        clave = ""
        dato = ""
        for palabra in mensaje.split(" "):
            if palabra in liClaves:
                if clave:
                    dClaves[clave] = dato.strip()
                clave = palabra
                dato = ""
            else:
                dato += " " + palabra
        if clave:
            dClaves[clave] = dato.strip()
        return dClaves

    def readOutput(self):
        if not self.lock:
            txt = self.motor.readAllStandardOutput()
            if txt:
                self.buffer += str(txt)
                if self.esperaOK:
                    if self.esperaOK in self.buffer and self.buffer.endswith("\n"):
                        self.buffer = ""
                else:
                    self.miraBuffer()
            self.runOrdenes()
        # else:
        #     QtCore.QTimer.singleShot(100, self.readOutput)
            # time.sleep(0.1)

    def orden_ok(self, orden, ok=None):
        self.liOrdenes.append((orden, ok))

    def ready_ok(self, orden):
        self.orden_ok(orden)
        self.orden_ok("isready", "readyok")

    def orden_fen(self, fen):
        posicionInicial = ControlPosicion.ControlPosicion()
        posicionInicial.leeFen(fen)

        self.siW = posicionInicial.siBlancas
        self.centipawns = self.cpu.configuracion.centipawns

        self.partida = Partida.Partida(posicionInicial)

        self.tableroPosicion(posicionInicial)

        self.ready_ok("stop")

        self.ready_ok("ucinewgame")
        self.ready_ok("position fen %s" % fen)

        self.orden_ok("go infinite", None)

    def orden_eval(self, fen):
        posicionInicial = ControlPosicion.ControlPosicion()
        posicionInicial.leeFen(fen)

        self.siW = posicionInicial.siBlancas
        self.centipawns = self.cpu.configuracion.centipawns

        self.partida = Partida.Partida(posicionInicial)

        self.tableroPosicion(posicionInicial)

        self.ready_ok("stop")

        self.ready_ok("ucinewgame")
        self.ready_ok("position fen %s" % fen)

        self.orden_ok("eval", None)

    def confTextoBase(self, una):
        mt = una["mate"]
        pts = una["puntos"]
        if mt:
            if not self.siW:
                mt = -mt
            return "M%+d" % mt
        else:
            if not self.siW:
                pts = -pts
            if self.centipawns:
                return "%d" % pts
            else:
                return "%+0.2f" % float(pts / 100.0)

    def runOrdenes(self):
        if self.liOrdenes:
            self.siOrden = True
            orden, ok = self.liOrdenes[0]
            del self.liOrdenes[0]
            self.escribe(orden)
            if ok:
                self.esperaOK = ok
            else:
                self.esperaOK = None
                self.runOrdenes()
        else:
            self.esperaOK = None

    def escribe(self, linea):
        self.motor.write(str(linea) + "\n")

    def confTablero(self):
        self.pause()
        menu = QTVarios.LCMenu(self)
        if self.siShowTablero:
            menu.opcion("hide", _("Hide"), Iconos.PuntoNaranja())
        else:
            menu.opcion("show", _("Show"), Iconos.PuntoNaranja())
        menu.separador()
        menu1 = menu.submenu(_("Arrows"), Iconos.PuntoNegro())
        for x in range(1, 31):
            menu1.opcion(x, str(x), Iconos.PuntoVerde(), siDeshabilitado=x == self.nArrows)
        resp = menu.lanza()
        if resp:
            if resp == "hide":
                self.siShowTablero = False
                self.tablero.hide()
            elif resp == "show":
                self.siShowTablero = True
                self.tablero.show()
            else:
                self.nArrows = resp
            self.guardarVideo()
        self.play()


class VentanaSiguiente(Ventana):
    def creaRestoControles(self):
        layout = Colocacion.V().control(self.tb).otro(self.layoutDT).margen(1)

        self.setLayout(layout)

        self.lanzaMotor()
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.siPlay = True
        self.liData = []

    def pause(self):
        self.siPlay = False
        self.liData = []
        self.tb.setPosVisible(1, True)
        self.tb.setPosVisible(2, False)
        self.ready_ok("stop")
        self.runOrdenes()

    def play(self):
        self.siPlay = True
        self.liData = []
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.ponFen(self.fen)

    def terminar(self):
        self.finalizar()
        self.accept()

    def ponFen(self, fen):
        if fen:

            if fen != self.fen:
                html = self.em.html()
                if self.fen and html:
                    self.almFEN[self.fen] = html
                self.liData = []

            self.fen = fen
            if self.siAnalizar():
                if len(self.liData) == 0:
                    self.em.ponHtml("")
                    self.orden_fen(fen)
                    self.runOrdenes()
            else:
                html = self.almFEN.get(fen, "")
                self.em.ponHtml(html)
                self.ready_ok("stop")
                self.runOrdenes()

        else:
            self.em.ponHtml("")
            self.ready_ok("stop")
            self.runOrdenes()


class VentanaJugadas(Ventana):
    def creaRestoControles(self):

        self.lbJug = Controles.LB(self).alinCentrado()

        self.emJug = Controles.EM(self).capturaDobleClick(self.miraMovimiento).soloLectura()

        layout = Colocacion.V().control(self.tb).control(self.emJug).control(self.lbJug).otro(self.layoutDT).margen(1)

        self.setLayout(layout)

        self.lifens = {}

        self.siMotorTrabajando = False
        self.em.hide()
        self.tablero.hide()
        self.lbJug.hide()
        self.siPrimeraVez = True

    def finalizar(self):
        self.paraMotorJugada()
        Ventana.finalizar(self)

    def portapapelesUltJug(self):
        # if self.liData and self.siAnalizar():
        if self.liData:
            una = self.liData[-1]
            pgn = una["pgn"]
            resp = '["FEN", "%s"]\n\n%s' % (self.fen, pgn)
            QTUtil.ponPortapapeles(resp)
            # if "..." in pgn:
            #     pgn = pgn.lstrip("0123456789. ")
            # li = pgn.split(" ")
            # resp = self.pgn1 + " " + li[0]
            # # resp += self.comentario(una)
            # if len(li) > 1:
            #     resp += " ".join(li[1:])

    def paraMotorJugada(self):
        self.lbJug.hide()
        self.em.hide()
        self.tablero.hide()

        # Si el motor esta trabajando -> stop
        if self.siMotorTrabajando:
            self.ready_ok("stop")
            self.runOrdenes()
            self.siMotorTrabajando = False

    def lanzaMotorJugada(self, pgn, fen):
        self.em.ponHtml("")
        if self.siAnalizar():
            self.lbJug.ponTexto("<b>%s<b>" % pgn)
            self.lbJug.show()
            self.em.show()
            self.tablero.show()
            # Si no hay motor, se lanza
            if self.motor is None:
                self.lanzaMotor()
            self.siMotorTrabajando = True
            self.liData = []
            self.orden_fen(fen)
            self.runOrdenes()

    def miraMovimiento(self, event):
        pos = self.emJug.posicion()
        txt = self.emJug.texto()

        ini = pos - 1
        while ini >= 0:
            c = txt[ini]
            if c in "\n ,":
                ini += 1
                break
            ini -= 1
        fin = pos
        ntxt = len(txt)
        while fin < ntxt:
            c = txt[fin]
            if c in "\n ,":
                break
            fin += 1

        pgn = txt[ini:fin]
        fen, pgn1 = self.dfens.get(pgn, (None, None))

        if fen is None:
            self.paraMotorJugada()

        else:
            self.pgn1 = pgn1
            self.lanzaMotorJugada(pgn, fen)

    def pause(self):
        self.siPlay = False
        self.paraMotorJugada()
        self.liData = []
        self.tb.setPosVisible(1, True)
        self.tb.setPosVisible(2, False)

    def play(self):
        self.siPlay = True
        self.liData = []
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.ponFen(self.fen)

    def terminar(self):
        self.accept()

    def ponFen(self, fen):
        if fen:
            if fen != self.fen:
                html = self.emJug.html()
                if self.fen and html:
                    self.almFEN[self.fen] = html
                self.em.ponHtml("")
                self.emJug.ponHtml("")
                self.liData = []

            self.fen = fen
            if self.siAnalizar():
                self.ponJugadas()
            else:
                html = self.almFEN.get(fen, "")
                self.emJug.ponHtml(html)
                self.ready_ok("stop")
                self.runOrdenes()
        else:
            self.paraMotorJugada()
            self.em.ponHtml("")
            self.emJug.ponHtml("")
            self.liData = []

    def pgn_fen(self, pv):
        pgn = self.pgn(pv)
        fen = self.partida.ultPosicion.fen()
        return pgn, fen

    def ponJugadas(self):

        self.paraMotorJugada()

        if self.siPrimeraVez:
            self.siPrimeraVez = False
            self.lbJug.ponTexto(_("Double click on any move to analyze it"))
            self.lbJug.show()

        posicionInicial = ControlPosicion.ControlPosicion()
        posicionInicial.leeFen(self.fen)

        self.dfens = {}

        self.partida = Partida.Partida(posicionInicial)

        self.tableroPosicion(posicionInicial)

        LCEngine.setFen(self.fen)
        liMovs = LCEngine.getMoves()
        jaqs = ""
        caps = ""
        rest = ""
        for xpv in liMovs:
            pv = xpv[1:]
            pgn1, fen = self.pgn_fen(pv)
            pgn = pgn1.lstrip("0123456789. ")
            self.dfens[pgn] = fen, pgn1
            pgn += ", "
            si = False
            if "x" in pgn:
                caps += pgn
                si = True
            if ("+" in pgn) or ("#" in pgn):
                jaqs += pgn
                si = True
            if not si:
                rest += pgn

        txt = '<table border="1" cellpadding="4" cellspacing="0" style="border-color: #ACA899">'

        txt += "<tr><th>%s</th><td>%s</td></tr>" % (_("Checks"), jaqs.rstrip(" ,"))
        txt += "<tr><th>%s</th><td>%s</td></tr>" % (_("Captured material"), caps.rstrip(" ,"))
        txt += "<tr><th>%s</th><td>%s</td></tr>" % (_("Rest"), rest.rstrip(" ,"))

        txt += "</table>"
        # txt += "<small>%s</small>"%_("Double click on any move to analyze it")
        self.emJug.ponHtml(txt)
        self.em.ponHtml("")


class VentanaIndices(Ventana):
    def __init__(self, cpu):
        Ventana.__init__(self, cpu, False)

    def creaRestoControles(self):

        liAcciones = (
            (_("Quit"), Iconos.Kibitzer_Terminar(), self.terminar),
            (_("Continue"), Iconos.Kibitzer_Continuar(), self.play),
            (_("Pause"), Iconos.Kibitzer_Pausa(), self.pause),
            (_("Analyze only color"), Iconos.P_16c(), self.color),
            ("%s: %s" % (_("Enable"), _("window on top")), Iconos.Top(), self.windowTop),
            ("%s: %s" % (_("Disable"), _("window on top")), Iconos.Bottom(), self.windowBottom),
        )
        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)
        self.tb.setAccionVisible(self.play, False)
        self.tb.setAccionVisible(self.windowTop, False)
        self.em = Controles.EM(self).soloLectura()

        layout = Colocacion.V().control(self.tb).control(self.em).margen(1)

        self.setLayout(layout)

        self.lanzaMotor(False)

    def ponFen(self, fen):
        if fen:

            if fen != self.fen:
                html = self.em.html()
                if self.fen and html:
                    self.almFEN[self.fen] = html
                self.liData = []

            self.fen = fen
            if self.siAnalizar():
                if len(self.liData) == 0:
                    self.em.ponHtml("")
                    self.orden_fen(fen)
                    self.runOrdenes()
            else:
                html = self.almFEN.get(fen, "")
                self.em.ponHtml(html)
                self.ready_ok("stop")
                self.runOrdenes()

        else:
            self.em.ponHtml("")
            self.ready_ok("stop")
            self.runOrdenes()

    def pause(self):
        self.siPlay = False
        self.liData = []
        self.tb.setPosVisible(1, True)
        self.tb.setPosVisible(2, False)
        self.ready_ok("stop")
        self.runOrdenes()

    def play(self):
        self.siPlay = True
        self.liData = []
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.ponFen(self.fen)

    def terminar(self):
        self.finalizar()
        self.accept()

    def miraBuffer(self):
        if not self.siPlay:
            return
        self.lock = True

        li = self.buffer.split("\n")
        if self.buffer.endswith("\n"):
            self.buffer = ""
        else:
            self.buffer = li[-1]
            li = li[:-1]
        mrm = XMotorRespuesta.MRespuestaMotor("", " w " in self.fen)
        mrm.dispatch("\n".join(li))
        mrm.ordena()
        mrm.maxTiempo = None
        mrm.maxProfundidad = 9999

        if mrm.liMultiPV:

            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.fen)

            litxt = ['<table border="1" cellpadding="4" cellspacing="0" style="border-color: #ACA899">', ]

            def tr(tp, mas=""):
                litxt.append('<tr><th>%s</th><td align="right">%.01f%%</td><td>%s%s</td></tr>' % (tp[0], tp[1], mas, tp[2]))

            tp = AnalisisIndexes.tp_gamestage(cp, mrm)
            litxt.append('<tr><th>%s</th><td align="right">%d</td><td>%s</td></tr>' % (tp[0], tp[1], tp[2]))

            pts = mrm.liMultiPV[0].puntosABS()
            if pts:
                w, b = _("White"), _("Black")
                siW = "w" in self.fen
                if pts > 0:
                    mas = w if siW else b
                elif pts < 0:
                    mas = b if siW else w
                mas += "-"
            else:
                mas = ""

            tr(AnalisisIndexes.tp_winprobability(cp, mrm), mas)
            tr(AnalisisIndexes.tp_complexity(cp, mrm))
            tr(AnalisisIndexes.tp_efficientmobility(cp, mrm))

            tr(AnalisisIndexes.tp_narrowness(cp, mrm))
            tr(AnalisisIndexes.tp_piecesactivity(cp, mrm))
            tr(AnalisisIndexes.tp_exchangetendency(cp, mrm))

            tp = AnalisisIndexes.tp_positionalpressure(cp, mrm)
            litxt.append('<tr><th>%s</th><td align="right">%dcp</td><td></td></tr>' % (tp[0], int(tp[1])))

            tr(AnalisisIndexes.tp_materialasymmetry(cp, mrm))

            # tr(AnalisisIndexes.tp_test1(cp, mrm))
            # tr(AnalisisIndexes.tp_test2(cp, mrm))
            # tr(AnalisisIndexes.tp_test3(cp, mrm))
            # tr(AnalisisIndexes.tp_test4(cp, mrm))
            # tr(AnalisisIndexes.tp_test5(cp, mrm))

            litxt.append("</table>")

            self.em.ponHtml("".join(litxt))

        self.lock = False


class EDP(Controles.ED):
    def ponHtml(self, txt):
        self.setText(txt)
        self.setCursorPosition(0)
        return self

    def html(self):
        return self.text()


class VentanaLinea(Ventana):
    def __init__(self, cpu):
        self.flags = {True: QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowStaysOnTopHint,
                      False: QtCore.Qt.Window | QtCore.Qt.FramelessWindowHint | QtCore.Qt.WindowStaysOnTopHint}
        Ventana.__init__(self, cpu, False, flags=self.flags[False])
        self.siMover = False
        self.setStyleSheet("""QLineEdit {
    color: rgb(127, 0, 63);
    selection-color: white;
    border: 1px groove gray;
    border-radius: 2px;
    padding: 2px 4px;
}""")

    def creaRestoControles(self):
        liAcciones = (
            (_("Quit"), Iconos.Kibitzer_Terminar(), self.terminar),
            (_("Continue"), Iconos.Kibitzer_Continuar(), self.play),
            (_("Pause"), Iconos.Kibitzer_Pausa(), self.pause),
            (_("Analyze only color"), Iconos.P_16c(), self.color),
            (_("Change window position"), Iconos.TamTablero(), self.mover),
            (_("Options"), Iconos.Opciones(), self.changeOptions),
        )
        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)
        self.tb.setFixedSize(120, 24)
        self.tb.setPosVisible(1, False)
        self.em = EDP(self)
        self.em.ponTipoLetra(peso=75, puntos=10)
        self.em.setReadOnly(True)

        layout = Colocacion.H().control(self.em).control(self.tb).margen(2)

        self.setLayout(layout)

        self.lanzaMotor(False)

    def terminar(self):
        self.finalizar()
        self.accept()

    def mover(self):
        # if self.siMover:
        #     self.guardarVideo()
        self.siMover = not self.siMover
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | self.flags[self.siMover])
        self.show()
        self.guardarVideo()
        # self.recuperarVideo()
        QTUtil.refreshGUI()

    def pause(self):
        self.siPlay = False
        self.liData = []
        self.tb.setPosVisible(1, True)
        self.tb.setPosVisible(2, False)
        self.ready_ok("stop")
        self.runOrdenes()

    def play(self):
        self.siPlay = True
        self.liData = []
        self.tb.setPosVisible(1, False)
        self.tb.setPosVisible(2, True)
        self.ponFen(self.fen)

    def ponFen(self, fen):
        if fen:
            if fen != self.fen:
                html = self.em.html()
                if self.fen and html:
                    self.almFEN[self.fen] = html
                self.liData = []

            self.fen = fen
            if self.siAnalizar():
                if len(self.liData) == 0:
                    self.em.ponHtml("")
                    self.orden_fen(fen)
                    self.runOrdenes()
            else:
                html = self.almFEN.get(fen, "")
                self.em.ponHtml(html)
                self.ready_ok("stop")
                self.runOrdenes()

        else:
            self.em.ponHtml("")
            self.ready_ok("stop")
            self.runOrdenes()

    def miraBuffer(self):
        self.lock = True

        li = self.buffer.split("\n")
        if self.buffer.endswith("\n"):
            self.buffer = ""
        else:
            self.buffer = li[-1]
            li = li[:-1]
        mrm = XMotorRespuesta.MRespuestaMotor("", " w " in self.fen)
        mrm.dispatch("\n".join(li))
        mrm.maxTiempo = None
        mrm.maxProfundidad = 9999
        mrm.ordena()

        if mrm.liMultiPV:
            rm = mrm.liMultiPV[0]
            p = Partida.Partida(fen=self.fen)
            p.leerPV(rm.pv)
            li = p.pgnSP().split(" ")
            if len(li) > 20:
                li = li[:20]

            self.em.ponHtml("[%02d] %s | %s" % (rm.depth, rm.abrTexto(), " ".join(li)))

        self.lock = False

    def finalizar(self):
        if self.siMover:
            self.guardarVideo()
        if self.motor:
            self.motor.close()
            self.motor = None


class VentanaStockfishEval(Ventana):
    def __init__(self, cpu):
        Ventana.__init__(self, cpu, False)

    def creaRestoControles(self):

        liAcciones = (
            (_("Quit"), Iconos.Kibitzer_Terminar(), self.terminar),
            ("%s: %s" % (_("Enable"), _("window on top")), Iconos.Top(), self.windowTop),
            ("%s: %s" % (_("Disable"), _("window on top")), Iconos.Bottom(), self.windowBottom),
        )
        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)
        self.tb.setAccionVisible(self.windowTop, False)
        self.em = Controles.EM(self).soloLectura()
        f = Controles.TipoLetra(nombre="Courier New", puntos=10)
        self.em.ponFuente(f)

        layout = Colocacion.V().control(self.tb).control(self.em).margen(1)

        self.setLayout(layout)

        self.lanzaMotor(True)

    def ponFen(self, fen):
        if fen:

            if fen != self.fen:
                html = self.em.html()
                if self.fen and html:
                    self.almFEN[self.fen] = html
                self.liData = []

            self.fen = fen
            if self.siAnalizar():
                if len(self.liData) == 0:
                    self.em.ponHtml("")
                    self.orden_eval(fen)
                    self.runOrdenes()
            else:
                html = self.almFEN.get(fen, "")
                self.em.ponHtml(html)
                self.ready_ok("stop")
                self.runOrdenes()

        else:
            self.em.ponHtml("")
            self.ready_ok("stop")
            self.runOrdenes()

    def terminar(self):
        self.finalizar()
        self.accept()

    def miraBuffer(self):
        if not self.siPlay:
            return
        self.lock = True

        li = self.buffer.split("\n")
        if self.buffer.endswith("\n"):
            self.buffer = ""
        else:
            self.buffer = li[-1]
            li = li[:-1]

        lir = []
        ok = False
        for linea in li:
            if not ok:
                if "TERM" in linea.upper():
                    ok = True
                else:
                    continue
            lir.append(linea)

            if "TOTAL EVALUATION" in linea.upper():
                break

        if lir:
            self.em.ponTexto("\n".join(lir))

        self.lock = False


class Orden:
    def __init__(self):
        self.clave = ""
        self.titulo = ""
        self.dv = {}


class CPU:
    def __init__(self, fdb):

        self.ipc = Util.IPC(fdb, False)

        self.configuracion = None
        self.configMotor = None
        self.titulo = None

        self.ventana = None
        self.motor = None

    def run(self):
        # Primero espera la orden de lucas
        while True:
            orden = self.recibe()
            if orden:
                break
            time.sleep(0.1)

        self.procesa(orden)

    def recibe(self):
        dv = self.ipc.pop()
        if not dv:
            return None

        orden = Orden()
        orden.clave = dv["__CLAVE__"]
        orden.dv = dv
        return orden

    def procesa(self, orden):
        clave = orden.clave
        if clave == CONFIGURACION:
            user = orden.dv["USER"]
            self.configuracion = Configuracion.Configuracion(user)
            self.configuracion.lee()
            self.configuracion.leeConfTableros()
            VarGen.configuracion = self.configuracion
            AperturasStd.reset()
            self.numkibitzer = orden.dv["NUMKIBITZER"]
            kibitzers = Kibitzers.Kibitzers()
            self.kibitzer = kibitzers.kibitzer(self.numkibitzer)
            prioridad = self.kibitzer.prioridad

            priorities = EngineThread.priorities

            if prioridad != priorities.normal:
                self.prioridad = priorities.value(prioridad)
            else:
                self.prioridad = None

            self.titulo = self.kibitzer.nombre

            if self.kibitzer.tipo != "B":
                self.configMotor = self.kibitzer.configMotor()

            self.key_video = "KIB%s" % self.kibitzer.huella
            self.dic_video = self.configuracion.restore_video(self.key_video)

            self.tipo = self.kibitzer.tipo
            self.posicionBase = self.kibitzer.posicionBase
            self.lanzaVentana()

        elif clave == FEN:
            self.fen, self.fenBase = orden.dv["FEN"].split("|")
            fen = self.fenBase if self.posicionBase else self.fen
            if self.tipo == "C":
                li = fen.split(" ")
                li[1] = "w" if li[1] == "b" else "b"
                li[3] = "-"  # Hay que tener cuidado con la captura al paso, stockfish crash.
                fen = " ".join(li)
            self.ventana.ponFen(fen)

        elif clave == TERMINAR:
            self.ipc.close()
            self.ventana.finalizar()
            self.ventana.reject()

    def save_video(self, dic):
        self.configuracion.save_video(self.key_video, dic)

    def lanzaVentana(self):
        app = QtGui.QApplication([])

        app.setStyle(QtGui.QStyleFactory.create("CleanLooks"))
        QtGui.QApplication.setPalette(QtGui.QApplication.style().standardPalette())

        self.configuracion.releeTRA()

        if self.tipo == "S":
            self.ventana = VentanaSiguiente(self)
        elif self.tipo == "C":
            self.ventana = VentanaSiguiente(self)
        elif self.tipo == "J":
            self.ventana = VentanaJugadas(self)
        elif self.tipo in "I":
            self.ventana = VentanaIndices(self)
        elif self.tipo == "L":
            self.ventana = VentanaLinea(self)
        elif self.tipo == "M":
            self.ventana = VentanaMultiPV(self)
        elif self.tipo == "E":
            self.ventana = VentanaStockfishEval(self)
        elif self.tipo == "B":
            self.ventana = VentanaPolyglot(self)
        self.ventana.show()

        VarGen.gc = QTUtil.GarbageCollector()

        return app.exec_()

    def compruebaInput(self):
        orden = self.recibe()
        if orden:
            self.procesa(orden)

    def dispatch(self, texto):
        if texto:
            self.ventana.ponTexto(texto)
        QTUtil.refreshGUI()

        return True


def run(fdb):
    ferr = open("./bug.kibitzer", "at")
    sys.stderr = ferr

    cpu = CPU(fdb)
    cpu.run()

    ferr.close()
