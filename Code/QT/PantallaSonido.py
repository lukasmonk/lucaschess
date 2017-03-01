import collections
import os
import time

from PyQt4 import QtCore, QtGui

from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import TabElementos
from Code.QT import TabTipos
from Code import Sonido
from Code import TrListas
from Code import Util
from Code import VarGen


class MesaSonido(QtGui.QGraphicsView):
    def __init__(self, parent):
        QtGui.QGraphicsView.__init__(self)

        self.setRenderHint(QtGui.QPainter.Antialiasing)
        self.setRenderHint(QtGui.QPainter.TextAntialiasing)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setTransformationAnchor(self.NoAnchor)
        self.escena = QtGui.QGraphicsScene(self)
        self.escena.setItemIndexMethod(self.escena.NoIndex)
        self.setScene(self.escena)

        self.pantalla = parent
        self.crea()

    def crea(self):
        # base
        cajon = TabTipos.Caja()
        cajon.posicion.orden = 1
        cajon.colorRelleno = 4294243572
        ancho, alto = 544, 100
        cajon.posicion.ancho, cajon.posicion.alto = ancho, alto
        cajon.posicion.x, cajon.posicion.y = 0, 0
        cajon.tipo = QtCore.Qt.NoPen

        self.cajonSC = TabElementos.CajaSC(self.escena, cajon)

        def tiempoSC(x, y, linea, minim, maxim, rutina, color):
            tt = TabTipos.Texto()
            tt.tipoLetra = TabTipos.TipoLetra("Courier New", 10)
            tt.posicion.ancho = 68
            tt.posicion.alto = 12
            tt.posicion.orden = 5
            tt.colorFondo = color
            tt.valor = "00:00:00"
            tt.posicion.x = x
            tt.posicion.y = y
            tt.linea = linea
            tt.min = x + minim
            tt.max = x + maxim
            tt.rutina = rutina
            sc = TabElementos.TiempoSC(self.escena, tt)
            sc.ponCentesimas(0)
            return sc

        self.txtInicio = tiempoSC(2, 40, "d", 0, 400, self.mv_inicio, 4294242784)
        self.txtFinal = tiempoSC(ancho - 6 - 68, 40, "i", -400, 0, self.mv_final, 4294242784)
        self.txtActual = tiempoSC(68 + 2, 10, "a", 0, 400, None, 4294242784)
        self.txtDuracion = tiempoSC(236, 78, None, 0, 0, None, 4289509046)

        tf = TabTipos.Caja()
        tf.posicion.orden = 2
        tf.posicion.x = 70
        tf.posicion.y = 44
        tf.posicion.ancho = 400
        tf.posicion.alto = 4
        tf.grosor = 0
        tf.colorRelleno = 4281413888
        self.linMain = TabElementos.CajaSC(self.escena, tf)

        tf = TabTipos.Caja()
        tf.posicion.orden = 2
        tf.posicion.x = 70
        tf.posicion.y = 32
        tf.posicion.ancho = 0
        tf.posicion.alto = 26
        tf.colorRelleno = 4281413888
        TabElementos.CajaSC(self.escena, tf)

        tf = TabTipos.Caja()
        tf.posicion.orden = 2
        tf.posicion.x = 470
        tf.posicion.y = 32
        tf.posicion.ancho = 0
        tf.posicion.alto = 26
        tf.colorRelleno = 4281413888
        TabElementos.CajaSC(self.escena, tf)

        self.setFixedSize(ancho, alto)

    def mv_inicio(self, posx):
        self.txtFinal.minimo = self.txtFinal.inicialx - 400 + posx
        self.txtActual.minimo = self.txtActual.inicialx + posx
        self.txtActual.compruebaPos()

        self.txtDuracion.ponCentesimas(self.txtFinal.calcCentesimas() - self.txtInicio.calcCentesimas())

    def mv_final(self, posx):
        self.txtInicio.maximo = self.txtInicio.inicialx + 400 + posx
        self.txtActual.maximo = self.txtActual.inicialx + 400 + posx
        self.txtActual.compruebaPos()

        self.txtDuracion.ponCentesimas(self.txtFinal.calcCentesimas() - self.txtInicio.calcCentesimas())

    def ponCentesimas(self, centesimas):
        for x in (self.txtActual, self.txtDuracion, self.txtFinal, self.txtInicio):
            x.ponCentesimas(centesimas)
            if centesimas == 0:
                x.posInicial()
        self.escena.update()

    def ponCentesimasActual(self, centesimas):
        self.txtActual.ponPosicion(centesimas)
        self.escena.update()

    def limites(self, siTotal):
        hasta = self.txtFinal.calcCentesimas()
        if siTotal:
            desde = self.txtInicio.calcCentesimas()
        else:
            desde = self.txtActual.calcCentesimas()
        return desde, hasta

    def siHayQueRecortar(self):
        return self.txtInicio.siMovido() or self.txtFinal.siMovido()

    def activaEdicion(self, siActivar):
        for x in (self.txtActual, self.txtFinal, self.txtInicio):
            x.activa(siActivar)


