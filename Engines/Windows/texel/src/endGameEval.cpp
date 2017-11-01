/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * endGameEval.cpp
 *
 *  Created on: Dec 26, 2014
 *      Author: petero
 */

#include "endGameEval.hpp"
#include "position.hpp"
#include "piece.hpp"
#include "parameters.hpp"

const int EndGameEval::distToH1A8[8][8] = { { 0, 1, 2, 3, 4, 5, 6, 7 },
                                            { 1, 2, 3, 4, 5, 6, 7, 6 },
                                            { 2, 3, 4, 5, 6, 7, 6, 5 },
                                            { 3, 4, 5, 6, 7, 6, 5, 4 },
                                            { 4, 5, 6, 7, 6, 5, 4, 3 },
                                            { 5, 6, 7, 6, 5, 4, 3, 2 },
                                            { 6, 7, 6, 5, 4, 3, 2, 1 },
                                            { 7, 6, 5, 4, 3, 2, 1, 0 } };

const int EndGameEval::winKingTable[64] = {
    0,   4,  10,  10,  10,  10,   4,   0,
    4,  15,  19,  20,  20,  19,  15,   4,
   10,  19,  25,  25,  25,  25,  19,  10,
   10,  20,  25,  25,  25,  25,  20,  10,
   10,  20,  25,  25,  25,  25,  20,  10,
   10,  19,  25,  25,  25,  25,  19,  10,
    4,  15,  19,  20,  20,  19,  15,   4,
    0,   4,  10,  10,  10,  10,   4,   0
};


template int EndGameEval::endGameEval<false>(const Position&, U64, int);
template int EndGameEval::endGameEval<true>(const Position&, U64, int);

