import os

from PyQt4 import QtGui, QtCore

from Code.Constantes import *
from Code import PGN
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios
from Code import TrListas
from Code import VarGen


class Variantes(QtGui.QWidget):

    def __init__(self, owner):
        self.owner = owner
        configuracion = VarGen.configuracion
        self.siFigurines = configuracion.figurinesPGN
        puntos = configuracion.puntosPGN

        QtGui.QWidget.__init__(self, self.owner)

        liAcciones = (
            (_("Append"), Iconos.Mas(), self.tbMasVariante), None,
            ("%s+%s" % (_("Append"), _("Engine")), Iconos.MasR(), self.tbMasVarianteR), None,
            (_("Edit"), Iconos.ComentarioEditar(), self.tbEditarVariante), None,
            (_("Remove"), Iconos.Borrar(), self.tbBorrarVariante), None,
        )
        tbVariantes = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=16)

        self.em = Controles.EM(self)#.capturaCambios(self.variantesCambiado)
        self.em.setReadOnly(True)
        self.em.capturaDobleClick(self.dobleClick)

        ly = Colocacion.V().control(tbVariantes).control(self.em).margen(3)

        f = Controles.TipoLetra(puntos=puntos)

        gbVariantes = Controles.GB(self.owner, _("Variants"), ly).ponFuente(f)

        layout = Colocacion.H().control(gbVariantes).margen(0)
        self.setLayout(layout)

        self.liVariantes = []

        self.jg = None

    def ponJugada(self, jg):
        self.jg = jg

        variantes = jg.variantes
        self.liVariantes = []
        if variantes:
            lista = variantes.split("\n\n")

            fen = self.jg.posicionBase.fen()

            for lineaPGN in lista:
                uno = PGN.UnPGN()
                uno.leeTexto('[FEN "%s"]\n%s' % (fen, lineaPGN))
                self.liVariantes.append(uno.partida)
        self.mostrar()

    def mostrar(self):
        if self.liVariantes:
            html = '<table cellpadding="2" cellspacing="0" border="1" style="border-color: lightgray" width="100%">'
            for n, partida in enumerate(self.liVariantes):
                bgcolor = "#F5F5F5" if n%2 else "white"
                pgn = partida.pgnHTML(siFigurines=self.siFigurines)
                html += '<tr bgcolor="%s"><td>%s</td></tr>' % (bgcolor, pgn)
            html += "</table>"
        else:
            html = ""
        self.em.ponHtml(html)

    def select(self):
        if len(self.liVariantes) == 0:
            return None
        menu = QTVarios.LCMenu(self)
        rondo = QTVarios.rondoPuntos()
        for num, variante in enumerate(self.liVariantes):
            jg = variante.jugada(0)
            menu.opcion(num, "%d. %s" % (num+1, jg.pgnBaseSP()), rondo.otro())
        return menu.lanza()

    def editar(self, numero, siEngineActivo=False):
        import Code.Variantes as Variantes

        gestor = self.owner.wParent.gestor

        siCompetitivo = False
        if hasattr(gestor, "siCompetitivo"):
            if gestor.siCompetitivo:
                siCompetitivo = gestor.estado != kFinJuego

        if siCompetitivo:
            siEngineActivo = False

        fen = self.jg.posicionBase.fen()
        if numero == -1:
            pgn = ""
        else:
            pgn = self.liVariantes[numero].pgnBaseRAW()

        resp = Variantes.editaVariante(gestor.procesador, gestor, fen, pgn, siEngineActivo=siEngineActivo, siCompetitivo=siCompetitivo)
        if resp:
            lineaPGN, pv = resp
            fen = self.jg.posicionBase.fen()

            uno = PGN.UnPGN()
            uno.leeTexto('[FEN "%s"]\n%s' % (fen, lineaPGN))
            if numero == -1:
                self.liVariantes.append(uno.partida)
            else:
                self.liVariantes[numero] = uno.partida
            self.guardar()
            self.mostrar()

    def guardar(self):
        if self.liVariantes:
            self.jg.variantes = "\n\n".join([variante.pgnBaseRAW() for variante in self.liVariantes])
        else:
            self.jg.variantes = ""

    def dobleClick(self, event):
        txt = self.em.texto()
        pos = self.em.posicion()
        # Hay que ver cuantos \n hay antes de esa posicion
        fila = txt[:pos].count("\n")-1
        if fila == -1:
            self.tbMasVariante()
        else:
            self.editar(fila)

    def tbMasVariante(self, siEngineActivo=False):
        self.editar(-1, False)

    def tbMasVarianteR(self):
        self.editar(-1, True)

    def tbEditarVariante(self):
        num = self.select()
        if num is None:
            self.editar(-1)
        else:
            self.editar(num)

    def tbBorrarVariante(self):
        num = self.select()
        if num is not None:
            del self.liVariantes[num]
            self.guardar()
            self.mostrar()


