import os

from PyQt4 import QtGui

from Code import Kibitzers
from Code import EngineThread
from Code import Books
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import QTUtil2
from Code.QT import Delegados
from Code.QT import FormLayout


class WKibitzers(QTVarios.WDialogo):
    def __init__(self, wParent, procesador):
        titulo = _("Kibitzers")
        icono = Iconos.Kibitzer()
        extparam = "kibitzer"
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        self.configuracion = procesador.configuracion
        self.procesador = procesador

        self.tipos = Kibitzers.Tipos()

        self.kibitzers = Kibitzers.Kibitzers()
        self.liKibActual = []

        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("New"), Iconos.Nuevo(), self.nuevo), None,
            (_("Remove"), Iconos.Borrar(), self.remove), None,
            (_("Copy"), Iconos.Copiar(), self.copy), None,
            (_("Up"), Iconos.Arriba(), self.up), None,
            (_("Down"), Iconos.Abajo(), self.down), None,
            (_("Polyglot book"), Iconos.Book(), self.polyglot), None,
            (_("External engines"), Iconos.Motores(), self.ext_engines), None
        )
        tb = Controles.TBrutina(self, liAcciones)

        self.splitter = QtGui.QSplitter(self)
        self.registrarSplitter(self.splitter, "kibitzers")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("TIPO", "", 30, siCentrado=True, edicion=Delegados.PmIconosBMT(self, dicIconos=self.tipos.dicDelegado()))
        oColumnas.nueva("NOMBRE", _("Kibitzer"), 209)
        self.gridKibitzers = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True, xid="kib")
        self.gridKibitzers.tipoLetra(puntos=self.configuracion.puntosPGN)
        self.registrarGrid(self.gridKibitzers)

        w = QtGui.QWidget()
        ly = Colocacion.V().control(self.gridKibitzers).margen(0)
        w.setLayout(ly)
        self.splitter.addWidget(w)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("CAMPO", _("Label"), 152, siDerecha=True)
        oColumnas.nueva("VALOR", _("Value"), 390, edicion=Delegados.MultiEditor(self))
        self.gridValores = Grid.Grid(self, oColumnas, siSelecFilas=False, xid="val", siEditable=True)
        self.gridValores.tipoLetra(puntos=self.configuracion.puntosPGN)
        self.registrarGrid(self.gridValores)

        w = QtGui.QWidget()
        ly = Colocacion.V().control(self.gridValores).margen(0)
        w.setLayout(ly)
        self.splitter.addWidget(w)

        self.splitter.setSizes([259, 562])  # por defecto

        ly = Colocacion.V().control(tb).control(self.splitter)
        self.setLayout(ly)

        self.recuperarVideo(anchoDefecto=849, altoDefecto=400)

        self.gridKibitzers.gotop()

    def polyglot(self):
        listaLibros = Books.ListaLibros()
        listaLibros.recuperaVar(self.configuracion.ficheroBooks)
        listaLibros.comprueba()
        menu = QTVarios.LCMenu(self)
        rondo = QTVarios.rondoPuntos()
        for book in listaLibros.lista:
            menu.opcion(("book", book), book.nombre, rondo.otro())
            menu.separador()
        menu.opcion(("install", None), _("Install new book"), Iconos.Nuevo())
        resp = menu.lanza()
        if resp:
            orden, book = resp
            if orden == "book":
                num = self.kibitzers.nuevoPolyglot(book)
                self.goto(num)
            elif orden == "install":
                fbin = QTUtil2.leeFichero(self, listaLibros.path, "bin", titulo=_("Polyglot book"))
                if fbin:
                    listaLibros.path = os.path.dirname(fbin)
                    nombre = os.path.basename(fbin)[:-4]
                    book = Books.Libro("P", nombre, fbin, True)
                    listaLibros.nuevo(book)
                    listaLibros.guardaVar(self.configuracion.ficheroBooks)
                    return self.polyglot()

    def me_setEditor(self, parent):
        recno = self.gridValores.recno()
        key = self.liKibActual[recno][2]
        nk = self.krecno()
        kibitzer = self.kibitzers.kibitzer(nk)
        valor = control = lista = minimo = maximo = None
        if key is None:
            return None
        elif key == "nombre":
            control = "ed"
            valor = kibitzer.nombre
        elif key == "tipo":
            valor = kibitzer.tipo
            if valor in "IB": # Indices/books no se cambian
                return None
            control = "cb"
            lista = Kibitzers.Tipos().comboSinIndices()
        elif key == "prioridad":
            control = "cb"
            lista = EngineThread.priorities.combo()
            valor = kibitzer.prioridad
        elif key == "posicionBase":
            kibitzer.posicionBase = not kibitzer.posicionBase
            self.kibitzers.save()
            self.goto(nk)
        elif key == "visible":
            kibitzer.visible = not kibitzer.visible
            self.kibitzers.save()
            self.goto(nk)
        elif key == "info":
            control = "ed"
            valor = kibitzer.idInfo
        elif key.startswith("opcion"):
            opcion = kibitzer.liOpciones[int(key[7:])]
            tipo = opcion.tipo
            valor = opcion.valor
            if tipo == "spin":
                control = "sb"
                minimo = opcion.min
                maximo = opcion.max
            elif tipo in ("check", "button"):
                opcion.valor = not valor
                self.kibitzers.save()
                self.goto(nk)
            elif tipo == "combo":
                lista = [(var, var) for var in opcion.liVars]
                control = "cb"
            elif tipo == "string":
                control = "ed"

        self.me_control = control
        self.me_key = key

        if control == "ed":
            return Controles.ED(parent, valor)
        elif control == "cb":
            return Controles.CB(parent, lista, valor)
        elif control == "sb":
            return Controles.SB(parent, valor, minimo, maximo)
        return None

    def me_ponValor(self, editor, valor):
        if self.me_control == "ed":
            editor.setText(str(valor))
        elif self.me_control in ("cb", "sb"):
            editor.ponValor(valor)

    def me_leeValor(self, editor):
        if self.me_control == "ed":
            return editor.texto()
        elif self.me_control in ("cb", "sb"):
            return editor.valor()

    def gridPonValor(self, grid, nfila, columna, valor):
        nk = self.krecno()
        kibitzer = self.kibitzers.kibitzer(nk)
        if self.me_key == "nombre":
            valor = valor.strip()
            if valor:
                kibitzer.nombre = valor
        elif self.me_key == "tipo":
            kibitzer.tipo = valor
        elif self.me_key == "prioridad":
            kibitzer.prioridad = valor
        elif self.me_key == "info":
            kibitzer.idInfo = valor.strip()
        elif self.me_key.startswith("opcion"):
            opcion = kibitzer.liOpciones[int(self.me_key[7:])]
            opcion.valor = valor
        self.kibitzers.save()
        self.goto(nk)

    def ext_engines(self):
        self.procesador.motoresExternos()

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def closeEvent(self, QCloseEvent):
        self.guardarVideo()

    def nuevo(self):
        liGen = [(None, None)]
        liGen.append((_("Kibitzer") + ":", ""))

        config = FormLayout.Combobox(_("Engine"), self.configuracion.comboMotoresCompleto())
        liGen.append((config, "stockfish"))

        liTipos = Kibitzers.Tipos().combo()
        config = FormLayout.Combobox(_("Type"), liTipos)
        liGen.append((config, "M"))

        config = FormLayout.Combobox(_("Process priority"), EngineThread.priorities.combo())
        liGen.append((config, EngineThread.priorities.normal))

        resultado = FormLayout.fedit(liGen, title=_("New"), parent=self, anchoMinimo=460, icon=Iconos.Kibitzer())

        if resultado:
            accion, resp = resultado

            nombre, motor, tipo, prioridad = resp

            # Indexes only with Rodent II
            if tipo == "I":
                motor = "rodentII"
                if not nombre: # para que no repita rodent II
                    nombre = _("Indexes") + " - RodentII"

            nombre = nombre.strip()
            if not nombre:
                for label, key in liTipos:
                    if key == tipo:
                        nombre = "%s: %s" % (label, motor)
            num = self.kibitzers.nuevo(nombre, motor, tipo, prioridad)
            self.goto(num)

    def remove(self):
        if self.kibitzers.lista:
            num = self.krecno()
            kib = self.kibitzers.kibitzer(num)
            if QTUtil2.pregunta(self, _("Are you sure?") + "\n %s" % kib.nombre):
                self.kibitzers.remove(num)
                self.gridKibitzers.refresh()
                nk = len(self.kibitzers)
                if nk > 0:
                    if num > nk:
                        num = nk -1
                    self.goto(num)

    def copy(self):
        num = self.krecno()
        if num >= 0:
            num = self.kibitzers.clonar(num)
            self.goto(num)

    def goto(self, num):
        self.gridKibitzers.goto(num, 0)
        self.gridKibitzers.refresh()
        self.actKibitzer()
        self.gridValores.refresh()

    def krecno(self):
        return self.gridKibitzers.recno()

    def up(self):
        num = self.kibitzers.up(self.krecno())
        if num is not None:
            self.goto(num)

    def down(self):
        num = self.kibitzers.down(self.krecno())
        if num is not None:
            self.goto(num)

    def gridNumDatos(self, grid):
        gid = grid.id
        if gid == "kib":
            return len(self.kibitzers)
        return len(self.liKibActual)

    def gridDato(self, grid, fila, oColumna):
        columna = oColumna.clave
        gid = grid.id
        if gid == "kib":
            return self.gridDatoKibitzers(fila, columna)
        elif gid == "val":
            return self.gridDatoValores(fila, columna)

    def gridDatoKibitzers(self, fila, columna):
        me = self.kibitzers.kibitzer(fila)
        if columna == "NOMBRE":
            return me.nombre
        elif columna == "TIPO":
            return me.tipo

    def gridDatoValores(self, fila, columna):
        li = self.liKibActual[fila]
        if columna == "CAMPO":
            return li[0]
        else:
            return li[1]

    def gridCambiadoRegistro(self, grid, fila, columna):
        if grid.id == "kib":
            self.goto(fila)

    def actKibitzer(self):
        self.liKibActual = []
        fila = self.krecno()
        if fila < 0:
            return

        me = self.kibitzers.kibitzer(fila)
        tipo = me.tipo
        self.liKibActual.append((_("Name"), me.nombre, "nombre"))
        self.liKibActual.append((_("Type"), me.ctipo(), "tipo"))
        self.liKibActual.append((_("Priority"), me.cpriority(), "prioridad"))

        self.liKibActual.append((_("Analysis of the base position"), str(me.posicionBase), "posicionBase"))
        self.liKibActual.append((_("Visible in menu"), str(me.visible), "visible"))

        if tipo != "B":
            self.liKibActual.append((_("Engine"), me.idName, None))

            self.liKibActual.append((_("Author"), me.idAuthor, None))
        self.liKibActual.append((_("File"), me.exe, None))
        if tipo != "B":
            self.liKibActual.append((_("Information"), me.idInfo, "info"))

            for num, opcion in enumerate(me.liOpciones):
                default = opcion.label_default()
                label_default = " (%s)" % default if default else ""
                valor = str(opcion.valor)
                if opcion.tipo in ("check", "button"):
                    valor = valor.lower()
                self.liKibActual.append(("%s%s" % (opcion.nombre, label_default), valor, "opcion,%d" % num))


