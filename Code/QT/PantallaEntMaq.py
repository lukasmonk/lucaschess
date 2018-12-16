import os

from PyQt4 import QtCore, QtGui

from Code import Books
from Code import ControlPosicion
from Code import Personalidades
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import Motores
from Code.QT import PantallaAperturas
from Code.QT import PantallaMotores
from Code.QT import PantallaTutor
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Util
from Code.QT import Voyager
from Code.Constantes import *


class WEntMaquina(QTVarios.WDialogo):
    def __init__(self, procesador, titulo):

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, Iconos.Libre(), "entMaquina")

        self.configuracion = procesador.configuracion
        self.procesador = procesador

        self.personalidades = Personalidades.Personalidades(self, self.configuracion)

        self.motores = Motores.Motores(self.configuracion)

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      (_("Configurations"), Iconos.Configurar(), self.configuraciones), None,
                      ]
        tb = QTVarios.LCTB(self, liAcciones)

        # Tab
        tab = Controles.Tab()

        def nuevoG():
            lyG = Colocacion.G()
            lyG.filaActual = 0
            return lyG

        gbStyle = """
            QGroupBox {
                font: bold 16px;
                background-color: #F2F2EC;/*qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #E0E0E0, stop: 1 #FFFFFF);*/
                border: 1px solid gray;
                border-radius: 3px;
                margin-top: 5ex; /* leave space at the top for the title */
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top center; /* position at the top center */
                padding: 0 3px;
             }
        """

        def _label(lyG, txt, ly, rutinaCHB=None, siCheck=False):
            gb = Controles.GB(self, txt, ly)
            if rutinaCHB:
                gb.conectar(rutinaCHB)
            elif siCheck:
                gb.setCheckable(True)
                gb.setChecked(False)

            gb.setStyleSheet(gbStyle)
            lyG.controlc(gb, lyG.filaActual, 0)
            lyG.filaActual += 1
            return gb

        # TAB General
        lyG = nuevoG()

        # # Color
        self.rbBlancas = Controles.RB(self, "").activa()
        self.rbBlancas.setIcon(QTVarios.fsvg2ico("Pieces/Chessicons/wp.svg", 64))
        self.rbNegras = Controles.RB(self, "")
        self.rbNegras.setIcon(QTVarios.fsvg2ico("Pieces/Chessicons/bp.svg", 64))
        self.rbRandom = Controles.RB(self, _("Random"))
        hbox = Colocacion.H().relleno().control(self.rbBlancas).espacio(30).control(self.rbNegras).espacio(30).control(
                self.rbRandom).relleno()
        _label(lyG, _("Select color"), hbox)

        # # Motores
        liDepths = [("--", 0)]
        for x in range(1, 31):
            liDepths.append((str(x), x))

        # ## Rival
        self.rival = self.configuracion.rivalInicial
        self.rivalTipo = Motores.INTERNO
        self.btRival = Controles.PB(self, "", self.cambiaRival, plano=False)
        self.edRtiempo = Controles.ED(self).tipoFloat().anchoMaximo(50)
        self.cbRdepth = Controles.CB(self, liDepths, 0).capturaCambiado(self.cambiadoDepth)
        lbTiempoSegundosR = Controles.LB2P(self, _("Time"))
        lbNivel = Controles.LB2P(self, _("Depth"))

        # ## Ajustar rival
        liAjustes = self.personalidades.listaAjustes(True)
        self.cbAjustarRival = Controles.CB(self, liAjustes, kAjustarMejor).capturaCambiado(self.ajustesCambiado)
        lbAjustarRival = Controles.LB2P(self, _("Set strength"))
        btAjustarRival = Controles.PB(self, _("Personality"), self.cambiaPersonalidades, plano=True).ponIcono(
                Iconos.Mas(), tamIcon=16)

        # ## Resign
        lbResign = Controles.LB2P(self, _("Resign/draw by engine"))
        liResign = ((_("Very early"), -100),
                    (_("Early"), -300),
                    (_("Average"), -500),
                    (_("Late"), -800),
                    (_("Very late"), -1000),
                    (_("Never"), -9999999))
        self.cbResign = Controles.CB(self, liResign, -800)

        lyH1 = Colocacion.H().control(self.btRival).espacio(20)
        lyH1.control(lbTiempoSegundosR).control(self.edRtiempo)
        lyH1.control(lbNivel).control(self.cbRdepth).relleno()
        lyH2 = Colocacion.H().control(lbAjustarRival).control(self.cbAjustarRival).control(btAjustarRival).relleno()
        lyH3 = Colocacion.H().control(lbResign).control(self.cbResign).relleno()
        ly = Colocacion.V().otro(lyH1).otro(lyH2).otro(lyH3)
        _label(lyG, _("Opponent"), ly)

        gb = Controles.GB(self, "", lyG)
        tab.nuevaTab(gb, _("Basic configuration"))

        # TAB Ayudas
        lyG = nuevoG()

        self.cbAtras = Controles.CHB(self, _("Takeback"), True)
        self.chbSummary = Controles.CHB(self, _("Save a summary when the game is finished in the main comment"), False)

        # # Tutor
        lbAyudas = Controles.LB2P(self, _("Available hints"))
        liAyudas = [(_("Maximum"), 99)]
        for i in range(1, 21):
            liAyudas.append((str(i), i))
        for i in range(25, 51, 5):
            liAyudas.append((str(i), i))
        self.cbAyudas = Controles.CB(self, liAyudas, 99)
        self.chbChance = Controles.CHB(self, _("Second chance"), True)
        btTutorChange = Controles.PB(self, _("Tutor change"), self.tutorChange, plano=False).ponIcono(Iconos.Tutor(), tamIcon=16)

        liThinks = [(_("Nothing"), -1), (_("Score"), 0)]
        for i in range(1, 5):
            liThinks.append(("%d %s" % (i, _("ply") if i == 1 else _("plies")), i))
        liThinks.append((_("All"), 9999))
        lbThoughtTt = Controles.LB(self, _("It is showed") + ":")
        self.cbThoughtTt = Controles.CB(self, liThinks, -1)

        self.chbContinueTt = Controles.CHB(self, _("The tutor thinks while you think"), True)

        lbBoxHeight = Controles.LB2P(self, _("Box height"))
        self.sbBoxHeight = Controles.SB(self, 7, 0, 999).tamMaximo(50)

        lbArrows = Controles.LB2P(self, _("Arrows with the best moves"))
        self.sbArrowsTt = Controles.SB(self, 3, 0, 999).tamMaximo(50)

        lyT1 = Colocacion.H().control(lbAyudas).control(self.cbAyudas).relleno()
        lyT1.control(self.chbChance).relleno().control(btTutorChange)
        lyT2 = Colocacion.H().control(self.chbContinueTt).relleno()
        lyT2.control(lbBoxHeight).control(self.sbBoxHeight).relleno()
        lyT3 = Colocacion.H().control(lbThoughtTt).control(self.cbThoughtTt).relleno()
        lyT3.control(lbArrows).control(self.sbArrowsTt)

        ly = Colocacion.V().otro(lyT1).espacio(16).otro(lyT2).otro(lyT3).relleno()

        self.gbTutor = Controles.GB(self, _("Tutor"), ly)
        self.gbTutor.setCheckable(True)
        self.gbTutor.setStyleSheet(gbStyle)

        lb = Controles.LB(self, _("It is showed") + ":")
        self.cbThoughtOp = Controles.CB(self, liThinks, -1)
        lbArrows = Controles.LB2P(self, _("Arrows to show"))
        self.sbArrows = Controles.SB(self, 7, 0, 999).tamMaximo(50)
        ly = Colocacion.H().control(lb).control(self.cbThoughtOp).relleno()
        ly.control(lbArrows).control(self.sbArrows).relleno()
        gbThoughtOp = Controles.GB(self, _("Thought of the opponent"), ly)
        gbThoughtOp.setStyleSheet(gbStyle)

        ly = Colocacion.V().espacio(16).control(self.gbTutor).espacio(16).control(gbThoughtOp)
        ly.espacio(16).control(self.cbAtras).control(self.chbSummary)
        gb = Controles.GB(self, "", ly)
        tab.nuevaTab(gb, _("Help configuration"))

        # TAB Tiempo
        lyG = nuevoG()
        self.edMinutos, self.lbMinutos = QTUtil2.spinBoxLB(self, 15, 0, 999, maxTam=50, etiqueta=_("Total minutes"))
        self.edSegundos, self.lbSegundos = QTUtil2.spinBoxLB(self, 6, -999, 999, maxTam=54,
                                                             etiqueta=_("Seconds added per move"))
        self.edMinExtra, self.lbMinExtra = QTUtil2.spinBoxLB(self, 0, -999, 999, maxTam=70,
                                                             etiqueta=_("Extra minutes for the player"))
        self.edZeitnot, self.lbZeitnot = QTUtil2.spinBoxLB(self, 0, -999, 999, maxTam=54,
                                                           etiqueta=_("Zeitnot: alarm sounds when remaining seconds"))
        lyH1 = Colocacion.H()
        lyH1.control(self.lbMinutos).control(self.edMinutos).espacio(30)
        lyH1.control(self.lbSegundos).control(self.edSegundos).relleno()
        lyH2 = Colocacion.H()
        lyH2.control(self.lbMinExtra).control(self.edMinExtra).relleno()
        lyH3 = Colocacion.H()
        lyH3.control(self.lbZeitnot).control(self.edZeitnot).relleno()
        ly = Colocacion.V().otro(lyH1).otro(lyH2).otro(lyH3)
        self.chbTiempo = _label(lyG, _("Time"), ly, siCheck=True)

        gb = Controles.GB(self, "", lyG)
        tab.nuevaTab(gb, _("Time"))

        # TAB Initial moves

        lyG = nuevoG()

        # Posicion
        self.btPosicion = Controles.PB(self, " " * 5 + _("Change") + " " * 5, self.posicionEditar).ponPlano(False)
        self.fen = ""
        self.btPosicionQuitar = Controles.PB(self, "", self.posicionQuitar).ponIcono(Iconos.Motor_No())
        self.btPosicionPegar = Controles.PB(self, "", self.posicionPegar).ponIcono(Iconos.Pegar16()).ponToolTip(
                _("Paste FEN position"))
        hbox = Colocacion.H().relleno().control(self.btPosicionQuitar).control(self.btPosicion).control(
                self.btPosicionPegar).relleno()
        _label(lyG, _("Start position"), hbox)

        # Aperturas
        self.btApertura = Controles.PB(self, " " * 5 + _("Undetermined") + " " * 5, self.editarApertura).ponPlano(False)
        self.bloqueApertura = None
        self.btAperturasFavoritas = Controles.PB(self, "", self.aperturasFavoritas).ponIcono(Iconos.Favoritos())
        self.btAperturasQuitar = Controles.PB(self, "", self.aperturasQuitar).ponIcono(Iconos.Motor_No())
        hbox = Colocacion.H().relleno().control(self.btAperturasQuitar).control(self.btApertura).control(
                self.btAperturasFavoritas).relleno()
        _label(lyG, _("Opening"), hbox)

        # Libros
        fvar = self.configuracion.ficheroBooks
        self.listaLibros = Books.ListaLibros()
        self.listaLibros.recuperaVar(fvar)
        # Comprobamos que todos esten accesibles
        self.listaLibros.comprueba()
        li = [(x.nombre, x) for x in self.listaLibros.lista]
        libInicial = li[0][1] if li else None
        self.cbBooks = QTUtil2.comboBoxLB(self, li, libInicial)
        self.btNuevoBook = Controles.PB(self, "", self.nuevoBook, plano=True).ponIcono(Iconos.Mas(), tamIcon=16)
        self.chbBookMandatory = Controles.CHB(self, _("Mandatory"), False)
        # Respuesta rival
        li = (
            (_("Selected by the player"), "su"),
            (_("Uniform random"), "au"),
            (_("Proportional random"), "ap"),
            (_("Always the highest percentage"), "mp"),
        )
        self.cbBooksRR = QTUtil2.comboBoxLB(self, li, "mp")
        self.lbBooksRR = Controles.LB2P(self, _("Opponent's move"))
        hbox = Colocacion.H().relleno().control(self.cbBooks).control(self.btNuevoBook).control(
                self.chbBookMandatory).relleno()
        hboxRR = Colocacion.H().relleno().control(self.lbBooksRR).control(self.cbBooksRR).relleno()
        hboxV = Colocacion.V().otro(hbox).otro(hboxRR)
        self.chbBook = _label(lyG, _("Book"), hboxV, siCheck=True)

        gb = Controles.GB(self, "", lyG)
        tab.nuevaTab(gb, _("Initial moves"))

        layout = Colocacion.V().control(tb).control(tab).relleno().margen(3)

        self.setLayout(layout)

        self.liAperturasFavoritas = []
        self.btAperturasFavoritas.hide()

        dic = Util.recuperaDIC(self.configuracion.ficheroEntMaquina)
        if not dic:
            dic = {}
        self.muestraDic(dic)

        self.ajustesCambiado()
        # self.ayudasCambiado()
        self.ponRival()

        self.recuperarVideo()

    def configuraciones(self):
        fichero = self.configuracion.ficheroEntMaquinaConf
        dbc = Util.DicSQL(fichero)
        liConf = dbc.keys(siOrdenados=True)
        menu = Controles.Menu(self)
        SELECCIONA, BORRA, AGREGA = range(3)
        for x in liConf:
            menu.opcion((SELECCIONA, x), x, Iconos.PuntoAzul())
        menu.separador()
        menu.opcion((AGREGA, None), _("Save current configuration"), Iconos.Mas())
        if liConf:
            menu.separador()
            submenu = menu.submenu(_("Remove"), Iconos.Delete())
            for x in liConf:
                submenu.opcion((BORRA, x), x, Iconos.PuntoRojo())
        resp = menu.lanza()

        if resp:
            op, k = resp

            if op == SELECCIONA:
                dic = dbc[k]
                self.muestraDic(dic)
            elif op == BORRA:
                if QTUtil2.pregunta(self, _X(_("Delete %1 ?"), k)):
                    del dbc[k]
            elif op == AGREGA:
                liGen = [(None, None)]

                liGen.append((_("Name") + ":", ""))

                resultado = FormLayout.fedit(liGen, title=_("Name"), parent=self, icon=Iconos.Libre())
                if resultado:
                    accion, liGen = resultado

                    nombre = liGen[0].strip()
                    if nombre:
                        dbc[nombre] = self.creaDic()

        dbc.close()

    def cambiaRival(self):
        resp = self.motores.menu(self)
        if resp:
            tp, cm = resp
            if tp == Motores.EXTERNO and cm is None:
                self.motoresExternos()
                return
            elif tp == Motores.MICPER:
                cm = PantallaMotores.eligeMotorEntMaq(self)
                if not cm:
                    return
            self.rivalTipo = tp
            self.rival = cm
            self.ponRival()

    def ponRival(self):
        self.btRival.ponTexto("   %s   " % self.rival.nombre)
        self.btRival.ponIcono(self.motores.dicIconos[self.rivalTipo])

    def cambiaPersonalidades(self):
        siRehacer = self.personalidades.lanzaMenu()
        if siRehacer:
            actual = self.cbAjustarRival.valor()
            self.cbAjustarRival.rehacer(self.personalidades.listaAjustes(True), actual)

    def ajustesCambiado(self):
        resp = self.cbAjustarRival.valor()
        if resp is None:
            self.cbAjustarRival.ponValor(kAjustarNivelAlto)

    def cambiadoDepth(self, num):
        if num > 0:
            self.edRtiempo.ponFloat(0.0)
        self.edRtiempo.setEnabled(num == 0)

    def posicionEditar(self):
        resp = Voyager.voyagerFEN(self, self.fen, wownerowner=self.procesador.pantalla)
        if resp is not None:
            self.fen = resp
            self.muestraPosicion()

    def posicionPegar(self):
        texto = QTUtil.traePortapapeles()
        if texto:
            cp = ControlPosicion.ControlPosicion()
            try:
                cp.leeFen(texto.strip())
                self.fen = cp.fen()
                if self.fen == ControlPosicion.FEN_INICIAL:
                    self.fen = ""
                self.muestraPosicion()
            except:
                pass

    def muestraPosicion(self):
        if self.fen:
            rotulo = self.fen
            self.btPosicionQuitar.show()
            self.btPosicionPegar.show()
            self.bloqueApertura = None
            self.muestraApertura()
        else:
            rotulo = _("Change")
            self.btPosicionQuitar.hide()
            self.btPosicionPegar.show()
        rotulo = " " * 5 + rotulo + " " * 5
        self.btPosicion.ponTexto(rotulo)

    def editarApertura(self):
        self.btApertura.setDisabled(True)  # Puede tardar bastante tiempo
        me = QTUtil2.unMomento(self)
        w = PantallaAperturas.WAperturas(self, self.configuracion, self.bloqueApertura)
        me.final()
        self.btApertura.setDisabled(False)
        if w.exec_():
            self.bloqueApertura = w.resultado()
            self.muestraApertura()

    def aperturasFavoritas(self):
        if len(self.liAperturasFavoritas) == 0:
            return
        menu = QTVarios.LCMenu(self)
        menu.setToolTip(_("To choose: <b>left button</b> <br>To erase: <b>right button</b>"))
        f = Controles.TipoLetra(puntos=8, peso=75)
        menu.ponFuente(f)
        nPos = 0
        for nli, bloque in enumerate(self.liAperturasFavoritas):
            if type(bloque) == tuple:  # compatibilidad con versiones anteriores
                bloque = bloque[0]
                self.liAperturasFavoritas[nli] = bloque
            menu.opcion(bloque, bloque.trNombre, Iconos.PuntoVerde())
            nPos += 1

        resp = menu.lanza()
        if resp:
            if menu.siIzq:
                self.bloqueApertura = resp
                self.muestraApertura()
            elif menu.siDer:
                bloqueApertura = resp
                if QTUtil2.pregunta(self,
                                    _X(_("Do you want to delete the opening %1 from the list of favourite openings?"),
                                       bloqueApertura.trNombre)):
                    del self.liAperturasFavoritas[nPos]

    def muestraApertura(self):
        if self.bloqueApertura:
            rotulo = self.bloqueApertura.trNombre + "\n" + self.bloqueApertura.pgn
            self.btAperturasQuitar.show()
            self.fen = ""
            self.muestraPosicion()
        else:
            rotulo = " " * 3 + _("Undetermined") + " " * 3
            self.btAperturasQuitar.hide()
        self.btApertura.ponTexto(rotulo)

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def creaDic(self):
        dic = {}
        dic["COLOR"] = "B" if self.rbBlancas.isChecked() else ("N" if self.rbNegras.isChecked() else "R")
        dic["FEN"] = self.fen
        dic["APERTURA"] = self.bloqueApertura

        dic["AYUDAS"] = self.cbAyudas.valor() if self.gbTutor.isChecked() else 0
        dic["ATRAS"] = self.cbAtras.valor()
        dic["ARROWS"] = self.sbArrows.valor()
        dic["BOXHEIGHT"] = self.sbBoxHeight.valor()
        dic["THOUGHTOP"] = self.cbThoughtOp.valor()
        dic["THOUGHTTT"] = self.cbThoughtTt.valor()
        dic["ARROWSTT"] =  self.sbArrowsTt.valor()
        dic["CONTINUETT"] = self.chbContinueTt.isChecked()
        dic["2CHANCE"] = self.chbChance.isChecked()
        dic["SUMMARY"] = self.chbSummary.isChecked()

        dr = dic["RIVAL"] = {}
        dr["MOTOR"] = self.rival.clave
        dr["TIPO"] = self.rivalTipo
        dr["TIEMPO"] = int(self.edRtiempo.textoFloat() * 10)
        dr["PROFUNDIDAD"] = self.cbRdepth.valor()
        dr["RESIGN"] = self.cbResign.valor()

        dic["SITIEMPO"] = self.chbTiempo.isChecked()
        if dic["SITIEMPO"]:
            dic["MINUTOS"] = self.edMinutos.value()
            dic["SEGUNDOS"] = self.edSegundos.value()
            dic["MINEXTRA"] = self.edMinExtra.value()
            dic["ZEITNOT"] = self.edZeitnot.value()

        dic["AJUSTAR"] = self.cbAjustarRival.valor()

        siBook = self.chbBook.isChecked()
        dic["BOOK"] = self.cbBooks.valor() if siBook else None
        dic["BOOKRR"] = self.cbBooksRR.valor() if siBook else None
        dic["BOOKMANDATORY"] = self.chbBookMandatory.isChecked() if siBook else None

        dic["APERTURASFAVORITAS"] = self.liAperturasFavoritas

        return dic

    def aceptar(self):
        self.dic = self.creaDic()
        Util.guardaDIC(self.dic, self.configuracion.ficheroEntMaquina)

        # Info para el gestor, sin grabar
        dr = self.dic["RIVAL"]
        dr["CM"] = self.rival
        dr["TIPO"] = self.rivalTipo

        self.guardarVideo()

        self.accept()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def muestraDic(self, dic):
        color = dic.get("COLOR", None)
        if color is None:
            siBlancas = dic.get("SIBLANCAS", True)
            color = "B" if siBlancas else "N"
        self.rbBlancas.activa(color == "B")
        self.rbNegras.activa(color == "N")
        self.rbRandom.activa(color == "R")

        self.fen = dic.get("FEN", "")

        ayudas = dic.get("AYUDAS", 7)

        self.gbTutor.setChecked(ayudas > 0)
        self.cbAyudas.ponValor(ayudas)
        self.sbArrows.ponValor(dic.get("ARROWS", 7))
        self.cbThoughtOp.ponValor(dic.get("THOUGHTOP", -1))
        self.cbThoughtTt.ponValor(dic.get("THOUGHTTT", -1))
        self.sbBoxHeight.ponValor(dic.get("BOXHEIGHT", 64))
        self.sbArrowsTt.ponValor(dic.get("ARROWSTT", 0))
        self.chbContinueTt.setChecked(dic.get("CONTINUETT", True))
        self.chbChance.setChecked(dic.get("2CHANCE", True))
        self.chbSummary.setChecked(dic.get("SUMMARY", False))

        self.bloqueApertura = dic.get("APERTURA", None)
        self.liAperturasFavoritas = dic.get("APERTURASFAVORITAS", [])
        if self.bloqueApertura:
            nEsta = -1
            for npos, bl in enumerate(self.liAperturasFavoritas):
                if bl.a1h8 == self.bloqueApertura.a1h8:
                    nEsta = npos
                    break
            if nEsta != 0:
                if nEsta != -1:
                    del self.liAperturasFavoritas[nEsta]
                self.liAperturasFavoritas.insert(0, self.bloqueApertura)
            while len(self.liAperturasFavoritas) > 10:
                del self.liAperturasFavoritas[10]
        if len(self.liAperturasFavoritas):
            self.btAperturasFavoritas.show()

        dr = dic.get("RIVAL", {})
        motor = dr.get("MOTOR", self.configuracion.rivalInicial)
        tipo = dr.get("TIPO", None)
        self.rivalTipo, self.rival = self.motores.busca(tipo, motor)
        self.ponRival()

        self.edRtiempo.ponFloat(float(dr.get("TIEMPO", 0)) / 10.0)
        self.cbRdepth.ponValor(dr.get("PROFUNDIDAD", 3))
        self.cbResign.ponValor(dr.get("RESIGN", -800))

        siTiempo = dic.get("SITIEMPO", False)
        if siTiempo:
            self.chbTiempo.setChecked(True)
            self.edMinutos.setValue(dic["MINUTOS"])
            self.edSegundos.setValue(dic["SEGUNDOS"])
            self.edMinExtra.setValue(dic.get("MINEXTRA", 0))
            self.edZeitnot.setValue(dic.get("ZEITNOT", 0))
        else:
            self.chbTiempo.setChecked(False)

        book = dic.get("BOOK", None)
        bookRR = dic.get("BOOKRR", None)
        self.chbBook.setChecked(book is not None)
        if book:
            for bk in self.listaLibros.lista:
                if bk.path == book.path:
                    book = bk
                    break
            self.cbBooks.ponValor(book)
            self.cbBooksRR.ponValor(bookRR)
            self.chbBookMandatory.setChecked(dic.get("BOOKMANDATORY", False))

        self.cbAtras.setChecked(dic.get("ATRAS", True))
        self.cbAjustarRival.ponValor(dic.get("AJUSTAR", kAjustarMejor))

        self.muestraApertura()
        self.muestraPosicion()

    def motoresExternos(self):
        w = PantallaMotores.WMotores(self, self.configuracion.ficheroMExternos)
        if w.exec_():
            self.ajustesCambiado()
            self.motores.rehazMotoresExternos()

    def nuevoBook(self):
        fbin = QTUtil2.leeFichero(self, self.listaLibros.path, "bin", titulo=_("Polyglot book"))
        if fbin:
            self.listaLibros.path = os.path.dirname(fbin)
            nombre = os.path.basename(fbin)[:-4]
            b = Books.Libro("P", nombre, fbin, False)
            self.listaLibros.nuevo(b)
            fvar = self.configuracion.ficheroBooks
            self.listaLibros.guardaVar(fvar)
            li = [(x.nombre, x) for x in self.listaLibros.lista]
            self.cbBooks.rehacer(li, b)

    def aperturasQuitar(self):
        self.bloqueApertura = None
        self.muestraApertura()

    def posicionQuitar(self):
        self.fen = ""
        self.muestraPosicion()

    def tutorChange(self):
        if PantallaTutor.cambioTutor(self, self.configuracion):
            self.procesador.cambiaXTutor()


