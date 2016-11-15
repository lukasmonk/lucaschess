import codecs
import random

from Code import ControlPosicion
from Code import Partida
from Code import VarGen
from Code.Constantes import *

class RespuestaMotor:
    def __init__(self, nombre, siBlancas):
        self.nombre = nombre
        self.sinInicializar = True
        self.mate = 0
        self.puntos = 0
        self.sinMovimientos = False  # o por jaquemate o por ahogado
        self.pv = "a1a1"
        self.desde = ""
        self.hasta = ""
        self.coronacion = ""
        self.depth = 0
        self.time = 0
        self.nodes = 0
        self.nps = 0
        self.seldepth = 0
        self.siBlancas = siBlancas

        self.tiempo = 0

        self.maxTiempo = 0

        # def __str__( self ):
        # resp = ""
        # for x in [ "sinInicializar", "mate", "puntos", "sinMovimientos", "pv", "desde", "hasta", "coronacion", "depth"]:
        # resp += x + " : " + str( eval("self.%s"%x) ) + "  "
        # return resp

    def movimiento(self):
        return self.desde + self.hasta + self.coronacion

    def getPV(self):
        if self.pv == "a1a1" or not self.pv:
            return self.movimiento()
        return self.pv.strip()

    def cambiaColor(self, posicionAnterior=None):
        # Se usa en tutor para analizar las jugadas siguientes a la del usuario
        # Si no encuentra ninguna jugada y la jugada previa es jaque, pasa a mate en 1
        if posicionAnterior and self.sinMovimientos:
            if posicionAnterior.siJaque():
                self.mate = 1
                return
        self.puntos = -self.puntos
        if self.mate:
            self.mate = -self.mate
            if self.mate > 0:
                self.mate += 1
        self.siBlancas = not self.siBlancas

    def siMejorQue(self, otra, difpts=0, difporc=0):
        if self.mate:
            if otra.mate < 0:
                if self.mate < 0:
                    return self.mate < otra.mate
                else:
                    return True
            elif otra.mate > 0:
                if self.mate < 0:
                    return False
                else:
                    return self.mate < otra.mate
            else:
                return self.mate > 0
        elif otra.mate:
            return otra.mate < 0
        dif = self.puntos - otra.puntos
        if dif <= 0:
            return False
        adif = abs(dif)
        if difpts:
            if adif <= difpts:
                return False
        if difporc:
            porc = int(abs(self.puntos) * difporc / 100)
            if adif < porc:
                return False
        return True

    def ponBlunder(self, pts_dif):
        self.nvBlunder = pts_dif

    def ponBrilliancie(self, pts_dif):
        self.nvBrillante = pts_dif

    def nivelBrillante(self):
        return getattr(self, "nvBrillante", 0)

    def nivelBlunder(self):
        return getattr(self, "nvBlunder", 0)

    def puntosABS(self):
        if self.mate:
            if self.mate < 0:
                puntos = -10000 - (self.mate + 1) * 10
            else:
                puntos = +10000 - (self.mate - 1) * 10
        else:
            puntos = self.puntos

        return puntos

    def puntosABS_5(self):
        if self.mate:
            if self.mate < 0:
                puntos = -10000 - (self.mate + 1) * 100
            else:
                puntos = +10000 - (self.mate - 1) * 100
        else:
            puntos = self.puntos

        return puntos

    def texto(self):
        if self.mate:
            if -1 <= self.mate <= 1:
                d = {True: _("White is in checkmate"), False: _("Black is in checkmate")}
                if self.mate == -1:
                    t = self.siBlancas
                else:
                    t = not self.siBlancas
                return d[t]

            else:
                d = {True: _("White mates in %1"), False: _("Black mates in %1")}
                if self.mate > 0:
                    t = self.siBlancas
                else:
                    t = not self.siBlancas
                return _X(d[t], str(abs(self.mate)))

        else:
            pt = self.puntos
            if not self.siBlancas:
                pt = -pt
            if VarGen.configuracion.centipawns:
                cp = "%+d" % pt
            else:
                cp = "%+0.2f" % float(pt / 100.0)
            return "%s %s" % (cp, _("points"))

    def abrTexto(self):
        c = self.abrTextoBase()
        if self.mate == 0:
            c = "%s %s" % (c, _("pts"))
        return c

    def abrTextoPDT(self):
        c = self.abrTextoBase()
        if self.depth:
            c += "/%d" % self.depth
        if self.time:
            c += "/%0.02f\"" % (1.0 * self.time / 1000.0,)
        return c

    def abrTextoBase(self):
        if self.mate != 0:
            mt = self.mate
            if mt == 1:
                return ""
            if not self.siBlancas:
                mt = -mt
            return "M%+d" % mt
        else:
            pts = self.puntos
            if not self.siBlancas:
                pts = -pts
            if VarGen.configuracion.centipawns:
                return "%d" % pts
            else:
                return "%+0.2f" % float(pts / 100.0)

    def base2texto(self):
        txt = ""
        for clave, texto in (("NOMBRE", self.nombre),
                             ("SININICIALIZAR", "S" if self.sinInicializar else "N"),
                             ("MATE", str(self.mate)),
                             ("PUNTOS", str(self.puntos)),
                             ("SINMOVIMIENTOS", "S" if self.sinInicializar else "N"),
                             ("PV", self.pv),
                             ("DESDE", self.desde),
                             ("HASTA", self.hasta),
                             ("CORONACION", self.coronacion),
                             ("DEPTH", str(self.depth)),
                             ("NODES", str(self.nodes)),
                             ("NPS", str(self.nps)),
                             ("SELDEPTH", str(self.seldepth)),
                             ):
            txt += "%s=%s\n" % (clave, texto)
        return txt

    def texto2base(self, texto):

        for linea in texto.split("\n"):
            pos = linea.find("=")
            if pos > 0:
                clave = linea[:pos]
                eti = linea[pos + 1:].strip()
                if clave == "NOMBRE":
                    self.nombre = eti
                elif clave == "SININICIALIZAR":
                    self.sinInicializar = eti == "S"
                elif clave == "MATE":
                    self.mate = int(eti)
                elif clave == "PUNTOS":
                    self.puntos = int(eti)
                elif clave == "SINMOVIMIENTOS":
                    self.sinMovimientos = eti == "S"
                elif clave == "PV":
                    self.pv = eti
                elif clave == "DESDE":
                    self.desde = eti
                elif clave == "HASTA":
                    self.hasta = eti
                elif clave == "CORONACION":
                    self.coronacion = eti
                elif clave == "DEPTH":
                    self.depth = int(eti)
                elif clave == "NODES":
                    self.nodes = int(eti)
                elif clave == "NPS":
                    self.nps = int(eti)
                elif clave == "SELDEPTH":
                    self.seldepth = int(eti)

