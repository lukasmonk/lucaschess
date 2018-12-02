import datetime
import random
import time

from Code import Apertura
from Code import Gestor
from Code import Jugada
from Code.QT import QTUtil2
from Code import Util
from Code import XMotorRespuesta
from Code.Constantes import *


def listaMotoresElo():
    x = """amyan|1|1112|5461
amyan|2|1360|6597
amyan|3|1615|7010
amyan|4|1914|7347
alaric|1|1151|5442
alaric|2|1398|6552
alaric|3|1781|7145
alaric|4|2008|7864
bikjump|1|1123|4477
bikjump|2|1204|5352
bikjump|3|1405|5953
bikjump|4|1710|6341
cheng|1|1153|5728
cheng|2|1402|6634
cheng|3|1744|7170
cheng|4|1936|7773
chispa|1|1109|5158
chispa|2|1321|6193
chispa|3|1402|6433
chispa|4|1782|7450
cinnamon|2|1111|4751
cinnamon|3|1151|5770
cinnamon|4|1187|5770
clarabit|1|1134|5210
clarabit|2|1166|6014
clarabit|3|1345|6407
clarabit|4|1501|6863
critter|1|1203|6822
critter|2|1618|7519
critter|3|1938|8196
critter|4|2037|8557
cyrano|1|1184|5587
cyrano|2|1392|6688
cyrano|3|1929|7420
cyrano|4|2033|7945
daydreamer|2|1358|6362
daydreamer|3|1381|6984
daydreamer|4|1629|7462
discocheck|1|1131|6351
discocheck|2|1380|6591
discocheck|3|1613|7064
discocheck|4|1817|7223
fruit|1|1407|6758
fruit|2|1501|6986
fruit|3|1783|7446
fruit|4|1937|8046
gaia|2|1080|5734
gaia|3|1346|6582
gaia|4|1766|7039
garbochess|1|1149|5640
garbochess|2|1387|6501
garbochess|3|1737|7231
garbochess|4|2010|7933
gaviota|1|1166|6503
gaviota|2|1407|7127
gaviota|3|1625|7437
gaviota|4|2026|7957
glaurung|2|1403|6994
glaurung|3|1743|7578
glaurung|4|2033|7945
greko|1|1151|5552
greko|2|1227|6282
greko|3|1673|6861
greko|4|1931|7518
hamsters|1|1142|5779
hamsters|2|1386|6365
hamsters|3|1649|7011
hamsters|4|1938|7457
komodo|1|1187|6636
komodo|2|1514|7336
komodo|3|1633|7902
komodo|4|2036|8226
lime|1|1146|5251
lime|2|1209|6154
lime|3|1500|6907
lime|4|1783|7499
pawny|2|1086|6474
pawny|3|1346|6879
pawny|4|1503|7217
rhetoric|1|1147|5719
rhetoric|2|1371|6866
rhetoric|3|1514|7049
rhetoric|4|1937|7585
rodent|2|1119|6490
rodent|3|1492|7185
rodent|4|1720|7519
rybka|1|1877|8203
rybka|2|2083|8675
rybka|3|2237|9063
rybka|4|2290|9490
simplex|1|1126|4908
simplex|2|1203|5868
simplex|3|1403|6525
simplex|4|1757|7265
stockfish|1|1200|6419
stockfish|2|1285|6252
stockfish|3|1382|6516
stockfish|4|1561|6796
texel|1|1165|6036
texel|2|1401|7026
texel|3|1506|7255
texel|4|1929|7813
toga|1|1202|6066
toga|2|1497|6984
toga|3|2031|7639
toga|4|2038|8254
ufim|1|1214|6161
ufim|2|1415|7260
ufim|3|2014|8032
ufim|4|2104|8363
umko|1|1151|6004
umko|2|1385|6869
umko|3|1883|7462
umko|4|2081|7887"""
    li = []
    for linea in x.split("\n"):
        clave, depth, fide, sts = linea.split("|")
        depth = int(depth)
        sts = int(sts)
        fide = int(fide)
        elo = int((fide + 0.2065 * sts + 154.51) / 2)
        li.append((elo, clave, depth))
    return li


