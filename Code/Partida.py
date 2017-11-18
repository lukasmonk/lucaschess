from Code import Util
from Code import ControlPosicion
from Code import Jugada
from Code import AperturasStd

OPENING, MIDDLEGAME, ENDGAME, ALLGAME = range(4)


class Partida:
    def __init__(self, iniPosicion=None, fen=None):
        self.firstComment = ""
        if fen:
            self.resetFEN(fen)
        else:
            self.reset(iniPosicion)

    def resetFEN(self, fen):
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fen)
        self.reset(cp)

    def reset(self, iniPosicion=None):
        self.liJugadas = []
        self.apertura = None

        self.siEmpiezaConNegras = False
        if iniPosicion:
            self.iniPosicion = iniPosicion.copia()
            self.siEmpiezaConNegras = not self.iniPosicion.siBlancas
            self.pendienteApertura = self.iniPosicion.fen() == ControlPosicion.FEN_INICIAL
        else:
            self.iniPosicion = ControlPosicion.ControlPosicion()
            self.iniPosicion.posInicial()
            self.pendienteApertura = True

        self.ultPosicion = self.iniPosicion.copia()

    def primeraJugada(self):
        return self.iniPosicion.jugadas

    def jugada(self, num):
        try:
            return self.liJugadas[num]
        except:
            return None

    def append_jg(self, jg):
        self.liJugadas.append(jg)
        self.ultPosicion = jg.posicion

    def siFenInicial(self):
        return self.iniPosicion.fen() == ControlPosicion.FEN_INICIAL

    def numJugadaPGN(self, njug):
        primera = int(self.iniPosicion.jugadas)
        if self.siEmpiezaConNegras:
            njug += 1
        return primera + njug / 2

    def numJugadas(self):
        return len(self.liJugadas)

    def __len__(self):
        return len(self.liJugadas)

    def last_jg(self):
        return self.liJugadas[-1]

    def guardaEnTexto(self):
        txt = self.iniPosicion.fen() + "|"
        txt += self.ultPosicion.fen() + "|"
        txt += str(len(self.liJugadas)) + "|"
        for jg in self.liJugadas:
            txt += jg.guardaEnTexto().replace("|", "-") + "|"
        txt += self.firstComment.replace("|", "-") + "|"
        return txt

    def leeOtra(self, otra):
        txt = otra.guardaEnTexto()
        self.recuperaDeTexto(txt)
        self.apertura = otra.apertura

    def recuperaDeTexto(self, txt):
        li = txt.split("|")
        self.iniPosicion = ControlPosicion.ControlPosicion().leeFen(li[0])
        self.ultPosicion = ControlPosicion.ControlPosicion().leeFen(li[1])
        nJG = int(li[2])
        self.liJugadas = []
        for n in range(nJG):
            jg = Jugada.Jugada()
            jg.recuperaDeTexto(li[n + 3])
            self.liJugadas.append(jg)
        if len(li) > nJG + 3:
            self.firstComment = li[nJG + 3]
        else:
            self.firstComment = ""
        self.siEmpiezaConNegras = not self.iniPosicion.siBlancas

    def si3repetidas(self):
        nJug = len(self.liJugadas)
        if nJug > 5:
            fenBase = self.liJugadas[nJug - 1].fenBase()
            liRep = [nJug - 1]
            for n in range(nJug - 1):
                if self.liJugadas[n].fenBase() == fenBase:
                    liRep.append(n)
                    if len(liRep) == 3:
                        return liRep
        return None

    def leerPV(self, pvBloque):
        posicion = self.ultPosicion
        pv = []
        for mov in pvBloque.split(" "):
            if len(mov) >= 4 and mov[0] in "abcdefgh" and mov[1] in "12345678" and mov[2] in "abcdefgh" \
                    and mov[3] in "12345678":
                pv.append(mov)
            else:
                break

        siB = self.siBlancas

        for mov in pv:
            desde = mov[:2]
            hasta = mov[2:4]
            if len(mov) == 5:
                coronacion = mov[4]
                if siB:
                    coronacion = coronacion.upper()
            else:
                coronacion = None
            siBien, mens, jg = Jugada.dameJugada(posicion, desde, hasta, coronacion)
            if siBien:
                self.liJugadas.append(jg)
                posicion = jg.posicion
            siB = not siB
        self.ultPosicion = posicion
        return self

    def damePosicion(self, pos):
        nJugadas = len(self.liJugadas)
        if nJugadas:
            return self.liJugadas[pos].posicion
        else:
            return self.iniPosicion

    def fenUltimo(self):
        return self.ultPosicion.fen()

    def fensActual(self):
        resp = self.iniPosicion.fen() + "\n"
        for jg in self.liJugadas:
            resp += jg.posicion.fen() + "\n"

        return resp

    def siBlancas(self):
        return self.ultPosicion.siBlancas

    def pgnBaseRAW(self, numJugada=None):
        if self.firstComment:
            resp = "{%s} " % self.firstComment
        else:
            resp = ""
        if numJugada is None:
            numJugada = self.primeraJugada()
        if self.siEmpiezaConNegras:
            resp += "%d... " % numJugada
            numJugada += 1
            salta = 1
        else:
            salta = 0
        for n, jg in enumerate(self.liJugadas):
            if n % 2 == salta:
                resp += " %d." % numJugada
                numJugada += 1
            resp += jg.pgnEN() + " "

        resp = resp.replace("\r\n", " ").replace("\n", " ").replace("\r", " ").replace("  ", " ").strip()

        return resp

    def pgnBase(self, numJugada=None):
        resp = self.pgnBaseRAW(numJugada)
        li = []
        ln = len(resp)
        pos = 0
        while pos < ln:
            while resp[pos] == " ":
                pos += 1
            final = pos + 80
            txt = resp[pos:final]
            if txt[-1] == " ":
                txt = txt[:-1]
            elif final < ln:
                if resp[final] == " ":
                    final += 1
                else:
                    while final > pos and resp[final - 1] != " ":
                        final -= 1
                    if final > pos:
                        txt = resp[pos:final]
                    else:
                        final = pos + 80
            li.append(txt)
            pos = final
        if li:
            li[-1] = li[-1].strip()
            return "\n".join(li)
        else:
            return ""

    def setFirstComment(self, txt, siReplace=False):
        if siReplace or not self.firstComment:
            self.firstComment = txt
        else:
            self.firstComment = "%s\n%s" % (self.firstComment.strip(), txt)

    def pgnSP(self, numJugada=None, hastaJugada=9999):
        if self.firstComment:
            resp = "{%s} " % self.firstComment
        else:
            resp = ""
        if numJugada is None:
            numJugada = self.primeraJugada()
        if self.siEmpiezaConNegras:
            resp += "%d... " % numJugada
            numJugada += 1
            salta = 1
        else:
            salta = 0
        for n, jg in enumerate(self.liJugadas):
            if n > hastaJugada:
                break
            if n % 2 == salta:
                resp += "%d." % numJugada
                numJugada += 1

            resp += jg.pgnSP() + " "

        return resp.strip()

    def pgnHTML(self, numJugada=None, hastaJugada=9999, siFigurines=True):
        liResp = []
        if self.firstComment:
            liResp.append("{%s}" % self.firstComment)
        if numJugada is None:
            numJugada = self.primeraJugada()
        if self.siEmpiezaConNegras:
            liResp.append('<span style="color:navy">%d...</span>' % numJugada)
            numJugada += 1
            salta = 1
        else:
            salta = 0
        for n, jg in enumerate(self.liJugadas):
            if n > hastaJugada:
                break
            if n % 2 == salta:
                x = '<span style="color:navy">%d.</span>' % numJugada
                numJugada += 1
            else:
                x = ""
            liResp.append(x + (jg.pgnHTML() if siFigurines else jg.pgnSP()))
        return " ".join(liResp)

    def siTerminada(self):
        if self.liJugadas:
            jg = self.liJugadas[-1]
            if jg.siTablas() or jg.siJaqueMate:
                return True
            if jg.posicion.siTerminada():
                if jg.siJaque:
                    jg.siJaqueMate = True
                else:
                    jg.siAhogado = True
                return True
        return False

    def resultado(self):
        resp = "*"
        if self.liJugadas:
            result = self.liJugadas[-1].resultado()
            if result:
                resp = result
        return resp

    def siEstaTerminada(self):
        return self.resultado() != "*"

    def pv(self):
        resp = ""
        for jg in self.liJugadas:
            resp += jg.movimiento() + " "
        return resp.strip()

    def anulaUltimoMovimiento(self, siBlancas):
        del self.liJugadas[-1]
        ndel = 1
        if self.liJugadas and self.liJugadas[-1].posicion.siBlancas != siBlancas:
            del self.liJugadas[-1]
            ndel += 1
        if self.liJugadas:
            self.ultPosicion = self.liJugadas[-1].posicion
        else:
            self.ultPosicion = self.iniPosicion
        return ndel

    def anulaSoloUltimoMovimiento(self):
        if self.liJugadas:
            del self.liJugadas[-1]
            if self.liJugadas:
                self.ultPosicion = self.liJugadas[-1].posicion
            else:
                self.ultPosicion = self.iniPosicion

    def copia(self, hastaJugada=None):
        p = Partida()
        p.leeOtra(self)
        if hastaJugada is not None:
            if hastaJugada == -1:
                p.liJugadas = []
            elif hastaJugada < (p.numJugadas() - 1):
                p.liJugadas = p.liJugadas[:hastaJugada + 1]
            if p.liJugadas:
                p.ultPosicion = p.liJugadas[-1].posicion.copia()
            else:
                p.ultPosicion = p.iniPosicion.copia()
        return p

    def copiaDesde(self, desdeJugada):
        if desdeJugada == 0:
            cp = self.iniPosicion
        else:
            cp = self.liJugadas[desdeJugada - 1].posicion
        p = Partida(cp)
        p.liJugadas = self.liJugadas[desdeJugada:]
        if p.liJugadas:
            p.ultPosicion = p.liJugadas[-1].posicion.copia()
        else:
            p.ultPosicion = p.iniPosicion.copia()
        return p

    def pgnBaseRAWcopy(self, numJugada, hastaJugada):
        resp = ""
        if numJugada is None:
            numJugada = self.primeraJugada()
        if self.siEmpiezaConNegras:
            resp += "%d... " % numJugada
            numJugada += 1
            salta = 1
        else:
            salta = 0
        for n, jg in enumerate(self.liJugadas[:hastaJugada + 1]):
            if n % 2 == salta:
                resp += " %d." % numJugada
                numJugada += 1

            resp += jg.pgnEN() + " "

        resp = resp.replace("\n", " ").replace("\r", " ").replace("  ", " ").strip()

        return resp

    def abandona(self, siBlancas):
        if not self.liJugadas:
            return

        jg = self.liJugadas[-1]

        if jg.siBlancas() == siBlancas:
            jg.abandona()
        else:
            jg.abandonaRival()

    def borraCV(self):
        self.firstComment = ""
        for jugada in self.liJugadas:
            jugada.borraCV()

    def remove_analysis(self):
        for jg in self.liJugadas:
            jg.analisis = None

    def calc_elo_color(self, formula, siBlancas):
        bad_moves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        verybad_moves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        nummoves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        sumelos = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        for jg in self.liJugadas:
            if jg.analisis:
                if jg.siBlancas() != siBlancas:
                    continue
                if jg.siApertura:
                    std = jg.estadoOME = OPENING
                else:
                    material = jg.posicionBase.valor_material()
                    std = jg.estadoOME = ENDGAME if material < 15 else MIDDLEGAME
                jg.calc_elo(formula)
                if jg.bad_move:
                    bad_moves[std] += 1
                elif jg.verybad_move:
                    verybad_moves[std] += 1
                nummoves[std] += 1
                sumelos[std] += jg.elo

        def calc_tope(verybad, bad, nummoves):
            if verybad or bad:
                return int(max(3500 - 16000.0*verybad/nummoves - 4000.0*bad/nummoves, 1200.0))
            else:
                return 3500

        topes = {}
        elos = {}
        for std in (OPENING, MIDDLEGAME, ENDGAME):
            nume = nummoves[std]
            sume = sumelos[std]
            tope = topes[std] = calc_tope(verybad_moves[std], bad_moves[std], nummoves[std])
            if nume:
                elos[std] = int((sume*1.0/nume)*tope/3500.0)
            else:
                elos[std] = 0

        sume = 0
        nume = 0
        tope = 3500
        for std in (OPENING, MIDDLEGAME, ENDGAME):
            sume += sumelos[std]
            nume += nummoves[std]
            if topes[std] < tope:
                tope = topes[std]

        if nume:
            elos[ALLGAME] = int((sume*1.0/nume)*tope/3500.0)
        else:
            elos[ALLGAME] = 0

        return elos

    def calc_elos(self, configuracion):
        if self.siFenInicial():
            AperturasStd.ap.asignaApertura(self)
        else:
            for jg in self.liJugadas:
                jg.siApertura = False
        with open("IntFiles/Formulas/eloperformance.formula") as f:
            formula = f.read().strip()

        elos = {}
        for siBlancas in (True, False):
            elos[siBlancas] = self.calc_elo_color(formula, siBlancas)

        elos[None] = {}
        for std in (OPENING, MIDDLEGAME, ENDGAME, ALLGAME):
            elos[None][std] = int((elos[True][std] + elos[False][std])/2.0)

        return elos


