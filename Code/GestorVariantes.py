import time

from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code import PGN
from Code.QT import Iconos
from Code import Util
from Code.Constantes import *


class GestorVariantes(Gestor.Gestor):
    def inicio(self, fen, lineaPGN, okMasOpciones, siBlancasAbajo, siEngineActivo=False, siCompetitivo=False):

        self.pensando(True)

        self.liKibitzersActivas = self.procesador.liKibitzersActivas

        self.okMasOpciones = okMasOpciones

        self.fen = fen
        self.lineaPGN = lineaPGN

        self.siAceptado = False

        self.siCompetitivo = siCompetitivo

        uno = PGN.UnPGN()
        uno.leeTexto('[FEN "%s"]\n%s' % (fen, lineaPGN))

        self.partida = uno.partida
        self.pgn.partida = self.partida

        self.tipoJuego = kJugSolo

        self.siJuegaHumano = True
        self.siJuegaPorMi = True
        self.dicRival = {}

        self.siJuegaMotor = False

        self.estado = kJugando

        self.siRevision = False

        self.siVolteoAutomatico = False
        self.pantalla.ponToolBar((k_aceptar, k_cancelar, k_atras, k_reiniciar, k_configurar, k_utilidades))

        self.siJugamosConBlancas = siBlancasAbajo
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.pantalla.ponRotulo1(None)
        self.pantalla.ponRotulo2(None)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.ponMensajero(self.mueveHumano)
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        self.refresh()

        if self.partida.numJugadas():
            self.mueveJugada(kMoverInicio)
            jg = self.partida.jugada(0)
            self.ponFlechaSC(jg.desde, jg.hasta)
            self.desactivaTodas()
        else:
            self.ponPosicion(self.partida.ultPosicion)

        self.pensando(False)

        siBlancas = self.partida.ultPosicion.siBlancas
        self.siJugamosConBlancas = siBlancas
        self.siJuegaHumano = True

        if siEngineActivo and not siCompetitivo:
            self.activeEngine()

        if not self.partida.numJugadas():
            self.siguienteJugada()

    def procesarAccion(self, clave):
        if clave == k_aceptar:
            self.siAceptado = True
            # self.resultado =
            self.procesador.pararMotores()
            self.pantalla.accept()

        elif clave == k_cancelar:
            self.procesador.pararMotores()
            self.pantalla.reject()

        elif clave == k_atras:
            self.atras()

        elif clave == k_reiniciar:
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar()

        elif clave == k_utilidades:
            liMasOpciones = []
            if self.okMasOpciones:
                liMasOpciones = (
                    ("libros", _("Consult a book"), Iconos.Libros()),
                    (None, None, None),
                    ("bookguide", _("Personal Opening Guide"), Iconos.BookGuide()),
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

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def valor(self):
        if self.siAceptado:
            pgn = self.partida.pgnBaseRAW().strip()
            a1h8 = self.partida.pv()
            return pgn, a1h8
        else:
            return None

    def finalX(self):
        self.procesador.pararMotores()
        return True

    def atras(self):
        if self.partida.numJugadas():
            self.partida.anulaSoloUltimoMovimiento()
            if not self.fen:
                self.partida.asignaApertura()
            self.ponteAlFinal()
            self.refresh()
            self.siguienteJugada()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas
        self.siJugamosConBlancas = siBlancas
        self.siJuegaHumano = True
        if self.siVolteoAutomatico:
            time.sleep(1)
            if siBlancas != self.tablero.siBlancasAbajo:
                self.tablero.rotaTablero()

        if self.partida.numJugadas() > 0:
            jgUltima = self.partida.last_jg()
            if jgUltima:
                if jgUltima.siJaqueMate:
                    self.ponResultado(kGanaRival, jgUltima.posicion.siBlancas)
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

        self.activaColor(siBlancas)
        self.refresh()

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False
        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg)
        if self.siJuegaMotor:
            self.siJuegaMotor = False
            self.desactivaTodas()
            self.juegaRival()
            self.siJuegaMotor = True  # Como juega por mi pasa por aqui, para que no se meta en un bucle infinito

        self.siguienteJugada()
        return True

    def masJugada(self, jg):

        self.beepExtendido(True)

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

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

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
        """
        Llamado para grabar el pgn
        """
        resp = ""
        st = set()
        hoy = Util.hoy()
        liPGN = []
        liPGN.append(["Event", _("Lucas Chess")])
        liPGN.append(["Date", "%d.%02d.%02d" % (hoy.year, hoy.month, hoy.day)])
        jugador = "unknown"
        liPGN.append(["White", jugador])
        liPGN.append(["Black", jugador])

        for eti, valor in liPGN:
            resp += '[%s "%s"]\n' % (eti, valor)
            st.add(eti)

        if "Result" not in st:
            if self.resultado == kDesconocido:
                result = "*"

            elif self.resultado == kTablas:
                result = "1/2-1/2"

            else:
                result = '1-0' if self.resultadoSiBlancas else '0-1'

            resp += '[Result "%s"]\n' % result
        else:
            result = st["Result"]

        if self.fen:
            if self.fen != ControlPosicion.FEN_INICIAL:
                resp += '[FEN "%s"]\n' % self.fen

        ap = self.partida.apertura
        if ap:
            if "ECO" not in st:
                resp += '[ECO "%s"]\n' % ap.eco
            if "Opening" not in st:
                resp += '[Opening "%s"]\n' % ap.trNombre

        resp += self.partida.pgnBase() + " " + result

        return resp

    def reiniciar(self):
        self.inicio(self.fen, self.lineaPGN, self.okMasOpciones, self.siJugamosConBlancas)

    def configurar(self):

        mt = _("Engine").lower()
        mt = _X(_("Disable %1"), mt) if self.siJuegaMotor else _X(_("Enable %1"), mt)

        if not self.siCompetitivo:
            liMasOpciones = (
                ("motor", mt, Iconos.Motores()),
            )
        else:
            liMasOpciones = []

        resp = Gestor.Gestor.configurar(self, liMasOpciones, siCambioTutor=not self.siCompetitivo)

        if resp == "motor":
            self.ponRotulo1("")
            if self.siJuegaMotor:
                self.xrival.terminar()
                self.xrival = None
                self.siJuegaMotor = False
            else:
                self.cambioRival()

    def juegaRival(self):
        if not self.siTerminada():
            self.pensando(True)
            rm = self.xrival.juega(nAjustado=self.xrival.nAjustarFuerza)
            if rm.desde:
                siBien, self.error, jg = Jugada.dameJugada(self.partida.ultPosicion, rm.desde, rm.hasta, rm.coronacion)
                self.masJugada(jg)
                self.movimientosPiezas(jg.liMovs)
                self.partida.ultPosicion = jg.posicion
            self.pensando(False)

    def activeEngine(self):
        dicBase = self.configuracion.leeVariables("ENG_VARIANTES")
        if dicBase:
            self.ponRival(dicBase)
        else:
            self.cambioRival()

    def cambioRival(self):

        if self.dicRival:
            dicBase = self.dicRival
        else:
            dicBase = self.configuracion.leeVariables("ENG_VARIANTES")

        import Code.QT.PantallaEntMaq as PantallaEntMaq

        dic = self.dicRival = PantallaEntMaq.cambioRival(self.pantalla, self.configuracion, dicBase, siGestorSolo=True)

        if dic:
            self.ponRival(dic)

    def ponRival(self, dic):
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
        self.configuracion.escVariables("ENG_VARIANTES", dic)
