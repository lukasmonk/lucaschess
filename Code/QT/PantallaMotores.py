import operator
import os
import random

from PyQt4 import QtCore, QtGui

from Code import Books
from Code import EnginesMicElo
from Code import MotoresExternos
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import FormLayout
from Code import Util
from Code import VarGen


class WMotores(QTVarios.WDialogo):
    def __init__(self, owner, ficheroMExternos):

        self.listaMotores = MotoresExternos.ListaMotoresExternos(ficheroMExternos)
        self.listaMotores.leer()
        self.siCambios = False

        # Dialogo ---------------------------------------------------------------
        icono = Iconos.MotoresExternos()
        titulo = _("External engines")
        extparam = "motoresExternos"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        liAcciones = [
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("New"), Iconos.TutorialesCrear(), self.nuevo), None,
            (_("Modify"), Iconos.Modificar(), self.modificar), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Copy"), Iconos.Copiar(), self.copiar), None,
            (_("Import"), Iconos.MasDoc(), self.importar), None,
            (_("Up"), Iconos.Arriba(), self.arriba), None,
            (_("Down"), Iconos.Abajo(), self.abajo), None,
            (_("Command"), Iconos.Terminal(), self.command), None,
        ]
        tb = QTVarios.LCTB(self, liAcciones)

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

    def command(self):
        separador = FormLayout.separador
        liGen = [separador]
        liGen.append(separador)
        config = FormLayout.Fichero(_("File"), "exe", False)
        liGen.append((config, ""))

        for num in range(1, 11):
            liGen.append((_("Argument %d") % num + ":", ""))
        liGen.append(separador)
        resultado = FormLayout.fedit(liGen, title=_("Command"), parent=self, anchoMinimo=600, icon=Iconos.Terminal())
        if resultado:
            nada, resp = resultado
            command = resp[0]
            liArgs = []
            if not command or not os.path.isfile(command):
                return
            for x in range(1, len(resp)):
                arg = resp[x].strip()
                if arg:
                    liArgs.append(arg)

            um = QTUtil2.unMomento(self)
            me = MotoresExternos.MotorExterno()
            resp = me.leerUCI(command, liArgs)
            um.final()
            if not resp:
                QTUtil2.mensaje(self, _X(_("The file %1 does not correspond to a UCI engine type."), command))
                return None

            # Editamos
            w = WMotor(self, self.listaMotores, me)
            if w.exec_():
                self.listaMotores.nuevo(me)
                self.grid.refresh()
                self.grid.gobottom(0)
                self.grabar()

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

    def importar(self):
        menu = QTVarios.LCMenu(self)
        lista = VarGen.configuracion.comboMotores()
        nico = QTVarios.rondoPuntos()
        for nombre, clave in lista:
            menu.opcion(clave, nombre, nico.otro())

        resp = menu.lanza()
        if not resp:
            return

        cm = VarGen.configuracion.buscaRival(resp)
        me = MotoresExternos.MotorExterno()
        me.exe = cm.ejecutable()
        me.alias = cm.clave
        me.idName = cm.nombre
        me.nombre = cm.nombre
        me.clave = cm.clave
        me.idAuthor = cm.autor
        me.idInfo = ""
        me.liOpciones = []
        me.maxMultiPV = cm.maxMultiPV
        me.multiPV = cm.multiPV
        me.elo = cm.elo
        me.leerUCI(me.exe)
        for op in me.liOpciones:
            for comando, valor in cm.liUCI:
                if op.nombre == comando:
                    if op.tipo == "check":
                        op.valor = valor.lower() == "true"
                    elif op.tipo == "spin":
                        op.valor = int(valor)
                    else:
                        op.valor = valor
                    break

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
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint |
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

        lbExe = Controles.LB(self, motorExterno.exe)

        if siTorneo:
            lbDepth = Controles.LB(self, _("Maximum depth") + ": ")
            self.sbDepth = Controles.SB(self, motorExterno.depth(), 0, 50)

            lbTime = Controles.LB(self, _("Maximum seconds to think") + ": ")
            self.sbTime = Controles.SB(self, motorExterno.time(), 0, 9999)

            lbBook = Controles.LB(self, _("Opening book") + ": ")
            fvar = VarGen.configuracion.ficheroBooks
            self.listaLibros = Books.ListaLibros()
            self.listaLibros.recuperaVar(fvar)
            # # Comprobamos que todos esten accesibles
            self.listaLibros.comprueba()
            li = [(x.nombre, x.path) for x in self.listaLibros.lista]
            li.insert(0, ("* " + _("None"), "-"))
            li.insert(0, ("* " + _("Default"), "*"))
            self.cbBooks = Controles.CB(self, li, motorExterno.book())
            btNuevoBook = Controles.PB(self, "", self.nuevoBook, plano=False).ponIcono(Iconos.Nuevo(), tamIcon=16)
            # # Respuesta rival
            li = (
                (_("Uniform random"), "au"),
                (_("Proportional random"), "ap"),
                (_("Always the highest percentage"), "mp"),
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
        ly.controlc(lbExe, 3, 0, 1, 2)

        if siTorneo:
            ly.otro(lyTorneo, 4, 0, 1, 2)

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
            lb.ponTexto("%s [%d-%d] :" % (opcion.nombre, opcion.min, opcion.max))
        elif tipo == "check":
            control = Controles.CHB(owner, " ", opcion.valor)
        elif tipo == "combo":
            liVars = []
            for var in opcion.liVars:
                liVars.append((var, var))
            control = Controles.CB(owner, liVars, opcion.valor)
        elif tipo == "string":
            control = Controles.ED(owner, opcion.valor)
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
    exeMotor = QTUtil2.leeFichero(wowner, folderEngines if folderEngines else ".", "%s EXE (*.exe)" % _("File"), _("Engine"))
    if not exeMotor:
        return None
    folderEngines = Util.dirRelativo(os.path.dirname(exeMotor))
    VarGen.configuracion.escVariables("FOLDER_ENGINES", folderEngines)

    # Leemos el UCI
    me = MotoresExternos.MotorExterno()
    um = QTUtil2.unMomento(wowner)
    resp = me.leerUCI(exeMotor)
    um.final()
    if not resp:
        QTUtil2.mensaje(wowner, _X(_("The file %1 does not correspond to a UCI engine type."), exeMotor))
        return None
    return me


class WEligeMotorElo(QTVarios.WDialogo):
    def __init__(self, gestor, elo, titulo, icono, tipo):
        QTVarios.WDialogo.__init__(self, gestor.pantalla, titulo, icono, tipo.lower())

        self.siMicElo = tipo == "MICELO"
        self.siMicPer = tipo == "MICPER"
        self.siMic = self.siMicElo or self.siMicPer

        self.gestor = gestor

        self.colorNoJugable = QTUtil.qtColorRGB(241, 226, 226)
        self.colorMenor = QTUtil.qtColorRGB(245, 245, 245)
        self.colorMayor = None
        self.elo = elo
        self.tipo = tipo

        # Toolbar
        liAcciones = [(_("Choose"), Iconos.Aceptar(), self.elegir), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      (_("Random opponent"), Iconos.FAQ(), self.selectRandom), None,
                      ]
        if self.siMicElo:
            liAcciones.append((_("Reset"), Iconos.Reiniciar(), self.reset))
            liAcciones.append(None)

        self.tb = QTVarios.LCTB(self, liAcciones)

        self.liMotores = self.gestor.listaMotores(elo)
        self.liMotoresActivos = self.liMotores
        dicValores = VarGen.configuracion.leeVariables(tipo)
        if not dicValores:
            dicValores = {}

        liFiltro = (
            ("---", None),
            (">=", ">"),
            ("<=", "<"),
            ("+-100", "100"),
            ("+-200", "200"),
            ("+-400", "400"),
            ("+-800", "800"),
        )

        self.cbElo = Controles.CB(self, liFiltro, None).capturaCambiado(self.filtrar)

        minimo = 9999
        maximo = 0
        for mt in self.liMotores:
            if mt.siJugable:
                if mt.elo < minimo:
                    minimo = mt.elo
                if mt.elo > maximo:
                    maximo = mt.elo
        self.sbElo, lbElo = QTUtil2.spinBoxLB(self, elo, minimo, maximo, maxTam=50, etiqueta=_("Elo"))
        self.sbElo.capturaCambiado(self.filtrar)

        if self.siMic:
            liCaract = []
            st = set()
            for mt in self.liMotores:
                mt.liCaract = li = mt.idInfo.split("\n")
                mt.txtCaract = ", ".join([_F(x) for x in li])
                for x in li:
                    if x not in st:
                        st.add(x)
                        liCaract.append((_F(x), x))
            liCaract.sort(key=lambda x: x[1])
            liCaract.insert(0, ("---", None))
            self.cbCaract = Controles.CB(self, liCaract, None).capturaCambiado(self.filtrar)

        ly = Colocacion.H().control(lbElo).control(self.cbElo).control(self.sbElo)
        if self.siMic:
            ly.control(self.cbCaract)
        ly.relleno(1)
        gbRandom = Controles.GB(self, "", ly)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 35, siCentrado=True)
        oColumnas.nueva("MOTOR", _("Name"), 140)
        oColumnas.nueva("ELO", _("Elo"), 60, siDerecha=True)
        if not self.siMicPer:
            oColumnas.nueva("GANA", _("Win"), 80, siCentrado=True)
            oColumnas.nueva("TABLAS", _("Draw"), 80, siCentrado=True)
            oColumnas.nueva("PIERDE", _("Lost"), 80, siCentrado=True)
        if self.siMic:
            oColumnas.nueva("INFO", _("Information"), 300, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siCabeceraMovible=False, altoFila=24)
        n = self.grid.anchoColumnas()
        self.grid.setMinimumWidth(n + 20)
        self.registrarGrid(self.grid)

        f = Controles.TipoLetra(puntos=9)
        self.grid.setFont(f)

        self.grid.gotop()

        # Layout
        lyH = Colocacion.H().control(self.tb).control(gbRandom)
        layout = Colocacion.V().otro(lyH).control(self.grid).margen(3)
        self.setLayout(layout)

        self.filtrar()

        self.recuperarVideo()

    def removeReset(self):
        self.tb.setAccionVisible(self.reset, False)

    def filtrar(self):
        cb = self.cbElo.valor()
        elo = self.sbElo.valor()
        if cb is None:
            self.liMotoresActivos = self.liMotores
            self.sbElo.setDisabled(True)
        else:
            self.sbElo.setDisabled(False)
            if cb == ">":
                self.liMotoresActivos = [x for x in self.liMotores if x.elo >= elo]
            elif cb == "<":
                self.liMotoresActivos = [x for x in self.liMotores if x.elo <= elo]
            elif cb in ("100", "200", "400", "800"):
                mx = int(cb)
                self.liMotoresActivos = [x for x in self.liMotores if abs(x.elo - elo) <= mx]
        if self.siMic:
            cc = self.cbCaract.valor()
            if cc:
                self.liMotoresActivos = [mt for mt in self.liMotoresActivos if cc in mt.liCaract]
        self.grid.refresh()

    def reset(self):
        if not QTUtil2.pregunta(self, _("Are you sure you want to set the original elo of all engines?")):
            return

        self.gestor.configuracion.escVariables("DicMicElos", {})
        self.cancelar()

    def cancelar(self):
        self.resultado = None
        self.guardarVideo()
        self.reject()

    def elegir(self):
        f = self.grid.recno()
        mt = self.liMotoresActivos[f]
        if mt.siJugable:
            self.resultado = mt
            self.guardarVideo()
            self.accept()
        else:
            QTUtil.beep()

    def selectRandom(self):
        li = []
        for mt in self.liMotoresActivos:
            if mt.siJugable:
                li.append(mt)
        if li:
            n = random.randint(0, len(li) - 1)
            self.resultado = li[n]
            self.guardarVideo()
            self.accept()
        else:
            QTUtil2.mensError(self, _("There is not a playable engine between these values"))

    def gridDobleClick(self, grid, fila, oColumna):
        self.elegir()

    def gridNumDatos(self, grid):
        return len(self.liMotoresActivos)

    def gridWheelEvent(self, quien, siAdelante):
        n = len(self.liMotoresActivos)
        f, c = self.grid.posActualN()
        f += -1 if siAdelante else +1
        if 0 <= f < n:
            self.grid.goto(f, c)

    def gridColorFondo(self, grid, fila, oColumna):
        mt = self.liMotoresActivos[fila]
        if mt.siOut:
            return self.colorNoJugable
        else:
            return self.colorMenor if mt.elo < self.elo else self.colorMayor

    def gridDato(self, grid, fila, oColumna):
        mt = self.liMotoresActivos[fila]
        clave = oColumna.clave
        if clave == "NUMERO":
            valor = "%2d" % mt.numero
        elif clave == "MOTOR":
            valor = " " + mt.alias.strip()
        elif clave == "ELO":
            valor = "%d " % mt.elo
        elif clave == "INFO":
            valor = mt.txtCaract
        else:
            if not mt.siJugable:
                return "x"
            if clave == "GANA":
                pts = mt.pgana
            elif clave == "TABLAS":
                pts = mt.ptablas
            elif clave == "PIERDE":
                pts = mt.ppierde

            valor = "%+d" % pts

        return valor


def eligeMotorElo(gestor, elo):
    titulo = _("Lucas-Elo") + ". " + _("Choose the opponent")
    icono = Iconos.Elo()
    w = WEligeMotorElo(gestor, elo, titulo, icono, "ELO")
    if w.exec_():
        return w.resultado
    else:
        return None


def eligeMotorMicElo(gestor, elo):
    titulo = _("Club players competition") + ". " + _("Choose the opponent")
    icono = Iconos.EloTimed()
    w = WEligeMotorElo(gestor, elo, titulo, icono, "MICELO")
    if w.exec_():
        return w.resultado
    else:
        return None


def eligeMotorEntMaq(pantalla):
    titulo = _("Choose the opponent")
    icono = Iconos.EloTimed()

    class GestorTmp:

        def __init__(self):
            self.pantalla = pantalla
            self.configuracion = VarGen.configuracion

        def listaMotores(self, elo):
            li = EnginesMicElo.listaCompleta()
            numX = len(li)
            for num, mt in enumerate(li):
                mt.siJugable = True
                mt.siOut = False
                mt.numero = numX - num
            return li

    w = WEligeMotorElo(GestorTmp(), 1600, titulo, icono, "MICPER")
    if w.exec_():
        return w.resultado
    else:
        return None
