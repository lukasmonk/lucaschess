from PyQt4 import QtCore, QtGui

import Code.ControlPosicion as ControlPosicion
import Code.DGT as DGT
import Code.Voice as Voice
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.QTVarios as QTVarios
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Tablero as Tablero

class WPosicion(QtGui.QDialog):
    def __init__(self, wParent, configuracion, fen):

        QtGui.QDialog.__init__(self, wParent)

        self.wParent = wParent

        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.configuracion = configuracion

        self.setWindowTitle(_("Position"))
        self.setWindowIcon(Iconos.Datos())

        # Quien mueve
        self.rbBlancas = Controles.RB(self, _("White"))
        self.rbNegras = Controles.RB(self, _("Black"))

        # Enroques permitidos
        self.cbBoo = Controles.CHB(self, _("White") + " O-O", True)
        self.cbBooo = Controles.CHB(self, _("White") + " O-O-O", True)
        self.cbNoo = Controles.CHB(self, _("Black") + " O-O", True)
        self.cbNooo = Controles.CHB(self, _("Black") + " O-O-O", True)

        # Peon al paso
        lbAlPaso = Controles.LB(self, _("En passant") + ":")
        self.edAlPaso = Controles.ED(self).controlrx("(-|[a-h][36])").anchoFijo(30)

        # Medias jugadas desde ultimo mov. peon
        self.edMedias, lbMedias = QTUtil2.spinBoxLB(self, 0, 0, 999,
                                                    etiqueta=_("Moves since the last pawn advance or capture"),
                                                    maxTam=50)

        # Jugadas
        self.edJugadas, lbJugadas = QTUtil2.spinBoxLB(self, 1, 1, 999, etiqueta=_("Moves"), maxTam=50)

        # Botones adicionales
        btPosInicial = Controles.PB(self, _("Start position"), self.posInicial, plano=False)
        btLimpiaTablero = Controles.PB(self, _("Clear board"), self.limpiaTablero, plano=False)
        btPegar = Controles.PB(self, _("Paste FEN position"), self.pegarPosicion, plano=False)
        btCopiar = Controles.PB(self, _("Copy FEN position"), self.copiarPosicion, plano=False)
        btVoyager = Controles.PB(self, "", self.lanzaVoyager, plano=False).ponIcono(Iconos.Voyager())
        self.btVoice = Controles.PB(self, "", self.voiceActive, plano=False).ponIcono(Iconos.S_Microfono())
        self.btVoiceX = Controles.PB(self, "", self.voiceDeactive, plano=False).ponIcono(Iconos.X_Microfono())

        # Tablero
        confTablero = configuracion.confTablero("POSICION", 24)
        self.posicion = ControlPosicion.ControlPosicion()
        if fen:
            self.posicion.leeFen(fen)
        else:
            self.posicion.posInicial()
        self.tablero = Tablero.PosTablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueve)
        self.tablero.mensBorrar = self.borraCasilla
        self.tablero.mensCrear = self.creaCasilla
        self.tablero.mensRepetir = self.repitePieza
        self.ultimaPieza = "P"
        self.piezas = self.tablero.piezas

        self.resetPosicion()

        # Piezas drag-drop
        self.dragDropWI = QTVarios.ListaPiezas(self, "P;N;B;R;Q;K", self.tablero)
        self.dragDropWB = QTVarios.ListaPiezas(self, "P,N,B,R,Q,K", self.tablero)
        self.dragDropBD = QTVarios.ListaPiezas(self, "k;q;r;b;n;p", self.tablero)
        self.dragDropBA = QTVarios.ListaPiezas(self, "k,q,r,b,n,p", self.tablero)
        self.tablero.ponDispatchDrop(self.dispatchDrop)
        self.tablero.baseCasillasSC.setAcceptDrops(True)

        # Ayuda
        lbAyuda = Controles.LB(self, _(
            "<ul><li><b>Add piece</b> : Right mouse button on empty square</li><li><b>Copy piece</b> : Left mouse button on empty square</li><li><b>Move piece</b> : Drag and drop piece with left mouse button</li><li><b>Delete piece</b> : Right mouse button on occupied square</li></ul>"))

        # Tool bar
        tb = QTUtil2.tbAcceptCancel(self, siReject=False)

        # Layout

        # # Quien mueve
        hbox = Colocacion.H().relleno().control(self.rbBlancas).espacio(30).control(self.rbNegras).relleno()
        gbColor = Controles.GB(self, _("Next move"), hbox)

        # # Enroques
        ly = Colocacion.G().control(self.cbBoo, 0, 0).control(self.cbNoo, 0, 1)
        ly.control(self.cbBooo, 1, 0).control(self.cbNooo, 1, 1)
        gbEnroques = Controles.GB(self, _("Castling moves possible"), ly)

        ## Otros
        ly = Colocacion.G()
        ly.controld(lbMedias, 0, 0, 1, 3).control(self.edMedias, 0, 3)
        ly.controld(lbAlPaso, 1, 0).control(self.edAlPaso, 1, 1)
        ly.controld(lbJugadas, 1, 2).control(self.edJugadas, 1, 3)
        gbOtros = Controles.GB(self, "", ly)

        ## Botones adicionales
        lyBA = Colocacion.H().control(btPosInicial).control(btLimpiaTablero).control(btPegar).control(btCopiar).control(
            btVoyager)

        ## Ayuda
        ly = Colocacion.H().control(lbAyuda)
        gbAyuda = Controles.GB(self, _("Help"), ly)

        ## Izquierda
        ly = Colocacion.V().control(gbColor).relleno().control(gbEnroques).relleno()
        ly.control(gbOtros).relleno().control(gbAyuda).margen(5)
        lyI = Colocacion.V().control(tb).otro(ly).margen(3)

        ## Derecha
        lyBT = Colocacion.H().control(self.btVoice).control(self.btVoiceX)
        lyDA = Colocacion.G()
        lyDA.controlc(self.dragDropBA, 0, 1).otro(lyBT, 0, 2)
        lyDA.controld(self.dragDropWI, 1, 0).control(self.tablero, 1, 1).control(self.dragDropBD, 1, 2)
        lyDA.controlc(self.dragDropWB, 2, 1)

        lyD = Colocacion.V().otro(lyDA).otro(lyBA).relleno()

        ## Completo
        ly = Colocacion.H().otro(lyI).otro(lyD).margen(3)
        self.setLayout(ly)

        if configuracion.siDGT:
            if not DGT.activarSegunON_OFF(self.dgt):  # Error
                QTUtil2.mensError(self, _("Error, could not detect the DGT board driver."))

        self.ponCursor()

        self.voyager = None
        self.bufferVoice = ""
        self.queueVoice = []
        self.isVoiceActive = False
        if not configuracion.voice:
            self.btVoiceX.setVisible(False)
            self.btVoice.setVisible(False)
        else:
            if Voice.runVoice.isActive():
                self.voiceActive()
            else:
                self.voiceDeactive()

    def voiceActive(self):
        self.btVoiceX.setVisible(True)
        self.btVoice.setVisible(False)
        Voice.runVoice.start(self.voice)
        self.isVoiceActive = True

    def voiceDeactive(self):
        self.btVoiceX.setVisible(False)
        self.btVoice.setVisible(True)
        if self.isVoiceActive:
            Voice.runVoice.stop()
            self.isVoiceActive = False

    def voice(self, lista):
        li = lista.split(" ")
        white = self.ultimaPieza.isupper()
        for w in li:
            if w in "PNBRQK":
                self.ultimaPieza = w if white else w.lower()
            elif w in "abcdefgh":
                self.bufferVoice = w
            elif w in "12345678":
                if self.bufferVoice:
                    pos = self.bufferVoice + w
                    self.bufferVoice = ""
                    self.actPosicion()
                    self.queueVoice.append( self.posicion.fen() )
                    if self.casillas[pos]:
                        self.borraCasilla(pos)
                    else:
                        self.repitePieza(pos)
            elif w == "WHITE":
                self.ultimaPieza = self.ultimaPieza.upper()
            elif w == "BLACK":
                self.ultimaPieza = self.ultimaPieza.lower()
            elif w == "TAKEBACK":
                if self.queueVoice:
                    fen = self.queueVoice[-1]
                    self.queueVoice = self.queueVoice[:-1]
                    self.posicion.leeFen(fen)
                    self.resetPosicion()

    def lanzaVoyager(self):
        self.actPosicion()
        self.voyager = WVoyager(self, self.configuracion, self.posicion)
        self.voyager.exec_()
        self.voyager = None
        self.resetPosicion()

    def dgt(self, quien, dato):
        if quien == "scan":
            siBlancas, alPaso, jugadas, movPeonCap, enroques = self.leeDatos()
            fen = dato + " " + \
                  ("w" if siBlancas else "b") + " " + \
                  enroques + " " + \
                  alPaso + " " + \
                  str(movPeonCap) + " " + \
                  str(jugadas)
            posic = ControlPosicion.ControlPosicion()
            posic.leeFen(fen)
            if posic.fen() != self.posicion.fen():
                self.posicion.leeFen(posic.fen())
                self.resetPosicion()

    def cierra(self):
        DGT.quitarDispatch()
        self.voiceDeactive()

    def closeEvent(self, event):
        self.cierra()
        event.accept()

    def ponCursor(self):
        cursor = self.piezas.cursor(self.ultimaPieza)
        for item in self.tablero.escena.items():
            item.setCursor(cursor)
        self.tablero.setCursor(cursor)

    def cambiaPiezaSegun(self, pieza):
        ant = self.ultimaPieza
        if ant.upper() == pieza:
            if ant == pieza:
                pieza = pieza.lower()
        self.ultimaPieza = pieza
        self.ponCursor()

    def mueve(self, desde, hasta):
        if desde == hasta:
            return
        if self.casillas[hasta]:
            self.tablero.borraPieza(hasta)
        self.casillas[hasta] = self.casillas[desde]
        self.casillas[desde] = None
        self.tablero.muevePieza(desde, hasta)

        self.ponCursor()

    def dispatchDrop(self, desde, pieza):
        if self.casillas[desde]:
            self.borraCasilla(desde)
        self.ponPieza(desde, pieza)

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
            liOpciones.append(( _("King"), "K"))
        liOpciones.extend(
            [( _("Queen"), "Q"), (_("Rook"), "R"), (_("Bishop"), "B"), (_("Knight"), "N"), (_("Pawn"), "P")])
        if not sik:
            liOpciones.append(( _("King"), "k"))
        liOpciones.extend(
            [( _("Queen"), "q"), (_("Rook"), "r"), (_("Bishop"), "b"), (_("Knight"), "n"), (_("Pawn"), "p")])

        for txt, pieza in liOpciones:
            icono = self.tablero.piezas.icono(pieza)

            accion = QtGui.QAction(icono, txt, menu)
            accion.clave = pieza
            menu.addAction(accion)

        resp = menu.exec_(QtGui.QCursor.pos())
        if resp:
            pieza = resp.clave
            self.ponPieza(desde, pieza)

    def ponPieza(self, desde, pieza):
        antultimo = self.ultimaPieza
        self.ultimaPieza = pieza
        self.repitePieza(desde)
        if pieza == "K":
            self.ultimaPieza = antultimo
        if pieza == "k":
            self.ultimaPieza = antultimo

        self.ponCursor()

    def repitePieza(self, desde):
        pieza = self.ultimaPieza
        if pieza in "kK":
            for pos, pz in self.casillas.iteritems():
                if pz == pieza:
                    self.borraCasilla(pos)
                    break
        if QtGui.QApplication.keyboardModifiers() & QtCore.Qt.ShiftModifier:
            if pieza.islower():
                pieza = pieza.upper()
            else:
                pieza = pieza.lower()
        self.casillas[desde] = pieza
        pieza = self.tablero.creaPieza(pieza, desde)
        pieza.activa(True)

        self.ponCursor()

    def leeDatos(self):
        siBlancas = self.rbBlancas.isChecked()
        alPaso = self.edAlPaso.texto().strip()
        if not alPaso:
            alPaso = "-"
        jugadas = self.edJugadas.value()
        movPeonCap = self.edMedias.value()

        enroques = ""
        for cont, pieza in ( (self.cbBoo, "K" ), (self.cbBooo, "Q" ), (self.cbNoo, "k"), (self.cbNooo, "q") ):
            if cont.isChecked():
                enroques += pieza
        if not enroques:
            enroques = "-"
        return siBlancas, alPaso, jugadas, movPeonCap, enroques

    def actPosicion(self):
        self.posicion.siBlancas, self.posicion.alPaso, \
        self.posicion.jugadas, self.posicion.movPeonCap, self.posicion.enroques = self.leeDatos()

    def aceptar(self):
        if self.posicion.siExistePieza("K") != 1:
            QTUtil2.mensError(self, _("King") + "-" + _("White") + "???")
            return
        if self.posicion.siExistePieza("k") != 1:
            QTUtil2.mensError(self, _("King") + "-" + _("Black") + "???")
            return

        self.actPosicion()

        self.fen = self.posicion.fen()  # Hace control de enroques y alPaso
        if self.fen == ControlPosicion.FEN_INICIAL:
            self.fen = ""
        self.cierra()
        self.accept()

    def cancelar(self):
        self.cierra()
        self.reject()

    def pegarPosicion(self):
        cb = QtGui.QApplication.clipboard()
        fen = cb.text()
        if fen:
            try:
                self.posicion.leeFen(str(fen))
                self.resetPosicion()
            except:
                pass

    def copiarPosicion(self):
        cb = QtGui.QApplication.clipboard()
        self.actPosicion()
        cb.setText(self.posicion.fen())

    def limpiaTablero(self):
        self.posicion.leeFen("8/8/8/8/8/8/8/8 w - - 0 1")
        self.resetPosicion()

    def posInicial(self):
        self.posicion.posInicial()
        self.resetPosicion()

    def resetPosicion(self):
        self.tablero.ponPosicion(self.posicion)
        self.casillas = self.posicion.casillas
        self.tablero.casillas = self.casillas
        self.tablero.activaTodas()

        if self.posicion.siBlancas:
            self.rbBlancas.activa(True)
        else:
            self.rbNegras.activa(True)

        # Enroques permitidos
        enroques = self.posicion.enroques
        self.cbBoo.setChecked("K" in enroques)
        self.cbBooo.setChecked("Q" in enroques)
        self.cbNoo.setChecked("k" in enroques)
        self.cbNooo.setChecked("q" in enroques)

        # Otros
        self.edAlPaso.ponTexto(self.posicion.alPaso)
        self.edJugadas.setValue(self.posicion.jugadas)
        self.edMedias.setValue(self.posicion.movPeonCap)

