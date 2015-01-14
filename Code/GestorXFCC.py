# -*- coding: latin-1 -*-

import os
import urllib2
import atexit

import suds
from suds.client import Client

from Code.Constantes import *
import Code.Util as Util
import Code.Jugada as Jugada
import Code.PGN as PGN
import Code.TrListas as TrListas
import Code.SQL.Base as Base
import Code.Gestor as Gestor
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.QTVarios as QTVarios
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.PantallaXFCC as PantallaXFCC
import Code.QT.FormLayout as FormLayout

def dicServers():
    return Util.ini2dic("IntFiles/xfcc.ini")

class DB_XFCC:
    def __init__(self, carpeta, server, user, dicServ, password=None, ):
        self.fichero = os.path.join(carpeta, "%s_%s.xfcc" % (server, user))
        self.server = server
        self.user = user
        self.wsdl = dicServ["wsdl"]
        self.url = dicServ["url"]

        self.db = Base.DBBase(self.fichero)

        self.tabla = "datos"

        if not self.db.existeTabla(self.tabla):
            self.creaTabla()

        self.dbf = self.db.dbf(self.tabla, "ID")

        dbConfig = Util.DicSQL(self.fichero, tabla="config")
        if password:
            dbConfig["password"] = Util.xor_crypt(password, "xfcc")
            self.password = password
        else:
            self.password = Util.xor_crypt(dbConfig["password"], "xfcc")
        dbConfig.close()

        atexit.register(self.close)

        self.reset()

    def reset(self):
        self.dbf.ponSelect(
            "ID,TURN,WHITE,BLACK,EVENT,TIMEPLAYER,TIMEOPPONENT,WHITEELO,BLACKELO,EVENTDATE,TIMECONTROL,DICREC,DICGAME,RESULT")
        self.dbf.ponOrden("TURN,ID DESC")
        self.dbf.ponCondicion("")
        self.dbf.leer()

    def creaTabla(self):
        tb = Base.TablaBase(self.tabla)
        tb.nuevoCampo("ID", "INTEGER", primaryKey=True)
        tb.nuevoCampo("TURN", "INTEGER")  # 0 : + move user, 1: - move oponent, 2: x finished
        tb.nuevoCampo("WHITE", "VARCHAR")
        tb.nuevoCampo("BLACK", "VARCHAR")
        tb.nuevoCampo("EVENT", "VARCHAR")
        tb.nuevoCampo("TIMEPLAYER", "VARCHAR")
        tb.nuevoCampo("TIMEOPPONENT", "VARCHAR")
        tb.nuevoCampo("WHITEELO", "INTEGER")
        tb.nuevoCampo("BLACKELO", "INTEGER")
        tb.nuevoCampo("EVENTDATE", "VARCHAR")
        tb.nuevoCampo("TIMECONTROL", "VARCHAR")
        tb.nuevoCampo("DICREC", "BLOB")  # info leida
        tb.nuevoCampo("DICGAME", "BLOB")  # info editada
        tb.nuevoCampo("RESULT", "VARCHAR")  # *, 1-0, 0-1, 1/2-1/2
        tb.nuevoIndice("IND_TURN", "TURN")
        self.db.generarTabla(tb)

    def close(self):
        if self.dbf:
            self.dbf.cerrar()
            self.dbf = None
        self.db.cerrar()

    def __len__(self):
        return self.dbf.reccount()

    def recno(self):
        return self.dbf.recno

    def goto(self, reg):
        return self.dbf.goto(reg)

    def registroActual(self):
        reg = self.dbf.registroActual()
        reg.DICREC = Util.blob2var(reg.DICREC)
        if reg.DICGAME:
            reg.DICGAME = Util.blob2var(reg.DICGAME)

        unpgn = PGN.UnPGN()
        unpgn.leeTexto(reg.DICREC["moves"])
        rd = reg.DICREC
        d = unpgn.dic
        d["Event"] = reg.EVENT
        d["Site"] = rd["site"]
        d["Date"] = rd.get("eventDate", "?")
        d["White"] = rd["white"]
        d["Black"] = rd["black"]
        d["Result"] = reg.RESULT
        if rd["whiteElo"]:
            d["WhiteElo"] = str(rd["whiteElo"])
        if rd["blackElo"]:
            d["BlackElo"] = str(rd["blackElo"])

        reg.partida = unpgn.partida
        reg.liCabeceras = unpgn.listaCabeceras()

        reg.drawOffered = rd["drawOffered"]
        reg.opponent = reg.BLACK if rd["hasWhite"] else reg.WHITE

        return reg

    def guardaGame(self, dic):
        reg = Util.Almacen()
        reg.DICGAME = Util.var2blob(dic)
        self.dbf.modificarReg(self.dbf.recno, reg)

    def getMyGames(self):
        try:
            client = Client(self.wsdl)
        except urllib2.URLError as e:
            return e.reason

        request = client.factory.create('GetMyGames')
        request.username = self.user
        request.password = self.password
        try:
            response = client.service.GetMyGames(request)
        except suds.WebFault as e:
            return str(e)

        # Miramos los games recibidos
        for game in response.XfccGame:

            dicrec = {}
            for atrib in dir(game):
                if not atrib.startswith("_"):
                    dicrec[atrib] = getattr(game, atrib)

            daysPlayer = game.daysPlayer
            hoursPlayer = game.hoursPlayer
            # minutesPlayer = game.minutesPlayer
            timePlayer = "%d %s %d %s" % ( daysPlayer, _("days"), hoursPlayer, _("hours") )

            daysOpponent = game.daysOpponent
            hoursOpponent = game.hoursOpponent
            # minutesOpponent = game.minutesOpponent
            timeOpponent = "%d %s %d %s" % ( daysOpponent, _("days"), hoursOpponent, _("hours") )

            timeControl = "?"
            if hasattr(game, "timeControl"):
                li = game.timeControl.split("+")
                if li:
                    t = li[0]
                    if t.isdigit():
                        undia = 60 * 60 * 24
                        dias = int(t) / undia
                        timeControl = "%d%s" % (dias, _("days"))
                        if len(li) == 2:
                            mas = li[1]
                            if mas.isdigit():
                                timeControl += "+%d%s" % (int(mas) / undia, _("days"))

            result = ""
            if hasattr(game, "result"):
                r = game.result
                if r != "Ongoing":
                    if r in ( "WhiteWins", "WhiteWinAdjudicated", "WhiteDefaulted" ):
                        result = "1-0"
                    elif r in ( "BlackWins", "BlackWinAdjudicated", "BlackDefaulted" ):
                        result = "0-1"
                    elif r in ( "Draw", "DrawAdjudicated", "BothDefaulted" ):
                        result = "1/2-1/2"

            id = game.id

            reg = Util.Almacen()
            reg.TURN = 0 if game.myTurn else ( 1 if game.result == "Ongoing" else 2 )
            reg.WHITE = game.white
            reg.BLACK = game.black
            reg.EVENT = game.event
            reg.TIMEPLAYER = timePlayer
            reg.TIMEOPPONENT = timeOpponent
            reg.WHITEELO = getattr(game, "whiteElo", 0)
            reg.BLACKELO = getattr(game, "blackElo", 0)
            reg.EVENTDATE = getattr(game, "eventDate", "?")
            reg.TIMECONTROL = timeControl
            reg.DICREC = Util.var2blob(dicrec)
            reg.RESULT = result

            # Comprobamos si está
            self.dbf.ponSelect("ID")
            self.dbf.ponCondicion("ID = %d" % id)
            self.dbf.leer()
            siEsta = self.dbf.reccount() > 0

            if siEsta:
                self.dbf.goto(0)
                self.dbf.modificarReg(0, reg)

            else:
                reg.ID = id
                self.dbf.insertarSoloReg(reg)

        self.reset()

        return None

    def siPendienteTurnoOtro(self):
        self.dbf.ponSelect("ID")
        self.dbf.ponCondicion("TURN = 1")
        self.dbf.leer()
        resp = self.dbf.reccount() > 0
        self.reset()
        return resp

    def makeAMove(self, resp, mensaje, jg, movecount):
        try:
            client = Client(self.wsdl)
        except urllib2.URLError as e:
            return False, e.reason

        request = client.factory.create('MakeAMove')
        request.username = self.user
        request.password = self.password
        request.gameId = self.dbf.ID
        request.resign = resp == "resign"
        request.acceptDraw = resp == "acceptDraw"
        request.movecount = movecount
        if resp == "play":
            request.myMove = jg.pgnBase
        request.offerDraw = resp == "offerDraw"
        request.claimDraw = resp == "claimDraw"
        request.myMessage = mensaje

        try:
            response = client.service.MakeAMove(request)
        except suds.WebFault as e:
            return False, str(e)

        d = {"Success": _("Success"),
             "ServerError": _("Server error"),
             "AuthenticationFailed": _("Authentication failed"),
             "InvalidGameID": _("Invalid game ID"),
             "NotYourGame": _("Not your game"),
             "NotYourTurn": _("Not your turn"),
             "InvalidMoveNumber": _("Invalid move number"),
             "InvalidMove": _("Invalid move"),
             "NoDrawWasOffered": _("No draw was offered"),
             "LostOnTime": _("Lost on time"),
             "YouAreOnLeave": _("You are on leave"),
             "MoveIsAmbigous": _("Move is ambiguous"),
             "FeatureUnavailable": _("Feature unavailable"), }
        resp = response
        return resp == "Success", d.get(resp, _("Unknown"))

