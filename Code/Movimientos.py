def posFC(pos):
    return pos / 8, pos % 8

def FCpos(f, c):
    return f * 8 + c

def posA1(pos):
    return chr(pos % 8 + 97) + chr(pos / 8 + 49)

def a1Pos(a1):
    f = ord(a1[1]) - 49
    c = ord(a1[0]) - 97
    return f * 8 + c

def liK(npos):
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

def knightmoves(a, b, no, nv, mx):
    if nv > mx:
        return []
    lia = dicN[a]
    if b in lia:
        return [[a, b]]
    lib = dicN[b]
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
    ot = celdas_ocupadas[:]
    ot.extend([x, y])
    nv = 1
    li = knightmoves(x, y, ot, 0, nv)
    while len(li) == 0:
        nv += 1
        li = knightmoves(x, y, ot, 0, nv)
    return li

def xpv2pv(xpv):
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
    return " ".join(li)

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

