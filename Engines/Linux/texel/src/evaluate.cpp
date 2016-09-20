/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * evaluate.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "evaluate.hpp"
#include <vector>

int Evaluate::pieceValueOrder[Piece::nPieceTypes] = {
    0,
    5, 4, 3, 2, 2, 1,
    5, 4, 3, 2, 2, 1
};


static const int empty[64] = { 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
                               0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0};

int Evaluate::castleFactor[256];

static StaticInitializer<Evaluate> evInit;

void
Evaluate::staticInitialize() {
    psTab1[Piece::EMPTY]   = empty;
    psTab1[Piece::WKING]   = kt1w.getTable();
    psTab1[Piece::WQUEEN]  = qt1w.getTable();
    psTab1[Piece::WROOK]   = rt1w.getTable();
    psTab1[Piece::WBISHOP] = bt1w.getTable();
    psTab1[Piece::WKNIGHT] = nt1w.getTable();
    psTab1[Piece::WPAWN]   = pt1w.getTable();
    psTab1[Piece::BKING]   = kt1b.getTable();
    psTab1[Piece::BQUEEN]  = qt1b.getTable();
    psTab1[Piece::BROOK]   = rt1b.getTable();
    psTab1[Piece::BBISHOP] = bt1b.getTable();
    psTab1[Piece::BKNIGHT] = nt1b.getTable();
    psTab1[Piece::BPAWN]   = pt1b.getTable();

    psTab2[Piece::EMPTY]   = empty;
    psTab2[Piece::WKING]   = kt2w.getTable();
    psTab2[Piece::WQUEEN]  = qt1w.getTable();
    psTab2[Piece::WROOK]   = rt1w.getTable();
    psTab2[Piece::WBISHOP] = bt2w.getTable();
    psTab2[Piece::WKNIGHT] = nt2w.getTable();
    psTab2[Piece::WPAWN]   = pt2w.getTable();
    psTab2[Piece::BKING]   = kt2b.getTable();
    psTab2[Piece::BQUEEN]  = qt1b.getTable();
    psTab2[Piece::BROOK]   = rt1b.getTable();
    psTab2[Piece::BBISHOP] = bt2b.getTable();
    psTab2[Piece::BKNIGHT] = nt2b.getTable();
    psTab2[Piece::BPAWN]   = pt2b.getTable();

    for (int i = 0; i < 256; i++) {
        int h1Dist = 100;
        bool h1Castle = (i & (1<<7)) != 0;
        if (h1Castle)
            h1Dist = 2 + BitBoard::bitCount(i & 0x0000000000000060L); // f1,g1
        int a1Dist = 100;
        bool a1Castle = (i & 1) != 0;
        if (a1Castle)
            a1Dist = 2 + BitBoard::bitCount(i & 0x000000000000000EL); // b1,c1,d1
        castleFactor[i] = 1024 / std::min(a1Dist, h1Dist);
    }

    // Knight mobility scores
    for (int sq = 0; sq < 64; sq++) {
        int x = Position::getX(sq);
        int y = Position::getY(sq);
        if (x >= 4) x = 7 - x;
        if (y >= 4) y = 7 - y;
        if (x < y) std::swap(x, y);
        int maxMob = 0;
        switch (y*8+x) {
        case  0: maxMob = 2; break; // a1
        case  1: maxMob = 3; break; // b1
        case  2: maxMob = 4; break; // c1
        case  3: maxMob = 4; break; // d1
        case  9: maxMob = 4; break; // b2
        case 10: maxMob = 6; break; // c2
        case 11: maxMob = 6; break; // d2
        case 18: maxMob = 8; break; // c3
        case 19: maxMob = 8; break; // d3
        case 27: maxMob = 8; break; // d4
        default:
            assert(false);
        }
        for (int m = 0; m <= 8; m++)
            knightMobScore[sq][m] = m * 16 / maxMob - 12;
    }
}

const int* Evaluate::psTab1[Piece::nPieceTypes];
const int* Evaluate::psTab2[Piece::nPieceTypes];

const int Evaluate::distToH1A8[8][8] = { { 0, 1, 2, 3, 4, 5, 6, 7 },
                                         { 1, 2, 3, 4, 5, 6, 7, 6 },
                                         { 2, 3, 4, 5, 6, 7, 6, 5 },
                                         { 3, 4, 5, 6, 7, 6, 5, 4 },
                                         { 4, 5, 6, 7, 6, 5, 4, 3 },
                                         { 5, 6, 7, 6, 5, 4, 3, 2 },
                                         { 6, 7, 6, 5, 4, 3, 2, 1 },
                                         { 7, 6, 5, 4, 3, 2, 1, 0 } };

static const int winKingTable[64] = {
    0,   4,  10,  10,  10,  10,   4,   0,
    4,  15,  19,  20,  20,  19,  15,   4,
   10,  19,  25,  25,  25,  25,  19,  10,
   10,  20,  25,  25,  25,  25,  20,  10,
   10,  20,  25,  25,  25,  25,  20,  10,
   10,  19,  25,  25,  25,  25,  19,  10,
    4,  15,  19,  20,  20,  19,  15,   4,
    0,   4,  10,  10,  10,  10,   4,   0
};

int Evaluate::knightMobScore[64][9];

Evaluate::Evaluate(EvalHashTables& et)
    : pawnHash(et.pawnHash),
      materialHash(et.materialHash),
      kingSafetyHash(et.kingSafetyHash),
      wKingZone(0), bKingZone(0),
      wKingAttacks(0), bKingAttacks(0),
      wAttacksBB(0), bAttacksBB(0),
      wPawnAttacks(0), bPawnAttacks(0) {
}

int
Evaluate::evalPos(const Position& pos) {
    return evalPos<false>(pos);
}

int
Evaluate::evalPosPrint(const Position& pos) {
    return evalPos<true>(pos);
}

template <bool print>
inline int
Evaluate::evalPos(const Position& pos) {
    int score = materialScore(pos, print);

    wKingAttacks = bKingAttacks = 0;
    wKingZone = BitBoard::kingAttacks[pos.getKingSq(true)]; wKingZone |= wKingZone << 8;
    bKingZone = BitBoard::kingAttacks[pos.getKingSq(false)]; bKingZone |= bKingZone >> 8;
    wAttacksBB = bAttacksBB = 0L;

    U64 pawns = pos.pieceTypeBB(Piece::WPAWN);
    wPawnAttacks = ((pawns & BitBoard::maskBToHFiles) << 7) |
                   ((pawns & BitBoard::maskAToGFiles) << 9);
    pawns = pos.pieceTypeBB(Piece::BPAWN);
    bPawnAttacks = ((pawns & BitBoard::maskBToHFiles) >> 9) |
                   ((pawns & BitBoard::maskAToGFiles) >> 7);

    score += pieceSquareEval(pos);
    if (print) std::cout << "eval pst    :" << score << std::endl;
    score += pawnBonus(pos);
    if (print) std::cout << "eval pawn   :" << score << std::endl;
    score += castleBonus(pos);
    if (print) std::cout << "eval castle :" << score << std::endl;

    score += rookBonus(pos);
    if (print) std::cout << "eval rook   :" << score << std::endl;
    score += bishopEval(pos, score);
    if (print) std::cout << "eval bishop :" << score << std::endl;
    score += knightEval(pos);
    if (print) std::cout << "eval knight :" << score << std::endl;
    score += threatBonus(pos);
    if (print) std::cout << "eval threat :" << score << std::endl;
    score += kingSafety(pos);
    if (print) std::cout << "eval king   :" << score << std::endl;
    if (mhd->endGame)
        score = endGameEval<true>(pos, score);
    if (print) std::cout << "eval endgame:" << score << std::endl;

    if (!pos.getWhiteMove())
        score = -score;
    return score;
}