def editarPosicion(win, config, fen=None):
    w = WPosicion(win, config, fen)
    if w.exec_():
        return w.fen
    else:
        return None

class WVoyager(WPosicion):
    def __init__(self, wParent, configuracion, posicion):
        QtGui.QDialog.__init__(self, wParent)

        self.wParent = wParent

        self.setWindowFlags(QtCore.Qt.Tool | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowStaysOnTopHint)

        self.configuracion = configuracion

        self.setWindowTitle(_("Voyager 2"))
        self.setWindowIcon(Iconos.Voyager())

        liAcciones = (  ( _("Quit"), Iconos.MainMenu(), self.accept ), None,
                        ( _("Start position"), Iconos.Inicio(), self.posInicial ), None,
                        ( _("Clear board"), Iconos.Borrar(), self.limpiaTablero ),
                        ( _("Paste FEN position"), Iconos.Pegar(), self.pegarPosicion ),
                        ( _("Copy FEN position"), Iconos.Copiar(), self.copiarPosicion ),
        )

        tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=20)

        # Tablero
        confTablero = configuracion.confTablero("VOYAGER", 24)
        self.posicion = posicion
        self.tablero = Tablero.PosTablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueve)
        self.tablero.mensBorrar = self.borraCasilla
        self.tablero.mensCrear = self.creaCasilla
        self.tablero.mensRepetir = self.repitePieza
        self.ultimaPieza = "P"
        self.piezas = self.tablero.piezas

        self.resetPosicion()

        self.tablero.ponDispatchDrop(self.dispatchDrop)
        self.tablero.baseCasillasSC.setAcceptDrops(True)

        ly = Colocacion.V().control(tb).control(self.tablero).margen(1)
        self.setLayout(ly)

        self.ponCursor()

    def resetPosicion(self):
        self.tablero.ponPosicion(self.posicion)
        self.casillas = self.posicion.casillas
        self.tablero.casillas = self.casillas
        self.tablero.activaTodas()
