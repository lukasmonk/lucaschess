# -*- coding: latin-1 -*-

import os
import operator

from PyQt4 import QtCore, QtGui

import Code.VarGen as VarGen
import Code.Util as Util
import Code.Books as Books
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Columnas as Columnas
import Code.QT.Grid as Grid
import Code.QT.QTVarios as QTVarios
import Code.MotoresExternos as MotoresExternos

class WMotores(QTVarios.WDialogo):
    def __init__(self, owner, ficheroMExternos):

        self.listaMotores = MotoresExternos.ListaMotoresExternos(ficheroMExternos)
        self.listaMotores.leer()
        self.siCambios = False

        # Diálogo ---------------------------------------------------------------
        icono = Iconos.MotoresExternos()
        titulo = _("External engines")
        extparam = "motoresExternos"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        liAcciones = [
            ( _("Close"), Iconos.MainMenu(), self.terminar ), None,
            ( _("New"), Iconos.TutorialesCrear(), self.nuevo ), None,
            ( _("Modify"), Iconos.Modificar(), self.modificar ), None,
            ( _("Remove"), Iconos.Borrar(), self.borrar ), None,
            ( _("Copy"), Iconos.Copiar(), self.copiar ), None,
            ( _("Up"), Iconos.Arriba(), self.arriba ), None,
            ( _("Down"), Iconos.Abajo(), self.abajo ), None,
        ]
        tb = Controles.TBrutina(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ALIAS", _("Alias"), 200)
        oColumnas.nueva("MOTOR", _("Engine"), 200)
        oColumnas.nueva("AUTOR", _("Author"), 200)
        oColumnas.nueva("INFO", _("Information"), 120)
        oColumnas.nueva("ELO", "ELO", 120, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)
        # n = self.grid.anchoColumnas()
        # self.grid.setFixedWidth( n + 20 )
        self.registrarGrid(self.grid)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid)
        self.setLayout(layout)

        self.recuperarVideo(siTam=True)

    def grabar(self):
        self.listaMotores.grabar()

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def gridNumDatos(self, grid):
        return self.listaMotores.numDatos()

    def gridDato(self, grid, fila, oColumna):
        me = self.listaMotores.liMotores[fila]
        clave = oColumna.clave
        if clave == "AUTOR":
            return me.idAuthor
        elif clave == "ALIAS":
            return me.alias
        elif clave == "MOTOR":
            return me.idName
        elif clave == "INFO":
            return me.idInfo.replace("\n", "-")
        elif clave == "ELO":
            return str(me.elo) if me.elo else "-"

    def nuevo(self):
        me = selectEngine(self)
        if not me:
            return

        # Editamos
        w = WMotor(self, self.listaMotores, me)
        if w.exec_():
            self.listaMotores.nuevo(me)
            self.grid.refresh()
            self.grid.gobottom(0)
            self.grabar()

    def gridDobleClick(self, grid, fil, col):
        self.modificar()

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave
        if clave == "ALIAS":
            clave = "alias"
        elif clave == "MOTOR":
            clave = "idName"
        elif clave == "ELO":
            clave = "elo"
        else:
            return
        self.listaMotores.liMotores.sort(key=operator.attrgetter(clave))
        self.grid.refresh()
        self.grid.gotop()
        self.grabar()

    def modificar(self):
        if len(self.listaMotores.liMotores):
            fila = self.grid.recno()
            if fila >= 0:
                me = self.listaMotores.liMotores[fila]
                # Editamos, y graba si hace falta
                w = WMotor(self, self.listaMotores, me)
                if w.exec_():
                    self.grid.refresh()
                    self.grabar()

    def arriba(self):
        fila = self.grid.recno()
        if fila > 0:
            li = self.listaMotores.liMotores
            a, b = li[fila], li[fila - 1]
            li[fila], li[fila - 1] = b, a
            self.grid.goto(fila - 1, 0)
            self.grid.refresh()
            self.grabar()

    def abajo(self):
        fila = self.grid.recno()
        li = self.listaMotores.liMotores
        if fila < len(li) - 1:
            a, b = li[fila], li[fila + 1]
            li[fila], li[fila + 1] = b, a
            self.grid.goto(fila + 1, 0)
            self.grid.refresh()
            self.grabar()

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), self.listaMotores.liMotores[fila].alias)):
                del self.listaMotores.liMotores[fila]
                self.grid.refresh()
                self.grabar()

    def copiar(self):
        fila = self.grid.recno()
        if fila >= 0:
            me = self.listaMotores.liMotores[fila].copiar()
            w = WMotor(self, self.listaMotores, me)
            if w.exec_():
                self.listaMotores.nuevo(me)
                self.grid.refresh()
                self.grid.gobottom(0)
                self.grabar()

