import base64
import os

from PyQt4 import QtCore, QtGui

from Code import ControlPosicion
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import PantallaTabVFlechas
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import TabFlechas
from Code.QT import Tablero
from Code import TrListas
from Code import Util
from Code import VarGen


class BotonTema(QtGui.QPushButton):
    def __init__(self, parent, rutina):
        QtGui.QPushButton.__init__(self, parent)

        self.setFixedSize(64, 64)
        self.qs = QtCore.QSize(64, 64)
        self.setIconSize(self.qs)

        self.rutina = rutina
        self.tema = None

        self.connect(self, QtCore.SIGNAL("clicked()"), self.pulsado)

    def ponTema(self, tema):
        self.setVisible(tema is not None)
        self.tema = tema
        if not tema:
            return
        self.setToolTip(tema["NOMBRE"])
        self.setIcon(QTVarios.iconoTema(tema, 64))

    def pulsado(self):
        self.rutina(self.tema)


class BotonColor(QtGui.QPushButton):
    def __init__(self, parent, rut_actual, rut_actualiza):
        QtGui.QPushButton.__init__(self, parent)

        self.setFixedSize(32, 32)

        self.rut_actual = rut_actual
        self.rut_actualiza = rut_actualiza
        self.connect(self, QtCore.SIGNAL("clicked()"), self.pulsado)

        self.parent = parent

        self.ponColor()

    def ponColor(self):
        ncolor = self.rut_actual()
        self.setStyleSheet("QWidget { background: %s }" % QTUtil.qtColor(ncolor).name())

    def pulsado(self):
        ncolor = self.rut_actual()
        color = QTUtil.qtColor(ncolor)
        color = QtGui.QColorDialog.getColor(color, self.parent, _("Choose a color"))
        if color.isValid():
            self.rut_actual(color.rgba())
            self.rut_actualiza()
            self.ponColor()


class BotonImagen(Colocacion.H):
    def __init__(self, parent, rut_actual, rut_actualiza, bt_asociado):
        Colocacion.H.__init__(self)
        self.width = 32
        self.height = 32
        self.btImagen = Controles.PB(parent, "", self.cambiar)
        self.btImagen.setFixedSize(self.width, self.height)
        self.btQuitar = Controles.PB(parent, "", self.quitaImagen).ponIcono(Iconos.Motor_No())
        self.bt_asociado = bt_asociado
        self.parent = parent

        self.rut_actual = rut_actual
        self.rut_actualiza = rut_actualiza

        self.control(self.btImagen)
        self.control(self.btQuitar)

        self.ponImagen()

    def setDisabled(self, si):
        self.btImagen.setDisabled(si)
        self.btQuitar.setDisabled(si)

    def quitaImagen(self):
        self.rut_actual("")
        self.ponImagen()
        self.rut_actualiza()

    def ponImagen(self):
        png64 = self.rut_actual()
        if png64:

            pm = QtGui.QPixmap()
            png = base64.b64decode(png64)

            # import os
            # n = 0
            # while os.path.isfile("mira_%d.png"%n):
            #     n+=1
            # f = open("mira_%d.png"%n,"wb")
            # f.write(png)
            # f.close()

            pm.loadFromData(png)
            icono = QtGui.QIcon(pm)
            self.btImagen.ponPlano(True)
            self.btImagen.ponTexto("")
            self.bt_asociado.hide()
            self.btQuitar.show()
        else:
            icono = QtGui.QIcon()
            self.btImagen.ponPlano(False)
            self.btImagen.ponTexto("?")
            self.bt_asociado.show()
            self.btQuitar.hide()
        self.btImagen.setIcon(icono)
        self.btImagen.setIconSize(QtCore.QSize(self.width, self.height))

    def cambiar(self):
        configuracion = VarGen.configuracion
        dic = configuracion.leeVariables("PantallaColores")
        dirSalvados = dic.get("PNGfolder", "")
        resp = QTUtil2.leeFichero(self.parent, dirSalvados, "%s PNG (*.png)" % _("File"))
        if resp:
            dirSalvados1 = os.path.dirname(resp)
            if dirSalvados != dirSalvados1:
                dic["PNGfolder"] = dirSalvados1
                configuracion.escVariables("PantallaColores", dic)
            f = open(resp, "rb")
            self.rut_actual(base64.b64encode(f.read()))
            f.close()
            self.ponImagen()
            self.rut_actualiza()


