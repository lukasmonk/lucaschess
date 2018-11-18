from PyQt4 import QtCore, QtGui

from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import Info
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios


def datos(wParent, configuracion, procesador):
    # Primero determinamos la categoria
    resp = dameCategoria(wParent, configuracion, procesador)
    if resp:
        categoria = resp
    else:
        return None

    w = wDatos(wParent, categoria, configuracion)
    if w.exec_():
        return categoria, w.nivel, w.siBlancas, w.puntos
    else:
        return None


def dameCategoria(wParent, configuracion, procesador):
    rival = configuracion.rival

    menu = QTVarios.LCMenu(wParent)

    menu.opcion(None, "%s: %d %s" % (_("Total score"), configuracion.puntuacion(), _("pts")), Iconos.NuevaPartida())
    menu.separador()
    menu.opcion(None, "%s: %s" % (_("Opponent"), rival.rotuloPuntos()), Iconos.Motor(), siDeshabilitado=False)
    menu.separador()

    # ---------- CATEGORIAS
    ant = 1
    for x in range(6):
        cat = rival.categorias.numero(x)
        txt = cat.nombre()
        nm = cat.nivelHecho

        nh = cat.hecho

        if nm > 0:
            txt += " %s %d" % (_("Level"), nm)
        if nh:
            if "B" in nh:
                txt += " +%s:%d" % (_("White"), nm + 1)
            if "N" in nh:
                txt += " +%s:%d" % (_("Black"), nm + 1)

                # if "B" not in nh:
                # txt += "  ...  %s:%d"%( _( "White" )[0],nm+1)
                # elif "N" not in nh:
                # txt += "  ...  %s:%d"%( _( "Black" )[0],nm+1)
                # else:
                # txt += "  ...  %s:%d"%( _( "White" )[0],nm+1)

        siDesHabilitado = (ant == 0)
        ant = nm
        menu.opcion(str(x), txt, cat.icono(), siDeshabilitado=siDesHabilitado)

    # ----------- RIVAL
    menu.separador()
    menuRival = menu.submenu(_("Change opponent"))

    puntuacion = configuracion.puntuacion()

    icoNo = Iconos.Motor_No()
    icoSi = Iconos.Motor_Si()
    icoActual = Iconos.Motor_Actual()
    grpNo = Iconos.Grupo_No()
    grpSi = Iconos.Grupo_Si()

    for grupo in configuracion.grupos.liGrupos:
        nombre = _X(_("%1 group"), grupo.nombre)
        if grupo.minPuntos > 0:
            nombre += " (+%d %s)" % (grupo.minPuntos, _("pts"))

        siDes = (grupo.minPuntos > puntuacion)
        if siDes:
            icoG = grpNo
            icoM = icoNo
        else:
            icoG = grpSi
            icoM = icoSi
        submenu = menuRival.submenu(nombre, icoG)

        for rv in grupo.liRivales:
            siActual = rv.clave == rival.clave
            ico = icoActual if siActual else icoM
            submenu.opcion("MT_" + rv.clave, rv.rotuloPuntos(), ico, siDes or siActual)
        menuRival.separador()

    # ----------- RIVAL
    menu.separador()
    menu.opcion("ayuda", _("Help"), Iconos.Ayuda())

    cursor = QtGui.QCursor.pos()
    resp = menu.lanza()
    if resp is None:
        return None
    elif resp == "ayuda":
        titulo = _("Competition")
        ancho, alto = QTUtil.tamEscritorio()
        ancho = min(ancho, 700)
        txt = _("<br><b>The aim is to obtain the highest possible score</b> :<ul><li>The current point score is displayed in the title bar.</li><li>To obtain points it is necessary to win on different levels in different categories.</li><li>To overcome a level it is necessary to win against the engine with white and with black.</li><li>The categories are ranked in the order of the following table:</li><ul><li><b>Beginner</b> : 5</li><li><b>Amateur</b> : 10</li><li><b>Candidate Master</b> : 20</li><li><b>Master</b> : 40</li><li><b>International Master</b> : 80</li><li><b>Grandmaster</b> : 160</li></ul><li>The score for each game is calculated by multiplying the playing level with the score of the category.</li><li>The engines are divided into groups.</li><li>To be able to play with an opponent of a particular group a minimum point score is required. The required score is shown next to the group label.</li></ul>")
        Info.info(wParent, _("Lucas Chess"), titulo, txt, ancho, Iconos.pmAyudaGR())
        return None

    elif resp.startswith("MT_"):
        procesador.cambiaRival(resp[3:])
        QtGui.QCursor.setPos(cursor)
        procesador.competicion()
        return None
    else:
        categoria = rival.categorias.numero(int(resp))
        return categoria


