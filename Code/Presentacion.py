import random
import time
import codecs

import LCEngine4 as LCEngine

from Code import Util
from Code import ControlPosicion
from Code.QT import Iconos
from Code.QT import Controles
from Code.QT import QTUtil2
from Code.QT import QTVarios


class GestorM1:
    def __init__(self, procesador):
        self.pantalla = procesador.pantalla
        self.tablero = self.pantalla.tablero
        self.procesador = procesador

        fmt = "./IntFiles/Mate/mate1.lst"
        with open(fmt) as f:
            li = [linea for linea in f.read().split("\n") if linea.strip()]
        linea = random.choice(li)
        li = linea.split("|")
        uno = random.choice(li)
        fen, mv0 = uno.split(",")
        fen += " w - - 1 1"
        LCEngine.setFen(fen)
        liMv = LCEngine.getExMoves()
        self.liMovs = []
        for mv in liMv:
            if mv.mate():
                self.liMovs.append(mv.movimiento())

        self.cp = ControlPosicion.ControlPosicion()
        self.cp.leeFen(fen)

        self.iniTime = time.time()

        self.siBlancas = " w " in fen
        self.tablero.bloqueaRotacion(False)
        self.tablero.ponMensajero(self.mueveHumano)
        self.tablero.ponPosicion(self.cp)
        self.tablero.ponerPiezasAbajo(self.siBlancas)
        self.tablero.activaColor(self.siBlancas)
        self.tablero.ponIndicador(self.siBlancas)

    def muestraMensaje(self):
        tm = time.time() - self.iniTime

        mensaje = "%s: %0.02f" % (_("Time"), tm)
        if tm < 10.0:
            pmImagen = Iconos.pmSol()
        elif tm < 20.0:
            pmImagen = Iconos.pmSolNubes()
        elif tm < 30.0:
            pmImagen = Iconos.pmNubes()
        else:
            pmImagen = Iconos.pmTormenta()

        background = "#5DB0DB"

        segundos = 2.6
        QTUtil2.mensajeTemporal(self.pantalla, mensaje, segundos, background=background, pmImagen=pmImagen)

    def mueveHumano(self, desde, hasta, coronacion=None):
        movimiento = desde + hasta
        if not coronacion and self.cp.siPeonCoronando(desde, hasta):
            coronacion = self.tablero.peonCoronando(self.siBlancas)
            if coronacion is None:
                return False
        if coronacion:
            movimiento += coronacion.lower()

        for mov in self.liMovs:
            if mov.lower() == movimiento:
                self.tablero.desactivaTodas()
                self.cp.mover(desde, hasta, coronacion)
                self.tablero.ponPosicion(self.cp)
                self.tablero.ponFlechaSC(desde, hasta)

                self.muestraMensaje()
                self.__init__(self.procesador)
                return True

        return False


