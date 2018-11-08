import collections

from PyQt4 import QtGui, QtCore

from Code import Jugada
from Code import Partida
from Code import TrListas
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import Tablero
from Code.QT import PantallaColores

SIN_VALORACION, MUY_MALO, MALO, BUENO, MUY_BUENO, INTERESANTE, DUDOSA = (0, 4, 2, 1, 3, 5, 6)
V_SIN, V_IGUAL, V_BLANCAS, V_NEGRAS, V_BLANCAS_MAS, V_NEGRAS_MAS, V_BLANCAS_MAS_MAS, V_NEGRAS_MAS_MAS = (
    0, 11, 14, 15, 16, 17, 18, 19)


class BoardLines(QtGui.QWidget):
    def __init__(self, panelOpening, configuracion):
        QtGui.QWidget.__init__(self)

        self.panelOpening = panelOpening
        self.dbop = panelOpening.dbop

        self.partidabase = panelOpening.partidabase
        self.num_jg_inicial = len(self.partidabase)
        self.posJugada = self.num_jg_inicial

        confTablero = configuracion.confTablero("POSLINES", 32)
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(True)
        self.tablero.ponMensajero(self.mueveHumano)
        self.tablero.dispatchSize(self.ajustaAncho)
        self.tablero.dbVisual_setFichero(self.dbop.nomFichero)
        self.tablero.dbVisual_setShowAllways(True)
        self.tablero.dbVisual_setSaveAllways(True)

        self.tablero.ponerPiezasAbajo(self.dbop.getconfig("WHITEBOTTOM", True))

        self.dbop.setdbVisual_Tablero(self.tablero) # To close

        self.intervalo = 1400

        tipoLetra = Controles.TipoLetra(puntos=configuracion.puntosPGN)

        lybt, bt = QTVarios.lyBotonesMovimiento(self, "", siTiempo=True, siLibre=False, tamIcon=24)

        self.lbPGN = Controles.LB(self).ponWrap()
        self.lbPGN.colocate = self.colocatePartida
        self.lbPGN.setStyleSheet("QLabel{ border-style: groove; border-width: 2px; border-color: LightSlateGray; padding: 8px;}")
        self.lbPGN.ponFuente(tipoLetra)
        self.lbPGN.setOpenExternalLinks(False)
        def muestraPos(txt):
            self.colocatePartida(int(txt))
        self.connect(self.lbPGN, QtCore.SIGNAL("linkActivated(QString)"), muestraPos)

        self.siFigurines = configuracion.figurinesPGN

        dicNAGs = TrListas.dicNAGs()
        self.dicValoracion = collections.OrderedDict()
        self.dicValoracion[BUENO] = (dicNAGs[1], PantallaColores.nag2ico(1, 16))
        self.dicValoracion[MALO] = (dicNAGs[2], PantallaColores.nag2ico(2, 16))
        self.dicValoracion[MUY_BUENO] = (dicNAGs[3], PantallaColores.nag2ico(3, 16))
        self.dicValoracion[MUY_MALO] = (dicNAGs[4], PantallaColores.nag2ico(4, 16))
        self.dicValoracion[INTERESANTE] = (dicNAGs[5], PantallaColores.nag2ico(5, 16))
        self.dicValoracion[DUDOSA] = (dicNAGs[6], PantallaColores.nag2ico(6, 16))
        self.dicValoracion[SIN_VALORACION] = (_("No rating"), QtGui.QIcon())

        self.dicVentaja = collections.OrderedDict()
        self.dicVentaja[V_SIN] = (_("Undefined"), QtGui.QIcon())
        self.dicVentaja[V_IGUAL] = (dicNAGs[11], Iconos.V_Blancas_Igual_Negras())
        self.dicVentaja[V_BLANCAS] = (dicNAGs[14], Iconos.V_Blancas())
        self.dicVentaja[V_BLANCAS_MAS] = (dicNAGs[16], Iconos.V_Blancas_Mas())
        self.dicVentaja[V_BLANCAS_MAS_MAS] = (dicNAGs[18], Iconos.V_Blancas_Mas_Mas())
        self.dicVentaja[V_NEGRAS] = (dicNAGs[15], Iconos.V_Negras())
        self.dicVentaja[V_NEGRAS_MAS] = (dicNAGs[17], Iconos.V_Negras_Mas())
        self.dicVentaja[V_NEGRAS_MAS_MAS] = (dicNAGs[19], Iconos.V_Negras_Mas_Mas())

        # Valoracion
        liOpciones = [(tit[0], k, tit[1]) for k, tit in self.dicValoracion.iteritems()]
        self.cbValoracion = Controles.CB(self, liOpciones, 0).capturaCambiado(self.cambiadoValoracion)
        self.cbValoracion.ponFuente(tipoLetra)

        # Ventaja
        liOpciones = [(tit, k, icon) for k, (tit, icon) in self.dicVentaja.iteritems()]
        self.cbVentaja = Controles.CB(self, liOpciones, 0).capturaCambiado(self.cambiadoVentaja)
        self.cbVentaja.ponFuente(tipoLetra)

        # Comentario
        self.emComentario = Controles.EM(self, siHTML=False).capturaCambios(self.cambiadoComentario)
        self.emComentario.ponFuente(tipoLetra)
        self.emComentario.altoFijo(5*configuracion.altoFilaPGN)
        lyVal = Colocacion.H().control(self.cbValoracion).control(self.cbVentaja)
        lyEd = Colocacion.V().otro(lyVal).control(self.emComentario)

        # Apertura
        self.lbApertura = Controles.LB(self).alinCentrado().ponFuente(tipoLetra).ponWrap()

        lyt = Colocacion.H().relleno().control(self.tablero).relleno()

        lya = Colocacion.H().relleno().control(self.lbPGN).relleno()

        layout = Colocacion.V()
        layout.otro(lyt)
        layout.otro(lybt)
        layout.otro(lya)
        layout.otro(lyEd)
        layout.control(self.lbApertura)
        layout.relleno()
        self.setLayout(layout)

        self.ajustaAncho()

        self.siReloj = False

        self.ponPartida(self.partidabase)

    def ponPartida(self, partida):
        partida.test_apertura()
        self.partida = partida
        rotulo = partida.rotuloApertura()
        if rotulo is not None:
            trans = partida.rotuloTransposition()
            if trans is not None:
                rotulo += "\n%s: %s" % (_("Transposition"), trans)
        else:
            rotulo = ""
        self.lbApertura.ponTexto(rotulo)

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def setvalue(self, clave, valor):
        if self.fenM2:
            dic = self.dbop.getfenvalue(self.fenM2)
            dic[clave] = valor
            self.dbop.setfenvalue(self.fenM2, dic)

    def cambiadoValoracion(self):
        self.setvalue("VALORACION", self.cbValoracion.valor())

    def cambiadoVentaja(self):
        self.setvalue("VENTAJA", self.cbVentaja.valor())

    def cambiadoComentario(self):
        self.setvalue("COMENTARIO", self.emComentario.texto().strip())

    def ajustaAncho(self):
        self.setFixedWidth(self.tablero.ancho+20)
        self.lbPGN.anchoFijo(self.tablero.ancho)

    def camposEdicion(self, siVisible):
        if self.siMoves:
            self.lbValoracion.setVisible(siVisible)
            self.cbValoracion.setVisible(siVisible)
            self.lbVentaja.setVisible(siVisible)
            self.cbVentaja.setVisible(siVisible)
            self.emComentario.setVisible(siVisible)

    def mueveHumano(self, desde, hasta, coronacion=""):
        cpActual = self.partida.jugada(self.posJugada).posicion if self.posJugada >= 0 else self.partida.iniPosicion
        if cpActual.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(cpActual.siBlancas)
            if coronacion is None:
                return

        siBien, mens, jg = Jugada.dameJugada(cpActual, desde, hasta, coronacion)

        if siBien:
            partida = Partida.Partida()
            partida.leeOtra(self.partida)

            if self.posJugada < self.partida.numJugadas()-1:
                partida.liJugadas = partida.liJugadas[:self.posJugada+1]
            partida.append_jg(jg)
            self.panelOpening.mueveHumano(partida)

    def resetValues(self):
        self.cbValoracion.ponValor(SIN_VALORACION)
        self.cbVentaja.ponValor(V_SIN)
        self.emComentario.ponTexto("")

    def colocatePartida(self, pos):
        self.fenM2 = None
        num_jugadas = self.partida.numJugadas()
        if num_jugadas == 0:
            self.posJugada = -1
            self.lbPGN.ponTexto("")
            self.tablero.ponPosicion(self.partida.iniPosicion)
            self.resetValues()
            self.activaPiezas()
            return

        if pos >= num_jugadas:
            self.siReloj = False
            pos = num_jugadas - 1
        elif pos < self.num_jg_inicial-1:
            pos = self.num_jg_inicial-1

        p = self.partida

        numJugada = 1
        pgn = ""
        style_number = "color:teal; font-weight: bold;"
        style_moves = "color:black;"
        style_select = "color:navy;font-weight: bold;"
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
            self.resetValues()
            self.activaPiezas()
            return

        jugada = self.partida.jugada(self.posJugada)
        posicion = jugada.posicion if jugada else self.partida.iniPosicion
        self.fenM2 = posicion.fenM2()
        dic = self.dbop.getfenvalue(self.fenM2)
        valoracion = dic.get("VALORACION", SIN_VALORACION)
        ventaja = dic.get("VENTAJA", V_SIN)
        comentario = dic.get("COMENTARIO", "")
        self.cbValoracion.ponValor(valoracion)
        self.cbVentaja.ponValor(ventaja)
        self.emComentario.ponTexto(comentario)

        self.tablero.ponPosicion(posicion)
        if jugada:
            self.tablero.ponFlechaSC(jugada.desde, jugada.hasta)
            posicionBase = jugada.posicionBase
            fenM2_base = posicionBase.fenM2()
            dicP = self.dbop.getfenvalue(fenM2_base)
            if "ANALISIS" in dicP:
                mrm = dicP["ANALISIS"]
                rm = mrm.mejorMov()
                self.tablero.creaFlechaMulti(rm.movimiento(), False)

        if self.siReloj:
            self.tablero.desactivaTodas()
        else:
            self.activaPiezas()

        self.panelOpening.setJugada(self.posJugada)

    def activaPiezas(self):
        self.tablero.desactivaTodas()
        if not self.siReloj and self.posJugada >= self.num_jg_inicial-1:
            if self.posJugada >= 0:
                jg = self.partida.jugada(self.posJugada)
                color = not jg.siBlancas()
            else:
                color = True
            self.tablero.activaColor(color)

    def MoverInicio(self):
         self.colocatePartida(0)

    def MoverAtras(self):
        self.colocatePartida(self.posJugada - 1)

    def MoverAdelante(self):
        self.colocatePartida(self.posJugada + 1)

    def MoverFinal(self):
        self.colocatePartida(99999)

    def MoverTiempo(self):
        if self.siReloj:
            self.siReloj = False
        else:
            self.siReloj = True
            if self.posJugada == self.partida.numJugadas()-1:
                self.MoverInicio()
            self.lanzaReloj()
        self.activaPiezas()

    def lanzaReloj(self):
        if self.siReloj:
            self.MoverAdelante()
            QtCore.QTimer.singleShot(self.intervalo, self.lanzaReloj)
