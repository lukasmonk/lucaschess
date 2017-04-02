"""
formlayout adapted to Lucas Chess
=================================

Original formlayout License Agreement (MIT License)
---------------------------------------------------

Copyright (c) 2009 Pierre Raybaut

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
"""
__license__ = __doc__

import os

from PyQt4 import QtCore, QtGui

from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code import Util

separador = (None, None)
SPINBOX, COMBOBOX, COLORBOX, DIAL, EDITBOX, FICHERO, FONTCOMBOBOX, CHSPINBOX = range(8)


class Spinbox:
    def __init__(self, label, minimo, maximo, ancho):
        self.tipo = SPINBOX
        self.label = label + ":"
        self.minimo = minimo
        self.maximo = maximo
        self.ancho = ancho


class CHSpinbox:
    def __init__(self, label, minimo, maximo, ancho, chlabel):
        self.tipo = CHSPINBOX
        self.label = label + ":"
        self.chlabel = chlabel
        self.minimo = minimo
        self.maximo = maximo
        self.ancho = ancho


class Combobox:
    def __init__(self, label, lista, si_editable=False, tooltip=None):
        self.tipo = COMBOBOX
        self.lista = lista  # (clave,titulo),....
        self.label = label + ":"
        self.si_editable = si_editable
        self.tooltip = tooltip


class FontCombobox:
    def __init__(self, label):
        self.tipo = FONTCOMBOBOX
        self.label = label + ":"


class Colorbox:
    def __init__(self, label, ancho, alto, siChecked=False, siSTR=False):
        self.tipo = COLORBOX
        self.ancho = ancho
        self.alto = alto
        self.siChecked = siChecked
        self.siSTR = siSTR
        self.label = label + ":"


class Editbox:
    def __init__(self, label, ancho=None, rx=None, tipo=str, siPassword=False, alto=1, decimales=1):
        self.tipo = EDITBOX
        self.label = label + ":"
        self.ancho = ancho
        self.rx = rx
        self.tipoCampo = tipo
        self.siPassword = siPassword
        self.alto = alto
        self.decimales = decimales


class Casillabox(Editbox):
    def __init__(self, label):
        Editbox.__init__(self, label, ancho=24, rx="[a-h][1-8]")


class Dial:
    def __init__(self, label, minimo, maximo, siporc=True):
        self.tipo = DIAL
        self.minimo = minimo
        self.maximo = maximo
        self.siporc = siporc
        self.label = "\n" + label + ":"


class Fichero:
    def __init__(self, label, extension, siSave, siRelativo=True, anchoMinimo=None, ficheroDefecto="", liHistorico=None):
        self.tipo = FICHERO
        self.extension = extension  # si extension puede tener un elemento como pgn o -> "Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"
        self.siSave = siSave
        self.label = label + ":"
        self.siRelativo = siRelativo
        self.anchoMinimo = anchoMinimo
        self.ficheroDefecto = ficheroDefecto
        self.liHistorico = liHistorico


class BotonFichero(QtGui.QPushButton):
    def __init__(self, fichero, extension, siSave, siRelativo, anchoMinimo, ficheroDefecto):
        QtGui.QPushButton.__init__(self)
        self.connect(self, QtCore.SIGNAL("clicked()"), self.cambiaFichero)
        self.fichero = fichero
        self.extension = extension
        self.siSave = siSave
        self.siRelativo = siRelativo
        self.anchoMinimo = anchoMinimo
        if anchoMinimo:
            self.setMinimumWidth(anchoMinimo)
        self.qm = QtGui.QFontMetrics(self.font())
        self.fichero = fichero
        self.ficheroDefecto = ficheroDefecto
        self.siPrimeraVez = True

    def cambiaFichero(self):
        titulo = _("File to save") if self.siSave else _("File to read")
        fbusca = self.fichero if self.fichero else self.ficheroDefecto
        filtro = self.extension if "(" in self.extension else (
            _("File") + " %s (*.%s)" % (self.extension, self.extension))
        if self.siSave:
            resp = QTUtil2.salvaFichero(self, titulo, fbusca, filtro)
        else:
            resp = QTUtil2.leeFichero(self, fbusca, self.extension, titulo=titulo)
        if resp:
            self.ponFichero(resp)

    def ponFichero(self, txt):
        self.fichero = txt
        if txt:
            txt = os.path.normpath(txt)
            if self.siRelativo:
                txt = Util.dirRelativo(txt)
            tamTxt = self.qm.boundingRect(txt).width()
            tmax = self.width() - 10
            if self.siPrimeraVez:
                self.siPrimeraVez = False
                tmax = self.anchoMinimo if self.anchoMinimo else tmax

            while tamTxt > tmax:
                txt = txt[1:]
                tamTxt = self.qm.boundingRect(txt).width()

        self.setText(txt)


