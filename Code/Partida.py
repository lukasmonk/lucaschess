from Code import Util
from Code import ControlPosicion
from Code import Jugada
from Code import AperturasStd

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
        self.pendienteApertura = True
        self.apertura = None

        self.siEmpiezaConNegras = False
        if iniPosicion:
            self.iniPosicion = iniPosicion.copia()
            self.siEmpiezaConNegras = not self.iniPosicion.siBlancas
        else:
            self.iniPosicion = ControlPosicion.ControlPosicion()
            self.iniPosicion.posInicial()

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
        li = resp.split(" ")

        n = 0
        rp = ""
        for x in li:
            n += len(x) + 1
            if n > 80:
                rp += "\n" + x
                n = len(x)
            else:
                rp += " " + x

        return rp.strip()

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

def pv_san(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    jg = p.jugada(0)
    return jg.pgnSP()

def pv_pgn(fen, pv):
    p = Partida(fen=fen)
    p.leerPV(pv)
    return p.pgnSP()

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

    def readPGN(self, configuracion, pgn):
        from Code import PGN # evita el circulo vicioso
        unpgn = PGN.UnPGN()
        unpgn.leeTexto(pgn)
        self.recuperaDeTexto(unpgn.partida.guardaEnTexto())
        self.asignaApertura(configuracion)

        self.liTags = unpgn.listaCabeceras()
        return self

    def asignaApertura(self, configuracion):
        ap = AperturasStd.ListaAperturasStd(configuracion, False, False)
        ap.asignaApertura(self)

    def pgn(self):
        li = ['[%s "%s"]\n'%(k,v) for k,v in self.liTags]
        txt = "".join(li)
        txt += "\n%s"%self.pgnBase()
        return txt

