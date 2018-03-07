import copy
import os
import subprocess

from PyQt4 import QtCore, QtGui

from Code import AperturasStd
from Code import ControlPosicion
from Code import Jugada
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import Util
from Code import VarGen
from Code import Variantes


class WAperturas(QTVarios.WDialogo):
    def __init__(self, owner, configuracion, bloqueApertura):
        icono = Iconos.Apertura()
        titulo = _("Select an opening")
        extparam = "selectOpening"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Variables--------------------------------------------------------------------------
        self.apStd = AperturasStd.apTrain
        self.configuracion = configuracion
        self.partida = Partida.Partida()
        self.bloqueApertura = bloqueApertura
        self.liActivas = []

        # Tablero
        confTablero = configuracion.confTablero("APERTURAS", 32)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponMensajero(self.mueveHumano)

        # Current pgn
        self.lbPGN = Controles.LB(self, "").ponWrap().ponTipoLetra(puntos=10, peso=75)

        # Movimiento
        self.siMoviendoTiempo = False

        lyBM, tbBM = QTVarios.lyBotonesMovimiento(self, "", siLibre=False, tamIcon=24)
        self.tbBM = tbBM

        # Tool bar
        tb = Controles.TBrutina(self)
        tb.new(_("Accept"), Iconos.Aceptar(), self.aceptar)
        tb.new(_("Cancel"), Iconos.Cancelar(), self.cancelar)
        tb.new(_("Reinit"), Iconos.Reiniciar(), self.resetPartida)
        tb.new(_("Takeback"), Iconos.Atras(), self.atras)
        tb.new(_("Remove"), Iconos.Borrar(), self.borrar)

        # Lista Aperturas
        oColumnas = Columnas.ListaColumnas()
        dicTipos = {"b": Iconos.pmSun(), "n": Iconos.pmPuntoAzul(), "l": Iconos.pmNaranja()}
        oColumnas.nueva("TIPO", "", 24, edicion=Delegados.PmIconosBMT(dicIconos=dicTipos))
        oColumnas.nueva("OPENING", _("Possible continuation"), 480)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, altoFila=32)
        self.registrarGrid(self.grid)

        # # Derecha
        lyD = Colocacion.V().control(tb).control(self.grid)
        gbDerecha = Controles.GB(self, "", lyD)

        # # Izquierda
        lyI = Colocacion.V().control(self.tablero).otro(lyBM).control(self.lbPGN)
        gbIzquierda = Controles.GB(self, "", lyI)

        splitter = QtGui.QSplitter(self)
        splitter.addWidget(gbIzquierda)
        splitter.addWidget(gbDerecha)
        self.registrarSplitter(splitter, "splitter")

        # Completo
        ly = Colocacion.H().control(splitter).margen(3)
        self.setLayout(ly)

        self.ponActivas()
        self.resetPartida()
        self.actualizaPosicion()

        dic = {'_SIZE_': '916,444', 'SP_splitter': [356, 548]}
        self.recuperarVideo(dicDef=dic)

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.tablero.desactivaTodas()

        movimiento = desde + hasta

        posicion = self.partida.jugada(self.posCurrent).posicion if self.posCurrent >= 0 else self.partida.ultPosicion

        # Peon coronando
        if not coronacion and posicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(posicion.siBlancas)
            if coronacion is None:
                self.sigueHumano()
                return False
        if coronacion:
            movimiento += coronacion

        siBien, mens, jg = Jugada.dameJugada(posicion, desde, hasta, coronacion)
        if siBien:
            self.nuevaJugada(jg)
        else:
            self.actualizaPosicion()

    def gridNumDatos(self, grid):
        return len(self.liActivas)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        ap = self.liActivas[fila]
        if clave == "TIPO":
            return "b" if ap.siBasic else "n"
        else:
            return ap.nombre + "\n" + ap.pgn

    def gridDobleClick(self, grid, fila, columna):
        if -1 < fila < len(self.liActivas):
            ap = self.liActivas[fila]
            self.partida.reset()
            self.partida.leerPV(ap.a1h8)
            self.ponActivas()

    def nuevaJugada(self, jg):
        self.posCurrent += 1
        if self.posCurrent < self.partida.numJugadas():
            self.partida.liJugadas = self.partida.liJugadas[:self.posCurrent]
        self.partida.append_jg(jg)
        self.ponActivas()

    def ponActivas(self):
        li = self.apStd.listaAperturasPosibles(self.partida, True)
        if li:
            li = sorted(li, key=lambda ap: ("A" if ap.siBasic else "Z") + ap.pgn)
        else:
            li = []
        self.liActivas = li

        self.tablero.ponPosicion(self.partida.ultPosicion)

        self.partida.asignaApertura()
        txt = self.partida.pgnSP()
        if self.partida.apertura:
            txt = '<span style="color:gray;">%s</span><br>%s' % (self.partida.apertura.nombre, txt)

        self.lbPGN.ponTexto(txt)
        self.posCurrent = self.partida.numJugadas() - 1

        self.actualizaPosicion()
        self.grid.refresh()
        self.grid.gotop()

        w = self.width()
        self.adjustSize()
        if self.width() != w:
            self.resize(w, self.height())

    def actualizaPosicion(self):
        if self.posCurrent > -1:
            jg = self.partida.jugada(self.posCurrent)
            posicion = jg.posicion
        else:
            posicion = self.partida.iniPosicion
            jg = None

        self.tablero.ponPosicion(posicion)
        self.tablero.activaColor(posicion.siBlancas)
        if jg:
            self.tablero.ponFlechaSC(jg.desde, jg.hasta)

    def resetPartida(self):
        self.partida.reset()
        if self.bloqueApertura:
            self.partida.leerPV(self.bloqueApertura.a1h8)
        self.ponActivas()
        self.mueve(siFinal=True)

    def terminar(self):
        self.siMoviendoTiempo = False
        self.guardarVideo()

    def closeEvent(self, event):
        self.terminar()

    def aceptar(self):
        self.terminar()
        self.accept()

    def cancelar(self):
        self.terminar()
        self.reject()

    def atras(self):
        self.partida.anulaSoloUltimoMovimiento()
        self.ponActivas()

    def borrar(self):
        self.partida.reset()
        self.ponActivas()

    def procesarTB(self):
        accion = self.sender().clave
        if accion == "MoverAdelante":
            self.mueve(nSaltar=1)
        elif accion == "MoverAtras":
            self.mueve(nSaltar=-1)
        elif accion == "MoverInicio":
            self.mueve(siInicio=True)
        elif accion == "MoverFinal":
            self.mueve(siFinal=True)
        elif accion == "MoverTiempo":
            self.mueveTiempo()

    def mueve(self, siInicio=False, nSaltar=0, siFinal=False):
        numJugadas = self.partida.numJugadas()
        if nSaltar:
            pos = self.posCurrent + nSaltar
            if 0 <= pos < numJugadas:
                self.posCurrent = pos
            else:
                return
        elif siInicio:
            self.posCurrent = -1
        elif siFinal:
            self.posCurrent = numJugadas - 1
        else:
            return
        self.actualizaPosicion()

    def mueveTiempo(self):
        if self.siMoviendoTiempo:
            self.siMoviendoTiempo = False

        else:
            self.siMoviendoTiempo = True
            self.mueve(siInicio=True)
            QtCore.QTimer.singleShot(1000, self.siguienteTiempo)

    def siguienteTiempo(self):
        if self.siMoviendoTiempo:
            if self.posCurrent < self.partida.numJugadas() - 1:
                self.mueve(nSaltar=1)
                QtCore.QTimer.singleShot(2500, self.siguienteTiempo)
            else:
                self.siMoviendoTiempo = False

    def resultado(self):
        if self.partida.numJugadas() == 0:
            return None
        ap = self.partida.apertura
        if ap is None:
            ap = AperturasStd.AperturasStd(_("Unknown"))
            ap.a1h8 = self.partida.pv()
        else:
            if not self.partida.last_jg().siApertura:
                p = Partida.Partida()
                p.leerPV(ap.a1h8)
                ap.a1h8 = self.partida.pv()
                ap.trNombre += " + %s" % (self.partida.pgnSP()[len(p.pgnSP()) + 1:],)

        ap.pgn = self.partida.pgnSP()
        return ap