/** Compensate for the fact that many knights are stronger compared to queens
 * than what the default material scores would predict. */
static inline int correctionNvsQ(int n, int q) {
    if (n <= q+1)
        return 0;
    int knightBonus = 0;
    if (q == 1)
        knightBonus = knightVsQueenBonus1;
    else if (q == 2)
        knightBonus = knightVsQueenBonus2;
    else if (q >= 3)
        knightBonus = knightVsQueenBonus3;
    int corr = knightBonus * (n - q - 1);
    return corr;
}

void
Evaluate::computeMaterialScore(const Position& pos, MaterialHashData& mhd, bool print) const {
    // Compute material part of score
    int score = pos.wMtrl() - pos.bMtrl();
    if (print) std::cout << "eval mtrlraw:" << score << std::endl;
    const int nWQ = BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN));
    const int nBQ = BitBoard::bitCount(pos.pieceTypeBB(Piece::BQUEEN));
    const int nWN = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    const int nBN = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));
    int wCorr = correctionNvsQ(nWN, nBQ);
    int bCorr = correctionNvsQ(nBN, nWQ);
    score += wCorr - bCorr;
    if (print) std::cout << "eval qncorr :" << score << std::endl;
    score += tradeBonus(pos, wCorr, bCorr);
    if (print) std::cout << "eval trade  :" << score << std::endl;

    const int nWR = BitBoard::bitCount(pos.pieceTypeBB(Piece::WROOK));
    const int nBR = BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK));
    { // Redundancy of major pieces
        int wMajor = nWQ + nWR;
        int bMajor = nBQ + nBR;
        int w = std::min(wMajor, 3);
        int b = std::min(bMajor, 3);
        score += majorPieceRedundancy[w*4+b];
    }
    if (print) std::cout << "eval majred :" << score << std::endl;
    mhd.id = pos.materialId();
    mhd.score = score;
    mhd.endGame = endGameEval<false>(pos, 0);

    // Compute interpolation factors
    const int wMtrl = pos.wMtrl();
    const int bMtrl = pos.bMtrl();
    const int wMtrlPawns = pos.wMtrlPawns();
    const int bMtrlPawns = pos.bMtrlPawns();
    const int wMtrlNoPawns = wMtrl - wMtrlPawns;
    const int bMtrlNoPawns = bMtrl - bMtrlPawns;
    { // Pawn
        const int loMtrl = pawnLoMtrl;
        const int hiMtrl = pawnHiMtrl;
        mhd.wPawnIPF = interpolate(bMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bPawnIPF = interpolate(wMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        if (wCorr > 100)
            mhd.wPawnIPF = mhd.wPawnIPF * 100 / wCorr;
        if (bCorr > 100)
            mhd.bPawnIPF = mhd.bPawnIPF * 100 / bCorr;
    }
    { // Knight/bishop
        const int loMtrl = minorLoMtrl;
        const int hiMtrl = minorHiMtrl;
        mhd.wKnightIPF = interpolate(bMtrl, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bKnightIPF = interpolate(wMtrl, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Castle
        const int loMtrl = castleLoMtrl;
        const int hiMtrl = castleHiMtrl;
        const int m = wMtrlNoPawns + bMtrlNoPawns;
        mhd.castleIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Passed pawn
        const int loMtrl = passedPawnLoMtrl;
        const int hiMtrl = passedPawnHiMtrl;
        mhd.wPassedPawnIPF = interpolate(bMtrlNoPawns-nBN*(nV/2), loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bPassedPawnIPF = interpolate(wMtrlNoPawns-nWN*(nV/2), loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // King safety
        const int loMtrl = kingSafetyLoMtrl;
        const int hiMtrl = kingSafetyHiMtrl;
        const int m = (wMtrlNoPawns + bMtrlNoPawns) / 2;
        mhd.kingSafetyIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
        if (wCorr + bCorr > 200)
            mhd.kingSafetyIPF = mhd.kingSafetyIPF * 200 / (wCorr + bCorr);
    }
    { // Different color bishops
        const int loMtrl = oppoBishopLoMtrl;
        const int hiMtrl = oppoBishopHiMtrl;
        const int m = wMtrlNoPawns + bMtrlNoPawns;
        mhd.diffColorBishopIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Knight outpost
        const int loMtrl = knightOutpostLoMtrl;
        const int hiMtrl = knightOutpostHiMtrl;
        mhd.wKnightOutPostIPF = interpolate(bMtrlPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        mhd.bKnightOutPostIPF = interpolate(wMtrlPawns, loMtrl, 0, hiMtrl, IPOLMAX);
    }
}

int
Evaluate::tradeBonus(const Position& pos, int wCorr, int bCorr) const {
    const int wM = pos.wMtrl() + wCorr;
    const int bM = pos.bMtrl() + bCorr;
    const int wPawn = pos.wMtrlPawns();
    const int bPawn = pos.bMtrlPawns();
    const int deltaScore = wM - bM;

    int pBonus = 0;
    pBonus += interpolate((deltaScore > 0) ? wPawn : bPawn, 0, -pawnTradePenalty * deltaScore / 100, pawnTradeThreshold, 0);
    pBonus += interpolate((deltaScore > 0) ? bM : wM, 0, pieceTradeBonus * deltaScore / 100, pieceTradeThreshold * 100, 0);

    return pBonus;
}

int
Evaluate::pieceSquareEval(const Position& pos) {
    int score = 0;
    const int wMtrlPawns = pos.wMtrlPawns();
    const int bMtrlPawns = pos.bMtrlPawns();

    // Kings/pawns
    {
        const int k1 = pos.psScore1(Piece::WKING) + pos.psScore1(Piece::WPAWN);
        const int k2 = pos.psScore2(Piece::WKING) + pos.psScore2(Piece::WPAWN);
        score += interpolate(k2, k1, mhd->wPawnIPF);
    }
    {
        const int k1 = pos.psScore1(Piece::BKING) + pos.psScore1(Piece::BPAWN);
        const int k2 = pos.psScore2(Piece::BKING) + pos.psScore2(Piece::BPAWN);
        score -= interpolate(k2, k1, mhd->bPawnIPF);
    }

    // Knights/bishops
    {
        int n1 = pos.psScore1(Piece::WKNIGHT) + pos.psScore1(Piece::WBISHOP);
        int n2 = pos.psScore2(Piece::WKNIGHT) + pos.psScore2(Piece::WBISHOP);
        score += interpolate(n2, n1, mhd->wKnightIPF);
        n1 = pos.psScore1(Piece::BKNIGHT) + pos.psScore1(Piece::BBISHOP);
        n2 = pos.psScore2(Piece::BKNIGHT) + pos.psScore2(Piece::BBISHOP);
        score -= interpolate(n2, n1, mhd->bKnightIPF);
    }

    // Queens
    {
        const U64 occupied = pos.occupiedBB();
        score += pos.psScore1(Piece::WQUEEN);
        U64 m = pos.pieceTypeBB(Piece::WQUEEN);
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
            wAttacksBB |= atk;
            score += queenMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB() | bPawnAttacks))];
            bKingAttacks += BitBoard::bitCount(atk & bKingZone) * 2;
            m &= m-1;
        }
        score -= pos.psScore1(Piece::BQUEEN);
        m = pos.pieceTypeBB(Piece::BQUEEN);
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
            bAttacksBB |= atk;
            score -= queenMobScore[BitBoard::bitCount(atk & ~(pos.blackBB() | wPawnAttacks))];
            wKingAttacks += BitBoard::bitCount(atk & wKingZone) * 2;
            m &= m-1;
        }
    }

    // Rooks
    {
        int r1 = pos.psScore1(Piece::WROOK);
        if (r1 != 0) {
            const int nP = bMtrlPawns / pV;
            const int s = r1 * std::min(nP, 6) / 6;
            score += s;
        }
        r1 = pos.psScore1(Piece::BROOK);
        if (r1 != 0) {
            const int nP = wMtrlPawns / pV;
            const int s = r1 * std::min(nP, 6) / 6;
            score -= s;
        }
    }

    return score;
}

int
Evaluate::castleBonus(const Position& pos) {
    if (pos.getCastleMask() == 0) return 0;

    const int k1 = kt1b[7*8+6] - kt1b[7*8+4];
    const int k2 = kt2b[7*8+6] - kt2b[7*8+4];
    const int ks = interpolate(k2, k1, mhd->castleIPF);

    const int castleValue = ks + rt1b[7*8+5] - rt1b[7*8+7];
    if (castleValue <= 0)
        return 0;

    U64 occupied = pos.occupiedBB();
    int tmp = (int) (occupied & 0x6E);
    if (pos.a1Castle()) tmp |= 1;
    if (pos.h1Castle()) tmp |= (1 << 7);
    const int wBonus = (castleValue * castleFactor[tmp]) >> 10;

    tmp = (int) ((occupied >> 56) & 0x6E);
    if (pos.a8Castle()) tmp |= 1;
    if (pos.h8Castle()) tmp |= (1 << 7);
    const int bBonus = (castleValue * castleFactor[tmp]) >> 10;

    return wBonus - bBonus;
}

int
Evaluate::pawnBonus(const Position& pos) {
    U64 key = pos.pawnZobristHash();
    PawnHashData& phd = pawnHash[(int)key & (pawnHash.size() - 1)];
    if (phd.key != key)
        computePawnHashData(pos, phd);
    this->phd = &phd;
    U64 m = phd.passedPawnsW;
    int score = phd.score;

    const int hiMtrl = passedPawnHiMtrl;
    score += interpolate(2 * phd.passedBonusW, phd.passedBonusW, mhd->wPassedPawnIPF);
    score -= interpolate(2 * phd.passedBonusB, phd.passedBonusB, mhd->bPassedPawnIPF);

    // Passed pawns are more dangerous if enemy king is far away
    int bestWPawnDist = 8;
    int bestWPromSq = -1;
    if (m != 0) {
        int mtrlNoPawns = pos.bMtrl() - pos.bMtrlPawns();
        if (mtrlNoPawns < hiMtrl) {
            int kingPos = pos.getKingSq(false);
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                int x = Position::getX(sq);
                int y = Position::getY(sq);
                int pawnDist = std::min(5, 7 - y);
                int kingDist = BitBoard::getKingDistance(kingPos, Position::getSquare(x, 7));
                int kScore = kingDist * 4;
                if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                score += interpolate(kScore, 0, mhd->wPassedPawnIPF);
                if (!pos.getWhiteMove())
                    kingDist--;
                if ((pawnDist < kingDist) && (mtrlNoPawns == 0)) {
                    if (BitBoard::northFill(1ULL<<sq) & (1LL << pos.getKingSq(true)))
                        pawnDist++; // Own king blocking pawn
                    if (pawnDist < bestWPawnDist) {
                        bestWPawnDist = pawnDist;
                        bestWPromSq = Position::getSquare(x, 7);
                    }
                }
                m &= m-1;
            }
        }
    }
    int bestBPawnDist = 8;
    int bestBPromSq = -1;
    m = phd.passedPawnsB;
    if (m != 0) {
        int mtrlNoPawns = pos.wMtrl() - pos.wMtrlPawns();
        if (mtrlNoPawns < hiMtrl) {
            int kingPos = pos.getKingSq(true);
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                int x = Position::getX(sq);
                int y = Position::getY(sq);
                int pawnDist = std::min(5, y);
                int kingDist = BitBoard::getKingDistance(kingPos, Position::getSquare(x, 0));
                int kScore = kingDist * 4;
                if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                score -= interpolate(kScore, 0, mhd->bPassedPawnIPF);
                if (pos.getWhiteMove())
                    kingDist--;
                if ((pawnDist < kingDist) && (mtrlNoPawns == 0)) {
                    if (BitBoard::southFill(1ULL<<sq) & (1LL << pos.getKingSq(false)))
                        pawnDist++; // Own king blocking pawn
                    if (pawnDist < bestBPawnDist) {
                        bestBPawnDist = pawnDist;
                        bestBPromSq = Position::getSquare(x, 0);
                    }
                }
                m &= m-1;
            }
        }
    }

    // Evaluate pawn races in pawn end games
    const int prBonus = pawnRaceBonus;
    if (bestWPromSq >= 0) {
        if (bestBPromSq >= 0) {
            int wPly = bestWPawnDist * 2; if (pos.getWhiteMove()) wPly--;
            int bPly = bestBPawnDist * 2; if (!pos.getWhiteMove()) bPly--;
            if (wPly < bPly - 1) {
                score += prBonus;
            } else if (wPly == bPly - 1) {
                if (BitBoard::getDirection(bestWPromSq, pos.getKingSq(false)))
                    score += prBonus;
            } else if (wPly == bPly + 1) {
                if (BitBoard::getDirection(bestBPromSq, pos.getKingSq(true)))
                    score -= prBonus;
            } else {
                score -= prBonus;
            }
        } else
            score += prBonus;
    } else if (bestBPromSq >= 0)
        score -= prBonus;

    return score;
}

