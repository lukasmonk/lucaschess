import os
import shutil
import urllib
import zipfile

from PyQt4 import QtCore, QtGui

from Code import Books
from Code import GM
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaAperturas
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Util
from Code import VarGen


class WGM(QTVarios.WDialogo):
    def __init__(self, procesador, siWoman):
        self.configuracion = procesador.configuracion
        self.procesador = procesador
        self.siWoman = siWoman

        self.dbHisto = Util.DicSQL(self.configuracion.ficheroGMhisto)
        self.bloqueApertura = None
        self.liAperturasFavoritas = []

        wParent = procesador.pantalla
        if siWoman:
            titulo = _("Play like a Woman Grandmaster")
            icono = Iconos.WGranMaestro()
        else:
            titulo = _("Play like a Grandmaster")
            icono = Iconos.GranMaestro()

        extparam = "gm"
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        flb = Controles.TipoLetra(puntos=10)

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      (_("One game"), Iconos.Uno(), self.unJuego), None,
                      (_("Import"), Iconos.ImportarGM(), self.importar)
                      ]
        tb = Controles.TBrutina(self, liAcciones)

        # Grandes maestros
        self.liGM = GM.listaGM(siWoman)
        li = [(x[0], x[1]) for x in self.liGM]
        li.insert(0, ("-", None))
        self.cbGM = QTUtil2.comboBoxLB(self, li, li[0][1] if len(self.liGM) == 0 else li[1][1])
        self.cbGM.capturaCambiado(self.compruebaGM)
        hbox = Colocacion.H().relleno().control(self.cbGM).relleno()
        gbGM = Controles.GB(self, _("Choose a Grandmaster"), hbox).ponFuente(flb)

        # Personales
        self.liPersonal = GM.listaGMpersonal(self.procesador.configuracion.dirPersonalTraining)
        if self.liPersonal:
            li = [(x[0], x[1]) for x in self.liPersonal]
            li.insert(0, ("-", None))
            self.cbPersonal = QTUtil2.comboBoxLB(self, li, li[0][1])
            self.cbPersonal.capturaCambiado(self.compruebaP)
            btBorrar = Controles.PB(self, "", self.borrarPersonal, plano=False).ponIcono(Iconos.Borrar(), tamIcon=16)
            hbox = Colocacion.H().relleno().control(self.cbPersonal).control(btBorrar).relleno()
            gbPersonal = Controles.GB(self, _("Personal games"), hbox).ponFuente(flb)

        # Color
        self.rbBlancas = Controles.RB(self, _("White"), rutina=self.compruebaColor)
        self.rbBlancas.activa(True)
        self.rbNegras = Controles.RB(self, _("Black"), rutina=self.compruebaColor)
        self.rbNegras.activa(False)

        # Contrario
        self.chContrario = Controles.CHB(self, _("Choose the opponent's move, when there are multiple possible answers"),
                                         False)

        # Juez
        liDepths = [("--", 0)]
        for x in range(1, 31):
            liDepths.append((str(x), x))
        self.liMotores = self.configuracion.comboMotoresMultiPV10()
        self.cbJmotor, self.lbJmotor = QTUtil2.comboBoxLB(self, self.liMotores, self.configuracion.tutorInicial, _("Engine"))
        self.edJtiempo = Controles.ED(self).tipoFloat().ponFloat(1.0).anchoFijo(50)
        self.lbJtiempo = Controles.LB2P(self, _("Time in seconds"))
        self.cbJdepth = Controles.CB(self, liDepths, 0).capturaCambiado(self.cambiadoDepth)
        self.lbJdepth = Controles.LB2P(self, _("Depth"))
        self.lbJshow = Controles.LB2P(self, _("Show rating"))
        self.chbEvals = Controles.CHB(self, _("Show all evaluations"), False)
        liOptions = [(_("All moves"), None), (_("Moves are different"), True), (_("Never"), False)]
        self.cbJshow = Controles.CB(self, liOptions, True)
        self.lbJmultiPV = Controles.LB2P(self, _("Number of moves evaluated by engine(MultiPV)"))
        li = [(_("Default"), "PD"),
              (_("Maximum"), "MX")]
        for x in (1, 3, 5, 10, 15, 20, 30, 40, 50, 75, 100, 150, 200):
            li.append((str(x), str(x)))
        self.cbJmultiPV = Controles.CB(self, li, "PD")

        # Inicial
        self.edJugInicial, lbInicial = QTUtil2.spinBoxLB(self, 1, 1, 99, etiqueta=_("Initial move"), maxTam=40)

        # Libros
        fvar = self.configuracion.ficheroBooks
        self.listaLibros = Books.ListaLibros()
        self.listaLibros.recuperaVar(fvar)
        # # Comprobamos que todos esten accesibles
        self.listaLibros.comprueba()
        li = [(x.nombre, x) for x in self.listaLibros.lista]
        li.insert(0, ("--", None))
        self.cbBooks, lbBooks = QTUtil2.comboBoxLB(self, li, None, _("Bypass moves in the book"))

        # Aperturas

        self.btApertura = Controles.PB(self, " " * 5 + _("Undetermined") + " " * 5, self.aperturasEditar).ponPlano(
                False)
        self.btAperturasFavoritas = Controles.PB(self, "", self.aperturasFavoritas).ponIcono(Iconos.Favoritos())
        self.btAperturasQuitar = Controles.PB(self, "", self.aperturasQuitar).ponIcono(Iconos.Motor_No())
        hbox = Colocacion.H().control(self.btAperturasQuitar).control(self.btApertura).control(
                self.btAperturasFavoritas).relleno()
        gbOpening = Controles.GB(self, _("Opening"), hbox)

        # gbBasic
        # # Color
        hbox = Colocacion.H().relleno().control(self.rbBlancas).espacio(10).control(self.rbNegras).relleno()
        gbColor = Controles.GB(self, _("Play with"), hbox).ponFuente(flb)

        # Tiempo
        ly1 = Colocacion.H().control(self.lbJmotor).control(self.cbJmotor).control(self.lbJshow).control(
                self.cbJshow).relleno()
        ly2 = Colocacion.H().control(self.lbJtiempo).control(self.edJtiempo)
        ly2.control(self.lbJdepth).control(self.cbJdepth).espacio(15).control(self.chbEvals).relleno()
        ly3 = Colocacion.H().control(self.lbJmultiPV).control(self.cbJmultiPV).relleno()
        ly = Colocacion.V().otro(ly1).otro(ly2).otro(ly3)
        self.gbJ = Controles.GB(self, _("Adjudicator"), ly).conectar(self.cambiaJuez)

        # Opciones
        vlayout = Colocacion.V().control(gbColor)
        vlayout.espacio(5).control(self.gbJ)
        vlayout.margen(20)
        gbBasic = Controles.GB(self, "", vlayout)
        gbBasic.setFlat(True)

        # Opciones avanzadas
        lyInicial = Colocacion.H().control(lbInicial).control(self.edJugInicial).relleno().control(lbBooks).control(
                self.cbBooks).relleno()
        vlayout = Colocacion.V().relleno().otro(lyInicial).control(gbOpening)
        vlayout.espacio(5).control(self.chContrario).margen(20).relleno()
        gbAdvanced = Controles.GB(self, "", vlayout)
        gbAdvanced.setFlat(True)

        # Historico
        self.liHisto = []
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("FECHA", _("Date"), 80, siCentrado=True)
        oColumnas.nueva("PACIERTOS", _("Hints"), 90, siCentrado=True)
        oColumnas.nueva("PUNTOS", _("Points accumulated"), 120, siCentrado=True)
        oColumnas.nueva("ENGINE", _("Adjudicator"), 100, siCentrado=True)
        oColumnas.nueva("RESUMEN", _("Game played"), 150)

        self.grid = grid = Grid.Grid(self, oColumnas, siSelecFilas=True, background=None)
        self.grid.coloresAlternados()
        self.registrarGrid(grid)

        # Tabs
        self.tab = Controles.Tab().ponPosicion("S")
        self.tab.nuevaTab(gbBasic, _("Basic"))
        self.tab.nuevaTab(gbAdvanced, _("Advanced"))
        self.tab.nuevaTab(self.grid, _("Track record"))

        # Cabecera
        lyCab = Colocacion.H().control(gbGM)
        if self.liPersonal:
            lyCab.control(gbPersonal)

        layout = Colocacion.V().control(tb).otro(lyCab).control(self.tab).margen(3)

        self.setLayout(layout)

        self.recuperaDic()
        self.cambiaJuez()
        self.compruebaGM()
        self.compruebaP()
        self.compruebaHisto()
        self.aperturaMuestra()
        self.btAperturasFavoritas.hide()

        self.recuperarVideo(anchoDefecto=450)

    def cambiadoDepth(self, num):
        tiempo = self.edJtiempo.textoFloat()
        if int(tiempo)*10 == 0:
            tiempo = 3.0
        self.edJtiempo.ponFloat(0.0 if num > 0 else tiempo)
        self.edJtiempo.setEnabled(num == 0)

    def closeEvent(self, event):
        self.guardarVideo()
        self.dbHisto.close()

    def compruebaGM_P(self, liGMP, tgm):
        tsiw = self.rbBlancas.isChecked()

        for nom, gm, siw, sib in liGMP:
            if gm == tgm:
                self.rbBlancas.setEnabled(siw)
                self.rbNegras.setEnabled(sib)
                if tsiw:
                    if not siw:
                        self.rbBlancas.activa(False)
                        self.rbNegras.activa(True)
                else:
                    if not sib:
                        self.rbBlancas.activa(True)
                        self.rbNegras.activa(False)
                break
        self.compruebaHisto()

    def compruebaGM(self):
        tgm = self.cbGM.valor()
        if tgm:
            if self.liPersonal:
                self.cbPersonal.ponValor(None)
            self.compruebaGM_P(self.liGM, tgm)

    def compruebaP(self):
        if not self.liPersonal:
            return
        tgm = self.cbPersonal.valor()
        if tgm:
            if self.liGM:
                self.cbGM.ponValor(None)
            self.compruebaGM_P(self.liPersonal, tgm)

    def compruebaHisto(self):
        tgmGM = self.cbGM.valor()
        tgmP = self.cbPersonal.valor() if self.liPersonal else None

        if tgmGM is None and tgmP is None:
            if len(self.liGM) > 1:
                tgmGM = self.liGM[1][1]
                self.cbGM.ponValor(tgmGM)
            else:
                self.liHisto = []
                return

        if tgmGM and tgmP:
            self.cbPersonal.ponValor(None)
            tgmP = None

        if tgmGM:
            tgm = tgmGM
        else:
            tgm = "P_" + tgmP

        self.liHisto = self.dbHisto[tgm]
        if self.liHisto is None:
            self.liHisto = []
        self.grid.refresh()

    def gridNumDatos(self, grid):
        return len(self.liHisto)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        dic = self.liHisto[fila]
        if clave == "FECHA":
            f = dic["FECHA"]
            return "%d/%02d/%d" % (f.day, f.month, f.year)
        elif clave == "PACIERTOS":
            return "%d%%" % dic["PACIERTOS"]
        elif clave == "PUNTOS":
            return "%d" % dic["PUNTOS"]
        elif clave == "JUEZ":
            return "%s (%d)" % (dic["JUEZ"], dic["TIEMPO"])
        elif clave == "RESUMEN":
            return dic.get("RESUMEN", "")

    def borrarPersonal(self):
        tgm = self.cbPersonal.valor()
        if tgm is None:
            return
        if not QTUtil2.pregunta(self, _X(_("Delete %1?"), tgm)):
            return

        base = os.path.join(self.configuracion.dirPersonalTraining, "%s.gm" % tgm)
        for x in "wbi":
            Util.borraFichero(base + x)

        self.liPersonal = GM.listaGMpersonal(self.configuracion.dirPersonalTraining)

        li = [(x[0], x[1]) for x in self.liPersonal]
        li.insert(0, ("-", None))
        self.cbPersonal.rehacer(li, li[0][1])

        self.compruebaP()

    def compruebaColor(self):
        tgm = self.cbGM.valor()
        tsiw = self.rbBlancas.isChecked()

        for nom, gm, siw, sib in self.liGM:
            if gm == tgm:
                if tsiw:
                    if not siw:
                        self.rbBlancas.activa(False)
                        self.rbNegras.activa(True)
                else:
                    if not sib:
                        self.rbBlancas.activa(True)
                        self.rbNegras.activa(False)

    def aceptar(self):
        if self.grabaDic():
            self.accept()
        else:
            self.reject()

    def unJuego(self):
        if not self.grabaDic():  # crea self.ogm
            return

        w = SelectGame(self, self.ogm, self.siWoman)
        if w.exec_():
            if w.partidaElegida is not None:
                self.record.partidaElegida = w.partidaElegida

                self.accept()

    def cancelar(self):
        self.reject()

    def importar(self):
        if importarGM(self, self.siWoman):
            liC = GM.listaGM(self.siWoman)
            self.cbGM.clear()
            for tp in liC:
                self.cbGM.addItem(tp[0], tp[1])
            self.cbGM.setCurrentIndex(0)

    def cambiaJuez(self):
        si = self.gbJ.isChecked()
        for control in (self.cbJmotor, self.lbJmotor,
                        self.edJtiempo, self.lbJtiempo,
                        self.lbJdepth, self.cbJdepth,
                        self.lbJshow, self.cbJshow,
                        self.lbJmultiPV, self.cbJmultiPV,
                        self.chbEvals,
                        ):
            control.setVisible(si)

    def grabaDic(self):
        rk = Util.Record()
        rk.gm = self.cbGM.valor()
        if rk.gm is None:
            rk.modo = "personal"
            rk.gm = self.cbPersonal.valor()
            if rk.gm is None:
                return False
        else:
            rk.modo = "estandar"
        rk.partidaElegida = None
        rk.siBlancas = self.rbBlancas.isChecked()
        rk.siJuez = self.gbJ.isChecked()
        rk.showevals = self.chbEvals.valor()
        rk.motor = self.cbJmotor.valor()
        rk.tiempo = int(self.edJtiempo.textoFloat() * 10)
        rk.mostrar = self.cbJshow.valor()
        rk.depth = self.cbJdepth.valor()
        rk.multiPV = self.cbJmultiPV.valor()
        rk.jugContrario = self.chContrario.isChecked()
        rk.jugInicial = self.edJugInicial.valor()
        if rk.siJuez and rk.tiempo <= 0 and rk.depth == 0:
            rk.siJuez = False
        rk.bypassBook = self.cbBooks.valor()
        rk.apertura = self.bloqueApertura

        default = "WGM" if self.siWoman else "GM"

        carpeta = default if rk.modo == "estandar" else self.configuracion.dirPersonalTraining
        self.ogm = GM.GM(carpeta, rk.gm)
        self.ogm.colorFilter(rk.siBlancas)
        if not len(self.ogm):
            QTUtil2.mensError(self, _("There are no games to play with this color"))
            return False

        self.ogm.isErasable = rk.modo == "personal" # para saber si se puede borrar
        self.record = rk
        self.record.siWoman = self.siWoman
        dic = {}

        for atr in dir(self.record):
            if not atr.startswith("_"):
                dic[atr.upper()] = getattr(self.record, atr)
        dic["APERTURASFAVORITAS"] = self.liAperturasFavoritas

        Util.guardaDIC(dic, self.configuracion.ficheroGM)

        return True

    def recuperaDic(self):
        dic = Util.recuperaDIC(self.configuracion.ficheroGM)
        if dic:
            gm = dic["GM"]
            modo = dic.get("MODO", "estandar")
            siBlancas = dic["SIBLANCAS"]
            siJuez = dic["SIJUEZ"]
            showevals = dic.get("SHOWEVALS", False)
            motor = dic["MOTOR"]
            tiempo = dic["TIEMPO"]
            depth = dic.get("DEPTH", 0)
            multiPV = dic.get("MULTIPV", "PD")
            mostrar = dic["MOSTRAR"]
            jugContrario = dic.get("JUGCONTRARIO", False)
            jugInicial = dic.get("JUGINICIAL", 1)
            self.liAperturasFavoritas = dic.get("APERTURASFAVORITAS", [])
            self.bloqueApertura = dic.get("APERTURA", None)
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
            bypassBook = dic.get("BYPASSBOOK", None)

            self.rbBlancas.setChecked(siBlancas)
            self.rbNegras.setChecked(not siBlancas)

            self.gbJ.setChecked(siJuez)
            self.cbJmotor.ponValor(motor)
            self.edJtiempo.ponFloat(float(tiempo / 10.0))
            self.cbJshow.ponValor(mostrar)
            self.chbEvals.ponValor(showevals)
            self.cbJdepth.ponValor(depth)
            self.cambiadoDepth(depth)
            self.cbJmultiPV.ponValor(multiPV)

            self.chContrario.setChecked(jugContrario)

            self.edJugInicial.ponValor(jugInicial)

            li = self.liGM
            cb = self.cbGM
            if modo == "personal":
                if self.liPersonal:
                    li = self.liPersonal
                    cb = self.cbGM
            for v in li:
                if v[1] == gm:
                    cb.ponValor(gm)
                    break
            if bypassBook:
                for book in self.listaLibros.lista:
                    if book.path == bypassBook.path:
                        self.cbBooks.ponValor(book)
                        break
            self.aperturaMuestra()

    def aperturasEditar(self):
        self.btApertura.setDisabled(True)  # Puede tardar bastante tiempo
        me = QTUtil2.unMomento(self)
        w = PantallaAperturas.WAperturas(self, self.configuracion, self.bloqueApertura)
        me.final()
        self.btApertura.setDisabled(False)
        if w.exec_():
            self.bloqueApertura = w.resultado()
            self.aperturaMuestra()

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
                self.aperturaMuestra()
            elif menu.siDer:
                bloqueApertura = resp
                if QTUtil2.pregunta(self,
                                    _X(_("Do you want to delete the opening %1 from the list of favourite openings?"),
                                       bloqueApertura.trNombre)):
                    del self.liAperturasFavoritas[nPos]

    def aperturaMuestra(self):
        if self.bloqueApertura:
            rotulo = self.bloqueApertura.trNombre + "\n" + self.bloqueApertura.pgn
            self.btAperturasQuitar.show()
        else:
            rotulo = " " * 3 + _("Undetermined") + " " * 3
            self.btAperturasQuitar.hide()
        self.btApertura.ponTexto(rotulo)

    def aperturasQuitar(self):
        self.bloqueApertura = None
        self.aperturaMuestra()


