import os
import sys
import time

from Code.Constantes import *
import Code.Util as Util
import Code.ControlPosicion as ControlPosicion
import Code.Jugada as Jugada
import Code.PGN as PGN
import Code.TrListas as TrListas
import Code.Gestor as Gestor
import Code.Voice as Voice
import Code.XVoyager as XVoyager
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.QTVarios as QTVarios
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.PantallaAperturas as PantallaAperturas
import Code.QT.PantallaPGN as PantallaPGN
import Code.QT.PantallaEntMaq as PantallaEntMaq
import Code.QT.PantallaSolo as PantallaSolo
import Code.QT.WinPosition as WinPosition

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

# def pgn_json(estado, pgn, jugadaInicial=None):
# unpgn = PGN.UnPGN()
# unpgn.leeTexto(pgn)
# dic = {}

# dic["labels"] = dict(unpgn.dic)
# li = []
# for jg in unpgn.partida.liJugadas:
# d = {}
# d["pv"] = jg.movimiento()
# if jg.comentario:
# d["comments"] = []
# for uno in jg.comentario.split("\n"):
# d["comments"].append(uno)
# if jg.variantes:
# d["variants"] = []
# for uno in jg.variantes.split("\n"):
# d["variants"].append(uno)
# li.append(d)
# dic["MOVES"] = li

# return dic