void
Evaluate::computePawnHashData(const Position& pos, PawnHashData& ph) {
    int score = 0;

    // Evaluate double pawns and pawn islands
    const U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 wPawnFiles = BitBoard::southFill(wPawns) & 0xff;
    const int wDouble = BitBoard::bitCount(wPawns) - BitBoard::bitCount(wPawnFiles);
    const int wIslands = BitBoard::bitCount(((~wPawnFiles) >> 1) & wPawnFiles);
    const int wIsolated = BitBoard::bitCount(~(wPawnFiles<<1) & wPawnFiles & ~(wPawnFiles>>1));

    const U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
    const U64 bPawnFiles = BitBoard::southFill(bPawns) & 0xff;
    const int bDouble = BitBoard::bitCount(bPawns) - BitBoard::bitCount(bPawnFiles);
    const int bIslands = BitBoard::bitCount(((~bPawnFiles) >> 1) & bPawnFiles);
    const int bIsolated = BitBoard::bitCount(~(bPawnFiles<<1) & bPawnFiles & ~(bPawnFiles>>1));

    score -= (wDouble - bDouble) * pawnDoubledPenalty;
    score -= (wIslands - bIslands) * pawnIslandPenalty;
    score -= (wIsolated - bIsolated) * pawnIsolatedPenalty;

    // Evaluate backward pawns, defined as a pawn that guards a friendly pawn,
    // can't be guarded by friendly pawns, can advance, but can't advance without
    // being captured by an enemy pawn.
    const U64 bPawnNoAtks = ~BitBoard::southFill(bPawnAttacks);
    const U64 wPawnNoAtks = ~BitBoard::northFill(wPawnAttacks);
    ph.outPostsW = bPawnNoAtks & wPawnAttacks;
    ph.outPostsB = wPawnNoAtks & bPawnAttacks;

    U64 wBackward = wPawns & ~((wPawns | bPawns) >> 8) & (bPawnAttacks >> 8) & wPawnNoAtks;
    wBackward &= (((wPawns & BitBoard::maskBToHFiles) >> 9) |
                  ((wPawns & BitBoard::maskAToGFiles) >> 7));
    wBackward &= ~BitBoard::northFill(bPawnFiles);
    U64 bBackward = bPawns & ~((wPawns | bPawns) << 8) & (wPawnAttacks << 8) & bPawnNoAtks;
    bBackward &= (((bPawns & BitBoard::maskBToHFiles) << 7) |
                  ((bPawns & BitBoard::maskAToGFiles) << 9));
    bBackward &= ~BitBoard::northFill(wPawnFiles);
    score -= (BitBoard::bitCount(wBackward) - BitBoard::bitCount(bBackward)) * pawnBackwardPenalty;

    // Evaluate passed pawn bonus, white
    U64 passedPawnsW = wPawns & ~BitBoard::southFill(bPawns | bPawnAttacks | (wPawns >> 8));
    int passedBonusW = 0;
    if (passedPawnsW != 0) {
        U64 guardedPassedW = passedPawnsW & wPawnAttacks;
        passedBonusW += pawnGuardedPassedBonus * BitBoard::bitCount(guardedPassedW);
        U64 m = passedPawnsW;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            int y = Position::getY(sq);
            passedBonusW += passedPawnBonus[y];
            m &= m-1;
        }
    }

    // Evaluate passed pawn bonus, black
    U64 passedPawnsB = bPawns & ~BitBoard::northFill(wPawns | wPawnAttacks | (bPawns << 8));
    int passedBonusB = 0;
    if (passedPawnsB != 0) {
        U64 guardedPassedB = passedPawnsB & bPawnAttacks;
        passedBonusB += pawnGuardedPassedBonus * BitBoard::bitCount(guardedPassedB);
        U64 m = passedPawnsB;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            int y = Position::getY(sq);
            passedBonusB += passedPawnBonus[7-y];
            m &= m-1;
        }
    }

    // Evaluate candidate passed pawn bonus
    const U64 wLeftAtks  = (wPawns & BitBoard::maskBToHFiles) << 7;
    const U64 wRightAtks = (wPawns & BitBoard::maskAToGFiles) << 9;
    const U64 bLeftAtks  = (bPawns & BitBoard::maskBToHFiles) >> 9;
    const U64 bRightAtks = (bPawns & BitBoard::maskAToGFiles) >> 7;
    const U64 bBlockSquares = ((bLeftAtks | bRightAtks) & ~(wLeftAtks | wRightAtks)) |
                              ((bLeftAtks & bRightAtks) & ~(wLeftAtks & wRightAtks));
    const U64 wCandidates = wPawns & ~BitBoard::southFill(bPawns | (wPawns >> 8) | bBlockSquares) & ~passedPawnsW;

    const U64 wBlockSquares = ((wLeftAtks | wRightAtks) & ~(bLeftAtks | bRightAtks)) |
                              ((wLeftAtks & wRightAtks) & ~(bLeftAtks & bRightAtks));
    const U64 bCandidates = bPawns & ~BitBoard::northFill(wPawns | (bPawns << 8) | wBlockSquares) & ~passedPawnsB;

    {
        U64 m = wCandidates;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            int y = Position::getY(sq);
            passedBonusW += candidatePassedBonus[y];
            m &= m-1;
        }
    }
    {
        U64 m = bCandidates;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            int y = Position::getY(sq);
            passedBonusB += candidatePassedBonus[7-y];
            m &= m-1;
        }
    }

    // Connected passed pawn bonus. Seems logical but scored -8 elo in tests
