import time

from PyQt4 import QtGui, QtCore

from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaPGN
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import TrListas
from Code import Util


class LearnPGNs(Util.DicSQL):
    def __init__(self, fichero):
        Util.DicSQL.__init__(self, fichero)
        self.regKeys = self.keys(True, True)

    def leeRegistro(self, num):
        return self.__getitem__(self.regKeys[num])

    def append(self, valor):
        k = str(Util.hoy())
        self.__setitem__(k, valor)
        self.regKeys = self.keys(True, True)

    def cambiaRegistro(self, num, valor):
        self.__setitem__(self.regKeys[num], valor)

    def borraRegistro(self, num):
        self.__delitem__(self.regKeys[num])
        self.regKeys = self.keys(True, True)

    def borraLista(self, li):
        li.sort()
        li.reverse()
        for x in li:
            self.__delitem__(self.regKeys[x])
        self.pack()
        self.regKeys = self.keys(True, True)


class WLearnBase(QTVarios.WDialogo):
    def __init__(self, procesador):

        titulo = _("Memorize a game")
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, Iconos.LearnGame(), "learngame")

        self.procesador = procesador
        self.configuracion = procesador.configuracion

        self.db = LearnPGNs(self.configuracion.ficheroLearnPGN)

        # Historico
        oColumnas = Columnas.ListaColumnas()

        def creaCol(clave, rotulo, siCentrado=True):
            oColumnas.nueva(clave, rotulo, 80, siCentrado=siCentrado)

        # # Claves segun orden estandar
        liBasic = ("EVENT", "SITE", "DATE", "ROUND", "WHITE", "BLACK", "RESULT", "ECO", "FEN", "WHITEELO", "BLACKELO")
        for clave in liBasic:
            rotulo = TrListas.pgnLabel(clave)
            creaCol(clave, rotulo, clave != "EVENT")
        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("New"), Iconos.Nuevo(), self.nuevo), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Learn"), Iconos.Empezar(), self.empezar),
        )
        self.tb = QTVarios.LCTB(self, liAcciones)

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.grid).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.grid)
        self.recuperarVideo(siTam=False)

        self.grid.gotop()

    def gridDobleClick(self, grid, fila, columna):
        self.empezar()

    def gridNumDatos(self, grid):
        return len(self.db)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.db.leeRegistro(fila)
        return reg.get(col, "")

    def terminar(self):
        self.guardarVideo()
        self.db.close()
        self.accept()

    def nuevo(self):
        unpgn = PantallaPGN.eligePartida(self)
        if unpgn and unpgn.partida.numJugadas():
            reg = unpgn.dic
            unpgn.partida.siTerminada()
            reg["PARTIDA"] = unpgn.partida.guardaEnTexto()
            self.db.append(reg)
            self.grid.refresh()
            self.grid.gotop()

    def borrar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.db.borraLista(li)
        self.grid.gotop()
        self.grid.refresh()

    def empezar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            w = WLearn1(self, li[0])
            w.exec_()


