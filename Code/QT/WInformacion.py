import os

from PyQt4 import QtGui

import Code.TrListas as TrListas
import Code.VarGen as VarGen
import Code.QT.Colocacion as Colocacion
import Code.QT.Controles as Controles
import Code.QT.Iconos as Iconos
import Code.QT.QTVarios as QTVarios

class InformacionPGN(QtGui.QWidget):
    def __init__(self, wParent):
        QtGui.QWidget.__init__(self, wParent)

        self.wParent = wParent

        self.jg = None
        self.partida = None

        puntos = VarGen.configuracion.puntosPGN

        f = Controles.TipoLetra(puntos=puntos, peso=75)
        # ftxt = Controles.TipoLetra( nombre="Courier New", puntos=puntos )
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
                    liOpciones.append(("$%d : %s" % ( x, dicNAGs[x]), str(x), QTVarios.fsvg2ico(fsvg, 16)))
                else:
                    liOpciones.append(("$%d : %s" % ( x, dicNAGs[x]), str(x)))
        self.maxNAGs = 10
        self.liNAGs = []
        for x in range(self.maxNAGs):
            cb = Controles.CB(self, liOpciones, "").ponAnchoMinimo().capturaCambiado(self.valoracionCambiada).ponFuente(
                f9)
            if x:
                cb.hide()
            self.liNAGs.append(cb)

        liOpciones = [(x, x) for x in ("-", "!", "!!", "?", "??", "!?", "?!" )]
        self.valoracionDirecta = Controles.CB(self, liOpciones, "-").ponAnchoFijo(42).capturaCambiado(self.valoracionDirectaCambiada)

        lyH = Colocacion.H().control(self.valoracionDirecta).control(self.liNAGs[0])
        ly = Colocacion.V().otro(lyH)
        for x in range(1, self.maxNAGs):
            ly.control(self.liNAGs[x])

        self.gbValoracion = Controles.GB(self, _("Rating"), ly).ponFuente(f)

        # Comentarios
        self.comentario = Controles.EM(self, siHTML=False).capturaCambios(self.comentarioCambiado).ponFuente(
            ftxt).anchoMinimo(200)

        ly = Colocacion.H().control(self.comentario)
        self.gbComentario = Controles.GB(self, _("Comments"), ly).ponFuente(f)

        # Variantes
        liAcciones = (
            ( _("Append"), Iconos.Mas(), "tbMasVariante" ), None,
            ( "%s+%s" % (_("Append"), _("Engine")), Iconos.MasR(), "tbMasVarianteR" ), None,
            ( _("Edit"), Iconos.ComentarioEditar(), "tbEditarVariante" ), None,
            ( _("Remove"), Iconos.Borrar(), "tbBorrarVariante" ), None,
        )
        tbVariantes = Controles.TB(self, liAcciones, siTexto=False, tamIcon=16)

        self.variantes = Controles.EM(self, siHTML=False).capturaCambios(self.variantesCambiado).ponFuente(ftxt)
        self.variantes.capturaDobleClick(self.variantesDobleClick)

        ly = Colocacion.V().control(tbVariantes).control(self.variantes)
        self.gbVariantes = Controles.GB(self, _("Variants"), ly).ponFuente(f)

        layout = Colocacion.V()
        layout.control(self.lbApertura)
        layout.control(self.gbValoracion)
        layout.control(self.gbComentario)
        layout.control(self.gbVariantes)
        layout.margen(5)

        self.setLayout(layout)

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def tbMasVariante(self, siEngineActivo=False):
        resp = self.editaVariante("", siEngineActivo)
        if resp is not None:
            txt = self.variantes.texto().strip()
            if txt:
                txt += "\n\n"
            nuevaLinea, a1h8 = resp
            txt += nuevaLinea
            self.variantes.ponTexto(txt)
            self.variantesCambiado()

    def tbMasVarianteR(self):
        self.tbMasVariante(siEngineActivo=True)

    def tbEditarVariante(self):
        self.variantesDobleClick(None)

    def tbBorrarVariante(self):
        txt = self.variantes.texto()
        pos = self.variantes.posicion()
        li = txt.replace("\n\n", "\n").split("\n")
        n = 0
        nLinea = -1
        for nLineaT, linea in enumerate(li):
            n += len(linea) + 1
            if pos < n:
                nLinea = nLineaT
                break
        if nLinea >= 0:
            del li[nLinea]
            txt = "\n\n".join(li)
            txt = txt.strip()
            self.variantes.ponTexto(txt)
            self.variantesCambiado()

    def ponJG(self, partida, jg, apertura):
        self.partida = partida
        self.jg = jg

        siJG = self.jg is not None
        self.gbValoracion.setVisible(siJG)
        self.gbVariantes.setVisible(siJG)

        if siJG:
            self.gbComentario.ponTexto("%s - %s" % ( _("Move"), _("Comments")))
            if apertura:
                self.lbApertura.ponTexto(apertura)
                if jg.siApertura:
                    self.lbApertura.ponColorFondoN("#eeeeee", "#474d59")
                else:
                    self.lbApertura.ponColorFondoN("#ffffff", "#aaaaaa")
                self.lbApertura.show()
            else:
                self.lbApertura.hide()

            self.comentario.ponTexto(jg.comentario)
            self.variantes.ponTexto(jg.variantes)

            li = jg.critica.split(" ")

            self.ponNAGs(li)

            self.valoracionDirecta.ponValor(jg.criticaDirecta if jg.criticaDirecta else "-")

        else:
            self.gbComentario.ponTexto("%s - %s" % ( _("Game"), _("Comments")))
            if partida:
                self.comentario.ponTexto(partida.firstComment)

    def ponNAGs(self, li):
        n = 0
        for nag in li:
            if nag.isdigit():
                cb = self.liNAGs[n]
                cb.ponValor(nag)
                cb.show()
                n += 1
        siShow = True
        for x in range(n, self.maxNAGs):
            cb = self.liNAGs[x]
            cb.ponValor("-")
            if siShow:
                siShow = False
                cb.show()
            else:
                cb.hide()

    def keyPressEvent(self, event):
        pass  # Para que ESC no cierre el programa

    def comentarioCambiado(self):
        if self.jg:
            self.jg.comentario = self.comentario.texto()
        else:
            self.partida.firstComment = self.comentario.texto()

    def variantesCambiado(self):
        if self.jg:
            self.jg.variantes = self.variantes.texto()

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

    def variantesDobleClick(self, event):
        txt = self.variantes.texto()
        pos = self.variantes.posicion()
        li = txt.replace("\n\n", "\n").split("\n")
        n = 0
        lineaPGN = ""
        for linea in li:
            n += len(linea) + 1
            if pos < n:
                lineaPGN = linea
                break
        if not lineaPGN.strip():
            self.tbMasVariante()
        else:
            resp = self.editaVariante(lineaPGN)
            if resp is not None:
                nuevaLinea, a1h8 = resp
                txt = txt.replace(lineaPGN, nuevaLinea)
                self.variantes.ponTexto(txt)
                self.variantesCambiado()

    def editaVariante(self, linea, siEngineActivo=False):
        import Code.Variantes as Variantes

        gestor = self.wParent.gestor
        pos, jg = gestor.jugadaActiva()
        if jg is None:
            return None
        fen = jg.posicionBase.fen()

        return Variantes.editaVariante(gestor.procesador, gestor, fen, linea, siEngineActivo=siEngineActivo)