class WEdicionSonido(QTVarios.WDialogo):
    ks_aceptar, ks_cancelar, ks_microfono, ks_wav, ks_play, ks_stopplay, \
    ks_stopmic, ks_record, ks_cancelmic, ks_limpiar, ks_grabar = range(11)

    def __init__(self, owner, titulo, wav=None, maxTime=None, nombre=None):

        # titulo = _("Sound edition" )
        icono = Iconos.S_Play()
        extparam = "sound"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.confich = VarGen.configuracion.ficheroDirSound

        # toolbar
        self.tb = QtGui.QToolBar("BASICO", self)
        self.tb.setIconSize(QtCore.QSize(32, 32))
        self.tb.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)
        self.preparaTB()

        # Nombre
        siNombre = nombre is not None
        if siNombre:
            lbNom = Controles.LB(self, _("Name") + ":")
            self.edNom = Controles.ED(self, nombre).anchoMinimo(180)
            lyNom = Colocacion.H().control(lbNom).control(self.edNom).relleno()

        # MesaSonido
        self.mesa = MesaSonido(self)
        self.taller = Sonido.TallerSonido(wav)

        self.mesa.ponCentesimas(self.taller.centesimas)

        self.maxTime = maxTime if maxTime else 300.0  # segundos=5 minutos

        layout = Colocacion.V().control(self.tb)
        if siNombre:
            layout.otro(lyNom)
        layout.control(self.mesa).margen(3)

        self.setLayout(layout)

        self.ponBaseTB()

        self.recuperarVideo(siTam=False)

    def nombre(self):  # Para los guiones
        return self.edNom.texto()

    def closeEvent(self, event):  # Cierre con X
        self.siGrabando = False
        self.siCancelado = True
        self.guardarVideo()

    def ponBaseTB(self):
        li = [self.ks_aceptar, self.ks_cancelar, None]
        if self.taller.siDatos():
            li.extend([self.ks_limpiar, None, self.ks_play, None, self.ks_grabar])
            self.mesa.activaEdicion(True)
        else:
            li.extend([self.ks_microfono, None, self.ks_wav])
            self.mesa.activaEdicion(False)
        self.ponToolBar(li)

    def preparaTB(self):

        self.dicTB = {}

        liOpciones = ((_("Accept"), Iconos.S_Aceptar(), self.ks_aceptar),
                      (_("Cancel"), Iconos.S_Cancelar(), self.ks_cancelar),
                      (_("Recording Mic"), Iconos.S_Microfono(), self.ks_microfono),
                      (_("Read wav"), Iconos.S_LeerWav(), self.ks_wav),
                      (_("Listen"), Iconos.S_Play(), self.ks_play),
                      (_("End"), Iconos.S_StopPlay(), self.ks_stopplay),
                      (_("End"), Iconos.S_StopMicrofono(), self.ks_stopmic),
                      (_("Begin"), Iconos.S_Record(), self.ks_record),
                      (_("Cancel"), Iconos.S_Cancelar(), self.ks_cancelmic),
                      (_("Delete"), Iconos.S_Limpiar(), self.ks_limpiar),
                      (_("Save wav"), Iconos.Grabar(), self.ks_grabar),
                      )

        for titulo, icono, clave in liOpciones:
            accion = QtGui.QAction(titulo, None)
            accion.setIcon(icono)
            accion.setIconText(titulo)
            self.connect(accion, QtCore.SIGNAL("triggered()"), self.procesaTB)
            accion.clave = clave
            self.dicTB[clave] = accion

    def ponToolBar(self, liAcciones):

        self.tb.clear()
        for k in liAcciones:
            if k is None:
                self.tb.addSeparator()
            else:
                self.dicTB[k].setVisible(True)
                self.dicTB[k].setEnabled(True)
                self.tb.addAction(self.dicTB[k])

        self.tb.liAcciones = liAcciones
        self.tb.update()

    def habilitaTB(self, kopcion, siHabilitar):
        self.dicTB[kopcion].setEnabled(siHabilitar)

    def procesaTB(self):
        accion = self.sender().clave
        if accion == self.ks_aceptar:
            if self.mesa.siHayQueRecortar():
                desde, hasta = self.mesa.limites(True)
                self.taller.recorta(desde, hasta)
            self.wav = self.taller.wav
            self.centesimas = self.taller.centesimas
            self.accept()
            self.guardarVideo()
            return
        elif accion == self.ks_cancelar:
            self.reject()
            return

        elif accion == self.ks_limpiar:
            self.limpiar()

        elif accion == self.ks_microfono:
            self.microfono()
        elif accion == self.ks_cancelmic:
            self.siGrabando = False
            self.siCancelado = True
            self.ponBaseTB()

        elif accion == self.ks_record:
            self.micRecord()
        elif accion == self.ks_stopmic:
            self.siGrabando = False
        elif accion == self.ks_wav:
            self.wav()
        elif accion == self.ks_grabar:
            self.grabar()
        elif accion == self.ks_play:
            self.play()
        elif accion == self.ks_stopplay:
            self.siPlay = False

    def microfono(self):
        self.ponToolBar((self.ks_cancelmic, None, self.ks_record))

    def micRecord(self):
        self.ponToolBar((self.ks_cancelmic, None, self.ks_stopmic))
        self.siGrabando = True
        self.siCancelado = False

        self.mesa.ponCentesimas(0)

        self.taller.micInicio()

        iniTime = time.time()

        while self.siGrabando:
            self.taller.micGraba()
            QTUtil.refreshGUI()
            t = time.time() - iniTime
            self.mesa.ponCentesimas(t * 100)
            if t > self.maxTime:
                break

        self.siGrabando = False
        self.taller.micFinal()
        if self.siCancelado:
            self.taller.limpiar()
            self.mesa.ponCentesimas(0)
        else:
            self.mesa.ponCentesimas(self.taller.centesimas)

        self.ponBaseTB()

    def limpiar(self):
        self.taller.limpiar()
        self.mesa.ponCentesimas(0)
        self.ponBaseTB()

    def wav(self):
        carpeta = Util.recuperaVar(self.confich)
        fichero = QTUtil2.leeFichero(self, carpeta, "wav")
        if fichero:
            carpeta = os.path.dirname(fichero)
            Util.guardaVar(self.confich, carpeta)
            if self.taller.leeWAV(fichero):
                self.mesa.ponCentesimas(self.taller.centesimas)
            else:
                QTUtil2.mensError(self, _("It is impossible to read this file, it is not compatible."))
            self.ponBaseTB()

    def grabar(self):
        carpeta = Util.recuperaVar(self.confich)
        filtro = _("File") + " wav (*.wav)"
        fichero = QTUtil2.salvaFichero(self, _("Save wav"), carpeta, filtro, siConfirmarSobreescritura=True)
        if fichero:
            carpeta = os.path.dirname(fichero)
            Util.guardaVar(self.confich, carpeta)
            f = open(fichero, "wb")
            f.write(self.taller.wav)
            f.close()
            self.ponBaseTB()

    def play(self):
        self.mesa.activaEdicion(False)
        self.ponToolBar((self.ks_stopplay,))

        centDesde, centHasta = self.mesa.limites(False)
        self.taller.playInicio(centDesde, centHasta)

        self.siPlay = True

        while self.siPlay:
            siSeguir, centActual = self.taller.play()
            if siSeguir:
                self.mesa.ponCentesimasActual(centActual)
                QTUtil.refreshGUI()
            else:
                self.mesa.ponCentesimasActual(centDesde)
                QTUtil.refreshGUI()
                break

        self.siPlay = False

        self.taller.playFinal()

        self.ponBaseTB()