//    if (passedPawnsW != 0) {
//        U64 mask = passedPawnsW;
//        mask = (((mask >> 7) | (mask << 1) | (mask << 9)) & BitBoard::maskBToHFiles) |
//               (((mask >> 9) | (mask >> 1) | (mask << 7)) & BitBoard::maskAToGFiles);
//        passedBonusW += 13 * BitBoard::bitCount(passedPawnsW & mask);
//    }
//    if (passedPawnsB != 0) {
//        U64 mask = passedPawnsB;
//        mask = (((mask >> 7) | (mask << 1) | (mask << 9)) & BitBoard::maskBToHFiles) |
//               (((mask >> 9) | (mask >> 1) | (mask << 7)) & BitBoard::maskAToGFiles);
//        passedBonusB += 13 * BitBoard::bitCount(passedPawnsB & mask);
//    }

    ph.key = pos.pawnZobristHash();
    ph.score = score;
    ph.passedBonusW = (short)passedBonusW;
    ph.passedBonusB = (short)passedBonusB;
    ph.passedPawnsW = passedPawnsW;
    ph.passedPawnsB = passedPawnsB;
}


int
Evaluate::rookBonus(const Position& pos) {
    int score = 0;
    const U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
    const U64 occupied = pos.occupiedBB();
    U64 m = pos.pieceTypeBB(Piece::WROOK);
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        const int x = Position::getX(sq);
        if ((wPawns & BitBoard::maskFile[x]) == 0) // At least half-open file
            score += (bPawns & BitBoard::maskFile[x]) == 0 ? rookOpenBonus : rookHalfOpenBonus;
        U64 atk = BitBoard::rookAttacks(sq, occupied);
        wAttacksBB |= atk;
        score += rookMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB() | bPawnAttacks))];
        if ((atk & bKingZone) != 0)
            bKingAttacks += BitBoard::bitCount(atk & bKingZone);
        m &= m-1;
    }
    U64 r7 = pos.pieceTypeBB(Piece::WROOK) & 0x00ff000000000000ULL;
    if (((r7 & (r7 - 1)) != 0) &&
        ((pos.pieceTypeBB(Piece::BKING) & 0xff00000000000000ULL) != 0))
        score += rookDouble7thRowBonus; // Two rooks on 7:th row
    m = pos.pieceTypeBB(Piece::BROOK);
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        const int x = Position::getX(sq);
        if ((bPawns & BitBoard::maskFile[x]) == 0)
            score -= (wPawns & BitBoard::maskFile[x]) == 0 ? rookOpenBonus : rookHalfOpenBonus;
        U64 atk = BitBoard::rookAttacks(sq, occupied);
        bAttacksBB |= atk;
        score -= rookMobScore[BitBoard::bitCount(atk & ~(pos.blackBB() | wPawnAttacks))];
        if ((atk & wKingZone) != 0)
            wKingAttacks += BitBoard::bitCount(atk & wKingZone);
        m &= m-1;
    }
    r7 = pos.pieceTypeBB(Piece::BROOK) & 0xff00L;
    if (((r7 & (r7 - 1)) != 0) &&
        ((pos.pieceTypeBB(Piece::WKING) & 0xffL) != 0))
      score -= rookDouble7thRowBonus; // Two rooks on 2:nd row
    return score;
}