def pv_san(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    jg = p.jugada(0)
    return jg.pgnSP()


def pv_pgn(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    return p.pgnSP()


def pv_pgn_raw(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    return p.pgnBaseRAW()


class PartidaCompleta(Partida):
    def __init__(self, iniPosicion=None, fen=None, liTags=None):
        self.liTags = liTags if liTags else []
        Partida.__init__(self, iniPosicion=iniPosicion, fen=fen)

    def iswhite(self):
        return self.iniPosicion.siBlancas

    def save(self):
        return Util.dic2txt(self.liTags) + "]" + self.guardaEnTexto()

    def restore(self, fromtxt):
        n = fromtxt.find("]")
        if n:
            self.liTags = Util.txt2dic(fromtxt[:n])
            self.recuperaDeTexto(fromtxt[n+1:])

    def setTags(self, litags):
        self.liTags = litags[:]

    def getTAG(self, tag):
        for k,v in self.liTags:
            if k.upper() == tag:
                return v
        return ""

    def dicTags(self):
        return {k:v for k, v in self.liTags}

    def readPGN(self, configuracion, pgn):
        from Code import PGN  # evita el circulo vicioso
        unpgn = PGN.UnPGN()
        if not unpgn.leeTexto(pgn):
            return None
        self.recuperaDeTexto(unpgn.partida.guardaEnTexto())
        self.asignaApertura(configuracion)

        self.liTags = unpgn.listaCabeceras()
        return self

    def asignaApertura_raw(self, ap):
        ap.asignaApertura(self)

    def asignaApertura(self, configuracion):
        AperturasStd.ap.asignaApertura(self)

    def pgn(self):
        li = ['[%s "%s"]\n'%(k,v) for k,v in self.liTags]
        txt = "".join(li)
        txt += "\n%s"%self.pgnBase()
        return txt

    def resetFEN(self, fen):
        ok = False
        for n, tag in enumerate(self.liTags):
            if tag[0] == "FEN":
                self.liTags[n] = ("FEN", fen)
                ok = True
                break
        if not ok and fen != ControlPosicion.FEN_INICIAL:
            self.liTags.append(("FEN", fen))
        Partida.resetFEN(self, fen)

