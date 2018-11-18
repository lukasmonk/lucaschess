import os
import subprocess

from PyQt4 import QtCore, QtGui

from Code import Books
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Util
from Code import VarGen


class WBooksCrear(QtGui.QDialog):
    def __init__(self, wParent):

        QtGui.QDialog.__init__(self, wParent)

        self.wParent = wParent

        self.fichero = ""

        self.setWindowTitle(_("Create a new book"))
        self.setWindowIcon(Iconos.Libros())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        f = Controles.TipoLetra(puntos=9, peso=75)

        self.configuracion = VarGen.configuracion
        fvar = self.configuracion.ficheroBooks
        self.listaLibros = Books.ListaLibros()
        self.listaLibros.recuperaVar(fvar)

        lbFichero = Controles.LB(self, _("Book to create") + ":").ponFuente(f)
        self.btFichero = Controles.PB(self, "", self.buscaFichero, False).anchoMinimo(450).ponFuente(f)
        lbMaxPly = Controles.LB(self, _("Maximum no. half moves (ply)") + ":").ponFuente(f)
        self.sbMaxPly = Controles.SB(self, 0, 0, 999).tamMaximo(50)
        lbMinGame = Controles.LB(self, _("Minimum number of games") + ":").ponFuente(f)
        self.sbMinGame = Controles.SB(self, 3, 1, 999).tamMaximo(50)
        lbMinScore = Controles.LB(self, _("Minimum score") + ":").ponFuente(f)
        self.sbMinScore = Controles.SB(self, 0, 0, 100).tamMaximo(50)
        self.chbOnlyWhite = Controles.CHB(self, _("White only"), False).ponFuente(f)
        self.chbOnlyBlack = Controles.CHB(self, _("Black only"), False).ponFuente(f)
        self.chbUniform = Controles.CHB(self, _("Uniform distribution"), False).ponFuente(f)

        lyf = Colocacion.H().control(lbFichero).control(self.btFichero)
        ly = Colocacion.G().margen(15)
        ly.otroc(lyf, 0, 0, 1, 2)
        ly.controld(lbMaxPly, 1, 0).control(self.sbMaxPly, 1, 1)
        ly.controld(lbMinGame, 2, 0).control(self.sbMinGame, 2, 1)
        ly.controld(lbMinScore, 3, 0).control(self.sbMinScore, 3, 1)
        ly.controlc(self.chbOnlyWhite, 4, 0, 1, 2)
        ly.controlc(self.chbOnlyBlack, 5, 0, 1, 2)
        ly.controlc(self.chbUniform, 6, 0, 1, 2)

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), "aceptar"), None,
                      (_("Cancel"), Iconos.Cancelar(), "cancelar"), None
                      ]
        tb = Controles.TB(self, liAcciones)

        # Layout
        layout = Colocacion.V().control(tb).otro(ly).margen(3)
        self.setLayout(layout)

    def buscaFichero(self):

        # Libros
        fbin = QTUtil2.salvaFichero(self, _("Polyglot book"), self.listaLibros.path,
                                    _("File") + " %s (*.%s)" % ("bin", "bin"))
        if fbin:
            self.listaLibros.path = os.path.dirname(fbin)
            self.fichero = fbin
            self.btFichero.ponTexto(self.fichero)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "aceptar":
            self.aceptar()
        elif accion == "cancelar":
            self.reject()

    def aceptar(self):
        if not self.fichero:
            return

        # Creamos el pgn
        fichTemporal = self.wParent.damePGNtemporal(self)
        if not fichTemporal:
            return

        me = QTUtil2.unMomento(self)

        # Creamos la linea de ordenes
        if VarGen.isWindows:
            exe = 'Engines/Windows/_tools/polyglot/polyglot.exe'
        else:
            exe = '%s/_tools/polyglot/polyglot' % VarGen.folder_engines
        li = [os.path.abspath(exe), 'make-book', "-pgn", fichTemporal, "-bin", self.fichero]
        Util.borraFichero(self.fichero)

        maxPly = self.sbMaxPly.valor()
        minGame = self.sbMinGame.valor()
        minScore = self.sbMinScore.valor()
        onlyWhite = self.chbOnlyWhite.valor()
        onlyBlack = self.chbOnlyBlack.valor()
        uniform = self.chbUniform.valor()
        if maxPly:
            li.append("-max-ply")
            li.append("%d" % maxPly)
        if minGame and minGame != 3:
            li.append("-min-game")
            li.append("%d" % minGame)
        if minScore:
            li.append("-min-score")
            li.append("%d" % minScore)
        if onlyBlack:
            li.append("-only-black")
        if onlyWhite:
            li.append("-only-white")
        if uniform:
            li.append("-uniform")

        # Ejecutamos
        process = subprocess.Popen(li, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        # Mostramos el resultado
        txt = process.stdout.read()
        if os.path.isfile(self.fichero):
            txt += "\n" + _X(_("Book created : %1"), self.fichero)
        me.final()
        QTUtil2.mensaje(self, txt)

        Util.borraFichero(fichTemporal)

        nombre = os.path.basename(self.fichero)[:-4]
        b = Books.Libro("P", nombre, self.fichero, False)
        self.listaLibros.nuevo(b)
        fvar = self.configuracion.ficheroBooks
        self.listaLibros.guardaVar(fvar)

        self.accept()


def polyglotCrear(owner):
    w = WBooksCrear(owner)
    w.exec_()


def polyglotUnir(owner):
    lista = [(None, None)]

    dict1 = {"FICHERO": "", "EXTENSION": "bin", "SISAVE": False}
    lista.append((_("File") + " 1 :", dict1))
    dict2 = {"FICHERO": "", "EXTENSION": "bin", "SISAVE": False}
    lista.append((_("File") + " 2 :", dict2))
    dictr = {"FICHERO": "", "EXTENSION": "bin", "SISAVE": True}
    lista.append((_("Book to create") + ":", dictr))

    while True:
        resultado = FormLayout.fedit(lista, title=_("Merge two books in one"), parent=owner, anchoMinimo=460,
                                     icon=Iconos.Libros())
        if resultado:
            resultado = resultado[1]
            error = None
            f1 = resultado[0]
            f2 = resultado[1]
            fr = resultado[2]

            if (not f1) or (not f2) or (not fr):
                error = _("Not indicated all files")
            elif f1 == f2:
                error = _("File") + " 1 = " + _("File") + " 2"
            elif f1 == fr:
                error = _("File") + " 1 = " + _("Book to create")
            elif f2 == fr:
                error = _("File") + " 2 = " + _("Book to create")

            if error:
                dict1["FICHERO"] = f1
                dict2["FICHERO"] = f2
                dictr["FICHERO"] = fr
                QTUtil2.mensError(owner, error)
                continue
        else:
            return

        if VarGen.isWindows:
            exe = 'Engines/Windows/_tools/polyglot/polyglot.exe'
        else:
            exe = '%s/_tools/polyglot/polyglot' % VarGen.folder_engines

        li = [os.path.abspath(exe), 'merge-book', "-in1", f1, "-in2", f2, "-out", fr]
        try:
            os.remove(fr)
        except:
            pass

        # Ejecutamos
        me = QTUtil2.unMomento(owner)

        process = subprocess.Popen(li, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        # Mostramos el resultado
        txt = process.stdout.read()
        if os.path.isfile(fr):
            txt += "\n" + _X(_("Book created : %1"), fr)
        me.final()
        QTUtil2.mensaje(owner, txt)

        return


class WBooks(QtGui.QDialog):
    def __init__(self, procesador):

        wParent = procesador.pantalla
        self.configuracion = procesador.configuracion
        self.procesador = procesador
        self.siCambios = False

        QtGui.QDialog.__init__(self, wParent)

        self.setWindowTitle(_("Training with a book"))
        self.setWindowIcon(Iconos.Libros())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint)

        self.setMinimumWidth(450)

        flb = Controles.TipoLetra(puntos=10)

        # Variables antiguas
        dic = self.recuperaDIC()
        if dic is None:
            dic = {}
        dic_siBlancas = dic.get("SIBLANCAS", True)
        dic_nomLibro = dic.get("NOMLIBRO", "")
        dic_RR = dic.get("RR", "au")
        dic_RJ = dic.get("RJ", False)

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), "aceptar"), None,
                      (_("Cancel"), Iconos.Cancelar(), "cancelar"), None,
                      ]
        tb = Controles.TB(self, liAcciones)

        # Color
        self.rbBlancas = QtGui.QRadioButton(_("White"))
        self.rbBlancas.setChecked(dic_siBlancas)
        self.rbNegras = QtGui.QRadioButton(_("Black"))
        self.rbNegras.setChecked(not dic_siBlancas)

        hbox = Colocacion.H().relleno().control(self.rbBlancas).espacio(10).control(self.rbNegras).relleno()
        gbColor = Controles.GB(self, _("Play with"), hbox).ponFuente(flb)

        # Libros
        fvar = self.configuracion.ficheroBooks
        self.listaLibros = Books.ListaLibros()
        self.listaLibros.recuperaVar(fvar)

        # # Comprobamos que todos esten accesibles
        self.listaLibros.comprueba()
        li = [(x.nombre, x) for x in self.listaLibros.lista]
        libInicial = None
        if dic_nomLibro:
            for nom, libro in li:
                if nom == dic_nomLibro:
                    libInicial = libro
                    break
        if libInicial is None:
            libInicial = li[0][1] if li else None
        self.cb = QTUtil2.comboBoxLB(self, li, libInicial)

        btNuevo = Controles.PB(self, "", self.nuevo, plano=False).ponIcono(Iconos.Nuevo(), tamIcon=16)
        btBorrar = Controles.PB(self, "", self.borrar, plano=False).ponIcono(Iconos.Borrar(), tamIcon=16)

        hbox = Colocacion.H().relleno().control(self.cb).control(btNuevo).control(btBorrar).relleno()
        gbLibro = Controles.GB(self, _("Book"), hbox).ponFuente(flb)

        # Respuesta rival
        li = (
            (_("Selected by the player"), "su"),
            (_("Uniform random"), "au"),
            (_("Proportional random"), "ap"),
            (_("Always the highest percentage"), "mp"),
        )
        self.cbRR = QTUtil2.comboBoxLB(self, li, dic_RR)
        hbox = Colocacion.H().relleno().control(self.cbRR).relleno()
        gbRR = Controles.GB(self, _("Opponent's move"), hbox).ponFuente(flb)

        # Respuesta jugador
        self.chRJ = Controles.CHB(self, _("Always the highest percentage"), dic_RJ)
        hbox = Colocacion.H().relleno().control(self.chRJ).relleno()
        gbRJ = Controles.GB(self, _("Player's move"), hbox).ponFuente(flb)

        vlayout = Colocacion.V()
        vlayout.control(gbColor).espacio(5)
        vlayout.control(gbLibro).espacio(5)
        vlayout.control(gbRR).espacio(5)
        vlayout.control(gbRJ)
        vlayout.margen(20)

        layout = Colocacion.V().control(tb).otro(vlayout).margen(3)

        self.setLayout(layout)

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "aceptar":
            self.libro = self.cb.valor()
            self.siBlancas = self.rbBlancas.isChecked()
            self.jugContrario = self.cbRR.valor()
            self.jugJugador = self.chRJ.valor()
            self.guardaDIC()
            if self.siCambios:
                fvar = self.configuracion.ficheroBooks
                self.listaLibros.guardaVar(fvar)
            self.accept()
        elif accion == "cancelar":
            self.reject()

    def nuevo(self):
        fbin = QTUtil2.leeFichero(self, self.listaLibros.path, "bin", titulo=_("Polyglot book"))
        if fbin:
            self.listaLibros.path = os.path.dirname(fbin)
            nombre = os.path.basename(fbin)[:-4]
            b = Books.Libro("P", nombre, fbin, False)
            self.listaLibros.nuevo(b)
            self.siCambios = True
            self.rehacerCB(b)

    def borrar(self):
        libro = self.cb.valor()
        if libro:
            if QTUtil2.pregunta(self, _X(_("Delete from list the book %1?"), libro.nombre)):
                self.listaLibros.borra(libro)
                self.siCambios = True
                self.rehacerCB(None)

    def rehacerCB(self, inicial):
        li = [(x.nombre, x) for x in self.listaLibros.lista]
        if inicial is None:
            inicial = li[0][1] if li else None
        self.cb.rehacer(li, inicial)

    def recuperaDIC(self):
        return Util.recuperaVar(self.configuracion.ficheroTrainBooks)

    def guardaDIC(self):
        dic = {}
        dic["SIBLANCAS"] = self.rbBlancas.isChecked()
        libro = self.cb.valor()
        dic["NOMLIBRO"] = None if libro is None else libro.nombre
        dic["RR"] = self.cbRR.valor()
        dic["RJ"] = self.chRJ.valor()
        Util.guardaVar(self.configuracion.ficheroTrainBooks, dic)


