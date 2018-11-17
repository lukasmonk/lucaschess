from PyQt4 import QtCore, QtGui

from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import InfoBase
from Code.QT import QTUtil


class WAbout(QtGui.QDialog):
    def __init__(self, procesador):
        super(WAbout, self).__init__(procesador.pantalla)

        self.setWindowTitle(_("About"))
        self.setWindowIcon(Iconos.Aplicacion())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)
        self.setMaximumWidth(QTUtil.anchoEscritorio())

        f = Controles.TipoLetra(puntos=10)  # 0, peso=75 )

        cabecera = '<span style="font-size:30pt; font-weight="700"; font-family:arial; color:#2D2B2B">%s</span><br>' % _(
                "Lucas Chess")
        cabecera += '<span style="font-size:15pt;">%s</span><br>' % _X(_("version %1"), procesador.version)
        cabecera += '<span style="font-size:10pt;color:2D2B2B">%s: %s</span>' % (_("Author"), '<a href="mailto:lukasmonk@gmail.com">Lucas Monge</a>')
        cabecera += ' - <a style="font-size:10pt; color:2D2B2B" href="%s">%s</a>' % (procesador.web, procesador.web)
        cabecera += ' - <a style="font-size:10pt; color:2D2B2B" href="%s">Blog : Fresh news</a><br>' % (procesador.blog,)
        cabecera += '%s <a style="font-size:10pt; color:2D2B2B" href="http://www.gnu.org/copyleft/gpl.html"> GPL</a>' % _("License")

        lbIco = Controles.LB(self).ponImagen(Iconos.pmAplicacion64())
        lbTitulo = Controles.LB(self, cabecera)

        # Tabs
        tab = Controles.Tab()
        tab.ponFuente(f)

        ib = InfoBase.ThanksTo()

        for n, (k, titulo) in enumerate(ib.dic.iteritems()):
            txt = ib.texto(k)
            lb = Controles.LB(self, txt)
            lb.ponFondoN("#F6F3EE")
            lb.ponFuente(f)
            tab.addTab(lb, titulo)

        lyV1 = Colocacion.H().control(lbIco).espacio(15).control(lbTitulo).relleno()
        layout = Colocacion.V().otro(lyV1).espacio(10).control(tab).margen(10)

        self.setLayout(layout)


class WInfo(QtGui.QDialog):
    def __init__(self, wParent, titulo, cabecera, txt, minTam, pmIcono):
        super(WInfo, self).__init__(wParent)

        self.setWindowTitle(titulo)
        self.setWindowIcon(Iconos.Aplicacion())
        self.setWindowFlags(QtCore.Qt.WindowCloseButtonHint | QtCore.Qt.Dialog | QtCore.Qt.WindowTitleHint)

        f = Controles.TipoLetra(puntos=20)

        lbIco = Controles.LB(self).ponImagen(pmIcono)
        lbTitulo = Controles.LB(self, cabecera).alinCentrado().ponFuente(f)
        lbTexto = Controles.LB(self, txt)
        lbTexto.setMinimumWidth(minTam - 84)
        lbTexto.setWordWrap(True)
        lbTexto.setTextFormat(QtCore.Qt.RichText)
        btSeguir = Controles.PB(self, _("Continue"), self.seguir).ponPlano(False)

        lyV1 = Colocacion.V().control(lbIco).relleno()
        lyV2 = Colocacion.V().control(lbTitulo).control(lbTexto).espacio(10).control(btSeguir)
        lyH = Colocacion.H().otro(lyV1).otro(lyV2).margen(10)

        self.setLayout(lyH)

    def seguir(self):
        self.close()


def info(parent, titulo, cabecera, txt, minTam, pmIcono):
    w = WInfo(parent, titulo, cabecera, txt, minTam, pmIcono)
    w.exec_()