class wDatos(QtGui.QDialog):
    def __init__(self, wParent, categoria, configuracion):
        super(wDatos, self).__init__(wParent)

        self.setWindowTitle(_("New game"))
        self.setWindowIcon(Iconos.Datos())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        tb = QTUtil2.tbAcceptCancel(self)

        f = Controles.TipoLetra(puntos=12, peso=75)
        flb = Controles.TipoLetra(puntos=10)

        self.maxNivel, self.maxNivelHecho, self.maxPuntos = configuracion.maxNivelCategoria(categoria)
        # self.maxNivel = maxNivel = categoria.nivelHecho+1
        # self.maxNivelHecho = categoria.hecho
        # self.maxPuntos = categoria.maxPuntos()

        self.ed = Controles.SB(self, self.maxNivel, 1, self.maxNivel).tamMaximo(40)
        lb = Controles.LB(self, categoria.nombre() + " " + _("Level"))

        lb.ponFuente(f)
        self.lbPuntos = Controles.LB(self).alinDerecha()
        self.connect(self.ed, QtCore.SIGNAL("valueChanged(int)"), self.nivelCambiado)

        siBlancas = not categoria.siHechoBlancas()
        self.rbBlancas = QtGui.QRadioButton(_("White"))
        self.rbBlancas.setChecked(siBlancas)
        self.rbNegras = QtGui.QRadioButton(_("Black"))
        self.rbNegras.setChecked(not siBlancas)
        self.connect(self.rbBlancas, QtCore.SIGNAL("clicked()"), self.ponMaxPuntos)
        self.connect(self.rbNegras, QtCore.SIGNAL("clicked()"), self.ponMaxPuntos)

        # Rival
        rival = configuracion.rival
        lbRMotor = Controles.LB(self, "<b>%s</b> : %s" % (_("Engine"), rival.nombre)).ponFuente(flb).ponWrap().anchoFijo(
                400)
        lbRAutor = Controles.LB(self, "<b>%s</b> : %s" % (_("Author"), rival.autor)).ponFuente(flb).ponWrap().anchoFijo(400)
        lbRWeb = Controles.LB(self,
                              '<b>%s</b> : <a href="%s">%s</a>' % (_("Web"), rival.url, rival.url)).ponWrap().anchoFijo(400).ponFuente(
                flb)

        ly = Colocacion.V().control(lbRMotor).control(lbRAutor).control(lbRWeb).margen(10)
        gbR = Controles.GB(self, _("Opponent"), ly).ponFuente(f)

        # Tutor
        tutor = configuracion.tutor
        lbTMotor = Controles.LB(self, "<b>%s</b> : %s" % (_("Engine"), tutor.nombre)).ponFuente(flb).ponWrap().anchoFijo(
                400)
        lbTAutor = Controles.LB(self, "<b>%s</b> : %s" % (_("Author"), tutor.autor)).ponFuente(flb).ponWrap().anchoFijo(400)
        siURL = hasattr(tutor, "url")
        if siURL:
            lbTWeb = Controles.LB(self,
                                  '<b>%s</b> : <a href="%s">%s</a>' % ("Web", tutor.url, tutor.url)).ponWrap().anchoFijo(400).ponFuente(
                    flb)

        ly = Colocacion.V().control(lbTMotor).control(lbTAutor)
        if siURL:
            ly.control(lbTWeb)
        ly.margen(10)
        gbT = Controles.GB(self, _("Tutor"), ly).ponFuente(f)

        hbox = Colocacion.H().relleno().control(self.rbBlancas).espacio(10).control(self.rbNegras).relleno()
        gbColor = Controles.GB(self, _("Play with"), hbox).ponFuente(f)

        lyNivel = Colocacion.H().control(lb).control(self.ed).espacio(10).control(self.lbPuntos).relleno()

        vlayout = Colocacion.V().otro(lyNivel).espacio(10).control(gbColor).espacio(10).control(gbR).espacio(
                10).control(gbT).margen(30)

        layout = Colocacion.V().control(tb).otro(vlayout).margen(3)

        self.setLayout(layout)

        self.ponMaxPuntos()

    def aceptar(self):
        self.nivel = self.ed.value()
        self.siBlancas = self.rbBlancas.isChecked()
        self.accept()

    def nivelCambiado(self, nuevo):
        self.ponMaxPuntos()

    def ponMaxPuntos(self):
        p = 0
        if self.ed.value() >= self.maxNivel:
            color = "B" if self.rbBlancas.isChecked() else "N"
            if color not in self.maxNivelHecho:
                p = self.maxPuntos
        self.lbPuntos.setText("%d %s" % (p, _("points")))
        self.puntos = p