class GestorSolo(Gestor.Gestor):
    def inicio(self, dic=None, fichero=None, pgn=None, jugadaInicial=None, siGrabar=True, siExterno=False):
        self.tipoJuego = kJugSolo

        siPGN = False
        if fichero:
            siExterno = True
            f = open(fichero, "rb")
            txt = f.read()
            f.close()
            dic = Util.txt2dic(txt)
            dic["ULTIMOFICHERO"] = fichero
        elif pgn:
            siPGN = True
            dic = pgn_pks(kJugando, pgn, jugadaInicial)
            self.resultadoPGN = None, None

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

        self.siJuegaPorMi = True
        self.dicRival = {}

        self.siJuegaMotor = dic.get("SIJUEGAMOTOR", False)

        self.ultimoFichero = dic.get("ULTIMOFICHERO", "")
        self.siCambios = dic.get("SICAMBIOS", False)

        self.siVolteoAutomatico = dic.get("VOLTEO", False)

        if "liPGN" not in dic:
            hoy = Util.hoy()
            self.liPGN = []
            self.liPGN.append(["Event", _("Lucas Chess")])
            self.liPGN.append(["Date", "%d.%02d.%02d" % (hoy.year, hoy.month, hoy.day)])
            jugador = self.configuracion.jugador
            if len(jugador) == 0:
                jugador = "Unknown"
            self.liPGN.append(["White", jugador])
            self.liPGN.append(["Black", jugador])
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
            self.listaAperturasStd.asignaApertura(self.partida)

        if "PARTIDA" in dic:
            self.partida.reset()
            self.partida.recuperaDeTexto(dic["PARTIDA"])
            self.listaAperturasStd.asignaApertura(self.partida)
            cp = self.partida.iniPosicion  # Para ver si las blancas abajo

        self.ponToolBar(siExterno, siGrabar)

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.pantalla.ponRotulo1(dic.get("ROTULO1", None))
        self.pantalla.ponRotulo2(None)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.iniPosicion if siExterno else self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(dic.get("SIBLANCASABAJO", cp.siBlancas))
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        if self.partida.numJugadas() > 0:
            if jugadaInicial is None:
                self.mueveJugada(kMoverInicio if siExterno else kMoverFinal)
                jg = self.partida.liJugadas[0 if siExterno else -1]
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

        self.activeVoice = False

        self.siguienteJugada()

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

        elif clave == k_reiniciar:
            self.reiniciar(self.reinicio)

        elif clave == k_configurar:
            self.configurarGS()

        elif clave == k_utilidades:
            liMasOpciones = (
                ( "libros", _("Consult a book"), Iconos.Libros() ),
                ( None, None, None ),
                ( "bookguide", _("Personal Opening Guide"), Iconos.BookGuide() ),
                ( None, None, None ),
                ( "play", _('Play current position'), Iconos.MoverJugar() )
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

        elif clave == k_grabar:
            if self.siPGN:
                self.grabarPGN()
                self.pantalla.accept()
            else:
                self.grabar()

        elif clave in (k_cancelar, k_finpartida):
            self.pantalla.reject()  # self.siPGN

        elif clave == k_grabarComo:
            self.grabarComo()

        elif clave == k_recuperar:
            self.recuperar()

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
            li = [k_mainmenu, k_recuperar, k_grabar, k_grabarComo, k_pgnInformacion, k_atras, k_ayudaMover, k_reiniciar,
                  k_configurar, k_utilidades]
            if not siGrabarComo:
                del li[2]
            if self.finExit:
                li[0] = k_finpartida
        self.pantalla.ponToolBar(li)

    def finPartida(self):
        # Comprobamos que no haya habido cambios desde el ultimo grabado
        if self.siCambios and self.partida.numJugadas():
            resp = QTUtil2.preguntaCancelar(self.pantalla, _("Do you want to save changes to a file?"), _("Yes"),
                                            _("No"))
            if resp is None:
                return
            elif resp:
                self.grabarComo()

        if self.activeVoice:
            self.setVoice( False )

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

        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas
        self.siJugamosConBlancas = siBlancas  # Compatibilidad, sino no funciona el cambio en pgn
        if self.siVolteoAutomatico:
            time.sleep(1)
            if siBlancas != self.tablero.siBlancasAbajo:
                self.tablero.rotaTablero()

        if self.partida.numJugadas() > 0:
            jgUltima = self.partida.liJugadas[-1]
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
        # Peon coronando
        if not coronacion and self.partida.ultPosicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.partida.ultPosicion.siBlancas)
            if coronacion is None:
                return False

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)

        if siBien:

            self.movimientosPiezas(jg.liMovs)

            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, True)
            self.error = ""

            if self.siJuegaMotor and not self.partida.siEstaTerminada():
                self.siJuegaMotor = False
                self.desactivaTodas()
                self.juegaRival()
                self.siJuegaMotor = True  # Como juega por mi pasa por aqui, para que no se meta en un bucle infinito

            self.siguienteJugada()
            return True
        else:
            self.error = mens
            return False

    def masJugada(self, jg, siNuestra):

        self.siCambios = True

        # Preguntamos al mono si hay movimiento
        if self.siTerminada():
            jg.siJaqueMate = jg.siJaque
            jg.siAhogado = not jg.siJaque

        self.partida.liJugadas.append(jg)
        if self.partida.pendienteApertura:
            self.listaAperturasStd.asignaApertura(self.partida)

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
        """
        Llamado para grabar el pgn
        """
        resp = ""
        st = set()
        for eti, valor in self.liPGN:
            etiU = eti.upper()
            if etiU in st:
                continue
            st.add(etiU)
            # if not self.siPGN:
            # if eti in ( "ECO", "FEN" ):
            # tit = eti
            # else:
            # tit = eti[0].upper()+eti[1:].lower()
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
            f = open(fichero, "wb")
            f.write(Util.dic2txt(dic))
            f.close()
            self.guardaDir(fichero)
            self.siCambios = False
            nombre = os.path.basename(fichero)
            QTUtil2.mensajeTemporal(self.pantalla, _X(_("Saved to %1"), nombre), 0.8)
            self.guardarHistorico(fichero)
            return True

        except:
            QTUtil2.mensError(self.pantalla, "%s : %s" % ( _("Unable to save"), fichero ))
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
        self.reiniciar(dic)
        self.ponToolBar(True, True)
        self.guardarHistorico(fich)

    def recuperar(self):
        menu = QTVarios.LCMenu(self.pantalla)

        li = self.listaHistorico()

        menu.opcion("open", _("File") + " ...", Iconos.Recuperar())
        menu.separador()
        menu.opcion("new", _("New"), Iconos.TutorialesCrear())

        if li:
            menu.separador()
            menuR = menu.submenu(_("Reopen"), Iconos.Historial())
            for path in li:
                menuR.opcion("r_%s" % path, path, Iconos.PuntoNaranja())
                menuR.separador()

        resp = menu.lanza()
        if resp:
            if resp == "open":
                return self.recuperarPKS()
            if resp == "new":
                return self.nuevo()
            return self.leeFichero(resp[2:])

    def nuevo(self):
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
            siFecha = clave.upper().endswith("DATE")
            trad = TrListas.pgnLabel(clave)
            if trad != clave:
                clave = trad
                # else:
                # clave = clave[0].upper()+clave[1:].lower()
            if siFecha:
                valor = valor.replace(".??", "")
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
                menu.opcion(clave, rotulo, Iconos.PuntoNaranja())

        menu.separador()
        menu.opcion("pgn", _("Edit PGN labels"), Iconos.PGN())

        resp = menu.lanza()
        if resp:
            self.editarEtiquetasPGN()

    def configurarGS(self):
        # self.test()
        # return

        mt = _("Engine").lower()
        mt = _X(_("Disable %1"), mt) if self.siJuegaMotor else _X(_("Enable %1"), mt)

        sep = ( None, None, None )

        liMasOpciones = [
            ( "rotacion", _("Auto-rotate board"), Iconos.JS_Rotacion() ), sep,
            ( "apertura", _("Opening"), Iconos.Apertura() ), sep,
            ( "posicion", _("Start position"), Iconos.Datos() ), sep,
            ( "pasteposicion", _("Paste FEN position"), Iconos.Pegar16() ), sep,
            ( "leerpgn", _("Read PGN"), Iconos.PGN_Importar() ), sep,
            ( "pastepgn", _("Paste PGN"), Iconos.Pegar16() ), sep,
            ( "motor", mt, Iconos.Motores() ), sep,
            ( "voyager", _("Voyager 2").replace("2", "1"), Iconos.Voyager1() ),
        ]
        if self.configuracion.voice:
            liMasOpciones.append(sep)
            if self.activeVoice:
                liMasOpciones.append(( "desvoice", _("Deactivate voice"), Iconos.X_Microfono() ))
            else:
                liMasOpciones.append(( "actvoice", _("Activate voice"), Iconos.S_Microfono() ))
        resp = self.configurar(liMasOpciones, siCambioTutor=True, siSonidos=True)

        if resp == "rotacion":
            self.siVolteoAutomatico = not self.siVolteoAutomatico
            siBlancas = self.partida.ultPosicion.siBlancas
            if self.siVolteoAutomatico:
                if siBlancas != self.tablero.siBlancasAbajo:
                    self.tablero.rotaTablero()
        elif resp == "apertura":
            bl, ps = PantallaAperturas.dameApertura(self.pantalla, self.configuracion, self.bloqueApertura,
                                                    self.posicApertura)
            if bl:
                self.bloqueApertura = bl
                self.posicApertura = ps
                self.fen = None
                self.reiniciar()

        elif resp == "posicion":
            resp = WinPosition.editarPosicion(self.pantalla, self.configuracion, self.fen)
            if resp is not None:
                self.fen = resp
                self.bloqueApertura = None
                self.posicApertura = None

                if self.xpgn:
                    siInicio = self.fen == ControlPosicion.FEN_INICIAL
                    li = self.xpgn.split("\n")
                    lin = []
                    siFen = False
                    for linea in li:
                        if linea.startswith("["):
                            if "FEN " in linea:
                                siFen = True
                                if siInicio:
                                    continue
                                linea = '[FEN "%s"]' % self.fen
                            lin.append(linea)
                        else:
                            break
                    if not siFen:
                        linea = '[FEN "%s"]' % self.fen
                        lin.append(linea)
                    self.liPGN = lin
                    self.xpgn = "\n".join(lin) + "\n\n*"

                self.reiniciar()

        elif resp == "pasteposicion":
            texto = QTUtil.traePortapapeles()
            if texto:
                cp = ControlPosicion.ControlPosicion()
                try:
                    cp.leeFen(str(texto))
                    self.fen = cp.fen()
                    self.bloqueApertura = None
                    self.posicApertura = None
                    self.reiniciar()
                except:
                    pass

        elif resp == "leerpgn":
            unpgn = PantallaPGN.eligePartida(self.pantalla)
            if unpgn:
                self.bloqueApertura = None
                self.posicApertura = None
                self.fen = unpgn.dic.get("FEN", None)
                dic = self.creaDic()
                dic["PARTIDA"] = unpgn.partida.guardaEnTexto()
                dic["liPGN"] = unpgn.listaCabeceras()
                dic["FEN"] = self.fen
                dic["SIBLANCASABAJO"] = unpgn.partida.ultPosicion.siBlancas
                self.reiniciar(dic)

        elif resp == "pastepgn":
            texto = QTUtil.traePortapapeles()
            if texto:
                unpgn = PGN.UnPGN()
                unpgn.leeTexto(texto)
                if unpgn.siError:
                    QTUtil2.mensError(self.pantalla, _("The text from the clipboard does not contain a chess game in PGN format"))
                    return
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
                self.xrival.terminar()
                self.xrival = None
                self.siJuegaMotor = False
            else:
                self.cambioRival()

        elif resp == "voyager":
            ptxt = XVoyager.xVoyager(self.pantalla, self.configuracion, self.partida)
            if ptxt:
                dic = self.creaDic()
                dic["PARTIDA"] = ptxt
                p = self.partida.copia()
                p.recuperaDeTexto(ptxt)
                dic["FEN"] = None if p.siFenInicial() else p.iniPosicion.fen()
                dic["SIBLANCASABAJO"] = self.tablero.siBlancasAbajo
                self.reiniciar(dic)

        elif resp == "actvoice":
            self.setVoice( True )

        elif resp == "desvoice":
            self.setVoice( False )

    def controlTeclado(self, nkey):
        if nkey in (86, 80):  # V,P
            self.paste(QTUtil.traePortapapeles())

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
                dic = self.creaDic()
                dic["PARTIDA"] = unpgn.partida.guardaEnTexto()
                dic["liPGN"] = unpgn.listaCabeceras()
                dic["FEN"] = self.fen
                dic["SIBLANCASABAJO"] = unpgn.partida.ultPosicion.siBlancas
                self.reiniciar(dic)
            else:
                cp.leeFen(str(texto))
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
                self.listaAperturasStd.asignaApertura(self.partida)
            self.ponteAlFinal()
            self.estado = kJugando
            self.refresh()
            self.siguienteJugada()

    def setVoice(self, ok):
        if ok:
            Voice.runVoice.start(self.voice)
            self.activeVoice = True
            self.liVoice = []
        else:
            Voice.runVoice.stop()
            self.activeVoice = False

    # def test(self):
        # if not hasattr(self,"litest"):
            # li = "e4 e5 Nf3 Nc6 Bb5 Nf6 O-O Nxe4 d4 Nd6 Bxc6 dxc6 dxe5 Nf5 "\
                           # "Qxd8+ Kxd8 Nc3 Bd7 h3 h6 Rd1 Kc8 a4 a5 b3 b6 Bb2 Ne7 "\
                           # "Rd2 c5 Ne2 Ng6 Rad1 Be6 c4 Be7 Nc3 Kb7 Nd5 Rad8 Nxe7 "\
                           # "Rxd2 Nxd2 Nxe7 Nf1 Kc8 f3 g5 Ng3 Rd8 Rxd8+ Kxd8 Kf2 Bf5 "\
                           # "Nxf5 Nxf5 g4 Nd4 Bxd4 cxd4 Ke2 Kd7 Kd3 c5 Ke4 Ke6 f4 "\
                           # "gxf4 Kxf4 d3 Ke3 Kxe5 Kxd3 Kf4 Ke2 Kg3 Ke3 Kxh3 Kf4 Kh4 "\
                           # "Kf5 Kg3".split(" ")
            # self.litest = li
            # self.postest = 0
            # self.liVoice = []
        # c = self.litest[self.postest]
        # if c[0] == "O":
            # self.voice(c)
        # else:
            # self.voice(" ".join(list(c)))
        # self.postest += 1

    def voice(self, txt):
        if "TAKEBACK" in txt:
            self.atras()
            self.liVoice = []
            return
        self.liVoice.extend(txt.split(" "))
        while True:
            resp = Voice.readPGN(self.liVoice, self.partida.ultPosicion)
            if resp is None:
                self.liVoice = []
                self.beepExtendido(True)
                return
            else:
                siMove, move, nGastados = resp
                if siMove:
                    self.mueveHumano(move.desde(), move.hasta(), move.coronacion())
                    n = len(self.liVoice)
                    if nGastados >= n:
                        self.liVoice = []
                        return
                    else:
                        self.liVoice = self.liVoice[nGastados:]
                        continue
                else:
                    return
