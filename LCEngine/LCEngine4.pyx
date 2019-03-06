cimport cython


cdef extern from "irina.h":
    ctypedef struct Move:
        pass

    void init_board()
    void fen_board(char *fen)
    char *board_fen(char *fen)

    int movegen()
    int pgn2pv(char *pgn, char *pv)
    int make_nummove(int num)
    char * playFen(char *fen, int depth, int time)
    int numMoves( )
    void getMove( int num, char * pv )
    char *board_fen(char *fen)
    int numBaseMove( )
    int searchMove( char *desde, char *hasta, char * promotion )
    void getMoveEx( int num, char * info )
    char * toSan(int num, char *sanMove)
    char inCheck()
    void set_level(int lv)

    void pgn_start(char * fich, int depth)
    void pgn_stop()
    int pgn_read( )
    char * pgn_game()
    char * pgn_pv()
    int pgn_numlabels()
    char * pgn_label(int num)
    char * pgn_value(int num)
    int pgn_raw()
    int pgn_numfens()
    char * pgn_fen(int num)




class PGNreader:
    def __init__(self, fich, depth):
        self.fich = fich
        self.depth = depth

    def __enter__(self):
        pgn_start(self.fich, self.depth)
        return self

    def __exit__(self, type, value, traceback):
        pgn_stop()

    def __iter__(self):
        return self

    def next(self):
        n = pgn_read()
        if n:
            pgn = pgn_game()
            pv = pgn_pv()
            d = {}
            dlw = {}
            n = pgn_numlabels()
            r = pgn_raw()
            fens = [ pgn_fen(num) for num in range(pgn_numfens()) ]
            if n:
                for x in range(n):
                    d[pgn_label(x).upper()] = pgn_value(x)
                    dlw[pgn_label(x).upper()] = pgn_label(x)
            return pgn, pv, d, r, fens, dlw
        else:
            raise StopIteration


def lc_pgn2pv(pgn1):
    cdef char pv[10];
    resp = pgn2pv(pgn1, pv)
    if resp == 9999:
        return ""
    else:
        return pv


def posFC(pos):
    return pos / 8, pos % 8

def FCpos(f, c):
    return f * 8 + c

def posA1(pos):
    return chr(pos % 8 + 97) + chr(pos / 8 + 49)

def a1Pos(a1):
    cdef int f, c
    f = ord(a1[1]) - 49
    c = ord(a1[0]) - 97
    return f * 8 + c

def move2num(a1h8q):
    num = a1Pos(a1h8q[:2]) + a1Pos(a1h8q[2:4])*64
    if len(a1h8q)>4:
        num += ({"q":1, "r":2, "b":3, "n":4}.get(a1h8q[4], 0))*64*64
    return num

def num2move(num):
    a1 = posA1(num%64)
    num /= 64
    h8 = posA1(num%64)
    num /= 64
    if num:
        q = {1:"q", 2:"r", 3:"b", 4:"n"}.get(num)
    else:
        q = ""
    return a1 + h8 + q

def liN(npos):
    cdef int fil, col, ft, ct

    fil, col = posFC(npos)
    liM = []
    for fi, ci in ( (+1, +2), (+1, -2), (-1, +2), (-1, -2), (+2, +1), (+2, -1), (-2, +1), (-2, -1) ):
        ft = fil + fi
        ct = col + ci
        if ft < 0 or ft > 7 or ct < 0 or ct > 7:
            continue

        t = FCpos(ft, ct)
        liM.append(t)
    return tuple(liM)


def knightmoves(a, b, no, nv, mx):
    if nv > mx:
        return []
    lia = liN(a)
    if b in lia:
        return [[a, b]]
    lib = liN(b)
    li = []
    for x in lia:
        if x not in no and x in lib:
            li.append([a, x, b])
    if li:
        return li

    li = []

    for x in lia:
        for y in lib:
            if x not in no and y not in no:
                nx = no[:]
                nx.append(x)
                nx.append(y)
                f = knightmoves(x, y, nx, nv + 1, mx)
                if f:
                    li.extend(f)
    if not li:
        return li
    xmin = 9999
    for x in li:
        nx = len(x)
        if nx < xmin:
            xmin = nx
    lidef = []
    for x in li:
        if len(x) == xmin:
            x.insert(0, a)
            x.append(b)
            lidef.append(x)
    return lidef

def liNMinimo(x, y, celdas_ocupadas):
    cdef int nv
    ot = celdas_ocupadas[:]
    ot.extend([x, y])
    nv = 1
    li = knightmoves(x, y, ot, 0, nv)
    while len(li) == 0:
        nv += 1
        li = knightmoves(x, y, ot, 0, nv)
    return li