int
Evaluate::bishopEval(const Position& pos, int oldScore) {
    int score = 0;
    const U64 occupied = pos.occupiedBB();
    U64 wBishops = pos.pieceTypeBB(Piece::WBISHOP);
    U64 bBishops = pos.pieceTypeBB(Piece::BBISHOP);
    if ((wBishops | bBishops) == 0)
        return 0;
    U64 m = wBishops;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::bishopAttacks(sq, occupied);
        wAttacksBB |= atk;
        score += bishMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB() | bPawnAttacks))];
        if ((atk & bKingZone) != 0)
            bKingAttacks += BitBoard::bitCount(atk & bKingZone);
        m &= m-1;
    }
    m = bBishops;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::bishopAttacks(sq, occupied);
        bAttacksBB |= atk;
        score -= bishMobScore[BitBoard::bitCount(atk & ~(pos.blackBB() | wPawnAttacks))];
        if ((atk & wKingZone) != 0)
            wKingAttacks += BitBoard::bitCount(atk & wKingZone);
        m &= m-1;
    }

    bool whiteDark  = (pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq ) != 0;
    bool whiteLight = (pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskLightSq) != 0;
    bool blackDark  = (pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq ) != 0;
    bool blackLight = (pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskLightSq) != 0;
    int numWhite = (whiteDark ? 1 : 0) + (whiteLight ? 1 : 0);
    int numBlack = (blackDark ? 1 : 0) + (blackLight ? 1 : 0);

    // Bishop pair bonus
    if (numWhite == 2) {
        const int numPawns = pos.wMtrlPawns() / pV;
        score += bishopPairValue - numPawns * bishopPairPawnPenalty;
    }
    if (numBlack == 2) {
        const int numPawns = pos.bMtrlPawns() / pV;
        score -= bishopPairValue - numPawns * bishopPairPawnPenalty;
    }

    if ((numWhite == 1) && (numBlack == 1) && (whiteDark != blackDark) &&
        (pos.wMtrl() - pos.wMtrlPawns() == pos.bMtrl() - pos.bMtrlPawns())) {
        const int penalty = (oldScore + score) * oppoBishopPenalty / 128;
        score -= interpolate(penalty, 0, mhd->diffColorBishopIPF);
    } else {
        if (numWhite == 1) {
            U64 bishColorMask = whiteDark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
            U64 m = pos.pieceTypeBB(Piece::WPAWN) & bishColorMask;
            m |= (m << 8) & pos.pieceTypeBB(Piece::BPAWN);
            score -= 2 * BitBoard::bitCount(m);
        }
        if (numBlack == 1) {
            U64 bishColorMask = blackDark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
            U64 m = pos.pieceTypeBB(Piece::BPAWN) & bishColorMask;
            m |= (m >> 8) & pos.pieceTypeBB(Piece::WPAWN);
            score += 2 * BitBoard::bitCount(m);
        }
    }

    // Penalty for bishop trapped behind pawn at a2/h2/a7/h7
    if (((wBishops | bBishops) & 0x0081000000008100L) != 0) {
        const int bTrapped1 = trappedBishopPenalty1;
        const int bTrapped2 = trappedBishopPenalty2;
        if ((pos.getPiece(48) == Piece::WBISHOP) && // a7
            (pos.getPiece(41) == Piece::BPAWN) &&
            (pos.getPiece(50) == Piece::BPAWN))
            score -= bTrapped1;
        if ((pos.getPiece(55) == Piece::WBISHOP) && // h7
            (pos.getPiece(46) == Piece::BPAWN) &&
            (pos.getPiece(53) == Piece::BPAWN))
            score -= (pos.pieceTypeBB(Piece::WQUEEN) != 0) ? bTrapped2 : bTrapped1;
        if ((pos.getPiece(8)  == Piece::BBISHOP) &&  // a2
            (pos.getPiece(17) == Piece::WPAWN) &&
            (pos.getPiece(10) == Piece::WPAWN))
            score += bTrapped1;
        if ((pos.getPiece(15) == Piece::BBISHOP) && // h2
            (pos.getPiece(22) == Piece::WPAWN) &&
            (pos.getPiece(13) == Piece::WPAWN))
            score += (pos.pieceTypeBB(Piece::BQUEEN) != 0) ? bTrapped2 : bTrapped1;
    }

    return score;
}

int
Evaluate::knightEval(const Position& pos) {
    int score = 0;
    U64 wKnights = pos.pieceTypeBB(Piece::WKNIGHT);
    U64 bKnights = pos.pieceTypeBB(Piece::BKNIGHT);
    if ((wKnights | bKnights) == 0)
        return 0;

    // Knight mobility
    U64 m = wKnights;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::knightAttacks[sq];
        wAttacksBB |= atk;
        score += knightMobScore[sq][BitBoard::bitCount(atk & ~pos.whiteBB())];
        m &= m-1;
    }
    m = bKnights;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        U64 atk = BitBoard::knightAttacks[sq];
        bAttacksBB |= atk;
        score -= knightMobScore[sq][BitBoard::bitCount(atk & ~pos.blackBB())];
        m &= m-1;
    }

    m = wKnights & phd->outPostsW;
    if (m != 0) {
        int outPost = 0;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            outPost += knightOutpostBonus[63-sq];
            m &= m-1;
        }
        score += interpolate(0, outPost, mhd->wKnightOutPostIPF);
    }

    m = bKnights & phd->outPostsB;
    if (m != 0) {
        int outPost = 0;
        while (m != 0) {
            int sq = BitBoard::numberOfTrailingZeros(m);
            outPost += knightOutpostBonus[sq];
            m &= m-1;
        }
        score -= interpolate(0, outPost, mhd->bKnightOutPostIPF);
    }

    return score;
}

