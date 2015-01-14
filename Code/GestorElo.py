# -*- coding: latin-1 -*-

import random
import operator
import datetime
import time

from Code.Constantes import *
import Code.Util as Util
import Code.Jugada as Jugada
import Code.MotorInternoGM as MotorInternoGM
import Code.XMotorRespuesta as XMotorRespuesta
import Code.Apertura as Apertura
import Code.Gestor as Gestor
import Code.QT.QTUtil2 as QTUtil2

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

    def rotulo(self):
        resp = self.nombre
        if self.depth:
            resp += " %d" % self.depth
        resp += " (%d)" % self.elo
        return resp

class GestorElo(Gestor.Gestor):
    def valores(self):
        lit = ( _("Monkey"),
                _("Donkey"),
                _("Bull"),
                _("Wolf"),
                _("Lion"),
                _("Rat"),
                _("Snake"),
        )

        self.liMotores = []
        for x in range(1, 8):
            self.liMotores.append(MotorElo(x * 108 + 50, lit[x - 1], x - 1, 0))

        def m(elo, clave, depth):
            self.liMotores.append(MotorElo(elo, Util.primeraMayuscula(clave), clave, depth))

        m(1029, "gaia", 1)
        m(1046, "pawny", 1)
        m(1053, "tarrasch", 2)
        m(1060, "tarrasch", 3)
        # m( 1063, "daydreamer", 2 ) juega muy mal
        m(1087, "lime", 1)
        m(1100, "clarabit", 1)
        m(1104, "chispa", 1)
        m(1146, "alaric", 1)
        m(1146, "bikjump", 1)
        m(1166, "pawny", 2)
        m(1178, "gaia", 2)
        m(1200, "garbochess", 1)
        m(1215, "chispa", 2)
        m(1227, "bikjump", 2)
        m(1237, "umko", 1)
        m(1251, "clarabit", 2)
        m(1274, "cyrano", 1)
        m(1278, "ufim", 1)
        m(1296, "alaric", 2)
        m(1303, "toga", 1)
        m(1318, "tarrasch", 4)
        m(1325, "stockfish", 1)
        m(1337, "lime", 2)
        m(1396, "komodo", 1)
        m(1403, "gaia", 3)
        m(1406, "critter", 1)
        m(1423, "clarabit", 3)
        m(1423, "pawny", 3)
        m(1435, "glaurung", 2)
        m(1438, "garbochess", 2)
        m(1470, "bikjump", 3)
        m(1477, "glaurung", 1)
        m(1479, "chispa", 3)
        m(1497, "lime", 3)
        m(1511, "ufim", 2)
        m(1519, "alaric", 3)
        m(1521, "umko", 2)
        m(1526, "stockfish", 2)
        m(1546, "toga", 2)
        m(1548, "komodo", 2)
        m(1555, "cyrano", 2)
        m(1558, "daydreamer", 3)
        m(1575, "critter", 2)
        m(1602, "pawny", 4)
        m(1626, "clarabit", 4)
        m(1646, "gaia", 4)
        m(1683, "stockfish", 3)
        m(1685, "bikjump", 4)
        m(1685, "glaurung", 3)
        m(1690, "daydreamer", 4)
        m(1702, "garbochess", 3)
        m(1727, "alaric", 4)
        m(1744, "umko", 3)
        m(1746, "komodo", 3)
        m(1754, "lime", 4)
        m(1759, "cyrano", 3)
        m(1781, "toga", 3)
        m(1783, "chispa", 4)
        m(1798, "critter", 3)
        m(1805, "stockfish", 4)
        m(1830, "rybka", 1)
        m(1849, "ufim", 3)
        m(1854, "glaurung", 4)
        m(1879, "garbochess", 4)
        m(1901, "komodo", 4)
        m(1908, "umko", 4)
        m(1925, "critter", 4)
        m(1928, "rybka", 2)
        m(1933, "toga", 4)
        m(1933, "ufim", 4)
        m(1955, "cyrano", 4)
        m(2026, "rybka", 3)
        m(2055, "rybka", 4)

        li = []
        for k, v in self.configuracion.dicRivales.iteritems():
            if v.elo > 2000:
                li.append((v.clave, v.elo))
        li = sorted(li, key=operator.itemgetter(1))
        for clave, elo in li:
            m(elo, clave, None)  # ponemos depth a None, para diferenciar del 0 de los motores internos

        self.liT = (
            (0, 50, 3), (20, 53, 5), (40, 58, 4), (60, 62, 4), (80, 66, 5), (100, 69, 4), (120, 73, 3), (140, 76, 3),
            (160, 79, 3), (180, 82, 2), (200, 84, 9), (300, 93, 4), (400, 97, 3) )
        self.liK = ( (0, 60), (800, 50), (1200, 40), (1600, 30), (2000, 30), (2400, 10) )

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
            if mtElo > elo + 400:
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
            self.apertura = Apertura.AperturaPol(100)  # 100 is any number
            self.apertura.recuperaEstado(aplazamiento["ESTADOAPERTURA"])

            self.datosMotor = MotorElo(aplazamiento["ELO"], aplazamiento["NOMBRE"], aplazamiento["CLAVE"],
                                       aplazamiento["DEPTH"])
            self.datosMotor.pgana = aplazamiento["PGANA"]
            self.datosMotor.ppierde = aplazamiento["PPIERDE"]
            self.datosMotor.ptablas = aplazamiento["PTABLAS"]
            self.datosMotor.siInterno = False
        else:
            self.apertura = Apertura.AperturaPol(datosMotor.depthOpen)
            self.datosMotor = datosMotor

        eloengine = self.datosMotor.elo
        eloplayer = self.configuracion.eloActivo(siCompetitivo)
        self.whiteElo = eloplayer if siBlancas else eloengine
        self.blackElo = eloplayer if not siBlancas else eloengine

        self.siRivalInterno = self.datosMotor.siInterno
        if self.siRivalInterno:
            self.xrival = MotorInternoGM.GestorMotor(self.datosMotor.clave, self)

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
        rotulo = "%s: <b>%s</b>" % (_("Opponent"), self.datosMotor.rotulo() )
        self.ponRotulo1(rotulo)

        nbsp = "&nbsp;" * 3

        txt = "%s:%+d%s%s:%+d%s%s:%+d" % ( _("Win"), self.datosMotor.pgana, nbsp,
                                           _("Draw"), self.datosMotor.ptablas, nbsp,
                                           _("Lost"), self.datosMotor.ppierde )

        self.ponRotulo2("<center>%s</center>" % txt)
        self.pgnRefresh(True)
        self.ponCapInfoPorDefecto()

        #-Aplazamiento 2/2--------------------------------------------------
        if aplazamiento:
            self.mueveJugada(kMoverFinal)

        self.ponPosicionDGT()

        self.siguienteJugada()

    def ponToolBar(self):
        if self.pteToolRendirse:
            liTool = ( k_cancelar, k_aplazar, k_atras, k_configurar, k_utilidades )
        else:
            if self.siCompetitivo:
                liTool = ( k_rendirse, k_tablas, k_aplazar, k_configurar, k_utilidades )
            else:
                liTool = ( k_rendirse, k_tablas, k_aplazar, k_atras, k_configurar, k_utilidades )

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
            jgUltima = self.partida.liJugadas[-1]
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

            iniT = time.clock()

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

            difT = time.clock() - iniT
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

        if self.siJuegaHumano:
            self.paraHumano()
        else:
            self.sigueHumano()
            return False

        # Peón coronando
        if not coronacion and self.partida.ultPosicion.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.partida.ultPosicion.siBlancas)
            if coronacion is None:
                self.sigueHumano()
                return False

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)

        if self.siTeclaPanico:
            self.sigueHumano()
            return False

        if siBien:

            if self.siTeclaPanico:
                self.sigueHumano()
                return False

            if self.siApertura:
                self.siApertura = self.apertura.compruebaHumano(self.fenUltimo(), desde, hasta)

            self.movimientosPiezas(jg.liMovs)

            self.partida.ultPosicion = jg.posicion
            self.masJugada(jg, True)
            self.error = ""
            self.siguienteJugada()
            return True
        else:
            self.sigueHumano()
            self.error = mens
            return False

    def masJugada(self, jg, siNuestra):

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

        mensaje = _("End Game")
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

        elif quien == kGanaRivalTiempo:
            mensaje = _X(_("%1 has won on time."), nombreContrario)
            self.resultado = kGanaRival

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
        QTUtil2.mensaje(self.pantalla, mensaje)
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
        clave = "%s-%d" % (self.datosMotor.nombre, self.datosMotor.depth if self.datosMotor.depth else 0 )
        dd[clave] = self.siJugamosConBlancas
        dd.close()

    def determinaColor(self, datosMotor):
        clave = "%s-%d" % (datosMotor.nombre, datosMotor.depth if datosMotor.depth else 0)
        if not self.siCompetitivo:
            clave += "NC"

        dd = Util.DicSQL(self.configuracion.fichEstadElo, tabla="color")
        previo = dd.get(clave, random.randint(0, 1) == 0)
        dd.close()
        return not previo

    def atras(self):
        if self.partida.numJugadas() > 2:
            self.partida.anulaUltimoMovimiento(self.siJugamosConBlancas)
            self.listaAperturasStd.asignaApertura(self.partida)
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

