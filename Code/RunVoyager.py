# -*- coding:latin-1 -*-
import sys

from PyQt4 import QtCore, QtGui

import Code.Util as Util
import Code.VarGen as VarGen
import Code.Configuracion as Configuracion
import Code.ControlPosicion as ControlPosicion
import Code.Partida as Partida
import Code.Jugada as Jugada
import Code.QT.Piezas as Piezas
import Code.Voice as Voice
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Tablero as Tablero
import Code.QT.Columnas as Columnas
import Code.QT.Grid as Grid
import Code.QT.Delegados as Delegados

# Muestra tabla + tablero en pequeño + bloque con datos posicion
# como un gestor solo en pequeño
# minimiza lc, y este espera hasta que se crea el fichero de txt temporal, con consultas cada segundo o 0.5
# permite entrada desde teclado y de voz.
# permite lanzar la pos inicial

# Activa voz si está así en configuración.

# En origen cada 0.5 consulta el fichero de intercambio, con la variable Terminado, si tiene esa variable # None coge
# el testigo y lee el pgn y se muestra setvisible(true).

MODO_POSICION, MODO_PARTIDA=range(2)

class WPosicion(QtGui.QWidget):
    def __init__(self, wparent, cpu):
        self.cpu = cpu
        self.posicion = cpu.partida.iniPosicion
        configuracion = cpu.configuracion

        self.wparent = wparent

        QtGui.QWidget.__init__(self, wparent)

        liAcciones = (
                ( _("Save"), Iconos.GrabarComo(), self.save ), None,
                ( _("Cancel"), Iconos.Cancelar(), self.cancelar ), None,
                ( _("Start position"), Iconos.Inicio(), self.inicial ), None,
                ( _("Clear board"), Iconos.Borrar(), self.limpiaTablero ),
                ( _("Paste FEN position"), Iconos.Pegar16(), self.pegar ),
                ( _("Copy FEN position"), Iconos.Copiar(), self.copiar ),
                ( _("Active voice"), Iconos.S_Microfono(), self.wparent.voice_active ),
                ( _("Deactive voice"), Iconos.X_Microfono(), self.wparent.voice_deactive ),
        )

        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=20)

        confTablero = configuracion.confTablero("VOYAGERPOS", 24)
        self.tablero = Tablero.PosTablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueve)
        self.tablero.mensBorrar = self.borraCasilla
        self.tablero.mensCrear = self.creaCasilla
        self.tablero.mensRepetir = self.repitePieza
        self.tablero.ponDispatchDrop(self.dispatchDrop)
        self.tablero.baseCasillasSC.setAcceptDrops(True)

        self.rbWhite = Controles.RB(self, _("White"))
        self.rbBlack = Controles.RB(self, _("Black"))

        self.cbWoo = Controles.CHB(self, _("White") + " O-O", True)
        self.cbWooo = Controles.CHB(self, _("White") + " O-O-O", True)
        self.cbBoo = Controles.CHB(self, _("Black") + " O-O", True)
        self.cbBooo = Controles.CHB(self, _("Black") + " O-O-O", True)

        lbEnPassant = Controles.LB(self, _("En passant") + ":")
        self.edEnPassant = Controles.ED(self).controlrx("(-|[a-h][36])").anchoFijo(30)

        self.edMovesPawn, lbMovesPawn = QTUtil2.spinBoxLB(self, 0, 0, 999,
                                                    etiqueta=_("Halfmove clock"),
                                                    maxTam=50)

        self.edFullMoves, lbFullMoves = QTUtil2.spinBoxLB(self, 1, 1, 999, etiqueta=_("Fullmove number"), maxTam=50)

        # COLOCACION -------------------------------------------------------------------------------------------
        hbox = Colocacion.H().relleno().control(self.rbWhite).espacio(15).control(self.rbBlack).relleno()
        gbColor = Controles.GB(self, _("Next move"), hbox)

        ly = Colocacion.G().control(self.cbWoo, 0, 0).control(self.cbBoo, 0, 1)
        ly.control(self.cbWooo, 1, 0).control(self.cbBooo, 1, 1)
        gbEnroques = Controles.GB(self, _("Castling moves possible"), ly)

        ly = Colocacion.G()
        ly.controld(lbMovesPawn, 0, 0, 1, 3).control(self.edMovesPawn, 0, 3)
        ly.controld(lbEnPassant, 1, 0).control(self.edEnPassant, 1, 1)
        ly.controld(lbFullMoves, 1, 2).control(self.edFullMoves, 1, 3)
        gbOtros = Controles.GB(self, "", ly)

        ly = Colocacion.V().control(self.tb).control(self.tablero)
        ly.control(gbColor).control(gbEnroques).control(gbOtros)
        ly.margen(1)
        self.setLayout(ly)

        self.ultimaPieza = "P"
        self.piezas = self.tablero.piezas
        self.resetPosicion()
        self.ponCursor()

        self.queueVoice = []
        self.bufferVoice = ""

    def save(self):
        self.actPosicion()
        siK = False
        sik = False
        for p in self.casillas.itervalues():
            if p == "K":
                siK = True
            elif p == "k":
                sik = True
        if siK and sik:
            self.wparent.setPosicion(self.posicion)
        self.wparent.ponModo(MODO_PARTIDA)

    def cancelar(self):
        self.wparent.ponModo(MODO_PARTIDA)
        self.voice_deactive()

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
        self.ponCursor(QtCore.Qt.ArrowCursor)

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
        siBlancas = self.rbWhite.isChecked()
        EnPassant = self.edEnPassant.texto().strip()
        if not EnPassant:
            EnPassant = "-"
        jugadas = self.edFullMoves.value()
        movPeonCap = self.edMovesPawn.value()

        enroques = ""
        for cont, pieza in ( (self.cbWoo, "K" ), (self.cbWooo, "Q" ), (self.cbBoo, "k"), (self.cbBooo, "q") ):
            if cont.isChecked():
                enroques += pieza
        if not enroques:
            enroques = "-"
        return siBlancas, EnPassant, jugadas, movPeonCap, enroques

    def actPosicion(self):
        self.posicion.siBlancas, self.posicion.EnPassant, \
        self.posicion.jugadas, self.posicion.movPeonCap, self.posicion.enroques = self.leeDatos()

    def setPosicion(self, posicion):
        self.posicion = posicion.copia()
        self.resetPosicion()

    def aceptar(self):
        if self.posicion.siExistePieza("K") != 1:
            QTUtil2.mensError(self, _("King") + "-" + _("White") + "???")
            return
        if self.posicion.siExistePieza("k") != 1:
            QTUtil2.mensError(self, _("King") + "-" + _("Black") + "???")
            return

        self.actPosicion()

        self.fen = self.posicion.fen()  # Hace control de enroques y EnPassant
        if self.fen == ControlPosicion.FEN_INICIAL:
            self.fen = ""
        self.cierra()
        self.accept()

    def pegar(self):
        cb = QtGui.QApplication.clipboard()
        fen = cb.text()
        if fen:
            try:
                self.posicion.leeFen(str(fen))
                self.resetPosicion()
            except:
                pass

    def copiar(self):
        cb = QtGui.QApplication.clipboard()
        self.actPosicion()
        cb.setText(self.posicion.fen())

    def limpiaTablero(self):
        self.posicion.leeFen("8/8/8/8/8/8/8/8 w - - 0 1")
        self.resetPosicion()

    def inicial(self):
        self.posicion.posInicial()
        self.resetPosicion()

    def resetPosicion(self):
        self.tablero.ponPosicion(self.posicion)
        self.casillas = self.posicion.casillas
        self.tablero.casillas = self.casillas
        self.tablero.activaTodas()

        if self.posicion.siBlancas:
            self.rbWhite.activa(True)
        else:
            self.rbBlack.activa(True)

        # Enroques permitidos
        enroques = self.posicion.enroques
        self.cbWoo.setChecked("K" in enroques)
        self.cbWooo.setChecked("Q" in enroques)
        self.cbBoo.setChecked("k" in enroques)
        self.cbBooo.setChecked("q" in enroques)

        # Otros
        self.edEnPassant.ponTexto(self.posicion.alPaso)
        self.edFullMoves.setValue(self.posicion.jugadas)
        self.edMovesPawn.setValue(self.posicion.movPeonCap)

    def setVoice(self):
        v_a = v_d = False
        if self.wparent.isVoice:
            if self.wparent.isVoiceActive:
                v_d = True
            else:
                v_a = True
        self.tb.setAccionVisible(self.wparent.voice_active, v_a)
        self.tb.setAccionVisible(self.wparent.voice_deactive, v_d)

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