def eligeJugadaBooks(pantalla, liJugadas, siBlancas, siSelectSiempre=True):
    pantalla.cursorFueraTablero()
    menu = QTVarios.LCMenu(pantalla)
    f = Controles.TipoLetra(nombre="Courier New", puntos=10)
    menu.ponFuente(f)

    titulo = _("White") if siBlancas else _("Black")
    icono = Iconos.Carpeta()

    menu.opcion(None, titulo, icono)
    menu.separador()

    icono = Iconos.PuntoNaranja() if siBlancas else Iconos.PuntoNegro()

    for desde, hasta, coronacion, pgn, peso in liJugadas:
        menu.opcion((desde, hasta, coronacion), pgn, icono)
        menu.separador()

    resp = menu.lanza()
    if resp:
        return resp
    else:
        if siSelectSiempre:
            desde, hasta, coronacion, pgn, peso = liJugadas[0]
            return desde, hasta, coronacion
        else:
            return None


def saltaJugadaBooks(gestor, liJugadas, jg):
    siBlancas = jg.posicionBase.siBlancas
    menu = QTVarios.LCMenu(gestor.pantalla)
    f = Controles.TipoLetra(nombre="Courier New", puntos=10)
    menu.ponFuente(f)

    icono = Iconos.PuntoNaranja() if siBlancas else Iconos.PuntoNegro()
    iconoActual = Iconos.Mover()

    for desde, hasta, coronacion, pgn, peso in liJugadas:
        ico = iconoActual if desde == jg.desde and hasta == jg.hasta else icono
        menu.opcion((desde, hasta, coronacion), pgn, ico)
        menu.separador()

    menu.opcion((None, None, None), _("Edit data"), Iconos.PuntoVerde())

    return menu.lanza()