class BotonFlecha(Colocacion.H):
    def __init__(self, parent, rut_actual, rut_defecto, rut_actualiza):
        Colocacion.H.__init__(self)
        self.width = 128
        self.height = 32
        self.btFlecha = Controles.PB(parent, "", self.cambiar)
        self.btFlecha.setFixedSize(self.width, self.height)
        self.btQuitar = Controles.PB(parent, "", self.ponDefecto).ponIcono(Iconos.Motor_No())
        self.parent = parent

        self.rut_actual = rut_actual
        self.rut_defecto = rut_defecto
        self.rut_actualiza = rut_actualiza

        self.control(self.btFlecha)
        self.control(self.btQuitar)

        self.ponImagen()

    def setDisabled(self, si):
        self.btFlecha.setDisabled(si)
        self.btQuitar.setDisabled(si)

    def cambiaFlecha(self, nueva):
        self.rut_actual(nueva)
        self.ponImagen()
        self.rut_actualiza()

    def ponDefecto(self):
        self.cambiaFlecha(self.rut_defecto())

    def ponImagen(self):
        bf = self.rut_actual()
        p = bf.posicion
        p.x = 0
        p.y = self.height / 2
        p.ancho = self.width
        p.alto = self.height / 2

        pm = TabFlechas.pixmapArrow(bf, self.width, self.height)
        icono = QtGui.QIcon(pm)
        self.btFlecha.setIcon(icono)
        self.btFlecha.setIconSize(QtCore.QSize(self.width, self.height))

    def cambiar(self):
        w = PantallaTabVFlechas.WTV_Flecha(self.parent, self.rut_actual(), False)
        if w.exec_():
            self.cambiaFlecha(w.regFlecha)


class DialNum(Colocacion.H):
    def __init__(self, parent, rut_actual, rut_actualiza):
        Colocacion.H.__init__(self)

        self.dial = QtGui.QSlider(QtCore.Qt.Horizontal, parent)
        self.dial.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.dial.setTickPosition(QtGui.QSlider.TicksBothSides)
        self.dial.setTickInterval(10)
        self.dial.setSingleStep(1)
        self.dial.setMinimum(0)
        self.dial.setMaximum(99)
        self.connect(self.dial, QtCore.SIGNAL("valueChanged (int)"), self.movido)
        self.lb = QtGui.QLabel(parent)

        self.rut_actual = rut_actual
        self.rut_actualiza = rut_actualiza

        self.control(self.dial)
        self.control(self.lb)

        self.ponValor()

    def ponValor(self):
        nvalor = self.rut_actual()
        self.dial.setValue(nvalor)
        self.lb.setText("%2d%%" % nvalor)

    def movido(self, valor):
        self.rut_actual(valor)
        self.ponValor()
        self.rut_actualiza()