class LBotonFichero(QtGui.QHBoxLayout):
    def __init__(self, parent, config, fichero):
        QtGui.QHBoxLayout.__init__(self)

        if config.liHistorico and not config.ficheroDefecto:
            config.ficheroDefecto = os.path.dirname(config.liHistorico[0])

        self.boton = BotonFichero(fichero, config.extension, config.siSave, config.siRelativo, config.anchoMinimo,
                                  config.ficheroDefecto)
        btCancelar = Controles.PB(parent, "", self.cancelar)
        btCancelar.ponIcono(Iconos.Delete()).anchoFijo(16)
        self.parent = parent

        self.addWidget(self.boton)
        self.addWidget(btCancelar)

        if config.liHistorico:
            btHistorico = Controles.PB(parent, "", self.historico).ponIcono(Iconos.Favoritos())
            self.addWidget(btHistorico)
            self.liHistorico = config.liHistorico
        self.boton.ponFichero(fichero)

    def historico(self):
        if self.liHistorico:
            menu = Controles.Menu(self.parent, puntos=8)
            menu.setToolTip(_("To choose: <b>left button</b> <br>To erase: <b>right button</b>"))
            for fichero in self.liHistorico:
                menu.opcion(fichero, fichero, Iconos.PuntoAzul())

            resp = menu.lanza()
            if resp:
                if menu.siIzq:
                    self.boton.ponFichero(resp)
                elif menu.siDer:
                    if QTUtil2.pregunta(self.parent, _("Do you want to remove file %s from the list?") % resp):
                        del self.liHistorico[self.liHistorico.index(resp)]

    def cancelar(self):
        self.boton.ponFichero("")


class BotonColor(QtGui.QPushButton):
    def __init__(self, parent, ancho, alto, siSTR, dispatch):
        QtGui.QPushButton.__init__(self, parent)

        self.setFixedSize(ancho, alto)

        self.connect(self, QtCore.SIGNAL("clicked()"), self.pulsado)

        self.xcolor = "" if siSTR else -1

        self.siSTR = siSTR

        self.dispatch = dispatch

    def ponColor(self, xcolor):

        self.xcolor = xcolor

        if self.siSTR:
            color = QtGui.QColor(xcolor)
        else:
            color = QtGui.QColor()
            color.setRgba(xcolor)
        self.setStyleSheet("QWidget { background-color: %s }" % color.name())

    def pulsado(self):
        if self.siSTR:
            color = QtGui.QColor(self.xcolor)
        else:
            color = QtGui.QColor()
            color.setRgba(self.xcolor)
        color = QtGui.QColorDialog.getColor(color, self.parentWidget(), _("Color"), QtGui.QColorDialog.ShowAlphaChannel|QtGui.QColorDialog.DontUseNativeDialog)
        if color.isValid():
            if self.siSTR:
                self.ponColor(color.name())
            else:
                self.ponColor(color.rgba())
        if self.dispatch:
            self.dispatch()

    def value(self):
        return self.xcolor


class BotonCheckColor(QtGui.QHBoxLayout):
    def __init__(self, parent, ancho, alto, dispatch):
        QtGui.QHBoxLayout.__init__(self)

        self.boton = BotonColor(parent, ancho, alto, False, dispatch)
        self.checkbox = QtGui.QCheckBox(parent)
        self.checkbox.setFixedSize(20, 20)

        self.connect(self.checkbox, QtCore.SIGNAL("clicked()"), self.pulsado)

        self.dispatch = dispatch

        self.addWidget(self.checkbox)
        self.addWidget(self.boton)

    def ponColor(self, ncolor):
        if ncolor == -1:
            self.boton.hide()
            self.checkbox.setChecked(False)
        else:
            self.boton.show()
            self.checkbox.setChecked(True)
            self.boton.ponColor(ncolor)

    def value(self):
        if self.checkbox.isChecked():
            return self.boton.xcolor
        else:
            return -1

    def pulsado(self):
        if self.checkbox.isChecked():
            if self.boton.xcolor == -1:
                self.boton.ponColor(0)
                self.boton.pulsado()
            else:
                self.boton.ponColor(self.boton.xcolor)
            self.boton.show()
        else:
            self.boton.hide()
        if self.dispatch:
            self.dispatch()