class WKibitzerLive(QTVarios.WDialogo):
    def __init__(self, wParent, configuracion, numkibitzer):
        self.kibitzers = Kibitzers.Kibitzers()
        self.kibitzer = self.kibitzers.kibitzer(numkibitzer)
        titulo = self.kibitzer.nombre
        icono = Iconos.Kibitzer()
        extparam = "kibitzerlive"
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        self.configuracion = configuracion

        self.liOpciones = self.leeOpciones()
        self.liOriginal = self.leeOpciones()

        liAcciones = (
            (_("Save"), Iconos.Grabar(), self.grabar), None,
            (_("Cancel"), Iconos.Cancelar(), self.reject), None,
        )
        tb = Controles.TBrutina(self, liAcciones)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("CAMPO", _("Label"), 152, siDerecha=True)
        oColumnas.nueva("VALOR", _("Value"), 390, edicion=Delegados.MultiEditor(self))
        self.gridValores = Grid.Grid(self, oColumnas, siSelecFilas=False, xid="val", siEditable=True)
        self.gridValores.tipoLetra(puntos=self.configuracion.puntosPGN)
        self.registrarGrid(self.gridValores)

        ly = Colocacion.V().control(tb).control(self.gridValores)
        self.setLayout(ly)

        self.recuperarVideo(anchoDefecto=600, altoDefecto=400)

        self.gridValores.gotop()

    def leeOpciones(self):
        li = []
        li.append([_("Priority"), self.kibitzer.cpriority(), "prioridad"])
        li.append([_("Analysis of the base position"), str(self.kibitzer.posicionBase), "posicionBase"])
        for num, opcion in enumerate(self.kibitzer.liOpciones):
            default = opcion.label_default()
            label_default = " (%s)" % default if default else ""
            valor = str(opcion.valor)
            if opcion.tipo in ("check", "button"):
                valor = valor.lower()
            li.append(["%s%s" % (opcion.nombre, label_default), valor, "%d" % num])
        return li

    def grabar(self):
        self.kibitzers.save()
        lidif_opciones = []
        xprioridad = None
        xposicionBase = None
        for x in range(len(self.liOpciones)):
            if self.liOpciones[x][1] != self.liOriginal[x][1]:
                key = self.liOpciones[x][2]
                if key == "prioridad":
                    prioridad = self.kibitzer.prioridad
                    priorities = EngineThread.priorities
                    xprioridad = priorities.value(prioridad)
                elif key == "posicionBase":
                    xposicionBase = self.kibitzer.posicionBase
                else:
                    numopcion = int(key)
                    opcion = self.kibitzer.liOpciones[numopcion]
                    lidif_opciones.append((opcion.nombre, opcion.valor))

        self.result_xprioridad = xprioridad
        self.result_opciones = lidif_opciones
        self.result_posicionBase = xposicionBase
        self.guardarVideo()
        self.accept()

    def me_setEditor(self, parent):
        recno = self.gridValores.recno()
        key = self.liOpciones[recno][2]
        control = lista = minimo = maximo = None
        if key == "prioridad":
            control = "cb"
            lista = EngineThread.priorities.combo()
            valor = self.kibitzer.prioridad
        elif key == "posicionBase":
            self.kibitzer.posicionBase = not self.kibitzer.posicionBase
            self.liOpciones[recno][1] = self.kibitzer.posicionBase
            self.gridValores.refresh()
        else:
            opcion = self.kibitzer.liOpciones[int(key)]
            tipo = opcion.tipo
            valor = opcion.valor
            if tipo == "spin":
                control = "sb"
                minimo = opcion.min
                maximo = opcion.max
            elif tipo in ("check", "button"):
                opcion.valor = not valor
                self.liOpciones[recno][1] = opcion.valor
                self.gridValores.refresh()
            elif tipo == "combo":
                lista = [(var, var) for var in opcion.liVars]
                control = "cb"
            elif tipo == "string":
                control = "ed"

        self.me_control = control
        self.me_key = key

        if control == "ed":
            return Controles.ED(parent, valor)
        elif control == "cb":
            return Controles.CB(parent, lista, valor)
        elif control == "sb":
            return Controles.SB(parent, valor, minimo, maximo)
        return None

    def me_ponValor(self, editor, valor):
        if self.me_control == "ed":
            editor.setText(str(valor))
        elif self.me_control in ("cb", "sb"):
            editor.ponValor(valor)

    def me_leeValor(self, editor):
        if self.me_control == "ed":
            return editor.texto()
        elif self.me_control in ("cb", "sb"):
            return editor.valor()

    def gridPonValor(self, grid, nfila, columna, valor):
        if self.me_key == "prioridad":
            self.kibitzer.prioridad = valor
        else:
            nopcion = int(self.me_key)
            opcion = self.kibitzer.liOpciones[nopcion]
            opcion.valor = valor
            self.liOpciones[nopcion+1][1] = valor

    def gridNumDatos(self, grid):
        return len(self.liOpciones)

    def gridDato(self, grid, fila, oColumna):
        columna = oColumna.clave
        li = self.liOpciones[fila]
        if columna == "CAMPO":
            return li[0]
        else:
            return li[1]
