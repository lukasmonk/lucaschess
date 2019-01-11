import cPickle

from Code import ControlPosicion
from Code import TrListas
from Code import VarGen

NOABANDONO, ABANDONO, ABANDONORIVAL = "N", "S", "R"

# def creaDicHTML():
#     base = '<img src="IntFiles/Figs/%s%s.png">'
#     dic = {}
#     for x in "nbrqk":
#         dic[x.upper()] = base % ("w", x)
#         dic[x] = base % ("b", x)
#     return dic


def creaDicHTML():
    base = '<span style="font-family:Chess Diagramm Pirat">%s</span>'
    ori = "KQRBNPkqrbnp"
    des = "rstuvw" + chr(126) + chr(130) + chr(131) + chr(132) + chr(133) + chr(134)
    def haz(c):
        return des[ori.index(c)]
    dic = {}
    for x in "nbrqk":
        dic[x.upper()] = base % haz(x.upper())
        dic[x] = base % haz(x)
    return dic

dicHTMLFigs = creaDicHTML()


class Jugada:
    def __init__(self):
        self.analisis = None
        self.criticaDirecta = ""

    def ponDatos(self, posicionBase, posicion, desde, hasta, coronacion):
        self.posicionBase = posicionBase
        self.posicion = posicion
        self.siApertura = False

        self.desde = desde
        self.hasta = hasta
        self.coronacion = coronacion if coronacion else ""
        self.siJaque = self.posicion.siJaque()
        self.siJaqueMate = False  # Se determina a posteriori con el motor
        self.siAhogado = False  # Se determina a posteriori con el motor
        self.siTablasRepeticion = False  # Se determina a posteriori con el motor
        self.siTablas50 = False
        self.siTablasAcuerdo = False
        self.siTablasFaltaMaterial = False
        self.siAbandono = NOABANDONO
        self.siDesconocido = False  # Si ha sido una terminacion de partida, por causas desconocidas
        self.pgnBase = posicionBase.pgn(desde, hasta, coronacion)
        self.liMovs = [("b", hasta), ("m", desde, hasta)]
        if self.posicion.liExtras:
            self.liMovs.extend(self.posicion.liExtras)

        self.variantes = ""
        self.comentario = ""
        self.critica = ""
        self.criticaDirecta = ""

    def abandona(self):
        self.siAbandono = ABANDONO

    def abandonaRival(self):
        self.siAbandono = ABANDONORIVAL

    def masCritica1_4(self, critica):
        li = self.critica.strip().split(" ")
        ln = []
        for x in li:
            if x not in ("1", "2", "3", "4"):
                ln.append(x)
        ln.append(critica)
        self.critica = " ".join(ln)

    def siBlancas(self):
        return self.posicionBase.siBlancas

    def fenBase(self):
        return self.posicion.fenBase()

    def siTablas(self):
        return self.siTablasRepeticion or self.siTablas50 or self.siTablasFaltaMaterial or self.siTablasAcuerdo \
               or self.siAhogado

    def pv2dgt(self):
        return self.posicionBase.pv2dgt(self.desde, self.hasta, self.coronacion)

    def siCaptura(self):
        if self.hasta:
            return self.posicionBase.casillas[self.hasta] is not None
        else:
            return False

    def movimiento(self):
        return self.desde + self.hasta + self.coronacion

    def pgnBaseSP(self):
        dConv = TrListas.dConv()
        resp = self.pgnBase
        for k in dConv.keys():
            if k in resp:
                resp = resp.replace(k, dConv[k])
        return resp

    def pgnSP(self):
        return self.pgnBaseSP() + self.resultadoSP()

    def pgnFigurinesSP(self):
        return self.pgnBase + self.resultadoSP()

    def pgnHTML(self, siFigurines):
        siBlancas = self.siBlancas()
        if siFigurines:
            li = []
            for c in self.pgnBase:
                if c in "NBRQK":
                    c = dicHTMLFigs[c if siBlancas else c.lower()]
                li.append(c)
            resp = "".join(li)
        else:
            resp = self.pgnBaseSP()
        resp += self.resto()
        result = self.resultado()
        if result:
            resp += " " + result
        return resp

    def etiAbandono(self):
        xab = self.siAbandono
        if xab == NOABANDONO:
            return ""
        siW = self.posicionBase.siBlancas
        ganaW = "1-0"
        ganaB = "0-1"
        if siW:
            return ganaB if xab == ABANDONO else ganaW
        else:
            return ganaW if xab == ABANDONO else ganaB

    def resultadoSP(self):
        if self.siDesconocido:
            resp = "*"
        elif self.siAbandono != NOABANDONO:
            resp = " " + self.etiAbandono()
        elif self.siAhogado:
            resp = "-" + _("Stalemate")
        elif self.siTablas():
            resp = "-" + _("Draw")
        else:
            resp = ""
        return resp

    def etiquetaSP(self):
        p = self.posicionBase
        return "%d.%s %s" % (p.jugadas, "" if p.siBlancas else "...", self.pgnSP())

    def numMove(self):
        return self.posicionBase.jugadas

    def listaSonidos(self):
        pgn = self.pgnBase
        liMedio = []
        liFinal = []
        if pgn[0] == "O":
            liInicial = [pgn]
        else:
            if "=" in pgn:
                ult = pgn[-1]
                if ult.lower() in "qrnb":
                    liFinal = ["=", pgn[-1]]
                    pgn = pgn[:-2]
                else:
                    liFinal = ["=", pgn[-2], pgn[-1]]
                    pgn = pgn[:-3]
            elif pgn.endswith("e.p."):
                pgn = pgn[:-4]
            liMedio = [pgn[-2], pgn[-1]]
            pgn = pgn[:-2]
            liInicial = list(pgn)
            if liInicial and liInicial[-1] == "x":
                piece = self.posicionBase.casillas[self.hasta]
                if piece is None: # e.p.
                    piece = "P"
                liInicial.append(piece.upper())
            if (not liInicial) or (not liInicial[0].isupper()):
                liInicial.insert(0, "P")

        li = liInicial
        li.extend(liMedio)
        li.extend(liFinal)
        return li

    def pgnEN(self):
        resp = self.pgnBase + self.resto()
        result = self.resultado()
        if result:
            resp += " " + result
        return resp

    def resto(self):
        resp = ""
        if self.criticaDirecta:
            resp += self.criticaDirecta

        if self.critica:
            self.critica = self.critica.strip()
            if "$" in self.critica:
                self.critica = self.critica.replace("$", "")
            li = self.critica.split(" ")
            for x in li:
                if x != "0":
                    resp += " $%s" % x
        if self.comentario:
            resp += " "
            for txt in self.comentario.strip().split("\n"):
                if txt:
                    resp += "{%s}" % txt.strip()
        if self.variantes:
            for x in self.variantes.strip().split("\n"):
                x = x.strip()
                if x:
                    resp += " (%s)" % x.strip()

        return (" %s" % resp.strip()) if resp else ""

    def resultado(self):
        if self.siAbandono != NOABANDONO:
            result = self.etiAbandono()
        elif self.siAhogado or self.siTablasRepeticion or self.siTablas50 or self.siTablasFaltaMaterial or self.siTablasAcuerdo:
            result = "1/2-1/2"
        elif self.siDesconocido:
            result = "*"
        else:
            result = ""
        return result

    def guardaEnTexto(self):
        def mas(c):
            return c + VarGen.XSEP

        def masL(si):
            return ("S" if si else "N") + VarGen.XSEP

        txt = ""
        txt += mas(self.posicionBase.fen())
        txt += mas(self.posicion.fen())
        txt += mas(self.desde)
        txt += mas(self.hasta)
        txt += mas(self.coronacion)
        txt += masL(self.siJaque)
        txt += masL(self.siJaqueMate)
        txt += masL(self.siAhogado)
        txt += masL(self.siTablasRepeticion)
        txt += masL(self.siTablasFaltaMaterial)
        txt += masL(self.siTablas50)
        txt += masL(self.siApertura)
        txt += mas(self.pgnBase)
        txt += mas(self.siAbandono)
        txt += mas(self.variantes)
        txt += mas(self.comentario)
        txt += mas(self.critica)
        txt += masL(self.siDesconocido)

        txt += mas(cPickle.dumps(self.analisis).replace("\r\n", "#&").replace("\n", "#&") if self.analisis else "")

        txt += mas(self.criticaDirecta)
        txt += masL(self.siTablasAcuerdo)

        return txt

    def recuperaDeTexto(self, txt):

        li = txt.split(VarGen.XSEP)

        def xL(num):
            return li[num] == "S"

        self.posicionBase = ControlPosicion.ControlPosicion()
        self.posicion = ControlPosicion.ControlPosicion()
        self.posicionBase.leeFen(li[0])
        self.posicion.leeFen(li[1])
        self.desde = li[2]
        self.hasta = li[3]
        self.coronacion = li[4]
        self.siJaque = xL(5)
        self.siJaqueMate = xL(6)
        self.siAhogado = xL(7)
        self.siTablasRepeticion = xL(8)
        self.siTablasFaltaMaterial = xL(9)
        self.siTablas50 = xL(10)
        self.siApertura = xL(11)
        self.pgnBase = li[12]
        self.siAbandono = li[13]
        self.variantes = li[14]
        self.comentario = li[15]
        self.critica = li[16]
        self.siDesconocido = xL(17)

        if len(li) > 18 and li[18]:
            try:
                self.analisis = cPickle.loads(str(li[18].replace("#&", "\n")))
            except:
                pass

        if len(li) > 19 and li[19]:
            self.criticaDirecta = li[19]

        if len(li) > 20:
            self.siTablasAcuerdo = xL(20)

        self.liMovs = [("b", self.hasta), ("m", self.desde, self.hasta)]
        if self.posicion.liExtras:
            self.liMovs.extend(self.posicion.liExtras)

    def clona(self):
        jg = Jugada()
        jg.recuperaDeTexto(self.guardaEnTexto())
        return jg

    def __str__(self):
        if self.siJaqueMate:
            mas = "Mate"
        elif self.siJaque:
            mas = "Jaque"
        elif self.siAhogado:
            mas = "Ahogado"
        elif self.siTablasRepeticion:
            mas = "Tablas por repeticion"
        elif self.siTablasAcuerdo:
            mas = "Tablas por acuerdo"
        else:
            mas = ""
        return "%s %s %s: %s" % (self.desde, self.hasta, mas, self.posicion.fen())

    def borraVariantesLC(self):
        li = self.variantes.split("\n\n")
        lir = []
        for x in li:
            if not (x.count("{") == 1 and x.count(".}") == 1):
                lir.append(x)
        if lir:
            self.variantes = "\n\n".join(lir).strip()
        else:
            self.variantes = ""

    def analisis2variantes(self, partidaTemporal, almVariantes, siBorrarPrevio):
        if not self.analisis:
            return
        mrm, pos = self.analisis
        if len(mrm.liMultiPV) == 0:
            return
        if siBorrarPrevio:
            self.borraVariantesLC()
        if almVariantes.infovariante:
            nombre = mrm.nombre
            if mrm.maxTiempo:
                t = "%0.2f" % (float(mrm.maxTiempo) / 1000.0,)
                t = t.rstrip("0")
                if t[-1] == ".":
                    t = t[:-1]
                etiT = "%s %s" % (_("Second(s)"), t)
            elif mrm.maxProfundidad:
                etiT = "%s %d" % (_("Depth"), mrm.maxProfundidad)
            else:
                etiT = ""
            etiT = " " + nombre + " " + etiT
        else:
            etiT = ""

        if almVariantes.mejorvariante:
            for rm in mrm.liMultiPV:
                if rm.movimiento() != self.movimiento():
                    self.analisis2variantesUno(partidaTemporal, rm, etiT, almVariantes.unmovevariante,
                                               almVariantes.siPDT)
                    break
        else:
            for rm in mrm.liMultiPV:
                self.analisis2variantesUno(partidaTemporal, rm, etiT, almVariantes.unmovevariante, almVariantes.siPDT)

    def analisis2variantesUno(self, partidaTemporal, rm, etiT, siUnMove, siPDT):

        puntuacion = rm.abrTextoPDT() if siPDT else rm.abrTexto()
        comentario = " {%s%s} " % (puntuacion, etiT)

        partidaTemporal.reset(self.posicionBase)
        partidaTemporal.leerPV(rm.pv)

        jugada = partidaTemporal.pgnBase(self.posicionBase.jugadas)
        li = jugada.split(" ")

        n = 1 if self.posicionBase.siBlancas else 2
        if siUnMove:
            li = li[:n]
        variante = (" ".join(li[:n]) + comentario + " ".join(li[n:])).replace("\n", " ").replace("\r", "").replace("  ", " ").strip()
        variantes = self.variantes
        if variantes:
            if variante in variantes:  # Sin repetidos
                return
            variantes = variantes.strip() + "\n\n" + variante
        else:
            variantes = variante
        self.variantes = variantes

    def borraCV(self):
        self.variantes = ""
        self.comentario = ""
        self.critica = ""
        self.criticaDirecta = ""

    def calc_elo(self, perfomance):
        if self.analisis:
            mrm, pos = self.analisis
            pts = mrm.liMultiPV[pos].puntosABS()
            pts0 = mrm.liMultiPV[0].puntosABS()
            lostp_abs = pts0 - pts
            self.elo, self.bad_move, self.verybad_move = perfomance.elo_bad_vbad(lostp_abs)

            li = list({rm.puntosABS() for rm in mrm.liMultiPV})
            li.sort()
            ant = li[0]
            nfactor = 1
            for x in range(1, len(li)):
                nue = li[x]
                if nue-ant > 15:
                    nfactor += 1
                    ant = nue
            self.elo_factor = nfactor

        else:
            self.elo = 0
            self.bad_move = False
            self.verybad_move = False

    def distancia(self):
        return ControlPosicion.distancia(self.desde, self.hasta)


def dameJugada(posicionBase, desde, hasta, coronacion):
    posicion = posicionBase.copia()
    siBien, mensError = posicion.mover(desde, hasta, coronacion)
    if siBien:
        jg = Jugada()
        jg.ponDatos(posicionBase, posicion, desde, hasta, coronacion)
        return True, None, jg
    else:
        return False, mensError, None
