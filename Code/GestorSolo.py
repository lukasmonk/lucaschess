import os
import sys
import time

from PyQt4.QtCore import Qt

from Code import ControlPosicion
from Code import Gestor
from Code import PGN
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import PantallaAperturas
from Code.QT import PantallaEntMaq
from Code.QT import PantallaPGN
from Code.QT import PantallaSolo
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import TrListas
from Code import Util
from Code.QT import Voyager
from Code.Constantes import *


def pgn_pks(estado, pgn, jugada_inicial=None):
    unpgn = PGN.UnPGN()
    unpgn.leeTexto(pgn)
    if jugada_inicial:
        jg = unpgn.partida.jugada(jugada_inicial)
        si_blancas_abajo = jg.posicionBase.siBlancas
    else:
        si_blancas_abajo = True

    return dict(VOLTEO=False, liPGN=unpgn.listaCabeceras(), FEN=unpgn.dic.get("FEN", None), ESTADO=estado,
                BLOQUEAPERTURA=None, POSICAPERTURA=None, SIJUEGAMOTOR=False, PARTIDA=unpgn.partida.guardaEnTexto(),
                SIBLANCASABAJO=si_blancas_abajo)


def partida_pks(estado, partidaCompleta):
    fen = None if partidaCompleta.siFenInicial() else partidaCompleta.iniPosicion.fen()
    siBlancasAbajo = True if fen is None else " w " in fen
    return dict(VOLTEO=False, liPGN=partidaCompleta.liTags, FEN=fen, ESTADO=estado,
                BLOQUEAPERTURA=None, POSICAPERTURA=None, SIJUEGAMOTOR=False, PARTIDA=partidaCompleta.guardaEnTexto(),
                SIBLANCASABAJO=siBlancasAbajo)

# def pgn_json(estado, pgn, jugadaInicial=None):
#     unpgn = PGN.UnPGN()
#     unpgn.leeTexto(pgn)

#     def body(unpgn, dic):

#     dic = {}

#     dic["labels"] = dict(unpgn.dic)
#     li = []
#     for jg in unpgn.partida.liJugadas:
#         d = {}
#         d["fromto"] = jg.movimiento()
#         if jg.comentario:
#             d["comments"] = []
#             for uno in jg.comentario.split("\n"):
#                 d["comments"].append(uno)
#         if jg.variantes:
#             d["variants"] = []
#             for uno in jg.variantes.split("\n"):
#                 d["variants"].append(uno)
#         li.append(d)
#     dic["MOVES"] = li

#     return dic


