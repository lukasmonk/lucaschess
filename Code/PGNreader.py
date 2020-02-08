import LCEngine4 as LCEngine

from Code import Util


class Move:
    def __init__(self):
        self.pgn = ""
        self.pv = ""
        self.comentarios = []
        self.variantes = []
        self.criticas = []
        self.desde = None
        self.hasta = None
        self.coronacion = None
        self.siMate = False
        self.siDesconocido = False  # Si ha sido una terminacion de partida, por causas desconocidas
        self.fenPrev = ""
        self.fen = ""

    def clona(self):
        m = Move()
        m.pgn = self.pgn
        m.pv = self.pv
        m.comentarios = self.comentarios
        m.variantes = self.variantes
        m.criticas = self.criticas
        m.desde = self.desde
        m.hasta = self.hasta
        m.coronacion = self.coronacion
        m.siMate = self.siMate
        m.siDesconocido = self.siDesconocido
        m.fenPrev = self.fenPrev
        m.fen = self.fen
        return m


class Moves:
    def __init__(self):
        self.liMoves = []
        self.firstComment = ""

    def toPGN(self):
        li = []
        if self.liMoves:
            mv = self.liMoves[0]
            siW = "w" in mv.fenPrev
            njug = int(mv.fenPrev.split(" ")[-1])
            if not siW:
                li.append("%d..." % njug)
            for mv in self.liMoves:
                if siW:
                    li.append("%d.%s" % (njug, mv.pgn))
                else:
                    njug += 1
                    li.append(mv.pgn)
                if mv.criticas:
                    for una in mv.criticas:
                        if una.isdigit():
                            li.append("$%s" % una)
                        else:
                            li.append(una)
                if mv.comentarios:
                    for uno in mv.comentarios:
                        li.append("{%s}" % uno)
                if mv.variantes:
                    for una in mv.variantes:
                        li.append("(%s)" % una.toPGN())
                siW = not siW
        return " ".join(li)

    def read(self, fen, txt):
        ntxt = len(txt)
        pos = 0

        mv = Move()
        mv.pgn = ""
        while pos < ntxt:
            c = txt[pos]

            if c in "123456789":
                pos += 1
                while pos < ntxt and txt[pos] in "1234567890.":
                    pos += 1

            elif c in "abcdfghKQRBNOo":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "abcdefghKQRBN12345678xX-Oo=p+":
                    hasta += 1
                    pos += 1

                mv = Move()
                x = mv.pgn = txt[desde:hasta + 1]
                if "-" in x:
                    if "o" in x:
                        x = x.replace("o", "O")
                    elif x[0] != "O":
                        x = x.replace("-", "")
                    mv.pgn = x
                self.liMoves.append(mv)

            elif c == "e":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "abcdefghKQRBN12345678xX-=p.":
                    hasta += 1
                    pos += 1

                mv = Move()
                x = mv.pgn = txt[desde:hasta + 1]
                if "-" in x:
                    x = x.replace("-", "")
                    mv.pgn = x
                if x.endswith("e.p."):
                    x = x[:-4]
                    mv.pgn = x
                if x and not (x in ("ep", "e.p.", "e.p", "ep.")):
                    self.liMoves.append(mv)

            elif c == "$":
                pos += 1
                desde = pos
                hasta = pos
                while pos < ntxt and txt[pos].isdigit():
                    hasta += 1
                    pos += 1
                mv.criticas.append(txt[desde:hasta])

            elif c in "?!":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "!?":
                    hasta += 1
                    pos += 1
                mv.criticas.append(txt[desde:hasta + 1])

            elif c == "(":
                pos += 1
                desde = pos
                hasta = pos
                par = 1
                coment = 0
                while pos < ntxt:
                    c = txt[pos]
                    if coment:
                        if c == "{":
                            coment += 1
                        elif c == "}":
                            coment -= 1
                    else:
                        if c == "(":
                            par += 1
                        elif c == ")":
                            par -= 1
                            if par == 0:
                                break
                        elif c == "{":
                            coment = 1
                    hasta += 1
                    pos += 1
                mv.variantes.append(txt[desde:hasta].replace("\r\n", " ").replace("\r", " ").replace("\n", " ").strip())

            elif c == "{":
                pos += 1
                desde = pos
                hasta = pos
                par = 1
                while pos < ntxt:
                    c = txt[pos]
                    if c == "{":
                        par += 1
                    elif c == "}":
                        par -= 1
                        if par == 0:
                            break
                    hasta += 1
                    pos += 1
                comment = txt[desde:hasta].replace("\r\n", " ").replace("\r", " ").replace("\n", " ").strip()
                if not mv.pgn:
                    self.firstComment = comment
                else:
                    mv.comentarios.append(comment)

            elif c == ";":
                pos += 1
                while pos < ntxt and txt[pos] != "\n":
                    pos += 1

            elif c == "#":
                mv.siMate = True
                pos += 1

            elif c == "*":
                mv.siDesconocido = True
                pos += 1

            elif c in "0":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "0Oo-+":
                    hasta += 1
                    pos += 1

                mv = Move()
                x = mv.pgn = txt[desde:hasta + 1].replace("0", "O").upper()
                if x in ("O-O", "O-O-O", "O-O+", "O-O-O+"):
                    self.liMoves.append(mv)
            else:
                pos += 1

        LCEngine.setFen(fen)
        fenPrev = fen
        for mv in self.liMoves:
            mv.fenPrev = fenPrev
            if mv.pgn in ("O-O+", "O-O-O+"):
                mv.pgn = mv.pgn[:-1]
            pv = LCEngine.lc_pgn2pv(mv.pgn)
            if len(pv) < 4:
                return False
            mv.pv = pv
            mv.desde = pv[:2]
            mv.hasta = pv[2:4]
            mv.coronacion = pv[4:]

            if not LCEngine.movePV(mv.desde, mv.hasta, mv.coronacion):
                return False
            fenPrev = LCEngine.getFen()
            mv.fen = fenPrev

        # Se hace separado para que no influya
        for mv in self.liMoves:
            if mv.variantes:
                livar = []
                for variante in mv.variantes:
                    moves = Moves()
                    if moves.read(mv.fenPrev, variante):
                        livar.append(moves)
                mv.variantes = livar

        return True


