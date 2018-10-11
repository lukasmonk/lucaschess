import collections

from Code import PGNreader
from Code import Util
from Code.Constantes import *


class ControlPGN:
    def __init__(self, gestor):
        self.gestor = gestor
        self.siFigurines = gestor.configuracion.figurinesPGN
        # self.partida = self.gestor.partida
        self.siMostrar = True
        self.showVariantes = gestor.configuracion.showVariantes

    def numDatos(self):
        if self.gestor.partida:
            n = self.gestor.partida.numJugadas()
            if self.gestor.partida.siEmpiezaConNegras:
                n += 1
            if n % 2 == 1:
                n += 1
            return n / 2
        else:
            return 0

    def soloJugada(self, fila, clave):
        lj = self.gestor.partida.liJugadas

        pos = fila * 2
        tam_lj = len(lj)

        if clave == "BLANCAS":
            if self.gestor.partida.siEmpiezaConNegras:
                pos -= 1
        else:
            if not self.gestor.partida.siEmpiezaConNegras:
                pos += 1

        if 0 <= pos <= (tam_lj - 1):
            return lj[pos]
        else:
            return None

    def dato(self, fila, clave):
        if clave == "NUMERO":
            return str(self.gestor.partida.primeraJugada() + fila)

        jg = self.soloJugada(fila, clave)
        if jg:
            if self.siMostrar:
                return jg.pgnFigurinesSP() if self.siFigurines else jg.pgnSP()
            else:
                return "-"
        else:
            return " "

    def conInformacion(self, fila, clave):
        if clave == "NUMERO":
            return None
        jg = self.soloJugada(fila, clave)
        if jg:
            if jg.siApertura or jg.critica or jg.comentario or jg.variantes:
                return jg
        return None

    def analisis(self, fila, clave):
        if clave == "NUMERO":
            return None
        return self.soloJugada(fila, clave)
        # if jg:
        # return jg.analisis
        # else:
        # return None

    def liVariantesPV(self, jg):
        liResp = []
        if jg.variantes:
            fen = jg.posicionBase.fen()
            for una in jg.variantes.split("\n"):

                g = PGNreader.Game()
                g.fen = fen
                g.readBody(una)
                if g.plies():
                    liResp.append((g.move(0).desde, g.move(0).hasta))

        return liResp

    def mueve(self, fila, siBlancas):
        siEmpiezaConNegras = self.gestor.partida.siEmpiezaConNegras

        if fila == 0 and siBlancas and siEmpiezaConNegras:
            return

        lj = self.gestor.partida.liJugadas
        pos = fila * 2
        if not siBlancas:
            pos += 1
        if siEmpiezaConNegras:
            pos -= 1

        tam_lj = len(lj)
        if tam_lj:

            siUltimo = (pos + 1) >= tam_lj
            if siUltimo:
                pos = tam_lj - 1

            jg = self.gestor.partida.jugada(pos)
            self.gestor.ponPosicion(jg.posicion)

            lipvvar = []
            if self.showVariantes:
                if jg.variantes:
                    fen = jg.posicionBase.fen()
                    for una in jg.variantes.split("\n"):

                        g = PGNreader.Game()
                        g.fen = fen
                        g.readBody(una)
                        if g.plies():
                            lipvvar.append((g.move(0).desde, g.move(0).hasta))
            self.gestor.ponFlechaSC(jg.desde, jg.hasta, lipvvar)

            if siUltimo:
                self.gestor.ponRevision(False)
                if self.gestor.siJuegaHumano and self.gestor.estado == kJugando:
                    self.gestor.activaColor(self.gestor.siJugamosConBlancas)
            else:
                self.gestor.ponRevision(self.gestor.estado == kJugando)
                self.gestor.desactivaTodas()
            self.gestor.refresh()

    def jugada(self, fila, clave):
        siBlancas = clave != "NEGRAS"

        pos = fila * 2
        if not siBlancas:
            pos += 1
        if self.gestor.partida.siEmpiezaConNegras:
            pos -= 1
        tam_lj = self.gestor.partida.numJugadas()
        if tam_lj == 0:
            return None, None

        if pos >= self.gestor.partida.numJugadas():
            pos = self.gestor.partida.numJugadas() - 1

        return pos, self.gestor.partida.jugada(pos)

    def actual(self):
        tipoJuego = self.gestor.tipoJuego

        if tipoJuego == kJugGM:
            return self.actualGM()
        elif tipoJuego in (kJugPGN, kJugSolo, kJugRoute, kJugEntLight, kJugAnotar):
            return self.gestor.actualPGN()

        if tipoJuego == kJugRemoto:
            rival = self.gestor.rival
        elif tipoJuego == kJugBooks:
            rival = self.gestor.libro.nombre
        elif tipoJuego in (kJugFics, kJugFide):
            rival = self.gestor.nombreObj
        elif self.gestor.xrival:  # foncap change
            rival = self.gestor.xrival.nombre  # foncap change
        else:  # foncap change
            rival = ""  # foncap change

        jugador = self.gestor.configuracion.jugador
        resultado = self.gestor.resultado
        siJugamosConBlancas = self.gestor.siJugamosConBlancas

        if resultado == kGanamos:
            r = "1-0" if siJugamosConBlancas else "0-1"
        elif resultado == kGanaRival:
            r = "0-1" if siJugamosConBlancas else "1-0"
        elif resultado == kTablas:
            r = "1/2-1/2"
        else:
            r = "*"
        if siJugamosConBlancas:
            blancas = jugador
            negras = rival
        else:
            blancas = rival
            negras = jugador
        hoy = Util.hoy()
        resp = '[Event "%s"]\n' % _("Lucas Chess")
        # Site (lugar): el lugar donde el evento se llevo a cabo.
        # Esto debe ser en formato "Ciudad, Region PAIS", donde PAIS es el codigo del mismo
        # en tres letras de acuerdo al codigo del Comite Olimpico Internacional. Como ejemplo: "Mexico, D.F. MEX".
        resp += '[Date "%d.%02d.%02d"]\n' % (hoy.year, hoy.month, hoy.day)
        # Round (ronda): La ronda original de la partida.
        resp += '[White "%s"]\n' % blancas
        resp += '[Black "%s"]\n' % negras
        resp += '[Result "%s"]\n' % r

        if self.gestor.fen:
            resp += '[FEN "%s"]\n' % self.gestor.fen

        xrival = getattr(self.gestor, "xrival", None)
        if xrival and tipoJuego not in [kJugRemoto, kJugBooks]:
            if xrival.motorProfundidad:
                resp += '[Depth "%d"]\n' % xrival.motorProfundidad

            if xrival.motorTiempoJugada:
                resp += '[TimeEngineMS "%d"]\n' % xrival.motorTiempoJugada

            if self.gestor.categoria:
                resp += '[Category "%s"]\n' % self.gestor.categoria.nombre()

        if tipoJuego not in [kJugBooks, kJugBoxing]:
            if self.gestor.ayudasPGN:
                resp += '[Hints "%d"]\n' % self.gestor.ayudasPGN

        if tipoJuego in (kJugElo, kJugMicElo):
            resp += '[WhiteElo "%d"]\n' % self.gestor.whiteElo
            resp += '[BlackElo "%d"]\n' % self.gestor.blackElo

        ap = self.gestor.partida.apertura
        if ap:
            resp += '[ECO "%s"]\n' % ap.eco
            resp += '[Opening "%s"]\n' % ap.trNombre

        dmore = getattr(self.gestor, "pgnLabelsAdded", None)
        if dmore:
            for k, v in dmore().iteritems():
                resp += '[%s "%s"]\n' % (k, v)

        resp += "\n" + self.gestor.partida.pgnBase()
        if not resp.endswith(r):
            resp += " %s" % r

        return resp

    def actualGM(self):
        gm = self.gestor.gm
        motorGM = self.gestor.motorGM

        partidaGM = motorGM.getLastGame()

        if partidaGM:
            event = partidaGM.event
            oponent = partidaGM.oponent
            fecha = partidaGM.date
            result = partidaGM.result
        else:
            event = "?"
            oponent = "?"
            fecha = "????.??.??"
            result = "*"

        if self.gestor.siBlancas:
            blancas = gm
            negras = oponent
        else:
            blancas = oponent
            negras = gm

        resp = '[Event "%s"]\n' % event
        resp += '[Date "%s"]\n' % fecha
        resp += '[White "%s"]\n' % blancas
        resp += '[Black "%s"]\n' % negras
        resp += '[Result "%s"]\n' % result.strip()

        ap = self.gestor.partida.apertura
        if ap:
            resp += '[ECO "%s"]\n' % ap.eco
            resp += '[Opening "%s"]\n' % ap.trNombre

        resp += "\n" + self.gestor.partida.pgnBase() + " " + result.strip()

        return resp

    def dicCabeceraActual(self):
        resp = self.actual()
        dic = collections.OrderedDict()
        for linea in resp.split("\n"):
            linea = linea.strip()
            if linea.startswith("["):
                li = linea.split('"')
                if len(li) == 3:
                    clave = li[0][1:].strip()
                    valor = li[1]
                    dic[clave] = valor
            else:
                break
        return dic
