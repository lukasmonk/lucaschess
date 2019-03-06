import LCEngine4 as LCEngine
from Code import Util
from Code import VarGen
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
            return self.liJugadas[-1] if len(self) > 0 else None

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

    def save2blob(self):
        return Util.str2blob(self.guardaEnTexto())

    def blob2restore(self, blob):
        self.recuperaDeTexto(Util.blob2str(blob))

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
        return self.leerLIPV(pvBloque.split(" "))

    def leerLIPV(self, lipv):
        posicion = self.ultPosicion
        pv = []
        for mov in lipv:
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
            liResp.append(x + (jg.pgnHTML(siFigurines)))
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
        return " ".join([jg.movimiento() for jg in self.liJugadas])

    def lipv(self):
        return [jg.movimiento() for jg in self.liJugadas]

    def pv_hasta(self, njug):
        return " ".join([jg.movimiento() for jg in self.liJugadas[:njug+1]])

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

    def calc_elo_color(self, perfomance, siBlancas):
        bad_moves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        verybad_moves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        nummoves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        sumelos = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        factormoves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        last = OPENING
        for jg in self.liJugadas:
            if jg.analisis:
                if jg.siBlancas() != siBlancas:
                    continue
                if last == ENDGAME:
                    std = ENDGAME
                else:
                    if jg.siBook:
                        std = OPENING
                    else:
                        std = MIDDLEGAME
                        material = jg.posicionBase.valor_material()
                        if material < 15:
                            std = ENDGAME
                        else:
                            pzW, pzB = jg.posicionBase.numPiezasWB()
                            if pzW < 3 and pzB < 3:
                                std = ENDGAME
                jg.estadoOME = std
                last = std
                jg.calc_elo(perfomance)
                if jg.bad_move:
                    bad_moves[std] += 1
                elif jg.verybad_move:
                    verybad_moves[std] += 1
                nummoves[std] += 1
                sumelos[std] += jg.elo*jg.elo_factor
                factormoves[std] += jg.elo_factor

        topes = {}
        elos = {}
        for std in (OPENING, MIDDLEGAME, ENDGAME):
            sume = sumelos[std]
            numf = factormoves[std]
            tope = topes[std] = perfomance.limit(verybad_moves[std], bad_moves[std], nummoves[std])
            if numf:
                elos[std] = int((sume*1.0/numf)*tope/perfomance.limit_max)
            else:
                elos[std] = 0

        sume = 0
        numf = 0
        tope = perfomance.limit_max
        for std in (OPENING, MIDDLEGAME, ENDGAME):
            sume += sumelos[std]
            numf += factormoves[std]
            if topes[std] < tope:
                tope = topes[std]

        if numf:
            elos[ALLGAME] = int((sume*1.0/numf)*tope/perfomance.limit_max)
        else:
            elos[ALLGAME] = 0

        return elos

    def calc_elo_colorFORM(self, perfomance, siBlancas):
        bad_moves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        verybad_moves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        nummoves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        sumelos = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        factormoves = {OPENING:0, MIDDLEGAME:0, ENDGAME:0}
        last = OPENING
        for jg in self.liJugadas:
            if jg.analisis:
                if jg.siBlancas() != siBlancas:
                    continue
                if last == ENDGAME:
                    std = ENDGAME
                else:
                    if jg.siBook:
                        std = OPENING
                    else:
                        std = MIDDLEGAME
                        material = jg.posicionBase.valor_material()
                        if material < 15:
                            std = ENDGAME
                        else:
                            pzW, pzB = jg.posicionBase.numPiezasWB()
                            if pzW < 3 and pzB < 3:
                                std = ENDGAME
                jg.estadoOME = std
                last = std
                jg.elo = calc_formula_elo(jg)
                # jg.calc_elo(perfomance)
                if jg.bad_move:
                    bad_moves[std] += 1
                elif jg.verybad_move:
                    verybad_moves[std] += 1
                nummoves[std] += 1
                sumelos[std] += jg.elo*1.0
                factormoves[std] += 1.0

        topes = {}
        elos = {}
        for std in (OPENING, MIDDLEGAME, ENDGAME):
            sume = sumelos[std]
            numf = factormoves[std]
            tope = topes[std] = perfomance.limit(verybad_moves[std], bad_moves[std], nummoves[std])
            if numf:
                elos[std] = int((sume*1.0/numf)*tope/perfomance.limit_max)
            else:
                elos[std] = 0

        sume = 0
        numf = 0
        tope = perfomance.limit_max
        for std in (OPENING, MIDDLEGAME, ENDGAME):
            sume += sumelos[std]
            numf += factormoves[std]
            if topes[std] < tope:
                tope = topes[std]

        if numf:
            elos[ALLGAME] = int((sume*1.0/numf)*tope/perfomance.limit_max)
        else:
            elos[ALLGAME] = 0

        return elos

    def calc_elos(self, configuracion):
        for jg in self.liJugadas:
            jg.siBook = False
        if self.siFenInicial():
            from Code import Apertura
            ap = Apertura.AperturaPol(999)
            for jg in self.liJugadas:
                jg.siBook = ap.compruebaHumano(jg.posicionBase.fen(), jg.desde, jg.hasta)
                if not jg.siBook:
                    break

        elos = {}
        for siBlancas in (True, False):
            elos[siBlancas] = self.calc_elo_color(configuracion.perfomance, siBlancas)

        elos[None] = {}
        for std in (OPENING, MIDDLEGAME, ENDGAME, ALLGAME):
            elos[None][std] = int((elos[True][std] + elos[False][std])/2.0)

        return elos

    def calc_elosFORM(self, configuracion):
        for jg in self.liJugadas:
            jg.siBook = False
        if self.siFenInicial():
            from Code import Apertura
            ap = Apertura.AperturaPol(999)
            for jg in self.liJugadas:
                jg.siBook = ap.compruebaHumano(jg.posicionBase.fen(), jg.desde, jg.hasta)
                if not jg.siBook:
                    break

        elos = {}
        for siBlancas in (True, False):
            elos[siBlancas] = self.calc_elo_colorFORM(configuracion.perfomance, siBlancas)

        elos[None] = {}
        for std in (OPENING, MIDDLEGAME, ENDGAME, ALLGAME):
            elos[None][std] = int((elos[True][std] + elos[False][std])/2.0)

        return elos

    def asignaApertura(self):
        AperturasStd.ap.asignaApertura(self)

    def asignaTransposition(self):
        AperturasStd.ap.asignaTransposition(self)

    def rotuloApertura(self):
        return self.apertura.trNombre if hasattr(self, "apertura") and self.apertura is not None else None

    def rotuloTransposition(self):
        if hasattr(self, "transposition"):
            ap = self.transposition
            if ap is not None:
                return "%s %s" % (self.jg_transposition.pgnSP(), ap.trNombre)
        return None

    def test_apertura(self):
        if not hasattr(self, "apertura") or self.pendienteApertura:
            self.asignaApertura()
            self.asignaTransposition()


