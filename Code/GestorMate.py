import time

from Code import TrListas
from Code import ControlPosicion
from Code import Gestor
from Code import Jugada
from Code import Partida
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code import Util
from Code.Constantes import *


class ConfigNivel:
    def __init__(self, mate):
        self.mate = mate

        eduardo = Util.listfiles("Trainings", "Checkmates by Eduardo Sadier", "*.fns")[0]
        pascal3 = "Trainings/From Pascal Georges in Scid/Mate in 3.fns"
        pascal4 = "Trainings/From Pascal Georges in Scid/Mate in 4 and more.fns"
        dicFicheros = {1: eduardo, 2: eduardo, 3: pascal3, 4: pascal4}

        self.fichero = dicFicheros[mate]

        liNivelMaximo = [500, 700, 80, 30]
        self.nivelMaximo = liNivelMaximo[mate - 1]
        liNivelPosiciones = [15, 10, 7, 5]
        self.posicionesNivel = liNivelPosiciones[mate - 1]
        liDepth = [2, 4, 6, 10]
        self.depth = liDepth[mate - 1]

        fmt = "./IntFiles/Mate/mate%d.lst" % mate
        with open(fmt) as f:
            dic = {}
            for nlinea, linea in enumerate(f):
                linea = linea.strip()
                if linea:
                    dic[nlinea] = [uno.split(",") for uno in linea.split("|")]

        # dic = Util.recuperaVar("./IntFiles/mate.nvd")
        self.dicMate = dic
        self.nivelMaximo = len(self.dicMate) / 10

    def dameFenPV(self, nivel, bloque):
        return self.dicMate[nivel * 10 + bloque]


class MateBloque:
    def __init__(self):
        self.errores = None
        self.tiempo = None

    def ponDatos(self, errores, tiempo):
        self.errores = errores
        self.tiempo = tiempo

    def siTerminado(self):
        return not ((self.errores is None) or (self.errores > 0))


class MateNivel:
    def __init__(self, nivel):
        self.nivel = nivel
        self.liMateBloques = []
        for x in range(10):
            self.liMateBloques.append(MateBloque())

    def ponBloque(self, bloque, mateBloque):
        self.liMateBloques[bloque] = mateBloque

    def dameBloque(self, bloque):
        return self.liMateBloques[bloque]

    def grabaBloque(self, bloque, errores, tiempo):
        mateb = self.liMateBloques[bloque]
        siGraba = False
        if mateb.errores is None:
            siGraba = True
        elif mateb.errores > errores:
            siGraba = True
        elif mateb.errores == errores and mateb.tiempo > tiempo:
            siGraba = True

        if siGraba:
            mateb.errores = errores
            mateb.tiempo = tiempo

        return siGraba  # Si nuevo record

    def siTerminado(self):
        for mateb in self.liMateBloques:
            if not mateb.siTerminado():
                return False
        return True

    def numBloqueSinHacer(self):
        for n, mateb in enumerate(self.liMateBloques):
            if not mateb.siTerminado():
                return n
        return 0


class MateDB(Util.DicSQL):
    def __init__(self, gestor, mate):
        nomFichero = gestor.configuracion.ficheroMate
        Util.DicSQL.__init__(self, nomFichero, tabla="m%d" % mate)

    def ultimoNivel(self):
        li = self.keys()

        if li:
            li.sort()
            mateNivel = self.__getitem__(li[-1])
        else:
            mateNivel = MateNivel(0)

        return mateNivel

    def nivel(self, nivel):
        clave = "%04d" % (nivel,)
        if clave in self:
            mateNivel = self.__getitem__(clave)
        else:
            mateNivel = MateNivel(nivel)
        return mateNivel


