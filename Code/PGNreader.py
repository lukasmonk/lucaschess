# -*- coding: latin-1 -*-
import codecs

import Code.Util as Util
import Code.SAK as SAK
import chardet.universaldetector

def openCodec(fich):
    f = open(fich)

    u = chardet.universaldetector.UniversalDetector()

    for n, x in enumerate(f):
        u.feed(x)
        if n == 1000:
            break
    f.close()
    u.close()

    encoding = u.result.get("encoding", "latin-1")
    return codecs.open(fich, "r", encoding, 'ignore')

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
        self.siDesconocido = False  # Si ha sido una terminación de partida, por causas desconocidas
        self.fenPrev = ""
        self.fen = ""

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
                while pos < ntxt and txt[pos] in "abcdefghKQRBN12345678xX-Oo=p":
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
                while pos < ntxt and txt[pos] in "abcdefghKQRBN12345678xX=p.":
                    hasta += 1
                    pos += 1

                mv = Move()
                x = mv.pgn = txt[desde:hasta + 1]
                if not (x in ("ep", "e.p.", "e.p", "ep." )):
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

            elif c == "0":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "0Oo-":
                    hasta += 1
                    pos += 1

                mv = Move()
                x = mv.pgn = txt[desde:hasta + 1].replace("0", "O").upper()
                if len(x) in (3, 5):
                    self.liMoves.append(mv)

            else:
                pos += 1

        SAK.sak.setFEN(fen)
        fenPrev = fen
        for mv in self.liMoves:
            mv.fenPrev = fenPrev
            pv = SAK.sak.pgn2pv(mv.pgn)
            if len(pv) < 4:
                return False
            mv.pv = pv
            mv.desde = pv[:2]
            mv.hasta = pv[2:4]
            mv.coronacion = pv[4:]

            fenPrev = SAK.sak.getFEN()
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

    def readFast(self, fen, txt):
        ntxt = len(txt)
        pos = 0

        SAK.sak.setFEN(fen)
        lip = []
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
                while pos < ntxt and txt[pos] in "abcdefghKQRBN12345678xX-Oo=p":
                    hasta += 1
                    pos += 1

                x = txt[desde:hasta + 1]
                if "-" in x:
                    if "0" in x:
                        x = x.replace("0", "O")
                    elif "o" in x:
                        x = x.replace("o", "O")
                    elif x[0] != "O":
                        x = x.replace("-", "")
                pv = SAK.sak.pgn2pv(x)
                if len(pv) < 4:
                    return None
                lip.append(pv)

            elif c == "e":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "abcdefghKQRBN12345678xX=p.":
                    hasta += 1
                    pos += 1

                x = txt[desde:hasta + 1]
                if not (x in ("ep", "e.p.", "e.p", "ep." )):
                    pv = SAK.sak.pgn2pv(x)
                    if len(pv) < 4:
                        return None
                    lip.append(pv)

            elif c == "(":
                pos += 1
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

            elif c == "{":
                pos += 1
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

            elif c == "$":
                pos += 1
                hasta = pos
                while pos < ntxt and txt[pos].isdigit():
                    hasta += 1
                    pos += 1

            elif c == ";":
                pos += 1
                while pos < ntxt and txt[pos] != "\n":
                    pos += 1

            elif c == "0":
                desde = pos
                hasta = pos
                pos += 1
                while pos < ntxt and txt[pos] in "-0Oo":
                    hasta += 1
                    pos += 1

                x = txt[desde:hasta + 1]
                x = x.replace("0", "O").upper()
                if len(x) in (3, 5):
                    pv = SAK.sak.pgn2pv(x)
                    if len(pv) < 4:
                        return None
                    lip.append(pv)

            else:
                pos += 1
        return " ".join(lip)

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
                clave = li[0].strip()
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

    def readBodyFast(self, body):
        self.pgn += "\n\n" + body + "\n"
        self.pvT = self.moves.readFast(self.fen, body)
        self.erroneo = self.pvT is None

    def pv(self):
        return self.pvT

    def move(self, num):
        return self.moves.liMoves[num]

def read1Game(pgn):
    pgnCab = []
    pgnMov = []
    siCab = True
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

def readGames(pgnfile, siFast=True):
    f = openCodec(pgnfile)
    pgnCab = []
    pgnMov = []
    siBCab = True
    siCab = False
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
        elif siMov:
            if linea:
                if linea[0] == '[' and linea.endswith("]"):
                    g = Game()
                    g.nbytes = nbytes
                    g.readLabels(pgnCab)
                    if siFast:
                        g.readBodyFast("\n".join(pgnMov))
                    else:
                        g.readBody("\n".join(pgnMov))
                    yield g
                    pgnCab = [linea, ]
                    siCab = True
                else:
                    pgnMov.append(linea)
            else:
                g = Game()
                g.nbytes = nbytes
                g.readLabels(pgnCab)
                if siFast:
                    g.readBodyFast("\n".join(pgnMov))
                else:
                    g.readBody("\n".join(pgnMov))
                yield g
                siBCab = True

    if not siBCab:
        g = Game()
        g.nbytes = nbytes
        g.readLabels(pgnCab)
        if siFast:
            g.readBodyFast("\n".join(pgnMov))
        else:
            g.readBody("\n".join(pgnMov))
        yield g
    f.close()