class WColores(QTVarios.WDialogo):
    def __init__(self, tableroOriginal):
        pantalla = tableroOriginal.parent()
        titulo = _("Colors")
        icono = Iconos.EditarColores()
        extparam = "WColores"
        QTVarios.WDialogo.__init__(self, pantalla, titulo, icono, extparam)

        self.tableroOriginal = tableroOriginal
        self.configuracion = VarGen.configuracion
        self.confTablero = tableroOriginal.confTablero.copia(tableroOriginal.confTablero._id)
        self.siBase = tableroOriginal.confTablero._id == "BASE"

        # Temas #############################################################################################################################
        liOpciones = [(_("Your themes"), self.configuracion.ficheroTemas)]
        for entry in Util.listdir("Themes"):
            filename = entry.name
            if filename.endswith("lktheme"):
                ctema = filename[:-8]
                liOpciones.append((ctema, "Themes/" + filename))

        self.cbTemas = Controles.CB(self, liOpciones, liOpciones[0][1]).capturaCambiado(self.cambiadoTema)
        self.lbSecciones = Controles.LB(self, _("Section") + ":")
        self.cbSecciones = Controles.CB(self, [], None).capturaCambiado(self.cambiadoSeccion)

        lyTemas = Colocacion.G()
        self.liBT_Temas = []
        for i in range(12):
            for j in range(6):
                bt = BotonTema(self, self.ponTema)
                lyTemas.control(bt, i, j)
                bt.ponTema(None)
                self.liBT_Temas.append(bt)

        def creaLB(txt):
            return Controles.LB(self, txt + ": ").alinDerecha()

        # Casillas
        lbTrans = Controles.LB(self, _("Degree of transparency"))
        lbPNG = Controles.LB(self, _("Image"))

        # # Blancas
        lbBlancas = creaLB(_("White squares"))
        self.btBlancas = BotonColor(self, self.confTablero.colorBlancas, self.actualizaTablero)
        self.btBlancasPNG = BotonImagen(self, self.confTablero.png64Blancas, self.actualizaTablero, self.btBlancas)
        self.dialBlancasTrans = DialNum(self, self.confTablero.transBlancas, self.actualizaTablero)

        # # Negras
        lbNegras = creaLB(_("Black squares"))
        self.btNegras = BotonColor(self, self.confTablero.colorNegras, self.actualizaTablero)
        self.btNegrasPNG = BotonImagen(self, self.confTablero.png64Negras, self.actualizaTablero, self.btNegras)
        self.dialNegrasTrans = DialNum(self, self.confTablero.transNegras, self.actualizaTablero)

        # Background
        lbFondo = creaLB(_("Background"))
        self.btFondo = BotonColor(self, self.confTablero.colorFondo, self.actualizaTablero)
        self.btFondoPNG = BotonImagen(self, self.confTablero.png64Fondo, self.actualizaTablero, self.btFondo)
        self.chbExtended = Controles.CHB(self, _("Extended to outer border"),
                                         self.confTablero.extendedColor()).capturaCambiado(self, self.extendedColor)

        # Actual
        self.chbTemas = Controles.CHB(self, _("Default"), self.confTablero.siDefTema()).capturaCambiado(self,
                                                                                                        self.defectoTemas)
        if self.siBase:
            self.chbTemas.ponValor(False)
            self.chbTemas.setVisible(False)
        # Exterior
        lbExterior = creaLB(_("Outer Border"))
        self.btExterior = BotonColor(self, self.confTablero.colorExterior, self.actualizaTablero)
        # Texto
        lbTexto = creaLB(_("Coordinates"))
        self.btTexto = BotonColor(self, self.confTablero.colorTexto, self.actualizaTablero)
        # Frontera
        lbFrontera = creaLB(_("Inner Border"))
        self.btFrontera = BotonColor(self, self.confTablero.colorFrontera, self.actualizaTablero)

        # Flechas
        lbFlecha = creaLB(_("Move indicator"))
        self.lyF = BotonFlecha(self, self.confTablero.fTransicion, self.confTablero.flechaDefecto,
                               self.actualizaTablero)
        lbFlechaAlternativa = creaLB(_("Arrow alternative"))
        self.lyFAlternativa = BotonFlecha(self, self.confTablero.fAlternativa,
                                          self.confTablero.flechaAlternativaDefecto, self.actualizaTablero)
        lbFlechaActivo = creaLB(_("Active moves"))
        self.lyFActual = BotonFlecha(self, self.confTablero.fActivo, self.confTablero.flechaActivoDefecto,
                                     self.actualizaTablero)
        lbFlechaRival = creaLB(_("Opponent moves"))
        self.lyFRival = BotonFlecha(self, self.confTablero.fRival, self.confTablero.flechaRivalDefecto,
                                    self.actualizaTablero)

        lyActual = Colocacion.G()
        lyActual.control(self.chbTemas, 0, 0)
        lyActual.controlc(lbPNG, 0, 2).controlc(lbTrans, 0, 3)
        lyActual.controld(lbBlancas, 1, 0).control(self.btBlancas, 1, 1).otroc(self.btBlancasPNG, 1, 2).otroc(
                self.dialBlancasTrans, 1, 3)
        lyActual.controld(lbNegras, 2, 0).control(self.btNegras, 2, 1).otroc(self.btNegrasPNG, 2, 2).otroc(
                self.dialNegrasTrans, 2, 3)
        lyActual.controld(lbFondo, 3, 0).control(self.btFondo, 3, 1).otroc(self.btFondoPNG, 3, 2).control(
                self.chbExtended, 3, 3)
        lyActual.controld(lbExterior, 4, 0).control(self.btExterior, 4, 1)
        lyActual.controld(lbTexto, 5, 0).control(self.btTexto, 5, 1)
        lyActual.controld(lbFrontera, 6, 0).control(self.btFrontera, 6, 1)
        lyActual.controld(lbFlecha, 7, 0).otro(self.lyF, 7, 1, 1, 4)
        lyActual.controld(lbFlechaAlternativa, 8, 0).otro(self.lyFAlternativa, 8, 1, 1, 4)
        lyActual.controld(lbFlechaActivo, 9, 0).otro(self.lyFActual, 9, 1, 1, 4)
        lyActual.controld(lbFlechaRival, 10, 0).otro(self.lyFRival, 10, 1, 1, 4)

        gbActual = Controles.GB(self, _("Active theme"), lyActual)

        lySecciones = Colocacion.H().control(self.lbSecciones).control(self.cbSecciones).relleno()
        ly = Colocacion.V().control(self.cbTemas).otro(lySecciones).otro(lyTemas).control(gbActual).relleno()
        gbTemas = Controles.GB(self, "", ly)
        gbTemas.setFlat(True)

        # mas opciones ##############################################################################################################
        def xDefecto(siDefecto):
            if self.siBase:
                siDefecto = False
            chb = Controles.CHB(self, _("Default"), siDefecto).capturaCambiado(self, self.defectoTableroM)
            if self.siBase:
                chb.setVisible(False)
            return chb

        def l2mas1(lyG, fila, a, b, c):
            if a:
                ly = Colocacion.H().controld(a).control(b)
            else:
                ly = Colocacion.H().control(b)
            lyG.otro(ly, fila, 0).control(c, fila, 1)

        # Coordenadas
        lyG = Colocacion.G()
        # _nCoordenadas
        lbCoordenadas = creaLB(_("Number"))
        liOpciones = [("0", 0), ("4", 4), ("2a", 2), ("2b", 3), ("2c", 5), ("2d", 6)]
        self.cbCoordenadas = Controles.CB(self, liOpciones, self.confTablero.nCoordenadas()).capturaCambiado(self.actualizaTableroM)
        self.chbDefCoordenadas = xDefecto(self.confTablero.siDefCoordenadas())
        l2mas1(lyG, 0, lbCoordenadas, self.cbCoordenadas, self.chbDefCoordenadas)

        # _tipoLetra
        lbTipoLetra = creaLB(_("Font"))
        self.cbTipoLetra = QtGui.QFontComboBox()
        self.cbTipoLetra.setEditable(False)
        self.cbTipoLetra.setFontFilters(self.cbTipoLetra.ScalableFonts)
        self.cbTipoLetra.setCurrentFont(QtGui.QFont(self.confTablero.tipoLetra()))
        self.connect(self.cbTipoLetra, QtCore.SIGNAL("currentIndexChanged(int)"), self.actualizaTableroM)
        self.chbDefTipoLetra = xDefecto(self.confTablero.siDefTipoLetra())
        l2mas1(lyG, 1, lbTipoLetra, self.cbTipoLetra, self.chbDefTipoLetra)

        # _cBold
        self.chbBold = Controles.CHB(self, _("Bold"), self.confTablero.siBold()).capturaCambiado(self, self.actualizaTableroM)
        self.chbDefBold = xDefecto(self.confTablero.siDefBold())
        l2mas1(lyG, 2, None, self.chbBold, self.chbDefBold)

        # _tamLetra
        lbTamLetra = creaLB(_("Size") + " %")
        self.sbTamLetra = Controles.SB(self, self.confTablero.tamLetra(), 1, 200).tamMaximo(50).capturaCambiado(
                self.actualizaTableroM)
        self.chbDefTamLetra = xDefecto(self.confTablero.siDefTamLetra())
        l2mas1(lyG, 3, lbTamLetra, self.sbTamLetra, self.chbDefTamLetra)

        # _sepLetras
        lbSepLetras = creaLB(_("Separation") + " %")
        self.sbSepLetras = Controles.SB(self, self.confTablero.sepLetras(), -1000, 1000).tamMaximo(50).capturaCambiado(
                self.actualizaTableroM)
        self.chbDefSepLetras = xDefecto(self.confTablero.siDefSepLetras())
        l2mas1(lyG, 4, lbSepLetras, self.sbSepLetras, self.chbDefSepLetras)

        gbCoordenadas = Controles.GB(self, _("Coordinates"), lyG)

        lyOtros = Colocacion.G()
        # _nomPiezas
        li = []
        lbPiezas = creaLB(_("Pieces"))
        for entry in Util.listdir("Pieces"):
            if entry.is_dir():
                li.append((entry.name, entry.name))
        li.sort(key=lambda x: x[0])
        self.cbPiezas = Controles.CB(self, li, self.confTablero.nomPiezas()).capturaCambiado(self.actualizaTableroM)
        self.chbDefPiezas = xDefecto(self.confTablero.siDefPiezas())
        l2mas1(lyOtros, 0, lbPiezas, self.cbPiezas, self.chbDefPiezas)

        # _tamRecuadro
        lbTamRecuadro = creaLB(_("Outer Border Size") + " %")
        self.sbTamRecuadro = Controles.SB(self, self.confTablero.tamRecuadro(), 0, 10000).tamMaximo(50).capturaCambiado(
                self.actualizaTableroM)
        self.chbDefTamRecuadro = xDefecto(self.confTablero.siDefTamRecuadro())
        l2mas1(lyOtros, 1, lbTamRecuadro, self.sbTamRecuadro, self.chbDefTamRecuadro)

        # _tamFrontera
        lbTamFrontera = creaLB(_("Inner Border Size") + " %")
        self.sbTamFrontera = Controles.SB(self, self.confTablero.tamFrontera(), 0, 10000).tamMaximo(50).capturaCambiado(
                self.actualizaTableroM)
        self.chbDefTamFrontera = xDefecto(self.confTablero.siDefTamFrontera())
        l2mas1(lyOtros, 2, lbTamFrontera, self.sbTamFrontera, self.chbDefTamFrontera)

        ly = Colocacion.V().control(gbCoordenadas).espacio(50).otro(lyOtros).relleno()

        gbOtros = Controles.GB(self, "", ly)
        gbOtros.setFlat(True)

        # Tablero ##########################################################################################################################
        cp = ControlPosicion.ControlPosicion().leeFen("2kr1b1r/2p1pppp/p7/3pPb2/1q3P2/2N1P3/PPP3PP/R1BQK2R w KQ - 0 1")
        self.tablero = Tablero.Tablero(self, self.confTablero, siMenuVisual=False)
        self.tablero.crea()
        self.tablero.ponPosicion(cp)
        self.rehazFlechas()

        liAcciones = [(_("Accept"), Iconos.Aceptar(), self.aceptar), None,
                      (_("Cancel"), Iconos.Cancelar(), self.cancelar), None,
                      (_("Your themes"), Iconos.Temas(), self.temas), None,
                      (_("Import"), Iconos.Mezclar(), self.importar), None,
                      (_("Export"), Iconos.Grabar(), self.exportar), None,
                      ]
        tb = QTVarios.LCTB(self, liAcciones)

        # tam tablero
        self.lbTamTablero = Controles.LB(self, "%d px" % self.tablero.width())

        # Juntamos
        lyT = Colocacion.V().control(tb).espacio(15).control(self.tablero).controli(self.lbTamTablero).relleno(
                1).margen(3)

        self.tab = Controles.Tab()
        self.tab.nuevaTab(gbTemas, _("Themes"))
        self.tab.nuevaTab(gbOtros, _("Other options"))
        ly = Colocacion.H().otro(lyT).control(self.tab)

        self.setLayout(ly)

        self.elegido = None

        self.liTemas = self.leeTemas()
        self.temaActual = {}
        if self.liTemas:
            txtTM = self.confTablero.grabaTema()
            txtBS = self.confTablero.grabaBase()
            for tema in self.liTemas:
                if tema:
                    if tema.get("TEXTO", "") == txtTM and txtBS == tema.get("BASE", ""):
                        self.temaActual = tema
                        break
        self.cambiadoTema()
        self.defectoTemas()

        self.extendedColor()

        self.siActualizando = False

        self.recuperarVideo(siTam=False)

    def extendedColor(self):
        siExt = self.chbExtended.valor()
        self.btExterior.setEnabled(not siExt)
        self.confTablero.extendedColor(siExt)

        self.actualizaTablero()

    def rehazFlechas(self):
        self.tablero.quitaFlechas()
        self.tablero.creaFlechaTmp("f2", "f4", True)
        self.tablero.creaFlechaTmp("d1", "d4", False)
        self.tablero.creaFlechaMov("f5", "d7", "ms100")
        self.tablero.creaFlechaMov("d6", "b4", "mt100")

    def cambiadoTema(self):
        fichTema = self.cbTemas.valor()
        self.liTemasTMP = Util.recuperaVar(fichTema)
        if not self.liTemasTMP:
            self.cbTemas.ponValor("Themes/Felicia.lktheme")
            self.cambiadoTema()
        else:
            self.ponSecciones()
            self.cambiadoSeccion()

    def ponSecciones(self):
        previo = self.cbSecciones.valor()
        liOpciones = []
        liFolders = []
        for n, uno in enumerate(self.liTemasTMP):
            if uno:
                if "SECCION" in uno:
                    folder = uno["SECCION"]
                    if folder not in liFolders:
                        liFolders.append(folder)
                        liOpciones.append((folder, folder))

        liOpciones.append((_("All"), None))

        select = previo if previo in liFolders else liOpciones[0][1]
        self.cbSecciones.rehacer(liOpciones, select)
        siVisible = len(liOpciones) > 1
        self.cbSecciones.setVisible(siVisible)
        self.lbSecciones.setVisible(siVisible)

    def cambiadoSeccion(self):
        seccionBusca = self.cbSecciones.valor()
        maxtemas = len(self.liBT_Temas)
        nPos = 0
        for nTema, tema in enumerate(self.liTemasTMP):
            if tema:
                seccion = tema.get("SECCION", None)

                if (seccionBusca is None) or (seccion == seccionBusca):
                    self.liBT_Temas[nPos].ponTema(tema)
                    nPos += 1
                    if nPos == maxtemas:
                        break

        for x in range(nPos, maxtemas):
            self.liBT_Temas[x].ponTema(None)

    def defectoTemas(self):
        siDefecto = self.chbTemas.valor()
        self.confTablero.ponDefTema(siDefecto)
        self.btExterior.setDisabled(siDefecto)

        self.btBlancas.setDisabled(siDefecto)
        self.btBlancasPNG.setDisabled(siDefecto)
        self.dialBlancasTrans.dial.setDisabled(siDefecto)

        self.btNegras.setDisabled(siDefecto)
        self.btNegrasPNG.setDisabled(siDefecto)
        self.dialNegrasTrans.dial.setDisabled(siDefecto)

        self.btTexto.setDisabled(siDefecto)
        self.btFrontera.setDisabled(siDefecto)

        self.lyF.setDisabled(siDefecto)
        self.lyFAlternativa.setDisabled(siDefecto)
        self.lyFActual.setDisabled(siDefecto)
        self.lyFRival.setDisabled(siDefecto)

        self.btFondo.setDisabled(siDefecto)
        self.btFondoPNG.setDisabled(siDefecto)

        self.actualizaTablero()

    def aceptar(self):
        self.confTablero.guardaEnDisco()
        self.tableroOriginal.reset(self.confTablero)

        self.guardarVideo()
        self.accept()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def importar(self):
        # import os
        # ntema = 0
        # self.liTemas = []
        # for carpeta in ("1","a","b"):
        # sc = 1
        # ps = 0
        # for fich in os.listdir("/tmp/rp/%s"%carpeta):
        # tema = Util.recuperaVar( "/tmp/rp/%s/%s"%(carpeta,fich) )
        # tema["NOMBRE"] = fich[:-9]
        # tema["SECCION"] = "%s-%d"%(carpeta,sc)
        # ps += 1
        # if ps == 12:
        # ps =0
        # sc += 1
        # self.ponTema(tema)
        # self.confTablero.png64Thumb( base64.b64encode(self.tablero.thumbnail(64)) )
        # tema["TEXTO"] = self.confTablero.grabaTema()
        # tema["BASE"] = self.confTablero.grabaBase()
        # self.liTemas.append(tema)
        # fichero = self.configuracion.ficheroTemas
        # Util.guardaVar( fichero, self.liTemas )
        dr = self.configuracion.leeVariables("PCOLORES")
        dirBase = dr["DIRBASE"] if dr else ""

        fich = QTUtil2.leeFichero(self, dirBase, "lktheme1")
        if fich:
            dr["DIRBASE"] = os.path.dirname(fich)
            self.configuracion.escVariables("PCOLORES", dr)
            tema = Util.recuperaVar(fich)
            self.ponTema(tema)

    def exportar(self):
        dr = self.configuracion.leeVariables("PCOLORES")
        dirBase = dr["DIRBASE"] if dr else ""
        fich = QTUtil2.salvaFichero(self, _("Colors"), dirBase, "*.lktheme1", True)
        if fich:
            dr["DIRBASE"] = os.path.dirname(fich)
            self.configuracion.escVariables("PCOLORES", dr)
            if not fich.lower().endswith("lktheme1"):
                fich += ".lktheme1"
            tema = {}
            tema["TEXTO"] = self.confTablero.grabaTema()
            tema["BASE"] = self.confTablero.grabaBase()
            Util.guardaVar(fich, tema)

    def ponTema(self, tema):
        ct = self.confTablero
        self.chbTemas.ponValor(False)
        self.defectoTemas()
        self.sinElegir = False
        ct.leeTema(tema.get("TEXTO", ""))
        if "BASE" in tema and tema["BASE"]:
            ct.leeBase(tema["BASE"])
        else:
            nomPiezas = ct.nomPiezas()
            ct._base.defecto()
            ct.cambiaPiezas(nomPiezas)

        ct = ct.copia(ct.id())  # para que los cambia captura no lo modifiquen

        self.btBlancasPNG.ponImagen()
        self.btNegrasPNG.ponImagen()
        self.btFondoPNG.ponImagen()

        self.lyF.ponImagen()
        self.lyFAlternativa.ponImagen()
        self.lyFActual.ponImagen()
        self.lyFRival.ponImagen()

        self.cbCoordenadas.ponValor(ct.nCoordenadas())
        self.chbDefCoordenadas.ponValor(ct.siDefCoordenadas())
        self.cbTipoLetra.setCurrentFont(QtGui.QFont(ct.tipoLetra()))
        self.chbDefTipoLetra.ponValor(ct.siDefTipoLetra())
        self.chbBold.ponValor(ct.siBold())
        self.chbDefBold.ponValor(ct.siDefBold())
        self.sbTamLetra.ponValor(ct.tamLetra())
        self.chbDefTamLetra.ponValor(ct.siDefTamLetra())
        self.sbSepLetras.ponValor(ct.sepLetras())
        self.chbDefSepLetras.ponValor(ct.siDefSepLetras())
        self.cbPiezas.ponValor(ct.nomPiezas())
        self.chbDefPiezas.ponValor(ct.siDefPiezas())
        self.sbTamRecuadro.ponValor(ct.tamRecuadro())
        self.chbDefTamRecuadro.ponValor(ct.siDefTamRecuadro())
        self.sbTamFrontera.ponValor(ct.tamFrontera())
        self.chbDefTamFrontera.ponValor(ct.siDefTamFrontera())

        self.chbExtended.ponValor(ct.extendedColor())

        self.actualizaTablero()

    def defectoTableroM(self):
        if self.siActualizando:
            return
        self.siActualizando = True

        self.actualizaTableroM()

        ct = self.confTablero
        for chb, obj, xv in (
                (self.chbDefCoordenadas, self.cbCoordenadas, ct.nCoordenadas),
                (self.chbDefBold, self.chbBold, ct.siBold),
                (self.chbDefTamLetra, self.sbTamLetra, ct.tamLetra),
                (self.chbDefSepLetras, self.sbSepLetras, ct.sepLetras),
                (self.chbDefPiezas, self.cbPiezas, ct.nomPiezas),
                (self.chbDefTamRecuadro, self.sbTamRecuadro, ct.tamRecuadro),
                (self.chbDefTamFrontera, self.sbTamFrontera, ct.tamFrontera)):
            if chb.valor():
                obj.ponValor(xv())
                obj.setEnabled(False)
            else:
                obj.setEnabled(True)

        if self.chbDefTipoLetra.valor():
            self.cbTipoLetra.setCurrentFont(QtGui.QFont(ct.tipoLetra()))
            self.cbTipoLetra.setEnabled(False)
        else:
            self.cbTipoLetra.setEnabled(True)

        self.siActualizando = False

    def actualizaTableroM(self):
        ct = self.confTablero

        ct.ponCoordenadas(None if self.chbDefCoordenadas.valor() else self.cbCoordenadas.valor())

        ct.ponTipoLetra(None if self.chbDefTipoLetra.valor() else self.cbTipoLetra.currentText())

        ct.ponBold(None if self.chbDefBold.valor() else self.chbBold.valor())

        ct.ponTamLetra(None if self.chbDefTamLetra.valor() else self.sbTamLetra.valor())

        ct.ponSepLetras(None if self.chbDefSepLetras.valor() else self.sbSepLetras.valor())

        ct.ponNomPiezas(None if self.chbDefPiezas.valor() else self.cbPiezas.valor())

        ct.ponTamRecuadro(None if self.chbDefTamRecuadro.valor() else self.sbTamRecuadro.valor())

        ct.ponTamFrontera(None if self.chbDefTamFrontera.valor() else self.sbTamFrontera.valor())

        self.actualizaTablero()

    def actualizaTablero(self):
        if hasattr(self, "tablero"):  # tras crear dial no se ha creado tablero
            # ct = self.confTablero
            self.tablero.crea()
            self.rehazFlechas()
            self.btExterior.ponColor()
            self.btBlancas.ponColor()
            self.btNegras.ponColor()
            self.btTexto.ponColor()
            self.btFrontera.ponColor()
            self.lbTamTablero.ponTexto("%dpx" % self.tablero.width())

    def leeTemas(self):
        fichero = self.configuracion.ficheroTemas
        liTemas = Util.recuperaVar(fichero)
        return [] if liTemas is None else liTemas

    def temas(self):
        siHay = len(self.liTemas) > 0
        menu = QTVarios.LCMenu(self)

        if siHay:
            mr = menu.submenu(_("Open"), Iconos.Recuperar())
            ponMenuTemas(mr, self.liTemas, "r")
            menu.separador()

        if self.temaActual:
            menu.opcion("grabar", _("Save") + " " + self.temaActual["NOMBRE"], Iconos.Grabar())
            menu.separador()

        menu.opcion("grabarComo", _("Save as"), Iconos.GrabarComo())
        menu.separador()

        if siHay:
            mr = menu.submenu(_("Remove"), Iconos.Borrar())
            ico = Iconos.PuntoNegro()
            for n, uno in enumerate(self.liTemas):
                if uno:
                    mr.opcion("b%d" % n, uno["NOMBRE"], ico)
                    mr.separador()

        resp = menu.lanza()
        if resp is None:
            return

        elif resp == "grabar":
            self.grabarTema(self.temaActual)

        elif resp == "grabarComo":
            tema = self.grabarTema({})
            self.liTemas.append(tema)

        elif resp.startswith("r"):
            num = int(resp[1:])
            tema = self.liTemas[num]
            self.temaActual = tema
            self.confTablero.leeTema(tema.get("TEXTO", ""))
            if "BASE" in tema:
                self.confTablero.leeBase(tema["BASE"])
            self.ponTema(tema)

        elif resp.startswith("b"):
            num = int(resp[1:])
            tema = self.liTemas[num]
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), tema["NOMBRE"])):
                if self.temaActual == tema:
                    self.temaActual = {}
                del self.liTemas[num]

        fichero = self.configuracion.ficheroTemas
        li = []
        for tema in self.liTemas:
            if tema:
                li.append(tema)
        Util.guardaVar(fichero, li)

    def grabarTema(self, tema):

        liGen = [(None, None)]

        nombre = tema["NOMBRE"] if tema else ""
        config = FormLayout.Editbox(_("Name"), ancho=160)
        liGen.append((config, nombre))

        seccion = tema.get("SECCION", "") if tema else ""
        config = FormLayout.Editbox(_("Section"), ancho=160)
        liGen.append((config, seccion))

        ico = Iconos.Grabar() if tema else Iconos.GrabarComo()

        resultado = FormLayout.fedit(liGen, title=_("Name"), parent=self, icon=ico)
        if resultado:
            accion, liResp = resultado
            nombre = liResp[0]
            seccion = liResp[1]

            if nombre:
                tema["NOMBRE"] = nombre
                if seccion:
                    tema["SECCION"] = seccion
                self.confTablero.png64Thumb(base64.b64encode(self.tablero.thumbnail(64)))
                tema["TEXTO"] = self.confTablero.grabaTema()
                tema["BASE"] = self.confTablero.grabaBase()
                self.temaActual = tema
                self.ponSecciones()
                return tema