class ControlMate:
    def __init__(self, gestor, mate):

        self.db = MateDB(gestor, mate)

        self.mate = mate

        self.mateNivel = self.db.ultimoNivel()

        self.configNivel = ConfigNivel(mate)
        # self.configNivel.hd()
        # self.configNivel = ConfigNivel(mate)

    def basadoEn(self):
        fich = self.configNivel.fichero[:-4]
        li = fich.split("/" if "/" in fich else "\\")
        d = TrListas.dicTraining()
        base = li[1]
        txt = d.get(base, _F(base))
        if len(li) == 3:
            fich = li[2]
            txt += ", " + d.get(fich, _F(fich))
        return txt

    def numDatos(self):
        return 10  # 1 grupo = siempre 10 bloques

    def analisis(self, fila, clave):  # compatibilidad
        return ""

    def soloJugada(self, fila, clave):  # compatibilidad
        return None

    def conInformacion(self, fila, clave):  # compatibilidad
        return None

    def mueve(self, fila, clave):  # compatibilidad
        return False

    def dato(self, fila, clave):
        if clave == "NIVEL":
            return str(fila + 1)
        else:
            mateBloque = self.mateNivel.dameBloque(fila)

            if clave == "ERRORES":
                errores = mateBloque.errores
                return "-" if errores is None else str(errores)
            if clave == "TIEMPO":
                tiempo = mateBloque.tiempo
                return "-" if tiempo is None else str(tiempo)
        return ""

    def inicia(self, bloque):

        self.bloque = bloque

        self.liFenPV = self.configNivel.dameFenPV(self.mateNivel.nivel, bloque)
        self.posFenPV = -1

        self.tiempoInicial = time.time()

    def final(self, errores):
        tiempo = int(time.time() - self.tiempoInicial)

        siRecord = self.mateNivel.grabaBloque(self.bloque, errores, tiempo)

        # Grabamos, si Record
        if siRecord:
            nivel = self.mateNivel.nivel
            clave = "%04d" % (nivel,)
            self.db[clave] = self.mateNivel

        return siRecord, tiempo

    def siguienteFenPV(self):
        self.posFenPV += 1
        return (None, None) if self.posFenPV == self.configNivel.posicionesNivel else self.liFenPV[self.posFenPV]

    def repiteFenPV(self):
        return self.liFenPV[self.posFenPV]

    def ultimoNivel(self):
        return self.db.ultimoNivel()

    def ponNivel(self, nivel):
        self.mateNivel = self.db.nivel(nivel)

    def depth(self):
        return self.configNivel.depth


