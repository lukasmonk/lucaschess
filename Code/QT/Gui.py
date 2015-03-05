# -*- coding: latin-1 -*-

import os

from PyQt4 import QtCore, QtGui

import Code.VarGen as VarGen
import Code.Usuarios as Usuarios
import Code.Voice as Voice
import Code.Configuracion as Configuracion
import Code.Util as Util
import Code.QT.Controles as Controles
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.QTVarios as QTVarios
import Code.QT.QTUtil as QTUtil

def lanzaGUI(procesador):
    """
    Lanzador del interfaz gráfico de la aplicación.
    """

    # Comprobamos el lenguaje
    app = QtGui.QApplication([])

    liUsuarios = Usuarios.listaUsuarios()
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
            for k, nombre, porc in li:
                rotulo = nombre
                if porc != "100":
                    rotulo += " (%s%%)"%porc
                menu.opcion(k, nombre, nico.otro())
            resp = menu.lanza()
            if resp:
                configuracion.traductor = resp
                configuracion.graba()

    # Estilo
    global stylename
    styleName = configuracion.estilo
    app.setStyle(QtGui.QStyleFactory.create(styleName))
    app.setPalette(QtGui.QApplication.style().standardPalette())
    app.setEffectEnabled(QtCore.Qt.UI_AnimateMenu)

    if configuracion.familia:
        font = Controles.TipoLetra(configuracion.familia)
        app.setFont(font)

    VarGen.gc = QTUtil.GarbageCollector()

    # Lanzamos la pantalla
    procesador.iniciarGUI()

    resp = app.exec_()
    Voice.runVoice.close()

    return resp

class WPassword(QtGui.QDialog):
    def __init__(self, liUsuarios):
        QtGui.QDialog.__init__(self, None)
        self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

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
    # Miramos si alguno tiene clave, si es así, lanzamos ventana
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
            menu = Controles.Menu(None)  # No puede ser LCmenu, ya que todavía no existe la configuración
            menu.separador()

            for usuario in liUsuarios:
                menu.opcion(usuario, usuario.nombre, Iconos.PuntoNaranja())
                menu.separador()

            usuario = menu.lanza()
            if usuario is None:
                return None

    return usuario

