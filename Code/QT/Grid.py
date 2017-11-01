"""
El grid es un TableView de QT.

Realiza llamadas a rutinas de la ventana donde esta ante determinados eventos, o en determinadas situaciones,
siempre que la rutina se haya definido en la ventana:

    - gridDobleClickCabecera : ante un doble click en la cabecera, normalmente se usa para la reordenacion de la tabla por la columna pulsada.
    - gridTeclaPulsada : al pulsarse una tecla, llama a esta rutina, para que pueda usarse por ejemplo en busquedas.
    - gridTeclaControl : al pulsarse una tecla de control, llama a esta rutina, para que pueda usarse por ejemplo en busquedas.
    - gridDobleClick : en el caso de un doble click en un registro, se hace la llamad a esta rutina
    - gridBotonDerecho : si se ha pulsado el boton derecho del raton.
    - gridPonValor : si hay un campo editable, la llamada se produce cuando se ha cambiado el valor tras la edicion.

    - gridColorTexto : si esta definida se la llama al mostrar el texto de un campo, para determinar el color del mismo.
    - gridColorFondo : si esta definida se la llama al mostrar el texto de un campo, para determinar el color del fondo del mismo.

"""

from PyQt4 import QtCore, QtGui

from Code.QT import QTUtil
from Code import VarGen