class Edit(Controles.ED):
    def __init__(self, parent, config, dispatch):
        Controles.ED.__init__(self, parent)
        if dispatch:
            self.textChanged.connect(dispatch)

        if config.rx:
            self.controlrx(config.rx)
        if config.ancho:
            self.anchoFijo(config.ancho)
        if config.siPassword:
            self.setEchoMode(QtGui.QLineEdit.Password)
        self.tipo = config.tipoCampo
        self.decimales = config.decimales

    def valor(self):
        if self.tipo == int:
            v = self.textoInt()
        elif self.tipo == float:
            v = self.textoFloat()
        else:
            v = self.texto()
        return v


class TextEdit(Controles.EM):
    def __init__(self, parent, config, dispatch):
        Controles.EM.__init__(self, parent)
        if dispatch:
            self.textChanged.connect(dispatch)

        self.altoMinimo(config.alto)

    def valor(self):
        return self.texto()


class DialNum(QtGui.QHBoxLayout):
    def __init__(self, parent, config, dispatch):
        QtGui.QHBoxLayout.__init__(self)

        self.dial = QtGui.QDial(parent)
        self.dial.setMinimum(config.minimo)
        self.dial.setMaximum(config.maximo)
        self.dial.setNotchesVisible(True)
        self.dial.setFixedSize(40, 40)
        self.lb = QtGui.QLabel(parent)

        self.dispatch = dispatch

        self.siporc = config.siporc

        self.connect(self.dial, QtCore.SIGNAL("valueChanged (int)"), self.movido)

        self.addWidget(self.dial)
        self.addWidget(self.lb)

    def ponValor(self, nvalor):
        self.dial.setValue(nvalor)
        self.ponLB()

    def ponLB(self):
        txt = "%d" % self.dial.value()
        if self.siporc:
            txt += "%"
        self.lb.setText(txt)

    def movido(self, valor):
        self.ponLB()
        if self.dispatch:
            self.dispatch()

    def value(self):
        return self.dial.value()


