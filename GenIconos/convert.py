# -*- coding: latin-1 -*-

import os.path
import os
import subprocess
import shutil

def convierte( fichero ) :
    print fichero
    li = [ "..\\convert.exe", fichero, "-strip", "hola.png" ]
    process = subprocess.call(li)
    os.remove(fichero)
    shutil.move( "hola.png", fichero )

def leeTema( cTema ) :
    f = open( cTema, "rb" )
    liImgs = f.read().splitlines()
    f.close()
    for x in liImgs :
        if x.startswith("#") :
            continue
        x = x.strip()
        if not x :
            continue
        li = x.split( " " )
        if len(li) == 3 :
            rotulo, carpeta, fichero = li
            convierte( os.path.join(carpeta, fichero) )
        else :
            print "error", x

leeTema( "Formatos.tema" )