class GestorChallenge101:
    def __init__(self, procesador):
        self.pantalla = procesador.pantalla
        self.tablero = self.pantalla.tablero
        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.cod_variables = "challenge101"
        self.puntos_totales = 0
        self.puntos_ultimo = 0
        self.pendientes = 10
        self.st_randoms = set()
        self.st_lines = set() # para no salvar mas de una vez una linea
        self.key = Util.hoy()
        random.seed()

        fmt = "./IntFiles/tactic0.bm"

        # Test if trainings exist
        # folder = "Trainings/Challenge101"
        # if not os.path.isdir(folder):
            # os.mkdir(folder)
            # temp = "Trainings/Challenge101/Difficulty %s.fns"
            # with open(fmt) as f:
                # for linea in f:
                    # fen, result, pgn_result, pgn, difficult = linea.strip().split("|")
                    # line = "%s|Challenge 101|%s|%s\n" %(fen, pgn_result, pgn)
                    # with open(temp % difficult, "ab") as q:
                        # q.write(line)

        with open(fmt) as f:
            self.li_lineas_posicion = [linea for linea in f if linea.strip()]

        self.siguiente_posicion()

    def siguiente_posicion(self):
        num_lineas_posicion = len(self.li_lineas_posicion)
        while True:
            random_pos = random.randint(0, num_lineas_posicion-1)
            if random_pos not in self.st_randoms:
                self.st_randoms.add(random_pos)
                break
        self.fen, self.result, self.pgn_result, self.pgn, self.difficult = self.li_lineas_posicion[random_pos].strip().split("|")
        self.difficult = int(self.difficult)

        self.cp = ControlPosicion.ControlPosicion()
        self.cp.leeFen(self.fen)

        self.siBlancas = " w " in self.fen
        self.tablero.bloqueaRotacion(False)
        self.tablero.ponMensajero(self.mueveHumano)
        self.tablero.ponPosicion(self.cp)
        self.tablero.ponerPiezasAbajo(self.siBlancas)
        self.tablero.activaColor(self.siBlancas)
        self.tablero.ponIndicador(self.siBlancas)

        self.intentos = 0
        self.max_intentos = (self.difficult+1)/2+4
        self.iniTime = time.time()

    def lee_results(self):
        dic = self.configuracion.leeVariables(self.cod_variables)
        results = dic.get("RESULTS", [])
        results.sort(key=lambda x:-x[1])
        return results

    def guarda_puntos(self):
        dic = self.configuracion.leeVariables(self.cod_variables)
        results = dic.get("RESULTS", [])
        if len(results) >= 10:
            ok = False
            for k, pts in results:
                if pts <= self.puntos_totales:
                    ok = True
                    break
        else:
            ok = True
        if ok:
            ok_find = False
            for n in range(len(results)):
                if results[n][0] == self.key:
                    ok_find = True
                    results[n] = (self.key, self.puntos_totales)
                    break
            if not ok_find:
                results.append((self.key, self.puntos_totales))
            results.sort(reverse=True, key=lambda x:"%4d%s" % (x[1], x[0]))
            if len(results) > 10:
                results = results[:10]
            dic["RESULTS"] = results
            self.configuracion.escVariables(self.cod_variables, dic)

    def menu(self):
        pantalla = self.procesador.pantalla
        pantalla.cursorFueraTablero()
        menu = QTVarios.LCMenu(pantalla)
        f = Controles.TipoLetra(nombre="Courier New", puntos=10)
        fbold = Controles.TipoLetra(nombre="Courier New", puntos=10, peso=700)
        fbolds = Controles.TipoLetra(nombre="Courier New", puntos=10, peso=500, siSubrayado=True)
        menu.ponFuente(f)

        li_results = self.lee_results()
        icono = Iconos.PuntoAzul()

        menu.separador()
        titulo = ("** %s **" % _("Challenge 101")).center(30)
        if self.pendientes == 0:
            menu.opcion("close", titulo, Iconos.Terminar())
        else:
            menu.opcion("continuar", titulo, Iconos.Pelicula_Seguir())
        menu.separador()
        ok_en_lista = False
        for n, (fecha, pts) in enumerate(li_results, 1):
            if fecha == self.key:
                ok_en_lista = True
                ico = Iconos.PuntoEstrella()
                tipoLetra = fbolds
            else:
                ico = icono
                tipoLetra = None
            txt = str(fecha)[:16]
            menu.opcion( None, "%2d. %-20s %6d" % (n, txt, pts), ico, tipoLetra=tipoLetra)

        menu.separador()
        menu.opcion(None, "")
        menu.separador()
        if self.puntos_ultimo:
            menu.opcion(None, ("+%d" % (self.puntos_ultimo)).center(30), tipoLetra=fbold)
        if self.pendientes == 0:
            if not ok_en_lista:
                menu.opcion(None, ("%s: %d" % (_("Score"), self.puntos_totales)).center(30), tipoLetra=fbold)
            menu.separador()
            menu.opcion("close", _("GAME OVER").center(30), Iconos.Terminar())
        else:
            menu.opcion(None, ("%s: %d" % (_("Score"), self.puntos_totales)).center(30), tipoLetra=fbold)
            menu.separador()
            menu.opcion(None, ("%s: %d" % (_("Positions left"), self.pendientes)).center(30), tipoLetra=fbold)
            menu.separador()
            menu.opcion(None, "")
            menu.separador()
            menu.opcion("continuar", _("Continue"), Iconos.Pelicula_Seguir())
            menu.separador()
            menu.opcion("close", _("Close"), Iconos.MainMenu())

        resp = menu.lanza()

        return not (resp == "close" or self.pendientes == 0)

    def mueveHumano(self, desde, hasta, coronacion=None):
        self.savePosition() # Solo cuando ha hecho un intento
        self.puntos_ultimo = 0
        if desde + hasta == self.result: # No hay coronaciones
            tm = time.time() - self.iniTime
            self.tablero.desactivaTodas()
            self.cp.mover(desde, hasta, coronacion)
            self.tablero.ponPosicion(self.cp)
            self.tablero.ponFlechaSC(desde, hasta)

            puntos = int(1000 - (1000/self.max_intentos)*self.intentos)
            if tm > 5.0:
                puntos -= int((tm-5.0)*5.0/3.0)

            if puntos > 0:
                self.puntos_totales += puntos
                self.guarda_puntos()
                self.puntos_ultimo = puntos

            self.pendientes -= 1
            if self.menu():
                self.siguiente_posicion()
            return True
        else:
            self.intentos += 1
            if self.intentos < self.max_intentos:
                QTUtil2.mensajeTemporalSinImagen(self.pantalla, str(self.max_intentos-self.intentos), 0.5, puntos=24, background="#ffd985")
            else:
                self.tablero.ponPosicion(self.cp)
                self.tablero.ponFlechaSC(self.result[:2], self.result[2:])
                self.pendientes = 0
                self.menu()
                return True
        return False

    def savePosition(self):
        line = "%s|%s|%s|%s\n" %(self.fen, str(self.key)[:19], self.pgn_result, self.pgn)
        if line not in self.st_lines:
            self.st_lines.add(line)
            fich = self.configuracion.ficheroPresentationPositions
            existe = Util.existeFichero(fich)
            f = codecs.open(fich, "ab", "utf-8")
            line = "%s|%s|%s|%s\n" %(self.fen, str(self.key)[:19], self.pgn_result, self.pgn)
            f.write(line)
            f.close()
            if not existe:
                self.procesador.entrenamientos.menu = None