/** Implements special knowledge for some endgame situations. */
template <bool doEval>
int
EndGameEval::endGameEval(const Position& pos, U64 passedPawns, int oldScore) {
    int score = oldScore;
    const int wMtrlPawns = pos.wMtrlPawns();
    const int bMtrlPawns = pos.bMtrlPawns();
    const int wMtrlNoPawns = pos.wMtrl() - wMtrlPawns;
    const int bMtrlNoPawns = pos.bMtrl() - bMtrlPawns;

    // Handle special endgames
    using MI = MatId;
    switch (pos.materialId()) {
    case 0:
    case MI::WN: case MI::BN: case MI::WB: case MI::BB:
    case MI::WN + MI::BN: case MI::WN + MI::BB:
    case MI::WB + MI::BN: case MI::WB + MI::BB:
        if (!doEval) return 1;
        return 0; // King + minor piece vs king + minor piece is a draw
    case MI::WQ + MI::BP: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int wq = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WQUEEN));
        int bk = pos.getKingSq(false);
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        return kqkpEval(wk, wq, bk, bp, pos.isWhiteMove(), score);
    }
    case MI::BQ + MI::WP: {
        if (!doEval) return 1;
        int bk = pos.getKingSq(false);
        int bq = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BQUEEN));
        int wk = pos.getKingSq(true);
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        return -kqkpEval(63-bk, 63-bq, 63-wk, 63-wp, !pos.isWhiteMove(), -score);
    }
    case MI::WQ: {
        if (!doEval) return 1;
        if (!pos.isWhiteMove() &&
            (pos.pieceTypeBB(Piece::BKING) & BitBoard::maskCorners) &&
            (pos.pieceTypeBB(Piece::WQUEEN) & BitBoard::sqMask(C2,B3,F2,G3,B6,C7,G6,F7)) &&
            (BitBoard::getTaxiDistance(pos.getKingSq(false),
                                       BitBoard::firstSquare(pos.pieceTypeBB(Piece::WQUEEN))) == 3))
            return 0;
        break;
    }
    case MI::BQ: {
        if (!doEval) return 1;
        if (pos.isWhiteMove() &&
            (pos.pieceTypeBB(Piece::WKING) & BitBoard::maskCorners) &&
            (pos.pieceTypeBB(Piece::BQUEEN) & BitBoard::sqMask(C2,B3,F2,G3,B6,C7,G6,F7)) &&
            (BitBoard::getTaxiDistance(pos.getKingSq(true),
                                       BitBoard::firstSquare(pos.pieceTypeBB(Piece::BQUEEN))) == 3))
            return 0;
        break;
    }
    case MI::WR + MI::BP: {
        if (!doEval) return 1;
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        return krkpEval(pos.getKingSq(true), pos.getKingSq(false),
                        bp, pos.isWhiteMove(), score);
    }
    case MI::BR + MI::WP: {
        if (!doEval) return 1;
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        return -krkpEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                         63-wp, !pos.isWhiteMove(), -score);
    }
    case MI::WR + MI::BB: {
        if (!doEval) return 1;
        score /= 8;
        const int kSq = pos.getKingSq(false);
        const int x = Position::getX(kSq);
        const int y = Position::getY(kSq);
        if ((pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq) != 0)
            score += (7 - distToH1A8[7-y][7-x]) * 7;
        else
            score += (7 - distToH1A8[7-y][x]) * 7;
        return score;
    }
    case MI::BR + MI::WB: {
        if (!doEval) return 1;
        score /= 8;
        const int kSq = pos.getKingSq(true);
        const int x = Position::getX(kSq);
        const int y = Position::getY(kSq);
        if ((pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq) != 0)
            score -= (7 - distToH1A8[7-y][7-x]) * 7;
        else
            score -= (7 - distToH1A8[7-y][x]) * 7;
        return score;
    }
    case MI::WR + MI::WP + MI::BR: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        int wr = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WROOK));
        int br = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BROOK));
        return krpkrEval(wk, bk, wp, wr, br, pos.isWhiteMove());
    }
    case MI::BR + MI::BP + MI::WR: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        int wr = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WROOK));
        int br = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BROOK));
        return -krpkrEval(63-bk, 63-wk, 63-bp, 63-br, 63-wr, !pos.isWhiteMove());
    }
    case MI::WR + MI::WP + MI::BR + MI::BP: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        int wr = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WROOK));
        int br = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BROOK));
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        return krpkrpEval(wk, bk, wp, wr, br, bp, pos.isWhiteMove(), score);
    }
    case MI::WN * 2:
    case MI::BN * 2:
        if (!doEval) return 1;
        return 0; // KNNK is a draw
    case MI::WN + MI::WB: {
        if (!doEval) return 1;
        bool darkBishop = (pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq) != 0;
        return kbnkEval(pos.getKingSq(true), pos.getKingSq(false), darkBishop);
    }
    case MI::BN + MI::BB: {
        if (!doEval) return 1;
        bool darkBishop = (pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq) != 0;
        return -kbnkEval(63-pos.getKingSq(false), 63-pos.getKingSq(true), darkBishop);
    }
    case MI::WP: {
        if (!doEval) return 1;
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        return kpkEval(pos.getKingSq(true), pos.getKingSq(false),
                       wp, pos.isWhiteMove());
    }
    case MI::BP: {
        if (!doEval) return 1;
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        return -kpkEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                        63-bp, !pos.isWhiteMove());
    }
    case MI::WP + MI::BP: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        if (kpkpEval(wk, bk, wp, bp, score))
            return score;
        break;
    }
    case MI::WB + MI::WP + MI::BB: {
        if (!doEval) return 1;
        int wb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WBISHOP));
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        int bb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BBISHOP));
        return kbpkbEval(pos.getKingSq(true), wb, wp, pos.getKingSq(false), bb, score);
    }
    case MI::BB + MI::BP + MI::WB: {
        if (!doEval) return 1;
        int bb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BBISHOP));
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        int wb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WBISHOP));
        return -kbpkbEval(63-pos.getKingSq(false), 63-bb, 63-bp, 63-pos.getKingSq(true), 63-wb, -score);
    }
    case MI::WB + MI::WP + MI::BN: {
        if (!doEval) return 1;
        int wb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WBISHOP));
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        int bn = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BKNIGHT));
        return kbpknEval(pos.getKingSq(true), wb, wp, pos.getKingSq(false), bn, score);
    }
    case MI::BB + MI::BP + MI::WN: {
        if (!doEval) return 1;
        int bb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BBISHOP));
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        int wn = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WKNIGHT));
        return -kbpknEval(63-pos.getKingSq(false), 63-bb, 63-bp, 63-pos.getKingSq(true), 63-wn, -score);
    }
    case MI::WN + MI::WP + MI::BB: {
        if (!doEval) return 1;
        int wn = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WKNIGHT));
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        int bb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BBISHOP));
        return knpkbEval(pos.getKingSq(true), wn, wp, pos.getKingSq(false), bb,
                         score, pos.isWhiteMove());
    }
    case MI::BN + MI::BP + MI::WB: {
        if (!doEval) return 1;
        int bn = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BKNIGHT));
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        int wb = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WBISHOP));
        return -knpkbEval(63-pos.getKingSq(false), 63-bn, 63-bp, 63-pos.getKingSq(true), 63-wb,
                          -score, !pos.isWhiteMove());
    }
    case MI::WN + MI::WP: {
        if (!doEval) return 1;
        int wn = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WKNIGHT));
        int wp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WPAWN));
        return knpkEval(pos.getKingSq(true), wn, wp, pos.getKingSq(false),
                        score, pos.isWhiteMove());
    }
    case MI::BN + MI::BP: {
        if (!doEval) return 1;
        int bn = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BKNIGHT));
        int bp = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BPAWN));
        return -knpkEval(63-pos.getKingSq(false), 63-bn, 63-bp, 63-pos.getKingSq(true),
                         -score, !pos.isWhiteMove());
    }
    }

    // QvsRMP fortress detection
    if (pos.pieceTypeBB(Piece::WQUEEN) && (wMtrlNoPawns == qV) &&
        pos.pieceTypeBB(Piece::BROOK) && pos.pieceTypeBB(Piece::BPAWN) &&
        pos.pieceTypeBB(Piece::BBISHOP, Piece::BKNIGHT) && (bMtrlNoPawns == rV + bV)) {
        if (!doEval)
            return 1;
        if (score > 0) {
            bool bishop = pos.pieceTypeBB(Piece::BBISHOP) != 0;
            int wq = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WQUEEN));
            int br = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BROOK));
            int bm = BitBoard::firstSquare(pos.pieceTypeBB(bishop ? Piece::BBISHOP : Piece::BKNIGHT));
            U64 wp = pos.pieceTypeBB(Piece::WPAWN);
            U64 bp = pos.pieceTypeBB(Piece::BPAWN);
            if (kqkrmFortress(bishop, wq, br, bm, wp, bp))
                return score / 8;
        }
    }
    if (pos.pieceTypeBB(Piece::BQUEEN) && (bMtrlNoPawns == qV) &&
        pos.pieceTypeBB(Piece::WROOK) && pos.pieceTypeBB(Piece::WPAWN) &&
        pos.pieceTypeBB(Piece::WBISHOP, Piece::WKNIGHT) && (wMtrlNoPawns == rV + bV)) {
        if (!doEval)
            return 1;
        if (score < 0) {
            bool bishop = pos.pieceTypeBB(Piece::WBISHOP) != 0;
            int bq = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BQUEEN));
            int wr = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WROOK));
            int wm = BitBoard::firstSquare(pos.pieceTypeBB(bishop ? Piece::WBISHOP : Piece::WKNIGHT));
            U64 bp = pos.pieceTypeBB(Piece::BPAWN);
            U64 wp = pos.pieceTypeBB(Piece::WPAWN);
            if (kqkrmFortress(bishop, Position::mirrorY(bq), Position::mirrorY(wr),
                              Position::mirrorY(wm), BitBoard::mirrorY(bp),
                              BitBoard::mirrorY(wp)))
                return score / 8;
        }
    }

    // QvsRP fortress detection
    if (pos.pieceTypeBB(Piece::WQUEEN) && (pos.wMtrl() == qV) &&
        pos.pieceTypeBB(Piece::BROOK) && pos.pieceTypeBB(Piece::BPAWN) &&
        (pos.bMtrl() - pos.bMtrlPawns() < rV * 2)) {
        if (!doEval) return 1;
        if (score > 0) {
            int wk = pos.getKingSq(true);
            int wq = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WQUEEN));
            int bk = pos.getKingSq(false);
            int br = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BROOK));
            U64 m = pos.pieceTypeBB(Piece::BPAWN);
            int newScore = score;
            while (m) {
                int bp = BitBoard::extractSquare(m);
                int s2 = kqkrpEval(wk, wq, bk, br, bp, pos.isWhiteMove(), score);
                newScore = std::min(newScore, s2);
            }
            if (newScore < score)
                return newScore;
        }
    }
    if (pos.pieceTypeBB(Piece::BQUEEN) && (pos.bMtrl() == qV) &&
        pos.pieceTypeBB(Piece::WROOK) && pos.pieceTypeBB(Piece::WPAWN) &&
        (pos.wMtrl() - pos.wMtrlPawns() < rV * 2)) {
        if (!doEval) return 1;
        if (score < 0) {
            int bk = pos.getKingSq(false);
            int bq = BitBoard::firstSquare(pos.pieceTypeBB(Piece::BQUEEN));
            int wk = pos.getKingSq(true);
            int wr = BitBoard::firstSquare(pos.pieceTypeBB(Piece::WROOK));
            U64 m = pos.pieceTypeBB(Piece::WPAWN);
            int newScore = score;
            while (m) {
                int wp = BitBoard::extractSquare(m);
                int s2 = -kqkrpEval(63-bk, 63-bq, 63-wk, 63-wr, 63-wp, !pos.isWhiteMove(), -score);
                newScore = std::max(newScore, s2);
            }
            if (newScore > score)
                return newScore;
        }
    }

    const int nWN = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    const int nBN = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));
    const int nWB1 = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskLightSq);
    const int nWB2 = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq);
    const int nBB1 = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskLightSq);
    const int nBB2 = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq);

    if (pos.materialId() == MI::WB * 2 + MI::BN) {
        if (!doEval) return 1;
        if (nWB1 == 1)
            return 100 + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if (pos.materialId() == MI::BB * 2 + MI::WN) {
        if (!doEval) return 1;
        if (nBB1 == 1)
            return -(100 + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }

    // Bonus for K[BN][BN]KQ
    if ((pos.bMtrl() == qV) && pos.pieceTypeBB(Piece::BQUEEN) && ((nWN >= 2) || (nWB1 + nWB2 >= 2))) {
        if (!doEval) return 1;
        if ((score < 0) && ((nWN >= 2) || ((nWB1 >= 1) && (nWB2 >= 1))))
            return -((pos.bMtrl() - pos.wMtrl()) / 8 + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }
    if ((pos.wMtrl() == qV) && pos.pieceTypeBB(Piece::WQUEEN) && ((nBN >= 2) || (nBB1 + nBB2 >= 2))) {
        if (!doEval) return 1;
        if ((score > 0) && ((nBN >= 2) || ((nBB1 >= 1) && (nBB2 >= 1))))
            return (pos.wMtrl() - pos.bMtrl()) / 8 + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if ((pos.bMtrl() == qV) && pos.pieceTypeBB(Piece::BQUEEN) && ((nWN >= 1) && (nWB1 + nWB2 >= 1))) {
        if (!doEval) return 1;
        if (score < 0)
            return -((pos.bMtrl() - pos.wMtrl()) / 2 + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }
    if ((pos.wMtrl() == qV) && pos.pieceTypeBB(Piece::WQUEEN) && ((nBN >= 1) && (nBB1 + nBB2 >= 1))) {
        if (!doEval) return 1;
        if (score > 0)
            return (pos.wMtrl() - pos.bMtrl()) / 2 + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }

    // Bonus for KRK
    if ((pos.bMtrl() == 0) && pos.pieceTypeBB(Piece::WROOK)) {
        if (!doEval) return 1;
        return 450 + pos.wMtrl() - pos.bMtrl() + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if ((pos.wMtrl() == 0) && pos.pieceTypeBB(Piece::BROOK)) {
        if (!doEval) return 1;
        return -(450 + pos.bMtrl() - pos.wMtrl() + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }

    // Bonus for KQK[BN]
    const int bV = ::bV;
    const int nV = ::nV;
    if (pos.pieceTypeBB(Piece::WQUEEN) && (bMtrlPawns == 0) && (pos.bMtrl() <= std::max(bV,nV))) {
        if (!doEval) return 1;
        return 235 + pos.wMtrl() - pos.bMtrl() + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if (pos.pieceTypeBB(Piece::BQUEEN) && (wMtrlPawns == 0) && (pos.wMtrl() <= std::max(bV,nV))) {
        if (!doEval) return 1;
        return -(235 + pos.bMtrl() - pos.wMtrl() + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }

    // Bonus for KQK
    if ((pos.bMtrl() == 0) && pos.pieceTypeBB(Piece::WQUEEN)) {
        if (!doEval) return 1;
        return 100 + pos.wMtrl() - pos.bMtrl() + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if ((pos.wMtrl() == 0) && pos.pieceTypeBB(Piece::BQUEEN)) {
        if (!doEval) return 1;
        return -(100 + pos.bMtrl() - pos.wMtrl() + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }

    if (pos.pieceTypeBB(Piece::WROOK, Piece::WKNIGHT, Piece::WQUEEN) == 0) {
        if (!doEval) return 1;
        if ((score > 0) && isBishopPawnDraw<true>(pos))
            return 0;
    }
    if (pos.pieceTypeBB(Piece::BROOK, Piece::BKNIGHT, Piece::BQUEEN) == 0) {
        if (!doEval) return 1;
        if ((score < 0) && isBishopPawnDraw<false>(pos))
            return 0;
    }

    // Correction for KQKNNNN which is generally won by the knights
    if ((pos.wMtrl() == qV) && pos.pieceTypeBB(Piece::WQUEEN) && (nBN >= 4)) {
        if (!doEval) return 1;
        return score - 125 - mateEval(pos.getKingSq(false), pos.getKingSq(true));
    }
    if ((pos.bMtrl() == qV) && pos.pieceTypeBB(Piece::BQUEEN) && (nWN >= 4)) {
        if (!doEval) return 1;
        return score + 125 + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }

    const int nWR = BitBoard::bitCount(pos.pieceTypeBB(Piece::WROOK));
    const int nBR = BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK));

    // Give bonus/penalty if advantage is/isn't large enough to win
    if ((wMtrlPawns == 0) && (wMtrlNoPawns <= bMtrlNoPawns + bV)) {
        if (!doEval) return 1;
        if (score > 0) {
            if (wMtrlNoPawns < rV) {
                return -pos.bMtrl() / 50;
            } else {
                int nMinor = nWN + nWB1 + nWB2;
                if ((nMinor == 1) && (nWR == 2) && (nBR >= 2))
                    return score;       // Often a win
                if ((nMinor <= 1) || !pos.pieceTypeBB(Piece::WROOK, Piece::WQUEEN))
                    return score / 8;   // Too little excess material, probably draw
                else
                    return score;       // May or may not be a win, TBs required
            }
        }
    }
    if ((bMtrlPawns == 0) && (bMtrlNoPawns <= wMtrlNoPawns + bV)) {
        if (!doEval) return 1;
        if (score < 0) {
            if (bMtrlNoPawns < rV) {
                return pos.wMtrl() / 50;
            } else {
                int nMinor = nBN + nBB1 + nBB2;
                if ((nMinor == 1) && (nBR == 2) && (nWR >= 2))
                    return score;       // Often a win
                if ((nMinor <= 1) || !pos.pieceTypeBB(Piece::BROOK, Piece::BQUEEN))
                    return score / 8;   // Too little excess material, probably draw
                else
                    return score;       // May or may not be a win, TBs required
            }
        }
    }

    // KRKBNN is generally a draw
    if (!pos.pieceTypeBB(Piece::WQUEEN, Piece::WROOK, Piece::WPAWN) &&
        (nWN <= 2) && (nWB1 + nWB2 <= 1) && (nBR > 0)) {
        if (!doEval) return 1;
        if (score > 0)
            return score / 8;
    }
    if (!pos.pieceTypeBB(Piece::BQUEEN, Piece::BROOK, Piece::BPAWN) &&
        (nBN <= 2) && (nBB1 + nBB2 <= 1) && (nWR > 0)) {
        if (!doEval) return 1;
        if (score < 0)
            return score / 8;
    }

    if ((bMtrlPawns == 0) && (wMtrlNoPawns - bMtrlNoPawns > bV)) {
        if (!doEval) return 1;
        return score + 300;       // Enough excess material, should win
    }
    if ((wMtrlPawns == 0) && (bMtrlNoPawns - wMtrlNoPawns > bV)) {
        if (!doEval) return 1;
        return score - 300;       // Enough excess material, should win
    }

    // Give bonus for advantage larger than KRKP, to avoid evaluation discontinuity
    if ((pos.bMtrl() == pV) && (nWR > 0) && (pos.wMtrl() > rV)) {
        if (!doEval) return 1;
        return score + krkpBonus;
    }
    if ((pos.wMtrl() == pV) && (nBR > 0) && (pos.bMtrl() > rV)) {
        if (!doEval) return 1;
        return score - krkpBonus;
    }

    // Bonus for KRPKN
    if ((nWR > 0) && pos.pieceTypeBB(Piece::WPAWN) &&
        !pos.pieceTypeBB(Piece::BBISHOP) && (pos.bMtrl() == nV)  && (bMtrlPawns == 0)) {
        if (!doEval) return 1;
        return score + krpknBonus;
    }
    if ((nBR > 0) && pos.pieceTypeBB(Piece::BPAWN) &&
        !pos.pieceTypeBB(Piece::WBISHOP) && (pos.wMtrl() == nV)  && (wMtrlPawns == 0)) {
        if (!doEval) return 1;
        return score - krpknBonus;
    }

    // Bonus for KRPKB
    int krpkbAdjustment = 0;
    if ((nWR > 0) && pos.pieceTypeBB(Piece::WPAWN) &&
        !pos.pieceTypeBB(Piece::BKNIGHT) && (pos.bMtrl() == bV)  && (bMtrlPawns == 0)) {
        if (!doEval) return 1;
        score += krpkbBonus;
        krpkbAdjustment += krpkbBonus;
    }
    if ((nBR > 0) && pos.pieceTypeBB(Piece::BPAWN) &&
        !pos.pieceTypeBB(Piece::WKNIGHT) && (pos.wMtrl() == bV)  && (wMtrlPawns == 0)) {
        if (!doEval) return 1;
        score -= krpkbBonus;
        krpkbAdjustment += krpkbBonus;
    }

    // Penalty for KRPKB when pawn is on a/h file
     if ((wMtrlNoPawns == rV) && (wMtrlPawns <= pV) && pos.pieceTypeBB(Piece::BBISHOP)) {
        if (!doEval) return 1;
        if (score - krpkbAdjustment > 0) {
            U64 pMask = pos.pieceTypeBB(Piece::WPAWN);
            U64 bMask = pos.pieceTypeBB(Piece::BBISHOP);
            if (((pMask & BitBoard::maskFile[0]) && (bMask & BitBoard::maskDarkSq)) ||
                ((pMask & BitBoard::maskFile[7]) && (bMask & BitBoard::maskLightSq))) {
                score = (score - krpkbAdjustment) * krpkbPenalty / 128;
                return score;
            }
        }
    }
    if ((bMtrlNoPawns == rV) && (bMtrlPawns <= pV) && pos.pieceTypeBB(Piece::WBISHOP)) {
        if (!doEval) return 1;
        if (score + krpkbAdjustment < 0) {
            U64 pMask = pos.pieceTypeBB(Piece::BPAWN);
            U64 bMask = pos.pieceTypeBB(Piece::WBISHOP);
            if (((pMask & BitBoard::maskFile[0]) && (bMask & BitBoard::maskLightSq)) ||
                ((pMask & BitBoard::maskFile[7]) && (bMask & BitBoard::maskDarkSq))) {
                score = (score + krpkbAdjustment) * krpkbPenalty / 128;
                return score;
            }
        }
    }

    auto getPawnAsymmetry = [passedPawns, &pos]() {
        int f1 = BitBoard::southFill(pos.pieceTypeBB(Piece::WPAWN)) & 0xff;
        int f2 = BitBoard::southFill(pos.pieceTypeBB(Piece::BPAWN)) & 0xff;
        int asymmetry = BitBoard::bitCount((f1 & ~f2) | (f2 & ~f1));
        U64 passedPawnsW = passedPawns & pos.pieceTypeBB(Piece::WPAWN);
        U64 passedPawnsB = passedPawns & pos.pieceTypeBB(Piece::BPAWN);
        asymmetry += BitBoard::bitCount(passedPawnsW) + BitBoard::bitCount(passedPawnsB);
        return asymmetry;
    };

    // Account for draw factor in rook endgames
    if ((nWR == 1) && (nBR == 1) &&
        (pos.pieceTypeBB(Piece::WQUEEN, Piece::WBISHOP, Piece::WKNIGHT,
                         Piece::BQUEEN, Piece::BBISHOP, Piece::BKNIGHT) == 0) &&
        (BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN, Piece::BPAWN)) > 1)) {
        if (!doEval) return 1;
        int asymmetry = getPawnAsymmetry();
        score = score * rookEGDrawFactor[std::min(asymmetry, 6)] / 128;
        return score;
    }

    // Correction for draw factor in RvsB endgames
    if ((nWR == 1) &&
        (BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP)) == 1) &&
        (pos.pieceTypeBB(Piece::WQUEEN, Piece::WBISHOP, Piece::WKNIGHT,
                         Piece::BQUEEN, Piece::BROOK, Piece::BKNIGHT) == 0) &&
        (wMtrlPawns - bMtrlPawns == -pV)) {
        if (!doEval) return 1;
        int asymmetry = getPawnAsymmetry();
        score = score * RvsBPDrawFactor[std::min(asymmetry, 6)] / 128;
        return score;
    }
    // Correction for draw factor in RvsB endgames
    if ((nBR == 1) &&
        (BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP)) == 1) &&
        (pos.pieceTypeBB(Piece::BQUEEN, Piece::BBISHOP, Piece::BKNIGHT,
                         Piece::WQUEEN, Piece::WROOK, Piece::WKNIGHT) == 0) &&
        (wMtrlPawns - bMtrlPawns == pV)) {
        if (!doEval) return 1;
        int asymmetry = getPawnAsymmetry();
        score = score * RvsBPDrawFactor[std::min(asymmetry, 6)] / 128;
        return score;
    }

    if (!doEval) return 0;
    return score;
}

int
EndGameEval::mateEval(int k1, int k2) {
    static const int loseKingTable[64] = {
        0,   4,   8,  12,  12,   8,   4,   0,
        4,   8,  12,  16,  16,  12,   8,   4,
        8,  12,  16,  20,  20,  16,  12,   8,
       12,  16,  20,  24,  24,  20,  16,  12,
       12,  16,  20,  24,  24,  20,  16,  12,
        8,  12,  16,  20,  20,  16,  12,   8,
        4,   8,  12,  16,  16,  12,   8,   4,
        0,   4,   8,  12,  12,   8,   4,   0
    };
    return winKingTable[k1] - loseKingTable[k2];
}

template <bool whiteBishop>
bool
EndGameEval::isBishopPawnDraw(const Position& pos) {
    const Piece::Type bishop = whiteBishop ? Piece::WBISHOP : Piece::BBISHOP;
    const bool darkBishop  = (pos.pieceTypeBB(bishop) & BitBoard::maskDarkSq) != 0;
    const bool lightBishop = (pos.pieceTypeBB(bishop) & BitBoard::maskLightSq) != 0;
    if (darkBishop && lightBishop)
        return false; // No draw against proper bishop pair

    const Piece::Type pawn = whiteBishop ? Piece::WPAWN : Piece::BPAWN;
    const U64 pawns = pos.pieceTypeBB(pawn);
    if (pawns == 0)
        return true; // Only bishops on same color can not win

    // Check for rook pawn + wrong color bishop
    if (whiteBishop) {
        if (!(pawns & BitBoard::maskBToHFiles) && !lightBishop &&
            (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(A8,B8,A7,B7)))
            return true;
        if (!(pawns & BitBoard::maskAToGFiles) && !darkBishop &&
            (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(G8,H8,G7,H7)))
            return true;
        if (pos.bMtrl() == 0) {
            if ((pos.bKingSq() == H8) && (pawns & BitBoard::sqMask(G6)) &&
                !darkBishop && !pos.isWhiteMove() && !(pawns & BitBoard::sqMask(G7)))
                return true; // Stalemate
            if ((pos.bKingSq() == A8) && (pawns & BitBoard::sqMask(B6)) &&
                !lightBishop && !pos.isWhiteMove() && !(pawns & BitBoard::sqMask(B7)))
                return true; // Stalemate
            if ((pawns == BitBoard::sqMask(B6)) &&
                (pos.pieceTypeBB(bishop) == BitBoard::sqMask(A7)) &&
                (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(A8,B7)) &&
                (!pos.isWhiteMove() || !(pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A6,C6,C7,C8))))
                return true;
            if ((pawns == BitBoard::sqMask(G6)) &&
                (pos.pieceTypeBB(bishop) == BitBoard::sqMask(H7)) &&
                (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(H8,G7)) &&
                (!pos.isWhiteMove() || !(pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(H6,F6,F7,F8))))
                return true;
        }
    } else {
        if (!(pawns & BitBoard::maskBToHFiles) && !darkBishop &&
            (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A1,B1,A2,B2)))
            return true;
        if (!(pawns & BitBoard::maskAToGFiles) && !lightBishop &&
            (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(G1,H1,G2,H2)))
            return true;
        if (pos.wMtrl() == 0) { // Check for stalemate
            if ((pos.wKingSq() == H1) && (pawns & BitBoard::sqMask(G3)) &&
                !lightBishop && pos.isWhiteMove() && !(pawns & BitBoard::sqMask(G2)))
                return true; // Stalemate
            if ((pos.wKingSq() == A1) && (pawns & BitBoard::sqMask(B3)) &&
                !darkBishop && pos.isWhiteMove() && !(pawns & BitBoard::sqMask(B2)))
                return true; // Stalemate
            if ((pawns == BitBoard::sqMask(B3)) &&
                (pos.pieceTypeBB(bishop) == BitBoard::sqMask(A2)) &&
                (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A1,B2)) &&
                (pos.isWhiteMove() || !(pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(A3,C3,C2,C1))))
                return true;
            if ((pawns == BitBoard::sqMask(G3)) &&
                (pos.pieceTypeBB(bishop) == BitBoard::sqMask(H2)) &&
                (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(H1,G2)) &&
                (pos.isWhiteMove() || !(pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(H3,F3,F2,F1))))
                return true;
        }
    }

    // Check for fortress containing WPb6, BPb7, white bishop on dark square
    const Piece::Type king = whiteBishop ? Piece::WKING : Piece::BKING;
    const Piece::Type oPawn = whiteBishop ? Piece::BPAWN : Piece::WPAWN;
    const Piece::Type oKnight = whiteBishop ? Piece::BKNIGHT : Piece::WKNIGHT;
    const int b7 = whiteBishop ? (darkBishop ? B7 : G7) : (lightBishop ? B2 : G2);
    const int b6 = whiteBishop ? (darkBishop ? B6 : G6) : (lightBishop ? B3 : G3);
    const int c7 = whiteBishop ? (darkBishop ? C7 : F7) : (lightBishop ? C2 : F2);
    const int a8 = whiteBishop ? (darkBishop ? A8 : H8) : (lightBishop ? A1 : H1);
    const int b8 = whiteBishop ? (darkBishop ? B8 : G8) : (lightBishop ? B1 : G1);
    const int c8 = whiteBishop ? (darkBishop ? C8 : F8) : (lightBishop ? C1 : F1);
    const int d8 = whiteBishop ? (darkBishop ? D8 : E8) : (lightBishop ? D1 : E1);
    const int d7 = whiteBishop ? (darkBishop ? D7 : E7) : (lightBishop ? D2 : E2);
    const U64 bFile = (whiteBishop == darkBishop) ? 0x0202020202020202ULL : 0x4040404040404040ULL;
    const U64 acFile = (whiteBishop == darkBishop) ? 0x0505050505050505ULL : 0xA0A0A0A0A0A0A0A0ULL;
    const U64 corner = whiteBishop ? (darkBishop ? BitBoard::sqMask(A8,B8,A7) : BitBoard::sqMask(G8,H8,H7))
                                   : (lightBishop ? BitBoard::sqMask(A1,B1,A2) : BitBoard::sqMask(G1,H1,H2));

    if ((pos.getPiece(b7) == oPawn) && (pos.getPiece(b6) == pawn) &&
        (pos.getPiece(a8) != oKnight) && ((pos.pieceTypeBB(king) & corner) == 0) &&
        (BitBoard::bitCount(pos.pieceTypeBB(oPawn) & acFile) <= 1)) {
        if (pos.getPiece(c7) == pawn) {
            if (BitBoard::bitCount(pawns & ~bFile) == 1) {
                int oKingSq = pos.getKingSq(!whiteBishop);
                if ((oKingSq == c8) || (oKingSq == d7))
                    return true;
            }
        } else {
            int oKingSq = pos.getKingSq(!whiteBishop);
            if ((pawns & ~bFile) == 0) {
                if ((oKingSq == a8) || (oKingSq == b8) || (oKingSq == c8) ||
                    (oKingSq == d8) || (oKingSq == d7))
                    return true;
            } else if (pos.isWhiteMove() != whiteBishop) { // Test if stale-mate
                int oMtrl = whiteBishop ? pos.bMtrl() : pos.wMtrl();
                U64 bShift = whiteBishop ? (pos.pieceTypeBB(bishop) << 8) : (pos.pieceTypeBB(bishop) >> 8);
                U64 kShift = whiteBishop ? (pos.pieceTypeBB(king) << 8) : (pos.pieceTypeBB(king) >> 8);
                const U64 md6_h2 = whiteBishop ?
                        (darkBishop  ? BitBoard::sqMask(D6,E5,F4,G3,H2) : BitBoard::sqMask(E6,D5,C4,B3,A2)) :
                        (lightBishop ? BitBoard::sqMask(D3,E4,F5,G6,H7) : BitBoard::sqMask(E3,D4,C5,B6,A7));
                if ((oMtrl == pV) ||
                    ((oMtrl == 2*pV) && ((bShift & pos.pieceTypeBB(oPawn)) ||
                                         ((kShift & pos.pieceTypeBB(oPawn)) &&
                                          ((pos.pieceTypeBB(oPawn) & md6_h2) == 0))))) {
                    const U64 mc7c8 = whiteBishop ?
                            (darkBishop  ? BitBoard::sqMask(C7,C8) : BitBoard::sqMask(F7,F8)) :
                            (lightBishop ? BitBoard::sqMask(C2,C1) : BitBoard::sqMask(F2,F1));
                    const U64 md6e6e7e8 = whiteBishop ?
                            (darkBishop  ? BitBoard::sqMask(D6,E6,E7,E8) : BitBoard::sqMask(E6,D6,D7,D8)) :
                            (lightBishop ? BitBoard::sqMask(D3,E3,E2,E1) : BitBoard::sqMask(E3,D3,D2,D1));
                    const U64 me7e8 = whiteBishop ?
                            (darkBishop  ? BitBoard::sqMask(E7,E8) : BitBoard::sqMask(D7,D8)) :
                            (lightBishop ? BitBoard::sqMask(E2,E1) : BitBoard::sqMask(D2,D1));
                    if (oKingSq == a8) {
                        if ((pos.pieceTypeBB(king) & mc7c8) ||
                                (pos.pieceTypeBB(bishop) & (md6_h2 | (1ULL << c7))))
                            return true;
                    } else if (oKingSq == c8) {
                        if (pos.getPiece(c7) == bishop) {
                            if (pos.pieceTypeBB(king) & md6e6e7e8)
                                return true;
                        } else {
                            if ((pos.pieceTypeBB(bishop) & md6_h2) && (pos.pieceTypeBB(king) & me7e8))
                                return true;
                        }
                    }
                }
            }
        }
    }

    // Check for fortress when all pawns are on the B file and there is no bishop
    if (whiteBishop) {
        if (pos.pieceTypeBB(Piece::WBISHOP) == 0) {
            if ((pos.pieceTypeBB(Piece::WPAWN,Piece::BPAWN) & ~BitBoard::maskFileB) == 0) {
                if ((pos.getPiece(B7) == Piece::BPAWN) &&
                    (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(A7,A8,B8)))
                    return true;
            }
            if ((pos.pieceTypeBB(Piece::WPAWN,Piece::BPAWN) & ~BitBoard::maskFileG) == 0) {
                if ((pos.getPiece(G7) == Piece::BPAWN) &&
                    (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(H7,H8,G8)))
                    return true;
            }
        }
    } else {
        if (pos.pieceTypeBB(Piece::BBISHOP) == 0) {
            if ((pos.pieceTypeBB(Piece::WPAWN,Piece::BPAWN) & ~BitBoard::maskFileB) == 0) {
                if ((pos.getPiece(B2) == Piece::WPAWN) &&
                    (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A2,A1,B1)))
                    return true;
            }
            if ((pos.pieceTypeBB(Piece::WPAWN,Piece::BPAWN) & ~BitBoard::maskFileG) == 0) {
                if ((pos.getPiece(G2) == Piece::WPAWN) &&
                    (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(H2,H1,G1)))
                    return true;
            }
        }
    }

    // Check for WPg7,h6, BPh7 fortress
    if (BitBoard::bitCount(pos.pieceTypeBB(bishop)) == 1) {
        if (whiteBishop) {
            U64 otherPawns = pos.pieceTypeBB(Piece::BPAWN);
            if (darkBishop) {  // H8 corner
                if ((pos.getPiece(H7) == Piece::BPAWN) &&
                    (pos.getPiece(H6) == Piece::WPAWN) &&
                    (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(H8,G8,F8,F7)) &&
                    ((pawns & 0x003F7F7F7F7F7F00ULL) == 0) &&
                    ((BitBoard::southFill((otherPawns & BitBoard::maskFileG) >> 7) & pawns) == 0))
                    return true;
            } else {           // A8 corner
                if ((pos.getPiece(A7) == Piece::BPAWN) &&
                    (pos.getPiece(A6) == Piece::WPAWN) &&
                    (pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(A8,B8,C8,C7)) &&
                    ((pawns & 0x00FCFEFEFEFEFE00ULL) == 0) &&
                    ((BitBoard::southFill((otherPawns & BitBoard::maskFileB) >> 9) & pawns) == 0))
                    return true;
            }
        } else {
            U64 otherPawns = pos.pieceTypeBB(Piece::WPAWN);
            if (lightBishop) { // H1 corner
                if ((pos.getPiece(H2) == Piece::WPAWN) &&
                    (pos.getPiece(H3) == Piece::BPAWN) &&
                    (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(H1,G1,F1,F2)) &&
                    ((pawns & 0x007F7F7F7F7F3F00ULL) == 0) &&
                    ((BitBoard::northFill((otherPawns & BitBoard::maskFileG) << 9) & pawns) == 0))
                    return true;
            } else {           // A1 corner
                if ((pos.getPiece(A2) == Piece::WPAWN) &&
                    (pos.getPiece(A3) == Piece::BPAWN) &&
                    (pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(A1,B1,C1,C2)) &&
                    ((pawns & 0x00FEFEFEFEFEFC00ULL) == 0) &&
                    ((BitBoard::northFill((otherPawns & BitBoard::maskFileB) << 7) & pawns) == 0))
                    return true;
            }
        }
    }

    return false;
}