class WMotor(QtGui.QDialog):
    def __init__(self, wParent, listaMotores, motorExterno, siTorneo=False):

        super(WMotor, self).__init__(wParent)

        self.setWindowTitle(motorExterno.idName)
        self.setWindowIcon(Iconos.Motor())
        self.setWindowFlags(
            QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint | QtCore.Qt.WindowMinimizeButtonHint | QtCore.Qt.WindowMaximizeButtonHint)

        scrollArea = genOpcionesME(self, motorExterno)

        self.motorExterno = motorExterno
        self.liMotores = listaMotores if siTorneo else listaMotores.liMotores
        self.siTorneo = siTorneo

        # Toolbar
        tb = QTUtil2.tbAcceptCancel(self)

        lbAlias = Controles.LB(self, _("Alias") + ": ")
        self.edAlias = Controles.ED(self, motorExterno.alias).anchoMinimo(360)

        lbInfo = Controles.LB(self, _("Information") + ": ")
        self.emInfo = Controles.EM(self, motorExterno.idInfo, siHTML=False).anchoMinimo(360).altoFijo(60)

        lbElo = Controles.LB(self, "ELO" + ": ")
        self.sbElo = Controles.SB(self, motorExterno.elo, 0, 4000)

        if siTorneo:
            lbDepth = Controles.LB(self, _("Maximum depth") + ": ")
            self.sbDepth = Controles.SB(self, motorExterno.depth(), 0, 50)

            lbTime = Controles.LB(self, _("Maximum seconds to think") + ": ")
            self.sbTime = Controles.SB(self, motorExterno.time(), 0, 9999)

            lbBook = Controles.LB(self, _("Opening book") + ": ")
            fvar = VarGen.configuracion.ficheroBooks
            self.listaLibros = Books.ListaLibros()
            self.listaLibros.recuperaVar(fvar)
            # # Comprobamos que todos estén accesibles
            self.listaLibros.comprueba()
            li = [(x.nombre, x.path) for x in self.listaLibros.lista]
            li.insert(0, ("* " + _("Engine book"), "-"))
            li.insert(0, ("* " + _("Default"), "*"))
            self.cbBooks = Controles.CB(self, li, motorExterno.book())
            btNuevoBook = Controles.PB(self, "", self.nuevoBook, plano=False).ponIcono(Iconos.Nuevo(), tamIcon=16)
            # # Respuesta rival
            li = (
                (_("Uniform random"), "au" ),
                (_("Proportional random"), "ap" ),
                (_("Always the highest percentage"), "mp" ),
            )
            self.cbBooksRR = QTUtil2.comboBoxLB(self, li, motorExterno.bookRR())
            lyBook = Colocacion.H().control(lbBook).control(self.cbBooks).control(self.cbBooksRR).control(
                btNuevoBook).relleno()
            lyDT = Colocacion.H().control(lbDepth).control(self.sbDepth).espacio(40).control(lbTime).control(
                self.sbTime).relleno()
            lyTorneo = Colocacion.V().otro(lyDT).otro(lyBook)

        # Layout
        ly = Colocacion.G()
        ly.controld(lbAlias, 0, 0).control(self.edAlias, 0, 1)
        ly.controld(lbInfo, 1, 0).control(self.emInfo, 1, 1)
        ly.controld(lbElo, 2, 0).control(self.sbElo, 2, 1)

        if siTorneo:
            ly.otro(lyTorneo, 3, 0, 1, 2)

        layout = Colocacion.V().control(tb).espacio(30).otro(ly).control(scrollArea)
        self.setLayout(layout)

        self.edAlias.setFocus()

    def nuevoBook(self):
        fbin = QTUtil2.leeFichero(self, self.listaLibros.path, "bin", titulo=_("Polyglot book"))
        if fbin:
            self.listaLibros.path = os.path.dirname(fbin)
            nombre = os.path.basename(fbin)[:-4]
            b = Books.Libro("P", nombre, fbin, False)
            self.listaLibros.nuevo(b)
            fvar = VarGen.configuracion.ficheroBooks
            self.listaLibros.guardaVar(fvar)
            li = [(x.nombre, x.path) for x in self.listaLibros.lista]
            li.insert(0, ("* " + _("Engine book"), "-"))
            li.insert(0, ("* " + _("Default"), "*"))
            self.cbBooks.rehacer(li, b.path)

    def aceptar(self):
        # Comprobamos que no se repita el nombre
        alias = self.edAlias.texto().strip()
        for motor in self.liMotores:
            if (self.motorExterno != motor) and (motor.alias == alias):
                QTUtil2.mensError(self, _(
                    "There is already another engine with the same alias, the alias must change in order to have both."))
                return
        self.motorExterno.alias = alias
        self.motorExterno.idInfo = self.emInfo.texto()
        self.motorExterno.elo = self.sbElo.valor()

        if self.siTorneo:
            self.motorExterno.depth(self.sbDepth.valor())
            self.motorExterno.time(self.sbTime.valor())
            pbook = self.cbBooks.valor()
            self.motorExterno.book(pbook)
            self.motorExterno.bookRR(self.cbBooksRR.valor())

        # Grabamos opciones
        saveOpcionesME(self.motorExterno)

        self.accept()

