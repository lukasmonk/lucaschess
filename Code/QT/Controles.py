# import datetime

from PyQt4 import QtCore, QtGui


class ED(QtGui.QLineEdit):
    """
    Control de entrada de texto en una linea.
    """

    def __init__(self, parent, texto=None):
        """
        @param parent: ventana propietaria.
        @param texto: texto inicial.
        """
        if texto:
            QtGui.QLineEdit.__init__(self, texto, parent)
        else:
            QtGui.QLineEdit.__init__(self, parent)
        self.parent = parent

        self.siMayusculas = False
        self.decimales = 1

        self.menu = None

    def soloLectura(self, sino):
        self.setReadOnly(sino)
        return self

    def password(self):
        self.setEchoMode(QtGui.QLineEdit.Password)
        return self

    def deshabilitado(self, sino):
        self.setDisabled(sino)
        return self

    def capturaIntro(self, rutina):
        self.connect(self, QtCore.SIGNAL("returnPressed()"), rutina)
        return self

    def capturaCambiado(self, rutina):
        self.connect(self, QtCore.SIGNAL("textEdited(QString)"), rutina)
        return self

    def ponTexto(self, texto):
        self.setText(texto)

    def texto(self):
        txt = self.text()
        return txt

    def alinCentrado(self):
        self.setAlignment(QtCore.Qt.AlignHCenter)
        return self

    def alinDerecha(self):
        self.setAlignment(QtCore.Qt.AlignRight)
        return self

    def anchoMinimo(self, px):
        self.setMinimumWidth(px)
        return self

    def anchoMaximo(self, px):
        self.setMaximumWidth(px)
        return self

    def caracteres(self, num):
        self.setMaxLength(num)
        self.numCaracteres = num
        return self

    def anchoFijo(self, px):
        self.setFixedWidth(px)
        return self

    def controlrx(self, regexpr):
        rx = QtCore.QRegExp(regexpr)
        validator = QtGui.QRegExpValidator(rx, self)
        self.setValidator(validator)
        return self

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def tipoFloat(self, valor=0.0, desde=0.0, hasta=36000.0, decimales=None):
        """
        Valida los caracteres suponiendo que es un tipo decimal con unas condiciones
        @param valor: valor inicial
        @param desde: valor minimo
        @param hasta: valor maximo
        @param decimales: num. decimales
        """
        if desde is None:
            self.setValidator(QtGui.QDoubleValidator(self))
        else:
            if decimales is None:
                decimales = self.decimales
            else:
                self.decimales = decimales
            self.setValidator(QtGui.QDoubleValidator(desde, hasta, decimales, self))
        self.setAlignment(QtCore.Qt.AlignRight)
        self.ponFloat(valor)
        return self

    def ponFloat(self, valor):
        fm = "%0." + str(self.decimales) + "f"
        self.ponTexto(fm % valor)
        return self

    def textoFloat(self):
        txt = self.text()
        return round(float(txt), self.decimales) if txt else 0.0

    def tipoInt(self, valor=0):
        """
        Valida los caracteres suponiendo que es un tipo entero con unas condiciones
        @param valor: valor inicial
        """
        self.setValidator(QtGui.QIntValidator(self))
        self.setAlignment(QtCore.Qt.AlignRight)
        self.ponInt(valor)
        return self

    def ponInt(self, valor):
        self.ponTexto(str(valor))
        return self

    def textoInt(self):
        txt = self.text()
        return int(txt) if txt else 0


class SB(QtGui.QSpinBox):
    """
    SpinBox: Entrada de numeros enteros, con control de incremento o reduccion
    """

    def __init__(self, parent, valor, desde, hasta):
        """
        @param valor: valor inicial
        @param desde: limite inferior
        @param hasta: limite superior
        """
        QtGui.QSpinBox.__init__(self, parent)
        self.setRange(desde, hasta)
        self.setSingleStep(1)
        self.setValue(int(valor))

    def tamMaximo(self, px):
        self.setFixedWidth(px)
        return self

    def valor(self):
        return self.value()

    def ponValor(self, valor):
        self.setValue(int(valor))

    def capturaCambiado(self, rutina):
        self.connect(self, QtCore.SIGNAL("valueChanged(int)"), rutina)
        return self


