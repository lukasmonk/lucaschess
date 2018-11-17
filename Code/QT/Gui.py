import os

from PyQt4 import QtCore, QtGui

from Code import Configuracion
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTVarios
from Code import Usuarios
from Code import Util
from Code import VarGen


def lanzaGUI(procesador):
    """
    Lanzador del interfaz grafico de la aplicacion.
    """

    # Comprobamos el lenguaje
    app = QtGui.QApplication([])

    liUsuarios = Usuarios.Usuarios().lista
    usuario = None
    if liUsuarios:
        usuario = pideUsuario(liUsuarios)
        if usuario is None:
            return
        user = str(usuario.numero) if usuario.numero else ""
    else:
        user = ""

    activeFolder = Configuracion.activeFolder()
    siPedirLenguaje = not os.path.isdir(activeFolder) or not os.listdir(activeFolder)
    procesador.iniciaConUsuario(user)
    configuracion = procesador.configuracion
    if usuario:
        if not configuracion.jugador:
            configuracion.jugador = usuario.nombre
            configuracion.graba()
        elif configuracion.jugador != usuario.nombre:
            for usu in liUsuarios:
                if usu.numero == usuario.numero:
                    usu.nombre = configuracion.jugador
                    Usuarios.Usuarios().guardaLista(liUsuarios)

    # Comprobamos el lenguaje
    if siPedirLenguaje and not configuracion.traductor:
        if user:
            confMain = Configuracion.Configuracion("")
            ori = confMain.ficheroMExternos
            confMain.lee()
            confMain.limpia(usuario.nombre)
            confMain.ponCarpetas(user)
            confMain.graba()
            procesador.configuracion = confMain

            Util.copiaFichero(ori, confMain.carpeta)

        else:
            li = configuracion.listaTraducciones()
            menu = QTVarios.LCMenu(None)

            nico = QTVarios.rondoPuntos()
            for k, nombre, porc, author in li:
                rotulo = nombre
                if porc != "100":
                    rotulo += " (%s%%)" % porc
                menu.opcion(k, nombre, nico.otro())
            resp = menu.lanza()
            if resp:
                configuracion.traductor = resp
                configuracion.graba()

    # Estilo
    app.setStyle(QtGui.QStyleFactory.create(configuracion.estilo))

    if configuracion.palette:
        qpalette = QtGui.QPalette(QtGui.QPalette.Dark)
        palette = configuracion.palette
        def cl(tipo):
            return QtGui.QColor(palette[tipo])
        qpalette.setColor(QtGui.QPalette.Window, cl("Window"))
        qpalette.setColor(QtGui.QPalette.WindowText, cl("WindowText"))

        qpalette.setColor(QtGui.QPalette.Base, cl("Base"))
        qpalette.setColor(QtGui.QPalette.Text, cl("Text"))
        qpalette.setColor(QtGui.QPalette.AlternateBase, cl("AlternateBase"))

        qpalette.setColor(QtGui.QPalette.ToolTipBase, cl("ToolTipBase"))
        qpalette.setColor(QtGui.QPalette.ToolTipText, cl("ToolTipText"))

        qpalette.setColor(QtGui.QPalette.Button, cl("Button"))
        qpalette.setColor(QtGui.QPalette.ButtonText, cl("ButtonText"))
        qpalette.setColor(QtGui.QPalette.BrightText, cl("BrightText"))

        qpalette.setColor(QtGui.QPalette.Link, cl("Link"))

    else:
        qpalette = QtGui.QApplication.style().standardPalette()

    app.setPalette(qpalette)

    app.setEffectEnabled(QtCore.Qt.UI_AnimateMenu)

    QtGui.QFontDatabase.addApplicationFont('IntFiles/PIRATDIA.TTF')

    if configuracion.familia:
        font = Controles.TipoLetra(configuracion.familia)
        app.setFont(font)

    VarGen.gc = QTUtil.GarbageCollector()

    # Lanzamos la pantalla
    procesador.iniciarGUI()

    resp = app.exec_()

    return resp


class WPassword(QtGui.QDialog):
    def __init__(self, liUsuarios):
        QtGui.QDialog.__init__(self, None)
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        main = liUsuarios[0]

        self.setWindowTitle(main.trlucas)
        self.setWindowIcon(Iconos.Usuarios())

        liOpciones = [(usuario.nombre, usuario) for usuario in liUsuarios]
        lbU = Controles.LB(self, main.trusuario + ":")
        self.cbU = Controles.CB(self, liOpciones, main)

        lbP = Controles.LB(self, main.trpassword + ":")
        self.edP = Controles.ED(self).password()

        btaceptar = Controles.PB(self, main.traceptar, rutina=self.accept, plano=False)
        btcancelar = Controles.PB(self, main.trcancelar, rutina=self.reject, plano=False)

        ly = Colocacion.G()
        ly.controld(lbU, 0, 0).control(self.cbU, 0, 1)
        ly.controld(lbP, 1, 0).control(self.edP, 1, 1)

        lybt = Colocacion.H().relleno().control(btaceptar).espacio(10).control(btcancelar)

        layout = Colocacion.V().otro(ly).espacio(10).otro(lybt).margen(10)

        self.setLayout(layout)
        self.edP.setFocus()

    def resultado(self):
        usuario = self.cbU.valor()
        return usuario if self.edP.texto().strip() == usuario.password else None


def pideUsuario(liUsuarios):
    # Miramos si alguno tiene clave, si es asi, lanzamos ventana
    siPass = False
    for usuario in liUsuarios:
        if usuario.password:
            siPass = True
    if siPass:
        intentos = 3
        while True:
            w = WPassword(liUsuarios)
            if w.exec_():
                usuario = w.resultado()
                if usuario:
                    break
            else:
                return None
            intentos -= 1
            if intentos == 0:
                return None
    else:
        if len(liUsuarios) == 1:
            usuario = liUsuarios[0]
        else:
            menu = Controles.Menu(None)  # No puede ser LCmenu, ya que todavia no existe la configuracion
            menu.separador()

            for usuario in liUsuarios:
                menu.opcion(usuario, usuario.nombre, Iconos.PuntoNaranja())
                menu.separador()

            usuario = menu.lanza()
            if usuario is None:
                return None

    return usuario