def ponMenuTemas(menuBase, liTemas, baseResp):
    baseResp += "%d"

    dFolders = Util.SymbolDict()
    liRoot = []
    for n, uno in enumerate(liTemas):
        if uno:
            if "SECCION" in uno:
                folder = uno["SECCION"]
                if folder not in dFolders:
                    dFolders[folder] = []
                dFolders[folder].append((uno, n))
            else:
                liRoot.append((uno, n))
    icoFolder = Iconos.DivisionF()
    for k in dFolders:
        mf = menuBase.submenu(k, icoFolder)
        for uno, n in dFolders[k]:
            mf.opcion(baseResp % n, uno["NOMBRE"], QTVarios.iconoTema(uno, 16))
    menuBase.separador()
    for uno, n in liRoot:
        menuBase.opcion(baseResp % n, uno["NOMBRE"], QTVarios.iconoTema(uno, 16))
    menuBase.separador()


def eligeTema(parent, fichTema):
    liTemas = Util.recuperaVar(fichTema)
    if not liTemas:
        return None

    menu = QTVarios.LCMenu(parent)

    ponMenuTemas(menu, liTemas, "")

    resp = menu.lanza()

    return None if resp is None else liTemas[int(resp)]


def nag2ico(nag, tam):
    with open("./IntFiles/NAGs/Color/nag_%d.svg" % nag) as f:
        dato = f.read()
        color = getattr(VarGen.configuracion, "color_nag%d" % nag)
        dato = dato.replace("#3139ae", color)
    return QTVarios.svg2ico(dato, tam)