class WPGN(QtGui.QWidget):
    def __init__(self, wparent, cpu):
        self.cpu = cpu
        self.partida = self.cpu.partida

        self.wparent = wparent
        self.configuracion = configuracion = cpu.configuracion
        QtGui.QWidget.__init__(self, wparent)

        liAcciones = (
                ( _("Save"), Iconos.Grabar(), self.save ), None,
                ( _("Start position"), Iconos.Datos(), self.inicial ), None,
                ( _("Clear"), Iconos.Borrar(), self.limpia ), None,
                ( _("Take back"), Iconos.Atras(), self.atras ), None,
                ( _("Active voice"), Iconos.S_Microfono(), self.wparent.voice_active ),
                ( _("Deactive voice"), Iconos.X_Microfono(), self.wparent.voice_deactive ),
        )

        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=20)

        confTablero = configuracion.confTablero("VOYAGERPGN", 24)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueveHumano)
        Delegados.generaPM(self.tablero.piezas)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 35, siCentrado=True)
        self.siFigurinesPGN = configuracion.figurinesPGN
        nAnchoColor = (self.tablero.ancho - 35 - 20) / 2
        oColumnas.nueva("BLANCAS", _("White"), nAnchoColor, edicion=Delegados.EtiquetaPGN(True if self.siFigurinesPGN else None))
        oColumnas.nueva("NEGRAS", _("Black"), nAnchoColor, edicion=Delegados.EtiquetaPGN(False if self.siFigurinesPGN else None))
        self.pgn = Grid.Grid(self, oColumnas, siCabeceraMovible=False,siSelecFilas=True)
        self.pgn.setMinimumWidth(self.tablero.ancho)

        ly = Colocacion.V().control(self.tb).control(self.tablero)
        ly.control(self.pgn)
        ly.margen(1)
        self.setLayout(ly)

        self.liVoice = []

        self.tablero.ponPosicion(self.partida.ultPosicion)
        self.siguienteJugada()

    def save(self):
        self.wparent.save()

    def limpia(self):
        self.partida.liJugadas = []
        self.partida.ultPosicion = self.partida.iniPosicion
        self.tablero.ponPosicion(self.partida.iniPosicion)
        self.siguienteJugada()

    def atras(self):
        n = self.partida.numJugadas()
        if n:
            self.partida.liJugadas = self.partida.liJugadas[:-1]
            jg = self.partida.jugada(n-2)
            if jg:
                self.partida.ultPosicion = jg.posicion
                self.tablero.ponPosicion(jg.posicion)
                self.tablero.ponFlechaSC(jg.desde, jg.hasta)
            else:
                self.partida.ultPosicion = self.partida.iniPosicion
                self.tablero.ponPosicion(self.partida.iniPosicion)
            self.siguienteJugada()

    def inicial(self):
        self.wparent.ponModo(MODO_POSICION)

    def siguienteJugada(self):
        self.tb.setAccionVisible(self.inicial,self.partida.numJugadas() == 0)
        if self.partida.siTerminada():
            self.tablero.desactivaTodas()
            return
        self.pgn.refresh()
        self.pgn.gobottom()
        self.tablero.activaColor(self.partida.ultPosicion.siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        if not coronacion and self.partida.ultPosicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.partida.ultPosicion.siBlancas)
            if coronacion is None:
                return False

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)

        if siBien:
            self.partida.liJugadas.append(jg)
            self.partida.ultPosicion = jg.posicion
            if self.partida.siTerminada():
                jg.siJaqueMate = jg.siJaque
                jg.siAhogado = not jg.siJaque
            resp = self.partida.si3repetidas()
            if resp:
                jg.siTablasRepeticion = True
            elif self.partida.ultPosicion.movPeonCap >= 100:
                jg.siTablas50 = True
            elif self.partida.ultPosicion.siFaltaMaterial():
                jg.siTablasFaltaMaterial = True
            self.tablero.ponPosicion(jg.posicion)
            self.tablero.ponFlechaSC(jg.desde, jg.hasta)

            self.siguienteJugada()
            return True
        else:
            return False

    def gridNumDatos(self, grid):
        n = self.partida.numJugadas()
        if not n:
            return 0
        if  self.partida.siEmpiezaConNegras:
            n += 1
        if n%2:
            n+= 1
        return n//2

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "NUMERO":
            return str(self.partida.iniPosicion.jugadas+fila)

        siIniBlack = self.partida.siEmpiezaConNegras
        nJug = self.partida.numJugadas()
        if fila == 0:
            w = None if siIniBlack else 0
            b = 0 if siIniBlack else 1
        else:
            n = fila*2
            w = n-1 if siIniBlack else n
            b = w+1
        if b >= nJug:
            b = None

        def xjug(n):
            if n is None:
                return ""
            jg = self.partida.jugada(n)
            if self.siFigurinesPGN:
                return jg.pgnFigurinesSP()
            else:
                return jg.pgnSP()
        if col == "BLANCAS":
            return xjug(w)
        else:
            return xjug(b)

    def voice(self, txt):
        if "TAKEBACK" in txt:
            self.atras()
            self.liVoice = []
            return
        self.liVoice.extend(txt.split(" "))
        while True:
            resp = Voice.readPGN(self.liVoice, self.partida.ultPosicion)
            if resp is None:
                self.liVoice = []
                return
            else:
                siMove, move, nGastados = resp
                if siMove:
                    self.mueveHumano(move.desde(), move.hasta(), move.coronacion())
                    n = len(self.liVoice)
                    if nGastados >= n:
                        self.liVoice = []
                        return
                    else:
                        self.liVoice = self.liVoice[nGastados:]
                        continue
                else:
                    return