int
Evaluate::threatBonus(const Position& pos) {
    int score = 0;

    // Sum values for all black pieces under attack
    wAttacksBB &= pos.pieceTypeBB(Piece::BKNIGHT, Piece::BBISHOP, Piece::BROOK, Piece::BQUEEN);
    wAttacksBB |= wPawnAttacks;
    U64 m = wAttacksBB & pos.blackBB() & ~pos.pieceTypeBB(Piece::BKING);
    int tmp = 0;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        tmp += ::pieceValue[pos.getPiece(sq)];
        m &= m-1;
    }
    score += tmp + tmp * tmp / threatBonus2;

    // Sum values for all white pieces under attack
    bAttacksBB &= pos.pieceTypeBB(Piece::WKNIGHT, Piece::WBISHOP, Piece::WROOK, Piece::WQUEEN);
    bAttacksBB |= bPawnAttacks;
    m = bAttacksBB & pos.whiteBB() & ~pos.pieceTypeBB(Piece::WKING);
    tmp = 0;
    while (m != 0) {
        int sq = BitBoard::numberOfTrailingZeros(m);
        tmp += ::pieceValue[pos.getPiece(sq)];
        m &= m-1;
    }
    score -= tmp + tmp * tmp / threatBonus2;
    return score / threatBonus1;
}

/** Compute king safety for both kings. */
int
Evaluate::kingSafety(const Position& pos) {
    const int minM = rV + bV;
    const int m = (pos.wMtrl() - pos.wMtrlPawns() + pos.bMtrl() - pos.bMtrlPawns()) / 2;
    if (m <= minM)
        return 0;
    int score = kingSafetyKPPart(pos);
    if (Position::getY(pos.wKingSq()) == 0) {
        if (((pos.pieceTypeBB(Piece::WKING) & 0x60L) != 0) && // King on f1 or g1
            ((pos.pieceTypeBB(Piece::WROOK) & 0xC0L) != 0) && // Rook on g1 or h1
            ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[6]) != 0) &&
            ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[7]) != 0)) {
            score -= trappedRookPenalty;
        } else
        if (((pos.pieceTypeBB(Piece::WKING) & 0x6L) != 0) && // King on b1 or c1
            ((pos.pieceTypeBB(Piece::WROOK) & 0x3L) != 0) && // Rook on a1 or b1
            ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[0]) != 0) &&
            ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[1]) != 0)) {
            score -= trappedRookPenalty;
        }
    }
    if (Position::getY(pos.bKingSq()) == 7) {
        if (((pos.pieceTypeBB(Piece::BKING) & 0x6000000000000000L) != 0) && // King on f8 or g8
            ((pos.pieceTypeBB(Piece::BROOK) & 0xC000000000000000L) != 0) && // Rook on g8 or h8
            ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[6]) != 0) &&
            ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[7]) != 0)) {
            score += trappedRookPenalty;
        } else
        if (((pos.pieceTypeBB(Piece::BKING) & 0x600000000000000L) != 0) && // King on b8 or c8
            ((pos.pieceTypeBB(Piece::BROOK) & 0x300000000000000L) != 0) && // Rook on a8 or b8
            ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[0]) != 0) &&
            ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[1]) != 0)) {
            score += trappedRookPenalty;
        }
    }
    score += (bKingAttacks - wKingAttacks) * kingAttackWeight;
    const int kSafety = interpolate(0, score, mhd->kingSafetyIPF);
    return kSafety;
}