def numEntrenamiento(wParent, titulo, hasta, etiqueta=None, pos=None, mensAdicional=None):
    w = WNumEntrenamiento(wParent, titulo, hasta, etiqueta, pos, mensAdicional)
    if w.exec_():
        return w.numero
    else:
        return None


class WNumEntrenamiento(QtGui.QDialog):
    def __init__(self, wParent, titulo, hasta, etiqueta=None, pos=None, mensAdicional=None):
        super(WNumEntrenamiento, self).__init__(wParent)

        self.setWindowTitle(titulo)
        self.setWindowIcon(Iconos.Datos())

        tb = QTUtil2.tbAcceptCancel(self)

        if pos is None:
            pos = 1  # random.randint( 1, hasta )

        if etiqueta is None:
            etiqueta = _("Training unit")

        self.ed, lb = QTUtil2.spinBoxLB(self, pos, 1, hasta, etiqueta=etiqueta, maxTam=60)
        lb1 = Controles.LB(self, "/ %d" % hasta)

        if mensAdicional:
            lb2 = Controles.LB(self, mensAdicional)
            lb2.ponWrap().anchoMinimo(250)

        lyH = Colocacion.H().relleno().control(lb).control(self.ed).control(lb1).relleno().margen(15)

        lyV = Colocacion.V().control(tb).otro(lyH)
        if mensAdicional:
            lyV.control(lb2)
        lyV.margen(3)

        self.setLayout(lyV)

        self.resultado = None

    def aceptar(self):
        self.numero = self.ed.value()
        self.accept()


def numPosicion(wParent, titulo, nFEN, pos, salta, tipo):
    liGen = [FormLayout.separador]

    label = "%s (1..%d)" % (_("Select position"), nFEN)
    liGen.append((FormLayout.Spinbox(label, 1, nFEN, 50), pos))

    liGen.append(FormLayout.separador)

    li = [(_("Sequential"), "s"),
          (_("Random"), "r"),
          (_("Random with same sequence based on position"), "rk")
          ]
    liGen.append((FormLayout.Combobox(_("Type"), li), tipo))

    liGen.append(FormLayout.separador)

    liGen.append((_("Jump to the next after solve") + ":", salta))

    resultado = FormLayout.fedit(liGen, title=titulo, parent=wParent, anchoMinimo=200,
                                 icon=Iconos.Entrenamiento())
    if resultado:
        posicion, tipo, jump = resultado[1]
        return posicion, tipo, jump
    else:
        return None
