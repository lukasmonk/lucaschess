# -*- coding: latin-1 -*-

import cPickle

import Code.Configuracion as Configuracion

class Usuario:
    nombre = ""
    numero = 0
    password = ""

def listaUsuarios():
    fichUsuarios = "%s/users.pkv" % Configuracion.activeFolder()
    try:
        f = open(fichUsuarios, "rb")
        s = f.read()
        f.close()
        v = cPickle.loads(s)
    except:
        v = None
    return v

def guardaUsuarios(liUsuarios):
    fichUsuarios = "%s/users.pkv" % Configuracion.activeFolder()
    f = open(fichUsuarios, "wb")
    f.write(cPickle.dumps(liUsuarios))
    f.close()