class MotorElo:
    def __init__(self, elo, nombre, clave, depth):
        self.elo = elo
        self.nombre = nombre
        self.clave = clave
        self.depth = depth
        self.siInterno = depth == 0
        self.depthOpen = (elo / 100 - 8) if elo < 2100 else 100
        if self.depthOpen < 2:
            self.depthOpen = 2
        self.alias = self.nombre
        if self.depth:
            self.alias += " %d" % self.depth

    def rotulo(self):
        resp = self.nombre
        if self.depth:
            resp += " %d" % self.depth
        resp += " (%d)" % self.elo
        return resp


class GestorElo(Gestor.Gestor):
    def valores(self):
        lit = ("Monkey",
               "Donkey",
               "Bull",
               "Wolf",
               "Lion",
               "Rat",
               "Snake",
               )

        self.liMotores = []
        for x in range(1, 8):
            self.liMotores.append(MotorElo(x * 108 + 50, _F(lit[x - 1]), lit[x - 1], 0))

        def m(elo, clave, depth):
            self.liMotores.append(MotorElo(elo, Util.primeraMayuscula(clave), clave, depth))

        for elo, clave, depth in listaMotoresElo():
            m(elo, clave, depth)

        for k, v in self.configuracion.dicRivales.iteritems():
            if v.elo > 2000:
                m(v.elo, v.clave, None)  # ponemos depth a None, para diferenciar del 0 de los motores internos

        self.liMotores.sort(key=lambda x: x.elo)

        self.liT = (
            (0, 50, 3), (20, 53, 5), (40, 58, 4), (60, 62, 4), (80, 66, 5), (100, 69, 4), (120, 73, 3), (140, 76, 3),
            (160, 79, 3), (180, 82, 2), (200, 84, 9), (300, 93, 4), (400, 97, 3))
        self.liK = ((0, 60), (800, 50), (1200, 40), (1600, 30), (2000, 30), (2400, 10))

    def calcDifElo(self, eloJugador, eloRival, resultado):
        if resultado == kGanamos:
            result = 1
        elif resultado == kTablas:
            result = 0
        else:
            result = -1
        return Util.fideELO(eloJugador, eloRival, result)

    def listaMotores(self, elo):
        self.valores()
        li = []
        numX = len(self.liMotores)
        for num, mt in enumerate(self.liMotores):
            mtElo = mt.elo
            mt.siOut = False
            if mtElo > elo + 400:
                mt.siOut = True
                mtElo = elo + 400
            mt.siJugable = abs(mtElo - elo) <= 400
            if mt.siJugable:
                def rot(res):
                    return self.calcDifElo(elo, mtElo, res)

                mt.pgana = rot(kGanamos)
                mt.ptablas = rot(kTablas)
                mt.ppierde = rot(kGanaRival)

                mt.numero = numX - num

                li.append(mt)

        return li

    def inicio(self, datosMotor, siCompetitivo, aplazamiento=None):

        self.tipoJuego = kJugElo
        self.siCompetitivo = siCompetitivo

        self.resultado = None
        self.siJuegaHumano = False
        self.estado = kJugando

        if aplazamiento:
            siBlancas = aplazamiento["SIBLANCAS"]
        else:
            siBlancas = self.determinaColor(datosMotor)

        self.siJugamosConBlancas = siBlancas
        self.siRivalConBlancas = not siBlancas

        self.rmRival = None
        self.liRMrival = []
        self.noMolestar = False
        self.resignPTS = -1000

        self.siTutorActivado = False
        self.pantalla.ponActivarTutor(self.siTutorActivado)

        self.ayudas = 0
        self.ayudasPGN = self.ayudas

        self.siApertura = True

        # -Aplazamiento 1/2--------------------------------------------------
        if aplazamiento:
            self.partida.recuperaDeTexto(aplazamiento["JUGADAS"])

            self.siApertura = aplazamiento["SIAPERTURA"]
            self.partida.pendienteApertura = aplazamiento["PENDIENTEAPERTURA"]
            self.partida.apertura = None if aplazamiento["APERTURA"] is None else self.listaAperturasStd.dic[
                aplazamiento["APERTURA"]]

            self.datosMotor = MotorElo(aplazamiento["ELO"], aplazamiento["NOMBRE"], aplazamiento["CLAVE"],
                                       aplazamiento["DEPTH"])
            self.datosMotor.pgana = aplazamiento["PGANA"]
            self.datosMotor.ppierde = aplazamiento["PPIERDE"]
            self.datosMotor.ptablas = aplazamiento["PTABLAS"]
            self.datosMotor.siInterno = False
            self.apertura = Apertura.AperturaPol(100, elo=self.datosMotor.elo)
            self.apertura.recuperaEstado(aplazamiento["ESTADOAPERTURA"])
        else:
            self.datosMotor = datosMotor
            self.apertura = Apertura.AperturaPol(100, elo=self.datosMotor.elo)

        eloengine = self.datosMotor.elo
        eloplayer = self.configuracion.eloActivo(siCompetitivo)
        self.whiteElo = eloplayer if siBlancas else eloengine
        self.blackElo = eloplayer if not siBlancas else eloengine

        self.siRivalInterno = self.datosMotor.siInterno
        if self.siRivalInterno:
            rival = self.configuracion.buscaRival("irina")
            depth = 2 if self.datosMotor.clave in ("Rat", "Snake") else 1
            self.xrival = self.procesador.creaGestorMotor(rival, None, depth)
            self.xrival.set_option("Personality", self.datosMotor.clave)

        else:
            rival = self.configuracion.buscaRival(self.datosMotor.clave)
            self.xrival = self.procesador.creaGestorMotor(rival, None, self.datosMotor.depth)

        self.pteToolRendirse = False
        if not siCompetitivo:
            self.pteToolRendirse = True
            self.maxPlyRendirse = 6
        elif self.siJugamosConBlancas:
            self.pteToolRendirse = True
            self.maxPlyRendirse = 1
        else:
            self.maxPlyRendirse = 0

        if aplazamiento and self.partida.numJugadas() > self.maxPlyRendirse:
            self.pteToolRendirse = False

        self.ponToolBar()

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.ponPiezasAbajo(siBlancas)
        self.quitaAyudas(True, siQuitarAtras=siCompetitivo)
        self.mostrarIndicador(True)
        rotulo = "%s: <b>%s</b>" % (_("Opponent"), self.datosMotor.rotulo())
        self.ponRotulo1(rotulo)

        nbsp = "&nbsp;" * 3

        txt = "%s:%+d%s%s:%+d%s%s:%+d" % (_("Win"), self.datosMotor.pgana, nbsp,
                                          _("Draw"), self.datosMotor.ptablas, nbsp,
                                          _("Loss"), self.datosMotor.ppierde)

        self.ponRotulo2("<center>%s</center>" % txt)
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        # -Aplazamiento 2/2--------------------------------------------------
        if aplazamiento:
            self.mueveJugada(kMoverFinal)

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponToolBar(self):
        if self.pteToolRendirse:
            liTool = (k_cancelar, k_aplazar, k_atras, k_configurar, k_utilidades)
        else:
            if self.siCompetitivo:
                liTool = (k_rendirse, k_tablas, k_aplazar, k_configurar, k_utilidades)
            else:
                liTool = (k_rendirse, k_tablas, k_aplazar, k_atras, k_configurar, k_utilidades)

        self.pantalla.ponToolBar(liTool)
        self.pantalla.mostrarOpcionToolbar(k_tablas, not self.siRivalInterno)

    def procesarAccion(self, clave):

        if clave in (k_rendirse, k_cancelar):
            self.rendirse()

        elif clave == k_tablas:
            self.tablasPlayer()

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
            aplazamiento["SIBLANCAS"] = self.siJugamosConBlancas
            aplazamiento["JUGADAS"] = self.partida.guardaEnTexto()
            aplazamiento["SICOMPETITIVO"] = self.siCompetitivo

            aplazamiento["SIAPERTURA"] = self.siApertura
            aplazamiento["PENDIENTEAPERTURA"] = self.partida.pendienteApertura
            aplazamiento["APERTURA"] = self.partida.apertura.a1h8 if self.partida.apertura else None
            aplazamiento["ESTADOAPERTURA"] = self.apertura.marcaEstado()

            aplazamiento["ELO"] = self.datosMotor.elo
            aplazamiento["NOMBRE"] = self.datosMotor.nombre
            aplazamiento["CLAVE"] = self.datosMotor.clave
            aplazamiento["DEPTH"] = self.datosMotor.depth
            aplazamiento["PGANA"] = self.datosMotor.pgana
            aplazamiento["PPIERDE"] = self.datosMotor.ppierde
            aplazamiento["PTABLAS"] = self.datosMotor.ptablas

            self.configuracion.graba(aplazamiento)
            self.estado = kFinJuego
            self.pantalla.accept()

    def finalX(self):
        return self.rendirse()

    def rendirse(self):
        if self.estado == kFinJuego:
            return True
        if (self.partida.numJugadas() > 0) and not self.pteToolRendirse:
            if not QTUtil2.pregunta(self.pantalla, _("Do you want to resign?") + " (%d)" % self.datosMotor.ppierde):
                return False  # no abandona
            self.partida.abandona(self.siJugamosConBlancas)
            self.ponResultado(kGanaRival)
        else:
            self.procesador.inicio()

        return False

    def siguienteJugada(self):

        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()
        siBlancas = self.partida.ultPosicion.siBlancas

        numJugadas = self.partida.numJugadas()

        if numJugadas > 0:
            jgUltima = self.partida.last_jg()
            if jgUltima:
                if jgUltima.siJaqueMate:
                    self.ponResultado(kGanaRival if self.siJugamosConBlancas == siBlancas else kGanamos)
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

        siRival = siBlancas == self.siRivalConBlancas
        self.ponIndicador(siBlancas)

        self.refresh()

        if siRival:
            self.pensando(True)
            self.desactivaTodas()

            iniT = time.time()

            siPensar = True

            if self.siApertura:

                dT, hT = 5, 5

                siBien, desde, hasta, coronacion = self.apertura.juegaMotor(self.fenUltimo())

                if siBien:
                    self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
                    self.rmRival.desde = desde
                    self.rmRival.hasta = hasta
                    self.rmRival.coronacion = coronacion
                    siPensar = False

            if siPensar:
                if self.siRivalInterno:
                    self.rmRival = self.xrival.juega()
                    dT, hT = 5, 15
                else:
                    nJugadas = self.partida.numJugadas()
                    if nJugadas > 30:
                        tp = 300
                    else:
                        tp = 600
                    self.rmRival = self.xrival.juegaTiempo(tp, tp, 0)  # engloba juega + juega Tiempo
                    pts = self.rmRival.puntosABS()
                    if pts > 100:
                        dT, hT = 5, 15
                    else:
                        dT, hT = 10, 35

            difT = time.time() - iniT
            t = random.randint(dT * 10, hT * 10) * 0.01
            if difT < t:
                time.sleep(t - difT)

            self.pensando(False)
            if self.mueveRival(self.rmRival):
                self.liRMrival.append(self.rmRival)
                if self.valoraRMrival(self.siRivalConBlancas):
                    self.siguienteJugada()

        else:

            self.siJuegaHumano = True
            self.activaColor(siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        if self.siApertura:
            self.siApertura = self.apertura.compruebaHumano(self.fenUltimo(), desde, hasta)

        self.movimientosPiezas(jg.liMovs)

        self.partida.ultPosicion = jg.posicion
        self.masJugada(jg, True)
        self.error = ""
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):

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

        # self.ponAyudas( self.ayudas )

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

        if self.pteToolRendirse:
            if self.partida.numJugadas() > self.maxPlyRendirse:
                self.pteToolRendirse = False
                self.ponToolBar()

    def mueveRival(self, respMotor):
        desde = respMotor.desde
        hasta = respMotor.hasta

        coronacion = respMotor.coronacion

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        if siBien:
            self.partida.ultPosicion = jg.posicion

            self.masJugada(jg, False)
            self.movimientosPiezas(jg.liMovs, True)

            self.error = ""

            return True
        else:
            self.error = mens
            return False

    def ponResultado(self, quien):
        self.resultado = quien
        self.desactivaTodas()
        self.siJuegaHumano = False

        self.estado = kFinJuego

        self.beepResultado(quien)

        nombreContrario = self.datosMotor.rotulo()

        mensaje = _("Game ended")
        if quien == kGanamos:
            mensaje = _X(_("Congratulations you have won against %1."), nombreContrario)

        elif quien == kGanaRival:
            mensaje = _X(_("Unfortunately you have lost against %1."), nombreContrario)

        elif quien == kTablas:
            mensaje = _X(_("Draw against %1."), nombreContrario)

        elif quien == kTablasRepeticion:
            mensaje = _X(_("Draw due to three times repetition (n. %1) against %2."), self.rotuloTablasRepeticion,
                         nombreContrario)
            self.resultado = kTablas

        elif quien == kTablas50:
            mensaje = _X(_("Draw according to the 50 move rule against %1."), nombreContrario)
            self.resultado = kTablas

        elif quien == kTablasFaltaMaterial:
            mensaje = _X(_("Draw, not enough material to mate %1."), nombreContrario)
            self.resultado = kTablas

            # elif quien == kGanamosTiempo:
            # mensaje = _X( _("Congratulations, you win against %1 on time."), nombreContrario )
            # self.resultado = kGanamos

        # elif quien == kGanaRivalTiempo:
        #     mensaje = _X(_("%1 has won on time."), nombreContrario)
        #     self.resultado = kGanaRival

        elo = self.configuracion.eloActivo(self.siCompetitivo)
        if self.resultado == kGanamos:
            difelo = self.datosMotor.pgana

        elif self.resultado == kGanaRival:
            difelo = self.datosMotor.ppierde

        else:
            difelo = self.datosMotor.ptablas

        nelo = elo + difelo
        if nelo < 0:
            nelo = 0
        self.configuracion.ponEloActivo(nelo, self.siCompetitivo)

        self.historial(elo, nelo)
        self.configuracion.graba()
        if not self.siCompetitivo:
            self.procesador.entrenamientos.rehaz()

        mensaje += "<br><br>%s : %d<br>" % (_("New Lucas-Elo"), self.configuracion.eloActivo(self.siCompetitivo))

        self.guardarGanados(quien == kGanamos)
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
        dic["RIVAL"] = self.datosMotor.rotulo()
        dic["RESULTADO"] = self.resultado
        dic["AELO"] = elo
        dic["NELO"] = nelo
        dic["MODONC"] = not self.siCompetitivo

        lik = Util.LIdisk(self.configuracion.fichEstadElo)
        lik.append(dic)
        lik.close()

        dd = Util.DicSQL(self.configuracion.fichEstadElo, tabla="color")
        clave = "%s-%d" % (self.datosMotor.nombre, self.datosMotor.depth if self.datosMotor.depth else 0)
        dd[clave] = self.siJugamosConBlancas
        dd.close()

    def determinaColor(self, datosMotor):
        clave = "%s-%d" % (datosMotor.clave, datosMotor.depth if datosMotor.depth else 0)
        if not self.siCompetitivo:
            clave += "NC"

        dd = Util.DicSQL(self.configuracion.fichEstadElo, tabla="color")
        previo = dd.get(clave, random.randint(0, 1) == 0)
        dd.close()
        return not previo

    def atras(self):
        if self.partida.numJugadas() > 2:
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.partida.asignaApertura()
            self.ponteAlFinal()
            self.refresh()
            self.siguienteJugada()

            # """
            # https://github.com/piongu/FIDE-rating-change-calculator/blob/master/window_qt.cpp

            # Rating Change Calculator by piongu - version 1.0
            # released 19.10.2011

            # Simple and user friendly program which calculates rating change (ELO System).
            # It is generally dedicated to chess players where this system is used by FIDE.

            # The folder should include:
            # qt-elo-change.exe
            # libgcc_s_dw2-1.dll
            # mingwm10.dll
            # QtCore4.dll
            # QtGui4.dll
            # README.txt

            # The program is totally free of charge. If you paid for this you got fooled.

            # ==Installation==
            # No installation needed!

            # ==Source==
            # https://github.com/piongu/FIDE-rating-change-calculator

            # ==Website==
            # http://piongu.wordpress.com/2011/10/19/fide-rating-change-calculator/

            # ==License==
            # Copyright (C) 2011 Piotr An Nguyen

            # This program is free software: you can redistribute it and/or modify
            # it under the terms of the GNU General Public License as published by
            # the Free Software Foundation, either version 3 of the License, or
            # (at your option) any later version.
            # """
            # def fideELO( user, otro, score, k=10 ):
            # def expected(diff):
            # tab=[392,375,358,345,327,316,303,291,279,268,257,246,236,226,216,207,198,189,180,171,163,154,146,138,130,122,114,107,99,92,84,77,69,62,54,47,40,33,26,18,11,4,0]
            # exp = 92
            # i = 0

            # while i<43 and abs(diff) < tab[i]:
            # i+= 1
            # exp-= 1
            # if diff > 0:
            # return exp/100.0
            # else:
            # return 1-exp/100.0

            # diff = user - otro
            # if abs(diff) > 400:
            # diff = 400 if user > otro else -400

            # return (score - expected(diff))*k;
