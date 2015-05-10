import atexit
import os
import struct

import Code.VarGen as VarGen
from Code.Movimientos import posA1, a1Pos, pv2xpv, xpv2pv
import Code.Util as Util
import Code.Partida as Partida
import Code.SQL.Base as Base
import Code.ControlPosicion as ControlPosicion
import Code.PGNreader as PGNreader

class RegSTAT:
    dicNP = {0: "", 1: "q", 2: "r", 3: "b", 4: "n"}
    dicPN = {"": 0, "q": 1, "r": 2, "b": 3, "n": 4}
    sizeSTAT = 26
    stSTAT = "<HIIIIII"

    def __init__(self, cfile, offset):
        self.file = cfile
        self.offset = offset

    def read(self):
        self.file.seek(self.offset, 0)

        xmove, self.brother, self.child, self.white, self.draw, self.black, self.other = \
            struct.unpack(self.stSTAT, self.file.read(self.sizeSTAT))

        self.move = self.num2move(xmove)

    def __str__(self):
        return "%s b%d c%d w%d d%d b%d o%d" % (
            self.move, self.brother, self.child, self.white, self.draw, self.black, self.other)

    def write(self, siMas=False):
        if siMas:
            self.file.seek(0, 2)
        else:
            self.file.seek(self.offset, 0)
        self.file.write(struct.pack(self.stSTAT, self.move2num(self.move), self.brother,
                                    self.child, self.white, self.draw, self.black, self.other))

    def num2move(self, num):
        d = num % 64
        num /= 64
        h = num % 64
        c = num / 64
        return posA1(d) + posA1(h) + self.dicNP.get(c, "")

    def move2num(self, move):
        if not move.strip():
            raise

        d = a1Pos(move[:2])
        h = a1Pos(move[2:4])
        c = self.dicPN.get(move[4:], 0)

        return d + 64 * ( h + 64 * c )

    def new(self, move):
        self.move, self.brother, self.child, self.white, self.draw, self.black, self.other = move, 0, 0, 0, 0, 0, 0

    def sum(self, white, draw, black, other):
        self.white += white
        self.draw += draw
        self.black += black
        self.other += other

    def clone(self):
        work = RegSTAT(self.file, self.offset)
        work.move, work.brother, work.child, work.white, work.draw, work.black, work.other = \
            self.move, self.brother, self.child, self.white, self.draw, self.black, self.other
        return work

    def nextByte(self):
        self.file.seek(0, 2)
        return self.file.tell()

    def walkchild(self, move, siCrear):
        if self.child:
            hijo = self.clone()
            hijo.offset = self.child
            while True:
                hijo.read()
                if hijo.move == move:
                    return hijo
                if hijo.brother:
                    hijo.offset = hijo.brother
                else:
                    if siCrear:
                        nextByte = self.nextByte()
                        hijo.brother = nextByte
                        hijo.write()
                        # Cambiamos al hermano
                        hijo.offset = nextByte
                        hijo.new(move)
                        hijo.write(True)
                        return hijo
                    else:
                        return None
        elif siCrear:
            nextByte = self.nextByte()
            self.child = nextByte
            self.write()
            hijo = self.clone()
            hijo.offset = nextByte
            hijo.new(move)
            hijo.write(True)
            return hijo
        else:
            return None

    def children(self):
        liChildren = []
        if self.child:
            work = self.clone()
            work.offset = work.child
            work.read()
            liChildren.append(work.clone())

            while work.brother:
                work.offset = work.brother
                work.read()
                liChildren.append(work.clone())
        return liChildren

    def maxBloq(self):
        resp = ""
        if self.white >= self.draw and self.white >= self.black:
            resp += "w"
        if self.black >= self.draw and self.black >= self.white:
            resp += "b"
        if self.draw >= self.white and self.draw >= self.black:
            resp += "d"
        return resp

    def games(self):
        return self.white + self.draw + self.black + self.other

    def gamesParcial(self, siWhite, siDraw):
        t = self.white if siWhite else self.black
        if siDraw:
            t += self.draw
        return t

