import random

import LCEngine4 as LCEngine
import chess
import chess.syzygy


class T4:
    def __init__(self):
        self.tb = chess.syzygy.Tablebases("./IntFiles/syzygy")

    def better_moves(self, fen, move):
        dic = self.checkFen(fen)

        def x_wdl(xwdl):
            li_xmin_pv = []
            xmin_dtz = None
            for pv, (wdl, dtz) in dic.iteritems():
                if wdl == xwdl:
                    ok = False
                    if not li_xmin_pv:
                        ok = True
                    else:
                        if xmin_dtz >= dtz:
                            ok = True
                    if ok:
                        if dtz == xmin_dtz:
                            li_xmin_pv.append(pv)
                        else:
                            li_xmin_pv = [pv]
                            xmin_dtz = dtz
            return li_xmin_pv, xmin_dtz

        wdl, dtz = dic.get(move, (-2, 0))
        for x in (2, 1, 0, -1, -2):
            if x >= wdl:
                li, xdtz = x_wdl(x)
                if li:
                    if x > wdl:
                        return li
                    if x < 0 and xdtz < dtz:
                        return li
                    if x >= 0 and xdtz >= dtz:
                        return li
        return []

    def best_move(self, fen):
        li = self.better_moves(fen, None)
        if li:
            return random.choice(li)
        else:
            return None

    def wdl_dtz(self, fen):
        board = chess.Board(fen)
        wdl = self.tb.probe_wdl(board)
        board = chess.Board(fen)
        dtz = self.tb.probe_dtz(board)
        return wdl, dtz

    def checkFen(self, fen):
        LCEngine.setFen(fen)
        liMoves = LCEngine.getMoves()
        dic = {}
        for xpv in liMoves:
            pv = xpv[1:]
            LCEngine.setFen(fen)
            LCEngine.movePV(pv[:2], pv[2:4], pv[4:])
            xfen = LCEngine.getFen()
            wdl, dtz = self.wdl_dtz(xfen)
            if wdl is not None and dtz is not None:
                dic[pv] = -wdl, -dtz
        return dic

    def wd_move(self, fen, move):
        LCEngine.setFen(fen)
        liMoves = LCEngine.getMoves()
        liMoves = map(lambda x: x[1:], liMoves)

        if move in liMoves:
            LCEngine.movePV(move[:2], move[2:4], move[4:])
            xfen = LCEngine.getFen()
            wdl, dtz = self.wdl_dtz(xfen)
        else:
            wdl, dtz = 2, 0

        return -wdl, -dtz

    def close(self):
        if self.tb:
            self.tb.close()
            self.tb = None