class EntrenamientoApertura(QTVarios.WDialogo):
    def __init__(self, owner, listaAperturasStd, dicDatos):
        icono = Iconos.Apertura()
        titulo = _("Learn openings by repetition")
        extparam = "opentrainingE"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        nombre = dicDatos.get("NOMBRE", "")
        self.listaAperturasStd = listaAperturasStd
        self.liBloques = self.leeBloques(dicDatos.get("LISTA", []))

        # Toolbar
        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      (_("Add"), Iconos.Nuevo(), self.nueva), None,
                      (_("Modify"), Iconos.Modificar(), self.modificar), None,
                      (_("Remove"), Iconos.Borrar(), self.borrar), None,
                      ]
        tb = Controles.TBrutina(self, liAcciones)

        lbNombre = Controles.LB(self, _("Name") + ": ")
        self.edNombre = Controles.ED(self, nombre)

        lyNombre = Colocacion.H().control(lbNombre).control(self.edNombre)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NOMBRE", _("Name"), 240)
        oColumnas.nueva("PGN", _("Moves"), 360)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)
        n = self.grid.anchoColumnas()
        self.grid.setMinimumWidth(n + 20)
        self.registrarGrid(self.grid)
        self.grid.gotop()

        layout = Colocacion.V().control(tb).otro(lyNombre).control(self.grid)

        self.setLayout(layout)
        self.recuperarVideo()

    def gridNumDatos(self, grid):
        return len(self.liBloques)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        bloque = self.liBloques[fila]
        if clave == "NOMBRE":
            return bloque.trNombre
        return bloque.pgn

    def gridDobleClick(self, grid, fil, col):
        self.modificar()

    def leeBloques(self, liPV):
        li = []
        for pv in liPV:
            p = Partida.Partida()
            p.leerPV(pv)
            p.asignaApertura()
            ap = p.apertura
            if ap is None:
                ap = AperturasStd.AperturasStd(_("Unknown"))
                ap.a1h8 = pv
            else:
                ap.a1h8 = pv
                ap.pgn = ap.pgn.replace(". ", ".")
                nap = len(ap.pgn)
                pgnSP = p.pgnSP()
                if len(pgnSP) > nap:
                    ap.trNombre += " + %s" % (pgnSP[nap + 1:],)
            ap.pgn = p.pgnSP()
            li.append(ap)
        return li

    def nueva(self):
        bloque = self.editar(None)
        if bloque:
            self.liBloques.append(bloque)
            if not self.edNombre.texto().strip():
                self.edNombre.ponTexto(bloque.trNombre)
            self.grid.refresh()
            self.grid.gobottom()

    def modificar(self):
        fila = self.grid.recno()
        if fila >= 0:
            bloque = self.liBloques[fila]
            bloque = self.editar(bloque)
            if bloque:
                self.liBloques[fila] = bloque
                self.grid.refresh()

    def editar(self, bloque):
        me = QTUtil2.unMomento(self)
        w = WAperturas(self, VarGen.configuracion, bloque)
        me.final()
        if w.exec_():
            return w.resultado()
        return None

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            bloque = self.liBloques[fila]
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), bloque.trNombre)):
                del self.liBloques[fila]
                self.grid.refresh()

    def aceptar(self):
        if not self.liBloques:
            QTUtil2.mensError(self, _("you have not indicated any opening"))
            return

        self.nombre = self.edNombre.texto().strip()
        if not self.nombre:
            if len(self.liBloques) == 1:
                self.nombre = self.liBloques[0].trNombre
            else:
                QTUtil2.mensError(self, _("Not indicated the name of training"))
                return

        self.accept()

    def cancelar(self):
        self.reject()

    def listaPV(self):
        li = []
        for bloque in self.liBloques:
            li.append(bloque.a1h8)
        return li