class FormWidget(QtGui.QWidget):
    def __init__(self, data, comment="", parent=None, dispatch=None):
        super(FormWidget, self).__init__(parent)
        from copy import deepcopy

        self.data = deepcopy(data)
        self.widgets = []
        self.labels = []
        self.formlayout = QtGui.QFormLayout(self)
        if comment:
            self.formlayout.addRow(QtGui.QLabel(comment, self))
            self.formlayout.addRow(QtGui.QLabel(" ", self))

        self.setup(dispatch)

    def setup(self, dispatch):
        for label, value in self.data:

            # Separador
            if label is None and value is None:
                self.formlayout.addRow(QtGui.QLabel(" ", self), QtGui.QLabel(" ", self))
                self.widgets.append(None)
                self.labels.append(None)

            # Comentario
            elif label is None:
                lb = Controles.LB(self, QTUtil2.resalta(value, 3))
                self.formlayout.addRow(lb)
                self.widgets.append(None)
                self.labels.append(None)

            else:
                # Otros tipos
                if not isinstance(label, (str, unicode)):
                    config = label
                    tipo = config.tipo
                    if tipo == SPINBOX:
                        field = QtGui.QSpinBox(self)
                        field.setMinimum(config.minimo)
                        field.setMaximum(config.maximo)
                        field.setValue(value)
                        field.setFixedWidth(config.ancho)
                        if dispatch:
                            field.valueChanged.connect(dispatch)
                    elif tipo == COMBOBOX:
                        field = Controles.CB(self, config.lista, value)
                        if config.si_editable:
                            field.setEditable(True)
                        if config.tooltip:
                            field.setToolTip(config.tooltip)

                        field.lista = config.lista
                        if dispatch:
                            field.currentIndexChanged.connect(dispatch)
                            # field = QtGui.QComboBox(self)
                            # for n, tp in enumerate(config.lista):
                            # if len(tp) == 3:
                            # field.addItem( tp[2], tp[0], tp[1] )
                            # else:
                            # field.addItem( tp[0], tp[1] )
                            # if tp[1] == value:
                            # field.setCurrentIndex( n )
                            # if dispatch:
                            # field.currentIndexChanged.connect( dispatch )
                    elif tipo == FONTCOMBOBOX:
                        field = QtGui.QFontComboBox(self)
                        if value:
                            font = Controles.TipoLetra(value)
                            field.setCurrentFont(font)
                    elif tipo == COLORBOX:
                        if config.siChecked:
                            field = BotonCheckColor(self, config.ancho, config.alto, dispatch)
                        else:
                            field = BotonColor(self, config.ancho, config.alto, config.siSTR, dispatch)
                        field.ponColor(value)

                    elif tipo == DIAL:
                        field = DialNum(self, config, dispatch)
                        field.ponValor(value)

                    elif tipo == EDITBOX:
                        if config.alto == 1:
                            field = Edit(self, config, dispatch)
                            tp = config.tipoCampo
                            if tp == str:
                                field.ponTexto(value)
                            elif tp == int:
                                field.tipoInt()
                                field.ponInt(value)
                            elif tp == float:
                                field.tipoFloat(0.0)
                                field.ponFloat(value)
                        else:
                            field = TextEdit(self, config, dispatch)
                            field.ponTexto(value)

                    elif tipo == FICHERO:
                        field = LBotonFichero(self, config, value)

                    label = config.label

                # Fichero
                elif isinstance(value, dict):
                    fichero = value["FICHERO"]
                    extension = value.get("EXTENSION", "pgn")
                    siSave = value.get("SISAVE", True)
                    siRelativo = value.get("SIRELATIVO", True)
                    field = BotonFichero(fichero, extension, siSave, siRelativo, 250, fichero)
                # Texto
                elif isinstance(value, (str, unicode)):
                    field = QtGui.QLineEdit(value, self)

                # Combo
                elif isinstance(value, (list, tuple)):
                    selindex = value.pop(0)
                    field = QtGui.QComboBox(self)
                    if isinstance(value[0], (list, tuple)):
                        keys = [key for key, _val in value]
                        value = [val for _key, val in value]
                    else:
                        keys = value
                    field.addItems(value)
                    if selindex in value:
                        selindex = value.index(selindex)
                    elif selindex in keys:
                        selindex = keys.index(selindex)
                    else:
                        selindex = 0
                    field.setCurrentIndex(selindex)

                # Checkbox
                elif isinstance(value, bool):
                    field = QtGui.QCheckBox(self)
                    field.setCheckState(QtCore.Qt.Checked if value else QtCore.Qt.Unchecked)
                    if dispatch:
                        field.stateChanged.connect(dispatch)

                # Float segundos
                elif isinstance(value, float):  # Para los segundos
                    v = "%0.1f" % value
                    field = QtGui.QLineEdit(v, self)
                    field.setValidator(QtGui.QDoubleValidator(0.0, 36000.0, 1, field))  # Para los segundos
                    field.setAlignment(QtCore.Qt.AlignRight)
                    field.setFixedWidth(40)

                # Numero
                elif isinstance(value, int):
                    field = QtGui.QSpinBox(self)
                    field.setMaximum(9999)
                    field.setValue(value)
                    field.setFixedWidth(80)

                # Linea
                else:
                    field = QtGui.QLineEdit(repr(value), self)

                self.formlayout.addRow(label, field)
                self.formlayout.setLabelAlignment(QtCore.Qt.AlignRight)
                self.widgets.append(field)
                self.labels.append(label)

    def get(self):
        valuelist = []
        for index, (label, value) in enumerate(self.data):
            field = self.widgets[index]
            if label is None:
                # Separator / Comment
                continue
            elif not isinstance(label, (str, unicode)):
                config = label
                tipo = config.tipo
                if tipo == SPINBOX:
                    value = int(field.value())
                elif tipo == COMBOBOX:
                    n = field.currentIndex()
                    value = field.lista[n][1]
                elif tipo == FONTCOMBOBOX:
                    value = field.currentFont().family()
                elif tipo == COLORBOX:
                    value = field.value()
                elif tipo == DIAL:
                    value = field.value()
                elif tipo == EDITBOX:
                    value = field.valor()
                elif tipo == FICHERO:
                    value = field.boton.fichero

            elif isinstance(value, (str, unicode)):
                value = unicode(field.text())
            elif isinstance(value, dict):
                value = field.fichero
            elif isinstance(value, (list, tuple)):
                index = int(field.currentIndex())
                if isinstance(value[0], (list, tuple)):
                    value = value[index][0]
                else:
                    value = value[index]
            elif isinstance(value, bool):
                value = field.checkState() == QtCore.Qt.Checked
            elif isinstance(value, float):
                value = float(field.text())
            elif isinstance(value, int):
                value = int(field.value())
            else:
                value = field.text()
            valuelist.append(value)
        return valuelist

    def getWidget(self, numero):
        n = -1
        for index, (label, value) in enumerate(self.data):
            field = self.widgets[index]
            if field is not None:
                n += 1
                if n == numero:
                    return field
        return None