def xpv2lipv(xpv):
    li = []
    siBlancas = True
    for c in xpv:
        x = ord(c)
        if x >= 58:
            move = posA1(x - 58)
            if siBlancas:
                base = move
            else:
                li.append(base + move)
            siBlancas = not siBlancas
        else:
            c = {50: "q", 51: "r", 52: "b", 53: "n"}.get(x, "")
            li[-1] += c
    return li

def xpv2pv(xpv):
    return " ".join(xpv2lipv(xpv))

def pv2xpv(pv):
    if pv:
        li = pv.split(" ")
        lix = []
        for move in li:
            d = chr(a1Pos(move[:2]) + 58)  # 58 is an arbitrary number, to remain in range 58..122
            h = chr(a1Pos(move[2:4]) + 58)
            c = move[4:]
            if c:
                c = {"q": chr(50), "r": chr(51), "b": chr(52), "n": chr(53)}.get(c.lower(), "")
            lix.append(d + h + c)
        return "".join(lix)
    else:
        return ""

def runFen( fen, depth, ms, level ):
    set_level(level)
    x = playFen(fen, depth, ms)
    set_level(0)
    return x

def setFen(fen):
    fen_board(fen)
    return movegen()

def getFen():
    cdef char fen[100]
    board_fen(fen)
    x = fen
    return x

def getMoves():
    cdef char pv[10]
    cdef int nmoves, x, nbase
    nmoves = numMoves()

    nbase = numBaseMove()
    li = []
    for x in range(nmoves):
        getMove(x+nbase, pv)
        r = pv
        li.append(r)
    return li

def getPGN(desdeA1H8, hastaA1H8, coronacion):
    cdef char san[10]

    if not coronacion:
        coronacion = ""

    num = searchMove( desdeA1H8, hastaA1H8, coronacion )
    if num == -1:
        return None

    toSan(num, san)
    return san

def xpv2pgn(xpv):
    cdef char san[10]
    setFenInicial()
    siW = True
    num = 1
    li = []
    tam = 0
    for pv in xpv2lipv(xpv):
        if siW:
            x = str(num)+"."
            tam += len(x)
            li.append(x)
            num += 1
        siW = not siW

        numMove = searchMove( pv[:2], pv[2:4], pv[4:] )
        if numMove == -1:
            break
        toSan(numMove, san)
        x = str(san)
        li.append(x)
        tam += len(x)
        if tam >= 80:
            li.append("\n")
            tam = 0
        else:
            li.append(" ")
            tam += 1
        make_nummove(numMove)
    return "".join(li)

def isCheck():
    return inCheck()

class InfoMove(object):
    def __init__(self, num):
        cdef char pv[10]
        cdef char info[10]
        cdef char san[10]

        getMove(num, pv)
        getMoveEx(num, info)
        toSan(num, san)

        # info = P a1 h8 q [K|Q|]

        self._castle_K = info[6] == "K"
        self._castle_Q = info[6] == "Q"
        self._ep = info[7]=="E"
        self._pv = pv
        self._san = san

        self._piece = info[0:1]
        self._from = info[1:3]
        self._to = info[3:5]
        self._promotion = info[5:6].strip()
        self._check = "+" in san
        self._mate = "#" in san
        self._capture = "x" in san

    def desde(self):
        return self._from

    def hasta(self):
        return self._to

    def coronacion(self):
        return self._promotion.lower()

    def movimiento(self):
        return self._from+self._to+self._promotion.lower()

    def jaque(self):
        return self._check

    def mate(self):
        return self._mate

    def captura(self):
        return self._capture

    def pieza(self):
        return self._piece

    def isCastleK(self):
        return self._castle_K

    def isCastleQ(self):
        return self._castle_Q

    def isEnPassant(self):
        return self._ep

def getExMoves():
    nmoves = numMoves()

    nbase = numBaseMove()
    li = []
    for x in range(nmoves):
        mv = InfoMove(x + nbase)
        li.append(mv)
    return li

def moveExPV(desde, hasta, coronacion):
    if not coronacion:
        coronacion = ""

    num = searchMove( desde, hasta, coronacion )
    if num == -1:
        return None

    infoMove = InfoMove(num)
    make_nummove(num)

    return infoMove

def movePV(desde, hasta, coronacion):
    if not coronacion:
        coronacion = ""

    num = searchMove( desde, hasta, coronacion )
    if num == -1:
        return False

    make_nummove(num)

    return True

