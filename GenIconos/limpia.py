# -*- coding: latin-1 -*-
import os
# import shutil



def leeFormato():
    with open("Formatos.tema") as f:
        li = []
        for linea in f:
            linea = linea.strip()
            if linea and not linea.strip().startswith("#"):
                nombre, folder, png = linea.split(" ")
                folder = folder.replace("/", "\\")
                li.append(os.path.join(".", folder, png))
    return li

def leeCarpeta(li, carpeta):
    for x in os.listdir(carpeta):
        x = os.path.join(carpeta, x)
        if os.path.isdir(x):
            leeCarpeta(li, x)
        elif x.endswith(".png"):
            li.append(x)


liFormato = leeFormato()

liPNG = []
leeCarpeta(liPNG, ".")


for x in liFormato:
    if x not in liPNG:
        print "Falta el fichero", x

for x in liPNG:
    if x not in liFormato:
        print "Borrado el fichero", x
        os.remove(x)





# miraCarpeta( "." )

# for x in linom :
    # if x not in li :
        # print "Si esta en formato y no en codigo", x
# for x in li :
    # if x not in linom :
        # print "No esta en formato y si en codigo", x


