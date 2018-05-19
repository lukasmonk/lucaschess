import time

from Code import Gestor
from Code import Jugada
from Code import ControlPosicion
from Code import TrListas
from Code.QT import QTUtil2
from Code.QT import Iconos
from Code import Util
from Code import OpeningLines
from Code import XMotorRespuesta
from Code import Partida
from Code.Constantes import *


class GestorOpeningLines(Gestor.Gestor):
    def inicio(self, pathFichero, modo, num_linea):
        self.tablero.saveVisual()

        self.pathFichero = pathFichero
        dbop = OpeningLines.Opening(pathFichero)
        self.tablero.dbVisual_setFichero(dbop.nomFichero)
        self.reinicio(dbop, modo, num_linea)

    def reinicio(self, dbop, modo, num_linea):
        self.dbop = dbop
        self.tipoJuego = kJugOpeningLines

        self.modo = modo
        self.num_linea = num_linea

        self.training = self.dbop.training()
        self.liGames = self.training["LIGAMES_%s" % modo.upper()]
        self.game = self.liGames[num_linea]
        self.liPV = self.game["LIPV"]
        self.numPV = len(self.liPV)

        self.calc_totalTiempo()

        self.dicFENm2 = self.training["DICFENM2"]
        li = self.dbop.getNumLinesPV(self.liPV)
        if len(li) > 10:
            mensLines = ",".join(["%d"%line for line in li[:10]]) + ", ..."
        else:
            mensLines = ",".join(["%d"%line for line in li])
        self.liMensBasic = [
            "%d/%d" % (self.num_linea+1, len(self.liGames)),
            "%s: %s" % (_("Lines"), mensLines),
        ]

        self.siAyuda = False
        self.tablero.dbVisual_setShowAllways(False)

        self.partida = Partida.Partida()

        self.ayudas = 9999  # Para que analice sin problemas

        self.siJugamosConBlancas = self.training["COLOR"] == "WHITE"
        self.siRivalConBlancas = not self.siJugamosConBlancas

        self.pantalla.ponToolBar((k_mainmenu, k_ayuda, k_reiniciar))
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(self.partida.ultPosicion)
        self.mostrarIndicador(True)
        self.quitaAyudas()
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.pgnRefresh(True)

        self.ponCapInfoPorDefecto()

        self.estado = kJugando

        self.ponPosicionDGT()

        self.errores = 0
        self.ini_time = time.time()
        self.muestraInformacion()
        self.siguienteJugada()

    def calc_totalTiempo(self):
        self.tm = 0
        for game in self.liGames:
            for tr in game["TRIES"]:
                self.tm += tr["TIME"]

    def ayuda(self):
        self.siAyuda = True
        self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_configurar, k_utilidades))
        self.tablero.dbVisual_setShowAllways(True)

        self.muestraAyuda()
        self.muestraInformacion()

    def muestraInformacion(self):
        li = []
        li.append("%s: %d" %(_("Errors"), self.errores))
        if self.siAyuda:
            li.append(_("Help activated"))
        self.ponRotulo1("\n".join(li))

        tgm = 0
        for tr in self.game["TRIES"]:
            tgm += tr["TIME"]

        mens = "\n" + "\n".join(self.liMensBasic)
        mens += "\n%s:\n    %s %s\n    %s %s" % (_("Working time"),
                                                    time.strftime("%H:%M:%S", time.gmtime(tgm)), _("Current"),
                                                    time.strftime("%H:%M:%S", time.gmtime(self.tm)), _("Total"))

        self.ponRotulo2(mens)

        if self.siAyuda:
            dicNAGs = TrListas.dicNAGs()
            mens3 = ""
            fenM2 = self.partida.ultPosicion.fenM2()
            reg = self.dbop.getfenvalue(fenM2)
            if reg:
                mens3 = reg.get("COMENTARIO", "")
                ventaja = reg.get("VENTAJA", 0)
                valoracion = reg.get("VALORACION", 0)
                if ventaja:
                    mens3 += "\n %s" % dicNAGs[ventaja]
                if valoracion:
                    mens3 += "\n %s" % dicNAGs[valoracion]
            self.ponRotulo3(mens3 if mens3 else None)


    def partidaTerminada(self, siCompleta):
        self.estado = kFinJuego
        tm = time.time() - self.ini_time
        li = [_("Line finished.")]
        if self.siAyuda:
            li.append(_("Help activated"))
        if self.errores > 0:
            li.append("%s: %d" % (_("Errors"), self.errores))

        if siCompleta:
            QTUtil2.mensaje(self.pantalla, "\n".join(li))

        dictry = {
            "DATE": Util.hoy(),
            "TIME": tm,
            "AYUDA": self.siAyuda,
            "ERRORS": self.errores
        }
        self.game["TRIES"].append(dictry)

        sinError = self.errores == 0 and not self.siAyuda
        if siCompleta:
            if sinError:
                self.game["NOERROR"] += 1
                noError = self.game["NOERROR"]
                if self.modo == "sequential":
                    salto = 2**(noError + 1)
                    numGames = len(self.liGames)
                    for x in range(salto, numGames):
                        game = self.liGames[x]
                        if game["NOERROR"] != noError:
                            salto = x
                            break

                    liNuevo = self.liGames[1:salto]
                    liNuevo.append(self.game)
                    if numGames > salto:
                        liNuevo.extend(self.liGames[salto:])
                    self.training["LIGAMES_SEQUENTIAL"] = liNuevo
                    self.pantalla.ponToolBar((k_mainmenu, k_siguiente))
                else:
                    self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_configurar, k_utilidades))
            else:
                self.game["NOERROR"] -= 1

                self.pantalla.ponToolBar((k_mainmenu, k_reiniciar, k_configurar, k_utilidades))
        else:
            if not sinError:
                self.game["NOERROR"] -= 1
        self.game["NOERROR"] = max(0, self.game["NOERROR"])

        self.dbop.setTraining(self.training)
        self.estado = kFinJuego
        self.calc_totalTiempo()
        self.muestraInformacion()

    def muestraAyuda(self):
        pv = self.liPV[len(self.partida)]
        self.tablero.creaFlechaMov(pv[:2], pv[2:4], "mt80")
        fenM2 = self.partida.ultPosicion.fenM2()
        for pv1 in self.dicFENm2[fenM2]:
            if pv1 != pv:
                self.tablero.creaFlechaMov(pv1[:2], pv1[2:4], "ms40")

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_reiniciar :
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_siguiente:
            self.reinicio(self.dbop, self.modo, self.num_linea)

        elif clave == k_ayuda:
            self.ayuda()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        return self.finPartida()

    def finPartida(self):
        self.dbop.close()
        self.tablero.restoreVisual()
        self.procesador.inicio()
        if self.modo == "static":
            self.procesador.openingsTrainingStatic(self.pathFichero)
        else:
            self.procesador.openings()
        return False

    def reiniciar(self):
        if len(self.partida) > 0 and self.estado != kFinJuego:
            self.partidaTerminada(False)
        self.reinicio(self.dbop, self.modo, self.num_linea)

    def siguienteJugada(self):
        self.muestraInformacion()
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        siRival = siBlancas == self.siRivalConBlancas

        numJugadas = len(self.partida)
        if numJugadas >= self.numPV:
            self.partidaTerminada(True)
            return
        pv = self.liPV[numJugadas]

        if siRival:
            self.desactivaTodas()

            self.rmRival = XMotorRespuesta.RespuestaMotor("Apertura", self.siRivalConBlancas)
            self.rmRival.desde = pv[:2]
            self.rmRival.hasta = pv[2:4]
            self.rmRival.coronacion = pv[4:]

            self.mueveRival(self.rmRival)
            self.siguienteJugada()

        else:
            self.activaColor(siBlancas)
            self.siJuegaHumano = True
            if self.siAyuda:
                self.muestraAyuda()

    def mueveHumano(self, desde, hasta, coronacion=""):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False
        pvSel = desde + hasta + coronacion
        pvObj = self.liPV[len(self.partida)]

        if pvSel != pvObj:
            fenM2 = jg.posicionBase.fenM2()
            li = self.dicFENm2[fenM2]
            if pvSel in li:
                mens = _("You have selected a correct move, but this line uses another one.")
                QTUtil2.mensajeTemporal(self.pantalla, mens, 2, posicion="tb", background="#C3D6E8")
                self.sigueHumano()
                return False

            self.errores += 1
            mens = "%s: %d" % (_("Error"), self.errores)
            QTUtil2.mensajeTemporal(self.pantalla, mens, 1.2, posicion="ad", background="#FF9B00", pmImagen=Iconos.pmError())
            self.muestraInformacion()
            self.sigueHumano()
            return False

        self.movimientosPiezas(jg.liMovs)

        self.masJugada(jg, True)
        self.siguienteJugada()
        return True

    def masJugada(self, jg, siNuestra):
        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()

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


