import os
import shutil

from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Usuarios
from Code.QT import FormLayout


class WUsuarios(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.configuracion = procesador.configuracion

        self.leeUsuarios()

        titulo = _("Users")
        icono = Iconos.Usuarios()
        extparam = "users"
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)

        # Toolbar
        liAcciones = ((_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      (_("New"), Iconos.Nuevo(), self.nuevo), None,
                      (_("Remove"), Iconos.Borrar(), self.borrar), None,
                      )
        tb = QTVarios.LCTB(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 40, siCentrado=True)
        oColumnas.nueva("USUARIO", _("User"), 140, edicion=Delegados.LineaTextoUTF8())
        # oColumnas.nueva("PASSWORD", _("Password"), 100, edicion=Delegados.LineaTextoUTF8(siPassword=True))

        self.grid = Grid.Grid(self, oColumnas, siEditable=True)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).margen(3)
        self.setLayout(layout)

        self.grid.gotop()
        self.grid.setFocus()

        self.siPlay = False

        self.registrarGrid(self.grid)

        if not self.recuperarVideo():
            self.resize(310, 400)

    def leeUsuarios(self):
        self.liUsuarios = Usuarios.Usuarios().lista
        if not self.liUsuarios:
            usuario = Usuarios.Usuario()
            usuario.numero = 0
            usuario.password = ""
            self.liUsuarios = [usuario]

        main = self.liUsuarios[0]
        main.nombre = self.configuracion.jugador
        # Para que al pedir la password siempre en el idioma del main en principio solo hace falta el password pero por si acaso se cambia de opinion
        main.trlucas = _("Lucas Chess")
        main.trusuario = _("User")
        main.trpassword = _("Password")
        main.traceptar = _("Accept")
        main.trcancelar = _("Cancel")

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def nuevo(self):

        li = []
        for usuario in self.liUsuarios:
            li.append(usuario.numero)

        # plantilla = self.configuracion.carpetaUsers + "/%d"
        numero = 1
        while (numero in li):# or os.path.isdir(plantilla % numero):
            numero += 1

        usuario = Usuarios.Usuario()
        usuario.nombre = _X(_("User %1"), str(numero))
        usuario.numero = numero
        usuario.password = ""

        self.liUsuarios.append(usuario)
        self.grid.refresh()
        self.grid.goto(len(self.liUsuarios) - 1, 1)
        self.grid.setFocus()

    def aceptar(self):
        self.grid.goto(len(self.liUsuarios) - 1, 1)
        self.grid.setFocus()
        self.guardarVideo()
        Usuarios.Usuarios().guardaLista(self.liUsuarios)
        self.accept()

    def borrar(self):
        fila = self.grid.recno()
        if fila > 0:
            usuario = self.liUsuarios[fila]
            carpeta = "%s/users/%d/"%(self.configuracion.carpeta, usuario.numero)
            if os.path.isdir(carpeta):
                if QTUtil2.pregunta(self, _("Do you want to remove all data of this user?")):
                    shutil.rmtree(carpeta)
            del self.liUsuarios[fila]
            self.grid.refresh()
            self.grid.setFocus()

    def gridNumDatos(self, grid):
        return len(self.liUsuarios)

    def gridPonValor(self, grid, fila, columna, valor):
        campo = columna.clave
        valor = valor.strip()
        usuario = self.liUsuarios[fila]
        if campo == "USUARIO":
            if valor:
                usuario.nombre = valor
            else:
                QTUtil.beep()
        # else:
        #     usuario.password = valor

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        usuario = self.liUsuarios[fila]
        if clave == "NUMERO":
            return str(usuario.numero) if usuario.numero else "-"
        elif clave == "USUARIO":
            return usuario.nombre
        # if clave == "PASSWORD":
        #     return "x" * len(usuario.password)


def editaUsuarios(procesador):
    w = WUsuarios(procesador)
    if w.exec_():
        pass


def setPassword(procesador):
    configuracion = procesador.configuracion

    npos = 0
    user = configuracion.user
    liUsuarios = Usuarios.Usuarios().lista
    if user:
        numero = int(user)
        for n, usu in enumerate(liUsuarios):
            if usu.numero == numero:
                npos = n
                break
        if npos == 0:
            return
    else:
        if not liUsuarios:
            usuario = Usuarios.Usuario()
            usuario.numero = 0
            usuario.password = ""
            usuario.nombre = configuracion.jugador
            liUsuarios = [usuario]

    usuario = liUsuarios[npos]

    while True:
        liGen = [FormLayout.separador]

        config = FormLayout.Editbox(_("Current"), ancho=120, siPassword=True)
        liGen.append((config, ""))

        config = FormLayout.Editbox(_("New"), ancho=120, siPassword=True)
        liGen.append((config, ""))

        config = FormLayout.Editbox(_("Repeat"), ancho=120, siPassword=True)
        liGen.append((config, ""))

        resultado = FormLayout.fedit(liGen, title=_("Set password"), parent=procesador.pantalla, icon=Iconos.Password())

        if resultado:
            previa, nueva, repite = resultado[1]

            error = ""
            if previa != usuario.password:
                error = _("Current password is not correct")
            else:
                if nueva != repite:
                    error = _("New password and repetition are not the same")

            if error:
                QTUtil2.mensError(procesador.pantalla, error)

            else:
                usuario.password = nueva
                Usuarios.Usuarios().guardaLista(liUsuarios)
                return