class WLearn1(QTVarios.WDialogo):
    def __init__(self, owner, numRegistro):

        QTVarios.WDialogo.__init__(self, owner, _("Learn a game"), Iconos.PGN(), "learn1game")

        self.owner = owner
        self.db = owner.db
        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion
        self.numRegistro = numRegistro
        self.registro = self.db.leeRegistro(numRegistro)

        self.partida = Partida.Partida()
        self.partida.recuperaDeTexto(self.registro["PARTIDA"])

        self.lbRotulo = Controles.LB(self, self.rotulo()).ponTipoLetra(puntos=12).ponColorFondoN("#076C9F", "#EFEFEF")

        self.liIntentos = self.registro.get("LIINTENTOS", [])

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("DATE", _("Date"), 80, siCentrado=True)
        oColumnas.nueva("LEVEL", _("Level"), 80, siCentrado=True)
        oColumnas.nueva("COLOR", _("Play with"), 80, siCentrado=True)
        oColumnas.nueva("ERRORS", _("Errors"), 80, siCentrado=True)
        oColumnas.nueva("HINTS", _("Hints"), 80, siCentrado=True)
        oColumnas.nueva("TIME", _("Time"), 80, siCentrado=True)
        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), "terminar"), None,
            (_("Train"), Iconos.Empezar(), "empezar"), None,
            (_("Remove"), Iconos.Borrar(), "borrar"), None,
        )
        self.tb = Controles.TB(self, liAcciones)

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.grid).control(self.lbRotulo).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.grid)
        self.recuperarVideo(siTam=False)

        self.grid.gotop()

    def rotulo(self):
        r = self.registro

        def x(k):
            return r.get(k, "")

        return "%s-%s : %s %s %s" % (x("WHITE"), x("BLACK"), x("DATE"), x("EVENT"), x("SITE"))

    def gridNumDatos(self, grid):
        return len(self.liIntentos)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.liIntentos[fila]

        if col == "DATE":
            f = reg["DATE"]
            return "%02d/%02d/%d-%02d:%02d" % (f.day, f.month, f.year, f.hour, f.minute)
        if col == "LEVEL":
            return str(reg["LEVEL"])
        if col == "COLOR":
            c = reg["COLOR"]
            if c == "b":
                return _("Black")
            elif c == "w":
                return _("White")
            else:
                return _("White") + "+" + _("Black")
        if col == "ERRORS":
            return str(reg["ERRORS"])
        if col == "HINTS":
            return str(reg["HINTS"])
        if col == "TIME":
            s = reg["SECONDS"]
            m = s / 60
            s -= m * 60
            return "%d\' %d\"" % (m, s)

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def guardar(self, dic):
        self.liIntentos.insert(0, dic)
        self.grid.refresh()
        self.grid.gotop()
        self.registro["LIINTENTOS"] = self.liIntentos
        self.db.cambiaRegistro(self.numRegistro, self.registro)

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def borrar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                li.sort()
                li.reverse()
                for x in li:
                    del self.liIntentos[x]
        self.grid.gotop()
        self.grid.refresh()

    def empezar(self):
        regBase = self.liIntentos[0] if self.liIntentos else {}

        liGen = [(None, None)]

        liGen.append((FormLayout.Spinbox(_("Level"), 0, self.partida.numJugadas(), 40), regBase.get("LEVEL", 0)))
        liGen.append((None, None))
        liGen.append((None, _("User play with") + ":"))
        liGen.append((_("White"), "w" in regBase.get("COLOR", "bw")))
        liGen.append((_("Black"), "b" in regBase.get("COLOR", "bw")))
        liGen.append((None, None))
        liGen.append((_("Show clock"), True))

        resultado = FormLayout.fedit(liGen, title=_("New try"), anchoMinimo=200, parent=self,
                                     icon=Iconos.TutorialesCrear())
        if resultado is None:
            return

        accion, liResp = resultado
        level = liResp[0]
        white = liResp[1]
        black = liResp[2]
        if not (white or black):
            return
        siClock = liResp[3]

        w = WLearnPuente(self, self.partida, level, white, black, siClock)
        w.exec_()