class Game:
    def __init__(self):
        self.labels = Util.SymbolDict()
        self.moves = Moves()
        self.fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        self.pgn = ""
        self.erroneo = False

    def plies(self):
        return len(self.moves.liMoves)

    def readLabels(self, liTxt):
        for linea in liTxt:
            li = linea[1:-1].replace('""', '"').split('"')
            if len(li) == 3:
                clave = li[0].strip().replace(" ", "")
                ok = True
                for c in clave:
                    if not( 33 < ord(c) < 127):
                        ok = False
                        break
                if not ok:
                    continue
                valor = li[1].strip()
                if clave and valor:
                    if clave.upper() == "FEN":
                        clave = clave.upper()
                        if valor:
                            self.fen = valor
                    if valor:
                        self.labels[clave] = valor
        self.pgn = "\n".join(liTxt)

    def readBody(self, body):
        self.pgn += "\n\n" + body + "\n"
        if not self.moves.read(self.fen, body):
            self.erroneo = True
        self.pvT = " ".join([move.pv for move in self.moves.liMoves if move.pv])

    def pv(self):
        return self.pvT

    def move(self, num):
        return self.moves.liMoves[num]


def read1Game(pgn):
    pgnCab = []
    pgnMov = []
    siCab = True
    siMov = False
    for linea in pgn.split("\n"):
        linea = linea.strip()
        if siCab:
            if linea:
                if linea[0] == "[":
                    pgnCab.append(linea)
                else:
                    siCab = False
                    siMov = True
                    pgnMov = [linea, ]
        elif siMov:
            if linea:
                pgnMov.append(linea)
    g = Game()
    g.readLabels(pgnCab)
    g.readBody("\n".join(pgnMov))
    return g


def readGames(pgnfile):
    with Util.OpenCodec(pgnfile) as f:
        pgnCab = []
        pgnMov = []
        siBCab = True
        siCab = False
        siMov = False
        nbytes = 0
        for linea in f:
            nbytes += len(linea)
            linea = linea.strip()
            if siBCab:
                if linea and linea[0] == "[":
                    pgnCab = [linea, ]
                    siBCab = False
                    siCab = True
            elif siCab:
                if linea:
                    if linea[0] == "[":
                        pgnCab.append(linea)
                    else:
                        siCab = False
                        siMov = True
                        pgnMov = [linea, ]
                else:
                    siCab = False
                    siMov = True
                    pgnMov = []
            elif siMov:
                if linea:
                    if linea[0] == '[' and linea.endswith("]"):
                        g = Game()
                        g.nbytes = nbytes
                        g.readLabels(pgnCab)
                        g.readBody("\n".join(pgnMov))
                        yield g
                        pgnCab = [linea, ]
                        siCab = True
                    else:
                        pgnMov.append(linea)
                # else:
                    # g = Game()
                    # g.nbytes = nbytes
                    # g.readLabels(pgnCab)
                    # g.readBody("\n".join(pgnMov))
                    # yield g
                    # siBCab = True

        if not siBCab:
            g = Game()
            g.nbytes = nbytes
            g.readLabels(pgnCab)
            g.readBody("\n".join(pgnMov))
            yield g