class InformacionPGN(QtGui.QWidget):
    def __init__(self, wParent):
        QtGui.QWidget.__init__(self, wParent)

        self.wParent = wParent

        self.jg = None
        self.partida = None

        configuracion = VarGen.configuracion

        puntos = configuracion.puntosPGN

        f = Controles.TipoLetra(puntos=puntos, peso=75)
        f9 = Controles.TipoLetra(puntos=puntos)
        ftxt = f9

        # Apertura
        self.lbApertura = Controles.LB(self, "").ponFuente(f).alinCentrado().ponColorFondoN("#eeeeee", "#474d59").ponWrap()
        self.lbApertura.hide()

        # Valoracion
        liOpciones = [("-", "-")]
        dicNAGs = TrListas.dicNAGs()

        carpNAGs = "./IntFiles/NAGs"

        for x in dicNAGs:
            if x:
                fsvg = "%s/$%d.svg" % (carpNAGs, x)
                if os.path.isfile(fsvg):
                    liOpciones.append(("$%d : %s" % (x, dicNAGs[x]), str(x), QTVarios.fsvg2ico(fsvg, 16)))
                else:
                    liOpciones.append(("$%d : %s" % (x, dicNAGs[x]), str(x)))
        self.maxNAGs = 10
        self.liNAGs = []
        for x in range(self.maxNAGs):
            cb = Controles.CB(self, liOpciones, "").ponAnchoMinimo().capturaCambiado(self.valoracionCambiada).ponFuente(f9)
            if x:
                cb.hide()
            self.liNAGs.append(cb)

        btNAGS = Controles.PB(self, "", self.masNAGs).ponIcono(Iconos.Mas()).anchoFijo(22)

        liOpciones = [(x, x) for x in ("-", "!", "!!", "?", "??", "!?", "?!")]
        self.valoracionDirecta = Controles.CB(self, liOpciones, "-").ponAnchoFijo(42).capturaCambiado(self.valoracionDirectaCambiada)

        lyH = Colocacion.H().control(self.valoracionDirecta).control(self.liNAGs[0]).control(btNAGS)
        ly = Colocacion.V().otro(lyH)
        for x in range(1, self.maxNAGs):
            ly.control(self.liNAGs[x])

        self.gbValoracion = Controles.GB(self, _("Rating"), ly).ponFuente(f)

        # Comentarios
        self.comentario = Controles.EM(self, siHTML=False).capturaCambios(self.comentarioCambiado).ponFuente(ftxt).anchoMinimo(200)
        ly = Colocacion.H().control(self.comentario).margen(3)
        self.gbComentario = Controles.GB(self, _("Comments"), ly).ponFuente(f)

        # Variantes
        self.variantes = Variantes(self)

        self.splitter = splitter = QtGui.QSplitter(self)
        self.splitter.setOrientation(QtCore.Qt.Vertical)
        splitter.addWidget(self.gbComentario)
        splitter.addWidget(self.variantes)

        layout = Colocacion.V()
        layout.control(self.lbApertura)
        layout.control(self.gbValoracion)
        layout.control(self.splitter)
        layout.margen(1)

        self.setLayout(layout)

    def masNAGs(self):
        for cb in self.liNAGs:
            if not cb.isVisible():
                cb.ponValor("-")
                cb.show()
                return

    def ponJG(self, partida, jg, apertura):
        self.partida = partida
        self.jg = jg

        if not apertura:
            self.lbApertura.hide()

        siJG = self.jg is not None
        self.gbValoracion.setVisible(siJG)
        self.variantes.setVisible(siJG)

        if siJG:
            self.gbComentario.ponTexto(_("Comments"))
            if apertura:
                self.lbApertura.ponTexto(apertura)
                if jg.siApertura:
                    self.lbApertura.ponColorFondoN("#eeeeee", "#474d59")
                else:
                    self.lbApertura.ponColorFondoN("#ffffff", "#aaaaaa")
                self.lbApertura.show()

            self.comentario.ponTexto(jg.comentario)
            self.variantes.ponJugada(jg)

            li = jg.critica.split(" ")

            self.ponNAGs(li)

            self.valoracionDirecta.ponValor(jg.criticaDirecta if jg.criticaDirecta else "-")

        else:
            self.gbComentario.ponTexto("%s - %s" % (_("Game"), _("Comments")))
            if partida:
                self.comentario.ponTexto(partida.firstComment)
                if apertura:
                    self.lbApertura.ponTexto(apertura)
                    self.lbApertura.ponColorFondoN("#eeeeee", "#474d59")
                    self.lbApertura.show()

    def ponNAGs(self, li):
        n = 0
        for nag in li:
            if nag.isdigit():
                cb = self.liNAGs[n]
                cb.ponValor(nag)
                cb.show()
                n += 1
        if n == 0:
            cb = self.liNAGs[0]
            cb.ponValor("-")
            cb.show()
        else:
            for x in range(n, self.maxNAGs):
                cb = self.liNAGs[x]
                cb.ponValor("-")
                cb.hide()

    def keyPressEvent(self, event):
        pass  # Para que ESC no cierre el programa

    def comentarioCambiado(self):
        if self.jg:
            self.jg.comentario = self.comentario.texto()
        else:
            self.partida.firstComment = self.comentario.texto()

    def valoracionCambiada(self, npos):
        if self.jg:
            li = []
            for x in range(self.maxNAGs):
                v = self.liNAGs[x].valor()
                if v != "-":
                    li.append(v)
            self.jg.critica = " ".join(li)
            self.ponNAGs(li)

    def valoracionDirectaCambiada(self, npos):
        if self.jg:
            criticaDirecta = self.valoracionDirecta.valor()
            self.jg.criticaDirecta = criticaDirecta if criticaDirecta != "-" else ""