class TreeSTAT:
    regSize = 26
    defaultDepth = 30

    def __init__(self, nomFichero, depth=None):
        self.nomFichero = nomFichero

        tf = Util.tamFichero(nomFichero)
        if (tf < self.regSize) or ((tf - 1) % self.regSize > 0):
            self.file = self.creaFichero(depth)
        else:
            self.file = self.abreFichero()
        self.depth = self.readDepth()
        self.root = RegSTAT(self.file, 1)
        self.root.read()

        atexit.register(self.close)

    def readDepth(self):
        self.file.seek(0, 0)
        depth = ord(self.file.read(1))
        return depth

    def writeDepth(self, depth):
        self.depth = depth
        self.file.seek(0, 0)
        self.file.write(chr(depth))

    def close(self):
        if self.file:
            self.file.close()
            self.file = None

    def reset(self, depth=None):
        self.close()
        Util.borraFichero(self.nomFichero)
        self.file = self.creaFichero(depth)
        self.root = RegSTAT(self.file, 1)
        self.root.read()

    def creaFichero(self, depth):
        f = open(self.nomFichero, "w+b")
        if depth is None:
            depth = self.defaultDepth
        self.depth = depth
        f.seek(0, 0)
        f.write(chr(depth))
        wr = RegSTAT(f, 1)
        wr.new("a1a1")
        wr.write(True)
        return f

    def abreFichero(self):
        f = open(self.nomFichero, "r+b")
        return f

    def append(self, pv, result, r=+1):
        w = d = b = o = 0
        if result == "1-0":
            w = r
        elif result == "0-1":
            b = r
        elif result == "1/2-1/2":
            d = r
        else:
            o = r

        self.root.sum(w, d, b, o)
        self.root.write()

        padre = self.root
        for n, move in enumerate(pv.split(" ")):
            if n >= self.depth:
                break

            hijo = padre.walkchild(move, True)
            hijo.sum(w, d, b, o)
            hijo.write()
            padre = hijo

    def appendColor(self, pv, result, siWhite, r=+1):
        w = d = b = o = 0
        if result == "1-0":
            w = r
        elif result == "0-1":
            b = r
        elif result == "1/2-1/2":
            d = r
        else:
            o = r

        self.root.sum(w, d, b, o)
        self.root.write()

        padre = self.root
        for n, move in enumerate(pv.split(" ")):
            if n >= self.depth:
                break

            hijo = padre.walkchild(move, True)
            if (n % 2 == 0 and siWhite) or (n % 2 == 1 and not siWhite):
                hijo.sum(w, d, b, o)
            else:
                hijo.sum(0, 0, 0, r)
            hijo.write()
            padre = hijo

    def flistAllpvs(self, maxDepth, minGames, siWhite, siDraw, pvBase):

        fich = VarGen.configuracion.ficheroTemporal("tmp")
        f = open(fich, "wb")

        stFenM2 = set()

        def mejor(work, pvPrevio, depth, cp):
            if depth > maxDepth:
                return
            if work.games() >= minGames:
                liChildren = work.children()
                if not liChildren:
                    return
                mv = liChildren[0]
                mx = mv.gamesParcial(siWhite, siDraw)
                li = []
                for uno in liChildren:
                    unomx = uno.gamesParcial(siWhite, siDraw)
                    if unomx > mx:
                        mx = unomx
                        li = [uno]
                    elif unomx == mx:
                        li.append(uno)
                for uno in li:
                    wm = uno.move
                    pv = (pvPrevio + " " + uno.move).strip()
                    if pv != "a1a1":
                        cpN = cp.copia()
                        cpN.moverPV(wm)
                        f.write("%s|%s|%s\n" % (pv2xpv(pv), wm, cpN.fen() ))
                        todos(uno, pv, depth + 1, cpN)

        def todos(work, pvPrevio, depth, cp):
            if depth > maxDepth:
                return
            if work.games() >= minGames:
                liChildren = work.children()
                if not liChildren:
                    return
                for uno in liChildren:
                    if uno.games() >= minGames:
                        wm = uno.move
                        pv = (pvPrevio + " " + uno.move).strip()
                        if pv != "a1a1":
                            cpN = cp.copia()
                            cpN.moverPV(wm)
                            f.write("%s|%s|%s\n" % (pv2xpv(pv), wm, cpN.fen() ))
                            fm2 = cpN.fenM2()
                            if fm2 not in stFenM2:  # Para que no se repitan los movimientos de los transpositions
                                stFenM2.add(fm2)
                                mejor(uno, pv, depth + 1, cpN)

        def inicia():
            cp = ControlPosicion.ControlPosicion()
            cp.posInicial()
            work = self.root
            pvActual = ""
            for pv in pvBase.split(" "):
                cp.moverPV(pv)
                pvActual += " " + pv
                f.write("%s|%s|%s\n" % (pv2xpv(pvActual.strip()), pv, cp.fen() ))
                liChildren = work.children()
                work = None
                for uno in liChildren:
                    if uno.move == pv:
                        work = uno
                        break
                if work is None:
                    work = self.root
                    break
            return cp, work

        cp, work = inicia()

        if siWhite and cp.siBlancas:
            mejor(work, pvBase, 0, cp)
        else:
            todos(work, pvBase, 0, cp)
        f.close()
        return fich

