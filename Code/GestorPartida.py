import time

from PyQt4.QtCore import Qt

from Code import Partida
from Code import ControlPosicion
from Code import Gestor
from Code import PGN
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import PantallaEntMaq
from Code.QT import PantallaPGN
from Code.QT import PantallaSolo
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import TrListas
from Code.QT import Voyager
from Code.Constantes import *


class GestorPartida(Gestor.Gestor):
    def inicio(self, partidaCompleta, siCompleta):
        self.tipoJuego = kJugSolo

        self.partida = partidaCompleta
        self.reinicio = self.partida.save()
        self.siCompleta = siCompleta

        self.siJuegaHumano = True
        self.siJugamosConBlancas = True

        self.siCambios = False

        self.siVolteoAutomatico = False

        self.estado = kJugando

        li = [k_grabar, k_cancelar, k_pgnInformacion, k_atras, k_reiniciar, k_configurar, k_utilidades]
        self.pantalla.ponToolBar(li)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.pantalla.ponRotulo1(None)
        self.pantalla.ponRotulo2(None)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.iniPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(partidaCompleta.iswhite())
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()
        self.ponteAlFinal()

        self.ponPosicionDGT()

        self.ponInformacion()

        self.refresh()

        self.siguienteJugada()

    def ponInformacion(self):
        if self.siCompleta:
            white = black = result = None
            for clave, valor in self.partida.liTags:
                clave = clave.upper()
                if clave == "WHITE":
                    white = valor
                elif clave == "BLACK":
                    black = valor
                elif clave == "RESULT":
                    result = valor
            self.ponRotulo1("%s : <b>%s</b><br>%s : <b>%s</b>" % (_("White"), white, _("Black"), black) if white and black else "")
            self.ponRotulo2("%s : <b>%s</b>" % (_("Result"), result) if result else "" )
            self.pantalla.ponWhiteBlack(white, black)

    def reiniciar(self):
        if self.siCambios and not QTUtil2.pregunta(self.pantalla, _("You will loose all changes, are you sure?")):
            return
        p = Partida.PartidaCompleta()
        p.restore(self.reinicio)
        self.inicio(p, self.siCompleta)

    def procesarAccion(self, clave):
        if clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_atras:
            self.atras()

        elif clave == k_grabar:
            self.pantalla.accept()

        elif clave == k_configurar:
            self.configurarGS()

        elif clave == k_utilidades:
            liMasOpciones = (
                ("libros", _("Consult a book"), Iconos.Libros()),
                (None, None, None),
                ("bookguide", _("Personal Opening Guide"), Iconos.BookGuide()),
                (None, None, None),
                ("play", _('Play current position'), Iconos.MoverJugar())
            )

            resp = self.utilidades(liMasOpciones)
            if resp == "libros":
                liMovs = self.librosConsulta(True)
                if liMovs:
                    for x in range(len(liMovs) - 1, -1, -1):
                        desde, hasta, coronacion = liMovs[x]
                        self.mueveHumano(desde, hasta, coronacion)
            elif resp == "bookguide":
                self.bookGuide()
            elif resp == "play":
                self.jugarPosicionActual()

        elif clave == k_pgnInformacion:
            self.informacion()

        elif clave in (k_cancelar, k_finpartida):
            self.finPartida()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finPartida(self):
        # Comprobamos que no haya habido cambios desde el ultimo grabado
        if self.siCambios:
            resp = QTUtil2.preguntaCancelar(self.pantalla, _("Do you want to cancel changes?"), _("Yes"), _("No"))
            if not resp:
                return False

        self.pantalla.reject()
        return True

    def finalX(self):
        return self.finPartida()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas
        self.siJugamosConBlancas = siBlancas  # Compatibilidad, sino no funciona el cambio en pgn
        if self.siVolteoAutomatico:
            time.sleep(1)
            if siBlancas != self.tablero.siBlancasAbajo:
                self.tablero.rotaTablero()

        if self.partida.numJugadas() > 0:
            jgUltima = self.partida.last_jg()
            if jgUltima:
                if jgUltima.siJaqueMate:
                    self.ponResultado(kGanaRival, not jgUltima.posicion.siBlancas)
                    return
                if jgUltima.siAhogado:
                    self.ponResultado(kTablas)
                    return
                if jgUltima.siTablasRepeticion:
                    self.ponResultado(kTablasRepeticion)
                    return
                if jgUltima.siTablas50:
                    self.ponResultado(kTablas50)
                    return
                if jgUltima.siTablasFaltaMaterial:
                    self.ponResultado(kTablasFaltaMaterial)
                    return

        self.ponIndicador(siBlancas)
        self.refresh()

        self.siJuegaHumano = True
        self.activaColor(siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.siJuegaHumano = True
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)

        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):
        self.siCambios = True

        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        resp = self.partida.si3repetidas()
        if resp:
            jg.siTablasRepeticion = True
            rotulo = ""
            for j in resp:
                rotulo += "%d," % (j / 2 + 1,)
            rotulo = rotulo.strip(",")
            self.rotuloTablasRepeticion = rotulo

        if self.partida.ultPosicion.movPeonCap >= 100:
            jg.siTablas50 = True

        if self.partida.ultPosicion.siFaltaMaterial():
            jg.siTablasFaltaMaterial = True

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

    def ponResultado(self, quien, siBlancas=None):
        self.desactivaTodas()

        self.resultadoQuien = quien
        self.resultadoSiBlancas = siBlancas

        self.resultado = quien

        if quien == kTablasRepeticion:
            self.resultado = kTablas

        elif quien == kTablas50:
            self.resultado = kTablas

        elif quien == kTablasFaltaMaterial:
            self.resultado = kTablas

    def actualPGN(self):
        resp = ""
        st = set()
        for eti, valor in self.partida.liTags:
            etiU = eti.upper()
            if etiU in st:
                continue
            st.add(etiU)
            resp += '[%s "%s"]\n' % (eti, valor)
            if etiU == "RESULT":
                result = valor

        if "RESULT" not in st:
            if self.resultado == kDesconocido:
                result = "*"

            elif self.resultado == kTablas:
                result = "1/2-1/2"

            else:
                result = '1-0' if self.resultadoSiBlancas else '0-1'

            resp += '[Result "%s"]\n' % result

        if self.fen:
            resp += '[FEN "%s"]\n' % self.fen

        ap = self.partida.apertura
        if ap:
            if "ECO" not in st:
                resp += '[ECO "%s"]\n' % ap.eco
            if "OPENING" not in st:
                resp += '[Opening "%s"]\n' % ap.trNombre

        resp += "\n" + self.partida.pgnBase() + " " + result

        return resp

    def editarEtiquetasPGN(self):
        resp = PantallaSolo.editarEtiquetasPGN(self.procesador, self.partida.liTags)
        if resp:
            self.partida.liTags = resp
            self.siCambios = True
            self.ponInformacion()

    def informacion(self):
        menu = QTVarios.LCMenu(self.pantalla)
        f = Controles.TipoLetra(puntos=10, peso=75)
        menu.ponFuente(f)

        siOpening = False
        for clave, valor in self.partida.liTags:
            trad = TrListas.pgnLabel(clave)
            if trad != clave:
                clave = trad
            menu.opcion(clave, "%s : %s" % (clave, valor), Iconos.PuntoAzul())
            if clave.upper() == "OPENING":
                siOpening = True

        if not siOpening:
            apertura = self.partida.apertura
            if apertura:
                menu.separador()
                nom = apertura.trNombre
                ape = _("Opening")
                rotulo = nom if ape.upper() in nom.upper() else ("%s : %s" % (ape, nom))
                menu.opcion("opening", rotulo, Iconos.PuntoNaranja())

        menu.separador()
        menu.opcion("pgn", _("Edit PGN labels"), Iconos.PGN())

        resp = menu.lanza()
        if resp:
            self.editarEtiquetasPGN()

    def configurarGS(self):
        sep = (None, None, None)

        liMasOpciones = [
            ("rotacion", _("Auto-rotate board"), Iconos.JS_Rotacion()), sep,
            ("leerpgn", _("Read PGN"), Iconos.PGN_Importar()), sep,
            ("pastepgn", _("Paste PGN"), Iconos.Pegar16()), sep,
        ]
        if not self.siCompleta:
            liMasOpciones.extend( [ ("posicion", _("Start position"), Iconos.Datos()), sep,
                                    ("pasteposicion", _("Paste FEN position"), Iconos.Pegar16()), sep,
                                    ("voyager", _("Voyager 2"), Iconos.Voyager1()) ] )

        resp = self.configurar(liMasOpciones, siCambioTutor=True, siSonidos=True)

        if resp == "rotacion":
            self.siVolteoAutomatico = not self.siVolteoAutomatico
            siBlancas = self.partida.ultPosicion.siBlancas
            if self.siVolteoAutomatico:
                if siBlancas != self.tablero.siBlancasAbajo:
                    self.tablero.rotaTablero()

        elif resp == "posicion":
            ini_fen = self.partida.iniPosicion.fen()
            cur_fen = Voyager.voyagerFEN(self.pantalla, ini_fen)
            if cur_fen and cur_fen != ini_fen:
                self.partida.resetFEN(cur_fen)
                self.inicio(self.partida, self.siCompleta)

        elif resp == "pasteposicion":
            texto = QTUtil.traePortapapeles()
            if texto:
                cp = ControlPosicion.ControlPosicion()
                try:
                    cp.leeFen(str(texto))
                    self.fen = cp.fen()
                    self.posicApertura = None
                    self.reiniciar()
                except:
                    pass

        elif resp == "leerpgn":
            unpgn = PantallaPGN.eligePartida(self.pantalla)
            if unpgn:
                partida = unpgn.partida
                if self.siCompleta and not partida.siFenInicial():
                    return
                p = Partida.PartidaCompleta()
                p.leeOtra(partida)
                p.asignaApertura()
                p.setTags(unpgn.listaCabeceras())
                self.reinicio = p.save()
                self.reiniciar()

        elif resp == "pastepgn":
            texto = QTUtil.traePortapapeles()
            if texto:
                unpgn = PGN.UnPGN()
                unpgn.leeTexto(texto)
                if unpgn.siError:
                    QTUtil2.mensError(self.pantalla, _("The text from the clipboard does not contain a chess game in PGN format"))
                    return
                partida = unpgn.partida
                if self.siCompleta and not partida.siFenInicial():
                    return
                p = Partida.PartidaCompleta()
                p.leeOtra(partida)
                p.asignaApertura()
                p.setTags(unpgn.listaCabeceras())
                self.reinicio = p.save()
                self.reiniciar()

        elif resp == "voyager":
            ptxt = Voyager.voyagerPartida(self.pantalla, self.partida)
            if ptxt:
                dic = self.creaDic()
                dic["PARTIDA"] = ptxt
                p = self.partida.copia()
                p.recuperaDeTexto(ptxt)
                dic["FEN"] = None if p.siFenInicial() else p.iniPosicion.fen()
                dic["SIBLANCASABAJO"] = self.tablero.siBlancasAbajo
                self.reiniciar(dic)

    def controlTeclado(self, nkey):
        if nkey == Qt.Key_V:  # V
            self.paste(QTUtil.traePortapapeles())

    def listHelpTeclado(self):
        return [
            ("V", _("Paste position")),
        ]

    def juegaRival(self):
        if not self.siTerminada():
            self.pensando(True)
            rm = self.xrival.juega(nAjustado=self.xrival.nAjustarFuerza)
            self.pensando(False)
            if rm.desde:
                self.mueveHumano(rm.desde, rm.hasta, rm.coronacion)

    def cambioRival(self):
        if self.dicRival:
            dicBase = self.dicRival
        else:
            dicBase = self.configuracion.leeVariables("ENG_GESTORSOLO")

        dic = self.dicRival = PantallaEntMaq.cambioRival(self.pantalla, self.configuracion, dicBase, siGestorSolo=True)

        if dic:
            for k, v in dic.iteritems():
                self.reinicio[k] = v

            dr = dic["RIVAL"]
            rival = dr["CM"]
            r_t = dr["TIEMPO"] * 100  # Se guarda en decimas -> milesimas
            r_p = dr["PROFUNDIDAD"]
            if r_t <= 0:
                r_t = None
            if r_p <= 0:
                r_p = None
            if r_t is None and r_p is None and not dic["SITIEMPO"]:
                r_t = 1000

            nAjustarFuerza = dic["AJUSTAR"]
            self.xrival = self.procesador.creaGestorMotor(rival, r_t, r_p, nAjustarFuerza != kAjustarMejor)
            self.xrival.nAjustarFuerza = nAjustarFuerza

            dic["ROTULO1"] = _("Opponent") + ": <b>" + self.xrival.nombre
            self.ponRotulo1(dic["ROTULO1"])
            self.siJuegaMotor = True
            self.configuracion.escVariables("ENG_GESTORSOLO", dic)

    def atras(self):
        if self.partida.numJugadas():
            self.partida.anulaSoloUltimoMovimiento()
            if not self.fen:
                self.partida.asignaApertura()
            self.ponteAlFinal()
            self.estado = kJugando
            self.refresh()
            self.siguienteJugada()

    def tituloVentana(self):
        white = ""
        black = ""
        event = ""
        date = ""
        result = ""
        for clave, valor in self.partida.liTags:
            if clave.upper() == "WHITE":
                white = valor
            elif clave.upper() == "BLACK":
                black = valor
            elif clave.upper() == "EVENT":
                event = valor
            elif clave.upper() == "DATE":
                date = valor
            elif clave.upper() == "RESULT":
                result = valor
        return "%s-%s (%s, %s,%s)" % (white, black, event, date, result)