def genOpcionesME(owner, motorExterno):
    fil = 0
    col = 0
    layout = Colocacion.G()
    for opcion in motorExterno.liOpciones:
        tipo = opcion.tipo
        lb = Controles.LB(owner, opcion.nombre + ":").alinDerecha()
        if tipo == "spin":
            control = QTUtil2.spinBoxLB(owner, opcion.valor, opcion.min, opcion.max,
                                        maxTam=50 if opcion.max < 1000 else 80)
            lb.ponTexto("%s [%d-%d] :" % ( opcion.nombre, opcion.min, opcion.max ))
        elif tipo == "check":
            control = Controles.CHB(owner, " ", opcion.valor)
        elif tipo == "combo":
            liVars = []
            for var in opcion.liVars:
                liVars.append((var, var))
            control = Controles.CB(owner, liVars, opcion.valor)
        elif tipo == "string":
            control = Controles.ED(owner, opcion.valor)  # todo check si cambiar self por None no causa problemas
        elif tipo == "button":
            control = Controles.CHB(owner, " ", opcion.valor)

        layout.controld(lb, fil, col).control(control, fil, col + 1)
        col += 2
        if col > 2:
            fil += 1
            col = 0
        opcion.control = control

    w = QtGui.QWidget(owner)
    w.setLayout(layout)
    scrollArea = QtGui.QScrollArea()
    scrollArea.setBackgroundRole(QtGui.QPalette.Light)
    scrollArea.setWidget(w)
    scrollArea.setWidgetResizable(True)

    return scrollArea

def saveOpcionesME(motorExterno):
    for opcion in motorExterno.liOpciones:
        tipo = opcion.tipo
        control = opcion.control
        if tipo == "spin":
            valor = control.value()
        elif tipo == "check":
            valor = control.isChecked()
        elif tipo == "combo":
            valor = control.valor()
        elif tipo == "string":
            valor = control.texto()
        elif tipo == "button":
            valor = control.isChecked()
        opcion.valor = valor

def selectEngine(wowner):
    """
    :param wowner: window
    :return: MotorExterno / None=error
    """
    # Pedimos el ejecutable
    folderEngines = VarGen.configuracion.leeVariables("FOLDER_ENGINES")
    exeMotor = QTUtil2.leeFichero(wowner, folderEngines if folderEngines else ".", "*", _("Engine"))
    if not exeMotor:
        return None
    folderEngines = Util.dirRelativo(os.path.dirname(exeMotor))
    VarGen.configuracion.escVariables("FOLDER_ENGINES", folderEngines)

    # Leemos el UCI
    me = MotoresExternos.MotorExterno()
    if not me.leerUCI(exeMotor):
        QTUtil2.mensaje(wowner, _X(_("The file %1 does not correspond to a UCI engine type."), exeMotor))
        return None

    return me