def editaSonido(owner, titulo, wav):
    w = WEdicionSonido(owner, titulo, wav)
    if w.exec_():
        return w.wav, w.centesimas
    else:
        return None


class WSonidos(QTVarios.WDialogo):
    def __init__(self, procesador):

        self.procesador = procesador

        self.db = Util.DicBLOB(procesador.configuracion.ficheroSounds, "general")
        self.creaListaSonidos()

        titulo = _("Custom sounds")
        icono = Iconos.S_Play()
        extparam = "sounds"
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)

        # Toolbar
        liAcciones = ((_("Close"), Iconos.MainMenu(), "terminar"), None,
                      (_("Modify"), Iconos.Modificar(), "modificar"), None,
                      (_("Listen"), Iconos.S_Play(), "play"),
                      )
        tb = Controles.TB(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("SONIDO", _("Sound"), 300, siCentrado=True)
        oColumnas.nueva("DURACION", _("Duration"), 60, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).margen(3)
        self.setLayout(layout)

        self.grid.gotop()
        self.grid.setFocus()

        self.siPlay = False

        self.registrarGrid(self.grid)

        if not self.recuperarVideo():
            self.resize(self.grid.anchoColumnas() + 30, 600)

    def closeEvent(self, event):
        self.guardarVideo()

    def procesarTB(self):
        self.siPlay = False
        accion = self.sender().clave
        if accion == "terminar":
            self.guardarVideo()
            self.accept()
            self.db.close()

        elif accion == "modificar":
            self.gridDobleClick(None, self.grid.recno(), None)

        elif accion == "play":
            self.play()

    def gridNumDatos(self, grid):
        return len(self.liSonidos)

    def gridDobleClick(self, grid, fila, oColumna):
        self.siPlay = False

        cl = self.liSonidos[fila][0]
        if cl is None:
            return

        wav = self.db[cl]

        resp = editaSonido(self, self.liSonidos[fila][1], wav)
        if resp is not None:
            wav, cent = resp
            if wav is None:
                self.liSonidos[fila][2] = None
                del self.db[cl]
            else:
                self.db[cl] = wav
                self.liSonidos[fila][2] = cent
            self.grid.refresh()

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        li = self.liSonidos[fila]
        if clave == "DURACION":
            if li[0] is None:
                return ""
            else:
                t = li[2]
                if t is None:

                    wav = self.db[li[0]]
                    if wav is None:
                        t = 0
                    else:
                        ts = Sonido.TallerSonido(wav)
                        t = ts.centesimas
                return "%02d:%02d:%02d" % Sonido.msc(t)

        else:
            return li[1]

    def play(self):
        li = self.liSonidos[self.grid.recno()]
        if li[0]:
            wav = self.db[li[0]]
            if wav is not None:
                ts = Sonido.TallerSonido(wav)
                ts.playInicio(0, ts.centesimas)
                self.siPlay = True
                while self.siPlay:
                    siSeguir, pos = ts.play()
                    if not siSeguir:
                        break
                ts.playFinal()

    def gridColorFondo(self, grid, fila, oColumna):
        li = self.liSonidos[fila]
        if li[0] is None:
            return QTUtil.qtColor(4294836181)
        else:
            return None

    def creaListaSonidos(self):

        self.liSonidos = [
            ["MC", _("After rival move"), None],
        ]

        # self.liSonidos.append( [ None, "", None ] )
        self.liSonidos.append([None, "- " + _("Results") + " -", None])

        d = collections.OrderedDict()
        d["GANAMOS"] = _("You win")
        d["GANARIVAL"] = _("Opponent wins")
        d["TABLAS"] = _("Stalemate")
        d["TABLASREPETICION"] = _("Draw due to three times repetition")
        d["TABLAS50"] = _("Draw according to the 50 move rule")
        d["TABLASFALTAMATERIAL"] = _("Draw, not enough material")
        d["GANAMOSTIEMPO"] = _("You win on time")
        d["GANARIVALTIEMPO"] = _("Opponent has won on time")

        for c, tr in d.iteritems():
            self.liSonidos.append([c, tr, None])

        # self.liSonidos.append( [ None, "", None ] )
        self.liSonidos.append([None, "- " + _("Rival moves") + " -", None])

        for c in "abcdefgh12345678":
            self.liSonidos.append([c, c, None])

        for c in "KQRBNP":
            t = TrListas.letterPiece(c)
            self.liSonidos.append([c, t, None])

        for c in ("O-O", "O-O-O", "=", "x", "#", "+"):
            self.liSonidos.append([c, c, None])

        self.liSonidos.append([None, "", None])
        self.liSonidos.append(["ZEITNOT", _("Zeitnot"), None])

        # for c in "abcdefgh":
        # for f in "12345678":
        # self.liSonidos.append( [ c+f, c+f, None ] )


class WSonidosGuion(QTVarios.WDialogo):
    def __init__(self, owner, db):

        self.db = db

        self.liSonidos = owner.listaSonidos()

        titulo = _("Custom sounds")
        icono = Iconos.S_Play()
        extparam = "sounds"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        # Toolbar
        liAcciones = ((_("Close"), Iconos.MainMenu(), "terminar"), None,
                      (_("New"), Iconos.Nuevo(), "nuevo"),
                      (_("Modify"), Iconos.Modificar(), "modificar"), None,
                      (_("Remove"), Iconos.Borrar(), "borrar"), None,
                      (_("Up"), Iconos.Arriba(), "arriba"),
                      (_("Down"), Iconos.Abajo(), "abajo"), None,
                      (_("Listen"), Iconos.S_Play(), "play"),
                      )
        tb = Controles.TB(self, liAcciones)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NOMBRE", _("Sound"), 300, siCentrado=True)
        oColumnas.nueva("DURACION", _("Duration"), 60, siCentrado=True)

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).margen(3)
        self.setLayout(layout)

        self.grid.gotop()
        self.grid.setFocus()

        self.siPlay = False

        self.registrarGrid(self.grid)

        if not self.recuperarVideo():
            self.resize(self.grid.anchoColumnas() + 30, 600)

    def closeEvent(self, event):
        self.guardarVideo()

    def gridNumDatos(self, grid):
        return len(self.liSonidos)

    def procesarTB(self):
        accion = self.sender().clave
        eval("self.%s()" % accion)

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def modificar(self):
        pass
        # self.gridDobleClick( None, self.grid.recno(), None )

    def nuevo(self):
        wav = None
        nombre = ""
        while True:
            w = WEdicionSonido(self, _("New"), wav=wav, maxTime=600000, nombre=nombre)
            resp = w.exec_()
            if resp:
                centesimas = w.centesimas
                if not centesimas:
                    return
                nombre = w.nombre().strip()
                if not nombre:
                    QTUtil2.mensError(self, _("Name missing"))
                    continue

                reg = Util.Almacen()
                reg.nombre = nombre
                reg.centesimas = centesimas
                reg.id = Util.nuevoID()
                reg.wav = w.wav
                reg.ordenVista = (self.liSonidos[-1].ordenVista + 1) if self.liSonidos else 1
                self.db[reg.id] = reg
                self.liSonidos.append(reg)
                self.grid.refresh()
                return

    def gridDobleClick(self, grid, fila, oColumna):
        self.siPlay = False

        cl = self.db[fila][0]
        if cl is None:
            return

        wav = self.db[cl]

        resp = editaSonido(self, self.liSonidos[fila][1], wav)
        if resp is not None:
            wav, cent = resp
            self.db[cl] = wav
            self.liSonidos[fila][2] = cent
            self.grid.refresh()

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        reg = self.liSonidos[fila]
        if clave == "DURACION":
            return "%02d:%02d:%02d" % Sonido.msc(reg.centesimas)
        elif clave == "NOMBRE":
            return reg.nombre

    def play(self):
        if self.grid.recno() >= 0:
            reg = self.db[self.grid.recno()]
            wav = reg.wav
            ts = Sonido.TallerSonido(wav)
            ts.playInicio(0, ts.centesimas)
            self.siPlay = True
            while self.siPlay:
                siSeguir, pos = ts.play()
                if not siSeguir:
                    break
            ts.playFinal()
