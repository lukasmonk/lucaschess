import os

from PIL import Image
from PyQt4 import QtCore, QtGui

from Code import ControlPosicion
from Code import Jugada
from Code import Partida
from Code import Util
from Code import VarGen
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
from Code.QT import Scanner

MODO_POSICION, MODO_PARTIDA = range(2)


def hamming_distance(string, other_string):
    # Adaptation from https://github.com/bunchesofdonald/photohash, MIT license
    """ Computes the hamming distance between two strings. """
    return sum(map(lambda x: 0 if x[0] == x[1] else 1, zip(string, other_string)))


def average_hash(img, hash_size=8):
    # Adaptation from https://github.com/bunchesofdonald/photohash, MIT license
    """ Computes the average hash of the given image. """
    # Open the image, resize it and convert it to black & white.
    image = img.resize((hash_size, hash_size), Image.ANTIALIAS).convert('L')
    pixels = list(image.getdata())

    avg = sum(pixels) / len(pixels)

    # Compute the hash based on each pixels value compared to the average.
    bits = "".join(map(lambda pixel: '1' if pixel > avg else '0', pixels))
    hashformat = "0{hashlength}x".format(hashlength=hash_size ** 2 // 4)
    return int(bits, 2).__format__(hashformat)


class WPosicion(QtGui.QWidget):
    def __init__(self, wparent, is_game, partida):
        self.partida = partida
        self.posicion = partida.iniPosicion
        self.configuracion = configuracion = VarGen.configuracion

        self.is_game = is_game

        self.wparent = wparent

        QtGui.QWidget.__init__(self, wparent)

        liAcciones = (
            (_("Save"), Iconos.GrabarComo(), self.save), None,
            (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
            (_("Start position"), Iconos.Inicio(), self.inicial), None,
            (_("Clear board"), Iconos.Borrar(), self.limpiaTablero),
            (_("Paste FEN position"), Iconos.Pegar16(), self.pegar),
            (_("Copy FEN position"), Iconos.Copiar(), self.copiar),
            (_("Scanner"), Iconos.Scanner(), self.scanner),
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

        dragDropWB = QTVarios.ListaPiezas(self, "P,N,B,R,Q,K", self.tablero, margen=0)
        dragDropBA = QTVarios.ListaPiezas(self, "k,q,r,b,n,p", self.tablero, margen=0)

        self.rbWhite = Controles.RB(self, _("White"), rutina=self.cambiaColor)
        self.rbBlack = Controles.RB(self, _("Black"), rutina=self.cambiaColor)

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

        self.vars_scanner = Scanner.Scanner_vars(self.configuracion.carpetaScanners)

        self.lb_scanner = Controles.LB(self)

        pb_scanner_deduce = Controles.PB(self, _("Deduce"), self.scanner_deduce, plano=False)
        self.chb_scanner_flip = Controles.CHB(self, _("Flip the board"), False).capturaCambiado(self, self.scanner_flip)
        self.pb_scanner_learn = Controles.PB(self, _("Learn"), self.scanner_learn, plano=False)
        self.pb_scanner_learn_quit = Controles.PB(self, "", self.scanner_learn_quit).ponIcono(Iconos.Menos(), tamIcon=24)
        self.pb_scanner_learn_quit.ponToolTip(_("Remove last learned")).anchoFijo(24)

        self.sb_scanner_tolerance, lb_scanner_tolerance = QTUtil2.spinBoxLB(self, self.vars_scanner.tolerance, 3, 20,
                                                                            etiqueta=_("Tolerance"), maxTam=50)

        self.cb_scanner_select, lb_scanner_select = QTUtil2.comboBoxLB(self, [], None, _("OPR"))
        self.cb_scanner_select.capturaCambiado(self.scanner_change)
        pb_scanner_more = Controles.PB(self, "", self.scanner_more).ponIcono(Iconos.Mas())

        self.chb_scanner_ask = Controles.CHB(self, _("Ask before new capture"), self.vars_scanner.ask)

        self.li_scan_pch = []
        self.is_scan_init = False

        # COLOCACION -------------------------------------------------------------------------------------------
        hbox = Colocacion.H().control(self.rbWhite).espacio(15).control(self.rbBlack)
        gbColor = Controles.GB(self, _("Side to play"), hbox)

        ly = Colocacion.G().control(self.cbWoo, 0, 0).control(self.cbBoo, 0, 1)
        ly.control(self.cbWooo, 1, 0).control(self.cbBooo, 1, 1)
        gbEnroques = Controles.GB(self, _("Castling moves possible"), ly)

        ly = Colocacion.G()
        ly.controld(lbMovesPawn, 0, 0, 1, 3).control(self.edMovesPawn, 0, 3)
        ly.controld(lbEnPassant, 1, 0).control(self.edEnPassant, 1, 1)
        ly.controld(lbFullMoves, 1, 2).control(self.edFullMoves, 1, 3)
        gbOtros = Controles.GB(self, "", ly)

        lyT = Colocacion.H().relleno().control(lb_scanner_tolerance).espacio(5).control(self.sb_scanner_tolerance).relleno()
        lyL = Colocacion.H().control(self.pb_scanner_learn).control(self.pb_scanner_learn_quit)
        lyS = Colocacion.H().control(lb_scanner_select).control(self.cb_scanner_select).control(pb_scanner_more)
        ly = Colocacion.V().control(pb_scanner_deduce).control(self.chb_scanner_flip).otro(lyT).otro(lyL).otro(lyS).control(self.chb_scanner_ask)
        self.gb_scanner = Controles.GB(self, "", ly)

        lyG = Colocacion.G()
        lyG.controlc(dragDropBA, 0, 0)
        lyG.control(self.tablero, 1, 0).control(self.lb_scanner, 1, 1)
        lyG.controlc(dragDropWB, 2, 0)
        lyG.controlc(gbColor, 3, 0).controlc(self.gb_scanner, 3, 1, numFilas=2)
        lyG.controlc(gbEnroques, 4, 0)
        lyG.controlc(gbOtros, 5, 0)

        layout = Colocacion.V()
        layout.controlc(self.tb)
        layout.otro(lyG)
        layout.margen(1)
        self.setLayout(layout)

        self.ultimaPieza = "P"
        self.piezas = self.tablero.piezas
        self.resetPosicion()
        self.ponCursor()

        self.lb_scanner.hide()
        self.pb_scanner_learn_quit.hide()
        self.gb_scanner.hide()

    def closeEvent(self, QCloseEvent):
        self.scanner_write()

    def cambiaColor(self):
        self.tablero.ponIndicador(self.rbWhite.isChecked())

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
            self.scanner_write()
            if self.is_game:
                self.wparent.ponModo(MODO_PARTIDA)
            else:
                self.wparent.save()
        else:
            if not siK:
                QTUtil2.mensError(self, _("King") + "-" + _("White") + "???")
                return
            if not sik:
                QTUtil2.mensError(self, _("King") + "-" + _("Black") + "???")
                return

    def cancelar(self):
        self.scanner_write()
        if self.is_game:
            self.wparent.ponModo(MODO_PARTIDA)
        else:
            self.wparent.cancelar()

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
        alPaso = self.edEnPassant.texto().strip()
        if not alPaso:
            alPaso = "-"
        jugadas = self.edFullMoves.value()
        movPeonCap = self.edMovesPawn.value()

        enroques = ""
        for cont, pieza in ((self.cbWoo, "K"), (self.cbWooo, "Q"), (self.cbBoo, "k"), (self.cbBooo, "q")):
            if cont.isChecked():
                enroques += pieza
        if not enroques:
            enroques = "-"
        return siBlancas, alPaso, jugadas, movPeonCap, enroques

    def actPosicion(self):
        self.posicion.siBlancas, self.posicion.alPaso, \
        self.posicion.jugadas, self.posicion.movPeonCap, self.posicion.enroques = self.leeDatos()

    def setPosicion(self, posicion):
        self.posicion = posicion.copia()
        self.resetPosicion()

    # def aceptar(self):
    #     if self.posicion.siExistePieza("K") != 1:
    #         QTUtil2.mensError(self, _("King") + "-" + _("White") + "???")
    #         return
    #     if self.posicion.siExistePieza("k") != 1:
    #         QTUtil2.mensError(self, _("King") + "-" + _("Black") + "???")
    #         return

    #     self.actPosicion()

    #     self.fen = self.posicion.fen()  # Hace control de enroques y EnPassant
    #     if self.fen == ControlPosicion.FEN_INICIAL:
    #         self.fen = ""
    #     self.cierra()
    #     self.accept()

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

    def scanner(self):
        pos = QTUtil.escondeWindow(self.wparent)
        seguir = True
        if self.chb_scanner_ask.valor() and not QTUtil2.pregunta(None, _("Bring the window to scan to front"),
                                                                 etiSi=_("Accept"), etiNo=_("Cancel"), si_top=True):
            seguir = False
        if seguir:
            fich_png = self.configuracion.ficheroTemporal("png")
            if not self.is_scan_init:
                self.scanner_init()
                self.is_scan_init = True

            sc = Scanner.Scanner(self.configuracion.carpetaScanners, fich_png)
            sc.exec_()

            self.vars_scanner.read()
            self.vars_scanner.tolerance = self.sb_scanner_tolerance.valor()  # releemos la variable

            if os.path.isfile(fich_png):
                if Util.tamFichero(fich_png):
                    self.scanner_read_png(fich_png)
                    self.pixmap = QtGui.QPixmap(fich_png)
                    tc = self.tablero.anchoCasilla * 8
                    pm = self.pixmap.scaled(tc, tc)
                    self.lb_scanner.ponImagen(pm)
                    self.lb_scanner.show()
                    self.gb_scanner.show()
                    self.scanner_deduce()

        self.wparent.move(pos)
        self.setFocus()

    def scanner_read_png(self, fdb):
        self.im_scanner = Image.open(fdb)
        self.scanner_process()

    def scanner_process(self):
        im = self.im_scanner
        flipped = self.chb_scanner_flip.isChecked()
        w, h = im.size
        tam = w / 8
        dic = {}
        dic_color = {}
        for f in range(8):
            for c in range(8):
                if flipped:
                    fil = chr(49 + f)
                    col = chr(97 + 7 - c)
                else:
                    fil = chr(49 + 7 - f)
                    col = chr(97 + c)
                x = c * tam + 2
                y = f * tam + 2
                x1 = x + tam - 4
                y1 = y + tam - 4
                im_t = im.crop((x, y, x1, y1))
                pos = "%s%s" % (col, fil)
                dic[pos] = average_hash(im_t, hash_size=8)
                dic_color[pos] = (f + c) % 2 == 0
        self.dicscan_pos_hash = dic
        self.dic_pos_color = dic_color
        siBlancasAbajo = self.tablero.siBlancasAbajo
        if (siBlancasAbajo and flipped) or ((not siBlancasAbajo) and (not flipped)):
            self.tablero.rotaTablero()

    def scanner_flip(self):
        self.scanner_process()
        self.scanner_deduce()

    def scanner_deduce_base(self):
        tolerance = self.sb_scanner_tolerance.valor()
        dic = {}
        for pos, hs in self.dicscan_pos_hash.iteritems():
            pz = None
            dt = 99999999
            cl = self.dic_pos_color[pos]
            for piece, color, hsp in self.li_scan_pch:
                if cl == color:
                    dtp = hamming_distance(hs, hsp)
                    if dtp < dt:
                        pz = piece
                        dt = dtp
                    elif dtp == dt and piece:
                        pz = piece
            if pz and dt <= tolerance:
                dic[pos] = pz
        return dic

    def scanner_deduce(self):
        self.actPosicion()
        fen = "8/8/8/8/8/8/8/8 w KQkq - 0 1"
        if not self.posicion.siBlancas:
            fen = fen.replace("w", "b")
        self.posicion.leeFen(fen)
        self.actPosicion()
        self.resetPosicion()
        dic = self.scanner_deduce_base()
        for pos, pz in dic.iteritems():
            self.ponPieza(pos, pz)

    def scanner_learn(self):
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.tablero.fenActual())

        self.n_scan_last_added = len(self.li_scan_pch)
        dic_deduced = self.scanner_deduce_base()

        for pos, pz_real in cp.casillas.iteritems():
            if pz_real:
                pz_deduced = dic_deduced.get(pos)
                if (not pz_deduced) or (pz_real != pz_deduced):
                    color = self.dic_pos_color[pos]
                    hs = self.dicscan_pos_hash[pos]
                    key = (pz_real, color, hs)
                    self.li_scan_pch.append(key)

        self.scanner_show_learned()

    def scanner_learn_quit(self):
        self.li_scan_pch = self.li_scan_pch[:self.n_scan_last_added]
        self.scanner_show_learned()

    def scanner_more(self):
        name = ""
        while True:
            liGen = []

            config = FormLayout.Editbox(_("Name"), ancho=120)
            liGen.append((config, name))

            resultado = FormLayout.fedit(liGen, title=_("New scanner"), parent=self, anchoMinimo=200,
                                         icon=Iconos.Scanner())
            if resultado:
                accion, liGen = resultado
                name = liGen[0].strip()
                if name:
                    fich = os.path.join(self.configuracion.carpetaScanners, "%s.scn" % name)
                    if Util.existeFichero(fich):
                        QTUtil2.mensError(self, _("This scanner already exists."))
                        continue
                    try:
                        with open(fich, "wb") as f:
                            f.write("")
                        self.scanner_reread(name)
                        return
                    except:
                        QTUtil2.mensError(self, _("This name is not valid to create a scanner file."))
                        continue
            return

    def scanner_init(self):
        scanner = self.vars_scanner.scanner
        self.scanner_reread(scanner)

    def scanner_change(self):
        fich_scanner = self.cb_scanner_select.valor()
        self.vars_scanner.scanner = os.path.basename(fich_scanner)[:-4]
        self.scanner_read()

    def scanner_reread(self, label_default):
        dsc = self.configuracion.carpetaScanners
        lista = [fich for fich in os.listdir(dsc) if fich.endswith(".scn")]
        li = [(fich[:-4], os.path.join(dsc, fich)) for fich in lista]
        fich_default = None
        if not label_default:
            if li:
                label_default, fich_default = li[0]

        for label, fich in li:
            if label == label_default:
                fich_default = fich

        self.cb_scanner_select.rehacer(li, fich_default)
        self.cb_scanner_select.show()
        self.scanner_read()

    def scanner_read(self):
        self.li_scan_pch = []
        self.n_scan_last_save = 0
        self.n_scan_last_added = 0
        fich = self.cb_scanner_select.valor()
        if not fich:
            return
        if Util.tamFichero(fich):
            with open(fich) as f:
                for linea in f:
                    self.li_scan_pch.append(eval(linea.strip()))
        self.n_scan_last_save = len(self.li_scan_pch)
        self.n_scan_last_added = self.n_scan_last_save

        self.scanner_show_learned()

    def scanner_show_learned(self):
        self.pb_scanner_learn.ponTexto("%s (%d)" % (_("Learn"), len(self.li_scan_pch)))
        self.pb_scanner_learn_quit.setVisible(self.n_scan_last_added < len(self.li_scan_pch))

    def scanner_write(self):
        fich_scanner = self.cb_scanner_select.valor()
        if not fich_scanner:
            return

        tam = len(self.li_scan_pch)
        if tam > self.n_scan_last_save:
            with open(fich_scanner, "ab") as q:
                for x in range(self.n_scan_last_save, tam):
                    q.write(str(self.li_scan_pch[x]).replace(" ", ""))
                    q.write("\n")
            self.n_scan_last_save = tam
            self.n_scan_last_added = tam

        self.vars_scanner.scanner = os.path.basename(fich_scanner)[:-4]
        self.vars_scanner.tolerance = self.sb_scanner_tolerance.valor()
        self.vars_scanner.ask = self.chb_scanner_ask.valor()
        self.vars_scanner.write()

    def keyPressEvent(self, event):
        k = event.key()

        if k == QtCore.Qt.Key_W:
            self.actPosicion()
            fen = self.posicion.fen()
            fich = "scanner.fns"
            with open(fich, "ab") as q:
                q.write(fen + "\n")
                QTUtil2.mensajeTemporal(self.parent(), _("Saved in %s") % fich, 0.3)

        elif k == QtCore.Qt.Key_D:
            self.scanner_deduce()

        event.ignore()


class WPGN(QtGui.QWidget):
    def __init__(self, wparent, partida):
        self.partida = partida

        self.wparent = wparent
        self.configuracion = configuracion = VarGen.configuracion
        QtGui.QWidget.__init__(self, wparent)

        liAcciones = (
            (_("Save"), Iconos.Grabar(), self.save), None,
            (_("Cancel"), Iconos.Cancelar(), self.wparent.cancelar), None,
            (_("Start position"), Iconos.Datos(), self.inicial), None,
            (_("Clear"), Iconos.Borrar(), self.limpia), None,
            (_("Take back"), Iconos.Atras(), self.atras), None,
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
        self.pgn = Grid.Grid(self, oColumnas, siCabeceraMovible=False, siSelecFilas=True)
        self.pgn.setMinimumWidth(self.tablero.ancho)

        ly = Colocacion.V().control(self.tb).control(self.tablero)
        ly.control(self.pgn)
        ly.margen(1)
        self.setLayout(ly)

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
            jg = self.partida.jugada(n - 2)
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
        self.tb.setAccionVisible(self.inicial, self.partida.numJugadas() == 0)
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
            self.partida.append_jg(jg)
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
        if self.partida.siEmpiezaConNegras:
            n += 1
        if n % 2:
            n += 1
        return n // 2

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        if col == "NUMERO":
            return str(self.partida.iniPosicion.jugadas + fila)

        siIniBlack = self.partida.siEmpiezaConNegras
        nJug = self.partida.numJugadas()
        if fila == 0:
            w = None if siIniBlack else 0
            b = 0 if siIniBlack else 1
        else:
            n = fila * 2
            w = n - 1 if siIniBlack else n
            b = w + 1
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


class Voyager(QTVarios.WDialogo):
    def __init__(self, owner, is_game, partida):

        titulo = _("Voyager 2") if is_game else _("Start position")
        icono = Iconos.Voyager() if is_game else Iconos.Datos()
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, "voyager")
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Window | QtCore.Qt.WindowStaysOnTopHint)

        self.is_game = is_game
        self.partida = partida
        self.resultado = None

        self.wPos = WPosicion(self, is_game, partida)
        self.wPGN = WPGN(self, partida)

        ly = Colocacion.V().control(self.wPos).control(self.wPGN).margen(0)
        self.setLayout(ly)

        self.ponModo(MODO_PARTIDA if self.is_game else MODO_POSICION)

        self.recuperarVideo(siTam=False)

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
        self.resultado = self.partida.guardaEnTexto() if self.is_game else self.partida.iniPosicion.fen()
        self.guardarVideo()
        self.accept()

    def cancelar(self):
        self.guardarVideo()
        self.reject()


def voyagerFEN(wowner, fen, si_esconde=True, wownerowner=None):
    if si_esconde:
        pos = QTUtil.escondeWindow(wowner)
        if wownerowner:
            pos_ownerowner = QTUtil.escondeWindow(wownerowner)
    partida = Partida.Partida(fen=fen)
    dlg = Voyager(wowner, False, partida)
    resp = dlg.resultado if dlg.exec_() else None
    if si_esconde:
        if wownerowner:
            wownerowner.move(pos_ownerowner)
            wownerowner.show()
        wowner.move(pos)
        wowner.show()

    return resp


def voyagerPartida(wowner, partida):
    pos = QTUtil.escondeWindow(wowner)
    dlg = Voyager(wowner, True, partida)
    resp = dlg.resultado if dlg.exec_() else None
    wowner.move(pos)
    wowner.show()
    return resp

