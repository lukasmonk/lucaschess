import os
import operator

import Code.VarGen as VarGen
import Code.Util as Util
import Code.Jugada as Jugada
from Code.Movimientos import xpv2pv, pv2xpv

class GMpartida:
    def __init__(self, linea):
        xpv, self.site, self.oponent, self.date, self.opening, self.result, self.color = linea.split("|")
        self.liPV = xpv2pv(xpv).split(" ")
        self.lenPV = len(self.liPV)

    def isWhite(self, siWhite):
        if siWhite:
            return "W" in self.color
        else:
            return "B" in self.color

    def isValidMove(self, ply, move):
        if ply < self.lenPV:
            return self.liPV[ply] == move
        else:
            return False

    def isFinished(self, ply):
        return ply >= self.lenPV

    def move(self, ply):
        return None if self.isFinished(ply) else self.liPV[ply]

    def rotulo(self, siGM=True):
        if siGM:
            return _("Opponent") + ": <b>%s (%s)</b>" % ( self.oponent, self.date )
        else:
            return "%s (%s)" % ( self.oponent, self.date )

    def rotuloBasico(self, siGM=True):
        if siGM:
            return _("Opponent") + ": %s (%s)" % ( self.oponent, self.date )
        else:
            return "%s (%s)" % ( self.oponent, self.date )

class GM:
    def __init__(self, carpeta, gm):
        self.gm = gm
        self.carpeta = carpeta

        self.dicAciertos = {}

        self.liGMPartidas = self.read()

        self.ply = 0

    def __len__(self):
        return len(self.liGMPartidas)

    def read(self):
        ficheroGM = self.gm + ".xgm"
        f = open(os.path.join(self.carpeta, ficheroGM), "rb")
        li = []
        for linea in f:
            linea = linea.strip()
            if linea:
                li.append(GMpartida(linea))
        f.close()
        return li

    def colorFilter(self, isWhite):
        self.liGMPartidas = [gmp for gmp in self.liGMPartidas if gmp.isWhite(isWhite)]

    def play(self, move):
        move = move.lower()

        liP = []
        ok = False
        nextPly = self.ply + 1
        for gmPartida in self.liGMPartidas:
            if gmPartida.isValidMove(self.ply, move):
                self.lastGame = gmPartida  # - Siempre hay una ultima
                ok = True
                if not gmPartida.isFinished(nextPly):
                    liP.append(gmPartida)

        self.liGMPartidas = liP
        self.ply += 1
        return ok

    def isValidMove(self, move):
        move = move.lower()

        for gmPartida in self.liGMPartidas:
            if gmPartida.isValidMove(self.ply, move):
                return True
        return False

    def isFinished(self):
        for gmp in self.liGMPartidas:
            if not gmp.isFinished(self.ply):
                return False
        return True

    def alternativas(self):
        li = []
        for gmPartida in self.liGMPartidas:
            move = gmPartida.move(self.ply)
            if move and move not in li:
                li.append(move)
        return li

    def dameJugadasTXT(self, posicionBase, siGM):
        li = []
        dRepeticiones = {}
        for gmPartida in self.liGMPartidas:
            move = gmPartida.move(self.ply)
            if move:
                if move not in dRepeticiones:
                    dRepeticiones[move] = [len(li), 1]
                    desde, hasta, coronacion = move[:2], move[2:4], move[4:]
                    siBien, mens, jg = Jugada.dameJugada(posicionBase, desde, hasta, coronacion)
                    li.append([desde, hasta, coronacion, gmPartida.rotuloBasico(siGM), jg.pgnSP()])
                else:
                    dRepeticiones[move][1] += 1
                    pos = dRepeticiones[move][0]
                    li[pos][3] = _("%d games") % dRepeticiones[move][1]
        return li

    def rotuloPartidaSiUnica(self, siGM=True):
        if len(self.liGMPartidas) == 1:
            return self.liGMPartidas[0].rotulo(siGM)
        else:
            return ""

    def resultado(self, partida):
        gPartida = self.lastGame
        apertura = partida.apertura.trNombre if partida.apertura else gPartida.opening

        txt = _("Opponent") + " : <b>" + gPartida.oponent + "</b><br>"
        site = gPartida.site
        if site:
            txt += _("Site") + " : <b>" + site + "</b><br>"
        txt += _("Date") + " : <b>" + gPartida.date + "</b><br>"
        txt += _("Opening") + " : <b>" + apertura + "</b><br>"
        txt += _("Result") + " : <b>" + gPartida.result + "</b><br>"
        txt += "<br>" * 2
        aciertos = 0
        for v in self.dicAciertos.itervalues():
            if v:
                aciertos += 1
        total = len(self.dicAciertos)
        if total:
            porc = int(aciertos * 100.0 / total)
            txt += _("Hints") + " : <b>%d%%</b>" % porc
        else:
            porc = 0

        site = " - %s" % site if site else ""
        txtResumen = "%s%s - %s - %s" % (gPartida.oponent, site, gPartida.date, gPartida.result)

        return txt, porc, txtResumen

    def ponPartidaElegida(self, numPartida):
        self.liGMPartidas = [self.liGMPartidas[numPartida]]

    def genToSelect(self):
        liRegs = []
        for num, part in enumerate(self.liGMPartidas):
            dic = dict(NOMBRE=part.oponent, FECHA=part.date, ECO=part.opening, RESULT=part.result, NUMERO=num)
            liRegs.append(dic)
        return liRegs