class GestorSolo(Gestor.Gestor):
    def inicio(self, dic=None, fichero=None, pgn=None, jugadaInicial=None, siGrabar=True, siExterno=False):
        self.tipoJuego = kJugSolo

        siPGN = False
        if fichero:
            siExterno = True
            with open(fichero, "rb") as f:
                txt = f.read()
            dic = Util.txt2dic(txt)
            dic["ULTIMOFICHERO"] = fichero
        elif pgn:
            siPGN = True
            dic = pgn_pks(kJugando, pgn, jugadaInicial)

            self.resultadoPGN = None, None
        # elif dic is None:
        #     li = self.listaHistorico()
        #     if li:
        #         fichero = li[0]
        #         with open(fichero, "rb") as f:
        #             txt = f.read()
        #         dic = Util.txt2dic(txt)
        #         dic["ULTIMOFICHERO"] = fichero

        if dic is None:
            dic = {}
        dic["SIPGN"] = siPGN
        dic["SIEXTERNO"] = siExterno
        self.siExterno = siExterno
        self.siGrabar = siGrabar
        self.xjugadaInicial = jugadaInicial
        self.xfichero = fichero
        self.xpgn = pgn
        self.siPGN = siPGN
        self.reinicio = dic
        self.finExit = dic.get("FINEXIT", False)

        self.siJuegaHumano = True
        self.siJugamosConBlancas = True

        self.tablero.setAcceptDropPGNs(self.dropPGN)

        self.siJuegaPorMi = True
        self.dicRival = {}

        self.siJuegaMotor = dic.get("SIJUEGAMOTOR", False) if not self.xrival else True

        self.ultimoFichero = dic.get("ULTIMOFICHERO", "")
        self.siCambios = dic.get("SICAMBIOS", False)

        self.siVolteoAutomatico = dic.get("VOLTEO", False)

        if "liPGN" not in dic:
            hoy = Util.hoy()
            self.liPGN = []
            self.liPGN.append(["Event", _("Lucas Chess")])
            self.liPGN.append(["Date", "%d.%02d.%02d" % (hoy.year, hoy.month, hoy.day)])
            # jugador = self.configuracion.jugador
            # if len(jugador) == 0:
            #     jugador = "Unknown"
            # self.liPGN.append(["White", jugador])
            # self.liPGN.append(["Black", jugador])
        else:
            self.liPGN = dic["liPGN"]

        self.fen = dic.get("FEN", None)

        cp = ControlPosicion.ControlPosicion()
        if self.fen:
            cp.leeFen(self.fen)
        else:
            cp.posInicial()

        self.partida.reset(cp)

        self.estado = dic.get("ESTADO", kJugando)

        self.bloqueApertura = dic.get("BLOQUEAPERTURA", None)
        self.posicApertura = dic.get("POSICAPERTURA", None)  # Utilizada para reeditar

        if self.bloqueApertura:
            self.partida.reset()
            self.partida.leerPV(self.bloqueApertura.a1h8)
            self.partida.asignaApertura()

        if "PARTIDA" in dic:
            self.partida.reset()
            self.partida.recuperaDeTexto(dic["PARTIDA"])
            self.partida.asignaApertura()
            cp = self.partida.iniPosicion  # Para ver si las blancas abajo

        self.ponToolBar(siExterno, siGrabar)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.pantalla.ponRotulo1(dic.get("ROTULO1", None))
        self.pon_rotulo()
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.iniPosicion if siExterno else self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(dic.get("SIBLANCASABAJO", cp.siBlancas))
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        if self.partida.numJugadas() > 0:
            if jugadaInicial is None:
                self.mueveJugada(kMoverInicio if siExterno else kMoverFinal)
                jg = self.partida.jugada(0 if siExterno else -1)
            else:
                self.ponteEnJugada(jugadaInicial)
                jg = self.partida.jugada(jugadaInicial)
            self.ponFlechaSC(jg.desde, jg.hasta)

        if self.siPGN:
            self.pgnInicial = self.pgn.actual()

        self.ponPosicionDGT()

        self.refresh()

        if "SICAMBIORIVAL" in dic:
            self.cambioRival()
            del dic["SICAMBIORIVAL"]  # que no lo vuelva a pedir

        self.valor_inicial = self.dame_valor_actual()

        self.siguienteJugada()

    def pon_rotulo(self):
        li = []
        for label, rotulo in self.liPGN:
            if label.upper() == "WHITE":
                li.append("%s: %s" % (_("White"), rotulo))
            elif label.upper() == "BLACK":
                li.append("%s: %s" % (_("Black"), rotulo))
            elif label.upper() == "RESULT":
                li.append("%s: %s" % (_("Result"), rotulo))
        mensaje = "\n".join(li)
        self.ponRotulo2(mensaje)

    def dropPGN(self, pgn):
        unpgn = PantallaPGN.eligePartida(self.pantalla, pgn)
        if unpgn:
            self.leerpgn(unpgn)

    def tituloVentanaPGN(self):
        white = ""
        black = ""
        event = ""
        date = ""
        result = ""
        for clave, valor in self.liPGN:
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

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_atras:
            self.atras()

        elif clave == k_file:
            self.file()

        elif clave == k_reiniciar:
            self.reiniciar(self.reinicio)

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
            self.pantalla.reject()  # self.siPGN

        elif clave == k_grabarComo:
            self.grabarComo()

        elif clave == k_ayudaMover:
            self.ayudaMover(999)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def ponToolBar(self, siGrabarComo, siGrabar):
        if self.siPGN:
            if siGrabar:
                li = [k_grabar, k_cancelar, k_pgnInformacion, k_atras, k_ayudaMover, k_reiniciar, k_configurar,
                      k_utilidades]
            else:
                li = [k_finpartida, k_pgnInformacion, k_configurar, k_utilidades]
        else:
            li = [k_mainmenu, k_file, k_pgnInformacion, k_atras, k_ayudaMover, k_reiniciar,
                  k_configurar, k_utilidades]
            if self.finExit:
                li[0] = k_finpartida
        self.pantalla.ponToolBar(li)

    def finPartida(self):
        self.tablero.setAcceptDropPGNs(None)

        # Comprobamos que no haya habido cambios desde el ultimo grabado
        self.siCambios = self.siCambios or self.valor_inicial != self.dame_valor_actual()
        if self.siCambios and self.partida.numJugadas():
            resp = QTUtil2.preguntaCancelar(self.pantalla, _("Do you want to save changes to a file?"), _("Yes"), _("No"))
            if resp is None:
                return
            elif resp:
                self.grabarComo()

        if self.siExterno or self.finExit:
            self.procesador.procesarAccion(k_terminar)
            self.procesador.pararMotores()
            self.procesador.quitaKibitzers()
            self.procesador.pantalla.accept()
            sys.exit(0)
        else:
            if self.siPGN:
                self.pantalla.reject()
            else:
                self.procesador.inicio()

    def finalX(self):
        self.finPartida()
        return False

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando
        self.siJuegaHumano = True # necesario

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

        self.activaColor(siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.siJuegaHumano = True
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)

        if self.siJuegaMotor and not self.partida.siEstaTerminada():
            self.siJuegaMotor = False
            self.desactivaTodas()
            self.juegaRival()
            self.siJuegaMotor = True  # Como juega por mi pasa por aqui, para que no se meta en un bucle infinito

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
        for eti, valor in self.liPGN:
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

    def dame_valor_actual(self):
        dic = self.creaDic()
        dic["PARTIDA"] = self.partida.guardaEnTexto()
        return Util.dic2txt(dic)

    def creaDic(self):
        dic = {}
        dic["VOLTEO"] = self.siVolteoAutomatico
        dic["liPGN"] = self.liPGN
        dic["FEN"] = self.fen
        dic["ESTADO"] = self.estado
        dic["BLOQUEAPERTURA"] = self.bloqueApertura
        dic["POSICAPERTURA"] = self.posicApertura
        dic["SIJUEGAMOTOR"] = self.siJuegaMotor
        if self.dicRival and self.siJuegaMotor:
            dic["ROTULO1"] = self.dicRival["ROTULO1"]
        return dic

    def reiniciar(self, dic=None):
        if dic is None:
            dic = self.creaDic()
        self.inicio(dic, fichero=self.xfichero, pgn=self.xpgn, jugadaInicial=self.xjugadaInicial,
                    siGrabar=self.siGrabar, siExterno=self.siExterno)

    def editarEtiquetasPGN(self):
        resp = PantallaSolo.editarEtiquetasPGN(self.procesador, self.liPGN)
        if resp:
            self.liPGN = resp
            self.siCambios = True
            self.pon_rotulo()

    def guardaDir(self, resp):
        direc = os.path.dirname(resp)
        if direc != self.configuracion.dirJS:
            self.configuracion.dirJS = direc
            self.configuracion.graba()

    def grabarFichero(self, fichero):
        try:
            dic = self.creaDic()
            dic["PARTIDA"] = self.partida.guardaEnTexto()
            dic["ULTIMOFICHERO"] = fichero
            dic["SIBLANCASABAJO"] = self.tablero.siBlancasAbajo
            with open(fichero, "wb") as f:
                f.write(Util.dic2txt(dic))
            self.valor_inicial = self.dame_valor_actual()
            self.guardaDir(fichero)
            self.siCambios = False
            nombre = os.path.basename(fichero)
            QTUtil2.mensajeTemporal(self.pantalla, _X(_("Saved to %1"), nombre), 0.8)
            self.guardarHistorico(fichero)
            return True

        except:
            QTUtil2.mensError(self.pantalla, "%s : %s" % (_("Unable to save"), fichero))
            return False

    def grabarComo(self):
        extension = "pks"
        siConfirmar = True
        if self.ultimoFichero:
            fichero = self.ultimoFichero
        else:
            fichero = self.configuracion.dirJS
        while True:
            resp = QTUtil2.salvaFichero(self.pantalla, _("File to save"), fichero,
                                        _("File") + " %s (*.%s)" % (extension, extension),
                                        siConfirmarSobreescritura=siConfirmar)
            if resp:
                resp = str(resp)
                if not siConfirmar:
                    if os.path.abspath(resp) != os.path.abspath(self.ultimoFichero) and os.path.isfile(resp):
                        yn = QTUtil2.preguntaCancelar(self.pantalla,
                                                      _X(_("The file %1 already exists, what do you want to do?"),
                                                         resp), si=_("Overwrite"), no=_("Choose another"))
                        if yn is None:
                            break
                        if not yn:
                            continue
                if self.grabarFichero(resp):
                    self.ultimoFichero = resp
                    self.ponToolBar(True, True)
                return resp
            break
        return None

    def grabar(self):
        if self.ultimoFichero:
            self.grabarFichero(self.ultimoFichero)
        else:
            resp = self.grabarComo()
            if resp:
                self.ultimoFichero = resp
                self.ponToolBar(True, True)
        self.guardarHistorico(self.ultimoFichero)

    def grabarPGN(self):
        actual = self.pgn.actual()
        nuevoPGN = None if self.pgnInicial == actual else actual
        pv = self.partida.pv()
        dicPGN = Util.SymbolDict()
        for eti, valor in self.liPGN:
            dicPGN[eti] = valor
        self.procesador.valorPGN = nuevoPGN, pv, dicPGN

    def leeFichero(self, fich):
        f = open(fich, "rb")
        txt = f.read()
        f.close()
        self.guardaDir(fich)
        dic = Util.txt2dic(txt)
        dic["ULTIMOFICHERO"] = fich
        self.xfichero = None
        self.xpgn = None
        self.xjugadaInicial = None
        self.reiniciar(dic)
        self.ponToolBar(True, True)
        self.guardarHistorico(fich)

    def file(self):
        menu = QTVarios.LCMenu(self.pantalla)
        if self.ultimoFichero:
            menuR = menu.submenu(_("Save"), Iconos.Grabar())
            rpath = self.ultimoFichero
            if os.curdir[:1] == rpath[:1]:
                rpath = os.path.relpath(rpath)
                if rpath.count("..") > 0:
                    rpath = self.ultimoFichero
            menuR.opcion("save", "%s: %s" %( _("Save"), rpath), Iconos.Grabar())
            menuR.separador()
            menuR.opcion("saveas", _("Save as"), Iconos.GrabarComo())
        else:
            menu.opcion("save", _("Save"), Iconos.Grabar())
        menu.separador()
        menu.opcion("new", _("New"), Iconos.TutorialesCrear())
        menu.separador()
        menu.opcion("open", _("Open"), Iconos.Recuperar())
        menu.separador()
        li = self.listaHistorico()
        if li:
            menu.separador()
            menuR = menu.submenu(_("Reopen"), Iconos.Historial())
            for path in li:
                menuR.opcion("reopen_%s" % path, path, Iconos.PuntoNaranja())
                menuR.separador()
        resp = menu.lanza()
        if resp is None:
            return
        if resp == "open":
            self.recuperarPKS()
        elif resp == "new":
            self.nuevo()
        elif resp.startswith("reopen_"):
            return self.leeFichero(resp[7:])
        elif resp == "save":
            self.grabar()
        elif resp == "saveas":
            self.grabarComo()

    # def recuperar(self):
    #     menu = QTVarios.LCMenu(self.pantalla)
    #     menu.opcion("open", _("File") + " ...", Iconos.Recuperar())
    #     menu.separador()

    #     resp = menu.lanza()
    #     if resp:
    #         if resp == "open":
    #             return self.recuperarPKS()
    #         if resp == "new":
    #             return self.nuevo()
    #         return self.leeFichero(resp[2:])

    def nuevo(self):
        self.xfichero = None
        self.xpgn = None
        self.xjugadaInicial = None
        self.reiniciar({})
        self.ponToolBar(False, True)

    def recuperarPKS(self):
        resp = QTUtil2.leeFichero(self.pantalla, self.configuracion.dirJS, "pks")
        if resp:
            self.leeFichero(resp)

    def listaHistorico(self):
        dic = self.configuracion.leeVariables("FICH_GESTORSOLO")
        if dic:
            li = dic.get("HISTORICO")
            if li:
                return [f for f in li if os.path.isfile(f)]
        return []

    def guardarHistorico(self, path):
        path = Util.dirRelativo(path)

        dic = self.configuracion.leeVariables("FICH_GESTORSOLO")
        if not dic:
            dic = {}
        lista = dic.get("HISTORICO", [])
        if path in lista:
            lista.pop(lista.index(path))
        lista.insert(0, path)
        dic["HISTORICO"] = lista[:20]
        self.configuracion.escVariables("FICH_GESTORSOLO", dic)

    def informacion(self):
        menu = QTVarios.LCMenu(self.pantalla)
        f = Controles.TipoLetra(puntos=10, peso=75)
        menu.ponFuente(f)

        siOpening = False
        for clave, valor in self.liPGN:
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
        mt = _("Engine").lower()
        mt = _X(_("Disable %1"), mt) if self.siJuegaMotor else _X(_("Enable %1"), mt)

        sep = (None, None, None)

        liMasOpciones = [
            ("rotacion", _("Auto-rotate board"), Iconos.JS_Rotacion()), sep,
            ("apertura", _("Opening"), Iconos.Apertura()), sep,
            ("posicion", _("Edit start position"), Iconos.Datos()), sep,
            ("pasteposicion", _("Paste FEN position"), Iconos.Pegar16()), sep,
            ("leerpgn", _("Read PGN"), Iconos.PGN_Importar()), sep,
            ("pastepgn", _("Paste PGN"), Iconos.Pegar16()), sep,
            ("motor", mt, Iconos.Motores()), sep,
            ("voyager", _("Voyager 2"), Iconos.Voyager1()),
        ]
        resp = self.configurar(liMasOpciones, siCambioTutor=True, siSonidos=True)

        if resp == "rotacion":
            self.siVolteoAutomatico = not self.siVolteoAutomatico
            siBlancas = self.partida.ultPosicion.siBlancas
            if self.siVolteoAutomatico:
                if siBlancas != self.tablero.siBlancasAbajo:
                    self.tablero.rotaTablero()
        elif resp == "apertura":
            me = self.unMomento()
            w = PantallaAperturas.WAperturas(self.pantalla, self.configuracion, self.bloqueApertura)
            me.final()
            if w.exec_():
                self.bloqueApertura = w.resultado()
                # self.posicApertura = ps
                self.fen = None
                self.xfichero = None
                self.xpgn = None
                self.xjugadaInicial = None
                self.reiniciar()

        elif resp == "posicion":
            self.startPosition()

        elif resp == "pasteposicion":
            texto = QTUtil.traePortapapeles()
            if texto:
                cp = ControlPosicion.ControlPosicion()
                try:
                    cp.leeFen(str(texto))
                    self.xfichero = None
                    self.xpgn = None
                    self.xjugadaInicial = None
                    self.fen = cp.fen()
                    self.bloqueApertura = None
                    self.posicApertura = None
                    self.reiniciar()
                except:
                    pass

        elif resp == "leerpgn":
            self.leerpgn()

        elif resp == "pastepgn":
            texto = QTUtil.traePortapapeles()
            if texto:
                unpgn = PGN.UnPGN()
                unpgn.leeTexto(texto)
                if unpgn.siError:
                    QTUtil2.mensError(self.pantalla, _("The text from the clipboard does not contain a chess game in PGN format"))
                    return
                self.xfichero = None
                self.xpgn = None
                self.xjugadaInicial = None
                self.bloqueApertura = None
                self.posicApertura = None
                self.fen = unpgn.dic.get("FEN", None)
                dic = self.creaDic()
                dic["PARTIDA"] = unpgn.partida.guardaEnTexto()
                dic["liPGN"] = unpgn.listaCabeceras()
                dic["FEN"] = self.fen
                dic["SIBLANCASABAJO"] = unpgn.partida.ultPosicion.siBlancas
                self.reiniciar(dic)

        elif resp == "motor":
            self.ponRotulo1("")
            if self.siJuegaMotor:
                if self.xrival:
                    self.xrival.terminar()
                    self.xrival = None
                self.siJuegaMotor = False
            else:
                self.cambioRival()

        elif resp == "voyager":
            ptxt = Voyager.voyagerPartida(self.pantalla, self.partida)
            if ptxt:
                self.xfichero = None
                self.xpgn = None
                self.xjugadaInicial = None
                dic = self.creaDic()
                dic["PARTIDA"] = ptxt
                p = self.partida.copia()
                p.recuperaDeTexto(ptxt)
                dic["FEN"] = None if p.siFenInicial() else p.iniPosicion.fen()
                dic["SIBLANCASABAJO"] = self.tablero.siBlancasAbajo
                self.reiniciar(dic)

    def leerpgn(self, unpgn=None):
        if unpgn is None:
            unpgn = PantallaPGN.eligePartida(self.pantalla)
        if unpgn:
            self.bloqueApertura = None
            self.posicApertura = None
            self.fen = unpgn.dic.get("FEN", None)
            self.xfichero = None
            self.xpgn = None
            self.xjugadaInicial = None
            dic = self.creaDic()
            dic["PARTIDA"] = unpgn.partida.guardaEnTexto()
            dic["liPGN"] = unpgn.listaCabeceras()
            dic["FEN"] = self.fen
            dic["SIBLANCASABAJO"] = unpgn.partida.ultPosicion.siBlancas
            self.reiniciar(dic)

    def controlTeclado(self, nkey):
        if nkey == Qt.Key_V:
            self.paste(QTUtil.traePortapapeles())
        elif nkey == Qt.Key_T:
            li = [self.fen if self.fen else ControlPosicion.FEN_INICIAL,"",self.partida.pgnBaseRAW()]
            self.saveSelectedPosition("|".join(li))
        elif nkey == Qt.Key_S:
            self.startPosition()

    def listHelpTeclado(self):
        return [
            ("V", _("Paste position")),
            ("T", _("Save position in 'Selected positions' file")),
            ("S", _("Set start position")),
        ]

    def startPosition(self):
        resp = Voyager.voyagerFEN(self.pantalla, self.fen)
        if resp is not None:
            self.xfichero = None
            self.xpgn = None
            self.xjugadaInicial = None
            self.fen = resp
            self.bloqueApertura = None
            self.posicApertura = None

            self.reiniciar()

    def paste(self, texto):
        cp = ControlPosicion.ControlPosicion()
        try:
            if "." in texto or '"' in texto:
                unpgn = PGN.UnPGN()
                unpgn.leeTexto(texto)
                if unpgn.siError:
                    return
                self.bloqueApertura = None
                self.posicApertura = None
                self.fen = unpgn.dic.get("FEN", None)
                self.xfichero = None
                self.xpgn = None
                self.xjugadaInicial = None
                dic = self.creaDic()
                dic["PARTIDA"] = unpgn.partida.guardaEnTexto()
                dic["liPGN"] = unpgn.listaCabeceras()
                dic["FEN"] = self.fen
                dic["SIBLANCASABAJO"] = unpgn.partida.ultPosicion.siBlancas
                self.reiniciar(dic)
            else:
                cp.leeFen(str(texto))
                self.xfichero = None
                self.xpgn = None
                self.xjugadaInicial = None
                self.fen = cp.fen()
                self.bloqueApertura = None
                self.posicApertura = None
                self.reiniciar()
        except:
            pass

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
        dicBase["SIBLANCAS"] = self.partida.iniPosicion.siBlancas

        dic = self.dicRival = PantallaEntMaq.cambioRival(self.pantalla, self.configuracion, dicBase, siGestorSolo=True)

        if dic:
            for k, v in dic.iteritems():
                self.reinicio[k] = v

            dr = dic["RIVAL"]
            rival = dr["CM"]
            if hasattr(rival, "icono"):
                delattr(rival, "icono") # problem with configuracion.escVariables and saving qt variables
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
            self.siJugamosConBlancas = dic["SIBLANCAS"]
            if self.partida.ultPosicion.siBlancas != self.siJugamosConBlancas and not self.partida.siEstaTerminada():
                self.siJuegaMotor = False
                self.desactivaTodas()
                self.juegaRival()
                self.siJuegaMotor = True

    def atras(self):
        if self.partida.numJugadas():
            self.partida.anulaSoloUltimoMovimiento()
            if self.siJuegaMotor:
                self.partida.anulaSoloUltimoMovimiento()
            if not self.fen:
                self.partida.asignaApertura()
            self.ponteAlFinal()
            self.estado = kJugando
            self.refresh()
            self.siguienteJugada()