def basico(procesador, hx, factor=1.0):
    def m(cl, t=4.0):
        n = len(cl) / 2
        li = []
        for x in range(n):
            li.append(cl[x * 2] + cl[x * 2 + 1])
        lista = []
        for x in range(n - 1):
            lista.append((li[x], li[x + 1]))
        return procesador.cpu.muevePiezaLI(lista, t * factor, padre=hx)

    li = ["b6a6a8", "b5a7c6b8", "b3d5d8", "c2c6e8",
          "f2h2h8", "g7h7h1e1", "g6f4h3g1", "g4h4h1",
          "d2f3h4f5h6g8", "e4a4a1", "e2a6c8", "g5c1", "e7d7d1", "b4f8", "b2b7", "e5f3d2b1", "e6c4f1", "f6f2"]

    n = random.randint(0, 7)
    primer = li[n]
    del li[n]

    hx = m(primer, 2.0 * factor)
    for uno in li:
        m(uno)
    return procesador.cpu.ponPosicion(procesador.posicionInicial)


def partidaDia(procesador, hx):
    dia = Util.hoy().day
    lid = Util.LIdisk("./IntFiles/31.pkl")
    dic = lid[dia - 1]
    lid.close()

    liMovs = dic["XMOVS"].split("|")

    cpu = procesador.cpu

    padre = cpu.ponPosicion(procesador.posicionInicial)
    padre = cpu.duerme(0.6, padre=padre, siExclusiva=True)

    for txt in liMovs:
        li = txt.split(",")
        tipo = li[0]

        if tipo == "m":
            desde, hasta, segundos = li[1], li[2], float(li[3])
            hx = cpu.muevePieza(desde, hasta, segundos=segundos, padre=padre)

        elif tipo == "b":
            segundos, movim = float(li[1]), li[2]
            n = cpu.duerme(segundos, padre=padre)
            cpu.borraPieza(movim, padre=n)

        elif tipo == "c":
            m1, m2 = li[1], li[2]
            cpu.cambiaPieza(m1, m2, padre=hx)

        elif tipo == "d":
            dato = float(li[1])
            padre = cpu.duerme(dato, padre=hx, siExclusiva=True)

        elif tipo == "t":
            li = dic["TOOLTIP"].split(" ")
            t = 0
            texto = ""
            for x in li:
                texto += x + " "
                t += len(x) + 1
                if t > 40:
                    texto += "<br>"
                    t = 0
            texto += '<br>Source wikipedia: http://en.wikipedia.org/wiki/List_of_chess_games'
            cpu.toolTip(texto, padre=hx)

            # hx = cpu.duerme( 3.0, padre = hx )