def dicGM():
    dic = {}
    f = open("gm/listaGM.txt", "rb")
    for linea in f:
        if linea:
            li = linea.split(VarGen.XSEP)
            gm = li[0].lower()
            nombre = li[1]
            dic[gm] = nombre
    f.close()

    return dic

def listaGM():
    dic = dicGM()
    li = []
    for fich in Util.listdir("GM"):
        fich = fich.lower()
        if fich.endswith(".xgm"):
            gm = fich[:-4].lower()
            try:
                li.append((dic[gm], gm, True, True ))
            except:
                pass
    li = sorted(li, key=operator.itemgetter(0))
    return li

def listaGMpersonal(carpeta):
    li = []
    for fich in Util.listdir(carpeta):
        fich = fich.lower()
        if fich.endswith(".xgm"):
            gm = fich[:-4]

            siW = siB = False
            with open(os.path.join(carpeta, fich)) as f:
                for linea in f:
                    gmp = GMpartida(linea.strip())
                    if not siW:
                        siW = gmp.isWhite(True)
                    if not siB:
                        siB = gmp.isWhite(False)
                    if siW and siB:
                        break
            if siW or siB:
                li.append((gm, gm, siW, siB))
    li = sorted(li)
    return li

def hayGMpersonal(carpeta):
    return len(listaGMpersonal(carpeta)) > 0

class FabGM:
    def __init__(self, configuracion, nomEntrenamiento, liJugadores):
        self.configuracion = configuracion
        self.nomEntrenamiento = nomEntrenamiento
        self.liJugadores = liJugadores

        self.f = None

        self.added = 0

    def write(self, txt):
        if self.f is None:
            fichero = os.path.join(self.configuracion.dirPersonalTraining, self.nomEntrenamiento) + ".xgm"
            self.f = open(fichero, "wb")
        self.f.write(txt)
        self.added += 1

    def close(self):
        if self.f:
            self.f.close()
            self.f = None

    def masMadera(self, pgn, partida, result):
        dic = pgn.dic
        if not ( "White" in dic) or not ( "Black" in dic ) or (result and not ( "Result" in dic )):
            return

        if self.liJugadores:
            xblancas = False
            xnegras = False

            for x in ["Black", "White"]:
                if x in dic:
                    jugador = dic[x].upper()
                    si = False
                    for uno in self.liJugadores:
                        siContrario = uno.startswith("^")
                        if siContrario:
                            uno = uno[1:]

                        siZ = uno.endswith("*")
                        siA = uno.startswith("*")
                        uno = uno.replace("*", "").strip().upper()
                        if siA:
                            if jugador.endswith(uno):
                                si = True
                            if siZ:  # form apara poner siA y siZ
                                si = uno in jugador
                        elif siZ:
                            if jugador.startswith(uno):
                                si = True
                        elif uno == jugador:
                            si = True
                        if si:
                            break
                    if si:
                        if x == "Black":
                            if siContrario:
                                xblancas = True
                            else:
                                xnegras = True
                        else:
                            if siContrario:
                                xnegras = True
                            else:
                                xblancas = True

            if not ( xblancas or xnegras ):
                return

            self.masMaderaUno(dic, partida, xblancas, xnegras, result)
        else:
            self.masMaderaUno(dic, partida, True, False, result)
            self.masMaderaUno(dic, partida, False, True, result)

    def masMaderaUno(self, dic, partida, xblancas, xnegras, tpResult):
        pk = ""
        for jg in partida.liJugadas:
            pk += jg.movimiento() + " "

        event = dic.get("Event", "-")
        oponente = dic["White"] + "-" + dic["Black"]
        date = dic.get("Date", "-").replace("?", "").strip(".")
        eco = dic.get("Eco", "-")
        result = dic.get("Result", "-")
        color = "W" if xblancas else "B"
        if tpResult:
            siEmpate = "2" in result
            siGanaBlancas = not siEmpate and result.startswith("1")
            siGanaNegras = not siEmpate and result.startswith("0")
            if tpResult == 1:
                if not ((xblancas and siGanaBlancas) or (xnegras and siGanaNegras) ):
                    return
            else:
                if not ( siEmpate or (xblancas and siGanaBlancas) or (xnegras and siGanaNegras) ):
                    return
        self.write("%s|%s|%s|%s|%s|%s|%s\n" % (pv2xpv(pk.strip()), event, oponente, date, eco, result, color))

    def xprocesa(self):
        self.close()
        return self.added