def eligeJugada(gestor, liJugadas, siGM):
    menu = QTVarios.LCMenu(gestor.pantalla)

    if siGM:
        titulo = gestor.nombreGM
        icono = Iconos.GranMaestro()
    else:
        titulo = _("Opponent's move")
        icono = Iconos.Carpeta()
    menu.opcion(None, titulo, icono)
    menu.separador()

    icono = Iconos.PuntoAzul() if siGM else Iconos.PuntoNaranja()

    for desde, hasta, coronacion, rotulo, pgn in liJugadas:

        if rotulo and (len(liJugadas) > 1):
            txt = "%s - %s" % (pgn, rotulo)
        else:
            txt = pgn
        menu.opcion((desde, hasta, coronacion), txt, icono)
        menu.separador()

    resp = menu.lanza()
    if resp:
        return resp
    else:
        desde, hasta, coronacion, rotulo, pgn = liJugadas[0]
        return desde, hasta, coronacion


class WImportar(QtGui.QDialog):
    def __init__(self, wParent, liGM):

        super(WImportar, self).__init__(wParent)

        self.liGM = liGM

        self.setWindowTitle(_("Import"))
        self.setWindowIcon(Iconos.ImportarGM())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint |
                QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint | QtCore.Qt.WindowMaximizeButtonHint)

        # Toolbar
        liAcciones = [(_("Import"), Iconos.Aceptar(), self.accept), None,
                      (_("Cancel"), Iconos.Cancelar(), self.reject), None,
                      (_("Mark"), Iconos.Marcar(), self.marcar), None,
                      ]
        tb = Controles.TBrutina(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ELEGIDO", "", 22, siChecked=True)
        oColumnas.nueva("NOMBRE", _("Grandmaster"), 140)
        oColumnas.nueva("PARTIDAS", _("Games"), 60, siDerecha=True)

        self.grid = Grid.Grid(self, oColumnas)
        n = self.grid.anchoColumnas()
        self.grid.setFixedWidth(n + 20)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).margen(3)
        self.setLayout(layout)

    def marcar(self):
        menu = QTVarios.LCMenu(self)
        f = Controles.TipoLetra(puntos=8, peso=75)
        menu.ponFuente(f)
        menu.opcion(1, _("All"), Iconos.PuntoVerde())
        menu.opcion(2, _("None"), Iconos.PuntoNaranja())
        resp = menu.lanza()
        if resp:
            for obj in self.liGM:
                obj["ELEGIDO"] = resp == 1
            self.grid.refresh()

    def gridNumDatos(self, grid):
        return len(self.liGM)

    def gridPonValor(self, grid, fila, columna, valor):
        self.liGM[fila][columna.clave] = valor

    def gridDato(self, grid, fila, oColumna):
        return self.liGM[fila][oColumna.clave]