def entrenamientoMaquina(procesador, titulo):
    w = WEntMaquina(procesador, titulo)
    if w.exec_():
        return w.dic
    else:
        return None


class WCambioRival(QtGui.QDialog):
    def __init__(self, wParent, configuracion, dic, siGestorSolo):
        super(WCambioRival, self).__init__(wParent)

        if not dic:
            dic = {}

        self.setWindowTitle(_("Change opponent"))
        self.setWindowIcon(Iconos.Motor())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        self.configuracion = configuracion
        self.personalidades = Personalidades.Personalidades(self, configuracion)

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), "aceptar"), None,
                      (_("Cancel"), Iconos.Cancelar(), "cancelar"), None,
                      ]
        tb = Controles.TB(self, liAcciones)

        # Blancas o negras
        self.rbBlancas = Controles.RB(self, _("White")).activa()
        self.rbNegras = Controles.RB(self, _("Black"))

        # Atras
        self.cbAtras = Controles.CHB(self, "", True)

        # Motores
        self.motores = Motores.Motores(configuracion)

        liDepths = [("--", 0)]
        for x in range(1, 31):
            liDepths.append((str(x), x))

        # # Rival
        self.rival = configuracion.rivalInicial
        self.rivalTipo = Motores.INTERNO
        self.btRival = Controles.PB(self, "", self.cambiaRival, plano=False)
        self.edRtiempo = Controles.ED(self).tipoFloat().anchoMaximo(50)
        self.cbRdepth = Controles.CB(self, liDepths, 0).capturaCambiado(self.cambiadoDepth)
        lbTiempoSegundosR = Controles.LB2P(self, _("Time"))
        lbNivel = Controles.LB2P(self, _("Depth"))

        # # Ajustar rival
        liAjustes = self.personalidades.listaAjustes(True)
        self.cbAjustarRival = Controles.CB(self, liAjustes, kAjustarMejor).capturaCambiado(self.ajustesCambiado)
        lbAjustarRival = Controles.LB2P(self, _("Set strength"))
        btAjustarRival = Controles.PB(self, "", self.cambiaPersonalidades, plano=False).ponIcono(Iconos.Nuevo(),
                                                                                                 tamIcon=16)
        btAjustarRival.ponToolTip(_("Personalities"))

        # Layout
        # Color
        hbox = Colocacion.H().relleno().control(self.rbBlancas).espacio(30).control(self.rbNegras).relleno()
        gbColor = Controles.GB(self, _("Play with"), hbox)

        # Atras
        hbox = Colocacion.H().espacio(10).control(self.cbAtras)
        gbAtras = Controles.GB(self, _("Takeback"), hbox).alinCentrado()

        ## Color + Atras
        hAC = Colocacion.H().control(gbColor).control(gbAtras)
        if siGestorSolo:
            # gbColor.hide()
            gbAtras.hide()

        # Motores
        # Rival
        ly = Colocacion.G()
        ly.controlc(self.btRival, 0, 0, 1, 4)
        ly.controld(lbTiempoSegundosR, 1, 0).controld(self.edRtiempo, 1, 1)
        ly.controld(lbNivel, 1, 2).control(self.cbRdepth, 1, 3)
        lyH = Colocacion.H().control(lbAjustarRival).control(self.cbAjustarRival).control(btAjustarRival).relleno()
        ly.otroc(lyH, 2, 0, 1, 4)
        gbR = Controles.GB(self, _("Opponent"), ly)

        lyResto = Colocacion.V()
        lyResto.otro(hAC).espacio(3)
        lyResto.control(gbR).espacio(1)
        lyResto.margen(8)

        layout = Colocacion.V().control(tb).otro(lyResto).relleno().margen(3)

        self.setLayout(layout)

        self.dic = dic
        self.recuperaDic()

        self.ajustesCambiado()
        self.ponRival()

    def cambiaRival(self):
        resp = self.motores.menu(self)
        if resp:
            tp, cm = resp
            if tp == Motores.EXTERNO and cm is None:
                self.motoresExternos()
                return
            self.rivalTipo = tp
            self.rival = cm
            self.ponRival()

    def ponRival(self):
        self.btRival.ponTexto("   %s   " % self.rival.nombre)
        self.btRival.ponIcono(self.motores.dicIconos[self.rivalTipo])

    def ajustesCambiado(self):
        resp = self.cbAjustarRival.valor()
        if resp is None:
            self.cbAjustarRival.ponValor(kAjustarNivelAlto)

    def cambiadoDepth(self, num):
        if num > 0:
            self.edRtiempo.ponFloat(0.0)
        self.edRtiempo.setEnabled(num == 0)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "aceptar":
            self.aceptar()

        elif accion == "cancelar":
            self.reject()

        elif accion == "motores":
            self.motoresExternos()

    def aceptar(self):
        dic = self.dic
        dic["SIBLANCAS"] = self.rbBlancas.isChecked()

        dr = dic["RIVAL"] = {}
        dr["MOTOR"] = self.rival.clave
        dr["TIEMPO"] = int(self.edRtiempo.textoFloat() * 10)
        dr["PROFUNDIDAD"] = self.cbRdepth.valor()
        dr["CM"] = self.rival
        dr["TIPO"] = self.rivalTipo

        dic["ATRAS"] = self.cbAtras.valor()
        dic["AJUSTAR"] = self.cbAjustarRival.valor()

        self.accept()

    def motoresExternos(self):
        w = PantallaMotores.WMotores(self, self.configuracion.ficheroMExternos)
        if w.exec_():
            self.ajustesCambiado()
            self.motores.rehazMotoresExternos()

    def recuperaDic(self):
        dic = self.dic
        siBlancas = dic.get("SIBLANCAS", True)
        self.rbBlancas.activa(siBlancas)
        self.rbNegras.activa(not siBlancas)

        self.cbAtras.setChecked(dic.get("ATRAS", True))

        dr = dic.get("RIVAL", {})
        motor = dr.get("MOTOR", self.configuracion.tutor.clave)
        tipo = dr.get("TIPO", Motores.INTERNO)
        self.rivalTipo, self.rival = self.motores.busca(tipo, motor)
        self.ponRival()

        self.edRtiempo.ponFloat(float(dr.get("TIEMPO", self.configuracion.tiempoTutor / 100)) / 10.0)
        self.cbRdepth.ponValor(dr.get("PROFUNDIDAD", 0))
        self.cbAjustarRival.ponValor(dic.get("AJUSTAR", kAjustarMejor))

    def cambiaPersonalidades(self):
        siRehacer = self.personalidades.lanzaMenu()
        if siRehacer:
            actual = self.cbAjustarRival.valor()
            self.cbAjustarRival.rehacer(self.personalidades.listaAjustes(True), actual)


def cambioRival(parent, configuracion, dic, siGestorSolo=False):
    w = WCambioRival(parent, configuracion, dic, siGestorSolo)

    if w.exec_():
        return w.dic
    else:
        return None


def dameMinutosExtra(pantalla):
    liGen = [(None, None)]

    config = FormLayout.Spinbox(_("Extra minutes for the player"), 1, 99, 50)
    liGen.append((config, 5))

    resultado = FormLayout.fedit(liGen, title=_("Time"), parent=pantalla, icon=Iconos.MoverTiempo())
    if resultado:
        accion, liResp = resultado
        return liResp[0]

    return None
