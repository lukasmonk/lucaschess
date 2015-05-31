# -*- coding: latin-1 -*-
import sys

select = raw_input( "Select GUI pyqt or pyside : " )

if select == "pyqt" :
    siPyside = False
elif select == "pyside" :
    siPyside = True
else :
    print "Goodbye"
    sys.exit()

# Especiales ----------------------------------------------------------------------------------------------
def hazRoot( siPyside ) :
    for fich in ( "lucas.py",  ) :
        f = open( fich, "rb")
        txt = ""
        for linea in f :
            if siPyside :
                if linea.startswith( "import sip" ) or linea.startswith( "sip.setapi" ) :
                    linea = "#" + linea
            else :
                if linea.startswith( "#import sip" ) or linea.startswith( "#sip.setapi" ) :
                    linea = linea.replace( "#", "" )
            txt += linea
        f.close()
        q = open( fich, "wb")
        q.write( txt )
        q.close()

def hazQTUtil2( siPyside ) :
    fich = "Code/QT/QTUtil2.py"
    f = open( fich, "rb")
    txt = ""
    for linea in f :
        if "if resp : #+pyside" in linea :
            if siPyside :
                linea = "    if resp : #+pyside\n"
            else :
                linea = "    #if resp : #+pyside\n"
        elif "resp = resp[0] #+pyside" in linea :
            if siPyside :
                linea = "        resp = resp[0] #+pyside\n"
            else :
                linea = "        #resp = resp[0] #+pyside\n"
        txt += linea
    f.close()
    q = open( fich, "wb")
    q.write( txt )
    q.close()

def hazInfo( siPyside ) :
    fich = "Code/QT/Info.py"
    f = open( fich, "rb")
    txt = ""
    for linea in f :
        if "ABOUT_GUI" in linea :
            if siPyside :
                linea = '               ( _( "ABOUT_GUI" ), "PySide - LGPL", "http://www.pyside.org/" ),\n'
            else :
                linea = '               ( _( "ABOUT_GUI" ), "PyQt4 - GPL", "http://www.riverbankcomputing.co.uk" ),\n'
        txt += linea
    f.close()
    q = open( fich, "wb")
    q.write( txt )
    q.close()
# Fin especiales ----------------------------------------------------------------------------------------------

import os, os.path
import glob
import datetime, time
import shutil
import win32file, win32con

def hazFich( fichero, siPyside ) :
    if fichero.endswith( ".py" ):
        print fichero
        li = []
        f = open( fichero, "rb" )
        vpyqt = "from PyQt4 import"
        vpyside = "from PySide import"
        for linea in f :
            if siPyside :
                if linea.startswith( vpyqt ) :
                    linea = linea.replace( vpyqt, vpyside )
            else :
                if linea.startswith( vpyside ) :
                    linea = linea.replace( vpyside, vpyqt )
            li.append( linea )
        f.close()
        q = open( fichero, "wb" )
        q.write( "".join(li))
        q.close()

#~ ############################################################################################
def hazDir( carpeta, siPyside ) :
    for fd in os.listdir( carpeta ) :
        fd = carpeta + "/" + fd
        if os.path.isfile( fd ) :
            hazFich( fd, siPyside )

hazRoot(siPyside)
hazQTUtil2(siPyside)
hazInfo(siPyside)
hazDir( "Code", siPyside )
hazDir( "Code/QT", siPyside )
# hazFich( "mosca.py", siPyside )
