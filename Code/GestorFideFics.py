import collections
import copy
import datetime
import random

import LCEngine4 as LCEngine

from Code import Apertura
from Code import Gestor
from Code import Partida
from Code.QT import PantallaJuicio
from Code.QT import QTUtil2
from Code.SQL import Base
from Code import Util
from Code.Constantes import *


class GestorFideFics(Gestor.Gestor):
    def selecciona(self, tipoJuego):
        self.tipoJuego = tipoJuego
        if tipoJuego == kJugFics:
            self._db = "./IntFiles/FicsElo.db"
            self._activo = self.configuracion.ficsActivo
            self._ponActivo = self.configuracion.ponFicsActivo
            self.nombreObj = _("Fics-player")  # self.cabs[ "White" if self.siJugamosConBlancas else "Black" ]
            self._fichEstad = self.configuracion.fichEstadFicsElo
            self._titulo = _("Fics-Elo")
            self._newTitulo = _("New Fics-Elo")
            self._TIPO = "FICS"

        elif tipoJuego == kJugFide:
            self._db = "./IntFiles/FideElo.db"
            self._activo = self.configuracion.fideActivo
            self._ponActivo = self.configuracion.ponFideActivo
            self.nombreObj = _("Fide-player")  # self.cabs[ "White" if self.siJugamosConBlancas else "Black" ]
            self._fichEstad = self.configuracion.fichEstadFideElo
            self._titulo = _("Fide-Elo")
            self._newTitulo = _("New Fide-Elo")
            self._TIPO = "FIDE"

        elif tipoJuego == kJugLichess:
            self._db = "./IntFiles/LichessElo.db"
            self._activo = self.configuracion.lichessActivo
            self._ponActivo = self.configuracion.ponLichessActivo
            self.nombreObj = _("Lichess-player")
            self._fichEstad = self.configuracion.fichEstadLichessElo
            self._titulo = _("Lichess-Elo")
            self._newTitulo = _("New Lichess-Elo")
            self._TIPO = "LICHESS"

    def eligeJuego(self, siCompetitivo, nivel):
        self.siCompetitivo = siCompetitivo
        color = self.determinaColor(nivel)
        db = Base.DBBase(self._db)
        dbf = db.dbfT("data", "ROWID", condicion="LEVEL=%d AND WHITE=%d" % (nivel, 1 if color else 0))
        dbf.leer()
        reccount = dbf.reccount()

        # dbf = db.dbfT("data", "ROWID,CABS,MOVS", condicion="LEVEL=%d AND WHITE=%d" % (nivel, 1 if color else 0 ))
        # dbf.leer()
        # reccount = dbf.reccount()
        # for recno in range(1,reccount+1):
        #     dbf.goto(recno)
        #     if "1652" in dbf.CABS:
        #         pint dbf.CABS
        #         pv = LCEngine.xpv2pv(dbf.MOVS)
        #         pint pv
        #         pint dbf.ROWID
        #         pint "-"*40
        # import Partida
        # f = open("12001.pgn","wb")
        # for recno in range(1,reccount+1):
        # dbf.goto(recno)
        # pv = LCEngine.xpv2pv(dbf.MOVS)
        # # if pv.startswith( "d2d4" ) and "RaulJRojel" in dbf.CABS and "tunante" in dbf.CABS:
        # # break
        # pv = LCEngine.xpv2pv(dbf.MOVS)
        # if pv.startswith( "d2d4" ):
        # p = Partida.Partida()
        # p.leerPV(pv)
        # c = "["+dbf.CABS.replace("=", ' "').replace("\n", '"]\n[')+"]\n"
        # f.write( '[ID "%d"]\n'%dbf.ROWID)
        # f.write( c )
        # f.write(p.pgnBase())
        # f.write("\n\n")
        # f.close()

        recno = random.randint(1, reccount)
        dbf.goto(recno)
        xid = dbf.ROWID
        dbf.cerrar()
        db.cerrar()

        return xid

    def readID(self, xid):
        db = Base.DBBase(self._db)
        dbf = db.dbfT("data", "LEVEL,WHITE,CABS,MOVS", condicion="ROWID=%d" % xid)
        dbf.leer()
        dbf.gotop()

        self.nivel = dbf.LEVEL

        siBlancas = dbf.WHITE
        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        pv = LCEngine.xpv2pv(dbf.MOVS)
        self.partidaObj = Partida.Partida()
        self.partidaObj.leerPV(pv)
        self.posJugadaObj = 0
        self.numJugadasObj = self.partidaObj.numJugadas()

        self.cabs = collections.OrderedDict()
        li = dbf.CABS.split("\n")
        for x in li:
            if x:
                clave, valor = x.split('=')
                self.cabs[clave] = valor

        dbf.cerrar()
        db.cerrar()

    def inicio(self, idGame, siCompetitivo, aplazamiento=None):
        self.siCompetitivo = siCompetitivo

        self.resultado = None
        self.siJuegaHumano = False
        self.estado = kJugando
        self.analisis = None
        self.comentario = None
        self.siAnalizando = False

        if aplazamiento:
            idGame = aplazamiento["IDGAME"]
            self.siCompetitivo = aplazamiento["SICOMPETITIVO"]

        self.readID(idGame)
        self.idGame = idGame

        self.eloObj = int(self.cabs["WhiteElo" if self.siJugamosConBlancas else "BlackElo"])
        self.eloUsu = self._activo(self.siCompetitivo)

        self.pwin = Util.fideELO(self.eloUsu, self.eloObj, +1)
        self.pdraw = Util.fideELO(self.eloUsu, self.eloObj, 0)
        self.plost = Util.fideELO(self.eloUsu, self.eloObj, -1)

        self.puntos = 0
        if aplazamiento:
            self.posJugadaObj = aplazamiento["POSJUGADAOBJ"]
            self.puntos = aplazamiento["PUNTOS"]

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(self.siTutorActivado)

        self.ayudas = 0
        self.ayudasPGN = 0

        # tutor = self.configuracion.buscaRival("stockfish")
        # self.xtutor = self.procesador.creaGestorMotor(tutor, None, None)
        self.xtutor.maximizaMultiPV()

        # -Aplazamiento 1/2--------------------------------------------------
        if aplazamiento:
            self.partida.recuperaDeTexto(aplazamiento["JUGADAS"])
            self.partida.pendienteApertura = aplazamiento["PENDIENTEAPERTURA"]
            self.partida.apertura = None if aplazamiento["APERTURA"] is None else self.listaAperturasStd.dic[
                aplazamiento["APERTURA"]]

        self.book = Apertura.AperturaPol(999)

        self.ponToolBar()

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.quitaAyudas(True, siQuitarAtras=siCompetitivo)
        self.mostrarIndicador(True)
        rotulo = "%s: <b>%d</b> | %s: <b>%d</b>" % (self._titulo, self.eloUsu, _("Elo rival"), self.eloObj)
        rotulo += " | %+d %+d %+d" % (self.pwin, self.pdraw, self.plost)
        self.ponRotulo1(rotulo)

        self.ponRotulo2("")
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        # -Aplazamiento 2/2--------------------------------------------------
        if aplazamiento:
            self.mueveJugada(kMoverFinal)
            self.ponPuntos()

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponPuntos(self):
        self.ponRotulo2("%s : <b>%d</b>" % (_("Points"), self.puntos))

    def ponToolBar(self):
        if self.siCompetitivo:
            liTool = (k_rendirse, k_aplazar, k_configurar, k_utilidades)
        else:
            liTool = (k_rendirse, k_aplazar, k_atras, k_configurar, k_utilidades)

        self.pantalla.ponToolBar(liTool)

    def procesarAccion(self, clave):
        if clave in (k_rendirse, k_cancelar):
            self.rendirse()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_atras:
            self.atras()

        elif clave == k_utilidades:
            self.utilidadesElo()

        elif clave == k_aplazar:
            self.aplazar()

        elif clave in self.procesador.liOpcionesInicio:
            self.procesador.procesarAccion(clave)

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def aplazar(self):
        if self.partida.numJugadas() and QTUtil2.pregunta(self.pantalla, _("Do you want to adjourn the game?")):
            aplazamiento = {}
            aplazamiento["TIPOJUEGO"] = self.tipoJuego
            aplazamiento["IDGAME"] = self.idGame
            aplazamiento["SIBLANCAS"] = self.siJugamosConBlancas
            aplazamiento["SICOMPETITIVO"] = self.siCompetitivo
            aplazamiento["POSJUGADAOBJ"] = self.posJugadaObj
            aplazamiento["JUGADAS"] = self.partida.guardaEnTexto()
            aplazamiento["PENDIENTEAPERTURA"] = self.partida.pendienteApertura
            aplazamiento["APERTURA"] = self.partida.apertura.a1h8 if self.partida.apertura else None
            aplazamiento["PUNTOS"] = self.puntos

            self.configuracion.graba(aplazamiento)
            self.estado = kFinJuego
            self.pantalla.accept()

    def finalX(self):
        return self.rendirse()

    def rendirse(self):
        if self.estado == kFinJuego:
            self.analizaTerminar()
            return True
        if self.partida.numJugadas() > 0:
            if not QTUtil2.pregunta(self.pantalla, _("Do you want to resign?") + " (%d)" % self.plost):
                return False  # no abandona
            self.puntos = -999
            self.analizaTerminar()
            self.ponResultado()
        else:
            self.analizaTerminar()
            self.procesador.inicio()

        return False

    def analizaInicio(self):
        if not self.siTerminada():
            self.xtutor.ac_inicio(self.partida)
            self.siAnalizando = True

    def analizaEstado(self):
        self.xtutor.motor.ac_lee()
        self.mrm = copy.deepcopy(self.xtutor.ac_estado())
        return self.mrm

    def analizaMinimo(self, minTime):
        self.mrm = copy.deepcopy(self.xtutor.ac_minimo(minTime, False))
        return self.mrm

    def analizaFinal(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.ac_final(-1)

    def analizaTerminar(self):
        if self.siAnalizando:
            self.siAnalizando = False
            self.xtutor.terminar()

    def siguienteJugada(self):
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()
        siBlancas = self.partida.ultPosicion.siBlancas

        numJugadas = self.partida.numJugadas()
        if numJugadas >= self.numJugadasObj:
            self.ponResultado()
            return

        siRival = siBlancas == self.siRivalConBlancas
        self.ponIndicador(siBlancas)

        self.refresh()

        if siRival:
            self.masJugada(False)
            self.siguienteJugada()
        else:

            self.siJuegaHumano = True
            self.pensando(True)
            if self.continueTt:
                self.analizaInicio()
            self.activaColor(siBlancas)
            self.pensando(False)

    def mueveHumano(self, desde, hasta, coronacion=""):
        jgUsu = self.checkMueveHumano(desde, hasta, coronacion)
        if not jgUsu:
            return False

        jgObj = self.partidaObj.jugada(self.posJugadaObj)

        analisis = None
        comentario = None

        siBookUsu = False
        siBookObj = False

        comentarioUsu = ""
        comentarioObj = ""
        comentarioPuntos = ""

        siAnalizaJuez = jgUsu.movimiento() != jgObj.movimiento()
        if self.book:
            fen = self.fenUltimo()
            siBookUsu = self.book.compruebaHumano(fen, desde, hasta)
            siBookObj = self.book.compruebaHumano(fen, jgObj.desde, jgObj.hasta)
            if siBookUsu:
                comentarioUsu = _("book move")
            if siBookObj:
                comentarioObj = _("book move")
            if siBookUsu and siBookObj:
                if jgObj.movimiento() != jgUsu.movimiento():
                    # comentario = "%s: %s" % (_("Same book move"), jgObj.pgnSP())
                # else:
                    bmove = _("book move")
                    comentario = "%s: %s %s\n%s: %s %s" % (self.nombreObj, jgObj.pgnSP(), bmove,
                                                           self.configuracion.jugador, jgUsu.pgnSP(), bmove)
                    w = PantallaJuicio.MensajeF(self.pantalla, comentario)
                    w.mostrar()
                siAnalizaJuez = False
            else:
                if not siBookObj:
                    self.book = None

        if siAnalizaJuez:
            um = QTUtil2.analizando(self.pantalla)
            if not self.continueTt:
                self.analizaInicio()
            mrm = self.analizaMinimo(5000)
            posicion = self.partida.ultPosicion

            rmUsu, nada = mrm.buscaRM(jgUsu.movimiento())
            if rmUsu is None:
                self.analizaFinal()
                rmUsu = self.xtutor.valora(posicion, jgUsu.desde, jgUsu.hasta, jgUsu.coronacion)
                mrm.agregaRM(rmUsu)
                self.analizaInicio()

            rmObj, posObj = mrm.buscaRM(jgObj.movimiento())
            if rmObj is None:
                self.analizaFinal()
                rmObj = self.xtutor.valora(posicion, jgObj.desde, jgObj.hasta, jgObj.coronacion)
                posObj = mrm.agregaRM(rmObj)
                self.analizaInicio()

            analisis = mrm, posObj
            um.final()

            w = PantallaJuicio.WJuicio(self, self.xtutor, self.nombreObj, posicion, mrm, rmObj, rmUsu, analisis)
            w.exec_()

            analisis = w.analisis
            dpts = w.difPuntos()

            self.puntos += dpts
            self.ponPuntos()

            comentarioUsu += " %s" % (rmUsu.abrTexto())
            comentarioObj += " %s" % (rmObj.abrTexto())

            comentarioPuntos = "%s = %d %+d %+d = %d" % (_("Points"), self.puntos - dpts, rmUsu.puntosABS(),
                                                         -rmObj.puntosABS(), self.puntos)

            comentario = "%s: %s %s\n%s: %s %s\n%s" % (self.nombreObj, jgObj.pgnSP(), comentarioObj,
                                                       self.configuracion.jugador, jgUsu.pgnSP(), comentarioUsu,
                                                       comentarioPuntos)

        self.analizaFinal()

        self.masJugada(True, comentario, analisis)
        self.siguienteJugada()
        return True

    def masJugada(self, siNuestra, comentario=None, analisis=None):

        jg = self.partidaObj.jugada(self.posJugadaObj)
        self.posJugadaObj += 1
        if analisis:
            jg.analisis = analisis
        if comentario:
            jg.comentario = comentario

        if comentario:
            self.comentario = comentario.replace("\n", "<br><br>") + "<br>"

        if not siNuestra:
            if self.posJugadaObj:
                self.comentario = None

        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()
        self.movimientosPiezas(jg.liMovs, True)
        self.tablero.ponPosicion(jg.posicion)
        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

    def ponResultado(self):
        self.analizaTerminar()
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.estado = kFinJuego

        if self.puntos < -50:
            mensaje = _X(_("Unfortunately you have lost against %1."), self.nombreObj)
            quien = kGanaRival
            difelo = self.plost
        elif self.puntos > 50:
            mensaje = _X(_("Congratulations you have won against %1."), self.nombreObj)
            quien = kGanamos
            difelo = self.pwin
        else:
            mensaje = _X(_("Draw against %1."), self.nombreObj)
            quien = kTablas
            difelo = self.pdraw

        self.beepResultado(quien)

        nelo = self.eloUsu + difelo
        if nelo < 0:
            nelo = 0
        self._ponActivo(nelo, self.siCompetitivo)

        self.historial(self.eloUsu, nelo)
        self.configuracion.graba()
        if not self.siCompetitivo:
            self.procesador.entrenamientos.rehaz()

        mensaje += "<br><br>%s : %d<br>" % (self._newTitulo, self._activo(self.siCompetitivo))

        self.mensajeEnPGN(mensaje)
        self.ponFinJuego()

        # def ponFinJuego( self ):
        # self.estado = kFinJuego
        # self.pantalla.ponTitulo()
        # self.desactivaTodas()
        # liOpciones = self.procesador.liOpcionesInicio[:]
        # if self.partida.numJugadas():
        # liOpciones.insert( 2, k_utilidades )
        # liOpciones.insert( 2, k_configurar )
        # else:
        # self.quitaCapturas()
        # self.pantalla.ponToolBar( liOpciones )

    def historial(self, elo, nelo):
        dic = {}
        dic["FECHA"] = datetime.datetime.now()
        dic["NIVEL"] = self.nivel
        dic["RESULTADO"] = self.resultado
        dic["AELO"] = elo
        dic["NELO"] = nelo
        dic["MODONC"] = not self.siCompetitivo

        lik = Util.LIdisk(self._fichEstad)
        lik.append(dic)
        lik.close()

        dd = Util.DicSQL(self._fichEstad, tabla="color")
        clave = "%s-%d" % (self._TIPO, self.nivel)
        dd[clave] = self.siJugamosConBlancas
        dd.close()

    def determinaColor(self, nivel):
        clave = "%s-%d" % (self._TIPO, nivel)
        if not self.siCompetitivo:
            clave += "NC"

        dd = Util.DicSQL(self._fichEstad, tabla="color")
        previo = dd.get(clave, random.randint(0, 1) == 0)
        dd.close()
        return not previo

    def atras(self):
        if self.partida.numJugadas() > 2:
            self.analizaFinal()
            ndel = self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.partida.asignaApertura()
            self.posJugadaObj -= ndel
            self.analisis = None
            self.ponteAlFinal()
            self.refresh()
            self.siguienteJugada()
