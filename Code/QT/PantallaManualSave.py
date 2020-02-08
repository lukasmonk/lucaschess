import os
import encodings
import codecs
from PyQt4 import QtGui, QtCore

from Code import ControlPosicion
from Code.QT import Voyager
from Code import Partida

from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import Tablero
from Code.QT import Columnas
from Code.QT import Grid
from Code.QT import Delegados
from Code.QT import QTUtil2


class WManualSave(QTVarios.WDialogo):
    def __init__(self, procesador):

        icono = Iconos.ManualSave()
        extparam = "manualsave"
        titulo = _("Save positions to FNS/PGN")
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.posicion = ControlPosicion.ControlPosicion()
        self.posicion.posInicial()

        self.gestor_motor = None
        self.pgn = None
        self.fns = None

        self.li_labels = [
            ["Site", ""],
            ["Event", ""],
            ["Date", ""],
            ["White", ""],
            ["Black", ""],
            ["WhiteElo", ""],
            ["BlackElo", ""],
            ["Result", ""],
        ]
        self.li_labels.extend([["", ""] for x in range(10)])

        self.li_analysis = []
        self.analyzing = False

        self.partida = None

        # Toolbar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("External engines"), Iconos.Motores(), self.ext_engines), None,
        )
        tb = QTVarios.LCTB(self, liAcciones)

        # Board + botones + solucion + boton salvado
        ##
        bt_change_position = Controles.PB(self, "   " + _("Change position"), self.change_position, plano=False)
        bt_change_position.ponIcono(Iconos.Datos(), 24)
        ##
        conf_tablero = self.configuracion.confTablero("MANUALSAVE", 32)
        self.tablero = Tablero.Tablero(self, conf_tablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(True)
        ##
        lybt, bt = QTVarios.lyBotonesMovimiento(self, "", siLibre=False, tamIcon=24, siTiempo=False)
        ##
        self.em_solucion = Controles.EM(self, siHTML=False).altoMinimo(40).capturaCambios(self.reset_partida)
        ##
        self.bt_solucion = Controles.PB(self, "   " + _("Save solution"), self.savesolucion, plano=False).ponIcono(Iconos.Grabar(), 24)
        self.bt_editar = Controles.PB(self, "   " + _("Edit"), self.editar_solucion, plano=False).ponIcono(Iconos.PlayGame())
        ly = Colocacion.V().control(self.em_solucion).control(self.bt_editar)
        gb = Controles.GB(self, _("Solution"), ly)
        ###
        lybtp = Colocacion.H().control(bt_change_position).espacio(20).control(self.bt_solucion)
        lyT = Colocacion.V().otro(lybtp).control(self.tablero).otro(lybt).control(gb)
        gb_left = Controles.GB(self, "", lyT)

        # Ficheros PGN + FNS
        lb_pgn = Controles.LB(self, _("PGN") + ": ")
        self.bt_pgn = Controles.PB(self, "", self.pgn_select, plano=False).anchoMinimo(300)
        bt_no_pgn = Controles.PB(self, "", self.pgn_unselect).ponIcono(Iconos.Delete()).anchoFijo(16)
        lb_fns = Controles.LB(self, _("FNS") + ": ")
        self.bt_fns = Controles.PB(self, "", self.fns_select, plano=False).anchoMinimo(300)
        bt_no_fns = Controles.PB(self, "", self.fns_unselect).ponIcono(Iconos.Delete()).anchoFijo(16)
        ## Codec
        lb_codec = Controles.LB(self, _("Encoding") + ": ")
        liCodecs = [k for k in set(v for k, v in encodings.aliases.aliases.iteritems())]
        liCodecs.sort()
        liCodecs = [(k, k) for k in liCodecs]
        liCodecs.insert(0, ("%s: %s" % (_("By default"), _("UTF-8")), "default"))
        self.codec = "default"
        self.cb_codecs = Controles.CB(self, liCodecs, self.codec)
        ###
        ly0 = Colocacion.G().control(lb_pgn, 0, 0).control(self.bt_pgn, 0, 1).control(bt_no_pgn, 0, 2)
        ly0.control(lb_fns, 1, 0).control(self.bt_fns, 1, 1).control(bt_no_fns, 1, 2)
        ly1 = Colocacion.H().control(lb_codec).control(self.cb_codecs).relleno(1)
        ly = Colocacion.V().otro(ly0).otro(ly1)
        gb_files = Controles.GB(self, _("File to save"), ly)

        # Labels + correlativo
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("LABEL", _("Label"), 80, edicion=Delegados.LineaTextoUTF8(), siCentrado=True)
        oColumnas.nueva("VALUE", _("Value"), 280, edicion=Delegados.LineaTextoUTF8())
        self.grid_labels = Grid.Grid(self, oColumnas, siEditable=True, xid=1)
        n = self.grid_labels.anchoColumnas()
        self.grid_labels.setFixedWidth(n + 20)
        self.registrarGrid(self.grid_labels)

        ##
        lb_number = Controles.LB(self, _("Correlative number")+": ")
        self.sb_number = Controles.SB(self, 0, 0, 99999999).tamMaximo(50)
        lb_number_help = Controles.LB(self, _("Replace symbol # in Value column (#=3, ###=003)"))
        lb_number_help.setWordWrap(True)

        ly_number = Colocacion.H().control(lb_number).control(self.sb_number).control(lb_number_help, 4)

        ly = Colocacion.V().control(self.grid_labels).otro(ly_number)
        gb_labels = Controles.GB(self, _("PGN labels"), ly)

        # Analysis + grid + start/stop + multiPV
        self.bt_start = Controles.PB(self, "", self.start).ponIcono(Iconos.Pelicula_Seguir(), 32)
        self.bt_stop = Controles.PB(self, "", self.stop).ponIcono(Iconos.Pelicula_Pausa(), 32)
        self.bt_stop.hide()

        lb_engine = Controles.LB(self, _("Engine") + ":")
        liMotores = self.configuracion.comboMotoresCompleto()
        self.cb_engine = Controles.CB(self, liMotores, self.configuracion.tutor.clave).capturaCambiado(self.reset_motor)

        lb_multipv = Controles.LB(self, _("Multi PV")+": ")
        self.sb_multipv = Controles.SB(self, 1, 1, 500).tamMaximo(50)
        ##
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("PDT", _("Evaluation"), 100, siCentrado=True)
        oColumnas.nueva("PGN", _("Solution"), 360)
        self.grid_analysis = Grid.Grid(self, oColumnas, siSelecFilas=True)
        self.registrarGrid(self.grid_analysis)
        ##
        lb_analysis_help = Controles.LB(self, _("Double click to send analysis line to solution"))
        ###
        ly_lin1 = Colocacion.H().control(self.bt_start).control(self.bt_stop).control(lb_engine).control(self.cb_engine)
        ly_lin1.relleno(1).control(lb_multipv).control(self.sb_multipv)
        ly = Colocacion.V().otro(ly_lin1).control(self.grid_analysis).control(lb_analysis_help)
        gb_analysis = Controles.GB(self, _("Analysis"), ly)

        # ZONA
        splitter_right = QtGui.QSplitter(self)
        splitter_right.setOrientation(QtCore.Qt.Vertical)
        splitter_right.addWidget(gb_files)
        splitter_right.addWidget(gb_labels)
        splitter_right.addWidget(gb_analysis)

        self.registrarSplitter(splitter_right, "RIGHT")
        ##
        splitter = QtGui.QSplitter(self)
        splitter.addWidget(gb_left)
        splitter.addWidget(splitter_right)

        self.registrarSplitter(splitter, "ALL")

        layout = Colocacion.V().control(tb).control(splitter).margen(5)

        self.setLayout(layout)

        self.inicializa()

    def inicializa(self):
        self.recuperarVideo(anchoDefecto=758, altoDefecto=596)

        dic_vars = self.configuracion.leeVariables("manual_save")
        if dic_vars:
            fen = dic_vars.get("FEN", self.posicion.fen())
            self.posicion.leeFen(fen)

            self.em_solucion.ponTexto(dic_vars.get("SOLUTION", ""))

            self.pgn = dic_vars.get("PGN", "")
            self.bt_pgn.ponTexto(self.pgn)

            self.fns = dic_vars.get("FNS", "")
            self.bt_fns.ponTexto(self.fns)

            self.cb_codecs.ponValor(dic_vars.get("CODEC",""))

            self.li_labels = dic_vars.get("LI_LABELS", [])

            self.sb_number.ponValor(dic_vars.get("NUMBER",0))

            self.cb_engine.ponValor(dic_vars.get("ENGINE", "honey"))

            self.sb_multipv.ponValor(dic_vars.get("MULTIPV", 1))

        self.tablero.ponPosicion(self.posicion)
        self.reset_motor()
        self.test_save_solucion()

    def test_save_solucion(self):
        self.bt_solucion.setDisabled((not self.pgn) and (not self.fns))

    def finaliza(self):
        self.analyzing = False
        self.gestor_motor.terminar()
        self.guardarVideo()

        dic_vars = {
            "FEN": self.posicion.fen(),
            "SOLUTION": self.em_solucion.texto(),
            "PGN": self.pgn,
            "FNS": self.fns,
            "CODEC": self.cb_codecs.valor(),
            "LI_LABELS": self.li_labels,
            "NUMBER": self.sb_number.valor(),
            "ENGINE": self.cb_engine.valor(),
            "MULTIPV": self.sb_multipv.valor()
        }

        self.configuracion.escVariables("manual_save", dic_vars)

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def gridNumDatos(self, grid):
        return len(self.li_labels if grid.id == 1 else self.li_analysis)

    def gridPonValor(self, grid, fila, oColumna, valor):
        n = 0 if oColumna.clave == "LABEL" else 1
        self.li_labels[fila][n] = valor

    def gridDato(self, grid, fila, oColumna):
        if grid.id == 1:
            n = 0 if oColumna.clave == "LABEL" else 1
            return self.li_labels[fila][n]
        else:
            if oColumna.clave == "PDT":
                return self.li_analysis[fila].ms_pdt
            else:
                return self.li_analysis[fila].ms_pgn

    def gridDobleClick(self, grid, fila, oColumna):
        if grid == self.grid_analysis:
            self.em_solucion.ponTexto(self.li_analysis[fila].ms_pgn)
            self.reset_partida()
        self.grid_analysis.goto(fila, 0)

    def start(self):
        self.sb_multipv.setDisabled(True)
        self.cb_engine.setDisabled(True)
        self.analyzing = True
        self.sb_multipv.setDisabled(True)
        self.show_stop()
        multipv = self.sb_multipv.valor()
        self.gestor_motor.actMultiPV(multipv)
        partida = Partida.Partida(self.posicion)
        self.gestor_motor.ac_inicio(partida)
        QtCore.QTimer.singleShot(1000, self.lee_analisis)

    def lee_analisis(self):
        if self.analyzing:
            mrm = self.gestor_motor.ac_estado()
            li = []
            for rm in mrm.liMultiPV:
                partida = Partida.Partida(self.posicion)
                partida.leerPV(rm.pv)
                partida.ms_pgn = partida.pgnBaseRAW()
                partida.ms_pdt = rm.abrTextoPDT()
                li.append(partida)
            self.li_analysis = li
            self.grid_analysis.refresh()

            QtCore.QTimer.singleShot(2000, self.lee_analisis)

    def stop(self):
        self.sb_multipv.setDisabled(False)
        self.cb_engine.setDisabled(False)
        self.analyzing = False
        self.show_start()
        if self.gestor_motor:
            self.gestor_motor.ac_final(0)

    def pgn_select(self):
        dirSalvados = self.configuracion.dirSalvados
        path = QTUtil2.salvaFichero(self, _("File to save"), dirSalvados, _("File") + " PGN (*.pgn)", False)
        if path:
            carpeta, fichero = os.path.split(path)
            if carpeta != self.configuracion.dirSalvados:
                self.configuracion.dirSalvados = carpeta
                self.configuracion.graba()
            self.pgn = path
            self.bt_pgn.ponTexto(path)
        self.test_save_solucion()

    def pgn_unselect(self):
        self.pgn = None
        self.bt_pgn.ponTexto("")
        self.test_save_solucion()

    def fns_select(self):
        dir_inicial = self.configuracion.dirPersonalTraining if self.fns is None else os.path.dirname(self.fns)
        path = QTUtil2.salvaFichero(self, _("File to save"), dir_inicial, _("File") + " FNS (*.fns)", False)
        if path:
            self.fns = path
            self.bt_fns.ponTexto(path)
        self.test_save_solucion()

    def fns_unselect(self):
        self.fns = None
        self.bt_fns.ponTexto("")
        self.test_save_solucion()

    def editar_solucion(self):
        self.reset_motor()
        pc = self.crea_partida()
        pc = self.procesador.gestorPartida(self, pc, False, self.tablero)
        if pc:
            self.posicion = pc.iniPosicion
            self.em_solucion.ponTexto(pc.pgnBaseRAW())

    def savesolucion(self):
        def open_file(fich):
            codec = self.cb_codecs.valor()
            if codec == "default":
                codec = "UTF-8"
            try:
                f = codecs.open(fich, "ab", encoding=codec)
            except:
                QTUtil2.mensError(self, _("Error opening file %s") % fich)
                f = None
            return f

        def write_file(fich, f, txt, quien):
            time = 0.5 if (self.pgn and self.fns) else 1.0
            try:
                f.write(txt)
                QTUtil2.mensajeTemporal(self,"%s: %s" %(quien, _("Saved")), time)
            except:
                QTUtil2.mensError(self, _("Error writing to file %s") % fich)
            f.close()

        pc = self.crea_partida()

        if self.pgn:
            f = open_file(self.pgn)
            if not f:
                return
            txt = pc.pgn() +"\n\n"
            write_file(self.pgn, f, txt, _("PGN"))

        if self.fns:
            f = open_file(self.fns)
            if not f:
                return
            fen = self.posicion.fen()
            li = ['%s: %s'%(k,v) for k,v in pc.liTags if k != "FEN"]
            labels = ",".join(li)
            solucion = self.em_solucion.texto()
            linea = "%s|%s|%s\n" % (fen, labels, solucion)
            write_file(self.fns, f, linea, _("FNS"))

    def change_position(self):
        prev = self.analyzing
        self.stop()
        fen = Voyager.voyagerFEN(self, self.posicion.fen(), wownerowner=self.procesador.pantalla)
        if fen is not None:
            self.em_solucion.ponTexto("")
            self.posicion.leeFen(fen)

            self.tablero.ponPosicion(self.posicion)

            self.sb_number.ponValor(self.sb_number.valor()+1)
        if prev:
            self.start()

    def reset_motor(self):
        clave = self.cb_engine.valor()
        if not clave:
            return
        self.analyzing = False
        if self.gestor_motor:
            self.gestor_motor.terminar()
        self.stop()
        conf_motor  = self.configuracion.buscaRivalExt(clave)

        multipv = self.sb_multipv.valor()
        self.gestor_motor = self.procesador.creaGestorMotor(conf_motor, 0, 0, siMultiPV=multipv>1)

    def ext_engines(self):
        if self.analyzing:
            return
        self.procesador.motoresExternos()
        valor = self.cb_engine.valor()
        liMotores = self.configuracion.comboMotoresCompleto()
        motor = self.configuracion.tutor.clave
        for rotulo, clave in liMotores:
            if clave == valor:
                motor = valor
                break
        self.cb_engine.rehacer(liMotores, motor)
        self.reset_motor()
        self.show_start()

    def show_start(self):
        self.bt_stop.hide()
        self.bt_start.show()

    def show_stop(self):
        self.bt_start.hide()
        self.bt_stop.show()

    def terminar(self):
        self.finaliza()
        self.accept()

    def closeEvent(self, event):
        self.finaliza()

    def crea_partida(self):
        li_tags = []
        number = self.sb_number.valor()
        for key, value in self.li_labels:
            if key and value:
                if "#" in value:
                    n = value.count("#")
                    if n > 1:
                        t = "#"*n
                        if t in value:
                            value = value.replace(t, "%0" + "%d"%n + "d")
                            value = value % number
                        else:
                            value = value.replace("#", str(number), 1)
                    else:
                        value = value.replace("#", str(number))
            li_tags.append((key, value))
        li_tags.append(("FEN", self.posicion.fen()))
        li = ['[%s "%s"]\n'%(k,v) for k,v in li_tags]
        txt = "".join(li)
        pc = Partida.PartidaCompleta(self.posicion)
        txt += "\n%s" % self.em_solucion.texto()
        pc.readPGN(txt)
        return pc

    def reset_partida(self):
        if self.partida:
            self.partida = None
            self.tablero.ponPosicion(self.posicion)

    def test_partida(self):
        if not self.partida:
            self.partida = self.crea_partida()
            self.tablero.ponPosicion(self.posicion)
            self.partida.mover_jugada = -1

    def MoverInicio(self):
        self.test_partida()
        self.partida.mover_jugada = -1
        self.tablero.ponPosicion(self.posicion)

    def MoverAtras(self):
        self.test_partida()
        if self.partida.mover_jugada >= 0:
            self.partida.mover_jugada -= 1
            if self.partida.mover_jugada == -1:
                self.tablero.ponPosicion(self.posicion)
            else:
                jg = self.partida.jugada(self.partida.mover_jugada)
                self.tablero.ponPosicion(jg.posicion)
                self.tablero.ponFlechaSC(jg.desde, jg.hasta)

    def MoverAdelante(self):
        self.test_partida()
        if self.partida.mover_jugada < (self.partida.numJugadas()-1):
            self.partida.mover_jugada += 1
            jg = self.partida.jugada(self.partida.mover_jugada)
            self.tablero.ponPosicion(jg.posicion)
            self.tablero.ponFlechaSC(jg.desde, jg.hasta)
            return True
        return False

    def MoverFinal(self):
        self.test_partida()
        if self.partida.numJugadas():
            self.partida.mover_jugada = self.partida.numJugadas()-1
            jg = self.partida.jugada(self.partida.mover_jugada)
            self.tablero.ponPosicion(jg.posicion)
            self.tablero.ponFlechaSC(jg.desde, jg.hasta)


def manual_save(procesador):
    w = WManualSave(procesador)
    w.exec_()
