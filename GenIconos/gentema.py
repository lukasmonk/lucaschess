# -*- coding: latin-1 -*-

import base64
import os.path

"""Añadido control de repeticiones"""

dx = [0]

def funcionPNG( qbin, dic, desde, nomFuncion, nomDir, nomFichero ) :
    cFich = "%s/%s"%(nomDir,nomFichero)
    if not os.path.isfile( cFich ) :
        print "No existe " + cFich
        return ""
    tt = ( nomDir.lower(), nomFichero.lower() )
    if tt in dic :
        de, a = dic[tt]
    else :
        f = open( cFich, "rb" )
        o = f.read()
        f.close()
        qbin.write(o)
        tam = len(o)
        de = desde
        a = de + tam
        desde = a
        dic[tt] = (de,a)
        dx[0] += 1
    t = "def pm%s():\n"%nomFuncion
    t += '    return PM(%d,%d)\n\n'%(de,a)
    t += "def %s():\n"%nomFuncion
    t += '    return QtGui.QIcon(pm%s())\n\n'%nomFuncion
    return desde, t

def leeTema( cTema ) :
    f = open( cTema, "rb" )
    liImgs = f.read().splitlines()
    f.close()


    q = open( "../Code/QT/Iconos.py", "wb" )

    q.write( """\"\"\"Iconos y pixmap usados en el programa\"\"\"
from PyQt4 import QtGui

f = open("./IntFiles/Iconos.bin","rb")
binIconos = f.read()
f.close()

def icono(name):
    return eval( "%s()"%name )

def pixmap(name):
    return eval( "pm%s()"%name )

def PM(desde, hasta):
    pm = QtGui.QPixmap()
    pm.loadFromData( binIconos[desde:hasta] )
    return pm

""" )

    qbin = open( "../IntFiles/Iconos.bin", "wb" )

    dic = {}
    rep = set()
    desde = 0
    for x in liImgs :
        if x.startswith("#") :
            continue
        x = x.strip()
        if not x :
            continue
        li = x.split( " " )
        if len(li) == 3 :
            if li[0] in rep :
                print "error repetido", li[0]
            rep.add(li[0])
            desde, txt = funcionPNG( qbin, dic, desde, li[0], li[1], li[2] )
            q.write( txt )
        else :
            print "error", x

    q.close()

leeTema( "Formatos.tema" )