int
EndGameEval::kqkpEval(int wKing, int wQueen, int bKing, int bPawn, bool whiteMove, int score) {
    bool canWin = false;
    if (((1ULL << bKing) & 0xFFFF) == 0) {
        canWin = true; // King doesn't support pawn
    } else if (std::abs(Position::getX(bPawn) - Position::getX(bKing)) > 2) {
        canWin = true; // King doesn't support pawn
    } else {
        switch (bPawn) {
        case A2:
            canWin = ((1ULL << wKing) & 0x0F1F1F1F1FULL) != 0;
            if (canWin && (bKing == A1) && (Position::getX(wQueen) == 1) && !whiteMove)
                canWin = false; // Stale-mate
            break;
        case C2:
            canWin = ((1ULL << wKing) & 0x071F1F1FULL) != 0;
            break;
        case F2:
            canWin = ((1ULL << wKing) & 0xE0F8F8F8ULL) != 0;
            break;
        case H2:
            canWin = ((1ULL << wKing) & 0xF0F8F8F8F8ULL) != 0;
            if (canWin && (bKing == H1) && (Position::getX(wQueen) == 6) && !whiteMove)
                canWin = false; // Stale-mate
            break;
        default:
            canWin = true;
            break;
        }
    }

    const int dist = BitBoard::getKingDistance(wKing, bPawn);
    score = score - 20 * (dist - 4);
    if (!canWin)
        score /= 50;
    return score;
}