class ControlGrid(QtCore.QAbstractTableModel):
    """
    Modelo de datos asociado al grid, y que realiza todo el trabajo asignado por QT.
    """

    def __init__(self, grid, wParent, oColumnasR):
        """
        @param tableView:
        @param oColumnasR: ListaColumnas con la configuracion de todas las columnas visualizables.
        """
        QtCore.QAbstractTableModel.__init__(self, wParent)
        self.grid = grid
        self.wParent = wParent
        self.siOrden = False
        self.hh = grid.horizontalHeader()
        self.siColorTexto = hasattr(self.wParent, "gridColorTexto")
        self.siColorFondo = hasattr(self.wParent, "gridColorFondo")
        self.siAlineacion = hasattr(self.wParent, "gridAlineacion")
        self.font = grid.font()
        self.siBold = hasattr(self.wParent, "gridBold")
        if self.siBold:
            self.bfont = QtGui.QFont(self.font)
            self.bfont.setWeight(75)

        self.oColumnasR = oColumnasR

    def rowCount(self, parent):
        """
        Llamada interna, solicitando el numero de registros.
        """
        self.numDatos = self.wParent.gridNumDatos(self.grid)
        return self.numDatos

    def refresh(self):
        """
        Si hay un cambio del numero de registros, la llamada a esta rutina actualiza la visualizacion.
        """
        self.emit(QtCore.SIGNAL("layoutAboutToBeChanged()"))
        ant_ndatos = self.numDatos
        nue_ndatos = self.wParent.gridNumDatos(self.grid)
        if ant_ndatos != nue_ndatos:
            if ant_ndatos < nue_ndatos:
                self.insertRows(ant_ndatos, nue_ndatos - ant_ndatos)
            else:
                self.removeRows(nue_ndatos, ant_ndatos - nue_ndatos)

        ant_ncols = self.numCols
        nue_ncols = self.oColumnasR.numColumnas()
        if ant_ncols != nue_ncols:
            if ant_ncols < nue_ncols:
                self.insertColumns(0, nue_ncols - ant_ncols)
            else:
                self.removeColumns(nue_ncols, ant_ncols - nue_ncols)

        self.emit(QtCore.SIGNAL("layoutChanged()"))

    def columnCount(self, parent):
        """
        Llamada interna, solicitando el numero de columnas.
        """
        self.numCols = self.oColumnasR.numColumnas()
        return self.numCols

    def data(self, index, role):
        """
        Llamada interna, solicitando informacion que ha de tener/contener el campo actual.
        """
        if not index.isValid():
            return None

        columna = self.oColumnasR.columna(index.column())

        if role == QtCore.Qt.TextAlignmentRole:
            if self.siAlineacion:
                resp = self.wParent.gridAlineacion(self.grid, index.row(), columna)
                if resp:
                    return columna.QTalineacion(resp)
            return columna.qtAlineacion
        elif role == QtCore.Qt.BackgroundRole:
            if self.siColorFondo:
                resp = self.wParent.gridColorFondo(self.grid, index.row(), columna)
                if resp:
                    return resp
            return columna.qtColorFondo
        elif role == QtCore.Qt.TextColorRole:
            if self.siColorTexto:
                resp = self.wParent.gridColorTexto(self.grid, index.row(), columna)
                if resp:
                    return resp
            return columna.qtColorTexto
        elif self.siBold and role == QtCore.Qt.FontRole:
            if self.wParent.gridBold(self.grid, index.row(), columna):
                return self.bfont
            return None

        if columna.siChecked:
            if role == QtCore.Qt.CheckStateRole:
                valor = self.wParent.gridDato(self.grid, index.row(), columna)
                return QtCore.Qt.Checked if valor else QtCore.Qt.Unchecked
        elif role == QtCore.Qt.DisplayRole:
            return self.wParent.gridDato(self.grid, index.row(), columna)

        return None

    def getAlineacion(self, index):
        columna = self.oColumnasR.columna(index.column())
        return self.wParent.gridAlineacion(self.grid, index.row(), columna)

    def getFondo(self, index):
        columna = self.oColumnasR.columna(index.column())
        return self.wParent.gridColorFondo(self.grid, index.row(), columna)

    def flags(self, index):
        """
        Llamada interna, solicitando mas informacion sobre las carcateristicas del campo actual.
        """
        if not index.isValid():
            return QtCore.Qt.ItemIsEnabled

        flag = QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable
        columna = self.oColumnasR.columna(index.column())
        if columna.siEditable:
            flag |= QtCore.Qt.ItemIsEditable

        if columna.siChecked:
            flag |= QtCore.Qt.ItemIsUserCheckable
        return flag

    def setData(self, index, valor, role=QtCore.Qt.EditRole):
        """
        Tras producirse la edicion de un campo en un registro se llama a esta rutina para cambiar el valor en el origen de los datos.
        Se lanza gridPonValor en la ventana propietaria.
        """
        if not index.isValid():
            return None
        if role == QtCore.Qt.EditRole or role == QtCore.Qt.CheckStateRole:
            columna = self.oColumnasR.columna(index.column())
            nfila = index.row()
            self.wParent.gridPonValor(self.grid, nfila, columna, valor)
            index2 = self.createIndex(nfila, 1)
            self.emit(QtCore.SIGNAL('dataChanged(const QModelIndex &,const QModelIndex &)'), index2, index2)

        return True

    def headerData(self, col, orientation, role):
        """
        Llamada interna, para determinar el texto de las cabeceras de las columnas.
        """
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            columna = self.oColumnasR.columna(col)
            return columna.cabecera
        return None


class Cabecera(QtGui.QHeaderView):
    """
    Se crea esta clase para poder implementar el doble click en la cabecera.
    """

    def __init__(self, tvParent, siCabeceraMovible):
        QtGui.QHeaderView.__init__(self, QtCore.Qt.Horizontal)
        self.setMovable(siCabeceraMovible)
        self.setClickable(True)
        self.tvParent = tvParent

    def mouseDoubleClickEvent(self, event):
        numColumna = self.logicalIndexAt(event.x(), event.y())
        self.tvParent.dobleClickCabecera(numColumna)
        return QtGui.QHeaderView.mouseDoubleClickEvent(self, event)

    def mouseReleaseEvent(self, event):
        QtGui.QHeaderView.mouseReleaseEvent(self, event)
        numColumna = self.logicalIndexAt(event.x(), event.y())
        self.tvParent.mouseCabecera(numColumna)


