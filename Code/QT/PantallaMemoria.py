import random
import time

from PyQt4 import QtCore, QtGui

from Code import ControlPosicion
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero


class WDatos(QtGui.QDialog):
    def __init__(self, wParent, txtcategoria, maxNivel):
        super(WDatos, self).__init__(wParent)

        self.setWindowTitle(_("Check your memory on a chessboard"))
        self.setWindowIcon(Iconos.Memoria())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        tb = QTUtil2.tbAcceptCancel(self)

        f = Controles.TipoLetra(puntos=12, peso=75)

        self.ed, lb = QTUtil2.spinBoxLB(self, maxNivel, 1, maxNivel, etiqueta=txtcategoria + " " + _("Level"), maxTam=40)
        lb.ponFuente(f)

        ly = Colocacion.H().control(lb).control(self.ed).margen(20)

        layout = Colocacion.V().control(tb).otro(ly).margen(3)

        self.setLayout(layout)

    def aceptar(self):
        self.nivel = self.ed.value()
        self.accept()


def paramMemoria(parent, txtCategoria, maxNivel):
    if maxNivel == 1:
        return 1

    # Datos
    w = WDatos(parent, txtCategoria, maxNivel)
    if w.exec_():
        return w.nivel
    else:
        return None


class WMemoria(QTVarios.WDialogo):
    def __init__(self, procesador, txtcategoria, nivel, segundos, listaFen, record):

        titulo = _("Check your memory on a chessboard")
        icono = Iconos.Memoria()
        extparam = "memoria"
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)

        f = Controles.TipoLetra(puntos=10, peso=75)

        self.configuracion = procesador.configuracion
        self.nivel = nivel
        self.segundos = segundos
        self.record = record

        # Tablero
        confTablero = self.configuracion.confTablero("MEMORIA", 48)

        self.listaFen = listaFen

        self.posicion = ControlPosicion.ControlPosicion()

        self.tablero = Tablero.PosTablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponDispatchDrop(self.dispatchDrop)
        self.tablero.baseCasillasSC.setAcceptDrops(True)
        self.ultimaPieza = "P"
        self.piezas = self.tablero.piezas

        tamPiezas = max(16, int(32 * self.tablero.confTablero.anchoPieza() / 48))
        self.listaPiezasW = QTVarios.ListaPiezas(self, "P,N,B,R,Q,K", self.tablero, tamPiezas, margen=0)
        self.listaPiezasB = QTVarios.ListaPiezas(self, "p,n,b,r,q,k", self.tablero, tamPiezas, margen=0)

        # Ayuda
        lbAyuda = Controles.LB(self, _(
                "<ul><li><b>Add piece</b> : Right mouse button on empty square</li><li><b>Copy piece</b> : Left mouse button on empty square</li><li><b>Move piece</b> : Drag and drop piece with left mouse button</li><li><b>Delete piece</b> : Right mouse button on occupied square</li></ul>"))
        ly = Colocacion.H().control(lbAyuda)
        self.gbAyuda = Controles.GB(self, _("Help"), ly)

        # Rotulos informacion
        lbCategoria = Controles.LB(self, txtcategoria).ponFuente(f)
        lbNivel = Controles.LB(self, _X(_("Level %1/%2"), str(nivel + 1), "25")).ponFuente(f)
        if record:
            lbRecord = Controles.LB(self, _X(_("Record %1 seconds"), str(record))).ponFuente(f)

        # Rotulo de tiempo
        self.rotuloDispone = Controles.LB(self,
                                          _X(_("You have %1 seconds to remember the position of %2 pieces"), str(self.segundos),
                                             str(self.nivel + 3))).ponWrap().ponFuente(f).alinCentrado()
        self.rotuloDispone1 = Controles.LB(self, _("when you know you can press the Continue button")).ponWrap().ponFuente(
                f).alinCentrado()
        ly = Colocacion.V().control(self.rotuloDispone).control(self.rotuloDispone1)
        self.gbTiempo = Controles.GB(self, "", ly)

        self.rotuloDispone1.hide()

        # Tool bar
        liAcciones = (
            (_("Start"), Iconos.Empezar(), "empezar"),
            (_("Continue"), Iconos.Pelicula_Seguir(), "seguir"),
            (_("Check"), Iconos.Check(), "comprobar"),
            (_("Target"), Iconos.Verde32(), "objetivo"),
            (_("Wrong"), Iconos.Rojo32(), "nuestro"),
            (_("Repeat"), Iconos.Pelicula_Repetir(), "repetir"),
            (_("Resign"), Iconos.Abandonar(), "abandonar"),
        )
        self.tb = tb = Controles.TB(self, liAcciones)
        self.ponToolBar(["empezar"])

        # Colocamos
        lyP = Colocacion.H().relleno().control(self.listaPiezasW).control(self.listaPiezasB).relleno().margen(0)
        lyT = Colocacion.V().control(self.tablero).otro(lyP).margen(0)

        lyI = Colocacion.V()
        lyI.control(tb)
        lyI.relleno()
        lyI.controlc(lbCategoria)
        lyI.controlc(lbNivel)
        if record:
            lyI.controlc(lbRecord)
        lyI.controlc(self.gbTiempo)
        lyI.relleno()
        lyI.control(self.gbAyuda)
        lyI.margen(3)

        ly = Colocacion.H().otro(lyT).otro(lyI).relleno()
        ly.margen(3)

        self.setLayout(ly)

        self.timer = None

        self.encenderExtras(False)

    def mueve(self, desde, hasta):
        if desde == hasta:
            return
        if self.casillas[hasta]:
            self.tablero.borraPieza(hasta)
        self.casillas[hasta] = self.casillas[desde]
        self.casillas[desde] = None
        self.tablero.muevePieza(desde, hasta)

    def borraCasilla(self, desde):
        self.casillas[desde] = None
        self.tablero.borraPieza(desde)

    def creaCasilla(self, desde):
        menu = QtGui.QMenu(self)

        siK = False
        sik = False
        for p in self.casillas.itervalues():
            if p == "K":
                siK = True
            elif p == "k":
                sik = True

        liOpciones = []
        if not siK:
            liOpciones.append((_("King"), "K"))
        liOpciones.extend(
                [(_("Queen"), "Q"), (_("Rook"), "R"), (_("Bishop"), "B"), (_("Knight"), "N"), (_("Pawn"), "P")])
        if not sik:
            liOpciones.append((_("King"), "k"))
        liOpciones.extend(
                [(_("Queen"), "q"), (_("Rook"), "r"), (_("Bishop"), "b"), (_("Knight"), "n"), (_("Pawn"), "p")])

        for txt, pieza in liOpciones:
            icono = self.tablero.piezas.icono(pieza)

            accion = QtGui.QAction(icono, txt, menu)
            accion.clave = pieza
            menu.addAction(accion)

        resp = menu.exec_(QtGui.QCursor.pos())
        if resp:
            pieza = resp.clave
            self.ponPieza(desde, pieza)

    def repitePieza(self, desde):
        self.casillas[desde] = self.ultimaPieza
        pieza = self.tablero.creaPieza(self.ultimaPieza, desde)
        pieza.activa(True)

    def ponPieza(self, desde, pieza):
        antultimo = self.ultimaPieza
        self.ultimaPieza = pieza
        self.repitePieza(desde)
        if pieza == "K":
            self.ultimaPieza = antultimo
        if pieza == "k":
            self.ultimaPieza = antultimo

    def dispatchDrop(self, desde, pieza):
        if self.casillas[desde]:
            self.borraCasilla(desde)
        self.ponPieza(desde, pieza)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "abandonar":
            self.reject()
        elif accion == "empezar":
            self.empezar()
        elif accion == "seguir":
            self.seguir()
        elif accion == "comprobar":
            self.comprobar()
        elif accion == "objetivo":
            self.objetivo()
        elif accion == "nuestro":
            self.nuestro()
        elif accion == "repetir":
            self.repetir()

    def ponToolBar(self, liAcciones):

        self.tb.clear()
        for k in liAcciones:
            self.tb.dicTB[k].setVisible(True)
            self.tb.dicTB[k].setEnabled(True)
            self.tb.addAction(self.tb.dicTB[k])

        self.tb.liAcciones = liAcciones
        self.tb.update()

    def empezar(self):
        # Ha pulsado empezar

        # Elegimos el fen de la lista
        nPos = random.randint(0, len(self.listaFen) - 1)
        self.fenObjetivo = self.listaFen[nPos]
        del self.listaFen[nPos]
        self.posicion.leeFen(self.fenObjetivo)
        self.tablero.ponPosicion(self.posicion)
        self.tablero.desactivaTodas()
        self.casillas = self.posicion.casillas
        self.tablero.casillas = self.casillas

        # Quitamos empezar y ponemos seguir
        self.ponToolBar(["seguir"])

        self.rotuloDispone.ponTexto(
                _X(_("You have %1 seconds to remember the position of %2 pieces"), str(self.segundos), str(self.nivel + 3)))
        self.rotuloDispone1.ponTexto(_("when you know you can press the Continue button"))
        self.rotuloDispone1.show()
        self.rotuloDispone1.show()
        self.gbTiempo.show()

        self.tiempoPendiente = self.segundos
        self.iniciaReloj()

    def seguir(self):
        self.paraReloj()

        self.tablero.ponMensajero(self.mueve)
        self.tablero.mensBorrar = self.borraCasilla
        self.tablero.mensCrear = self.creaCasilla
        self.tablero.mensRepetir = self.repitePieza

        # Quitamos seguir y ponemos comprobar
        self.ponToolBar(["comprobar"])

        self.rotuloDispone1.ponTexto(
                _X(_("When you've loaded the %1 pieces you can click the Check button"), str(self.nivel + 3)))
        self.rotuloDispone.setVisible(False)

        self.iniTiempo = time.time()

        for k in self.casillas:
            self.casillas[k] = None
        self.tablero.ponPosicion(self.posicion)

        self.encenderExtras(True)

    def encenderExtras(self, si):
        self.gbAyuda.setVisible(si)
        self.listaPiezasW.setEnabled(si)
        self.listaPiezasB.setEnabled(si)

    def ponCursor(self):
        cursor = self.piezas.cursor(self.ultimaPieza)
        for item in self.tablero.escena.items():
            item.setCursor(cursor)
        self.tablero.setCursor(cursor)

    def comprobar(self):

        self.tiempo = int(time.time() - self.iniTiempo)

        fenNuevo = self.posicion.fen()
        fenNuevo = fenNuevo[:fenNuevo.index(" ")]
        fenComprobar = self.fenObjetivo
        fenComprobar = fenComprobar[:fenComprobar.index(" ")]

        if fenComprobar == fenNuevo:
            mens = _X(_("Right, it took %1 seconds."), str(self.tiempo))
            if self.tiempo < self.record or self.record == 0:
                mens += "<br>" + _("New record!")
            QTUtil2.mensaje(self, mens)
            self.accept()
            return

        QTUtil2.mensaje(self, _("The position is incorrect."))

        self.fenNuestro = self.posicion.fen()

        self.tablero.ponMensajero(None)
        self.tablero.mensBorrar = None
        self.tablero.mensCrear = None
        self.tablero.mensRepetir = None
        self.tablero.desactivaTodas()

        self.gbTiempo.hide()

        self.encenderExtras(False)

        # Quitamos comprobar y ponemos el resto
        li = ["objetivo", "nuestro"]
        if len(self.listaFen):
            li.append("repetir")

        self.ponToolBar(li)

    def objetivo(self):
        self.posicion.leeFen(self.fenObjetivo)
        self.tablero.ponPosicion(self.posicion)
        self.tablero.desactivaTodas()

    def nuestro(self):
        self.posicion.leeFen(self.fenNuestro)
        self.tablero.ponPosicion(self.posicion)
        self.tablero.desactivaTodas()

    def repetir(self):
        self.rotuloDispone.ponTexto(
                _X(_("You have %1 seconds to remember the position of %2 pieces"), str(self.segundos), str(self.nivel + 3)))
        self.rotuloDispone.show()
        self.rotuloDispone1.hide()
        self.gbTiempo.show()

        self.empezar()

    def reloj(self):
        self.tiempoPendiente -= 1

        self.rotuloDispone.ponTexto(
                _X(_("You have %1 seconds to remember the position of %2 pieces"), str(self.tiempoPendiente),
                   str(self.nivel + 3)))
        if self.tiempoPendiente == 0:
            self.seguir()

    def iniciaReloj(self):
        if self.timer is not None:
            self.timer.stop()
            del self.timer

        self.timer = QtCore.QTimer(self)
        self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.reloj)
        self.timer.start(1000)

    def paraReloj(self):
        if self.timer is not None:
            self.timer.stop()
            del self.timer
            self.timer = None


def lanzaMemoria(procesador, txtcategoria, nivel, segundos, listaFen, record):
    w = WMemoria(procesador, txtcategoria, nivel, segundos, listaFen, record)
    if w.exec_():
        return w.tiempo
    else:
        return None