int
EndGameEval::kqkrpEval(int wKing, int wQueen, int bKing, int bRook, int bPawn, bool whiteMove, int score) {
    if (!(BitBoard::bPawnAttacks[bPawn] & (1ULL << bRook)))
        return score; // Rook not protected by pawn, no fortress
    if ((1ULL << bPawn) & (BitBoard::maskFileE | BitBoard::maskFileF |
                           BitBoard::maskFileG | BitBoard::maskFileH)) { // Mirror X
        wKing ^= 7;
        wQueen ^= 7;
        bKing ^= 7;
        bRook ^= 7;
        bPawn ^= 7;
    }
    bool drawish = false;
    switch (bPawn) {
    case A6:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(A8,B8,A7,B7)) &&
                  (Position::getX(wKing) >= 2) &&
                  (Position::getY(wKing) <= 3);
        break;
    case A2:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(A4,B4,A3,B3));
        break;
    case B7:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(A8,B8,C8,A7,C7)) &&
                  (Position::getY(wKing) <= 4);
        break;
    case B6:
        if (bRook == C5) {
            drawish = ((1ULL << bKing) & BitBoard::sqMask(A7,B7)) ||
                      (((1ULL << bKing) & BitBoard::sqMask(A8,B8)) &&
                       ((Position::getX(wKing) >= 3) || (Position::getY(wKing) <= 3)) &&
                       (((1ULL << wQueen) & BitBoard::maskRow7) ||
                        (!whiteMove && (wQueen != A6)) ||
                        (whiteMove && !((1ULL << wQueen) & BitBoard::sqMask(A6,A5,A4,A3,A2,A1,B5,B4,B3,B2,B1,C4,D3,E2,F1)))));
        }
        break;
    case B5:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(A6,B6,C6,A5)) &&
                  (Position::getY(wKing) <= 2);
        break;
    case B4:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(A6,B6,C6,A5,B5,C5)) &&
                  ((Position::getY(wKing) <= 2) || (Position::getX(wKing) >= 4));
        break;
    case B3:
        drawish = (((1ULL << bKing) & BitBoard::sqMask(A4,B4,C4,A3)) &&
                   ((1ULL << wKing) & BitBoard::maskRow1Row8)) ||
                  (((1ULL << bKing) & BitBoard::sqMask(A5,B5)) &&
                   ((1ULL << wQueen) & BitBoard::maskRow4) &&
                   ((1ULL << wKing) & BitBoard::maskRow1));
        break;
    case B2:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(A4,B4,C4,A3,B3,C3,A2,C2)) &&
                  ((1ULL << wKing) & BitBoard::sqMask(A4,B4,C4,A3,B3,C3,A2,C2)) == 0;
        break;
    case C7:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(B8,C8,D8,B7,D7)) &&
                  (Position::getY(wKing) <= 4);
        break;
    case C3:
        drawish = (((1ULL << bKing) & BitBoard::sqMask(B4,C4,D4)) &&
                   ((1ULL << wKing) & BitBoard::maskRow1Row8)) ||
                  (((1ULL << bKing) & BitBoard::sqMask(B5,C5)) &&
                   (((1ULL << wQueen) & BitBoard::maskRow4) || !whiteMove) &&
                   ((1ULL << wKing) & BitBoard::maskRow1)) ||
                  ((((bKing == B3) && (wQueen != B5)) || ((bKing == D3) && (wQueen != D5))) &&
                   ((1ULL << wKing) & BitBoard::maskRow1));

        break;
    case C2:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(B3,C3,D3,B2,D2)) &&
                  ((Position::getX(wKing) == 0) || (Position::getX(wKing) >= 4));
        break;
    case D7:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(C8,D8,E8,C7,E7)) &&
                  (Position::getY(wKing) <= 4);
        break;
    case D3:
        drawish = (((1ULL << bKing) & BitBoard::sqMask(C4,D4,E4)) &&
                   ((1ULL << wKing) & BitBoard::maskRow1Row8)) ||
                  (((1ULL << bKing) & BitBoard::sqMask(C5,D5,E5)) &&
                   (((1ULL << wQueen) & BitBoard::maskRow4) || !whiteMove) &&
                   ((1ULL << wKing) & BitBoard::maskRow1)) ||
                  ((((bKing == C3) && (wQueen != C5)) || ((bKing == E3) && (wQueen != E5))) &&
                   ((1ULL << wKing) & BitBoard::maskRow1));
        break;
    case D2:
        drawish = ((1ULL << bKing) & BitBoard::sqMask(C4,D4,E4,C3,D3,E3,C2,E2)) &&
                  ((1ULL << wKing) & BitBoard::sqMask(C4,D4,E4,C3,D3,E3,C2,E2)) == 0;
        break;
    default:
        drawish = false;
        break;
    }
    return drawish ? score / 16 : score;
}