def pv_san(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    jg = p.jugada(0)
    return jg.pgnSP()


def pv_pgn(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    return p.pgnSP()


def lipv_lipgn(lipv):
    LCEngine.setFenInicial()
    li_pgn = []
    for pv in lipv:
        info = LCEngine.moveExPV(pv[:2], pv[2:4], pv[4:])
        li_pgn.append(info._san)
    return li_pgn


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

    def set_extend_tags(self):
        op = self.getTAG("OPENING")
        eco = self.getTAG("ECO")
        if not op or not eco:
            self.asignaApertura()
            if self.apertura:
                if not op:
                    self.liTags.append(("Opening", self.apertura.trNombre))
                if not eco:
                    self.liTags.append(("ECO", self.apertura.eco))
        ply = self.getTAG("PLYCOUNT")
        if not ply:
            self.liTags.append(("PlyCount", str(self.numJugadas())))

    def readPGN(self, pgn):
        from Code import PGN  # evita el circulo vicioso
        unpgn = PGN.UnPGN()
        if not unpgn.leeTexto(pgn):
            return None
        self.recuperaDeTexto(unpgn.partida.guardaEnTexto())
        self.asignaApertura()

        self.liTags = unpgn.listaCabeceras()
        return self

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

    def titulo(self, litags):
        li = []
        for key in litags:
            tag = self.getTAG(key)
            if tag:
                li.append(tag)
        return "-".join(li)

# firstLG = [True]


def calc_formula_elo(jg):  # , limit=200.0):
    with open("./IntFiles/Formulas/eloperformance.formula") as f:
        formula = f.read().strip()

    # dataLG = []
    # titLG = []

    # def LG(key, value):
    #     titLG.append(key)
    #     dataLG.append(str(value))

    cp = jg.posicionBase
    mrm, pos = jg.analisis

    # LG("move", mrm.liMultiPV[pos].movimiento())

    pts = mrm.liMultiPV[pos].puntosABS_5()
    pts0 = mrm.liMultiPV[0].puntosABS_5()
    lostp_abs = pts0 - pts

    # LG("pts best", pts0)
    # LG("pts current", pts)
    # LG("xlostp", lostp_abs)

    piew = pieb = 0
    mat = 0.0
    matw = matb = 0.0
    dmat = {"k": 3.0, "q": 9.9, "r": 5.5, "b": 3.5, "n": 3.1, "p": 1.0}
    for k, v in cp.casillas.iteritems():
        if v:
            m = dmat[v.lower()]
            mat += m
            if v.isupper():
                piew += 1
                matw += m
            else:
                pieb += 1
                matb += m
    base = mrm.liMultiPV[0].puntosABS()
    siBlancas = cp.siBlancas

    gmo34 = gmo68 = gmo100 = 0
    for rm in mrm.liMultiPV:
        dif = abs(rm.puntosABS() - base)
        if dif < 34:
            gmo34 += 1
        elif dif < 68:
            gmo68 += 1
        elif dif < 101:
            gmo100 += 1
    gmo = float(gmo34) + float(gmo68) ** 0.8 + float(gmo100) ** 0.5
    plm = (cp.jugadas - 1) * 2
    if not siBlancas:
        plm += 1

    # xshow: Factor de conversion a puntos para mostrar
    xshow = +1 if siBlancas else -1
    if not VarGen.configuracion.centipawns:
        xshow = 0.01*xshow

    li = (
        ("xpiec", piew if siBlancas else pieb),
        ("xpie", piew + pieb),
        ("xeval", base if siBlancas else -base),
        ("xstm", +1 if siBlancas else -1),
        ("xplm", plm),
        ("xshow", xshow),
        ("xlost", lostp_abs)
    )
    for k, v in li:
        if k in formula:
            formula = formula.replace(k, "%d.0" % v)
    li = (
        ("xgmo", gmo),
        ("xmat", mat),
        ("xpow", matw if siBlancas else matb),
    )
    for k, v in li:

        # LG(k, v)

        if k in formula:
            formula = formula.replace(k, "%f" % v)
    # if "xcompl" in formula:
    #     formula = formula.replace("xcompl", "%f" % calc_formula_elo("complexity", cp, mrm))
    try:
        x = float(eval(formula))
        # if x < 0.0:
        #     x = -x
        # if x > limit:
        #     x = limit

        # LG("elo", int(min(3500, max(0, x))))
        # LG("other elo", int(jg.elo))

        # with open("FormulaELO.csv", "ab") as q:
        #     if firstLG[0]:
        #         firstLG[0] = False
        #         q.write(",".join(titLG) + "\r\n")
        #     q.write(",".join(dataLG) + "\r\n")

        return min(3500, max(0, x))
    except:
        return 0.0