def cambiaColores(parent, configuracion):
    separador = (None, None)

    liColor = []
    liColor.append(separador)
    liColor.append((_("Reset to default") + ":", False))
    liColor.append(separador)

    palette = configuracion.palette
    palette_std = QtGui.QApplication.style().standardPalette()

    liPalette = []
    def xcolor(txt, tipo):
        config = FormLayout.Colorbox( txt, 40, 20, siSTR=True)
        color = QtGui.QColor(palette[tipo]) if palette else palette_std.color(getattr(QtGui.QPalette, tipo))
        liColor.append((config, color))
        liPalette.append(tipo)

    xcolor(_("General background"), "Window")
    xcolor(_("General foreground"), "WindowText")
    liColor.append(separador)
    xcolor(_("Text entry background"), "Base")
    xcolor(_("Text entry foreground"), "Text")
    xcolor(_("Alternate background"), "AlternateBase")
    liColor.append(separador)
    xcolor(_("Tool tip background"), "ToolTipBase")
    xcolor(_("Tool tip foreground"), "ToolTipText")
    liColor.append(separador)
    xcolor(_("Button background"), "Button")
    xcolor(_("Button foreground"), "ButtonText")
    xcolor(_("Bright text"), "BrightText")
    liColor.append(separador)
    xcolor(_("Links"), "Link")

    # QtGui.QPalette.Window	10	A general background color.
    # QtGui.QPalette.WindowText	0	A general foreground color.
    # QtGui.QPalette.Base	9	Used mostly as the background color for text entry widgets, but can also be used for other painting - such as the background of combobox drop down lists and toolbar handles. It is usually white or another light color.
    # QtGui.QPalette.AlternateBase	16	Used as the alternate background color in views with alternating row colors (see QAbstractItemView::setAlternatingRowColors()).
    # QtGui.QPalette.ToolTipBase	18	Used as the background color for QToolTip and QWhatsThis. Tool tips use the Inactive color group of QPalette, because tool tips are not active windows.
    # QtGui.QPalette.ToolTipText	19	Used as the foreground color for QToolTip and QWhatsThis. Tool tips use the Inactive color group of QPalette, because tool tips are not active windows.
    # QtGui.QPalette.Text	6	The foreground color used with Base. This is usually the same as the WindowText, in which case it must provide good contrast with Window and Base.
    # QtGui.QPalette.Button	1	The general button background color. This background can be different from Window as some styles require a different background color for buttons.
    # QtGui.QPalette.ButtonText	8	A foreground color used with the Button color.
    # QtGui.QPalette.BrightText	7	A text color that is very different from WindowText, and contrasts well with e.g. Dark. Typically used for text that needs to be drawn where Text or WindowText would give poor contrast, such as on pressed push buttons. Note that text colors can be used for things other than just words; text colors are usually used for text, but it's quite common to use the text color roles for lines, icons, etc.

    liPGN = []
    liPGN.append(separador)
    liPGN.append((_("Reset to default") + ":", False))
    liPGN.append(separador)

    dicNAGs = TrListas.dicNAGs()
    config = FormLayout.Colorbox(dicNAGs[1], 40, 20, siSTR=True)
    liPGN.append((config, configuracion.color_nag1))

    config = FormLayout.Colorbox(dicNAGs[2], 40, 20, siSTR=True)
    liPGN.append((config, configuracion.color_nag2))

    config = FormLayout.Colorbox(dicNAGs[3], 40, 20, siSTR=True)
    liPGN.append((config, configuracion.color_nag3))

    config = FormLayout.Colorbox(dicNAGs[4], 40, 20, siSTR=True)
    liPGN.append((config, configuracion.color_nag4))

    config = FormLayout.Colorbox(dicNAGs[5], 40, 20, siSTR=True)
    liPGN.append((config, configuracion.color_nag5))

    config = FormLayout.Colorbox(dicNAGs[6], 40, 20, siSTR=True)
    liPGN.append((config, configuracion.color_nag6))

    liTables = []
    liTables.append(separador)
    liTables.append((_("Reset to default") + ":", False))
    liTables.append(separador)

    liTables.append((None, _("Selected row")))
    config = FormLayout.Colorbox(_("Background"), 40, 20, siSTR=True)
    color = "#678DB2" if configuracion.tablaSelBackground is None else configuracion.tablaSelBackground
    liTables.append((config, color))

    lista = []
    lista.append((liColor, _("Windows"), ""))
    lista.append((liPGN, _("PGN"), ""))
    lista.append((liTables, _("Tables"), ""))

    # Editamos
    resultado = FormLayout.fedit(lista, title=_("Colors"), parent=parent, anchoMinimo=240, icon=Iconos.Opciones())

    if resultado:
        accion, resp = resultado

        liColor, liPGN, liTables = resp

        if liColor[0]:
            palette = None
        else:
            palette = {}
            for n, tipo in enumerate(liPalette):
                palette[tipo] = liColor[n+1]
        configuracion.palette = palette

        if liPGN[0]:
            configuracion.coloresPGNdefecto()
        else:
            (configuracion.color_nag1, configuracion.color_nag2, configuracion.color_nag3,
             configuracion.color_nag4, configuracion.color_nag5, configuracion.color_nag6) = liPGN[1:]

        if liTables[0]:
            configuracion.tablaSelBackground = None
        else:
            configuracion.tablaSelBackground = liTables[1]

        configuracion.graba()

        return True
    else:
        return False