bool
EndGameEval::kqkrmFortress(bool bishop, int wQueen, int bRook, int bMinor, U64 wPawns, U64 bPawns) {
    U64 needPawnGuard = 0;
    U64 bpAtk = BitBoard::bPawnAttacksMask(bPawns);
    U64 bmAtk = bishop ? BitBoard::bishopAttacks(bMinor, wPawns | bPawns | (1ULL << bRook))
                       : BitBoard::knightAttacks[bMinor];
    U64 brAtk = BitBoard::rookAttacks(bRook, wPawns | bPawns | (1ULL << bMinor));
    if (!(bmAtk & (1ULL << bRook)))
        needPawnGuard |= 1ULL << bRook;
    if (!(brAtk & (1ULL << bMinor)))
        needPawnGuard |= 1ULL << bMinor;

    while (true) {
        if ((bpAtk & needPawnGuard) != needPawnGuard)
            return false;
        U64 newBPawns = bPawns & (bpAtk | bmAtk | brAtk);
        if (newBPawns == bPawns)
            break;
        bPawns = newBPawns;
        bpAtk = BitBoard::bPawnAttacksMask(bPawns);
    }
    U64 safeMask = bPawns | (1ULL << bRook) | (1ULL << bMinor);
    U64 tmp = wPawns;
    while (tmp) {
        int wp = BitBoard::extractSquare(tmp);
        U64 span = BitBoard::northFill(1ULL << wp) & ~BitBoard::northFill(safeMask);
        if ((span & BitBoard::maskRow8) || (BitBoard::wPawnAttacksMask(span) & safeMask))
            return false;
    }
    return true;
}

