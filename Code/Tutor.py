import codecs
import os

import LCEngine4 as LCEngine

from Code import Analisis
from Code import Partida
from Code.QT import PantallaTutor
from Code.QT import QTUtil2
from Code import Util


class Tutor:
    def __init__(self, procesador, gestor, jg, desde, hasta, siEntrenando):
        self.procesador = procesador
        self.gestor = gestor

        self.difpts = procesador.configuracion.tutorDifPts
        self.difporc = procesador.configuracion.tutorDifPorc

        self.partida = gestor.partida

        self.pantalla = procesador.pantalla
        self.gestorTutor = gestor.xtutor
        self.ultPosicion = self.partida.ultPosicion
        self.jg = jg
        self.desde = desde
        self.hasta = hasta
        self.mrmTutor = gestor.mrmTutor
        self.rmRival = gestor.rmRival
        self.siBlancas = gestor.siJugamosConBlancas
        self.siEntrenando = siEntrenando
        self.listaRM = None  # necesario

        self.salvarCSV = gestor.configuracion.salvarCSV

        self.siMoviendoTiempo = False

    def elegir(self, siPuntos, liApPosibles=None):

        self.rmUsuario, posUsuario = self.mrmTutor.buscaRM(self.jg.movimiento())
        if self.rmUsuario is None:
            # Elegimos si la opcion del tutor es mejor que la del usuario
            # Ponemos un mensaje mientras piensa
            me = QTUtil2.mensEspera.inicio(self.pantalla, _("Analyzing the move...."), posicion="ad")

            fen = self.jg.posicion.fen()
            mrmUsuario = self.gestorTutor.analiza(fen)
            self.rmUsuario = mrmUsuario.liMultiPV[0]
            self.rmUsuario.cambiaColor(self.jg.posicion)

            me.final()

            if me.teclaPulsada(self.gestor.teclaPanico):
                self.gestor.siTeclaPanico = True
                return False

        # Estadisticas
        if self.salvarCSV:
            self.guardaEstadisticas()

        # Comparamos la puntuacion del usuario con la del tutor
        if not self.mrmTutor.mejorRMQue(self.rmUsuario, self.difpts, self.difporc):
            return False

        # Creamos la lista de movimientos analizados por el tutor
        self.listaRM = self.hazListaRM(posUsuario)  # rm,nombre

        # Creamos la ventana

        siRival = self.rmRival and " " in self.rmRival.getPV()

        self.liApPosibles = liApPosibles
        siApertura = not (liApPosibles is None)
        if siApertura:
            siRival = False

        self.w = w = PantallaTutor.PantallaTutor(self, self, siRival, siApertura, self.siBlancas, siPuntos)

        self.cambiadoRM(0)

        self.partidaUsuario = Partida.Partida(self.jg.posicion)
        self.partidaUsuario.append_jg(self.jg)
        self.partidaUsuario.leerPV(self.rmUsuario.getPV())
        self.posUsuario = 0
        self.maxUsuario = len(self.partidaUsuario.liJugadas)
        self.tableroUsuario.ponPosicion(self.jg.posicion)
        w.ponPuntuacionUsuario(self.rmUsuario.texto())

        if siRival:
            self.rmRival.cambiaColor()
            pvBloque = self.rmRival.getPV()
            n = pvBloque.find(" ")
            if n > 0:
                pvBloque = pvBloque[n + 1:].strip()
            else:
                pvBloque = ""

            if pvBloque:
                self.partidaRival = Partida.Partida(self.ultPosicion)
                self.partidaRival.leerPV(pvBloque)
                self.posRival = 0
                self.maxRival = len(self.partidaRival.liJugadas) - 1
                if self.maxRival >= 0:
                    self.tableroRival.ponPosicion(self.partidaRival.liJugadas[0].posicion)
                    self.mueveRival(True)
                    w.ponPuntuacionRival(self.rmRival.texto())

        self.mueveTutor(True)
        self.mueveUsuario(True)

        if w.exec_():
            if w.siElegidaApertura:
                desde = self.partidaAperturas.jugada(0).desde
                hasta = self.partidaAperturas.jugada(0).hasta
                if desde == self.desde and hasta == self.hasta:
                    return False
                self.desde = desde
                self.hasta = hasta
                self.coronacion = ""
            elif w.respLibro:
                self.desde, self.hasta, self.coronacion = w.respLibro
            else:
                rm = self.listaRM[self.posRM][0]
                self.desde = rm.desde
                self.hasta = rm.hasta
                self.coronacion = rm.coronacion
            return True
        return False

    def ponVariantes(self, jg, numJugada):
        if self.listaRM:
            rm, nombre = self.listaRM[0]
            partida = Partida.Partida(self.jg.posicionBase)
            partida.leerPV(rm.getPV())
            txt = partida.pgnBaseRAW(numJugada)
            puntos = rm.texto()

            li = txt.split(" ")
            n = 1 if self.jg.posicionBase.siBlancas else 2
            vtut = (" ".join(li[:n]) + " {%s} " % puntos + " ".join(li[n:])).replace("  ", " ").strip()

            self.partidaUsuario.siEmpiezaConNegras = not self.partidaUsuario.siEmpiezaConNegras
            txt = self.partidaUsuario.pgnBaseRAW(numJugada)
            puntos = self.rmUsuario.texto()
            vusu = "%s : %s" % (puntos, txt)
            jg.variantes = vtut.replace("\n", "")
            jg.comentario = vusu.replace("\n", "")

    def hazListaRM(self, posUsuario):
        li = []
        pb = self.jg.posicionBase

        for n, rm in enumerate(self.mrmTutor.liMultiPV):
            if n != posUsuario:
                pv1 = rm.getPV().split(" ")[0]
                desde = pv1[:2]
                hasta = pv1[2:4]
                coronacion = pv1[4] if len(pv1) == 5 else ""
                nombre = pb.pgnSP(desde, hasta, coronacion)
                nombre += " " + rm.abrTexto()

                li.append((rm, nombre))
        return li

    def cambiadoRM(self, pos):
        self.posRM = pos
        rm = self.listaRM[pos][0]
        self.partidaTutor = Partida.Partida(self.ultPosicion)
        self.partidaTutor.leerPV(rm.getPV())

        self.w.ponPuntuacionTutor(rm.texto())

        self.posTutor = 0
        self.maxTutor = len(self.partidaTutor)
        self.mueveTutor(True)

    def mueve(self, quien, que):

        quien = quien[0].upper() + quien[1:]
        funcion = eval("self.mueve" + quien)

        if que == "Adelante":
            funcion(nSaltar=1)
        elif que == "Atras":
            funcion(nSaltar=-1)
        elif que == "Inicio":
            funcion(siBase=True)
        elif que == "Final":
            funcion(siFinal=True)
        elif que == "Libre":
            self.analiza(quien)
        elif que == "Tiempo":
            tb = eval("self.w.tb" + quien)
            posMax = eval("self.max" + quien)
            self.mueveTiempo(funcion, tb, posMax)

    def mueveTiempo(self, funcion, tb, posMax):
        if self.siMoviendoTiempo:
            self.siMoviendoTiempo = False
            self.tiempoOtrosTB(True)
            self.w.paraReloj()
            return

        def otrosTB(siHabilitar):
            for accion in tb.liAcciones:
                if not accion.clave.endswith("MoverTiempo"):
                    accion.setEnabled(siHabilitar)

        self.tiempoFuncion = funcion
        self.tiempoPosMax = posMax
        self.tiempoPos = -1
        self.tiempoOtrosTB = otrosTB
        self.siMoviendoTiempo = True
        otrosTB(False)
        funcion(siBase=True)
        self.w.iniciaReloj(self.mueveTiempo1)

    def mueveTiempo1(self):
        self.tiempoPos += 1
        if self.tiempoPos == self.tiempoPosMax:
            self.siMoviendoTiempo = False
            self.tiempoOtrosTB(True)
            self.w.paraReloj()
            return
        if self.tiempoPos == 0:
            self.tiempoFuncion(siInicio=True)
        else:
            self.tiempoFuncion(nSaltar=1)

    def mueveUsuario(self, siInicio=False, nSaltar=0, siFinal=False, siBase=False):
        if nSaltar:
            pos = self.posUsuario + nSaltar
            if 0 <= pos < self.maxUsuario:
                self.posUsuario = pos
            else:
                return
        elif siInicio:
            self.posUsuario = 0
        elif siBase:
            self.posUsuario = -1
        else:
            self.posUsuario = self.maxUsuario - 1

        jg = self.partidaUsuario.jugada(self.posUsuario if self.posUsuario > -1 else 0)
        if siBase:
            self.tableroUsuario.ponPosicion(jg.posicionBase)
        else:
            self.tableroUsuario.ponPosicion(jg.posicion)
            self.tableroUsuario.ponFlechaSC(jg.desde, jg.hasta)

    def mueveTutor(self, siInicio=False, nSaltar=0, siFinal=False, siBase=False):
        if nSaltar:
            pos = self.posTutor + nSaltar
            if 0 <= pos < self.maxTutor:
                self.posTutor = pos
            else:
                return
        elif siInicio:
            self.posTutor = 0
        elif siBase:
            self.posTutor = -1
        else:
            self.posTutor = self.maxTutor - 1

        jg = self.partidaTutor.jugada(self.posTutor if self.posTutor > -1 else 0)
        if siBase:
            self.tableroTutor.ponPosicion(jg.posicionBase)
        else:
            self.tableroTutor.ponPosicion(jg.posicion)
            self.tableroTutor.ponFlechaSC(jg.desde, jg.hasta)

    def mueveRival(self, siInicio=False, nSaltar=0, siFinal=False, siBase=False):
        if nSaltar:
            pos = self.posRival + nSaltar
            if 0 <= pos < self.maxRival:
                self.posRival = pos
            else:
                return
        elif siInicio:
            self.posRival = 0
        elif siBase:
            self.posRival = -1
        else:
            self.posRival = self.maxRival - 1

        jg = self.partidaRival.jugada(self.posRival if self.posRival > -1 else 0)
        if siBase:
            self.tableroRival.ponPosicion(jg.posicionBase)
        else:
            self.tableroRival.ponPosicion(jg.posicion)
            self.tableroRival.ponFlechaSC(jg.desde, jg.hasta)

    def mueveApertura(self, siInicio=False, nSaltar=0, siFinal=False, siBase=False):
        if nSaltar:
            pos = self.posApertura + nSaltar
            if 0 <= pos < self.maxApertura:
                self.posApertura = pos
            else:
                return
        elif siInicio:
            self.posApertura = 0
        elif siBase:
            self.posApertura = -1
        else:
            self.posApertura = self.maxApertura - 1

        jg = self.partidaAperturas.jugada(self.posApertura if self.posApertura > -1 else 0)
        if siBase:
            self.tableroAperturas.ponPosicion(jg.posicionBase)
        else:
            self.tableroAperturas.ponPosicion(jg.posicion)
            self.tableroAperturas.ponFlechaSC(jg.desde, jg.hasta)

    def ponTablerosGUI(self, tableroTutor, tableroUsuario, tableroRival, tableroAperturas):
        self.tableroTutor = tableroTutor
        self.tableroTutor.exePulsadoNum = self.exePulsadoNumTutor
        self.tableroUsuario = tableroUsuario
        self.tableroUsuario.exePulsadoNum = self.exePulsadoNumUsuario
        self.tableroRival = tableroRival
        self.tableroAperturas = tableroAperturas

    def cambiarApertura(self, numero):
        self.partidaAperturas = Partida.Partida(self.ultPosicion)
        self.partidaAperturas.leerPV(self.liApPosibles[numero].a1h8)
        self.tableroAperturas.ponPosicion(self.partidaAperturas.jugada(0).posicion)
        self.maxApertura = len(self.partidaAperturas)
        self.mueveApertura(siInicio=True)

    def opcionesAperturas(self):
        return [(ap.trNombre, num) for num, ap in enumerate(self.liApPosibles)]

    def analiza(self, quien):
        if quien == "Tutor":
            rmTutor = self.listaRM[self.posRM][0]
            jg = self.partidaTutor.jugada(self.posTutor)
            pts = rmTutor.texto()
        else:
            jg = self.partidaUsuario.jugada(self.posUsuario)
            pts = self.rmUsuario.texto()

        Analisis.AnalisisVariantes(self.w, self.gestor.xtutor, jg, self.siBlancas, pts)

    def exePulsadoNumTutor(self, siActivar, numero):
        if numero in [1, 8]:
            if siActivar:
                # Que jugada esta en el tablero
                jg = self.partidaTutor.jugada(self.posTutor if self.posTutor > -1 else 0)
                if self.posTutor == -1:
                    fen = jg.posicionBase.fen()
                else:
                    fen = jg.posicion.fen()
                siBlancas = " w " in fen
                if siBlancas:
                    siMB = numero == 1
                else:
                    siMB = numero == 8
                self.tableroTutor.quitaFlechas()
                if self.tableroTutor.flechaSC:
                    self.tableroTutor.flechaSC.hide()
                li = LCEngine.getCaptures(fen, siMB)
                for m in li:
                    d = m.desde()
                    h = m.hasta()
                    self.tableroTutor.creaFlechaMov(d, h, "c")
            else:
                self.tableroTutor.quitaFlechas()
                if self.tableroTutor.flechaSC:
                    self.tableroTutor.flechaSC.show()

    def exePulsadoNumUsuario(self, siActivar, numero):
        if numero in [1, 8]:
            if siActivar:
                # Que jugada esta en el tablero
                jg = self.partidaUsuario.jugada(self.posUsuario if self.posUsuario > -1 else 0)
                if self.posUsuario == -1:
                    fen = jg.posicionBase.fen()
                else:
                    fen = jg.posicion.fen()
                siBlancas = " w " in fen
                if siBlancas:
                    siMB = numero == 1
                else:
                    siMB = numero == 8
                self.tableroUsuario.quitaFlechas()
                if self.tableroUsuario.flechaSC:
                    self.tableroUsuario.flechaSC.hide()
                li = LCEngine.getCaptures(fen, siMB)
                for m in li:
                    d = m.desde()
                    h = m.hasta()
                    self.tableroUsuario.creaFlechaMov(d, h, "c")
            else:
                self.tableroUsuario.quitaFlechas()
                if self.tableroUsuario.flechaSC:
                    self.tableroUsuario.flechaSC.show()

    def guardaEstadisticas(self):
        date = str(Util.hoy())
        li = date.split(" ")
        fecha = li[0]
        hora = li[1].split(".")[0]
        fen = self.jg.posicion.fen()
        rmTutor = self.mrmTutor.liMultiPV[0]
        suggested_move = rmTutor.desde + rmTutor.hasta + rmTutor.coronacion
        suggested_move_puntos = rmTutor.puntos
        suggested_move_mate = rmTutor.mate
        player_move = self.rmUsuario.desde + self.rmUsuario.hasta + self.rmUsuario.coronacion
        player_move_puntos = self.rmUsuario.puntos
        player_move_mate = self.rmUsuario.mate
        tutor = self.gestorTutor.nombre.replace('"', "").replace(';', "")
        tutor_tiempo = self.gestorTutor.motorTiempoJugada

        try:
            if not os.path.isfile(self.salvarCSV):
                f = codecs.open(self.salvarCSV, "w", "utf-8", 'ignore')
                ntutor = _("Tutor").replace('"', "").replace(';', "")
                jugador = _("Player").replace('"', "").replace(';', "")
                puntos = _("points")
                mate = _("Mate").lower()
                txt = '"%s";"%s";"FEN";"%s";"%s";"%s";"%s";"%s";"%s";"%s";"%s"\n' % (
                    _("Date"), _("Time"), ntutor, ntutor + "-" + puntos, ntutor + "-" + mate, jugador,
                    jugador + "-" + puntos, jugador + "-" + mate, ntutor + "-" + _("Engine"), _("Time engine MS"))
                f.write(txt)
            else:
                f = codecs.open(self.salvarCSV, "a", "utf-8", 'ignore')

            f.write('%s;%s;"%s";"%s";%d;%d;"%s";%d;%d;"%s";%d\n' % (
                fecha, hora, fen, suggested_move, suggested_move_puntos, suggested_move_mate,
                player_move, player_move_puntos, player_move_mate, tutor, tutor_tiempo))
            f.close()
        except:
            pass