class GestorXFCC(Gestor.Gestor):
    def inicio(self, db):

        self.tipoJuego = kJugXFCC
        self.db = db

        self.regActual = db.registroActual()
        self.reajustar()

        self.estado = kFinJuego if self.regActual.TURN > 1 else kJugando

        # Si pendiente jugar y solicita el otro draw
        self.siJuegaHumano = True
        self.siJugamosConBlancas = self.regActual.DICREC["hasWhite"]

        self.liPGN = self.regActual.liCabeceras

        self.ponToolBar()

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.pantalla.ponRotulo1('<span style="font-weight:bold;font-size:24px">%s - %s</span>' % (
            self.regActual.WHITE, self.regActual.BLACK))

        if self.regActual.drawOffered:
            rotulo = '<span style="color:red;font-weight:bold;font-size:26px">%s</span>' % (
                _X(_("Draw offered by %1"), self.regActual.opponent),)
        else:
            rotulo = None
        self.pantalla.ponRotulo2(rotulo)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        if self.partida.numJugadas() > 0:
            nbase = self.partidaBase.numJugadas()
            if nbase:
                njg = nbase - 1
                self.ponteEnJugada(njg)
                jg = self.partida.jugada(njg)
            else:
                self.mueveJugada(kMoverFinal)
                jg = self.partida.liJugadas[-1]
            jg.siXFCC = True
            self.ponFlechaSC(jg.desde, jg.hasta)

        self.ponPosicionDGT()

        self.refresh()

        self.siguienteJugada()

    def procesarAccion(self, clave):

        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_atras:
            self.atras()

        elif clave == k_reiniciar:
            self.reajustar()
            if self.partida.numJugadas() > 0:
                nbase = self.partidaBase.numJugadas()
                if nbase:
                    njg = nbase - 1
                    self.ponteEnJugada(njg)
                    jg = self.partida.jugada(njg)
                else:
                    self.mueveJugada(kMoverFinal)
                    jg = self.partida.liJugadas[-1]
                jg.siXFCC = True
                self.ponFlechaSC(jg.desde, jg.hasta)
            self.ponPosicionDGT()
            self.refresh()
            self.siguienteJugada()

        elif clave == k_pgnFicheroRepite:
            self.grabar()
            if PantallaXFCC.pantallaXFCC(self, self.db):
                self.inicio(self.db)

        elif clave == k_configurar:
            self.configurar(siCambioTutor=False, siSonidos=True)

        elif clave == k_utilidades:
            liMasOpciones = (
                ( "libros", _("Consult a book"), Iconos.Libros() ),
                ( None, None, None ),
                ( "bookguide", _("Personal Opening Guide"), Iconos.BookGuide() ),
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

        elif clave == k_pgnInformacion:
            self.informacion()

        elif clave == k_enviar:
            self.enviar()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def ponToolBar(self):
        if self.regActual.TURN == 0:
            if self.partidaBase.numJugadas() < self.partida.numJugadas():
                li = [k_mainmenu, k_enviar, k_pgnFicheroRepite, k_pgnInformacion, k_atras, k_reiniciar, k_configurar,
                      k_utilidades]
            else:
                li = [k_mainmenu, k_enviar, k_pgnFicheroRepite, k_pgnInformacion, k_reiniciar, k_configurar,
                      k_utilidades]
        else:
            if self.partidaBase.numJugadas() < self.partida.numJugadas():
                li = [k_mainmenu, k_pgnFicheroRepite, k_pgnInformacion, k_atras, k_reiniciar, k_configurar,
                      k_utilidades]
            else:
                li = [k_mainmenu, k_pgnFicheroRepite, k_pgnInformacion, k_reiniciar, k_configurar, k_utilidades]
        self.pantalla.ponToolBar(li)

    def finPartida(self):
        self.grabar()
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
        # Peón coronando
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
            self.siguienteJugada()
            self.ponToolBar()
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

    def grabar(self):
        dic = {}
        dic["PARTIDA"] = self.partida.guardaEnTexto()
        self.db.guardaGame(dic)

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

        menu.lanza()

    def atras(self):
        if self.partida.numJugadas() > self.partidaBase.numJugadas():
            self.partida.anulaSoloUltimoMovimiento()
            if not self.fen:
                self.listaAperturasStd.asignaApertura(self.partida)
            self.ponteAlFinal()
            self.ponToolBar()
            self.refresh()
            self.siguienteJugada()

    def reajustar(self):
        # Datos recibidos
        self.partidaBase = self.regActual.partida

        # Datos activos
        self.partida.reset()
        if self.regActual.DICGAME:
            self.partida.recuperaDeTexto(self.regActual.DICGAME["PARTIDA"])

        nBase = self.partidaBase.numJugadas()
        nJug = self.partida.numJugadas()

        for n in range(nJug):
            if n >= nBase:
                break
            jg = self.partida.jugada(n)
            jgBase = self.partidaBase.jugada(n)
            if jg.movimiento() != jgBase.movimiento():
                # Creamos una variante de aquí al final
                varianteResto = self.partida.pgnBaseRAWcopy(n, nJug - 1)

                self.partida.liJugadas = self.partida.liJugadas[:n]

                # Se miran todas las variantes, por si alguna coincide con el movimiento base y si es así se añade
                variantesPrevias = jg.variantes
                siEncontradaVariante = False
                if jg.variantes:
                    fen = jg.posicionBase.fen()
                    for variante in jg.variantes.split("\n"):
                        pgn = PGN.UnPGN()
                        pgn.leeTexto('[FEN "%s"]\n%s' % (fen, variante))
                        partidaVariante = pgn.partida

                        njg = partidaVariante.numJugadas()
                        if njg:
                            jg0 = partidaVariante.jugada(0)
                            if jg0.desde and jg0.hasta and jg0.movimiento() == jgBase.movimiento():
                                for x in range(njg):
                                    self.partida.liJugadas.append(partidaVariante.jugada(x))
                                variantesPrevias = variantesPrevias.replace(variante + "\n", "")
                                jg0.variantes = (varianteResto + "\n" + variantesPrevias).strip()
                                siEncontradaVariante = True
                                break

                if not siEncontradaVariante:
                    self.partida.liJugadas = self.partida.liJugadas[:n]
                    jg0 = jgBase.clona()
                    jg0.variantes = (varianteResto + "\n" + variantesPrevias).strip()
                    self.partida.liJugadas.append(jg0)

                self.partida.ultPosicion = self.partida.liJugadas[-1].posicion.copia()
                break

        nJug = self.partida.numJugadas()
        if nJug < nBase:
            for x in range(nJug, nBase):
                self.partida.liJugadas.append(self.partidaBase.jugada(x).clona())
        if nBase:
            self.partida.ultPosicion = self.partida.liJugadas[-1].posicion.copia()
        self.listaAperturasStd.asignaApertura(self.partida)

    def enviar(self):

        nBase = self.partidaBase.numJugadas()
        nJug = self.partida.numJugadas()
        siJugado = nJug > nBase

        d = {
            "acceptDraw": (_("Accept draw"), Iconos.Tablas()),
            "offerDraw": (_("Offer draw"), Iconos.Tablas()),
            "claimDraw": (_("Claim draw"), Iconos.Tablas()),
            "resign": (_("Resign"), Iconos.Rendirse()),
        }
        if siJugado:
            jg = self.partida.jugada(nBase)
            d["play"] = ("%s: %s" % (_("Play move"), jg.pgnSP() ), Iconos.Enviar())
        else:
            jg = None

        lista = []

        def mas(clave):
            txt, icono = d[clave]
            lista.append((txt, clave))

        # Si ha ofrecido draw
        defecto = "resign"
        if siJugado:
            defecto = "play"
            mas("play")

        if self.regActual.drawOffered:
            defecto = "acceptDraw"
            mas("acceptDraw")
        else:
            if siJugado:
                mas("offerDraw")

        mas("resign")
        mas("claimDraw")

        liGen = [(None, None)]

        config = FormLayout.Combobox(_("Send"), lista)
        liGen.append(( config, defecto ))

        config = FormLayout.Editbox(_("Message"), alto=5)
        liGen.append(( config, "" ))

        resultado = FormLayout.fedit(liGen, title=_("Send"), parent=self.pantalla, icon=Iconos.Enviar())

        if resultado:
            resp, mensaje = resultado[1]
            if QTUtil2.pregunta(self.pantalla, _X(_("Are you sure you want: %1 ?"), d[resp][0])):
                movecount = nBase / 2 + 1
                ok, mens = self.db.makeAMove(resp, mensaje.strip(), jg, movecount)

                if ok:
                    QTUtil2.mensaje(self.pantalla, mens)
                    self.db.getMyGames()
                    self.procesarAccion(k_pgnFicheroRepite)
                else:
                    QTUtil2.mensError(self.pantalla, "%s:\n%s" % ( _("An error has occurred"), mens ))