int
EndGameEval::kpkEval(int wKing, int bKing, int wPawn, bool whiteMove) {
    if (Position::getX(wKing) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        wPawn ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 32 + Position::getY(wKing)*4+Position::getX(wKing);
    index = index * 64 + bKing;
    index = index * 48 + wPawn - 8;

    int bytePos = index / 8;
    int bitPos = index % 8;
    bool draw = (((int)kpkTable[bytePos]) & (1 << bitPos)) == 0;
    if (draw)
        return 0;
    return qV - pV / 4 * (7-Position::getY(wPawn));
}

bool
EndGameEval::kpkpEval(int wKing, int bKing, int wPawn, int bPawn, int& score) {
    const U64 wKingMask = 1ULL << wKing;
    const U64 bKingMask = 1ULL << bKing;
    if (wPawn == B6 && bPawn == B7) {
        if ((bKingMask & BitBoard::sqMask(A8,B8,C8,D8,D7)) &&
            ((wKingMask & BitBoard::sqMask(A8,B8,A7)) == 0)) {
            score = 0;
            return true;
        }
    } else if (wPawn == G6 && bPawn == G7) {
        if ((bKingMask & BitBoard::sqMask(E8,F8,G8,H8,E7)) &&
            ((wKingMask & BitBoard::sqMask(G8,H8,H7)) == 0)) {
            score = 0;
            return true;
        }
    } else if (wPawn == B2 && bPawn == B3) {
        if ((wKingMask & BitBoard::sqMask(A1,B1,C1,D1,D2)) &&
            ((bKingMask & BitBoard::sqMask(A1,B1,A2)) == 0)) {
            score = 0;
            return true;
        }
    } else if (wPawn == G2 && bPawn == G3) {
        if ((wKingMask & BitBoard::sqMask(E1,F1,G1,H1,E2)) &&
            ((bKingMask & BitBoard::sqMask(G1,H1,H2)) == 0)) {
            score = 0;
            return true;
        }
    }
    return false;
}

int
EndGameEval::krkpEval(int wKing, int bKing, int bPawn, bool whiteMove, int score) {
    if (Position::getX(bKing) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        bPawn ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 32 + Position::getY(bKing)*4+Position::getX(bKing);
    index = index * 48 + bPawn - 8;
    index = index * 8 + Position::getY(wKing);
    U8 mask = krkpTable[index];
    bool canWin = (mask & (1 << Position::getX(wKing))) != 0;

    score = score + Position::getY(bPawn) * pV / 4;
    if (!canWin)
        score /= 50;
    else
        score += krkpBonus;
    return score;
}

int
EndGameEval::krpkrEval(int wKing, int bKing, int wPawn, int wRook, int bRook, bool whiteMove) {
    if (Position::getX(wPawn) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        wPawn ^= 7;
        wRook ^= 7;
        bRook ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 24 + (Position::getY(wPawn)-1)*4+Position::getX(wPawn);
    index = index * 64 + wKing;
    const U64 kMask = krpkrTable[index];
    const bool canWin = (kMask & (1ULL << bKing)) != 0;
    U64 kingNeighbors = BitBoard::kingAttacks[bKing];
    const U64 occupied = (1ULL<<wKing) | (1ULL<<bKing) | (1ULL<<wPawn) | (1ULL<<bRook);
    const U64 rAtk = BitBoard::rookAttacks(wRook, occupied);
    kingNeighbors &= ~(BitBoard::kingAttacks[wKing] | BitBoard::wPawnAttacks[wPawn] | rAtk);
    bool close;
    if (canWin) {
        close = (kMask & kingNeighbors) != kingNeighbors;
    } else {
        close = (kMask & kingNeighbors) != 0;
    }
    int score = pV + Position::getY(wPawn) * pV / 4;
    if (canWin) {
        if (!close)
            score += pV;
    } else {
        if (close)
            score /= 2;
        else
            score /= 4;
    }
    return score;
}

int
EndGameEval::krpkrpEval(int wKing, int bKing, int wPawn, int wRook, int bRook, int bPawn, bool whiteMove, int score) {
    int hiScore = krpkrEval(wKing, bKing, wPawn, wRook, bRook, whiteMove);
    if (score > hiScore * 14 / 16)
        return hiScore * 14 / 16;
    int loScore = -krpkrEval(63-bKing, 63-wKing, 63-bPawn, 63-bRook, 63-wRook, !whiteMove);
    if (score < loScore * 14 / 16)
        return loScore * 14 / 16;
    return score;
}

int
EndGameEval::kbnkEval(int wKing, int bKing, bool darkBishop) {
    int score = 640;
    if (darkBishop) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
    }
    static const int bkTable[64] = { 17, 15, 12,  9,  7,  4,  2,  0,
                                     15, 20, 17, 15, 12,  9,  4,  2,
                                     12, 17, 22, 20, 17, 15,  9,  4,
                                      9, 15, 20, 25, 22, 17, 12,  7,
                                      7, 12, 17, 22, 25, 20, 15,  9,
                                      4,  9, 15, 17, 20, 22, 17, 12,
                                      2,  4,  9, 12, 15, 17, 20, 15,
                                      0,  2,  4,  7,  9, 12, 15, 17 };

    score += winKingTable[wKing] - bkTable[bKing];
    score -= std::min(0, BitBoard::getTaxiDistance(wKing, bKing) - 3);
    return score;
}

int
EndGameEval::kbpkbEval(int wKing, int wBish, int wPawn, int bKing, int bBish, int score) {
    U64 wPawnMask = 1ULL << wPawn;
    U64 pawnPath = BitBoard::northFill(wPawnMask);
    U64 bKingMask = 1ULL << bKing;
    U64 wBishMask = 1ULL << wBish;
    U64 wBishControl = (wBishMask & BitBoard::maskDarkSq) ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
    if ((bKingMask & pawnPath) && ((bKingMask & wBishControl) == 0))
        return 0;

    U64 bBishMask = 1ULL << bBish;
    if (((wBishMask & BitBoard::maskDarkSq) == 0) != ((bBishMask & BitBoard::maskDarkSq) == 0)) { // Different color bishops
        if (((bBishMask | BitBoard::bishopAttacks(bBish, bKingMask)) & pawnPath & ~wPawnMask) != 0)
            if (!(wPawn == A6 && bBish == B8) && !(wPawn == H6 && bBish == G8))
                return 0;
    }

    if (bKingMask & BitBoard::wPawnBlockerMask[wPawn])
        return score / 4;
    return score;
}

int
EndGameEval::kbpknEval(int wKing, int wBish, int wPawn, int bKing, int bKnight, int score) {
    U64 wPawnMask = 1ULL << wPawn;
    U64 pawnPath = BitBoard::northFill(wPawnMask);
    U64 bKingMask = 1ULL << bKing;
    U64 wBishMask = 1ULL << wBish;
    U64 wBishControl = (wBishMask & BitBoard::maskDarkSq) ? BitBoard::maskDarkSq : BitBoard::maskLightSq;

    U64 edges = 0xff818181818181ffULL;
    U64 bKnightMask = 1ULL << bKnight;
    if ((bKnightMask & edges & ~wBishControl) != 0) // Knight on edge square where it can be trapped
        return score;

    if ((bKingMask & pawnPath) && ((bKingMask & wBishControl) == 0))
        return 0;

    if (bKingMask & BitBoard::wPawnBlockerMask[wPawn])
        return score / 4;
    return score;
}

int
EndGameEval::knpkbEval(int wKing, int wKnight, int wPawn, int bKing, int bBish, int score, bool wtm) {
    U64 wPawnMask = 1ULL << wPawn;
    U64 bBishMask = 1ULL << bBish;
    U64 bBishControl = (bBishMask & BitBoard::maskDarkSq) ? BitBoard::maskDarkSq : BitBoard::maskLightSq;

    U64 p = wPawnMask;
    if (bBishControl & wPawnMask) {
        U64 bKingMask = 1ULL << bKing;
        U64 wKnightMask = 1ULL << wKnight;
        if (!wtm && (BitBoard::bishopAttacks(bBish, bKingMask | wKnightMask) & wPawnMask))
            return 0;
        p <<= 8;
    }
    U64 pawnDrawishMask = 0x183c7e7e7e7eULL;
    if (p & pawnDrawishMask)
        return score / 32;

    return score;
}

int
EndGameEval::knpkEval(int wKing, int wKnight, int wPawn, int bKing, int score, bool wtm) {
    if (Position::getX(wPawn) >= 4) { // Mirror X
        wKing ^= 7;
        wKnight ^= 7;
        wPawn ^= 7;
        bKing ^= 7;
    }
    if (wPawn == A7) {
        if (bKing == A8 || bKing == B7) // Fortress
            return 0;
        if (wKing == A8 && (bKing == C7 || bKing == C8)) {
            bool knightDark = Position::darkSquare(Position::getX(wKnight), Position::getY(wKnight));
            bool kingDark = Position::darkSquare(Position::getX(bKing), Position::getY(bKing));
            if (wtm == (knightDark == kingDark)) // King trapped
                return 0;
        }
    }
    return score;
}