def makeMove(move):
    desde = move[:2]
    hasta = move[2:4]
    coronacion = move[4:]
    num = searchMove( desde, hasta, coronacion )
    if num == -1:
        return False

    make_nummove(num)
    return True

def fen2fenM2(fen):
    sp1 = fen.rfind(" ")
    sp2 = fen.rfind(" ", 0, sp1)
    return fen[:sp2]

def setFenInicial():
    inifen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    setFen(inifen)

def makePV(pv):
    setFenInicial()
    if pv:
        for move in pv.split(" "):
            makeMove(move)
    return getFen()


def getCapturesFEN(fen):
    setFen(fen)
    nmoves = numMoves()
    nbase = numBaseMove()
    li = []
    for x in range(nmoves):
        mv = InfoMove(x + nbase)
        if mv.captura():
            li.append(mv)
    return li

def getCaptures(fen, siMB):
    if not siMB:
        fen = fenOB(fen)
    return getCapturesFEN(fen)

def fenOB(fen):
    li = fen.split(" ")
    li[3] = "-"
    li[1] = "w" if li[1] == "b" else "b"
    return " ".join(li)

def fenTerminado(fen):
    return setFen(fen) == 0


def liK(npos):
    cdef int fil, col, ft, ct
    liM = []
    fil, col = posFC(npos)
    for fi, ci in ( (+1, +1), (+1, -1), (-1, +1), (-1, -1), (+1, 0), (-1, 0), (0, +1), (0, -1) ):
        ft = fil + fi
        ct = col + ci
        if ft < 0 or ft > 7 or ct < 0 or ct > 7:
            continue
        liM.append(FCpos(ft, ct))
    return tuple(liM)


def liBR(npos, fi, ci):
    cdef int fil, col, ft, ct

    fil, col = posFC(npos)
    liM = []
    ft = fil + fi
    ct = col + ci
    while True:
        if ft < 0 or ft > 7 or ct < 0 or ct > 7:
            break

        t = FCpos(ft, ct)
        liM.append(t)
        ft += fi
        ct += ci
    return tuple(liM)


def liN(npos):
    cdef int fil, col, ft, ct

    fil, col = posFC(npos)
    liM = []
    for fi, ci in ( (+1, +2), (+1, -2), (-1, +2), (-1, -2), (+2, +1), (+2, -1), (-2, +1), (-2, -1) ):
        ft = fil + fi
        ct = col + ci
        if ft < 0 or ft > 7 or ct < 0 or ct > 7:
            continue

        t = FCpos(ft, ct)
        liM.append(t)
    return tuple(liM)

def liP(npos, siW):
    cdef int fil, col, ft, ct, inc

    fil, col = posFC(npos)
    liM = []
    liX = []
    if siW:
        filaIni = 1
        salto = +1
    else:
        filaIni = 6
        salto = -1
    sig = FCpos(fil + salto, col)
    liM.append(sig)

    if fil == filaIni:
        sig2 = FCpos(fil + salto * 2, col)
        liM.append(sig2)

    for inc in ( +1, -1 ):
        ft = fil + salto
        ct = col + inc
        if not (ft < 0 or ft > 7 or ct < 0 or ct > 7):
            t = FCpos(ft, ct)
            liX.append(t)

    return tuple(liM), tuple(liX)

dicK = {}
for i in range(64):
    dicK[i] = liK(i)

dicQ = {}
for i in range(64):
    li = []
    for f_i, c_i in ( (1, 1), (1, -1), (-1, 1), (-1, -1), (1, 0), (-1, 0), (0, 1), (0, -1) ):
        lin = liBR(i, f_i, c_i)
        if lin:
            li.append(lin)
    dicQ[i] = tuple(li)

dicB = {}
for i in range(64):
    li = []
    for f_i, c_i in ( (1, 1), (1, -1), (-1, 1), (-1, -1) ):
        lin = liBR(i, f_i, c_i)
        if lin:
            li.append(lin)
    dicB[i] = tuple(li)

dicR = {}
for i in range(64):
    li = []
    for f_i, c_i in ( (1, 0), (-1, 0), (0, 1), (0, -1) ):
        lin = liBR(i, f_i, c_i)
        if lin:
            li.append(lin)
    dicR[i] = tuple(li)

dicN = {}
for i in range(64):
    dicN[i] = liN(i)

dicPW = {}
for i in range(8, 56):
    dicPW[i] = liP(i, True)

dicPB = {}
for i in range(8, 56):
    dicPB[i] = liP(i, False)