class WLearnPuente(QTVarios.WDialogo):
    INICIO, FINAL_JUEGO, REPLAY, REPLAY_CONTINUE = range(4)

    def __init__(self, owner, partida, nivel, white, black, siClock):

        QTVarios.WDialogo.__init__(self, owner, owner.rotulo(), Iconos.PGN(), "learnpuente")

        self.owner = owner
        self.procesador = owner.procesador
        self.configuracion = self.procesador.configuracion
        self.partida = partida
        self.nivel = nivel
        self.white = white
        self.black = black
        self.siClock = siClock

        self.repTiempo = 3000
        self.repWorking = False

        # Tool bar
        self.tb = Controles.TB(self, [])

        self.ponToolbar(self.INICIO)

        # Tableros
        confTablero = self.configuracion.confTablero("LEARNPGN", 48)

        self.tableroIni = Tablero.Tablero(self, confTablero)
        self.tableroIni.crea()
        self.tableroIni.ponMensajero(self.mueveHumano, None)
        self.lbIni = Controles.LB(self).alinCentrado().ponColorFondoN("#076C9F", "#EFEFEF").anchoMinimo(
                self.tableroIni.ancho)
        lyIni = Colocacion.V().control(self.tableroIni).control(self.lbIni)

        self.tableroFin = Tablero.TableroEstatico(self, confTablero)
        self.tableroFin.crea()
        self.lbFin = Controles.LB(self).alinCentrado().ponColorFondoN("#076C9F", "#EFEFEF").anchoMinimo(
                self.tableroFin.ancho)
        lyFin = Colocacion.V().control(self.tableroFin).control(self.lbFin)

        # Rotulo tiempo
        f = Controles.TipoLetra(puntos=30, peso=75)
        self.lbReloj = Controles.LB(self, "00:00").ponFuente(f).alinCentrado().ponColorFondoN("#076C9F",
                                                                                              "#EFEFEF").anchoMinimo(200)
        self.lbReloj.setFrameStyle(QtGui.QFrame.Box | QtGui.QFrame.Raised)

        # Movimientos
        flb = Controles.TipoLetra(puntos=11)
        self.lbInfo = Controles.LB(self).anchoFijo(200).ponWrap().ponFuente(flb)

        # Layout
        lyC = Colocacion.V().control(self.lbReloj).control(self.lbInfo).relleno()
        lyTM = Colocacion.H().otro(lyIni).otro(lyC).otro(lyFin).relleno()

        ly = Colocacion.V().control(self.tb).otro(lyTM).relleno().margen(3)

        self.setLayout(ly)

        self.recuperarVideo()
        self.adjustSize()

        if siClock:
            self.timer = QtCore.QTimer(self)
            self.connect(self.timer, QtCore.SIGNAL("timeout()"), self.ajustaReloj)
            self.timer.start(500)
        else:
            self.lbReloj.hide()

        self.reset()

    def procesarTB(self):
        getattr(self, self.sender().clave)()

    def ponToolbar(self, tipo):

        if tipo == self.INICIO:
            liAcciones = (
                (_("Cancel"), Iconos.Cancelar(), "cancelar"), None,
                (_("Reinit"), Iconos.Reiniciar(), "reset"), None,
                (_("Help"), Iconos.AyudaGR(), "ayuda"), None,
            )
        elif tipo == self.FINAL_JUEGO:
            liAcciones = (
                (_("Close"), Iconos.MainMenu(), "final"), None,
                (_("Reinit"), Iconos.Reiniciar(), "reset"), None,
                (_("Replay game"), Iconos.Pelicula(), "replay"), None,
            )
        elif tipo == self.REPLAY:
            liAcciones = (
                (_("Cancel"), Iconos.Cancelar(), "repCancelar"), None,
                (_("Reinit"), Iconos.Inicio(), "repReiniciar"), None,
                (_("Slow"), Iconos.Pelicula_Lento(), "repSlow"), None,
                (_("Pause"), Iconos.Pelicula_Pausa(), "repPause"), None,
                (_("Fast"), Iconos.Pelicula_Rapido(), "repFast"), None,
            )
        elif tipo == self.REPLAY_CONTINUE:
            liAcciones = (
                (_("Cancel"), Iconos.Cancelar(), "repCancelar"), None,
                (_("Continue"), Iconos.Pelicula_Seguir(), "repContinue"), None,
            )
        self.tb.reset(liAcciones)

    def replay(self):
        self.ponToolbar(self.REPLAY)

        self.repJugada = -1
        self.repWorking = True
        self.tableroIni.ponPosicion(self.partida.iniPosicion)
        self.replayDispatch()

    def replayDispatch(self):
        QTUtil.refreshGUI()
        if not self.repWorking:
            return
        self.repJugada += 1
        self.ponInfo()
        numJugadas = self.partida.numJugadas()
        if self.repJugada == numJugadas:
            self.ponToolbar(self.FINAL_JUEGO)
            return

        jg = self.partida.jugada(self.repJugada)
        self.tableroIni.ponPosicion(jg.posicion)
        self.tableroIni.ponFlechaSC(jg.desde, jg.hasta)
        self.lbIni.ponTexto("%d/%d" % (self.repJugada + 1, numJugadas))

        self.tableroFin.ponPosicion(jg.posicion)
        self.tableroFin.ponFlechaSC(jg.desde, jg.hasta)
        self.lbFin.ponTexto("%d/%d" % (self.repJugada + 1, numJugadas))

        QtCore.QTimer.singleShot(self.repTiempo, self.replayDispatch)

    def repCancelar(self):
        self.repWorking = False
        self.ponToolbar(self.FINAL_JUEGO)
        self.ponInfo()

    def repReiniciar(self):
        self.repJugada = -1

    def repSlow(self):
        self.repTiempo += 500

    def repFast(self):
        if self.repTiempo >= 800:
            self.repTiempo -= 500
        else:
            self.repTiempo = 200

    def repPause(self):
        self.repWorking = False
        self.ponToolbar(self.REPLAY_CONTINUE)

    def repContinue(self):
        self.repWorking = True
        self.ponToolbar(self.REPLAY)
        self.replayDispatch()

    def reset(self):
        if self.siClock:
            self.timer.start()
        self.baseTiempo = time.time()
        self.tableroIni.ponPosicion(self.partida.iniPosicion)

        self.movActual = -1

        self.errors = 0
        self.hints = 0

        self.siguiente()

    def ponInfo(self):
        njg = self.repJugada if self.repWorking else self.movActual - 1
        txtPGN = self.partida.pgnSP(hastaJugada=njg)
        texto = "<big><center><b>%s</b>: %d<br><b>%s</b>: %d</center><br><br>%s</big>" % (
            _("Errors"), self.errors, _("Hints"), self.hints, txtPGN)
        self.lbInfo.ponTexto(texto)

    def siguiente(self):
        numJugadas = self.partida.numJugadas()
        self.movActual += 1
        self.ponInfo()
        self.lbIni.ponTexto("%d/%d" % (self.movActual, numJugadas))
        if self.movActual == numJugadas:
            self.finalJuego()
            return

        jg = self.partida.jugada(self.movActual)

        self.tableroIni.ponPosicion(jg.posicionBase)
        if self.movActual > 0:
            jgant = self.partida.jugada(self.movActual - 1)
            self.tableroIni.ponFlechaSC(jgant.desde, jgant.hasta)

        mfin = self.movActual + self.nivel - 1
        if self.nivel == 0:
            mfin += 1
        if mfin >= numJugadas:
            mfin = numJugadas - 1

        jgf = self.partida.jugada(mfin)
        self.tableroFin.ponPosicion(jgf.posicion)
        if self.nivel == 0:
            self.tableroFin.ponFlechaSC(jgf.desde, jgf.hasta)
        self.lbFin.ponTexto("%d/%d" % (mfin + 1, numJugadas))

        color = jg.posicionBase.siBlancas

        if (color and self.white) or (not color and self.black):
            self.tableroIni.activaColor(color)
        else:
            self.siguiente()

    def mueveHumano(self, desde, hasta, coronacion=""):

        jg = self.partida.jugada(self.movActual)

        # Peon coronando
        if not coronacion and jg.posicionBase.siPeonCoronando(desde, hasta):
            coronacion = self.tableroIni.peonCoronando(jg.posicionBase.siBlancas)
            if coronacion is None:
                coronacion = "q"

        if desde == jg.desde and hasta == jg.hasta and coronacion.lower() == jg.coronacion.lower():
            self.tableroIni.ponFlechaSC(desde, hasta)
            self.siguiente()
            return False  # Que actualice solo siguiente
        else:
            if hasta != desde:
                self.errors += 1
                self.tableroIni.ponFlechasTmp([(jg.desde, jg.hasta, False)])
            self.ponInfo()
            return False

    def ayuda(self):

        jg = self.partida.jugada(self.movActual)
        self.tableroIni.ponFlechaSC(jg.desde, jg.hasta)
        self.hints += 1

        self.ponInfo()

    def guardar(self):
        color = ""
        if self.white:
            color += "w"
        if self.black:
            color += "b"
        dic = {}
        dic["SECONDS"] = int(time.time() - self.baseTiempo)
        dic["DATE"] = Util.hoy()
        dic["LEVEL"] = self.nivel
        dic["COLOR"] = color
        dic["HINTS"] = self.hints
        dic["ERRORS"] = self.errors
        self.owner.guardar(dic)

    def finalJuego(self):
        numJugadas = self.partida.numJugadas()
        self.lbIni.ponTexto("%d/%d" % (numJugadas, numJugadas))
        self.tableroIni.ponPosicion(self.partida.ultPosicion)
        if self.siClock:
            self.timer.stop()
        self.guardar()

        self.ponToolbar(self.FINAL_JUEGO)

    def final(self):
        if self.siClock:
            self.timer.stop()
        self.siClock = False
        self.guardarVideo()
        self.accept()

    def cancelar(self):
        self.final()

    def ajustaReloj(self):
        if self.siClock:
            s = int(time.time() - self.baseTiempo)

            m = s / 60
            s -= m * 60

            self.lbReloj.ponTexto("%02d:%02d" % (m, s))
