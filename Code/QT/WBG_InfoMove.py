from PyQt4 import QtGui, QtCore

from Code import ControlPosicion
from Code import Jugada
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import VarGen


class WInfomove(QtGui.QWidget):
    def __init__(self, winBookGuide, siMoves=True):
        QtGui.QWidget.__init__(self)

        self.siMoves = siMoves
        self.winBookGuide = winBookGuide
        if self.siMoves:
            self.tree = winBookGuide.wmoves.tree
        else:
            self.tree = None
        self.movActual = None

        configuracion = VarGen.configuracion
        confTablero = configuracion.confTablero("INFOMOVEBOOKGUIDE", 32)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(True)
        self.tablero.ponMensajero(self.mueveHumano)
        self.cpActual = ControlPosicion.ControlPosicion()
        self.historia = None
        self.posHistoria = None

        self.intervalo = 1400

        lybt, bt = QTVarios.lyBotonesMovimiento(self, "", siTiempo=True, siLibre=False, tamIcon=24)

        self.lbPGN = Controles.LB(self).anchoFijo(self.tablero.ancho).ponWrap()
        self.lbPGN.colocate = self.colocatePartida
        self.lbPGN.setStyleSheet("QLabel{ border-style: groove; border-width: 2px; border-color: LightSlateGray; padding: 8px;}")
        self.lbPGN.ponTipoLetra(puntos=configuracion.puntosPGN)
        self.lbPGN.setOpenExternalLinks(False)
        def muestraPos(txt):
            self.colocatePartida(int(txt))
        self.connect(self.lbPGN, QtCore.SIGNAL("linkActivated(QString)"), muestraPos)

        self.siFigurines = configuracion.figurinesPGN

        if siMoves:
            tree = winBookGuide.wmoves.tree
            # Valoracion
            liOpciones = [(tit[0], k, tit[1]) for k, tit in tree.dicValoracion.iteritems()]
            self.lbValoracion = Controles.LB(self, _("Rating") + ":")
            self.cbValoracion = Controles.CB(self, liOpciones, 0).capturaCambiado(self.cambiadoValoracion)

            # Ventaja
            liOpciones = [(tit, k, icon) for k, (tit, icon) in tree.dicVentaja.iteritems()]
            self.lbVentaja = Controles.LB(self, _("Advantage") + ":")
            self.cbVentaja = Controles.CB(self, liOpciones, 0).capturaCambiado(self.cambiadoVentaja)

            # Comentario
            self.emComentario = Controles.EM(self, siHTML=False).capturaCambios(self.cambiadoComentario)

            lyVal = Colocacion.H().control(self.lbValoracion).control(self.cbValoracion).relleno()
            lyVen = Colocacion.H().control(self.lbVentaja).control(self.cbVentaja).relleno()
            lyEd = Colocacion.V().otro(lyVal).otro(lyVen).control(self.emComentario)
        else:
            self.lbOpening = Controles.LB(self).alinCentrado().ponWrap()
            self.lbOpening.ponTipoLetra(puntos=10, peso=200)
            lyO = Colocacion.H().relleno().control(self.lbOpening).relleno()

        lyt = Colocacion.H().relleno().control(self.tablero).relleno()

        lya = Colocacion.H().relleno().control(self.lbPGN).relleno()

        layout = Colocacion.V()
        layout.otro(lyt)
        layout.otro(lybt)
        if not siMoves:
            layout.otro(lyO)
        layout.otro(lya)
        if siMoves:
            layout.otro(lyEd)
        layout.relleno()
        self.setLayout(layout)

        self.usoNormal = True

        self.siReloj = False

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def modoNormal(self):
        self.usoNormal = True
        self.MoverFinal()

    def modoPartida(self, partida, jugada):
        self.usoNormal = False
        self.partida = partida
        if partida.apertura:
            txt = partida.apertura.trNombre
            if partida.pendienteApertura:
                txt += " ..."
            if not self.siMoves:
                self.lbOpening.ponTexto(txt)
        else:
            if not self.siMoves:
                self.lbOpening.ponTexto("")
        self.colocatePartida(jugada)
        self.camposEdicion(False)

    def modoFEN(self, partida, fen, jugada):
        self.usoNormal = False
        self.partida = partida
        self.lbOpening.ponTexto(fen)
        self.colocatePartida(jugada)
        self.camposEdicion(False)

    def setBookGuide(self, bookGuide):
        self.bookGuide = bookGuide

    def cambiadoValoracion(self):
        if self.movActual:
            self.movActual.nag(self.cbValoracion.valor())
            self.tree.resetValoracion(self.movActual)

    def cambiadoVentaja(self):
        if self.movActual:
            self.movActual.adv(self.cbVentaja.valor())
            self.tree.resetVentaja(self.movActual)

    def cambiadoComentario(self):
        if self.emComentario.hasFocus() and self.movActual:
            self.movActual.comment(self.emComentario.texto())
            self.tree.resetComentario(self.movActual)

    def ponMovimiento(self, move):
        if self.movActual:
            self.movActual.graphics(self.tablero.exportaMovibles())

        self.movActual = move

        self.historia = move.historia()
        self.MoverFinal()

        siVisible = move != self.bookGuide.root
        self.camposEdicion(siVisible)

        if siVisible:
            self.cbValoracion.ponValor(move.nag())
            self.cbVentaja.ponValor(move.adv())
            self.emComentario.ponTexto(move.comment())

    def camposEdicion(self, siVisible):
        if self.siMoves:
            self.lbValoracion.setVisible(siVisible)
            self.cbValoracion.setVisible(siVisible)
            self.lbVentaja.setVisible(siVisible)
            self.cbVentaja.setVisible(siVisible)
            self.emComentario.setVisible(siVisible)

    def mueveHumano(self, desde, hasta, coronacion=""):
        if self.cpActual.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.cpActual.siBlancas)
            if coronacion is None:
                return
        else:
            coronacion = ""

        siBien, mens, jg = Jugada.dameJugada(self.cpActual, desde, hasta, coronacion)

        if siBien:
            pv = desde + hasta + coronacion
            self.winBookGuide.seleccionaPV(pv)

    def colocate(self, pos):
        if not self.historia:
            self.tablero.activaColor(True)
            return
        lh = len(self.historia) - 1
        if pos >= lh:
            self.siReloj = False
            pos = lh
        if pos < 0:
            return self.MoverInicio()

        self.posHistoria = pos

        move = self.historia[self.posHistoria]
        self.cpActual.leeFen(move.fen())
        self.tablero.ponPosicion(self.cpActual)
        pv = move.pv()
        if pv:
            self.tablero.ponFlechaSC(pv[:2], pv[2:4])

        if self.posHistoria != lh:
            self.tablero.desactivaTodas()
        else:
            self.tablero.activaColor(self.cpActual.siBlancas)

        nh = len(self.historia)
        li = []
        for x in range(1, nh):
            uno = self.historia[x]
            xp = uno.pgnNum()
            if x > 1:
                if ".." in xp:
                    xp = xp.split("...")[1]
            if x == self.posHistoria:
                xp = '<span style="color:blue">%s</span>' % xp
            li.append(xp)
        pgn = " ".join(li)
        self.lbPGN.ponTexto(pgn)

    def colocatePartida(self, pos):
        if not self.partida.numJugadas():
            self.lbPGN.ponTexto("")
            self.tablero.ponPosicion(self.partida.iniPosicion)
            return
        lh = self.partida.numJugadas() - 1
        if pos >= lh:
            self.siReloj = False
            pos = lh

        p = self.partida

        numJugada = p.primeraJugada()
        pgn = ""
        style_number = "color:teal; font-weight: bold;"
        style_moves = "color:black;"
        style_select = "color:navy;font-weight: bold;"
        if p.siEmpiezaConNegras:
            pgn += '<span style="%s">%d...</span>' % (style_number, numJugada)
            numJugada += 1
            salta = 1
        else:
            salta = 0
        for n, jg in enumerate(p.liJugadas):
            if n % 2 == salta:
                pgn += '<span style="%s">%d.</span>' % (style_number, numJugada)
                numJugada += 1

            xp = jg.pgnHTML(self.siFigurines)
            if n == pos:
                xp = '<span style="%s">%s</span>' % (style_select, xp)
            else:
                xp = '<span style="%s">%s</span>' % (style_moves, xp)

            pgn += '<a href="%d" style="text-decoration:none;">%s</a> ' % (n, xp)

        self.lbPGN.ponTexto(pgn)

        self.posJugada = pos

        if pos < 0:
            self.tablero.ponPosicion(self.partida.iniPosicion)
            return

        jugada = self.partida.jugada(self.posJugada)
        posicion = jugada.posicion

        self.tablero.ponPosicion(posicion)
        self.tablero.ponFlechaSC(jugada.desde, jugada.hasta)

        self.tablero.desactivaTodas()

    def MoverInicio(self):
        if self.usoNormal:
            self.posHistoria = -1
            posicion = ControlPosicion.ControlPosicion().posInicial()
        else:
            # self.colocatePartida(-1)
            self.posJugada = -1
            posicion = self.partida.iniPosicion
        self.tablero.ponPosicion(posicion)

    def MoverAtras(self):
        if self.usoNormal:
            self.colocate(self.posHistoria - 1)
        else:
            self.colocatePartida(self.posJugada - 1)

    def MoverAdelante(self):
        if self.usoNormal:
            self.colocate(self.posHistoria + 1)
        else:
            self.colocatePartida(self.posJugada + 1)

    def MoverFinal(self):
        if self.usoNormal:
            self.colocate(len(self.historia) - 1)
        else:
            self.colocatePartida(99999)

    def MoverTiempo(self):
        if self.siReloj:
            self.siReloj = False
        else:

            menu = QTVarios.LCMenu(self)
            menu.opcion("previo", "%s: %0.02f" % (_("Duration of interval (secs)"), self.intervalo / 1000.0),
                        Iconos.MoverTiempo())
            menu.opcion("otro", _("Change interval"), Iconos.Configurar())
            resp = menu.lanza()
            if not resp:
                return

            if resp == "otro":
                liGen = [(None, None)]
                config = FormLayout.Editbox(_("Duration of interval (secs)"), 40, tipo=float)
                liGen.append((config, self.intervalo / 1000.0))
                resultado = FormLayout.fedit(liGen, title=_("Interval"), parent=self, icon=Iconos.MoverTiempo())
                if resultado is None:
                    return
                accion, liResp = resultado
                tiempo = liResp[0]
                if tiempo > 0.01:
                    self.intervalo = int(tiempo * 1000)
                else:
                    return

            self.siReloj = True
            self.MoverInicio()
            self.lanzaReloj()

    def lanzaReloj(self):
        if self.siReloj:
            self.MoverAdelante()
            QtCore.QTimer.singleShot(self.intervalo, self.lanzaReloj)