class Voyager(QtGui.QDialog):
    def __init__(self, cpu):
        QtGui.QDialog.__init__(self)

        configuracion = cpu.configuracion
        self.cpu = cpu
        self.partida = cpu.partida

        self.setWindowFlags(QtCore.Qt.Tool | QtCore.Qt.WindowStaysOnTopHint)

        self.setWindowTitle(_("Voyager 2").replace("2", "1"))
        self.setWindowIcon(Iconos.Voyager())

        self.wPos = WPosicion(self, cpu)
        self.wPGN = WPGN(self, cpu)

        ly = Colocacion.V().control(self.wPos).control(self.wPGN).margen(0)
        self.setLayout(ly)

        if configuracion.voice:
            self.isVoice = True
            self.isVoiceActive = True
            self.voice_active()
        else:
            self.isVoice = False
            self.isVoiceActive = False
            self.voice_deactive()

        self.ponModo(MODO_PARTIDA)

    def voice(self, txt):
        if self.modo == MODO_POSICION:
            self.wPos.voice(txt)
        else:
            self.wPGN.voice(txt)

    def ponModo(self, modo):
        self.modo = modo
        if modo == MODO_POSICION:
            self.wPos.setPosicion(self.partida.iniPosicion)
            self.wPGN.setVisible(False)
            self.wPos.setVisible(True)
        else:
            self.wPos.setVisible(False)
            self.wPGN.setVisible(True)

    def setPosicion(self, posicion):
        self.partida.iniPosicion = posicion
        self.wPGN.limpia()

    def save(self):
        self.cierra(self.partida.guardaEnTexto())
        self.accept()

    def cierra(self, valor):
        Voice.runVoice.close()
        self.cpu.setRaw(valor)

    def closeEvent(self, event):
        self.cierra(None)
        event.accept()

    def setVoice(self):
        v_a = v_d = False
        if self.isVoice:
            if self.isVoiceActive:
                v_d = True
            else:
                v_a = True
        self.wPGN.tb.setAccionVisible(self.voice_active, v_a)
        self.wPGN.tb.setAccionVisible(self.voice_deactive, v_d)
        self.wPos.tb.setAccionVisible(self.voice_active, v_a)
        self.wPos.tb.setAccionVisible(self.voice_deactive, v_d)

    def voice_active(self):
        self.isVoiceActive = True
        Voice.runVoice.setConf(self, False)
        Voice.runVoice.start(self.voice)
        self.setVoice()

    def voice_deactive(self):
        self.isVoiceActive = False
        Voice.runVoice.close()
        self.setVoice()