class CB(QtGui.QComboBox):
    """
    ComboBox : entrada de una lista de opciones = etiqueta,clave[,icono]
    """

    def __init__(self, parent, liOpciones, valorInicial):
        """
        @param liOpciones: lista de (etiqueta,clave)
        @param valorInicial: valor inicial
        """
        QtGui.QComboBox.__init__(self, parent)
        self.rehacer(liOpciones, valorInicial)

    def valor(self):
        return self.itemData(self.currentIndex())

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def rehacer(self, liOpciones, valorInicial):
        self.liOpciones = liOpciones
        self.clear()
        nindex = 0
        for n, opcion in enumerate(liOpciones):
            if len(opcion) == 2:
                etiqueta, clave = opcion
                self.addItem(etiqueta, clave)
            else:
                etiqueta, clave, icono = opcion
                self.addItem(icono, etiqueta, clave)
            if clave == valorInicial:
                nindex = n
        self.setCurrentIndex(nindex)

    def ponValor(self, valor):
        for n, opcion in enumerate(self.liOpciones):
            clave = opcion[1]
            if clave == valor:
                self.setCurrentIndex(n)
                break

    def ponAncho(self, px):
        r = self.geometry()
        r.setWidth(px)
        self.setGeometry(r)
        return self

    def ponAnchoFijo(self, px):
        self.setFixedWidth(px)
        return self

    def ponAnchoMinimo(self):
        self.setSizeAdjustPolicy(self.AdjustToMinimumContentsLengthWithIcon)
        return self

    def capturaCambiado(self, rutina):
        self.connect(self, QtCore.SIGNAL("currentIndexChanged(int)"), rutina)
        return self


class CHB(QtGui.QCheckBox):
    """
    CheckBox : entrada de una campo seleccionable
    """

    def __init__(self, parent, etiqueta, valorInicial):
        """
        @param etiqueta: rotulo mostrado
        @param valorInicial: valor inicial : True/False
        """
        QtGui.QCheckBox.__init__(self, etiqueta, parent)
        self.setChecked(valorInicial)

    def ponValor(self, si):
        self.setChecked(si)
        return self

    def valor(self):
        return self.isChecked()

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def capturaCambiado(self, owner, rutina):
        owner.connect(self, QtCore.SIGNAL("clicked()"), rutina)
        return self

    def anchoFijo(self, px):
        self.setFixedWidth(px)
        return self


class LB(QtGui.QLabel):
    """
    Etiquetas de texto.
    """

    def __init__(self, parent, texto=None):
        """
        @param texto: texto inicial.
        """
        if texto:
            QtGui.QLabel.__init__(self, texto, parent)
        else:
            QtGui.QLabel.__init__(self, parent)

        self.setOpenExternalLinks(True)
        self.setTextInteractionFlags(QtCore.Qt.TextBrowserInteraction|QtCore.Qt.TextSelectableByMouse)

    def ponTexto(self, texto):
        self.setText(texto)

    def texto(self):
        return self.text()

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def alinCentrado(self):
        self.setAlignment(QtCore.Qt.AlignCenter)
        return self

    def alinDerecha(self):
        self.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        return self

    def anchoMaximo(self, px):
        self.setMaximumWidth(px)
        return self

    def anchoFijo(self, px):
        self.setFixedWidth(px)
        return self

    def anchoMinimo(self, px):
        self.setMinimumWidth(px)
        return self

    def altoMinimo(self, px):
        self.setMinimumHeight(px)
        return self

    def altoFijo(self, px):
        self.setFixedHeight(px)
        return self

    def ponAlto(self, px):
        rec = self.geometry()
        rec.setHeight(px)
        self.setGeometry(rec)
        return self

    def alineaY(self, otroLB):
        rec = self.geometry()
        rec.setY(otroLB.geometry().y())
        self.setGeometry(rec)
        return self

    def ponImagen(self, pm):
        self.setPixmap(pm)
        return self

    def ponFondo(self, color):
        return self.ponFondoN(color.name())

    def ponFondoN(self, color):
        self.setStyleSheet("QWidget { background-color: %s }" % color)
        return self

    def ponColor(self, color):
        return self.ponColorN(color.name())

    def ponColorN(self, color):
        self.setStyleSheet("QWidget { color: %s }" % color)
        return self

    def ponColorFondoN(self, color, fondo):
        self.setStyleSheet("QWidget { color: %s; background-color: %s}" % (color, fondo))
        return self

    def ponWrap(self):
        self.setWordWrap(True)
        return self

    def ponAncho(self, px):
        r = self.geometry()
        r.setWidth(px)
        self.setGeometry(r)
        return self


