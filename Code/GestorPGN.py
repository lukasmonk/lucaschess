# -*- coding: latin-1 -*-

import os
import sys
import random

from Code import Gestor
from Code import PGN
from Code import Partida
from Code.QT import Iconos
from Code.QT import PantallaPGN
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
import Code.SQL.Base as SQLBase
from Code import Util
from Code.Constantes import *


class GestorPGN(Gestor.Gestor):
    def inicio(self, opcion):
        self.tipoJuego = kJugPGN
        self.finExit = False
        self.estado = kFinJuego
        self.nuestroFichero = self.configuracion.salvarFichero
        self.siNuestroFichero = self.nuestroFichero and os.path.isfile(self.nuestroFichero)

        liOpciones = [k_mainmenu]
        self.muestraInicial = True  # Para controlar si con un Cancel buscando un PGN se puede terminar

        self.tablero.ponPosicion(self.procesador.posicionInicial)
        self.mostrarIndicador(False)
        self.pantalla.ponToolBar(liOpciones)
        self.pantalla.activaJuego(False, False)
        self.quitaCapturas()
        self.refresh()
        self.procesarAccion(opcion)

    def procesarAccion(self, clave):

        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_pgnPaste:
            self.paste()

        elif clave == k_pgnFichero:
            self.fichero(siBuscar=True)

        elif clave == k_anterior:
            self.ficheroMostrar(self.dicDB, True, siAnterior=True)

        elif clave == k_siguiente:
            self.ficheroMostrar(self.dicDB, True, siSiguiente=True)

        elif clave == k_pgnNuestroFichero:
            self.fichero(siNuestro=True)

        elif clave == k_trasteros:
            self.trasterosMenu()

        elif clave == k_pgnInformacion:
            self.informacion()

        elif clave == k_configurar:
            self.configurar(siCambioTutor=True)

        elif clave == k_utilidades:
            liMasOpciones = (
                ("libros", _("Consult a book"), Iconos.Libros()),
                (None, None, None),
                ("bookguide", _("Personal Opening Guide"), Iconos.BookGuide()),
                (None, None, None),
                ("jugarSolo", _X(_('Open in "%1"'), _("Create your own game")), Iconos.JuegaSolo()),
                (None, None, None),
                ("play", _('Play current position'), Iconos.MoverJugar())
            )
            resp = self.utilidades(liMasOpciones)
            if resp == "libros":
                self.librosConsulta(False)
            elif resp == "bookguide":
                self.bookGuide()
            elif resp == "jugarSolo":
                self.procesador.jugarSolo(partida=self.partidaCompleta())
            elif resp == "play":
                self.jugarPosicionActual()

        elif clave == k_pgnFicheroRepite:
            self.ficheroRepite()

        elif clave == k_jugadadia:
            self.miniatura()

        elif clave == k_pgnComandoExterno:
            self.comandoExterno()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def comandoExterno(self):
        self.finExit = True
        fichero = sys.argv[1]
        if os.path.isfile(fichero):
            siTemporal = os.path.dirname(fichero).lower() == "tmp"
            self.pantalla.soloEdicionPGN(None if siTemporal else fichero)
            self.ficheroTemporal = fichero
            self.fichero(path=fichero)
        else:
            self.finPartida()

    def finPartida(self):
        if self.finExit:
            fichero = sys.argv[1]
            if os.path.dirname(fichero).lower() == "tmp":
                Util.borraFichero(fichero)
            self.procesador.procesarAccion(k_terminar)
            self.procesador.pararMotores()
            self.procesador.quitaKibitzers()
            sys.exit(0)
        else:
            self.quitaCapturas()
            self.procesador.inicio()

    def finalX(self):
        self.finPartida()
        return False

    def trasterosMenu(self):
        menu = QTVarios.LCMenu(self.pantalla)

        icoTras = Iconos.Trastero()
        liTras = self.configuracion.liTrasteros
        for ntras, uno in enumerate(liTras):
            carpeta, trastero = uno
            menu.opcion(ntras, "%s  (%s)" % (trastero, carpeta), icoTras)
            menu.separador()

        resp = menu.lanza()
        if resp is not None:
            carpeta, trastero = liTras[resp]
            path = os.path.join(carpeta, trastero)
            self.fichero(path=path)

    def paste(self):
        texto = QTUtil.traePortapapeles()
        if texto:
            pgn = PGN.UnPGN()
            encoding = Util.txt_encoding(texto)
            if encoding != "latin1":
                texto = texto.decode(encoding)
            try:
                pgn.leeTexto(texto)
            except:
                pgn.siError = True
            if pgn.siError:
                QTUtil2.mensError(self.pantalla,
                                  _("The text from the clipboard does not contain a chess game in PGN format"))
                self.finPartida()
                return
            self.pgnPaste = texto
            self.mostrar(pgn, False)

    def miniatura(self):
        self.pensando(True)

        fichero = "./IntFiles/miniaturas.gm"
        tam = Util.tamFichero(fichero)
        pos = random.randint(0, tam-600)
        with open(fichero) as fm:
            fm.seek(pos)
            fm.readline()
            linea = fm.readline()
            lig = linea.split("|")
            liTags = []
            pv = lig[-1]
            for n in range(len(lig)-1):
                if "·" in lig[n]:
                    k, v = lig[n].split("·")
                    liTags.append((k, v))
            p = Partida.PartidaCompleta(liTags=liTags)
            p.leerPV(pv)
            txt = p.pgn()
            pgn = PGN.UnPGN()
            pgn.leeTexto(txt)
        self.pensando(False)
        if pgn.siError:
            return
        self.pgnPaste = txt
        self.mostrar(pgn, False)

    def mostrar(self, pgn, siRepiteFichero, siBlancas=None):
        self.pensando(True)
        self.partida.leeOtra(pgn.partida)
        self.partida.asignaApertura()
        blancas = pgn.variable("WHITE")
        negras = pgn.variable("BLACK")
        resultado = pgn.variable("RESULT")

        if siBlancas is None:
            siBlancas = not self.partida.siEmpiezaConNegras
        self.siJugamosConBlancas = siBlancas
        self.tablero.ponerPiezasAbajo(siBlancas)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas()
        self.ponRotulo1("%s : <b>%s</b><br>%s : <b>%s</b>" % (_("White"), blancas, _("Black"), negras))
        self.ponRotulo2("%s : <b>%s</b>" % (_("Result"), resultado))

        self.tablero.desactivaTodas()

        self.ponCapInfoPorDefecto()

        if self.partida.siFenInicial():
            self.ponteAlPrincipio()
        else:
            self.ponteAlPrincipioColor()

        self.pensando(False)

        liOpciones = [k_mainmenu, k_pgnInformacion]  # ,k_pgnPaste,k_pgnFichero,k_jugadadia ]
        # if self.configuracion.liTrasteros:
        # liOpciones.append( k_trasteros )

        if siRepiteFichero:
            liOpciones.insert(2, k_pgnFicheroRepite)
            liOpciones.insert(2, k_siguiente)
            liOpciones.insert(2, k_anterior)
            # if self.siNuestroFichero:
            # liOpciones.append( k_pgnNuestroFichero )
        liOpciones.append(k_configurar)
        liOpciones.append(k_utilidades)
        self.pantalla.ponToolBar(liOpciones)

        self.muestraInicial = False

    def fichero(self, siNuestro=False, siBuscar=False, path=""):
        if path:
            if not os.path.isfile(path):
                return
        self.siFicheroNuestro = siNuestro
        if siNuestro:
            path = self.nuestroFichero
        elif siBuscar:
            # Elegimos el fichero
            files = QTVarios.select_pgns(self.pantalla)
            if not files:
                if self.muestraInicial:
                    self.finPartida()
                return
            if len(files) == 1:
                path = files[0]
            else:
                path = self.configuracion.ficheroTemporal("pgn")
                with open(path, "wb") as q:
                    for fich in files:
                        with open(fich, "rb") as f:
                            q.write(f.read())

        # ~ else ya esta el nombre

        fpgn = PGN.PGN(self.configuracion)

        dicDB = fpgn.leeFichero(self.pantalla, path)
        if dicDB is None:
            return None

        self.ficheroMostrar(dicDB, False)

    def ficheroRepite(self):
        self.ficheroMostrar(self.dicDB, True)

    def ficheroMostrar(self, dicDB, siRepite, siAnterior=False, siSiguiente=False):

        bd = SQLBase.DBBase(dicDB["PATHDB"])

        if (not siRepite) and self.siFicheroNuestro:
            orden = "ROWID DESC"
        else:
            orden = ""

        dClavesTam = dicDB["DCLAVES"]
        dbf = bd.dbf("GAMES", ",".join(dClavesTam.keys()),
                     orden=orden)  # La lectura se hace en la pantalla, para que la haga en el mismo sitio tanto siRepite como si no

        estadoWpgn = dicDB["ESTADOWPGN"] if siRepite else None

        if siAnterior or siSiguiente:
            siSeguir = True
            siSeHaBorradoAlgo = False
            dbf.leer()
            recno = estadoWpgn.recno
            if siAnterior:
                if recno > 0:
                    recno -= 1
            elif siSiguiente:
                if recno < dbf.reccount() - 1:
                    recno += 1
            dbf.goto(recno)
            estadoWpgn.recno = recno
        else:
            siSeguir, estadoWpgn, siSeHaBorradoAlgo = PantallaPGN.elegirPGN(self.pantalla, dbf, dClavesTam, self,
                                                                            estadoWpgn)

        if siSeguir:
            self.pensando(True)
            rid = dbf.rowid(dbf.recno)
            self.dicDB = dicDB
            dicDB["ESTADOWPGN"] = estadoWpgn
            dbf.cerrar()
            dbf = bd.dbfT("GAMES", ",".join(dClavesTam.keys()) + ",PGN", condicion="ROWID=%d" % rid)
            dbf.leer()
            dbf.gotop()
            dicDatos = dbf.dicValores()
            self.pgnPaste = dicDatos["PGN"]

            dbf.cerrar()
            pgn = PGN.UnPGN()
            pgn.leeTexto(self.pgnPaste)
            siMostrar = not pgn.siError
            self.pensando(False)
            if not siMostrar:
                QTUtil2.mensError(self.pantalla, _("This is not a valid PGN file"))

        else:
            siMostrar = False

        bd.cerrar()

        if siSeHaBorradoAlgo:
            fpgn = PGN.PGN(self.configuracion)
            fpgn.borraReferenciaA(dicDB["FICHERO"])

        if siMostrar:
            self.mostrar(pgn, True)
        elif self.muestraInicial or self.finExit:
            self.finPartida()

    def informacion(self):
        li = self.pgnPaste.split("\n")

        menu = QTVarios.LCMenu(self.pantalla)

        clave = ""

        for linea in li:
            if linea.startswith("["):
                ti = linea.split('"')
                if len(ti) == 3:
                    clave = ti[0][1:].strip()
                    siFecha = clave.upper().endswith("DATE")
                    if clave.upper() == "OPENING":
                        continue
                    clave = clave[0].upper() + clave[1:].lower()
                    valor = ti[1].strip()
                    if siFecha:
                        valor = valor.replace(".??", "").replace(".?", "")
                    valor = valor.strip("?")
                    if valor:
                        menu.opcion(clave, "%s : %s" % (clave, valor), Iconos.PuntoAzul())

        apertura = self.partida.apertura
        if apertura:
            menu.separador()
            nom = apertura.trNombre
            ape = _("Opening")
            rotulo = nom if ape.upper() in nom.upper() else ("%s : %s" % (ape, nom))
            menu.opcion(clave, rotulo, Iconos.PuntoNaranja())

        menu.lanza()

    def actualPGN(self):

        # cabecera sera el inicio de pgnpaste
        txt = self.pgnPaste.strip()
        cab = ""
        result = ""
        for linea in txt.split("\n"):
            if linea.startswith("["):
                if "result" in linea.lower():
                    li = linea.split('"')
                    result = li[1]
                cab += linea.strip() + "\n"
            else:
                break
        return cab + "\n" + self.partida.pgnBase() + " " + result

    def partidaCompleta(self):
        txt = self.pgnPaste.strip()
        liTags = []
        for linea in txt.split("\n"):
            if linea.startswith("["):
                ti = linea.split('"')
                if len(ti) == 3:
                    clave = ti[0][1:].strip()
                    valor = ti[1].strip()
                    liTags.append([clave, valor])
            else:
                break
        pc = Partida.PartidaCompleta(liTags=liTags)
        pc.leeOtra(self.partida)
        return pc
