from Code import Util
import cPickle

from Code import Configuracion


class Usuario:
    nombre = ""
    numero = 0
    password = ""


class Usuarios:
    def __init__(self):
        self.fichero = "%s/users.pkx" % Configuracion.activeFolder()
        ant = "%sv"% self.fichero[:-1]
        if Util.existeFichero(ant):
            self.antversion(ant)
        self.lista = self.read()

    def antversion(self, ant):
        try:
            with open(ant) as f:
                s = f.read()
                self.lista = cPickle.loads(s)
                self.save()
        except:
            pass
        Util.borraFichero(ant)

    def save(self):
        Util.guardaDIC(self.lista, self.fichero)

    def read(self):
        if Util.existeFichero(self.fichero):
            resp = Util.recuperaDIC(self.fichero)
            return resp if resp else []
        return []

    def guardaLista(self, lista):
        self.lista = lista
        self.save()