def LB2P(parent, texto):
    return LB(parent, texto + ": ")


class PB(QtGui.QPushButton):
    """
    Boton.
    """

    def __init__(self, parent, texto, rutina=None, plano=True):
        """
        @param parent: ventana propietaria, necesario para conectar una rutina.
        @param texto: etiqueta inicial.
        @param rutina: rutina a la que se conecta el boton.
        """
        QtGui.QPushButton.__init__(self, texto, parent)
        self.wParent = parent
        self.setFlat(plano)
        if rutina:
            self.conectar(rutina)

    def ponIcono(self, icono, tamIcon=16):
        self.setIcon(icono)
        self.setIconSize(QtCore.QSize(tamIcon, tamIcon))
        return self

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def anchoFijo(self, px):
        self.setFixedWidth(px)
        return self

    def altoFijo(self, px):
        self.setFixedHeight(px)
        return self

    def cuadrado(self, px):
        self.setFixedSize(px, px)
        return self

    def anchoMinimo(self, px):
        self.setMinimumWidth(px)
        return self

    def conectar(self, rutina):
        self.wParent.connect(self, QtCore.SIGNAL("clicked()"), rutina)
        return self

    def ponFondo(self, txtFondo):
        self.setStyleSheet("QWidget { background: %s }" % txtFondo)
        return self

    def ponFondoN(self, ncolor):
        self.setStyleSheet("QWidget { background-color: %d }" % ncolor)
        return self

    def ponPlano(self, siPlano):
        self.setFlat(siPlano)
        return self

    def ponToolTip(self, txt):
        self.setToolTip(txt)
        return self

    def ponTexto(self, txt):
        self.setText(txt)


class RB(QtGui.QRadioButton):
    """
    RadioButton: lista de alternativas
    """

    def __init__(self, wParent, texto, rutina=None):
        QtGui.QRadioButton.__init__(self, texto, wParent)
        if rutina:
            wParent.connect(self, QtCore.SIGNAL("clicked()"), rutina)

    def activa(self, siActivar=True):
        self.setChecked(siActivar)
        return self


class GB(QtGui.QGroupBox):
    """
    GroupBox: Recuadro para agrupamiento de controles
    """

    def __init__(self, wParent, texto, layout):
        QtGui.QGroupBox.__init__(self, texto, wParent)
        self.setLayout(layout)
        self.wParent = wParent

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def alinCentrado(self):
        self.setAlignment(QtCore.Qt.AlignHCenter)
        return self

    def conectar(self, rutina):
        self.setCheckable(True)
        self.setChecked(False)
        self.wParent.connect(self, QtCore.SIGNAL("clicked()"), rutina)
        return self

    def ponTexto(self, texto):
        self.setTitle(texto)
        return self


class EM(QtGui.QTextEdit):
    """
    Control de entrada de texto en varias lineas.
    """

    def __init__(self, parent, texto=None, siHTML=True):
        """
        @param texto: texto inicial.
        """
        QtGui.QTextEdit.__init__(self, parent)
        self.parent = parent

        self.menu = None  # menu de contexto
        self.rutinaDobleClick = None

        self.setAcceptRichText(siHTML)

        if texto:
            if siHTML:
                self.setText(texto)
            else:
                self.insertPlainText(texto)

    def ponHtml(self, texto):
        self.setHtml(texto)
        return self

    def insertarHtml(self, texto):
        self.insertHtml(texto)
        return self

    def insertarTexto(self, texto):
        self.insertPlainText(texto)
        return self

    def soloLectura(self):
        self.setReadOnly(True)
        return self

    def texto(self):
        return self.toPlainText()

    def ponTexto(self, txt):
        self.setText("")
        self.insertarTexto(txt)

    def html(self):
        return self.toHtml()

    def ponAncho(self, px):
        r = self.geometry()
        r.setWidth(px)
        self.setGeometry(r)
        return self

    def anchoMinimo(self, px):
        self.setMinimumWidth(px)
        return self

    def altoMinimo(self, px):
        self.setMinimumHeight(px)
        return self

    def altoFijo(self, px):
        self.setFixedHeight(px)
        return self

    def anchoFijo(self, px):
        self.setFixedWidth(px)
        return self

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def ponWrap(self, siPoner):
        self.setWordWrapMode(QtGui.QTextOption.WordWrap if siPoner else QtGui.QTextOption.NoWrap)
        return self

    def capturaCambios(self, rutina):
        self.parent.connect(self, QtCore.SIGNAL("textChanged()"), rutina)
        return self

    def capturaDobleClick(self, rutina):
        self.rutinaDobleClick = rutina
        return self

    def mouseDoubleClickEvent(self, event):
        if self.rutinaDobleClick:
            self.rutinaDobleClick(event)

    def posicion(self):
        return self.textCursor().position()


