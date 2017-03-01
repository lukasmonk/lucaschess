from PyQt4 import QtGui

from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios


class TConf(QtGui.QWidget):
    def __init__(self, dicValoracion, dicVentaja):
        QtGui.QWidget.__init__(self)

        lyVal = Colocacion.V()
        self.chbVal = Controles.CHB(self, _("All/None"), True).capturaCambiado(self, self.capturaVal)
        lyVal.control(self.chbVal)
        lyVal.espacio(15)
        self.liVal = []
        for k, (txt, ico) in dicValoracion.iteritems():
            lb = Controles.LB(self).ponImagen(ico.pixmap(16, 16))
            chb = Controles.CHB(self, txt, True)
            chb.id = k
            lyW = Colocacion.H().control(lb).control(chb).relleno()
            lyVal.otro(lyW)
            self.liVal.append(chb)
        lyVal.relleno()
        gbVal = Controles.GB(self, _("Rating"), lyVal)

        lyVen = Colocacion.V()
        self.chbVen = Controles.CHB(self, _("All/None"), True).capturaCambiado(self, self.capturaVen)
        lyVen.control(self.chbVen)
        lyVen.espacio(15)
        self.liVen = []
        for k, (txt, ico) in dicVentaja.iteritems():
            lb = Controles.LB(self).ponImagen(ico.pixmap(16, 16))
            chb = Controles.CHB(self, txt, True)
            chb.id = k
            lyW = Colocacion.H().control(lb).control(chb).relleno()
            lyVen.otro(lyW)
            self.liVen.append(chb)
        lyVen.relleno()
        gbVen = Controles.GB(self, _("Advantage"), lyVen)

        layout = Colocacion.H().relleno().control(gbVal).control(gbVen).relleno()
        self.setLayout(layout)

    def capturaVal(self):
        ok = self.chbVal.valor()
        for chb in self.liVal:
            chb.ponValor(ok)

    def capturaVen(self):
        ok = self.chbVen.valor()
        for chb in self.liVen:
            chb.ponValor(ok)

    def resultado(self):
        reVal = []
        for chb in self.liVal:
            if not chb.valor():
                reVal.append(chb.id)

        reVen = []
        for chb in self.liVen:
            if not chb.valor():
                reVen.append(chb.id)

        return reVal, reVen


class WTraining(QTVarios.WDialogo):
    def __init__(self, wParent, dicValoracion, dicVentaja):
        icono = Iconos.TutorialesCrear()
        extparam = "trainingMyOwnBook"
        titulo = _("Create a training")
        QTVarios.WDialogo.__init__(self, wParent, titulo, icono, extparam)

        tb = QTUtil2.tbAcceptCancel(self)

        lbName = Controles.LB(self, _("Name") + ": ")
        self.name = Controles.ED(self).anchoFijo(200)
        lyName = Colocacion.H().relleno().control(lbName).control(self.name).relleno()

        self.chbAddSTD = Controles.CHB(self, _("Add a label with the standard opening"), True)
        lyNameSTD = Colocacion.V().otro(lyName).control(self.chbAddSTD).relleno()

        self.sbDepth, lbDepth = QTUtil2.spinBoxLB(self, 30, 2, 999, etiqueta=_("Depth"), maxTam=48)
        lyDepth = Colocacion.H().relleno().control(lbDepth).control(self.sbDepth).relleno()

        self.rbBlancas = Controles.RB(self, _("White")).activa()
        self.rbNegras = Controles.RB(self, _("Black"))
        hbox = Colocacion.H().control(self.rbBlancas).control(self.rbNegras)
        gbColor = Controles.GB(self, _("Play with"), hbox)

        self.tcWhite = TConf(dicValoracion, dicVentaja)
        ly = Colocacion.H().control(self.tcWhite)
        gbWhite = Controles.GB(self, "", ly)
        gbWhite.setStyleSheet("QWidget { background-color: %s }" % "Ivory")

        self.tcBlack = TConf(dicValoracion, dicVentaja)
        ly = Colocacion.H().control(self.tcBlack)
        gbBlack = Controles.GB(self, "", ly)
        gbBlack.setStyleSheet("QWidget { background-color: %s }" % "Lavender")

        tab = Controles.Tab()
        tab.nuevaTab(gbWhite, _("Conditions to White"))
        tab.nuevaTab(gbBlack, _("Conditions to Black"))

        lyHead = Colocacion.H().otro(lyNameSTD).espacio(15).otro(lyDepth).espacio(15).control(gbColor)

        layout = Colocacion.V().control(tb).otro(lyHead).control(tab).margen(3)

        self.setLayout(layout)

        self.recuperarVideo()

        self.siAceptado = False

    def resultado(self):
        dic = {}
        dic["WHITE"] = self.tcWhite.resultado()
        dic["BLACK"] = self.tcBlack.resultado()
        dic["SIWHITE"] = self.rbBlancas.isChecked()
        dic["NAME"] = self.name.texto().strip()
        dic["DEPTH"] = self.sbDepth.valor()
        dic["ADDSTD"] = self.chbAddSTD.valor()
        return dic

    def aceptar(self):
        if not self.name.texto().strip():
            QTUtil2.mensError(self, _("Name is missing"))
            return
        self.siAceptado = True
        self.accept()
