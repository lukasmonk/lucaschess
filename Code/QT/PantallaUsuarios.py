import os

import Code.Usuarios as Usuarios
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.QTUtil as QTUtil
import Code.QT.Grid as Grid
import Code.QT.Columnas as Columnas
import Code.QT.QTVarios as QTVarios
import Code.QT.Delegados as Delegados

class WUsuarios(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.configuracion = procesador.configuracion

        self.leeUsuarios()

        titulo = _("Users")
        icono = Iconos.Usuarios()
        extparam = "users"
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)

        # Toolbar
        liAcciones = (  ( _("Accept"), Iconos.Aceptar(), "aceptar" ), None,
                        ( _("Cancel"), Iconos.Cancelar(), "cancelar" ), None,
                        ( _("New"), Iconos.Nuevo(), "nuevo" ), None,
                        ( _("Remove"), Iconos.Borrar(), "borrar" ), None,
        )
        tb = Controles.TB(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 40, siCentrado=True)
        oColumnas.nueva("USUARIO", _("User"), 140, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("PASSWORD", _("Password"), 100, edicion=Delegados.LineaTextoUTF8(siPassword=True))

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
        self.liUsuarios = Usuarios.listaUsuarios()
        if self.liUsuarios is None:
            usuario = Usuarios.Usuario()
            usuario.numero = 0
            usuario.nombre = self.configuracion.jugador
            usuario.password = ""
            self.liUsuarios = [usuario]

        main = self.liUsuarios[0]
        # Para que al pedir la password siempre en el idioma del main en principio solo hace falta el password pero por si acaso se cambia de opinion
        main.trlucas = _("Lucas Chess")
        main.trusuario = _("User")
        main.trpassword = _("Password")
        main.traceptar = _("Accept")
        main.trcancelar = _("Cancel")

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "aceptar":
            self.guardarVideo()
            self.aceptar()
        elif accion == "cancelar":
            self.guardarVideo()
            self.reject()
        elif accion == "nuevo":
            self.nuevo()
        elif accion == "borrar":
            self.borrar()

    def nuevo(self):

        li = []
        for usuario in self.liUsuarios:
            li.append(usuario.numero)

        plantilla = self.configuracion.carpetaUsers + "/%d"
        numero = 1
        while (numero in li) or os.path.isdir(plantilla % numero):
            numero += 1

        usuario = Usuarios.Usuario()
        usuario.nombre = _X(_("User %1"), str(numero))
        usuario.numero = numero
        usuario.clave = ""

        self.liUsuarios.append(usuario)
        self.grid.refresh()
        self.grid.goto(len(self.liUsuarios) - 1, 1)
        self.grid.setFocus()

    def aceptar(self):

        Usuarios.guardaUsuarios(self.liUsuarios)
        self.accept()

    def borrar(self):

        fila = self.grid.recno()
        if fila > 0:
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
        else:
            usuario.password = valor

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        usuario = self.liUsuarios[fila]
        if clave == "NUMERO":
            return str(usuario.numero) if usuario.numero else "-"
        elif clave == "USUARIO":
            return usuario.nombre
        if clave == "PASSWORD":
            return "x" * len(usuario.password)

def editaUsuarios(procesador):
    w = WUsuarios(procesador)
    if w.exec_():
        pass