class Menu(QtGui.QMenu):
    """
    Menu popup.

    Ejemplo::

        menu = Controles.Menu(window)

        menu.opcion( "op1", "Primera opcion", icono )
        menu.separador()
        menu.opcion( "op2", "Segunda opcion", icono1 )
        menu.separador()

        menu1 = menu.submenu( "Submenu", icono2 )
        menu1.opcion( "op3_1", "opcion 1", icono3 )
        menu1.separador()
        menu1.opcion( "op3_2", "opcion 2", icono3 )
        menu1.separador()

        resp = menu.lanza()

        if resp:
            if resp == "op1":
                ..........

            elif resp == "op2":
                ................
    """

    def __init__(self, parent, titulo=None, icono=None, siDeshabilitado=False, puntos=None, siBold=True):

        QtGui.QMenu.__init__(self, parent)

        if titulo:
            self.setTitle(titulo)
        if icono:
            self.setIcon(icono)
        if siDeshabilitado:
            self.setDisabled(True)

        if puntos:
            tl = TipoLetra(puntos=puntos, peso=75) if siBold else TipoLetra(puntos=puntos)
            self.setFont(tl)

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def opcion(self, clave, rotulo, icono=None, siDeshabilitado=False, tipoLetra=None, siChecked=None):
        if icono:
            accion = QtGui.QAction(icono, rotulo, self)
        else:
            accion = QtGui.QAction(rotulo, self)
        accion.clave = clave
        if siDeshabilitado:
            accion.setDisabled(True)
        if tipoLetra:
            accion.setFont(tipoLetra)
        if siChecked is not None:
            accion.setCheckable(True)
            accion.setChecked(siChecked)

        self.addAction(accion)
        return accion

    def submenu(self, rotulo, icono=None, siDeshabilitado=False):
        menu = Menu(self, rotulo, icono, siDeshabilitado)
        menu.setFont(self.font())
        self.addMenu(menu)
        return menu

    def mousePressEvent(self, event):
        self.siIzq = event.button() == QtCore.Qt.LeftButton
        self.siDer = event.button() == QtCore.Qt.RightButton
        resp = QtGui.QMenu.mousePressEvent(self, event)
        return resp

    def separador(self):
        self.addSeparator()

    def lanza(self):
        QtCore.QCoreApplication.processEvents()
        QtGui.QApplication.processEvents()
        resp = self.exec_(QtGui.QCursor.pos())
        import gc
        gc.collect()
        if resp:
            return resp.clave
        else:
            return None