class CabeceraHeight(Cabecera):
    def __init__(self, tvParent, siCabeceraMovible, height):
        Cabecera.__init__(self, tvParent, siCabeceraMovible)
        self.height = height

    def sizeHint(self):
        baseSize = Cabecera.sizeHint(self)
        baseSize.setHeight(self.height)
        return baseSize


class Grid(QtGui.QTableView):
    """
    Implementa un TableView, en base a la configuracion de una lista de columnas.
    """

    def __init__(self, wParent, oColumnas, dicVideo=None, altoFila=20, siSelecFilas=False, siSeleccionMultiple=False,
                 siLineas=True, siEditable=False, siCabeceraMovible=True, xid=None, background="",
                 siCabeceraVisible=True, altoCabecera=None):
        """
        @param wParent: ventana propietaria
        @param oColumnas: configuracion de las columnas.
        @param altoFila: altura de todas las filas.
        """

        assert wParent is not None

        QtGui.QTableView.__init__(self)

        if VarGen.configuracion.tablaSelBackground:
            p = self.palette()
            p.setBrush(QtGui.QPalette.Inactive, QtGui.QPalette.Highlight, QtGui.QBrush(QtGui.QColor(VarGen.configuracion.tablaSelBackground)))
            p.setBrush(QtGui.QPalette.Active, QtGui.QPalette.Highlight, QtGui.QBrush(QtGui.QColor(VarGen.configuracion.tablaSelBackground)))
            self.setPalette(p)

        self.wParent = wParent
        self.id = xid

        self.oColumnas = oColumnas
        if dicVideo:
            self.recuperarVideo(dicVideo)
        self.oColumnasR = self.oColumnas.columnasMostrables()  # Necesario tras recuperar video

        self.cg = ControlGrid(self, wParent, self.oColumnasR)

        self.setModel(self.cg)
        self.setShowGrid(siLineas)

        if background == "":
            self.setStyleSheet("QTableView {background: %s;}" % QTUtil.backgroundGUI())
        elif background is not None:
            self.setStyleSheet("QTableView {background: %s;}" % background)

        self.coloresAlternados()

        if altoCabecera:
            hh = CabeceraHeight(self, siCabeceraMovible, altoCabecera)
        else:
            hh = Cabecera(self, siCabeceraMovible)
        self.setHorizontalHeader(hh)
        if not siCabeceraVisible:
            hh.setVisible(False)

        vh = self.verticalHeader()
        vh.setResizeMode(QtGui.QHeaderView.Fixed)
        vh.setDefaultSectionSize(altoFila)
        vh.setVisible(False)

        self.seleccionaFilas(siSelecFilas, siSeleccionMultiple)

        self.ponAnchosColumnas()  # es necesario llamarlo desde aqui

        self.siEditable = siEditable

    def buscaCabecera(self, clave):
        return self.oColumnas.buscaColumna(clave)

    def coloresAlternados(self):
        self.setAlternatingRowColors(True)

    def seleccionaFilas(self, siSelecFilas, siSeleccionMultiple):
        sel = QtGui.QAbstractItemView.SelectRows if siSelecFilas else QtGui.QAbstractItemView.SelectItems
        if siSeleccionMultiple:
            selMode = QtGui.QAbstractItemView.ExtendedSelection
        else:
            selMode = QtGui.QAbstractItemView.SingleSelection
        self.setSelectionMode(selMode)
        self.setSelectionBehavior(sel)

    def releerColumnas(self):
        """
        Cuando se cambia la configuracion de las columnas, se vuelven a releer y se indican al control de datos.
        """
        self.oColumnasR = self.oColumnas.columnasMostrables()
        self.cg.oColumnasR = self.oColumnasR
        self.cg.refresh()
        self.ponAnchosColumnas()

    def ponAnchosColumnas(self):
        for numCol, columna in enumerate(self.oColumnasR.liColumnas):
            self.setColumnWidth(numCol, columna.ancho)
            if columna.edicion and columna.siMostrar:
                self.setItemDelegateForColumn(numCol, columna.edicion)

    def keyPressEvent(self, event):
        """
        Se gestiona este evento, ante la posibilidad de que la ventana quiera controlar,
        cada tecla pulsada, llamando a la rutina correspondiente si existe (gridTeclaPulsada/gridTeclaControl)
        """
        resp = QtGui.QTableView.keyPressEvent(self, event)
        k = event.key()
        m = int(event.modifiers())
        siShift = (m & QtCore.Qt.ShiftModifier) > 0
        siControl = (m & QtCore.Qt.ControlModifier) > 0
        siAlt = (m & QtCore.Qt.AltModifier) > 0
        if hasattr(self.wParent, "gridTeclaPulsada"):
            if not (siControl or siAlt) and k < 256:
                self.wParent.gridTeclaPulsada(self, event.text())
        if hasattr(self.wParent, "gridTeclaControl"):
            self.wParent.gridTeclaControl(self, k, siShift, siControl, siAlt)
        return resp

    def selectionChanged(self, uno, dos):
        if hasattr(self.wParent, "gridCambiadoRegistro"):
            fil, columna = self.posActual()
            self.wParent.gridCambiadoRegistro(self, fil, columna)
        self.refresh()

    def wheelEvent(self, event):
        if hasattr(self.wParent, "gridWheelEvent"):
            self.wParent.gridWheelEvent(self, event.delta() > 0)
        else:
            QtGui.QTableView.wheelEvent(self, event)

    def mouseDoubleClickEvent(self, event):
        """
        Se gestiona este evento, ante la posibilidad de que la ventana quiera controlar,
        cada doble click, llamando a la rutina correspondiente si existe (gridDobleClick)
        con el numero de fila y el objeto columna como argumentos
        """
        if self.siEditable:
            QtGui.QTableView.mouseDoubleClickEvent(self, event)
        if hasattr(self.wParent, "gridDobleClick") and event.button() == 1:
            fil, columna = self.posActual()
            self.wParent.gridDobleClick(self, fil, columna)

    def mousePressEvent(self, event):
        """
        Se gestiona este evento, ante la posibilidad de que la ventana quiera controlar,
        cada pulsacion del boton derecho, llamando a la rutina correspondiente si existe (gridBotonDerecho)
        """
        QtGui.QTableView.mousePressEvent(self, event)
        button = event.button()
        if button == 2:
            if hasattr(self.wParent, "gridBotonDerecho"):
                class Vacia:
                    pass

                modif = Vacia()
                m = int(event.modifiers())
                modif.siShift = (m & QtCore.Qt.ShiftModifier) > 0
                modif.siControl = (m & QtCore.Qt.ControlModifier) > 0
                modif.siAlt = (m & QtCore.Qt.AltModifier) > 0
                fil, col = self.posActual()
                self.wParent.gridBotonDerecho(self, fil, col, modif)
        elif button == 1:
            if hasattr(self.wParent, "gridBotonIzquierdo"):
                fil, col = self.posActual()
                self.wParent.gridBotonIzquierdo(self, fil, col)

    def dobleClickCabecera(self, numColumna):
        """
        Se gestiona este evento, ante la posibilidad de que la ventana quiera controlar,
        los doble clicks sobre la cabecera , normalmente para cambiar el orden de la columna,
        llamando a la rutina correspondiente si existe (gridDobleClickCabecera) y con el
        argumento del objeto columna
        """
        if hasattr(self.wParent, "gridDobleClickCabecera"):
            self.wParent.gridDobleClickCabecera(self, self.oColumnasR.columna(numColumna))

    def mouseCabecera(self, numColumna):
        """
        Se gestiona este evento, ante la posibilidad de que la ventana quiera controlar,
        los doble clicks sobre la cabecera , normalmente para cambiar el orden de la columna,
        llamando a la rutina correspondiente si existe (gridDobleClickCabecera) y con el
        argumento del objeto columna
        """
        if hasattr(self.wParent, "gridMouseCabecera"):
            self.wParent.gridMouseCabecera(self, self.oColumnasR.columna(numColumna))

    def guardarVideo(self, dic):
        """
        Guarda en el diccionario de video la configuracion actual de todas las columnas

        @param dic: diccionario de video donde se guarda la configuracion de las columnas
        """
        liClaves = []
        for n, columna in enumerate(self.oColumnasR.liColumnas):
            columna.ancho = self.columnWidth(n)
            columna.posicion = self.columnViewportPosition(n)
            columna.guardarConf(dic, self)
            liClaves.append(columna.clave)

        # Las que no se muestran
        for columna in self.oColumnas.liColumnas:
            if columna.clave not in liClaves:
                columna.guardarConf(dic, self)

    def recuperarVideo(self, dic):
        """
        Recupera del diccionario de video la configuracion actual de todas las columnas
        """

        # Miramos en dic, si hay columnas calculadas y las a_adimos
        liCalc = [k.split(".")[0] for k in dic.keys() if k.startswith("CALC_")]
        if liCalc:
            s = set(liCalc)
            for clave in s:
                columna = self.oColumnas.nueva(clave, siOrden=False)

        # Leemos todas las columnas
        for columna in self.oColumnas.liColumnas:
            columna.recuperarConf(dic, self)

        self.oColumnas.liColumnas.sort(lambda x, y: cmp(x.posicion, y.posicion))

    def anchoColumnas(self):
        """
        Calcula el ancho que corresponde a todas las columnas mostradas.
        """
        ancho = 0
        for n, columna in enumerate(self.oColumnasR.liColumnas):
            ancho += columna.ancho
        return ancho

    def fixMinWidth(self):
        nAncho = self.anchoColumnas() + 24
        self.setMinimumWidth(nAncho)
        return nAncho

    def recno(self):
        """
        Devuelve la fila actual.
        """
        n = self.currentIndex().row()
        nX = self.cg.numDatos - 1
        return n if n <= nX else nX

    def reccount(self):
        return self.cg.numDatos

    def recnosSeleccionados(self):
        li = []
        for x in self.selectionModel().selectedIndexes():
            li.append(x.row())

        return list(set(li))

    def goto(self, fila, col):
        """
        Se situa en una posicion determinada.
        """
        elem = self.cg.createIndex(fila, col)
        self.setCurrentIndex(elem)
        self.scrollTo(elem)

    def gotop(self):
        """
        Se situa al principio del grid.
        """
        if self.cg.numDatos > 0:
            self.goto(0, 0)

    def gobottom(self, col=0):
        """
        Se situa en el ultimo registro del frid.
        """
        if self.cg.numDatos > 0:
            self.goto(self.cg.numDatos - 1, col)

    def refresh(self):
        """
        Hace un refresco de la visualizacion del grid, ante algun cambio en el contenido.
        """
        self.cg.refresh()

    def posActual(self):
        """
        Devuelve la posicion actual.

        @return: tupla con ( num fila, objeto columna )
        """
        columna = self.oColumnasR.columna(self.currentIndex().column())
        return self.recno(), columna

    def posActualN(self):
        """
        Devuelve la posicion actual.

        @return: tupla con ( num fila, num  columna )
        """
        return self.recno(), self.currentIndex().column()

    def tipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        font = QtGui.QFont()
        if txt is None:
            cursiva = 1 if siCursiva else 0
            subrayado = 1 if siSubrayado else 0
            tachado = 1 if siTachado else 0
            txt = "%s,%d,-1,5,%d,%d,%d,%d,0,0" % (nombre, puntos, peso, cursiva, subrayado, tachado)
        font.fromString(txt)
        self.setFont(font)

    def ponAltoFila(self, altoFila):
        vh = self.verticalHeader()
        vh.setResizeMode(QtGui.QHeaderView.Fixed)
        vh.setDefaultSectionSize(altoFila)
        vh.setVisible(False)