int
Evaluate::kingSafetyKPPart(const Position& pos) {
    const U64 key = pos.pawnZobristHash() ^ pos.kingZobristHash();
    KingSafetyHashData& ksh = kingSafetyHash[(int)key & (kingSafetyHash.size() - 1)];
    if (ksh.key != key) {
        int score = 0;
        const U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
        const U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
        { // White pawn shelter bonus
            int safety = 0;
            int halfOpenFiles = 0;
            if (Position::getY(pos.wKingSq()) < 2) {
                U64 shelter = 1ULL << Position::getX(pos.wKingSq());
                shelter |= ((shelter & BitBoard::maskBToHFiles) >> 1) |
                           ((shelter & BitBoard::maskAToGFiles) << 1);
                shelter <<= 8;
                safety += 3 * BitBoard::bitCount(wPawns & shelter);
                safety -= 2 * BitBoard::bitCount(bPawns & (shelter | (shelter << 8)));
                shelter <<= 8;
                safety += 2 * BitBoard::bitCount(wPawns & shelter);
                shelter <<= 8;
                safety -= BitBoard::bitCount(bPawns & shelter);

                U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                if (wOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG * BitBoard::bitCount(wOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH * BitBoard::bitCount(wOpen & 0x18);
                }
                U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                if (bOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG * BitBoard::bitCount(bOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH * BitBoard::bitCount(bOpen & 0x18);
                }
                safety = std::min(safety, 8);
            }
            const int kSafety = (safety - 9) * kingSafetyWeight - halfOpenFiles;
            score += kSafety;
        }
        { // Black pawn shelter bonus
            int safety = 0;
            int halfOpenFiles = 0;
            if (Position::getY(pos.bKingSq()) >= 6) {
                U64 shelter = 1ULL << (56 + Position::getX(pos.bKingSq()));
                shelter |= ((shelter & BitBoard::maskBToHFiles) >> 1) |
                           ((shelter & BitBoard::maskAToGFiles) << 1);
                shelter >>= 8;
                safety += 3 * BitBoard::bitCount(bPawns & shelter);
                safety -= 2 * BitBoard::bitCount(wPawns & (shelter | (shelter >> 8)));
                shelter >>= 8;
                safety += 2 * BitBoard::bitCount(bPawns & shelter);
                shelter >>= 8;
                safety -= BitBoard::bitCount(wPawns & shelter);

                U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                if (wOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG * BitBoard::bitCount(wOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH * BitBoard::bitCount(wOpen & 0x18);
                }
                U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                if (bOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG * BitBoard::bitCount(bOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH * BitBoard::bitCount(bOpen & 0x18);
                }
                safety = std::min(safety, 8);
            }
            const int kSafety = (safety - 9) * kingSafetyWeight - halfOpenFiles;
            score -= kSafety;
        }
        // Pawn storm bonus
        static const int kingZone[8] = {0,0,0, 1,1, 2,2,2};
        static const U64 pStormMask[3] = { 0x0707070707070707ULL, 0, 0xE0E0E0E0E0E0E0E0ULL };
        const int wKingZone = kingZone[Position::getX(pos.wKingSq())];
        const int bKingZone = kingZone[Position::getX(pos.bKingSq())];
        const int kingDiff = std::abs(wKingZone - bKingZone);
        if (kingDiff > 1) {
            U64 m = wPawns & pStormMask[bKingZone];
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                score += pawnStormBonus * (Position::getY(sq)-5);
                m &= m - 1;
            }
            m = bPawns & pStormMask[wKingZone];
            while (m != 0) {
                int sq = BitBoard::numberOfTrailingZeros(m);
                score += pawnStormBonus * (Position::getY(sq)-2);
                m &= m - 1;
            }
        }

        ksh.key = key;
        ksh.score = score;
    }
    return ksh.score;
}

static int
mateEval(int k1, int k2) {
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

/** Implements special knowledge for some endgame situations. */
template <bool doEval>
int
Evaluate::endGameEval(const Position& pos, int oldScore) const {
    int score = oldScore;
    const int wMtrlPawns = pos.wMtrlPawns();
    const int bMtrlPawns = pos.bMtrlPawns();
    const int wMtrlNoPawns = pos.wMtrl() - wMtrlPawns;
    const int bMtrlNoPawns = pos.bMtrl() - bMtrlPawns;

    // Handle special endgames
    typedef MatId MI;
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
        int wq = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WQUEEN));
        int bk = pos.getKingSq(false);
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        return kqkpEval(wk, wq, bk, bp, pos.getWhiteMove(), score);
    }
    case MI::BQ + MI::WP: {
        if (!doEval) return 1;
        int bk = pos.getKingSq(false);
        int bq = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BQUEEN));
        int wk = pos.getKingSq(true);
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        return -kqkpEval(63-bk, 63-bq, 63-wk, 63-wp, !pos.getWhiteMove(), -score);
    }
    case MI::WR + MI::BP: {
        if (!doEval) return 1;
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        return krkpEval(pos.getKingSq(true), pos.getKingSq(false),
                        bp, pos.getWhiteMove(), score);
    }
    case MI::BR + MI::WP: {
        if (!doEval) return 1;
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        return -krkpEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                         63-wp, !pos.getWhiteMove(), -score);
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
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        int wr = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WROOK));
        int br = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BROOK));
        return krpkrEval(wk, bk, wp, wr, br, pos.getWhiteMove());
    }
    case MI::BR + MI::BP + MI::WR: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        int wr = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WROOK));
        int br = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BROOK));
        return -krpkrEval(63-bk, 63-wk, 63-bp, 63-br, 63-wr, !pos.getWhiteMove());
    }
    case MI::WR + MI::WP + MI::BR + MI::BP: {
        if (!doEval) return 1;
        int wk = pos.getKingSq(true);
        int bk = pos.getKingSq(false);
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        int wr = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WROOK));
        int br = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BROOK));
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        return krpkrpEval(wk, bk, wp, wr, br, bp, pos.getWhiteMove(), score);
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
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        return kpkEval(pos.getKingSq(true), pos.getKingSq(false),
                       wp, pos.getWhiteMove());
    }
    case MI::BP: {
        if (!doEval) return 1;
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        return -kpkEval(63-pos.getKingSq(false), 63-pos.getKingSq(true),
                        63-bp, !pos.getWhiteMove());
    }
    case MI::WB + MI::WP + MI::BB: {
        if (!doEval) return 1;
        int wb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WBISHOP));
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        int bb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BBISHOP));
        return kbpkbEval(pos.getKingSq(true), wb, wp, pos.getKingSq(false), bb, score);
    }
    case MI::BB + MI::BP + MI::WB: {
        if (!doEval) return 1;
        int bb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BBISHOP));
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        int wb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WBISHOP));
        return -kbpkbEval(63-pos.getKingSq(false), 63-bb, 63-bp, 63-pos.getKingSq(true), 63-wb, -score);
    }
    case MI::WB + MI::WP + MI::BN: {
        if (!doEval) return 1;
        int wb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WBISHOP));
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        int bn = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BKNIGHT));
        return kbpknEval(pos.getKingSq(true), wb, wp, pos.getKingSq(false), bn, score);
    }
    case MI::BB + MI::BP + MI::WN: {
        if (!doEval) return 1;
        int bb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BBISHOP));
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        int wn = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WKNIGHT));
        return -kbpknEval(63-pos.getKingSq(false), 63-bb, 63-bp, 63-pos.getKingSq(true), 63-wn, -score);
    }
    case MI::WN + MI::WP + MI::BB: {
        if (!doEval) return 1;
        int wn = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WKNIGHT));
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        int bb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BBISHOP));
        return knpkbEval(pos.getKingSq(true), wn, wp, pos.getKingSq(false), bb,
                         score, pos.getWhiteMove());
    }
    case MI::BN + MI::BP + MI::WB: {
        if (!doEval) return 1;
        int bn = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BKNIGHT));
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        int wb = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WBISHOP));
        return -knpkbEval(63-pos.getKingSq(false), 63-bn, 63-bp, 63-pos.getKingSq(true), 63-wb,
                          -score, !pos.getWhiteMove());
    }
    case MI::WN + MI::WP: {
        if (!doEval) return 1;
        int wn = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WKNIGHT));
        int wp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::WPAWN));
        return knpkEval(pos.getKingSq(true), wn, wp, pos.getKingSq(false),
                        score, pos.getWhiteMove());
    }
    case MI::BN + MI::BP: {
        if (!doEval) return 1;
        int bn = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BKNIGHT));
        int bp = BitBoard::numberOfTrailingZeros(pos.pieceTypeBB(Piece::BPAWN));
        return -knpkEval(63-pos.getKingSq(false), 63-bn, 63-bp, 63-pos.getKingSq(true),
                         -score, !pos.getWhiteMove());
    }
    }

    const int nWN = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    const int nBN = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));
    const int nWB1 = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskLightSq);
    const int nWB2 = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq);
    const int nBB1 = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskLightSq);
    const int nBB2 = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq);
    const int qV = ::qV;

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
        return 400 + pos.wMtrl() - pos.bMtrl() + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if ((pos.wMtrl() == 0) && pos.pieceTypeBB(Piece::BROOK)) {
        if (!doEval) return 1;
        return -(400 + pos.bMtrl() - pos.wMtrl() + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
    }

    // Bonus for KQK[BN]
    const int bV = ::bV;
    const int nV = ::nV;
    if (pos.pieceTypeBB(Piece::WQUEEN) && (pos.bMtrlPawns() == 0) && (pos.bMtrl() <= std::max(bV,nV))) {
        if (!doEval) return 1;
        return 200 + pos.wMtrl() - pos.bMtrl() + mateEval(pos.getKingSq(true), pos.getKingSq(false));
    }
    if (pos.pieceTypeBB(Piece::BQUEEN) && (pos.wMtrlPawns() == 0) && (pos.wMtrl() <= std::max(bV,nV))) {
        if (!doEval) return 1;
        return -(200 + pos.bMtrl() - pos.wMtrl() + mateEval(pos.getKingSq(false), pos.getKingSq(true)));
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

    // Only bishops on same color can not win
    if (pos.pieceTypeBB(Piece::WROOK, Piece::WKNIGHT, Piece::WQUEEN, Piece::WPAWN) == 0) {
        if (!doEval) return 1;
        if (score > 0) {
            if (((pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq) == 0) ||
                ((pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskLightSq) == 0))
                return 0;
        }
    }
    if (pos.pieceTypeBB(Piece::BROOK, Piece::BKNIGHT, Piece::BQUEEN, Piece::BPAWN) == 0) {
        if (!doEval) return 1;
        if (score < 0) {
            if (((pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq) == 0) ||
                ((pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskLightSq) == 0))
                return 0;
        }
    }

    // Give bonus/penalty if advantage is/isn't large enough to win
    if ((wMtrlPawns == 0) && (wMtrlNoPawns <= bMtrlNoPawns + bV)) {
        if (!doEval) return 1;
        if (score > 0) {
            if (wMtrlNoPawns < rV)
                return -pos.bMtrl() / 50;
            else
                return score / 8;        // Too little excess material, probably draw
        }
    } else if (pos.pieceTypeBB(Piece::WROOK, Piece::WKNIGHT, Piece::WQUEEN) == 0) {
        if (!doEval) return 1;
        if (score > 0) {
            // Check for rook pawn + wrong color bishop
            if (((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskBToHFiles) == 0) &&
                ((pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskLightSq) == 0) &&
                ((pos.pieceTypeBB(Piece::BKING) & 0x0303000000000000ULL) != 0)) {
                return 0;
            } else
            if (((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskAToGFiles) == 0) &&
                ((pos.pieceTypeBB(Piece::WBISHOP) & BitBoard::maskDarkSq) == 0) &&
                ((pos.pieceTypeBB(Piece::BKING) & 0xC0C0000000000000ULL) != 0)) {
                return 0;
            }
        }
    }
    if ((bMtrlPawns == 0) && (bMtrlNoPawns <= wMtrlNoPawns + bV)) {
        if (!doEval) return 1;
        if (score < 0) {
            if (bMtrlNoPawns < rV)
                return pos.wMtrl() / 50;
            else
                return score / 8;        // Too little excess material, probably draw
        }
    } else if (pos.pieceTypeBB(Piece::BROOK, Piece::BKNIGHT, Piece::BQUEEN) == 0) {
        if (!doEval) return 1;
        if (score < 0) {
            // Check for rook pawn + wrong color bishop
            if (((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskBToHFiles) == 0) &&
                ((pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskDarkSq) == 0) &&
                ((pos.pieceTypeBB(Piece::WKING) & 0x0303ULL) != 0)) {
                return 0;
            } else
            if (((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskAToGFiles) == 0) &&
                ((pos.pieceTypeBB(Piece::BBISHOP) & BitBoard::maskLightSq) == 0) &&
                ((pos.pieceTypeBB(Piece::WKING) & 0xC0C0ULL) != 0)) {
                return 0;
            }
        }
    }

    if ((bMtrlPawns == 0) && (wMtrlNoPawns - bMtrlNoPawns > bV)) {
        if (!doEval) return 1;
        return score + 300;       // Enough excess material, should win
    }
    if ((wMtrlPawns == 0) && (bMtrlNoPawns - wMtrlNoPawns > bV)) {
        if (!doEval) return 1;
        return score - 300;       // Enough excess material, should win
    }

    if (!doEval) return 0;
    return score;
}

int
Evaluate::kqkpEval(int wKing, int wQueen, int bKing, int bPawn, bool whiteMove, int score) {
    bool canWin = false;
    if (((1ULL << bKing) & 0xFFFF) == 0) {
        canWin = true; // King doesn't support pawn
    } else if (std::abs(Position::getX(bPawn) - Position::getX(bKing)) > 2) {
        canWin = true; // King doesn't support pawn
    } else {
        switch (bPawn) {
        case 8:  // a2
            canWin = ((1ULL << wKing) & 0x0F1F1F1F1FULL) != 0;
            if (canWin && (bKing == 0) && (Position::getX(wQueen) == 1) && !whiteMove)
                canWin = false; // Stale-mate
            break;
        case 10: // c2
            canWin = ((1ULL << wKing) & 0x071F1F1FULL) != 0;
            break;
        case 13: // f2
            canWin = ((1ULL << wKing) & 0xE0F8F8F8ULL) != 0;
            break;
        case 15: // h2
            canWin = ((1ULL << wKing) & 0xF0F8F8F8F8ULL) != 0;
            if (canWin && (bKing == 7) && (Position::getX(wQueen) == 6) && !whiteMove)
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
Evaluate::kpkEval(int wKing, int bKing, int wPawn, bool whiteMove) {
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

int
Evaluate::krkpEval(int wKing, int bKing, int bPawn, bool whiteMove, int score) {
    if (Position::getX(bKing) >= 4) { // Mirror X
        wKing ^= 7;
        bKing ^= 7;
        bPawn ^= 7;
    }
    int index = whiteMove ? 0 : 1;
    index = index * 32 + Position::getY(bKing)*4+Position::getX(bKing);
    index = index * 48 + bPawn - 8;
    index = index * 8 + Position::getY(wKing);
    byte mask = krkpTable[index];
    bool canWin = (mask & (1 << Position::getX(wKing))) != 0;

    score = score + Position::getY(bPawn) * pV / 4;
    if (!canWin)
        score /= 50;
    return score;
}

int
Evaluate::krpkrEval(int wKing, int bKing, int wPawn, int wRook, int bRook, bool whiteMove) {
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
Evaluate::krpkrpEval(int wKing, int bKing, int wPawn, int wRook, int bRook, int bPawn, bool whiteMove, int score) {
    int hiScore = krpkrEval(wKing, bKing, wPawn, wRook, bRook, whiteMove);
    if (score > hiScore * 14 / 16)
        return hiScore * 14 / 16;
    int loScore = -krpkrEval(63-bKing, 63-wKing, 63-bPawn, 63-bRook, 63-wRook, !whiteMove);
    if (score < loScore * 14 / 16)
        return loScore * 14 / 16;
    return score;
}

int
Evaluate::kbnkEval(int wKing, int bKing, bool darkBishop) {
    int score = 600;
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
Evaluate::kbpkbEval(int wKing, int wBish, int wPawn, int bKing, int bBish, int score) {
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
            if (!(((wPawn == 40) && (bBish == 57)) || ((wPawn == 47) && (bBish == 62))))
                return 0;
    }

    if (bKingMask & BitBoard::wPawnBlockerMask[wPawn])
        return score / 4;
    return score;
}

int
Evaluate::kbpknEval(int wKing, int wBish, int wPawn, int bKing, int bKnight, int score) {
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
Evaluate::knpkbEval(int wKing, int wKnight, int wPawn, int bKing, int bBish, int score, bool wtm) {
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
Evaluate::knpkEval(int wKing, int wKnight, int wPawn, int bKing, int score, bool wtm) {
    if (Position::getX(wPawn) >= 4) { // Mirror X
        wKing ^= 7;
        wKnight ^= 7;
        wPawn ^= 7;
        bKing ^= 7;
    }
    if (wPawn == 48) { // a7
        if ((bKing == 56) || (bKing == 49)) // Fortress
            return 0;
        if ((wKing == 56) && ((bKing == 50) || (bKing == 58))) {
            bool knightDark = Position::darkSquare(Position::getX(wKnight), Position::getY(wKnight));
            bool kingDark = Position::darkSquare(Position::getX(bKing), Position::getY(bKing));
            if (wtm == (knightDark == kingDark)) // King trapped
                return 0;
        }
    }
    return score;
}

std::shared_ptr<Evaluate::EvalHashTables>
Evaluate::getEvalHashTables() {
    return std::make_shared<EvalHashTables>();
}