class TB(QtGui.QToolBar):
    """
    Crea una barra de tareas simple.

    @param liAcciones: lista de acciones, en forma de tupla = titulo, icono, clave
    @param siTexto: si muestra texto
    @param tamIcon: tama_o del icono
    @param rutina: rutina que se llama al pulsar una opcion. Por defecto sera parent.procesarTB().
        Y la clave enviada se obtiene de self.sender().clave
    """

    def __init__(self, parent, liAcciones, siTexto=True, tamIcon=32, rutina=None, puntos=None, background=None):

        QtGui.QToolBar.__init__(self, "BASICO", parent)

        self.setIconSize(QtCore.QSize(tamIcon, tamIcon))

        self.parent = parent

        if rutina is None:
            rutina = parent.procesarTB

        self.rutina = rutina

        if siTexto:
            self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)

        self.f = TipoLetra(puntos=puntos) if puntos else None

        if background:
            self.setStyleSheet("QWidget { background: %s }" % background)

        self.ponAcciones(liAcciones)

    def ponAcciones(self, liAcciones):
        self.dicTB = {}
        lista = []
        for datos in liAcciones:
            if datos:
                if type(datos) == int:
                    self.addWidget(LB("").anchoFijo(datos))
                else:
                    titulo, icono, clave = datos
                    accion = QtGui.QAction(titulo, self.parent)
                    accion.setIcon(icono)
                    accion.setIconText(titulo)
                    self.parent.connect(accion, QtCore.SIGNAL("triggered()"), self.rutina)
                    accion.clave = clave
                    if self.f:
                        accion.setFont(self.f)
                    lista.append(accion)
                    self.addAction(accion)
                    self.dicTB[clave] = accion
            else:
                self.addSeparator()
        self.liAcciones = lista

    def reset(self, liAcciones):
        self.clear()
        self.ponAcciones(liAcciones)
        self.update()

    def vertical(self):
        self.setOrientation(QtCore.Qt.Vertical)
        return self

    def setAccionVisible(self, key, value):
        for accion in self.liAcciones:
            if accion.clave == key:
                accion.setVisible(value)


class TBrutina(QtGui.QToolBar):
    """
    Crea una barra de tareas simple.

    @param liAcciones: lista de acciones, en forma de tupla = titulo, icono, clave
    @param siTexto: si muestra texto
    @param tamIcon: tama_o del icono
    @param rutina: rutina que se llama al pulsar una opcion. Por defecto sera parent.procesarTB().
        Y la clave enviada se obtiene de self.sender().clave
    """

    def __init__(self, parent, liAcciones=None, siTexto=True, tamIcon=32, puntos=None, background=None, style=None):

        QtGui.QToolBar.__init__(self, "BASICO", parent)

        if style:
            self.setToolButtonStyle(style)
            if style != QtCore.Qt.ToolButtonTextUnderIcon:
                tamIcon = 16
        elif siTexto:
            self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)

        self.setIconSize(QtCore.QSize(tamIcon, tamIcon))

        self.parent = parent

        self.f = TipoLetra(puntos=puntos) if puntos else None

        if background:
            self.setStyleSheet("QWidget { background: %s }" % background)

        if liAcciones:
            self.ponAcciones(liAcciones)

        else:
            self.dicTB = {}
            self.liAcciones = []

    def new(self, titulo, icono, clave, sep=True, toolTip=None):
        accion = QtGui.QAction(titulo, self.parent)
        accion.setIcon(icono)
        accion.setIconText(titulo)
        if toolTip:
            accion.setToolTip(toolTip)
        self.parent.connect(accion, QtCore.SIGNAL("triggered()"), clave)
        if self.f:
            accion.setFont(self.f)
        self.liAcciones.append(accion)
        self.addAction(accion)
        self.dicTB[clave] = accion
        if sep:
            self.addSeparator()

    def ponAcciones(self, liAcc):
        self.dicTB = {}
        self.liAcciones = []
        for datos in liAcc:
            if datos:
                if type(datos) == int:
                    self.addWidget(LB("").anchoFijo(datos))
                elif len(datos) == 3:
                    titulo, icono, clave = datos
                    self.new(titulo, icono, clave, False)
                else:
                    titulo, icono, clave, toolTip = datos
                    self.new(titulo, icono, clave, False, toolTip=toolTip)
            else:
                self.addSeparator()

    def reset(self, liAcciones):
        self.clear()
        self.ponAcciones(liAcciones)
        self.update()

    def vertical(self):
        self.setOrientation(QtCore.Qt.Vertical)
        return self

    def setPosVisible(self, pos, value):
        self.liAcciones[pos].setVisible(value)

    def setAccionVisible(self, key, value):
        accion = self.dicTB.get(key, None)
        if accion:
            accion.setVisible(value)