class EntrenamientoAperturas(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.procesador = procesador
        self.ficheroDatos = procesador.configuracion.ficheroEntAperturas
        self.ficheroParam = procesador.configuracion.ficheroEntAperturasPar
        self.listaAperturasStd = AperturasStd.apTrain
        self.lista = self.leer()

        owner = procesador.pantalla
        icono = Iconos.Apertura()
        titulo = _("Learn openings by repetition")
        extparam = "opentraining"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        liAcciones = [(_("Close"), Iconos.MainMenu(), self.terminar), None,
                      (_("Train"), Iconos.Entrenar(), self.entrenar), None,
                      (_("New"), Iconos.TutorialesCrear(), self.nuevo), None,
                      (_("Modify"), Iconos.Modificar(), self.modificar), None,
                      (_("Remove"), Iconos.Borrar(), self.borrar), None,
                      (_("Copy"), Iconos.Copiar(), self.copiar), None,
                      (_("Up"), Iconos.Arriba(), self.arriba), None,
                      (_("Down"), Iconos.Abajo(), self.abajo), None,
                      ("+" + _("Openings"), Iconos.Recuperar(), self.masaperturas), None,
                      (_("Book"), Iconos.Libros(), self.polyglot, _("Create a polyglot book")), None
                      ]
        tb = Controles.TBrutina(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NOMBRE", _("Name"), 240)
        oColumnas.nueva("BLANCAS", _("White"), 120, siCentrado=True)
        oColumnas.nueva("NEGRAS", _("Black"), 120, siCentrado=True)
        oColumnas.nueva("AMBOS", _("White & Black"), 120, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)
        n = self.grid.anchoColumnas()
        self.grid.setMinimumWidth(n + 20)
        self.registrarGrid(self.grid)
        self.grid.gotop()

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).margen(8)
        self.setLayout(layout)

        self.recuperarVideo(siTam=True)

        self.siReverse = False

    def terminar(self):
        self.guardarVideo()
        self.reject()
        return

    def masaperturas(self):
        w = AperturasPersonales(self.procesador, self)
        w.exec_()
        AperturasStd.reset()
        self.listaAperturasStd = AperturasStd.apTrain

    def polyglot(self):
        recno = self.grid.recno()
        if recno < 0:
            return
        reg = self.lista[recno]

        # Pedimos el fichero a generar
        fbin = QTUtil2.salvaFichero(self, _("Polyglot book"), reg["NOMBRE"] + ".bin", _("Polyglot book") + " (*.bin)")
        if not fbin:
            return

        me = QTUtil2.unMomento(self)

        # Determinamos el fichero de trabajo
        plTMP = self.procesador.configuracion.ficheroTemporal("deleteme_%d.pgn")
        n = 0
        while True:
            fichTMP = plTMP % n
            if Util.existeFichero(fichTMP):
                n += 1
            else:
                break

        # Creamos el fichero de trabajo
        f = open(fichTMP, "wb")
        for pv in reg["LISTA"]:
            f.write('[Result "1/2-1/2"]\n')
            p = Partida.Partida()
            p.leerPV(pv)
            f.write(p.pgnBase() + " 1/2-1/2\n\n")
        f.close()

        # Ejecutamos
        if VarGen.isWindows:
            exe = 'Engines/Windows/_tools/polyglot/polyglot.exe'
        else:
            exe = '%s/_tools/polyglot/polyglot' % VarGen.folder_engines
        li = [os.path.abspath(exe),
              'make-book',
              "-pgn", fichTMP,
              "-bin", fbin,
              "-max-ply", "99",
              "-min-game", "1",
              "-uniform"]
        Util.borraFichero(fbin)
        process = subprocess.Popen(li, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        me.final()

        # Mostramos el resultado
        txt = process.stdout.read()
        if os.path.isfile(fbin):
            txt += "\n" + _X(_("Book created : %1"), fbin)

        QTUtil2.mensaje(self, txt)

        Util.borraFichero(fichTMP)

    def gridNumDatos(self, grid):
        return len(self.lista)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        reg = self.lista[fila]
        if clave == "NOMBRE":
            return reg["NOMBRE"]

        return "%d" % reg[clave]

    def entrenar(self, fil=None, col=None):
        if len(self.lista) == 0:
            return
        if fil is None:
            fil = self.grid.recno()

        # Ultimo entrenamiento
        dicPar = Util.recuperaVar(self.ficheroParam)
        if dicPar is None:
            jugamos = "BLANCAS"
            repeticiones = 5
        else:
            jugamos = dicPar["JUGAMOS"]
            repeticiones = dicPar["REPETICIONES"]
        if not ((col is None) or (col.clave == "NOMBRE")):
            jugamos = col.clave

        # Datos
        liGen = [(None, None)]

        liJ = [(_("White"), "BLANCAS"), (_("Black"), "NEGRAS"), (_("White & Black"), "AMBOS")]
        config = FormLayout.Combobox(_("Play with"), liJ)
        liGen.append((config, jugamos))

        liR = [(_("Undefined"), 0)]

        for x in range(4):
            liR.append((str(x + 1), x + 1))

        for x in range(5, 105, 5):
            liR.append((str(x), x))
        config = FormLayout.Combobox(_("Model"), liR)
        liGen.append((config, repeticiones))

        # Editamos
        resultado = FormLayout.fedit(liGen, title=_("Train"), parent=self, anchoMinimo=360, icon=Iconos.Entrenar())
        if resultado is None:
            return

        accion, liResp = resultado
        jugamos = liResp[0]
        repeticiones = liResp[1]

        dicPar = {}
        dicPar["JUGAMOS"] = jugamos
        dicPar["REPETICIONES"] = repeticiones
        Util.guardaVar(self.ficheroParam, dicPar)

        self.resultado = (self.listaAperturasStd, self.ficheroDatos, self.lista, fil, jugamos, repeticiones)
        self.accept()

    def gridDobleClick(self, grid, fil, col):
        self.entrenar(fil, col)

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave
        if clave != "NOMBRE":
            return

        li = sorted(self.lista, key=lambda x: x['NOMBRE'].upper())

        if self.siReverse:
            li.reverse()

        self.siReverse = not self.siReverse

        self.lista = li

        self.grid.refresh()
        self.grid.gotop()

        self.grabar()

    def editar(self, fila):
        reg = {} if fila is None else self.lista[fila]
        w = EntrenamientoApertura(self, self.listaAperturasStd, reg)
        if w.exec_():
            reg = {}
            reg["NOMBRE"] = w.nombre
            reg["LISTA"] = w.listaPV()
            reg["BLANCAS"] = 0
            reg["NEGRAS"] = 0
            reg["AMBOS"] = 0

            if fila is None:
                self.lista.append(reg)
                self.grid.gobottom()
            else:
                self.lista[fila] = reg
            self.grid.refresh()
            self.grabar()

    def nuevo(self):
        self.editar(None)

    def modificar(self):
        recno = self.grid.recno()
        if recno >= 0:
            self.editar(recno)

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            if QTUtil2.pregunta(self, _X(_("Do you want to delete training %1?"), self.lista[fila]["NOMBRE"])):
                del self.lista[fila]
                self.grid.refresh()
                self.grabar()

    def copiar(self):
        fila = self.grid.recno()
        if fila >= 0:
            reg = self.lista[fila]
            nreg = copy.deepcopy(reg)
            self.lista.append(nreg)
            self.grid.refresh()
            self.grabar()

    def leer(self):
        lista = Util.recuperaVar(self.ficheroDatos)
        if lista is None:
            lista = []
        return lista

    def grabar(self):
        Util.guardaVar(self.ficheroDatos, self.lista)

    def arriba(self):
        fila = self.grid.recno()
        if fila > 0:
            self.lista[fila - 1], self.lista[fila] = self.lista[fila], self.lista[fila - 1]
            self.grid.goto(fila - 1, 0)
            self.grid.refresh()
            self.grid.setFocus()
            self.grabar()

    def abajo(self):
        fila = self.grid.recno()
        if 0 <= fila < (len(self.lista) - 1):
            self.lista[fila + 1], self.lista[fila] = self.lista[fila], self.lista[fila + 1]
            self.grid.goto(fila + 1, 0)
            self.grid.refresh()
            self.grid.setFocus()
            self.grabar()


class AperturasPersonales(QTVarios.WDialogo):
    def __init__(self, procesador, owner=None):

        self.procesador = procesador
        self.ficheroDatos = procesador.configuracion.ficheroPersAperturas
        self.lista = self.leer()

        if owner is None:
            owner = procesador.pantalla
        icono = Iconos.Apertura()
        titulo = _("Custom openings")
        extparam = "customopen"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        tb = Controles.TBrutina(self)
        tb.new(_("Close"), Iconos.MainMenu(), self.terminar)
        tb.new(_("New"), Iconos.TutorialesCrear(), self.nuevo)
        tb.new(_("Modify"), Iconos.Modificar(), self.modificar)
        tb.new(_("Remove"), Iconos.Borrar(), self.borrar)
        tb.new(_("Copy"), Iconos.Copiar(), self.copiar)
        tb.new(_("Up"), Iconos.Arriba(), self.arriba)
        tb.new(_("Down"), Iconos.Abajo(), self.abajo)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NOMBRE", _("Name"), 240)
        oColumnas.nueva("ECO", "ECO", 70, siCentrado=True)
        oColumnas.nueva("PGN", "PGN", 280)
        oColumnas.nueva("ESTANDAR", _("Add to standard list"), 120, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)
        n = self.grid.anchoColumnas()
        self.grid.setMinimumWidth(n + 20)
        self.registrarGrid(self.grid)
        self.grid.gotop()

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).margen(8)
        self.setLayout(layout)

        self.recuperarVideo(siTam=True)

        self.dicPGNSP = {}

    def terminar(self):
        self.guardarVideo()
        self.reject()
        return

    def gridNumDatos(self, grid):
        return len(self.lista)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        reg = self.lista[fila]
        if clave == "ESTANDAR":
            return _("Yes") if reg["ESTANDAR"] else _("No")
        elif clave == "PGN":
            pgn = reg["PGN"]
            if pgn not in self.dicPGNSP:
                p = Partida.Partida()
                p.leerPV(reg["A1H8"])
                self.dicPGNSP[pgn] = p.pgnSP()
            return self.dicPGNSP[pgn]
        return reg[clave]

    def gridDobleClick(self, grid, fil, col):
        self.editar(fil)

    def gridDobleClickCabecera(self, grid, oColumna):
        clave = oColumna.clave

        li = sorted(self.lista, key=lambda x: x[clave].upper())

        self.lista = li

        self.grid.refresh()
        self.grid.gotop()

        self.grabar()

    def editar(self, fila):

        if fila is None:
            nombre = ""
            eco = ""
            pgn = ""
            estandar = True
            titulo = _("New opening")

        else:
            reg = self.lista[fila]

            nombre = reg["NOMBRE"]
            eco = reg["ECO"]
            pgn = reg["PGN"]
            estandar = reg["ESTANDAR"]

            titulo = nombre

        # Datos
        liGen = [(None, None)]
        liGen.append((_("Name") + ":", nombre))
        config = FormLayout.Editbox("ECO", ancho=30, rx="[A-Z, a-z][0-9][0-9]")
        liGen.append((config, eco))
        liGen.append((_("Add to standard list") + ":", estandar))

        # Editamos
        resultado = FormLayout.fedit(liGen, title=titulo, parent=self, anchoMinimo=460, icon=Iconos.Apertura())
        if resultado is None:
            return

        accion, liResp = resultado
        nombre = liResp[0].strip()
        if not nombre:
            return
        eco = liResp[1].upper()
        estandar = liResp[2]

        fen = ControlPosicion.FEN_INICIAL

        self.procesador.procesador = self.procesador  # ya que editaVariante espera un gestor

        resp = Variantes.editaVariante(self.procesador, self.procesador, fen, pgn, titulo=nombre, siBlancasAbajo=True)

        if resp:
            pgn, a1h8 = resp

            reg = {}
            reg["NOMBRE"] = nombre
            reg["ECO"] = eco
            reg["PGN"] = pgn
            reg["A1H8"] = a1h8
            reg["ESTANDAR"] = estandar

            if fila is None:
                self.lista.append(reg)
                self.grid.refresh()
                self.grabar()
            else:
                self.lista[fila] = reg
            self.grid.refresh()
            self.grabar()

    def nuevo(self):
        self.editar(None)

    def modificar(self):
        recno = self.grid.recno()
        if recno >= 0:
            self.editar(recno)

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            if QTUtil2.pregunta(self, _X(_("Do you want to delete the opening %1?"), self.lista[fila]["NOMBRE"])):
                del self.lista[fila]
                self.grid.refresh()
                self.grabar()

    def copiar(self):
        fila = self.grid.recno()
        if fila >= 0:
            reg = self.lista[fila]
            nreg = copy.deepcopy(reg)
            self.lista.append(nreg)
            self.grid.refresh()
            self.grabar()

    def leer(self):
        lista = Util.recuperaVar(self.ficheroDatos)
        if lista is None:
            lista = []

        return lista

    def grabar(self):
        Util.guardaVar(self.ficheroDatos, self.lista)

    def arriba(self):
        fila = self.grid.recno()
        if fila > 0:
            self.lista[fila - 1], self.lista[fila] = self.lista[fila], self.lista[fila - 1]
            self.grid.goto(fila - 1, 0)
            self.grid.refresh()
            self.grid.setFocus()
            self.grabar()

    def abajo(self):
        fila = self.grid.recno()
        if 0 <= fila < (len(self.lista) - 1):
            self.lista[fila + 1], self.lista[fila] = self.lista[fila], self.lista[fila + 1]
            self.grid.goto(fila + 1, 0)
            self.grid.refresh()
            self.grid.setFocus()
            self.grabar()