class FormComboWidget(QtGui.QWidget):
    def __init__(self, datalist, comment="", parent=None):
        super(FormComboWidget, self).__init__(parent)
        layout = Colocacion.V()
        self.setLayout(layout)
        self.combobox = QtGui.QComboBox(self)
        layout.control(self.combobox)

        self.stackwidget = QtGui.QStackWidget(self)
        layout.control(self.stackwidget)
        self.connect(self.combobox, QtCore.SIGNAL("currentIndexChanged(int)"),
                     self.stackwidget, QtCore.SLOT("setCurrentIndex(int)"))

        self.widgetlist = []
        for data, title, comment in datalist:
            self.combobox.addItem(title)
            widget = FormWidget(data, comment=comment, parent=self)
            self.stackwidget.addWidget(widget)
            self.widgetlist.append(widget)

    def get(self):
        return [widget.get() for widget in self.widgetlist]


class FormTabWidget(QtGui.QWidget):
    def __init__(self, datalist, comment="", parent=None, dispatch=None):
        super(FormTabWidget, self).__init__(parent)
        layout = Colocacion.V()
        self.tabwidget = QtGui.QTabWidget()
        layout.control(self.tabwidget)
        self.setLayout(layout)
        self.widgetlist = []
        for data, title, comment in datalist:
            if len(data[0]) == 3:
                widget = FormComboWidget(data, comment=comment, parent=self)
            else:
                widget = FormWidget(data, comment=comment, parent=self, dispatch=dispatch)
            index = self.tabwidget.addTab(widget, title)
            self.tabwidget.setTabToolTip(index, comment)
            self.widgetlist.append(widget)

    def get(self):
        return [widget.get() for widget in self.widgetlist]

    def getWidget(self, numTab, numero):
        return self.widgetlist[numTab].getWidget(numero)


class FormDialog(QtGui.QDialog):
    """Form Dialog"""

    def __init__(self, data, title="", comment="",
                 icon=None, parent=None, siDefecto=True, dispatch=None):
        super(FormDialog, self).__init__(parent, QtCore.Qt.Dialog)

        # Form
        if isinstance(data[0][0], (list, tuple)):
            self.formwidget = FormTabWidget(data, comment=comment, parent=self, dispatch=dispatch)
            if dispatch:
                dispatch(self.formwidget)
        elif len(data[0]) == 3:
            self.formwidget = FormComboWidget(data, comment=comment, parent=self)
        else:
            self.formwidget = FormWidget(data, comment=comment, parent=self, dispatch=dispatch)
            if dispatch:
                dispatch(self.formwidget)  # enviamos el form de donde tomar datos cuando hay cambios

        tb = QTUtil2.tbAcceptCancel(self, siDefecto, siReject=False)

        layout = Colocacion.V()
        layout.control(tb)
        layout.control(self.formwidget)
        layout.margen(3)

        self.setLayout(layout)

        self.setWindowTitle(title)
        if not isinstance(icon, QtGui.QIcon):
            icon = QtGui.QWidget().style().standardIcon(QtGui.QStyle.SP_MessageBoxQuestion)
        self.setWindowIcon(icon)

    def aceptar(self):
        self.accion = "aceptar"
        self.data = self.formwidget.get()
        self.accept()

    def defecto(self):
        self.accion = "defecto"
        if not QTUtil2.pregunta(self, _("Are you sure you want to set the default configuration?")):
            return
        self.data = self.formwidget.get()
        self.accept()

    def cancelar(self):
        self.data = None
        self.reject()

    def get(self):
        """Return form result"""
        return self.accion, self.data


def fedit(data, title="", comment="", icon=None, parent=None, siDefecto=False, anchoMinimo=None, dispatch=None):
    """
    Create form dialog and return result
    (if Cancel button is pressed, return None)

    data: datalist, datagroup

    datalist: list/tuple of (field_name, field_value)
    datagroup: list/tuple of (datalist *or* datagroup, title, comment)

    -> one field for each member of a datalist
    -> one tab for each member of a top-level datagroup
    -> one page (of a multipage widget, each page can be selected with a combo
       box) for each member of a datagroup inside a datagroup

    Supported types for field_value:
      - int, float, str, unicode, bool
      - colors: in Qt-compatible text form, i.e. in hex format or name (red,...)
                (automatically detected from a string)
      - list/tuple:
          * the first element will be the selected index (or value)
          * the other elements can be couples (key, value) or only values
    """
    dialog = FormDialog(data, title, comment, icon, parent, siDefecto, dispatch)
    if anchoMinimo:
        dialog.setMinimumWidth(anchoMinimo)
    if dialog.exec_():
        QtCore.QCoreApplication.processEvents()
        QtGui.QApplication.processEvents()
        return dialog.get()