class TipoLetra(QtGui.QFont):
    def __init__(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        QtGui.QFont.__init__(self)
        if txt is None:
            cursiva = 1 if siCursiva else 0
            subrayado = 1 if siSubrayado else 0
            tachado = 1 if siTachado else 0
            txt = "%s,%d,-1,5,%d,%d,%d,%d,0,0" % (nombre, puntos, peso, cursiva, subrayado, tachado)
        self.fromString(txt)


class Tab(QtGui.QTabWidget):
    def nuevaTab(self, widget, texto, pos=None):
        if pos is None:
            self.addTab(widget, texto)
        else:
            self.insertTab(pos, widget, texto)

    def posActual(self):
        return self.currentIndex()

    def ponValor(self, cual, valor):
        self.setTabText(cual, valor)

    def activa(self, cual):
        self.setCurrentIndex(cual)

    def ponPosicion(self, pos):
        rpos = self.North
        if pos == "S":
            rpos = self.South
        elif pos == "E":
            rpos = self.East
        elif pos == "W":
            rpos = self.West
        self.setTabPosition(rpos)
        return self

    def ponIcono(self, pos, icono):
        self.setTabIcon(pos, icono)

    def ponFuente(self, f):
        self.setFont(f)
        return self

    def ponTipoLetra(self, nombre="", puntos=8, peso=50, siCursiva=False, siSubrayado=False, siTachado=False, txt=None):
        f = TipoLetra(nombre, puntos, peso, siCursiva, siSubrayado, siTachado, txt)
        self.setFont(f)
        return self

    def dispatchChange(self, dispatch):
        self.connect(self, QtCore.SIGNAL("currentChanged (int)"), dispatch)

        # def formaTriangular( self ):
        # self.setTabShape(self.Triangular)


class SL(QtGui.QSlider):
    def __init__(self, parent, minimo, maximo, nvalor, dispatch, tick=10, step=1):
        QtGui.QSlider.__init__(self, QtCore.Qt.Horizontal, parent)

        self.setMinimum(minimo)
        self.setMaximum(maximo)

        self.dispatch = dispatch
        if tick:
            self.setTickPosition(QtGui.QSlider.TicksBelow)
            self.setTickInterval(tick)
        self.setSingleStep(step)

        self.setValue(nvalor)

        self.connect(self, QtCore.SIGNAL("valueChanged (int)"), self.movido)

    def ponValor(self, nvalor):
        self.setValue(nvalor)
        return self

    def movido(self, valor):
        if self.dispatch:
            self.dispatch()

    def valor(self):
        return self.value()

    def ponAncho(self, px):
        self.setFixedWidth(px)
        return self

    # class PRB(QtGui.QProgressBar):
    # def __init__(self, minimo, maximo):
    # QtGui.QProgressBar.__init__(self)
    # self.setMinimum(minimo)
    # self.setMaximum(maximo)

    # def ponFormatoValor(self):
    # self.setFormat("%v")
    # return self

    # class Fecha(QtGui.QDateTimeEdit):
    # def __init__(self, fecha=None):
    # QtGui.QDateTimeEdit.__init__(self)

    # self.setDisplayFormat("dd-MM-yyyy")

    # self.setCalendarPopup(True)
    # calendar = QtGui.QCalendarWidget()
    # calendar.setFirstDayOfWeek(QtCore.Qt.Monday)
    # calendar.setGridVisible(True)
    # self.setCalendarWidget(calendar)

    # if fecha:
    # self.ponFecha(fecha)

    # def fecha2date(self, fecha):
    # date = QtCore.QDate()
    # date.setDate(fecha.year, fecha.month, fecha.day)
    # return date

    # def ponFecha(self, fecha):
    # self.setDate(self.fecha2date(fecha))
    # return self

    # def fecha(self):
    # date = self.date()
    # fecha = datetime.date(date.year(), date.month(), date.day())
    # return fecha

    # def minima(self, fecha):
    # previa = self.date()
    # fecha = self.fecha2date(fecha)

    # if previa < fecha:
    # self.ponFecha(fecha)

    # self.setMinimumDate(fecha)
    # return self

    # def maxima(self, fecha):
    # previa = self.date()
    # fecha = self.fecha2date(fecha)
    # if previa > fecha:
    # self.ponFecha(fecha)

    # self.setMaximumDate(fecha)
    # return self