class GestorMate(Gestor.Gestor):
    def inicio(self, mate):

        self.mate = mate

        self.controlMate = self.pgn = ControlMate(self, mate)  # El pgn lo usamos para mostrar el nivel actual

        self.pantalla.columnas60(True, cNivel=_("Blk"))

        self.pantalla.activaJuego(True, False, siAyudas=False)
        self.quitaAyudas(True, False)
        self.quitaCapturas()
        self.ponPiezasAbajo(True)
        self.ponMensajero(self.mueveHumano)
        self.pgnRefresh(True)
        self.pantalla.base.pgn.gotop()

        self.pantalla.ponRotulo1("<center><h1>%s</h1></center>" % _X(_("Mate in %1"), str(mate)))
        self.pantalla.ponRotulo2(_X(_("Based : %1"), self.controlMate.basadoEn()))
        self.tablero.exePulsadoNum = None

        self.lbNivel = self.pantalla.base.lbJugBlancas.ponColorFondoN("black", "#EDEFF1")

        # self.lbBloque = self.pantalla.base.lbJugNegras.ponColorFondoN( "black", "#EDEFF1" )

        self.finJuego()

        rival = self.configuracion.buscaRival(self.configuracion.tutorInicial)
        rival.ponMultiPV(0, 0)

        self.xrival = self.procesador.creaGestorMotor(rival, None, self.controlMate.depth())

        self.quitaInformacion()

        self.refresh()

    def numDatos(self):
        return self.pgn.numDatos()

    def procesarAccion(self, clave):

        if clave == k_mainmenu:  # No es al modo estandar porque hay una partida dentro de la partida
            self.finJuego()

        elif clave == k_terminar:
            self.finPartida()

        elif clave == k_mateNivel:
            self.cambiarJuego()

        elif clave == k_abandonar:
            self.finJuego()

        elif clave == k_reiniciar:
            self.repiteMate(False, self.numMov > 0)

        elif clave == k_ayuda:
            self.ayudaMate()

        else:
            Gestor.Gestor.rutinaAccionDef(self, clave)

    def finalX(self):
        self.finPartida()
        return False

    def finPartida(self):
        self.lbNivel.ponColorFondoN("black", "white")

        self.tablero.quitaFlechas()
        self.pantalla.columnas60(False)
        self.procesador.inicio()

    def finJuego(self):

        self.ponRotuloNivel()
        self.ponRotuloBloque(False)
        self.pantalla.base.pgn.show()

        self.pantalla.ponToolBar((k_terminar, k_mateNivel))
        self.desactivaTodas()
        self.estado = kFinJuego
        self.refresh()

    def cambiarJuego(self):
        mateg = self.controlMate.ultimoNivel()
        ult_nivel = mateg.nivel + 1
        ult_bloque = mateg.numBloqueSinHacer() + 1
        if mateg.siTerminado():
            ult_nivel += 1
            ult_bloque = 1

        liGen = [(None, None)]

        config = FormLayout.Spinbox(_("Level"), 1, ult_nivel, 50)
        liGen.append((config, ult_nivel))

        config = FormLayout.Spinbox(_("Block"), 1, 10, 50)
        liGen.append((config, ult_bloque))

        resultado = FormLayout.fedit(liGen, title=_("Level"), parent=self.pantalla, icon=Iconos.Jugar())

        if resultado:
            nv, bl = resultado[1]
            nv -= 1
            bl -= 1
            self.controlMate.ponNivel(nv)
            self.ponRotuloNivel()
            self.refresh()
            self.jugar(bl)

    def ponRotuloNivel(self):
        nivel = self.controlMate.mateNivel.nivel

        txt = "%s : %d/%d" % (_("Level"), nivel + 1, self.controlMate.configNivel.nivelMaximo)

        self.lbNivel.ponTexto(txt)
        self.lbNivel.show()

    def ponRotuloBloque(self, siPoner):
        txt = "<center><h1>%s</h1>" % _X(_("Mate in %1"), str(self.mate))
        if siPoner:
            bloque = self.controlMate.bloque + 1
            posicion = self.controlMate.posFenPV + 1
            errores = self.errores
            totbloques = self.controlMate.configNivel.posicionesNivel

            txt += "<h3>%s : %d </h3><h3> %s : %d </h3><h3> %d / %d</h3>" % (
                _("Block"), bloque, _("Errors"), errores, posicion, totbloques)
        self.pantalla.ponRotulo1(txt + "</center>")

    def siguienteMate(self):
        fen, pv = self.controlMate.siguienteFenPV()
        if fen is None:
            # Hemos terminado el bloque
            siRecord, tiempo = self.controlMate.final(self.errores)
            txt = "<center><h3> %s : %d</h3><h3>%s : %d </h3></center>" % (
                _("Errors"), self.errores, _("Second(s)"), tiempo)

            if siRecord:
                txt += "<h3>%s</h3>" % _("Congratulations you have achieved a new record in this block.")

            self.mensajeEnPGN(txt)
            self.finJuego()

        else:
            fen += " w - - 0 1"
            self.iniciaPosicion(fen)

    def repiteMate(self, siMensaje, siError):

        if siError:
            self.errores += 1
        fen, pv = self.controlMate.repiteFenPV()
        fen += " w - - 0 1"

        if siMensaje:
            QTUtil2.mensErrorSobreControl(self.pantalla, _("Incorrect. Try again."), self.lbNivel)

        self.iniciaPosicion(fen)

    def ayudaMate(self):
        self.errores += 1
        self.ponRotuloBloque(True)
        self.siAyuda = True

        if self.numMov == 0:
            fen, pv = self.controlMate.repiteFenPV()

        else:
            rm = self.xrival.juega()
            if rm.mate != self.mate - self.numMov:
                self.repiteMate(False, False)
                self.ayudaMate()
                return

            pv = rm.pv.split(" ")[0]

        self.tablero.creaFlechaMov(pv[:2], pv[2:4], "2")

    def iniciaPosicion(self, fen):

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fen)
        self.ponPosicion(cp)
        self.activaColor(True)
        self.siJugamosConBlancas = cp.siBlancas
        self.tablero.quitaFlechas()
        self.partida = Partida.Partida(cp)
        li = [k_mainmenu]
        if self.mate > 1:
            li.append(k_reiniciar)
        li.append(k_ayuda)
        self.pantalla.ponToolBar(li)
        self.numMov = 0
        self.siAyuda = False

        self.ponRotuloBloque(True)
        # self.miraKibitzers(None, None)

        self.refresh()

    def jugar(self, bloque):

        if self.estado == kJugando:
            self.estado = kFinJuego
            self.desactivaTodas()
            self.finJuego()
            return

        self.controlMate.inicia(bloque)
        self.errores = 0

        self.estado = kJugando

        self.pantalla.base.pgn.hide()

        self.siguienteMate()

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.siJuegaHumano = True  # necesario para el check
        jg = self.checkMueveHumano(desde, hasta, coronacion)
        if not jg:
            return False

        self.partida.append_jg(jg)
        if self.siAyuda:
            self.tablero.quitaFlechas()
        self.movimientosPiezas(jg.liMovs, False)
        if self.siTerminada():
            if jg.siJaque:
                self.siguienteMate()
            else:
                self.repiteMate(True, True)
            return

        self.numMov += 1
        if self.numMov == self.mate:
            self.repiteMate(True, True)
            return

        # Juega rival con depth 3
        rm = self.xrival.juega()
        desde = rm.desde
        hasta = rm.hasta
        coronacion = rm.coronacion

        siBien, mens, jg = Jugada.dameJugada(self.partida.ultPosicion, desde, hasta, coronacion)
        self.partida.append_jg(jg)
        self.ponFlechaSC(jg.desde, jg.hasta)
        self.movimientosPiezas(jg.liMovs, False)
        if self.siTerminada():
            self.repiteMate(True, True)
            return
        self.activaColor(True)  # Caso en que hay coronacion, sino no se activa la dama

    def analizaPosicion(self, fila, clave):
        if self.estado == kJugando:
            self.finJuego()
            return
        self.jugar(fila)