def importarGM(ownerGM, siWoman):
    # Primero nos tenemos que traer la lista de la web
    fichz = "_listaGM.zip"
    ficht = "_listaGM.txt"
    fichtg = "GM/listaGM.txt"
    if siWoman:
        fichtg = "w" + fichtg
    try:
        os.remove(ficht)
    except:
        pass
    if siWoman:
        web = "https://lucaschess.pythonanywhere.com/static/wgm"
        message = _("Reading the list of Women Grandmaster from the web")
    else:
        web = "https://lucaschess.pythonanywhere.com/static/gm"
        message = _("Reading the list of Grandmasters from the web")
    me = QTUtil2.mensEspera.inicio(ownerGM, message)
    siError = False
    try:
        urllib.urlretrieve("%s/%s" % (web, fichz), fichz)
        zfobj = zipfile.ZipFile(fichz)
        for name in zfobj.namelist():
            outfile = open(name, 'wb')
            outfile.write(zfobj.read(name))
            outfile.close()
        zfobj.close()
        os.remove(fichz)
    except:
        siError = True
    me.final()

    if siError:
        QTUtil2.mensError(ownerGM, _("List of Grandmasters currently unavailable; please check Internet connectivity"))
        return False

    shutil.copy(ficht, fichtg)
    os.remove(ficht)

    f = open(fichtg, "rb")
    liGM = []
    for linea in f:
        linea = linea.strip()
        if linea:
            gm, nombre, ctam, cpart = linea.split(VarGen.XSEP)
            fichero = "GM/%s.xgm" % gm
            if siWoman:
                fichero = "w" + fichero
            if Util.tamFichero(fichero) != int(ctam):  # si no existe tam = -1
                dic = {"GM": gm, "NOMBRE": nombre, "PARTIDAS": cpart, "ELEGIDO": False}
                liGM.append(dic)
    f.close()

    if len(liGM) == 0:
        QTUtil2.mensaje(ownerGM, _("You have all Grandmasters installed."))
        return False

    w = WImportar(ownerGM, liGM)
    if w.exec_():
        for dic in liGM:
            if dic["ELEGIDO"]:
                gm = dic["GM"]
                gm = gm[0].upper() + gm[1:].lower()
                me = QTUtil2.mensEspera.inicio(ownerGM, _X(_("Import %1"), gm), opacity=1.0)

                # Descargamos
                fzip = gm + ".zip"
                urllib.urlretrieve("%s/%s.zip" % (web, gm), fzip)

                zfobj = zipfile.ZipFile(fzip)
                for name in zfobj.namelist():
                    fichero = "GM/%s" % name
                    if siWoman:
                        fichero = "w" + fichero
                    outfile = open(fichero, 'wb')
                    outfile.write(zfobj.read(name))
                    outfile.close()
                zfobj.close()
                os.remove(fzip)

                me.final()

        return True

    return False