class MRespuestaMotor:
    def __init__(self, nombre, siBlancas):
        self.nombre = nombre
        self.tiempo = 0
        self.siBlancas = siBlancas

        self.depth = 0

        self.maxTiempo = 0
        self.maxProfundidad = 0

        self.dicMultiPV = {}
        self.liMultiPV = []

        self.saveLines = False
        self.lines = []

    def reset(self):
        self.tiempo = 0

        self.depth = 0

        self.maxTiempo = 0
        self.maxProfundidad = 0

        self.dicMultiPV = {}
        self.liMultiPV = []

        self.saveLines = False
        self.lines = []

    def save_lines(self):
        self.saveLines = True
        self.lines = []

    def setTimeDepth(self, maxTiempo, maxProfundidad):
        self.maxTiempo = maxTiempo
        self.maxProfundidad = maxProfundidad

    def dispatch(self, linea):
        if linea.startswith("info ") and " pv " in linea:
            self.miraPV(linea[5:])

        elif linea.startswith("bestmove"):
            self.miraBestMove(linea)

        elif linea.startswith("info ") and " score " in linea:
            self.miraScore(linea[5:])

        if self.saveLines:
            self.lines.append(linea)

    def dispatchPV(self, pv):
        self.dispatch("info depth 1 score cp 0 time 1 pv %s" % pv)
        self.dispatch("bestmove %s" % pv)
        self.ordena()

    def ordena(self):
        li = []
        setYa = set()
        for k, rm in self.dicMultiPV.iteritems():

            mov = rm.movimiento()
            if mov in setYa:
                continue
            setYa.add(mov)
            li.append(rm)
        self.liMultiPV = sorted(li, key=lambda rm: -rm.puntosABS())  # de mayor a menor

        # elpeor = True
        # for n, rm1 in enumerate(self.liMultiPV):
        # if rm.siMejorQue( rm1, 0, 0 ):
        # self.liMultiPV.insert( n, rm )
        # elpeor = False
        # break
        # if elpeor:

    def __len__(self):
        return len(self.liMultiPV)

    def miraPV(self, pvBase):
        dClaves = self.miraClaves(pvBase, ("multipv", "depth", "seldepth", "score", "time",
                                           "nodes", "pv", "hashfull", "tbhits", "nps", "currmove", "currmovenumber"))

        if "pv" in dClaves:
            pv = dClaves["pv"].strip()
            if not pv:
                return
        else:
            return

        if "nodes" in dClaves:  # Toga en multipv, envia 0 si no tiene nada que contar
            if (dClaves["nodes"] == "0") and ("mate" not in pvBase):
                return

        if "score" in dClaves:
            if "mate 0 " in pvBase:
                return

        if "multipv" in dClaves:
            kMulti = dClaves["multipv"]
            if kMulti not in self.dicMultiPV:
                self.dicMultiPV[kMulti] = RespuestaMotor(self.nombre, self.siBlancas)
        else:
            if len(self.dicMultiPV) == 0:
                kMulti = "1"
                self.dicMultiPV[kMulti] = RespuestaMotor(self.nombre, self.siBlancas)
            else:
                kMulti = self.dicMultiPV.keys()[0]

        rm = self.dicMultiPV[kMulti]
        rm.sinInicializar = False

        if "depth" in dClaves:
            depth = int(dClaves["depth"].strip())
            if self.maxProfundidad:
                if rm.desde:  # Es decir que ya tenemos datos (rm.pv al principio = a1a1
                    if (depth > self.maxProfundidad) and (depth > rm.depth):
                        return
            rm.depth = depth

        if "time" in dClaves:
            rm.time = int(dClaves["time"].strip())

        if "nodes" in dClaves:
            rm.nodes = int(dClaves["nodes"].strip())

        if "nps" in dClaves:
            nps = dClaves["nps"].strip()
            if " " in nps:
                nps = nps.split(" ")[0]
            if nps.isdigit():
                rm.nps = int(nps)

        if "seldepth" in dClaves:
            rm.seldepth = int(dClaves["seldepth"].strip())

        if "score" in dClaves:
            score = dClaves["score"].strip()
            if score.startswith("cp "):
                rm.puntos = int(score.split(" ")[1])
                rm.mate = 0
                rm.sinMovimientos = False
            elif score.startswith("mate "):
                rm.puntos = 0
                rm.mate = int(score.split(" ")[1])
                rm.sinMovimientos = False

        if "pv" in dClaves:
            pv = dClaves["pv"].strip()
            x = pv.find(" ")
            pv1 = pv[:x] if x >= 0 else pv
            rm.pv = pv
            rm.desde = pv1[:2]
            rm.hasta = pv1[2:4]
            rm.coronacion = pv1[4].lower() if len(pv1) == 5 else ""

    def miraScore(self, pvBase):
        dClaves = self.miraClaves(pvBase, ("multipv", "depth", "seldepth", "score", "time",
                                           "nodes", "pv", "hashfull", "tbhits", "nps", "currmove", "currmovenumber"))

        if "multipv" in dClaves:
            kMulti = dClaves["multipv"]
            if kMulti not in self.dicMultiPV:
                self.dicMultiPV[kMulti] = RespuestaMotor(self.nombre, self.siBlancas)
        else:
            if len(self.dicMultiPV) == 0:
                kMulti = "1"
                self.dicMultiPV[kMulti] = RespuestaMotor(self.nombre, self.siBlancas)
            else:
                kMulti = self.dicMultiPV.keys()[0]

        rm = self.dicMultiPV[kMulti]
        rm.sinInicializar = False

        if "depth" in dClaves:
            depth = dClaves["depth"].strip()
            if depth.isdigit():
                depth = int(depth)
                if self.maxProfundidad:
                    if rm.desde:  # Es decir que ya tenemos datos (rm.pv al principio = a1a1
                        if (depth > self.maxProfundidad) and (depth > rm.depth):
                            return
                rm.depth = depth

        if "time" in dClaves:
            tm = dClaves["time"].strip()
            if tm.isdigit():
                rm.time = int(tm)

        score = dClaves["score"].strip()
        if score.startswith("cp "):
            rm.puntos = int(score.split(" ")[1])
            rm.mate = 0
            rm.sinMovimientos = False
        elif score.startswith("mate "):
            rm.puntos = 0
            rm.mate = int(score.split(" ")[1])
            if not rm.mate:  # stockfish mate 0
                rm.mate = -1

    def agregaRM(self, rm):  # Para los analisis MultiPV donde no han considerado una jugada
        n = 1
        while True:
            if str(n) not in self.dicMultiPV:
                self.dicMultiPV[str(n)] = rm
                break
            n += 1
        self.ordena()
        for pos, rm1 in enumerate(self.liMultiPV):
            if rm1.movimiento() == rm.movimiento():
                return pos

    def miraBestMove(self, bestmove):
        if len(self.dicMultiPV) > 1:
            return

        rm = self.dicMultiPV.get("1", RespuestaMotor(self.nombre, self.siBlancas))

        # rm = RespuestaMotor( self.nombre, self.siBlancas )
        self.dicMultiPV = {"1": rm}

        rm.sinInicializar = False
        dClaves = self.miraClaves(bestmove, ("bestmove", "ponder"))
        rm.desde = ""
        rm.hasta = ""
        rm.coronacion = ""
        rm.sinMovimientos = False
        if "bestmove" in dClaves:
            bestmove = dClaves["bestmove"].strip()
            rm.sinMovimientos = True
            if bestmove:
                if len(bestmove) >= 4:
                    def bien(a8):
                        return a8[0] in "abcdefgh" and a8[1] in "12345678"

                    d = bestmove[:2]
                    h = bestmove[2:4]

                    if bien(d) and bien(h):
                        rm.sinMovimientos = d == h
                        rm.desde = d
                        rm.hasta = h
                        if len(bestmove) == 5 and bestmove[4].lower() in "qbrn":
                            rm.coronacion = bestmove[4].lower()
                        if rm.pv == "a1a1":  # No muestra pvs solo directamente
                            rm.pv = bestmove

        else:
            rm.sinMovimientos = True

    def miraClaves(self, mensaje, liClaves):
        dClaves = {}
        clave = ""
        dato = ""
        for palabra in mensaje.split(" "):
            if palabra in liClaves:
                if clave:
                    dClaves[clave] = dato.strip()
                clave = palabra
                dato = ""
            else:
                dato += " " + palabra
        if clave:
            dClaves[clave] = dato.strip()
        return dClaves

        # def rmMultiPV( self, movim ):
        # movim = movim.lower()
        # for k, rm in enumerate(self.liMultiPV):
        # if rm.movimiento() == movim:
        # return rm, k
        # return None, None

    def mejorRMQue(self, rm, difpts, difporc):
        if self.liMultiPV:
            return self.liMultiPV[0].siMejorQue(rm, difpts, difporc)
        else:
            return False

    def buscaRM(self, movimiento):
        movimiento = movimiento.lower()
        for n, rm in enumerate(self.liMultiPV):
            if rm.movimiento() == movimiento:
                return rm, n
        return None, -1

    def mejorMovQue(self, movimiento):
        if self.liMultiPV:
            rm, n = self.buscaRM(movimiento)
            if rm is None:
                return True
            if n == 0:
                return False
            return self.liMultiPV[0].siMejorQue(rm)
        return False

    def numMejorMovQue(self, movimiento):
        rm, n = self.buscaRM(movimiento)
        num = len(self.liMultiPV)
        if rm is None:
            return num
        x = 0
        for rm1 in self.liMultiPV:
            if rm1.siMejorQue(rm):
                x += 1
        return x

    def rmBest(self):
        num = len(self.liMultiPV)
        if num == 0:
            return None
        rm = self.liMultiPV[0]
        for x in range(1, num):
            rm1 = self.liMultiPV[x]
            if rm1.siMejorQue(rm):
                rm = rm1
        return rm

    def difPointsBest(self, movimiento):
        rmbest = self.rmBest()
        if not rmbest:
            return 0, 0
        pbest = rmbest.puntosABS_5()
        rm, n = self.buscaRM(movimiento)
        if rm is None:
            return pbest, 0
        return pbest, rm.puntosABS_5()

    def mejorMov(self):
        if self.liMultiPV:
            return self.liMultiPV[0]
        return RespuestaMotor(self.nombre, self.siBlancas)

    def bestmoves(self):
        li = []
        if not self.liMultiPV:
            return li
        n = len(self.liMultiPV)
        rm0 = self.liMultiPV[0]
        li.append(rm0)
        if n > 1:
            for n in range(1,n):
                rm = self.liMultiPV[n]
                if rm0.puntosABS() == rm.puntosABS():
                    li.append(rm)
                else:
                    break
        return li

    def mejorMovDetectaBlunders(self, fdbg, mindifpuntos, maxmate):
        rm0 = self.liMultiPV[0]
        if maxmate:
            if 0 < rm0.mate <= maxmate:
                if fdbg:
                    fdbg.write("1. %s : %s %d <= %d\n" % (rm0.pv, _("Mate"), rm0.mate, maxmate))
                return True
        if mindifpuntos:
            partidaBase = self.partida  # asignada por el gestorMotor
            numJugadas = partidaBase.numJugadas()
            puntosPrevios = 0
            if numJugadas >= 3:
                jg = partidaBase.jugada(numJugadas - 3)
                if hasattr(jg, "puntosABS_3"):  # se graban en mejormovajustado
                    puntosPrevios = jg.puntosABS_3
            difpuntos = rm0.puntosABS() - puntosPrevios  # son puntos ganados por el motor y perdidos por el jugador
            if difpuntos > mindifpuntos:
                if fdbg:
                    fdbg.write("1. %s : %s %d > %d\n" % (rm0.pv, _("Points lost"), difpuntos, mindifpuntos))
                return True
        return False

    def ajustaPersonalidad(self, una):
        def x(clave):
            return una.get(clave, 0)

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.fenBase)

        dbg = x("DEBUG")
        if dbg:
            fdbg = codecs.open(dbg, "a", 'utf-8', 'ignore')
            fdbg.write("\n%s\n" % cp.tablero())
            dpr = _("In the expected moves")
        else:
            fdbg = None

        # Aterrizaje
        aterrizaje = una.get("ATERRIZAJE", 50)

        # Blunders
        mindifpuntos = x("MINDIFPUNTOS")
        maxmate = x("MAXMATE")
        if self.mejorMovDetectaBlunders(fdbg, mindifpuntos, maxmate):
            if fdbg:
                fdbg.close()
            return kAjustarMejor, mindifpuntos, maxmate, dbg, aterrizaje

        # Comprobamos donde estamos, si medio o final
        tipo = "F" if cp.totalPiezas() <= x("MAXPIEZASFINAL") else ""

        # Variable a analizar
        xMpn = x("MOVERPEON" + tipo)
        xApz = x("AVANZARPIEZA" + tipo)
        xJ = x("JAQUE" + tipo)
        xC = x("CAPTURAR" + tipo)

        x2B = x("2BPR" + tipo)
        xAvPR = x("AVANZARPR" + tipo)
        xJPR = x("JAQUEPR" + tipo)
        xCPR = x("CAPTURARPR" + tipo)

        # Miramos todas las propuestas
        for num, rm in enumerate(self.liMultiPV):
            partida = Partida.Partida(cp).leerPV(rm.pv)
            jg0 = partida.jugada(0)
            ps0 = partida.iniPosicion
            psZ = partida.ultPosicion

            if dbg:
                pgn = ""
                si = False
                for jg in partida.liJugadas:
                    pgn += jg.pgnEN() + " "
                    if si:
                        pgn += "- "
                    si = not si
                pgn = pgn.strip("- ")

                if rm.mate:
                    fdbg.write("%2d; %s ; %d %s\n" % (num + 1, pgn, rm.mate, _("Mate")))
                else:
                    # fdbg.write( "%2d. %s : %d\n"%(num+1,pgn,rm.puntos) )
                    fdbg.write("%2d; %s; %d\n" % (num + 1, pgn, rm.puntos))

            if rm.mate:
                continue

            if xMpn:
                if ps0.casillas[jg0.desde].lower() == "p":
                    rm.puntos += xMpn
                    if dbg:
                        fdbg.write("    %s : %d -> %d\n" % (_("To move a pawn"), xMpn, rm.puntos))

            if xApz:
                if ps0.casillas[jg0.desde].lower() != "p":
                    if tipo == "F":
                        dif = ps0.distanciaPiezaKenemigo(jg0.desde) - ps0.distanciaPiezaKenemigo(jg0.hasta)
                    else:
                        nd = int(jg0.desde[1])
                        nh = int(jg0.hasta[1])
                        dif = nh - nd
                        if not ps0.siBlancas:
                            dif = -dif
                    if dif > 0:
                        rm.puntos += xApz
                        if dbg:
                            fdbg.write("    %s : %d -> %d\n" % (_("Advance piece"), xApz, rm.puntos))

            if xJ:
                if jg0.siJaque:
                    rm.puntos += xJ
                    if dbg:
                        fdbg.write("    %s : %d -> %d\n" % (_("Make check"), xJ, rm.puntos))

            if xC:
                if jg0.siCaptura():
                    rm.puntos += xC
                    if dbg:
                        fdbg.write("    %s : %d -> %d\n" % (_("Capture"), xJ, rm.puntos))

            if x2B:
                if psZ.numPiezas("B") == 2:
                    rm.puntos += x2B
                    if dbg:
                        fdbg.write("    %s : %d -> %d \n" % (_("Keep the two bishops"), x2B, rm.puntos))

            if xAvPR:
                pZ = psZ.pesoWB()
                p0 = ps0.pesoWB()
                valorWB = pZ - p0
                if not cp.siBlancas:
                    valorWB = -valorWB
                if valorWB > 0:
                    rm.puntos += xAvPR
                    if dbg:
                        if not cp.siBlancas:
                            p0 = -p0
                            pZ = -pZ
                        fdbg.write(
                                "    %s (%s) : %d -> %d [%d => %d]\n" % (_("Advance"), dpr, xAvPR, rm.puntos, p0, pZ))

            if xJPR:
                n = True
                for jg in partida.liJugadas:
                    if n and jg.siJaque:
                        rm.puntos += xJPR
                        if dbg:
                            fdbg.write("    %s (%s) : %d -> %d\n" % (_("Make check"), dpr, xJPR, rm.puntos))
                        break
                    n = not n

            if xCPR:
                n = True
                for jg in partida.liJugadas:
                    if n and jg.siCaptura():
                        rm.puntos += xCPR
                        if dbg:
                            fdbg.write("    %s (%s) : %d -> %d\n" % (_("Capture"), dpr, xCPR, rm.puntos))
                        break
                    n = not n

        # Ordenamos
        li = []
        for rm in self.liMultiPV:
            elpeor = True
            for n, rm1 in enumerate(li):
                if rm.siMejorQue(rm1, 0, 0):
                    li.insert(n, rm)
                    elpeor = False
                    break
            if elpeor:
                li.append(rm)
        self.liMultiPV = li

        if dbg:
            fdbg.write("Result :\n")
            for num, rm in enumerate(self.liMultiPV):
                partida = Partida.Partida(cp).leerPV(rm.pv)

                pgn = ""
                si = False
                for jg in partida.liJugadas:
                    pgn += jg.pgnEN() + " "
                    if si:
                        pgn += "- "
                    si = not si
                pgn = pgn.strip("- ")

                if rm.mate:
                    fdbg.write("%d. %s : %d %s\n" % (num + 1, pgn, rm.mate, _("Mate")))
                else:
                    fdbg.write("%d. %s : %d\n" % (num + 1, pgn, rm.puntos))

            fdbg.write("\n")
            fdbg.close()

        return una.get("AJUSTARFINAL" if tipo == "F" else "AJUSTAR",
                       kAjustarMejor), mindifpuntos, maxmate, dbg, aterrizaje

    def mejorMovAjustadoNivel(self, nTipo):
        if nTipo == kAjustarNivelAlto:
            dic = {kAjustarMejor: 60, kAjustarSuperiorMM: 30, kAjustarSuperiorM: 15, kAjustarSuperior: 10,
                   kAjustarSimilar: 5}
        elif nTipo == kAjustarNivelMedio:
            dic = {kAjustarSuperiorMM: 5, kAjustarSuperiorM: 10, kAjustarSuperior: 25, kAjustarSimilar: 60,
                   kAjustarInferior: 25, kAjustarInferiorM: 10, kAjustarInferiorMM: 5}
        elif nTipo == kAjustarNivelBajo:
            dic = {kAjustarSimilar: 25, kAjustarInferior: 60, kAjustarInferiorM: 25, kAjustarInferiorMM: 10}
        tp = 0
        for k, v in dic.iteritems():
            tp += v
        sel = random.randint(1, tp)
        t = 0
        for k, v in dic.iteritems():
            t += v
            if sel <= t:
                return k

    def mejorMovAjustadoSuperior(self, nivel, mindifpuntos, maxmate, aterrizaje):

        if self.mejorMovDetectaBlunders(None, mindifpuntos, maxmate):
            return self.liMultiPV[0]

        # Buscamos una jugada positiva que no sea de mate
        # Si no la hay, cogemos el ultimo
        rmIni = None
        for rm in self.liMultiPV:
            if rm.mate == 0:
                rmIni = rm
                break
        if rmIni is None:
            return self.liMultiPV[-1]  # Mandamos el mate peor

        ptsIni = rmIni.puntosABS()
        if ptsIni > aterrizaje:
            minimo = ptsIni - aterrizaje
        else:
            minimo = 0

        li = []
        for rm in self.liMultiPV:
            pts = rm.puntosABS()
            if pts < minimo:
                break
            li.append(rm)

        if not li:
            return self.liMultiPV[0]
        nLi = len(li)

        return li[0] if nLi < nivel else li[-nivel]

    def mejorMovAjustadoInferior(self, nivel, mindifpuntos, maxmate, aterrizaje):

        if self.mejorMovDetectaBlunders(None, mindifpuntos, maxmate):
            return self.liMultiPV[0]

        # Buscamos una jugada positiva que no sea de mate
        # Si no la hay, cogemos el ultimo
        rmIni = None
        for rm in self.liMultiPV:
            if rm.mate == 0:
                rmIni = rm
                break
        if rmIni is None:
            return self.liMultiPV[-1]  # Mandamos el mate peor

        ptsIni = rmIni.puntosABS()
        if ptsIni > aterrizaje:  # el motor hace una jugada bastante peor, pero no malisima
            minimo = ptsIni - aterrizaje
        else:
            minimo = 0

        li = []
        for rmSel in self.liMultiPV:
            pts = rmSel.puntosABS()
            if pts < minimo:
                li.append(rmSel)
        if not li:
            return self.liMultiPV[-1]  # Mandamos la jugada peor
        nLi = len(li)

        return li[-1] if nLi < nivel else li[nivel - 1]

    def mejorMovAjustadoSimilar(self, mindifpuntos, maxmate, aterrizaje):

        if self.mejorMovDetectaBlunders(None, mindifpuntos, maxmate):
            return self.liMultiPV[0]

        # Buscamos una jugada positiva que no sea de mate
        # Si no la hay, cogemos el ultimo
        rmIni = None
        for rm in self.liMultiPV:
            if rm.mate == 0:
                rmIni = rm
                break
        if rmIni is None:
            return self.liMultiPV[-1]  # Mandamos el mate peor

        ptsIni = rmIni.puntosABS()
        if ptsIni > aterrizaje:  # el motor hace una jugada peor, pero no malisima
            minimo = ptsIni - aterrizaje
        else:
            minimo = 0

        rmAnt = self.liMultiPV[0]
        rmSel = rmAnt
        ptsAnt = rmAnt.puntosABS()
        for rm in self.liMultiPV:
            pts = rm.puntosABS()
            if pts < minimo:
                rmSel = rm if abs(ptsAnt) > abs(pts) else rmAnt
                break
            rmAnt = rm
            ptsAnt = pts
        return rmSel

    def mejorMovAjustado(self, nTipo):
        if self.liMultiPV:

            rmSel = None
            aterrizaje = 50
            siPersonalidad = nTipo >= 1000  # Necesario para grabar los puntos

            if siPersonalidad:
                nTipo, mindifpuntos, maxmate, dbg, aterrizaje = self.ajustaPersonalidad(
                        self.liPersonalidades[nTipo - 1000])

            if nTipo == kAjustarMejor:
                rmSel = self.liMultiPV[0]
            elif nTipo == kAjustarPeor:
                rmSel = self.liMultiPV[-1]
            elif nTipo in (kAjustarNivelAlto, kAjustarNivelBajo, kAjustarNivelMedio):
                nTipo = self.mejorMovAjustadoNivel(nTipo)  # Se corta el if para que se calcule el nTipo

            if nTipo in (kAjustarSuperior, kAjustarSuperiorM, kAjustarSuperiorMM):
                nivel = {kAjustarSuperior: 1, kAjustarSuperiorM: 2, kAjustarSuperiorMM: 3}
                if not siPersonalidad:
                    mindifpuntos, maxmate = 200, 2
                rmSel = self.mejorMovAjustadoSuperior(nivel[nTipo], mindifpuntos, maxmate, aterrizaje)

            elif nTipo == kAjustarSimilar:
                if not siPersonalidad:
                    mindifpuntos, maxmate = 300, 1
                rmSel = self.mejorMovAjustadoSimilar(mindifpuntos, maxmate, aterrizaje)

            elif nTipo in (kAjustarInferior, kAjustarInferiorM, kAjustarInferiorMM):
                nivel = {kAjustarInferior: 1, kAjustarInferiorM: 2, kAjustarInferiorMM: 3}
                if not siPersonalidad:
                    mindifpuntos, maxmate = 400, 1
                rmSel = self.mejorMovAjustadoInferior(nivel[nTipo], mindifpuntos, maxmate, aterrizaje)

            if rmSel is None:
                rmSel = self.liMultiPV[0]

            # Para comprobar perdida de puntos
            if self.partida.numJugadas():
                self.partida.last_jg().puntosABS_3 = rmSel.puntosABS()

            return rmSel
        return RespuestaMotor(self.nombre, self.siBlancas)

    def miraBrilliancies(self, mindepth, minpuntos):
        # Que hay un minimo de opciones
        if len(self.liMultiPV) < 5:
            return

        # Se hace despues de dispatch, ya sabemos el bestmove
        # determinamos si el bestmove es una brillancie
        rmbr = self.liMultiPV[0]
        ptsbr = rmbr.puntosABS()

        # 1. Que la situacion en que se queda sea positiva
        if ptsbr < minpuntos / 2:
            return

            # 2. Hay una diferencia considerable de puntos con el segundo clasificado
            # dif = ptsbr-self.liMultiPV[1].puntosABS()
            # if dif < (minpuntos/2):
            # return

        # 3. Leemos el texto enviado por el engine
        busca = "pv " + rmbr.movimiento() + " "
        listaClaves = ("multipv", "depth", "seldepth", "score", "time", "nodes", "pv", "hashfull", "tbhits", "nps")
        for linea in self.lines:
            if busca in linea:
                dClaves = self.miraClaves(linea, listaClaves)

                if not ("depth" in dClaves):
                    continue
                depth = int(dClaves["depth"].strip())

                if not ("score" in dClaves):
                    continue

                rm = RespuestaMotor("", self.siBlancas)
                score = dClaves["score"].strip()
                if score.startswith("cp "):
                    rm.puntos = int(score.split(" ")[1])
                    rm.mate = 0
                elif score.startswith("mate "):
                    rm.puntos = 0
                    rm.mate = int(score.split(" ")[1])
                dif = ptsbr - rm.puntosABS()

                if depth >= mindepth:  # es un brilliancie
                    rmbr.ponBrilliancie(depth)
                    return

                elif dif < minpuntos:  # primeras depths ya se sabia que era buena jugada
                    return