class CPU():
    def __init__(self, fdb):
        self.fdb = fdb
        self.getRaw()
        VarGen.todasPiezas = Piezas.TodasPiezas()
        VarGen.configuracion = self.configuracion

    def getRaw(self):
        db = Util.DicRaw(self.fdb)
        self.configuracion = Configuracion.Configuracion(db["USER"])
        self.configuracion.lee()
        self.configuracion.leeConfTableros()
        self.partida = Partida.Partida()
        txt = db["PARTIDA"]
        if txt:
            self.partida.recuperaDeTexto(txt)
        # posicion = ControlPosicion.ControlPosicion()
        # posicion.leeFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 5")
        # self.partida = Partida.Partida(posicion)
        db.close()

    def setRaw(self, txtpartida):
        db = Util.DicRaw(self.fdb)
        db["TXTGAME"] = txtpartida
        db.close()

    def run(self):
        app = QtGui.QApplication([])

        app.setStyle(QtGui.QStyleFactory.create("CleanLooks"))
        QtGui.QApplication.setPalette(QtGui.QApplication.style().standardPalette())

        w = Voyager(self)
        w.exec_()

def run(fdb):
    ferr = open("bug.voyager", "at")
    sys.stderr = ferr

    cpu = CPU(fdb)
    cpu.run()

    ferr.close()