class SelectGame(QTVarios.WDialogo):
    def __init__(self, wgm, ogm, siWoman):
        self.ogm = ogm
        self.liRegs = ogm.genToSelect()
        self.siReverse = False
        self.claveSort = None

        dgm = GM.dicGM(siWoman)
        nombre = dgm.get(ogm.gm, ogm.gm)
        titulo = "%s - %s" % (_("One game"), nombre)
        icono = Iconos.Uno()
        extparam = "gm1g"
        QTVarios.WDialogo.__init__(self, wgm, titulo, icono, extparam)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NOMBRE", _("Opponent"), 180)
        oColumnas.nueva("FECHA", _("Date"), 90, siCentrado=True)
        oColumnas.nueva("EVENT", _("Event"), 140, siCentrado=True)
        oColumnas.nueva("ECO", _("ECO"), 40, siCentrado=True)
        oColumnas.nueva("RESULT", _("Result"), 64, siCentrado=True)
        self.grid = grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        nAnchoPgn = self.grid.anchoColumnas() + 20
        self.grid.setMinimumWidth(nAnchoPgn)
        self.grid.coloresAlternados()

        self.registrarGrid(grid)

        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      ]
        if ogm.isErasable:
            liAcciones.append((_("Remove"), Iconos.Borrar(), self.remove))
            liAcciones.append(None)

        tb = Controles.TBrutina(self, liAcciones)

        layout = Colocacion.V().control(tb).control(grid).margen(3)
        self.setLayout(layout)

        self.recuperarVideo(anchoDefecto=400)
        self.partidaElegida = None

    def gridNumDatos(self, grid):
        return len(self.liRegs)

    def gridDato(self, grid, fila, oColumna):
        return self.liRegs[fila][oColumna.clave]

    def gridDobleClick(self, grid, fila, columna):
        self.aceptar()

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave

        self.liRegs = sorted(self.liRegs, key=lambda x: x[clave].upper())

        if self.claveSort == clave:
            if self.siReverse:
                self.liRegs.reverse()

            self.siReverse = not self.siReverse
        else:
            self.siReverse = True

        self.grid.refresh()
        self.grid.gotop()

    def aceptar(self):
        self.partidaElegida = self.liRegs[self.grid.recno()]["NUMERO"]
        self.guardarVideo()
        self.accept()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def remove(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                li.sort(reverse=True)
                for x in li:
                    self.ogm.remove(x)
                    del self.liRegs[x]
                self.grid.refresh()