class DBgames:
    def __init__(self, nomFichero, segundosBuffer=0.8):
        self.nomFichero = Util.dirRelativo(nomFichero)
        self.liCamposBase = ["EVENT", "SITE", "DATE", "WHITE", "BLACK", "RESULT", "ECO", "WHITEELO", "BLACKELO",
                             "PLIES"]
        self.liCamposWork = ["XPV", "PGN"]

        self.segundosBuffer = segundosBuffer

        self.db = Base.DBBase(nomFichero)
        self.tabla = "games"
        if not self.db.existeTabla(self.tabla):
            self.creaTabla()

        liCampos = []
        liCampos.extend(self.liCamposWork)
        liCampos.extend(self.liCamposBase)
        self.dbf = self.db.dbfCache(self.tabla, ",".join(liCampos))
        self.liOrden = []
        atexit.register(self.close)

        self.dbSTAT = TreeSTAT(self.nomFichero + "-stat")
        self.dbSTATbase = self.dbSTAT
        self.dbSTATplayer = None

    def rotulo(self):
        rotulo = os.path.basename(self.nomFichero)[:-4]
        if rotulo == "Initial Database Games":
            rotulo = _("Initial Database Games")
        return rotulo

    def depthStat(self):
        return self.dbSTAT.depth

    def flistAllpvs(self, maxDepth, minGames, siWhite, siDraw, pvBase):
        return self.dbSTAT.flistAllpvs(maxDepth, minGames, siWhite, siDraw, pvBase)

    def close(self):
        if self.dbf:
            self.dbf.cerrar()
            self.dbf = None
        if self.db:
            self.db.cerrar()
            self.db = None
        if self.dbSTATbase:
            self.dbSTATbase.close()
            self.dbSTATbase = None
        if self.dbSTATplayer:
            self.dbSTATplayer.close()
            self.dbSTATplayer = None

    def reccount(self):
        return self.dbf.reccount()

    def reccountTotal(self):
        return self.dbSTAT.root.games()

    def creaTabla(self):
        tb = Base.TablaBase(self.tabla)
        tb.nuevoCampo("XPV", "VARCHAR", notNull=True, primaryKey=True)
        for campo in self.liCamposBase:
            tb.nuevoCampo(campo, "VARCHAR")
        tb.nuevoCampo("PGN", "BLOB")
        self.db.generarTabla(tb)

    def goto(self, num):
        self.dbf.goto(num)

    def field(self, nfila, name):
        try:
            self.dbf.goto(nfila)
            return getattr(self.dbf.reg, name)
        except:
            return ""

    def damePV(self, fila):
        xpv = self.field(fila, "XPV")
        return xpv2pv(xpv)

    def filterPV(self, pv, condicionAdicional=None):
        xpv = pv2xpv(pv)
        condicion = 'XPV LIKE "%s%%"' % xpv if xpv else ""
        if self.dbSTATplayer and self.dbSTAT == self.dbSTATplayer:
            playerBusq, siSta, siEnd, playerORI = self.player
            playerORI = playerORI.upper().strip().replace("*", "%")
            if siSta or siEnd:
                condicionAdicional = 'upper(WHITE) like "%s" or upper(BLACK) like "%s"' % (playerORI, playerORI)
            else:
                condicionAdicional = 'upper(WHITE) == "%s" or upper(BLACK) == "%s"' % (playerORI, playerORI)
        if condicionAdicional:
            if condicion:
                condicion += " AND (%s)" % condicionAdicional
            else:
                condicion = condicionAdicional
        self.dbf.ponCondicion(condicion)
        self.dbf.leerBuffer(segundos=self.segundosBuffer)
        self.dbf.gotop()

    def ponOrden(self, liOrden):
        li = ["%s %s" % (campo, tipo) for campo, tipo in liOrden]
        orden = ",".join(li)
        self.dbf.ponOrden(orden)
        self.dbf.leerBuffer(segundos=self.segundosBuffer)
        self.dbf.gotop()
        self.liOrden = liOrden

    def dameOrden(self):
        return self.liOrden

    def appendSTAT(self, pv, res, r, white, black):
        self.dbSTATbase.append(pv, res, r)
        if self.dbSTATplayer:
            if self.esPlayer(white):
                self.dbSTATplayer.appendColor(pv, res, True, r)
            elif self.esPlayer(black):
                self.dbSTATplayer.appendColor(pv, res, False, r)

    def cambiarUno(self, recno, nuevoPGN, pvNue, dicS_PGN):
        siNuevo = recno is None

        if not siNuevo:
            self.dbf.goto(recno)
            reg = self.dbf.reg
            pvAnt = xpv2pv(reg.XPV)
            resAnt = reg.RESULT
        resNue = dicS_PGN.get("Result", "*")

        br = self.dbf.baseRegistro()
        br.XPV = pv2xpv(pvNue)
        br.EVENT = dicS_PGN.get("Event", "")
        br.SITE = dicS_PGN.get("Site", "")
        br.DATE = dicS_PGN.get("Date", "")
        br.WHITE = dicS_PGN.get("White", "")
        br.BLACK = dicS_PGN.get("Black", "")
        br.RESULT = resNue
        br.PLIES = "%3d" % (pvNue.strip().count(" ") + 1,)
        br.ECO = dicS_PGN.get("ECO", "")
        br.WHITEELO = dicS_PGN.get("WhiteElo", "")
        br.BLACKELO = dicS_PGN.get("BlackElo", "")
        br.PGN = Util.var2blob(nuevoPGN)

        siRepetido = False
        if siNuevo:
            try:
                self.dbf.insertar(br, okCommit=True, okCursorClose=True)
                self.appendSTAT(pvNue, resNue, +1, br.WHITE, br.BLACK)
            except:
                siRepetido = True
        else:
            try:
                self.dbf.modificarReg(recno, br)
                if pvAnt != pvNue or resAnt != resNue:
                    self.appendSTAT(pvAnt, resAnt, -1, reg.WHITE, reg.BLACK)
                    self.appendSTAT(pvNue, resNue, +1, br.WHITE, br.BLACK)
            except:
                siRepetido = True

        return not siRepetido

    def borrarLista(self, lista):
        for recno in lista:
            self.dbf.goto(recno)
            reg = self.dbf.reg
            pv = xpv2pv(reg.XPV)
            result = reg.RESULT
            self.appendSTAT(pv, result, -1, reg.WHITE, reg.BLACK)

        self.dbf.borrarLista(lista)
        self.dbf.leerBuffer(segundos=self.segundosBuffer)

    def leerMasRegistros(self):
        return self.dbf.leerMasBuffer(segundos=self.segundosBuffer)

    def siFaltanRegistrosPorLeer(self):
        return self.dbf.siBufferPendiente

    def pvStats(self, pv):
        work = self.dbSTAT.root
        if pv:
            for move in pv.split(" "):
                work = work.walkchild(move, False)
                if not work:
                    break
        return work

    def getSummary(self, pvBase, dicAnalisis):
        root = self.dbSTAT.root
        liMoves = []
        siBlancas = True

        if pvBase is None:
            w, d, b, o = root.white, root.draw, root.black, root.other
            tt = w + d + b + o
            dic = {}
            dic["pvmove"] = ""
            dic["pv"] = ""
            dic["analisis"] = None
            dic["games"] = tt
            dic["white"] = w
            dic["draw"] = d
            dic["black"] = b
            dic["other"] = o
            dic["pwhite"] = w * 100.0 / tt if tt else 0.0
            dic["pdraw"] = d * 100.0 / tt if tt else 0.0
            dic["pblack"] = b * 100.0 / tt if tt else 0.0
            dic["pother"] = o * 100.0 / tt if tt else 0.0
            dic["move"] = _("Total")
            liMoves.append(dic)

        else:
            work = root
            if pvBase:
                li = pvBase.split(" ")
                siBlancas = len(li) % 2 == 0
                for move in li:
                    work = work.walkchild(move, False)
                    if not work:
                        return []

            liBrothers = work.children()
            tt = 0

            lipvmove = []
            for st in liBrothers:
                w, d, b, o = st.white, st.draw, st.black, st.other
                t = w + d + b + o
                # if t == 0:
                # continue

                dic = {}
                pvmove = st.move
                pv = pvBase + " " + pvmove
                pv = pv.strip()
                lipvmove.append(pvmove)
                dic["pvmove"] = pvmove
                dic["pv"] = pv
                dic["analisis"] = dicAnalisis.get(pvmove, None)
                dic["games"] = t
                tt += t
                dic["white"] = w
                dic["draw"] = d
                dic["black"] = b
                dic["other"] = o
                dic["pwhite"] = w * 100.0 / t if t else 0.0
                dic["pdraw"] = d * 100.0 / t if t else 0.0
                dic["pblack"] = b * 100.0 / t if t else 0.0
                dic["pother"] = o * 100.0 / t if t else 0.0

                liMoves.append(dic)

            for pvmove in dicAnalisis:
                if pvmove not in lipvmove:
                    dic = {}
                    pv = pvBase + " " + pvmove
                    pv = pv.strip()
                    dic["pvmove"] = pvmove
                    dic["pv"] = pv
                    dic["analisis"] = dicAnalisis[pvmove]
                    dic["games"] = 0
                    dic["white"] = 0
                    dic["draw"] = 0
                    dic["black"] = 0
                    dic["other"] = 0
                    dic["pwhite"] = 0.00
                    dic["pdraw"] = 0.00
                    dic["pblack"] = 0.00
                    dic["pother"] = 0.00

                    liMoves.append(dic)

            liMoves = sorted(liMoves, key=lambda dic: -dic["games"])

        for dic in liMoves:
            dic["pgames"] = dic["games"] * 100.0 / tt if tt else 0.0
            dic["pdrawwhite"] = dic["pwhite"] + dic["pdraw"]
            dic["pdrawblack"] = dic["pblack"] + dic["pdraw"]
            if siBlancas:
                dic["win"] = dic["white"]
                dic["lost"] = dic["black"]
                dic["pwin"] = dic["pwhite"]
                dic["plost"] = dic["pblack"]
                dic["pdrawwin"] = dic["pdrawwhite"]
                dic["pdrawlost"] = dic["pdrawblack"]
            else:
                dic["lost"] = dic["white"]
                dic["win"] = dic["black"]
                dic["pdrawlost"] = dic["pdrawwhite"]
                dic["pdrawwin"] = dic["pdrawblack"]
                dic["plost"] = dic["pwhite"]
                dic["pwin"] = dic["pblack"]

            pvmove = dic["pvmove"]
            if pvmove:
                pv = dic["pv"]
                p = Partida.Partida()
                p.leerPV(pv)
                if p.numJugadas():
                    jg = p.liJugadas[-1]
                    dic["move"] = jg.etiquetaSP().replace(" ", "")
                else:
                    dic["move"] = pvmove
                dic["partida"] = p

        return liMoves

    def recrearSTAT(self, dispatch, depth):
        self.dbSTAT.reset(depth)
        recno = 0
        self.filterPV("")
        reccount = self.dbf.reccount()
        dispatch(0, reccount)
        if reccount:
            while True:
                self.dbf.goto(recno)
                pv = xpv2pv(self.dbf.reg.XPV)
                result = self.dbf.reg.RESULT
                self.dbSTAT.append(pv, result)

                if recno % 100 == 0:
                    resp = dispatch(recno, reccount)
                    if not resp:
                        return

                recno += 1
                if recno >= reccount:
                    if self.dbf.siBufferPendiente:
                        self.dbf.leerMasBuffer(chunk=3000)
                        reccount = self.dbf.reccount()
                        if recno >= reccount:
                            break
                    else:
                        break

    def esPlayer(self, quien):
        playerBusq, siSta, siEnd, playerORI = self.player
        quien = quien.strip().upper()
        if siEnd and siSta:
            return playerBusq in quien
        elif siEnd:
            return quien.endswith(playerBusq)
        elif siSta:
            return quien.startswith(playerBusq)
        else:
            return quien == playerBusq

    def recrearSTATplayer(self, dispatch, depth, player):
        self.dbSTAT = TreeSTAT(self.nomFichero + "-stat-player")
        self.dbSTATplayer = self.dbSTAT
        player = player.strip()
        siEnd = player.startswith("*")
        siSta = player.endswith("*")

        self.player = (player.strip().strip("*").upper(), siSta, siEnd, player)

        self.dbSTAT.reset(depth)
        recno = 0
        self.filterPV("")
        reccount = self.dbf.reccount()
        dispatch(0, reccount)
        while True:
            self.dbf.goto(recno)
            reg = self.dbf.reg
            pv = xpv2pv(reg.XPV)
            result = reg.RESULT
            if self.esPlayer(reg.WHITE):
                self.dbSTAT.appendColor(pv, result, True)
            elif self.esPlayer(reg.BLACK):
                self.dbSTAT.appendColor(pv, result, False)

            if recno % 100 == 0:
                resp = dispatch(recno, reccount)
                if not resp:
                    return

            recno += 1
            if recno >= reccount:
                if self.dbf.siBufferPendiente:
                    self.dbf.leerMasBuffer(chunk=3000)
                    reccount = self.dbf.reccount()
                    if recno >= reccount:
                        break
                else:
                    break

    def append(self, pv, event, site, date, white, black, result, eco, whiteelo, blackelo, pgn, okCommit):
        br = self.dbf.baseRegistro()
        br.XPV = pv2xpv(pv)
        br.EVENT = event
        br.SITE = site
        br.DATE = date
        br.WHITE = white
        br.BLACK = black
        br.RESULT = result
        br.PLIES = "%3d" % (pv.strip().count(" ") + 1,)
        br.ECO = eco
        br.WHITEELO = whiteelo
        br.BLACKELO = blackelo
        br.PGN = Util.var2blob(pgn)
        siRepetido = False
        try:
            self.dbf.insertar(br, okCommit=okCommit, okCursorClose=okCommit)
            self.appendSTAT(pv, result, +1, white, black)
        except:
            siRepetido = True
        return not siRepetido

    def __getitem__(self, num):
        self.dbf.goto(num)
        reg = self.dbf.reg
        return reg

    def leePGNrecno(self, recno):
        self.dbf.goto(recno)
        v = self.dbf.reg.PGN
        pgn = Util.blob2var(v) if v else None
        return pgn

    def leerPGN(self, fichero, dlTmp):
        erroneos = duplicados = importados = 0

        # 1.File pgn -> temporal clean
        for n, g in enumerate(PGNreader.readGames(fichero)):
            if n == 10000:
                break
            if n % 100 == 0:
                if not dlTmp.actualiza(n, erroneos, duplicados, importados):
                    break
            if g.erroneo:
                erroneos += 1
                continue
            pv = g.pv()
            if not pv:
                erroneos += 1
                continue
            pgn = g.pgn

            get = g.labels.get
            if get("FEN", None):
                erroneos += 1
                continue

            def get(label):
                if label in g.labels:
                    return g.labels[label]
                return ""

            event = get("EVENT")
            site = get("SITE")
            date = get("DATE")
            white = get("WHITE")
            black = get("BLACK")
            result = get("RESULT")
            eco = get("ECO")
            whiteelo = get("WHITEELO")
            blackelo = get("BLACKELO")

            if self.append(pv, event, site, date, white, black, result, eco, whiteelo, blackelo, pgn, False):
                importados += 1
            else:
                duplicados += 1

        dlTmp.actualiza(n + 1, erroneos, duplicados, importados)

        dlTmp.ponSaving()
        self.dbf.commit()
        dlTmp.ponContinuar()

        return

    def blankPGN(self):
        hoy = Util.hoy()
        return '[Date "%d.%02d.%02d"]\n\n*' % (hoy.year, hoy.month, hoy.day)