class GestorOpeningLinesPositions(Gestor.Gestor):
    def inicio(self, pathFichero):
        self.pathFichero = pathFichero
        dbop = OpeningLines.Opening(pathFichero)
        self.reinicio(dbop)

    def reinicio(self, dbop):
        self.dbop = dbop
        self.tipoJuego = kJugOpeningLines

        self.training = self.dbop.training()
        self.liTrainPositions = self.training["LITRAINPOSITIONS"]
        self.trposition = self.liTrainPositions[0]

        self.tm = 0
        for game in self.liTrainPositions:
            for tr in game["TRIES"]:
                self.tm += tr["TIME"]

        self.liMensBasic = [
            "%s: %d" % (_("Moves"), len(self.liTrainPositions)),
        ]

        self.siAyuda = False

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.trposition["FENM2"] + " 0 1")

        self.partida = Partida.Partida(iniPosicion=cp)

        self.ayudas = 9999  # Para que analice sin problemas

        self.siJugamosConBlancas = self.training["COLOR"] == "WHITE"
        self.siRivalConBlancas = not self.siJugamosConBlancas

        self.pantalla.ponToolBar((k_mainmenu, k_ayuda, k_reiniciar))
        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.ponMensajero(self.mueveHumano)
        self.ponPosicion(cp)
        self.mostrarIndicador(True)
        self.quitaAyudas()
        self.ponPiezasAbajo(self.siJugamosConBlancas)
        self.pgnRefresh(True)

        self.ponCapInfoPorDefecto()

        self.estado = kJugando

        self.ponPosicionDGT()

        self.quitaInformacion()

        self.errores = 0
        self.ini_time = time.time()
        self.muestraInformacion()
        self.siguienteJugada()

    def ayuda(self):
        self.siAyuda = True
        self.pantalla.ponToolBar((k_mainmenu, k_reiniciar))

        self.muestraAyuda()
        self.muestraInformacion()

    def muestraInformacion(self):
        li = []
        li.append("%s: %d" %(_("Errors"), self.errores))
        if self.siAyuda:
            li.append(_("Help activated"))
        self.ponRotulo1("\n".join(li))

        tgm = 0
        for tr in self.trposition["TRIES"]:
            tgm += tr["TIME"]

        mas = time.time() - self.ini_time

        mens = "\n" + "\n".join(self.liMensBasic)
        mens += "\n%s:\n    %s %s\n    %s %s" % (_("Working time"),
                                                    time.strftime("%H:%M:%S", time.gmtime(tgm+mas)), _("Current"),
                                                    time.strftime("%H:%M:%S", time.gmtime(self.tm+mas)), _("Total"))

        self.ponRotulo2(mens)

    def posicionTerminada(self):
        tm = time.time() - self.ini_time
        li = [_("Finished.")]
        if self.siAyuda:
            li.append(_("Help activated"))
        if self.errores > 0:
            li.append("%s: %d" % (_("Errors"), self.errores))

        QTUtil2.mensajeTemporal(self.pantalla, "\n".join(li), 1.2)

        dictry = {
            "DATE": Util.hoy(),
            "TIME": tm,
            "AYUDA": self.siAyuda,
            "ERRORS": self.errores
        }
        self.trposition["TRIES"].append(dictry)

        sinError = self.errores == 0 and not self.siAyuda
        if sinError:
            self.trposition["NOERROR"] += 1
        else:
            self.trposition["NOERROR"] = max(0, self.trposition["NOERROR"]-1)
        noError = self.trposition["NOERROR"]
        salto = 2**(noError + 1) + 1
        numPosics = len(self.liTrainPositions)
        for x in range(salto, numPosics):
            posic = self.liTrainPositions[x]
            if posic["NOERROR"] != noError:
                salto = x
                break

        liNuevo = self.liTrainPositions[1:salto]
        liNuevo.append(self.trposition)
        if numPosics > salto:
            liNuevo.extend(self.liTrainPositions[salto:])
        self.training["LITRAINPOSITIONS"] = liNuevo
        self.pantalla.ponToolBar((k_mainmenu, k_siguiente))

        self.dbop.setTraining(self.training)
        self.estado = kFinJuego
        self.muestraInformacion()

    def muestraAyuda(self):
        liMoves = self.trposition["MOVES"]
        for pv in liMoves:
            self.tablero.creaFlechaMov(pv[:2], pv[2:4], "mt80")

    def procesarAccion(self, clave):
        if clave == k_mainmenu:
            self.finPartida()

        elif clave == k_reiniciar :
            self.reiniciar()

        elif clave == k_configurar:
            self.configurar(siSonidos=True)

        elif clave == k_utilidades:
            self.utilidades()

        elif clave == k_siguiente:
            self.reinicio(self.dbop)

        elif clave == k_ayuda:
            self.ayuda()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        return self.finPartida()

    def finPartida(self):
        self.dbop.close()
        self.procesador.inicio()
        self.procesador.openings()
        return False

    def reiniciar(self):
        self.reinicio(self.dbop)

    def siguienteJugada(self):
        self.muestraInformacion()
        if self.estado == kFinJuego:
            return

        self.estado = kJugando

        self.siJuegaHumano = False
        self.ponVista()

        siBlancas = self.partida.ultPosicion.siBlancas

        self.ponIndicador(siBlancas)
        self.refresh()

        self.activaColor(siBlancas)
        self.siJuegaHumano = True
        if self.siAyuda:
            self.muestraAyuda()

    def mueveHumano(self, desde, hasta, coronacion=""):
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False
        pvSel = desde + hasta + coronacion
        lipvObj = self.trposition["MOVES"]

        if pvSel not in lipvObj:
            self.errores += 1
            mens = "%s: %d" % (_("Error"), self.errores)
            QTUtil2.mensajeTemporal(self.pantalla, mens, 2, posicion="ad", background="#FF9B00")
            self.muestraInformacion()
            self.sigueHumano()
            return False

        self.movimientosPiezas(jg.liMovs)

        self.masJugada(jg, True)
        self.posicionTerminada()
        return True

    def masJugada(self, jg, siNuestra):
        self.partida.append_jg(jg)
        if self.partida.pendienteApertura:
            self.partida.asignaApertura()

        self.ponFlechaSC(jg.desde, jg.hasta)
        self.beepExtendido(siNuestra)

        self.pgnRefresh(self.partida.ultPosicion.siBlancas)
        self.refresh()

        self.ponPosicionDGT()