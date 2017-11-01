/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2016  Peter Österlund, peterosterlund2@gmail.com

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
#include "endGameEval.hpp"
#include "constants.hpp"
#include "parameters.hpp"
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

int Evaluate::castleMaskFactor[256];

static StaticInitializer<Evaluate> evInit;


/** Get bitboard mask for a square translated (dx,dy). Return 0 if square outside board. */
static inline U64 getMask(int sq, int dx, int dy) {
    int x = Position::getX(sq) + dx;
    int y = Position::getY(sq) + dy;
    if (x >= 0 && x < 8 && y >= 0 && y < 8)
        return 1ULL << Position::getSquare(x, y);
    else
        return 0;
}

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
    psTab2[Piece::WQUEEN]  = qt2w.getTable();
    psTab2[Piece::WROOK]   = rt1w.getTable();
    psTab2[Piece::WBISHOP] = bt2w.getTable();
    psTab2[Piece::WKNIGHT] = nt2w.getTable();
    psTab2[Piece::WPAWN]   = pt2w.getTable();
    psTab2[Piece::BKING]   = kt2b.getTable();
    psTab2[Piece::BQUEEN]  = qt2b.getTable();
    psTab2[Piece::BROOK]   = rt1b.getTable();
    psTab2[Piece::BBISHOP] = bt2b.getTable();
    psTab2[Piece::BKNIGHT] = nt2b.getTable();
    psTab2[Piece::BPAWN]   = pt2b.getTable();

    // Initialize knight/bishop king safety patterns
    for (int sq = 0; sq < 64; sq++) {
        const int x = Position::getX(sq);
        const int y = Position::getY(sq);
        int dx = (x < 4) ? -1 : 1;
        int dy = (y < 4) ? 1 : -1;
        U64 n = getMask(sq, -dx, 0) | getMask(sq, dx, 0) | getMask(sq, 0, dy) | getMask(sq, 0, 2*dy) | getMask(sq, dx, 2*dy);
        U64 b = getMask(sq, -dx, 0) | getMask(sq, 0, dy) | getMask(sq, dx, 2*dy);
        knightKingProtectPattern[sq] = n;
        bishopKingProtectPattern[sq] = b;
    }
}

void
Evaluate::updateEvalParams() {
    // Castle bonus
    for (int i = 0; i < 256; i++) {
        int h1Dist = 100;
        bool h1Castle = (i & (1<<7)) != 0;
        if (h1Castle)
            h1Dist = BitBoard::bitCount(i & BitBoard::sqMask(F1,G1));
        int a1Dist = 100;
        bool a1Castle = (i & 1) != 0;
        if (a1Castle)
            a1Dist = BitBoard::bitCount(i & BitBoard::sqMask(B1,C1,D1));
        int dist = std::min(a1Dist, h1Dist);
        castleMaskFactor[i] = dist < 4 ? castleFactor[dist] : 0;
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
        case A1: maxMob = 2; break;
        case B1: maxMob = 3; break;
        case C1: maxMob = 4; break;
        case D1: maxMob = 4; break;
        case B2: maxMob = 4; break;
        case C2: maxMob = 6; break;
        case D2: maxMob = 6; break;
        case C3: maxMob = 8; break;
        case D3: maxMob = 8; break;
        case D4: maxMob = 8; break;
        default:
            assert(false);
        }
        for (int m = 0; m <= 8; m++) {
            int offs = 0;
            switch (maxMob) {
            case 2: offs = 0; break;
            case 3: offs = 3; break;
            case 4: offs = 7; break;
            case 6: offs = 12; break;
            case 8: offs = 19; break;
            }
            knightMobScoreA[sq][m] = knightMobScore[offs + std::min(m, maxMob)];
        }
    }
}

const int* Evaluate::psTab1[Piece::nPieceTypes];
const int* Evaluate::psTab2[Piece::nPieceTypes];

int Evaluate::knightMobScoreA[64][9];
U64 Evaluate::knightKingProtectPattern[64];
U64 Evaluate::bishopKingProtectPattern[64];

Evaluate::Evaluate(EvalHashTables& et)
    : pawnHash(et.pawnHash),
      materialHash(et.materialHash),
      kingSafetyHash(et.kingSafetyHash),
      evalHash(et.evalHash),
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
    const bool useHashTable = !print;
    EvalHashData* ehd = nullptr;
    U64 key = pos.historyHash();
    if (useHashTable) {
        ehd = &getEvalHashEntry(evalHash, key);
        if ((ehd->data ^ key) < (1 << 16))
            return (ehd->data & 0xffff) - (1 << 15);
    }

    int score = materialScore(pos, print);

    wKingAttacks = bKingAttacks = 0;
    wKingZone = BitBoard::kingAttacks[pos.getKingSq(true)]; wKingZone |= wKingZone << 8;
    bKingZone = BitBoard::kingAttacks[pos.getKingSq(false)]; bKingZone |= bKingZone >> 8;
    wAttacksBB = bAttacksBB = 0;
    wQueenContactChecks = bQueenContactChecks = 0;
    wContactSupport = bContactSupport = 0;

    wPawnAttacks = BitBoard::wPawnAttacksMask(pos.pieceTypeBB(Piece::WPAWN));
    bPawnAttacks = BitBoard::bPawnAttacksMask(pos.pieceTypeBB(Piece::BPAWN));

    score += pieceSquareEval(pos);
    if (print) std::cout << "info string eval pst    :" << score << std::endl;
    score += pawnBonus(pos);
    if (print) std::cout << "info string eval pawn   :" << score << std::endl;
    score += castleBonus(pos);
    if (print) std::cout << "info string eval castle :" << score << std::endl;

    score += rookBonus(pos);
    if (print) std::cout << "info string eval rook   :" << score << std::endl;
    score += bishopEval(pos, score);
    if (print) std::cout << "info string eval bishop :" << score << std::endl;
    score += knightEval(pos);
    if (print) std::cout << "info string eval knight :" << score << std::endl;
    score += threatBonus(pos);
    if (print) std::cout << "info string eval threat :" << score << std::endl;
    score += protectBonus(pos);
    if (print) std::cout << "info string eval protect:" << score << std::endl;
    score += kingSafety(pos);
    if (print) std::cout << "info string eval king   :" << score << std::endl;
    if (mhd->endGame)
        score = EndGameEval::endGameEval<true>(pos, phd->passedPawns, score);
    if (print) std::cout << "info string eval endgame:" << score << std::endl;
    if (pos.pieceTypeBB(Piece::WPAWN, Piece::BPAWN)) {
        int hmc = clamp(pos.getHalfMoveClock() / 10, 0, 9);
        score = score * halfMoveFactor[hmc] / 128;
    }
    if (print) std::cout << "info string eval halfmove:" << score << std::endl;
    if (score > 0) {
        int nStale = BitBoard::bitCount(BitBoard::southFill(phd->stalePawns & pos.pieceTypeBB(Piece::WPAWN)) & 0xff);
        score = score * stalePawnFactor[nStale] / 128;
    } else if (score < 0) {
        int nStale = BitBoard::bitCount(BitBoard::southFill(phd->stalePawns & pos.pieceTypeBB(Piece::BPAWN)) & 0xff);
        score = score * stalePawnFactor[nStale] / 128;
    }
    if (print) std::cout << "info string eval staleP :" << score << std::endl;

    if (!pos.isWhiteMove())
        score = -score;

    if (useHashTable)
        ehd->data = (key & 0xffffffffffff0000ULL) + (score + (1 << 15));

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
    if (print) std::cout << "info string eval mtrlraw:" << score << std::endl;
    const int nWQ = BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN));
    const int nBQ = BitBoard::bitCount(pos.pieceTypeBB(Piece::BQUEEN));
    const int nWN = BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT));
    const int nBN = BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT));
    int wCorr = correctionNvsQ(nWN, nBQ);
    int bCorr = correctionNvsQ(nBN, nWQ);
    score += wCorr - bCorr;
    if (print) std::cout << "info string eval qncorr :" << score << std::endl;
    score += tradeBonus(pos, wCorr, bCorr);
    if (print) std::cout << "info string eval trade  :" << score << std::endl;

    const int nWR = BitBoard::bitCount(pos.pieceTypeBB(Piece::WROOK));
    const int nBR = BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK));
    { // Redundancy of major pieces
        int wMajor = nWQ + nWR;
        int bMajor = nBQ + nBR;
        int w = std::min(wMajor, 3);
        int b = std::min(bMajor, 3);
        score += majorPieceRedundancy[w*4+b];
    }
    if (print) std::cout << "info string eval majred :" << score << std::endl;

    const int wMtrl = pos.wMtrl();
    const int bMtrl = pos.bMtrl();
    const int wMtrlPawns = pos.wMtrlPawns();
    const int bMtrlPawns = pos.bMtrlPawns();
    const int wMtrlNoPawns = wMtrl - wMtrlPawns;
    const int bMtrlNoPawns = bMtrl - bMtrlPawns;

    // Handle imbalances
    const int nWB = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP));
    const int nBB = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP));
    const int nWP = BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN));
    const int nBP = BitBoard::bitCount(pos.pieceTypeBB(Piece::BPAWN));
    {
        const int dQ = nWQ - nBQ;
        const int dR = nWR - nBR;
        const int dB = nWB - nBB;
        const int dN = nWN - nBN;
        int nMinor = nWB + nWN + nBB + nBN;
        if ((dQ == 1) && (dR == -2)) {
            score += QvsRRBonus[std::min(4, nMinor)];
        } else if ((dQ == -1) && (dR == 2)) {
            score -= QvsRRBonus[std::min(4, nMinor)];
        }

        const int dP = nWP - nBP;
        if ((dR == 1) && (dB + dN == -1)) {
            score += RvsMBonus[clamp(dP, -3, 3)+3];
            if (wMtrlNoPawns == rV && dB == -1 && dP == -1)
                score += RvsBPBonus;
        } else if ((dR == -1) && (dB + dN == 1)) {
            score -= RvsMBonus[clamp(-dP, -3, 3)+3];
            if (bMtrlNoPawns == rV && dB == 1 && dP == 1)
                score -= RvsBPBonus;
        }

        if ((dR == 1) && (dB + dN == -2)) {
            score += RvsMMBonus[clamp(dP, -3, 3)+3];
        } else if ((dR == -1) && (dB + dN == 2)) {
            score -= RvsMMBonus[clamp(-dP, -3, 3)+3];
        }

        if ((dQ == 1) && (dR == -1) && (dB + dN == -1)) {
            score += (nWR == 0) ? QvsRMBonus1 : QvsRMBonus2;
        } else if ((dQ == -1) && (dR == 1) && (dB + dN == 1)) {
            score -= (nBR == 0) ? QvsRMBonus1 : QvsRMBonus2;
        }
    }
    if (print) std::cout << "info string eval imbala :" << score << std::endl;
    mhd.id = pos.materialId();
    mhd.score = score;
    mhd.endGame = EndGameEval::endGameEval<false>(pos, 0, 0);

    // Compute interpolation factors
    { // Pawn
        const int loMtrl = pawnLoMtrl;
        const int hiMtrl = pawnHiMtrl;
        mhd.pawnIPF = interpolate(wMtrlNoPawns + bMtrlNoPawns, loMtrl, 0, hiMtrl, IPOLMAX);
        if (wCorr + bCorr > 200)
            mhd.pawnIPF = mhd.pawnIPF * 200 / (wCorr + bCorr);
    }
    { // Knight/bishop
        const int loMtrl = minorLoMtrl;
        const int hiMtrl = minorHiMtrl;
        mhd.knightIPF = interpolate(wMtrl + bMtrl, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    { // Castle
        const int loMtrl = castleLoMtrl;
        const int hiMtrl = castleHiMtrl;
        const int m = wMtrlNoPawns + bMtrlNoPawns;
        mhd.castleIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
    }
    {
        const int loMtrl = queenLoMtrl;
        const int hiMtrl = queenHiMtrl;
        const int m = wMtrlNoPawns + bMtrlNoPawns;
        mhd.queenIPF = interpolate(m, loMtrl, 0, hiMtrl, IPOLMAX);
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
        mhd.knightOutPostIPF = interpolate(wMtrlPawns + bMtrlPawns, loMtrl, 0, hiMtrl, IPOLMAX);
    }
}

int
Evaluate::tradeBonus(const Position& pos, int wCorr, int bCorr) const {
    const int wM = pos.wMtrl() + wCorr;
    const int bM = pos.bMtrl() + bCorr;
    const int wPawn = pos.wMtrlPawns();
    const int bPawn = pos.bMtrlPawns();
    const int deltaScore = wM - bM;

    int pBonus = deltaScore * 11 / 128;
    pBonus += interpolate((deltaScore > 0) ? wPawn : bPawn, 0, -pawnTradePenalty * deltaScore / 100, pawnTradeThreshold, 0);

    return pBonus;
}

int
Evaluate::pieceSquareEval(const Position& pos) {
    int score = 0;

    // Kings/pawns
    if (pos.wMtrlPawns() + pos.bMtrlPawns() > 0) {
        const int k1 = (pos.psScore1(Piece::WKING) + pos.psScore1(Piece::WPAWN)) -
                       (pos.psScore1(Piece::BKING) + pos.psScore1(Piece::BPAWN));
        const int k2 = (pos.psScore2(Piece::WKING) + pos.psScore2(Piece::WPAWN)) -
                       (pos.psScore2(Piece::BKING) + pos.psScore2(Piece::BPAWN));
        score += interpolate(k2, k1, mhd->pawnIPF);
    } else { // Use symmetric tables if no pawns left
        if (pos.wMtrl() > pos.bMtrl())
            score += EndGameEval::mateEval(pos.getKingSq(true), pos.getKingSq(false));
        else if (pos.wMtrl() < pos.bMtrl())
            score -= EndGameEval::mateEval(pos.getKingSq(false), pos.getKingSq(true));
        else
            score += EndGameEval::winKingTable[pos.getKingSq(true)] -
                     EndGameEval::winKingTable[pos.getKingSq(false)];
    }

    // Knights/bishops
    {
        int n1 = (pos.psScore1(Piece::WKNIGHT) + pos.psScore1(Piece::WBISHOP)) -
                 (pos.psScore1(Piece::BKNIGHT) + pos.psScore1(Piece::BBISHOP));
        int n2 = (pos.psScore2(Piece::WKNIGHT) + pos.psScore2(Piece::WBISHOP)) -
                 (pos.psScore2(Piece::BKNIGHT) + pos.psScore2(Piece::BBISHOP));
        score += interpolate(n2, n1, mhd->knightIPF);
    }

    // Queens
    {
        const U64 occupied = pos.occupiedBB();
        int q1 = pos.psScore1(Piece::WQUEEN) - pos.psScore1(Piece::BQUEEN);
        int q2 = pos.psScore2(Piece::WQUEEN) - pos.psScore2(Piece::BQUEEN);
        score += interpolate(q2, q1, mhd->queenIPF);
        U64 m = pos.pieceTypeBB(Piece::WQUEEN);
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
            wAttacksBB |= atk;
            score += queenMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB() | bPawnAttacks))];
            bKingAttacks += BitBoard::bitCount(atk & bKingZone) * 2;
            wQueenContactChecks = atk & BitBoard::kingAttacks[pos.bKingSq()];
        }
        m = pos.pieceTypeBB(Piece::BQUEEN);
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            U64 atk = BitBoard::rookAttacks(sq, occupied) | BitBoard::bishopAttacks(sq, occupied);
            bAttacksBB |= atk;
            score -= queenMobScore[BitBoard::bitCount(atk & ~(pos.blackBB() | wPawnAttacks))];
            wKingAttacks += BitBoard::bitCount(atk & wKingZone) * 2;
            bQueenContactChecks = atk & BitBoard::kingAttacks[pos.wKingSq()];
        }
    }

    // Rooks
    {
        int r1 = pos.psScore1(Piece::WROOK);
        if (r1 != 0) {
            const int nP = BitBoard::bitCount(pos.pieceTypeBB(Piece::BPAWN));
            const int s = r1 * std::min(nP, 6) / 6;
            score += s;
        }
        r1 = pos.psScore1(Piece::BROOK);
        if (r1 != 0) {
            const int nP = BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN));
            const int s = r1 * std::min(nP, 6) / 6;
            score -= s;
        }
    }

    return score;
}

int
Evaluate::castleBonus(const Position& pos) {
    if (pos.getCastleMask() == 0) return 0;

    const int k1 = kt1b[G8] - kt1b[E8];
    const int k2 = kt2b[G8] - kt2b[E8];
    const int ks = interpolate(k2, k1, mhd->castleIPF);

    const int castleValue = ks + rt1b[F8] - rt1b[H8];
    if (castleValue <= 0)
        return 0;

    U64 occupied = pos.occupiedBB();
    int tmp = (int) (occupied & BitBoard::sqMask(B1,C1,D1,F1,G1));
    if (pos.a1Castle()) tmp |= 1;
    if (pos.h1Castle()) tmp |= (1 << 7);
    const int wBonus = (castleValue * castleMaskFactor[tmp]) >> 7;

    tmp = (int) ((occupied >> 56) & BitBoard::sqMask(B1,C1,D1,F1,G1));
    if (pos.a8Castle()) tmp |= 1;
    if (pos.h8Castle()) tmp |= (1 << 7);
    const int bBonus = (castleValue * castleMaskFactor[tmp]) >> 7;

    return wBonus - bBonus;
}

int
Evaluate::pawnBonus(const Position& pos) {
    U64 key = pos.pawnZobristHash();
    PawnHashData& phd = getPawnHashEntry(pawnHash, key);
    if (phd.key != key)
        computePawnHashData(pos, phd);
    this->phd = &phd;
    int score = phd.score;

    // Bonus for own king supporting passed pawns
    int passedScore = phd.passedBonusW;
    const U64 passedPawnsW = phd.passedPawns & pos.pieceTypeBB(Piece::WPAWN);
    U64 m = passedPawnsW;
    if (m != 0) {
        U64 kMask = pos.pieceTypeBB(Piece::WKING);
        int ky = Position::getY(pos.getKingSq(true));
        if ((m << 8) & kMask)
            passedScore += kingPPSupportK[0] * kingPPSupportP[ky-1] / 32;
        else if ((m << 16) & kMask)
            passedScore += kingPPSupportK[1] * kingPPSupportP[ky-2] / 32;
        m = ((m & BitBoard::maskAToGFiles) << 1) | ((m & BitBoard::maskBToHFiles) >> 1);
        if (m & kMask)
            passedScore += kingPPSupportK[2] * kingPPSupportP[ky-0] / 32;
        if ((m << 8) & kMask)
            passedScore += kingPPSupportK[3] * kingPPSupportP[ky-1] / 32;
        if ((m << 16) & kMask)
            passedScore += kingPPSupportK[4] * kingPPSupportP[ky-2] / 32;

        // Penalty for opponent pieces blocking passed pawns
        U64 ppBlockSquares = passedPawnsW << 8;
        if (ppBlockSquares & pos.blackBB()) {
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BKNIGHT)) * ppBlockerBonus[0];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BBISHOP)) * ppBlockerBonus[1];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BROOK))   * ppBlockerBonus[2];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BQUEEN))  * ppBlockerBonus[3];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BKING))   * ppBlockerBonus[4];
        }
        ppBlockSquares = BitBoard::northFill(passedPawnsW << 16);
        if (ppBlockSquares & pos.blackBB()) {
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BKNIGHT)) * ppBlockerBonus[5];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BBISHOP)) * ppBlockerBonus[6];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BROOK))   * ppBlockerBonus[7];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BQUEEN))  * ppBlockerBonus[8];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::BKING))   * ppBlockerBonus[9];
        }

        // Bonus for rook behind passed pawn
        m = BitBoard::southFill(passedPawnsW);
        passedScore += RBehindPP1 * BitBoard::bitCount(m & pos.pieceTypeBB(Piece::WROOK));
        passedScore -= RBehindPP2 * BitBoard::bitCount(m & pos.pieceTypeBB(Piece::BROOK));
    }
    score += interpolate(passedScore * passedPawnEGFactor / 32, passedScore, mhd->wPassedPawnIPF);

    passedScore = phd.passedBonusB;
    const U64 passedPawnsB = phd.passedPawns & pos.pieceTypeBB(Piece::BPAWN);
    m = passedPawnsB;
    if (m != 0) {
        U64 kMask = pos.pieceTypeBB(Piece::BKING);
        int ky = Position::getY(pos.getKingSq(false));
        if ((m >> 8) & kMask)
            passedScore += kingPPSupportK[0] * kingPPSupportP[6-ky] / 32;
        else if ((m >> 16) & kMask)
            passedScore += kingPPSupportK[1] * kingPPSupportP[5-ky] / 32;
        m = ((m & BitBoard::maskAToGFiles) << 1) | ((m & BitBoard::maskBToHFiles) >> 1);
        if (m & kMask)
            passedScore += kingPPSupportK[2] * kingPPSupportP[7-ky] / 32;
        if ((m >> 8) & kMask)
            passedScore += kingPPSupportK[3] * kingPPSupportP[6-ky] / 32;
        if ((m >> 16) & kMask)
            passedScore += kingPPSupportK[4] * kingPPSupportP[5-ky] / 32;

        // Penalty for opponent pieces blocking passed pawns
        U64 ppBlockSquares = passedPawnsB >> 8;
        if (ppBlockSquares & pos.whiteBB()) {
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WKNIGHT)) * ppBlockerBonus[0];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WBISHOP)) * ppBlockerBonus[1];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WROOK))   * ppBlockerBonus[2];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WQUEEN))  * ppBlockerBonus[3];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WKING))   * ppBlockerBonus[4];
        }
        ppBlockSquares = BitBoard::southFill(passedPawnsB >> 16);
        if (ppBlockSquares && pos.whiteBB()) {
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WKNIGHT)) * ppBlockerBonus[5];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WBISHOP)) * ppBlockerBonus[6];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WROOK))   * ppBlockerBonus[7];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WQUEEN))  * ppBlockerBonus[8];
            passedScore -= BitBoard::bitCount(ppBlockSquares & pos.pieceTypeBB(Piece::WKING))   * ppBlockerBonus[9];
        }

        // Bonus for rook behind passed pawn
        m = BitBoard::northFill(passedPawnsB);
        passedScore += RBehindPP1 * BitBoard::bitCount(m & pos.pieceTypeBB(Piece::BROOK));
        passedScore -= RBehindPP2 * BitBoard::bitCount(m & pos.pieceTypeBB(Piece::WROOK));
    }
    score -= interpolate(passedScore * passedPawnEGFactor / 32, passedScore, mhd->bPassedPawnIPF);

    // Passed pawns are more dangerous if enemy king is far away
    const int hiMtrl = passedPawnHiMtrl;
    m = passedPawnsW;
    int bestWPawnDist = 8;
    int bestWPromSq = -1;
    if (m != 0) {
        int mtrlNoPawns = pos.bMtrl() - pos.bMtrlPawns();
        if (mtrlNoPawns < hiMtrl) {
            int kingPos = pos.getKingSq(false);
            while (m != 0) {
                int sq = BitBoard::extractSquare(m);
                int x = Position::getX(sq);
                int y = Position::getY(sq);
                int pawnDist = std::min(5, 7 - y);
                int kingDist = BitBoard::getKingDistance(kingPos, Position::getSquare(x, 7));
                int kScore = kingDist * 4;
                if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                score += interpolate(kScore, 0, mhd->wPassedPawnIPF);
                if (!pos.isWhiteMove())
                    kingDist--;
                if ((pawnDist < kingDist) && (mtrlNoPawns == 0)) {
                    if (BitBoard::northFill(1ULL<<sq) & (1LL << pos.getKingSq(true)))
                        pawnDist++; // Own king blocking pawn
                    if (pawnDist < bestWPawnDist) {
                        bestWPawnDist = pawnDist;
                        bestWPromSq = Position::getSquare(x, 7);
                    }
                }
            }
        }
    }
    int bestBPawnDist = 8;
    int bestBPromSq = -1;
    m = passedPawnsB;
    if (m != 0) {
        int mtrlNoPawns = pos.wMtrl() - pos.wMtrlPawns();
        if (mtrlNoPawns < hiMtrl) {
            int kingPos = pos.getKingSq(true);
            while (m != 0) {
                int sq = BitBoard::extractSquare(m);
                int x = Position::getX(sq);
                int y = Position::getY(sq);
                int pawnDist = std::min(5, y);
                int kingDist = BitBoard::getKingDistance(kingPos, Position::getSquare(x, 0));
                int kScore = kingDist * 4;
                if (kingDist > pawnDist) kScore += (kingDist - pawnDist) * (kingDist - pawnDist);
                score -= interpolate(kScore, 0, mhd->bPassedPawnIPF);
                if (pos.isWhiteMove())
                    kingDist--;
                if ((pawnDist < kingDist) && (mtrlNoPawns == 0)) {
                    if (BitBoard::southFill(1ULL<<sq) & (1LL << pos.getKingSq(false)))
                        pawnDist++; // Own king blocking pawn
                    if (pawnDist < bestBPawnDist) {
                        bestBPawnDist = pawnDist;
                        bestBPromSq = Position::getSquare(x, 0);
                    }
                }
            }
        }
    }

    // Evaluate pawn races in pawn end games
    const int prBonus = pawnRaceBonus;
    if (bestWPromSq >= 0) {
        if (bestBPromSq >= 0) {
            int wPly = bestWPawnDist * 2; if (pos.isWhiteMove()) wPly--;
            int bPly = bestBPawnDist * 2; if (!pos.isWhiteMove()) bPly--;
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

template <bool white>
static inline int
evalConnectedPP(int x, int y, U64 ppMask) {
    if ((x >= 7) || !(BitBoard::maskFile[x+1] & ppMask))
        return 0;

    int y2 = 0;
    if (white) {
        for (int i = 6; i >= 1; i--) {
            int sq = Position::getSquare(x+1, i);
            if (ppMask & (1ULL << sq)) {
                y2 = i;
                break;
            }
        }
    } else {
        for (int i = 1; i <= 6; i++) {
            int sq = Position::getSquare(x+1, i);
            if (ppMask & (1ULL << sq)) {
                y2 = i;
                break;
            }
        }
    }
    if (y2 == 0)
        return 0;

    if (!white) {
        y = 7 - y;
        y2 = 7 - y2;
    }
    return connectedPPBonus[(y-1)*6 + (y2-1)];
}

/** Compute subset of squares given by mask that white is in control over, ie
 *  squares that have at least as many white pawn guards as black has pawn
 *  attacks on the square. */
static inline U64
wPawnCtrlSquares(U64 mask, U64 wPawns, U64 bPawns) {
    U64 wLAtks = (wPawns & BitBoard::maskBToHFiles) << 7;
    U64 wRAtks = (wPawns & BitBoard::maskAToGFiles) << 9;
    U64 bLAtks = (bPawns & BitBoard::maskBToHFiles) >> 9;
    U64 bRAtks = (bPawns & BitBoard::maskAToGFiles) >> 7;
    return ((mask & ~bLAtks & ~bRAtks) |
            (mask & (bLAtks ^ bRAtks) & (wLAtks | wRAtks)) |
            (mask & wLAtks & wRAtks));
}

static inline U64
bPawnCtrlSquares(U64 mask, U64 wPawns, U64 bPawns) {
    U64 wLAtks = (wPawns & BitBoard::maskBToHFiles) << 7;
    U64 wRAtks = (wPawns & BitBoard::maskAToGFiles) << 9;
    U64 bLAtks = (bPawns & BitBoard::maskBToHFiles) >> 9;
    U64 bRAtks = (bPawns & BitBoard::maskAToGFiles) >> 7;
    return ((mask & ~wLAtks & ~wRAtks) |
            (mask & (wLAtks ^ wRAtks) & (bLAtks | bRAtks)) |
            (mask & bLAtks & bRAtks));
}

U64
Evaluate::computeStalePawns(const Position& pos) {
    const U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);

    // Compute stale white pawns
    U64 wStale;
    {
        U64 wPawnCtrl = wPawnCtrlSquares(wPawns, wPawns, bPawns);
        for (int i = 0; i < 4; i++)
            wPawnCtrl |= wPawnCtrlSquares((wPawnCtrl << 8) & ~bPawns, wPawnCtrl, bPawns);
        wPawnCtrl &= ~BitBoard::maskRow8;
        U64 wPawnCtrlLAtk = (wPawnCtrl & BitBoard::maskBToHFiles) << 7;
        U64 wPawnCtrlRAtk = (wPawnCtrl & BitBoard::maskAToGFiles) << 9;

        U64 bLAtks = (bPawns & BitBoard::maskBToHFiles) >> 9;
        U64 bRAtks = (bPawns & BitBoard::maskAToGFiles) >> 7;
        U64 wActive = ((bLAtks ^ bRAtks) |
                       (bLAtks & bRAtks & (wPawnCtrlLAtk | wPawnCtrlRAtk)));
        for (int i = 0; i < 4; i++)
            wActive |= (wActive & ~(wPawns | bPawns)) >> 8;
        wStale = wPawns & ~wActive;
    }

    // Compute stale black pawns
    U64 bStale;
    {
        U64 bPawnCtrl = bPawnCtrlSquares(bPawns, wPawns, bPawns);
        for (int i = 0; i < 4; i++)
            bPawnCtrl |= bPawnCtrlSquares((bPawnCtrl >> 8) & ~wPawns, wPawns, bPawnCtrl);
        bPawnCtrl &= ~BitBoard::maskRow1;
        U64 bPawnCtrlLAtk = (bPawnCtrl & BitBoard::maskBToHFiles) >> 9;
        U64 bPawnCtrlRAtk = (bPawnCtrl & BitBoard::maskAToGFiles) >> 7;

        U64 wLAtks = (wPawns & BitBoard::maskBToHFiles) << 7;
        U64 wRAtks = (wPawns & BitBoard::maskAToGFiles) << 9;
        U64 bActive = ((wLAtks ^ wRAtks) |
                       (wLAtks & wRAtks & (bPawnCtrlLAtk | bPawnCtrlRAtk)));
        for (int i = 0; i < 4; i++)
            bActive |= (bActive & ~(wPawns | bPawns)) << 8;
        bStale = bPawns & ~bActive;
    }

    return wStale | bStale;
}

void
Evaluate::computePawnHashData(const Position& pos, PawnHashData& ph) {
    int score = 0;

    // Evaluate double pawns and pawn islands
    const U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 wPawnFiles = BitBoard::southFill(wPawns) & 0xff;
    const int wIslands = BitBoard::bitCount(((~wPawnFiles) >> 1) & wPawnFiles);

    const U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
    const U64 bPawnFiles = BitBoard::southFill(bPawns) & 0xff;
    const int bIslands = BitBoard::bitCount(((~bPawnFiles) >> 1) & bPawnFiles);
    score -= (wIslands - bIslands) * pawnIslandPenalty;

    // Evaluate doubled pawns
    const U64 wDoubled = BitBoard::northFill(wPawns << 8) & wPawns;
    U64 m = wDoubled;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        score -= pawnDoubledPenalty[Position::getX(sq)];
    }
    const U64 bDoubled = BitBoard::southFill(bPawns >> 8) & bPawns;
    m = bDoubled;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        score += pawnDoubledPenalty[Position::getX(sq)];
    }

    // Evaluate isolated pawns
    const U64 wIsolated = wPawns & ~BitBoard::northFill(BitBoard::southFill(
            ((wPawns & BitBoard::maskAToGFiles) << 1) |
            ((wPawns & BitBoard::maskBToHFiles) >> 1)));
    m = wIsolated;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        score -= pawnIsolatedPenalty[Position::getX(sq)];
    }
    const U64 bIsolated = bPawns & ~BitBoard::northFill(BitBoard::southFill(
            ((bPawns & BitBoard::maskAToGFiles) << 1) |
            ((bPawns & BitBoard::maskBToHFiles) >> 1)));
    m = bIsolated;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        score += pawnIsolatedPenalty[Position::getX(sq)];
    }

    // Evaluate backward pawns, defined as a pawn that guards a friendly pawn,
    // can't be guarded by friendly pawns, can advance, but can't advance without
    // being captured by an enemy pawn.
    const U64 bPawnNoAtks = ~BitBoard::southFill(bPawnAttacks);
    const U64 wPawnNoAtks = ~BitBoard::northFill(wPawnAttacks);
    ph.outPostsW = bPawnNoAtks & wPawnAttacks;
    ph.outPostsB = wPawnNoAtks & bPawnAttacks;

    U64 wBackward = wPawns & ~((wPawns | bPawns) >> 8) & (bPawnAttacks >> 8) & wPawnNoAtks;
    wBackward &= BitBoard::bPawnAttacksMask(wPawns);
    wBackward &= ~BitBoard::northFill(bPawnFiles);
    wBackward &= BitBoard::maskRow2 | BitBoard::maskRow3;
    U64 bBackward = bPawns & ~((wPawns | bPawns) << 8) & (wPawnAttacks << 8) & bPawnNoAtks;
    bBackward &= BitBoard::wPawnAttacksMask(bPawns);
    bBackward &= ~BitBoard::northFill(wPawnFiles);
    bBackward &= BitBoard::maskRow6 | BitBoard::maskRow7;
    score -= (BitBoard::bitCount(wBackward) - BitBoard::bitCount(bBackward)) * pawnBackwardPenalty;

    // Evaluate "semi-backward pawns", defined as pawns on 2:nd or 3:rd rank that can advance,
    // but the advanced pawn is attacked by an enemy pawn.
    U64 wSemiBackward = wPawns & ~((wPawns | bPawns) >> 8) & (bPawnAttacks >> 8);
    score -= BitBoard::bitCount(wSemiBackward & BitBoard::maskRow2) * pawnSemiBackwardPenalty1;
    score -= BitBoard::bitCount(wSemiBackward & BitBoard::maskRow3) * pawnSemiBackwardPenalty2;
    U64 bSemiBackward = bPawns & ~((wPawns | bPawns) << 8) & (wPawnAttacks << 8);
    score += BitBoard::bitCount(bSemiBackward & BitBoard::maskRow7) * pawnSemiBackwardPenalty1;
    score += BitBoard::bitCount(bSemiBackward & BitBoard::maskRow6) * pawnSemiBackwardPenalty2;

    // Evaluate passed pawn bonus, white
    U64 passedPawnsW = wPawns & ~BitBoard::southFill(bPawns | bPawnAttacks | (wPawns >> 8));
    int passedBonusW = 0;
    if (passedPawnsW != 0) {
        U64 m = passedPawnsW;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            int x = Position::getX(sq);
            int y = Position::getY(sq);
            passedBonusW += passedPawnBonusX[x] + passedPawnBonusY[y];
            passedBonusW += evalConnectedPP<true>(x, y, passedPawnsW);
        }
    }

    // Evaluate passed pawn bonus, black
    U64 passedPawnsB = bPawns & ~BitBoard::northFill(wPawns | wPawnAttacks | (bPawns << 8));
    int passedBonusB = 0;
    if (passedPawnsB != 0) {
        U64 m = passedPawnsB;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            int x = Position::getX(sq);
            int y = Position::getY(sq);
            passedBonusB += passedPawnBonusX[x] + passedPawnBonusY[7-y];
            passedBonusB += evalConnectedPP<false>(x, y, passedPawnsB);
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
            int sq = BitBoard::extractSquare(m);
            int y = Position::getY(sq);
            passedBonusW += candidatePassedBonus[y];
        }
    }
    {
        U64 m = bCandidates;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            int y = Position::getY(sq);
            passedBonusB += candidatePassedBonus[7-y];
        }
    }

    { // Bonus for pawns protected by pawns
        U64 m = wPawnAttacks & wPawns;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            score += protectedPawnBonus[63-sq];
        }
        m = bPawnAttacks & bPawns;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            score -= protectedPawnBonus[sq];
        }
    }
    { // Bonus for pawns attacked by pawns
        U64 m = wPawnAttacks & bPawns;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            score += attackedPawnBonus[63-sq];
        }
        m = bPawnAttacks & wPawns;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            score -= attackedPawnBonus[sq];
        }
    }

    U64 stalePawns = computeStalePawns(pos) & ~passedPawnsW & ~passedPawnsB;
    score -= BitBoard::bitCount(wPawns & ~((stalePawns & wPawns) | passedPawnsW)) * activePawnPenalty;
    score += BitBoard::bitCount(bPawns & ~((stalePawns & bPawns) | passedPawnsB)) * activePawnPenalty;

    ph.key = pos.pawnZobristHash();
    ph.score = score;
    ph.passedBonusW = (S16)passedBonusW;
    ph.passedBonusB = (S16)passedBonusB;
    ph.passedPawns = passedPawnsW | passedPawnsB;
    ph.stalePawns = stalePawns;
}

int
Evaluate::rookBonus(const Position& pos) {
    int score = 0;
    const U64 wPawns = pos.pieceTypeBB(Piece::WPAWN);
    const U64 bPawns = pos.pieceTypeBB(Piece::BPAWN);
    const U64 occupied = pos.occupiedBB();
    U64 m = pos.pieceTypeBB(Piece::WROOK);
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        const int x = Position::getX(sq);
        if ((wPawns & BitBoard::maskFile[x]) == 0) // At least half-open file
            score += (bPawns & BitBoard::maskFile[x]) == 0 ? rookOpenBonus : rookHalfOpenBonus;
        U64 atk = BitBoard::rookAttacks(sq, occupied);
        wAttacksBB |= atk;
        wContactSupport |= atk;
        score += rookMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB() | bPawnAttacks))];
        if ((atk & bKingZone) != 0)
            bKingAttacks += BitBoard::bitCount(atk & bKingZone);
    }
    U64 r7 = pos.pieceTypeBB(Piece::WROOK) & BitBoard::maskRow7;
    if (((r7 & (r7 - 1)) != 0) &&
        ((pos.pieceTypeBB(Piece::BKING) & BitBoard::maskRow8) != 0))
        score += rookDouble7thRowBonus; // Two rooks on 7:th row
    m = pos.pieceTypeBB(Piece::BROOK);
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        const int x = Position::getX(sq);
        if ((bPawns & BitBoard::maskFile[x]) == 0)
            score -= (wPawns & BitBoard::maskFile[x]) == 0 ? rookOpenBonus : rookHalfOpenBonus;
        U64 atk = BitBoard::rookAttacks(sq, occupied);
        bAttacksBB |= atk;
        bContactSupport |= atk;
        score -= rookMobScore[BitBoard::bitCount(atk & ~(pos.blackBB() | wPawnAttacks))];
        if ((atk & wKingZone) != 0)
            wKingAttacks += BitBoard::bitCount(atk & wKingZone);
    }
    r7 = pos.pieceTypeBB(Piece::BROOK) & BitBoard::maskRow2;
    if (((r7 & (r7 - 1)) != 0) &&
        ((pos.pieceTypeBB(Piece::WKING) & BitBoard::maskRow1) != 0))
      score -= rookDouble7thRowBonus; // Two rooks on 2:nd row
    return score;
}

int
Evaluate::bishopEval(const Position& pos, int oldScore) {
    int score = 0;
    const U64 occupied = pos.occupiedBB();
    const U64 wBishops = pos.pieceTypeBB(Piece::WBISHOP);
    const U64 bBishops = pos.pieceTypeBB(Piece::BBISHOP);
    if ((wBishops | bBishops) == 0)
        return 0;
    U64 m = wBishops;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        U64 atk = BitBoard::bishopAttacks(sq, occupied);
        wAttacksBB |= atk;
        wContactSupport |= atk;
        score += bishMobScore[BitBoard::bitCount(atk & ~(pos.whiteBB() | bPawnAttacks))];
        if ((atk & bKingZone) != 0)
            bKingAttacks += BitBoard::bitCount(atk & bKingZone);
    }
    m = bBishops;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        U64 atk = BitBoard::bishopAttacks(sq, occupied);
        bAttacksBB |= atk;
        bContactSupport |= atk;
        score -= bishMobScore[BitBoard::bitCount(atk & ~(pos.blackBB() | wPawnAttacks))];
        if ((atk & wKingZone) != 0)
            wKingAttacks += BitBoard::bitCount(atk & wKingZone);
    }

    bool whiteDark  = wBishops & BitBoard::maskDarkSq;
    bool whiteLight = wBishops & BitBoard::maskLightSq;
    bool blackDark  = bBishops & BitBoard::maskDarkSq;
    bool blackLight = bBishops & BitBoard::maskLightSq;

    // Bishop pair bonus
    if (whiteDark && whiteLight) {
        int numMinors = BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP, Piece::BKNIGHT));
        const int numPawns = BitBoard::bitCount(pos.pieceTypeBB(Piece::WPAWN));
        score += bishopPairValue[std::min(numMinors,3)] - numPawns * bishopPairPawnPenalty;
    }
    if (blackDark && blackLight) {
        int numMinors = BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP, Piece::WKNIGHT));
        const int numPawns = BitBoard::bitCount(pos.pieceTypeBB(Piece::BPAWN));
        score -= bishopPairValue[std::min(numMinors,3)] - numPawns * bishopPairPawnPenalty;
    }

    if ((whiteDark != whiteLight) && (blackDark != blackLight) && (whiteDark != blackDark) &&
        (pos.wMtrl() - pos.wMtrlPawns() == pos.bMtrl() - pos.bMtrlPawns())) {
        const int penalty = (oldScore + score) * oppoBishopPenalty / 128;
        score -= interpolate(penalty, 0, mhd->diffColorBishopIPF);
    } else {
        if (whiteDark != whiteLight) {
            U64 bishColorMask = whiteDark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
            U64 m = pos.pieceTypeBB(Piece::WPAWN) & bishColorMask;
            m |= (m << 8) & pos.pieceTypeBB(Piece::BPAWN);
            score -= 2 * BitBoard::bitCount(m);
        }
        if (blackDark != blackLight) {
            U64 bishColorMask = blackDark ? BitBoard::maskDarkSq : BitBoard::maskLightSq;
            U64 m = pos.pieceTypeBB(Piece::BPAWN) & bishColorMask;
            m |= (m >> 8) & pos.pieceTypeBB(Piece::WPAWN);
            score += 2 * BitBoard::bitCount(m);
        }
    }

    // Penalty for bishop trapped behind pawn at a2/h2/a7/h7
    if (((wBishops | bBishops) & BitBoard::sqMask(A2,H2,A7,H7)) != 0) {
        const int bTrapped = trappedBishopPenalty;
        if ((pos.getPiece(A7) == Piece::WBISHOP) &&
            (pos.getPiece(B6) == Piece::BPAWN) &&
            (pos.getPiece(C7) == Piece::BPAWN))
            score -= bTrapped;
        if ((pos.getPiece(H7) == Piece::WBISHOP) &&
            (pos.getPiece(G6) == Piece::BPAWN) &&
            (pos.getPiece(F7) == Piece::BPAWN))
            score -= bTrapped;
        if ((pos.getPiece(A2)  == Piece::BBISHOP) &&
            (pos.getPiece(B3) == Piece::WPAWN) &&
            (pos.getPiece(C2) == Piece::WPAWN))
            score += bTrapped;
        if ((pos.getPiece(H2) == Piece::BBISHOP) &&
            (pos.getPiece(G3) == Piece::WPAWN) &&
            (pos.getPiece(F2) == Piece::WPAWN))
            score += bTrapped;
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
        int sq = BitBoard::extractSquare(m);
        U64 atk = BitBoard::knightAttacks[sq];
        wAttacksBB |= atk;
        wContactSupport |= atk;
        score += knightMobScoreA[sq][BitBoard::bitCount(atk & ~pos.whiteBB() & ~bPawnAttacks)];
    }

    m = bKnights;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        U64 atk = BitBoard::knightAttacks[sq];
        bAttacksBB |= atk;
        bContactSupport |= atk;
        score -= knightMobScoreA[sq][BitBoard::bitCount(atk & ~pos.blackBB() & ~wPawnAttacks)];
    }

    m = wKnights & phd->outPostsW;
    if (m != 0) {
        int outPost = 0;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            outPost += knightOutpostBonus[63-sq];
        }
        score += interpolate(0, outPost, mhd->knightOutPostIPF);
    }

    m = bKnights & phd->outPostsB;
    if (m != 0) {
        int outPost = 0;
        while (m != 0) {
            int sq = BitBoard::extractSquare(m);
            outPost += knightOutpostBonus[sq];
        }
        score -= interpolate(0, outPost, mhd->knightOutPostIPF);
    }

    return score;
}

int
Evaluate::threatBonus(const Position& pos) {
    int score = 0;

    // Sum values for all black pieces under attack
    U64 atk = wAttacksBB & pos.pieceTypeBB(Piece::BKNIGHT, Piece::BBISHOP, Piece::BROOK, Piece::BQUEEN);
    atk |= wPawnAttacks;
    U64 m = atk & pos.blackBB() & ~pos.pieceTypeBB(Piece::BKING);
    int tmp = 0;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        tmp += ::pieceValue[pos.getPiece(sq)];
    }
    score += tmp + tmp * tmp / threatBonus2;

    // Sum values for all white pieces under attack
    atk = bAttacksBB & pos.pieceTypeBB(Piece::WKNIGHT, Piece::WBISHOP, Piece::WROOK, Piece::WQUEEN);
    atk |= bPawnAttacks;
    m = atk & pos.whiteBB() & ~pos.pieceTypeBB(Piece::WKING);
    tmp = 0;
    while (m != 0) {
        int sq = BitBoard::extractSquare(m);
        tmp += ::pieceValue[pos.getPiece(sq)];
    }
    score -= tmp + tmp * tmp / threatBonus2;
    score /= threatBonus1;

    // Compute "latent" pawn attacks on enemy pieces
    const U64 occupied = pos.occupiedBB();
    U64 pawnTargets = (pos.pieceTypeBB(Piece::WPAWN) << 8) & ~occupied;
    pawnTargets |= ((pawnTargets & BitBoard::maskRow3) << 8) & ~occupied;
    pawnTargets &= wPawnAttacks;
    int latentAttacks = 0;
    latentAttacks += BitBoard::bitCount(BitBoard::wPawnAttacksMask(pawnTargets) &
                                        pos.pieceTypeBB(Piece::BKNIGHT, Piece::BBISHOP,
                                                        Piece::BROOK, Piece::BQUEEN));
    pawnTargets = (pos.pieceTypeBB(Piece::BPAWN) >> 8) & ~occupied;
    pawnTargets |= ((pawnTargets & BitBoard::maskRow6) >> 8) & ~occupied;
    pawnTargets &= bPawnAttacks;
    latentAttacks -= BitBoard::bitCount(BitBoard::bPawnAttacksMask(pawnTargets) &
                                        pos.pieceTypeBB(Piece::WKNIGHT, Piece::WBISHOP,
                                                        Piece::WROOK, Piece::WQUEEN));
    score += latentAttacks * latentAttackBonus;

    return score;
}

int
Evaluate::protectBonus(const Position& pos) {
    int score = 0;
    score += BitBoard::bitCount(pos.pieceTypeBB(Piece::WKNIGHT) & wPawnAttacks) * ::protectBonus[0];
    score += BitBoard::bitCount(pos.pieceTypeBB(Piece::WBISHOP) & wPawnAttacks) * ::protectBonus[1];
    score += BitBoard::bitCount(pos.pieceTypeBB(Piece::WROOK  ) & wPawnAttacks) * ::protectBonus[2];
    score += BitBoard::bitCount(pos.pieceTypeBB(Piece::WQUEEN ) & wPawnAttacks) * ::protectBonus[3];
    score -= BitBoard::bitCount(pos.pieceTypeBB(Piece::BKNIGHT) & bPawnAttacks) * ::protectBonus[0];
    score -= BitBoard::bitCount(pos.pieceTypeBB(Piece::BBISHOP) & bPawnAttacks) * ::protectBonus[1];
    score -= BitBoard::bitCount(pos.pieceTypeBB(Piece::BROOK  ) & bPawnAttacks) * ::protectBonus[2];
    score -= BitBoard::bitCount(pos.pieceTypeBB(Piece::BQUEEN ) & bPawnAttacks) * ::protectBonus[3];
    return score;
}

/** Compute king safety for both kings. */
int
Evaluate::kingSafety(const Position& pos) {
    const int minM = (rV + bV) * 2;
    const int m = pos.wMtrl() - pos.wMtrlPawns() + pos.bMtrl() - pos.bMtrlPawns();
    if (m <= minM)
        return 0;

    const int wKing = pos.getKingSq(true);
    const int bKing = pos.getKingSq(false);
    int score = kingSafetyKPPart(pos);
    if (Position::getY(wKing) == 0) {
        if (((pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(F1,G1)) != 0) &&
            ((pos.pieceTypeBB(Piece::WROOK) & BitBoard::sqMask(G1,H1)) != 0) &&
            ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[6]) != 0)) {
            score -= ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[7]) != 0) ?
                     trappedRookPenalty1 : trappedRookPenalty2;
        } else
        if (((pos.pieceTypeBB(Piece::WKING) & BitBoard::sqMask(B1,C1)) != 0) &&
            ((pos.pieceTypeBB(Piece::WROOK) & BitBoard::sqMask(A1,B1)) != 0) &&
            ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[1]) != 0)) {
            score -= ((pos.pieceTypeBB(Piece::WPAWN) & BitBoard::maskFile[0]) != 0) ?
                     trappedRookPenalty1 : trappedRookPenalty2;
        }
    }
    if (Position::getY(bKing) == 7) {
        if (((pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(F8,G8)) != 0) &&
            ((pos.pieceTypeBB(Piece::BROOK) & BitBoard::sqMask(G8,H8)) != 0) &&
            ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[6]) != 0)) {
            score += ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[7]) != 0) ?
                     trappedRookPenalty1 : trappedRookPenalty2;
        } else
        if (((pos.pieceTypeBB(Piece::BKING) & BitBoard::sqMask(B8,C8)) != 0) &&
            ((pos.pieceTypeBB(Piece::BROOK) & BitBoard::sqMask(A8,B8)) != 0) &&
            ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[1]) != 0)) {
            score += ((pos.pieceTypeBB(Piece::BPAWN) & BitBoard::maskFile[0]) != 0) ?
                     trappedRookPenalty1 : trappedRookPenalty2;
        }
    }

    // Bonus for minor pieces protecting king
    score += BitBoard::bitCount(Evaluate::knightKingProtectPattern[wKing] & pos.pieceTypeBB(Piece::WKNIGHT)) * knightKingProtectBonus;
    score += BitBoard::bitCount(Evaluate::bishopKingProtectPattern[wKing] & pos.pieceTypeBB(Piece::WBISHOP)) * bishopKingProtectBonus;
    score -= BitBoard::bitCount(Evaluate::knightKingProtectPattern[bKing] & pos.pieceTypeBB(Piece::BKNIGHT)) * knightKingProtectBonus;
    score -= BitBoard::bitCount(Evaluate::bishopKingProtectPattern[bKing] & pos.pieceTypeBB(Piece::BBISHOP)) * bishopKingProtectBonus;

    score += kingAttackWeight[std::min(bKingAttacks, 13)] - kingAttackWeight[std::min(wKingAttacks, 13)];

    // Bonus for non-losing queen contact checks
    wAttacksBB |= wPawnAttacks;
    bAttacksBB |= bPawnAttacks;
    wContactSupport |= BitBoard::kingAttacks[pos.wKingSq()] | wPawnAttacks;
    bContactSupport |= BitBoard::kingAttacks[pos.bKingSq()] | bPawnAttacks;
    score += qContactCheckBonus[clamp(getNContactChecks(pos)+2, 0, 4)];

    // Bonus for piece majority on the side where the kings are located
    static const int kingZone[8] = {0,0,0, 1,1, 2,2,2};
    const int wKingZone = kingZone[Position::getX(wKing)];
    const int bKingZone = kingZone[Position::getX(bKing)];
    if ((wKingZone == 0 && bKingZone == 0) || (wKingZone == 2 && bKingZone == 2)) {
        U64 wPieces = pos.pieceTypeBB(Piece::WQUEEN, Piece::WROOK, Piece::WKNIGHT);
        U64 bPieces = pos.pieceTypeBB(Piece::BQUEEN, Piece::BROOK, Piece::BKNIGHT);
        U64 mask = (wKingZone == 0) ? 0x0F0F0F0F0F0F0F0FULL : 0xF0F0F0F0F0F0F0F0ULL;
        int delta = BitBoard::bitCount(wPieces & mask) - BitBoard::bitCount(bPieces & mask);
        score += pieceKingAttackBonus[clamp(delta+3, 0, 6)];
    }

    const int kSafety = interpolate(0, score, mhd->kingSafetyIPF);
    return kSafety;
}

template <bool white, bool right>
static inline int
evalKingPawnShelter(const Position& pos) {
    const int mPawn = white ? Piece::WPAWN : Piece::BPAWN;
    const int oPawn = white ? Piece::BPAWN : Piece::WPAWN;

    const int yBeg = white ? 1 :  6;
    const int yInc = white ? 1 : -1;
    const int yEnd = white ? 4 :  3;
    const int xBeg = right ? 5 :  2;
    const int xInc = right ? 1 : -1;
    const int xEnd = right ? 8 : -1;
    int idx = 0;
    int score = 0;
    for (int y = yBeg; y != yEnd; y += yInc) {
        for (int x = xBeg; x != xEnd; x += xInc) {
            int p = pos.getPiece(Position::getSquare(x, y));
            if (p == mPawn)
                score += pawnShelterTable[idx];
            else if (p == oPawn)
                score -= pawnStormTable[idx];
            idx++;
        }
    }
    return score;
}

int
Evaluate::kingSafetyKPPart(const Position& pos) {
    const U64 key = pos.pawnZobristHash() ^ pos.kingZobristHash();
    KingSafetyHashData& ksh = getKingSafetyHashEntry(kingSafetyHash, key);
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
                safety += kingSafetyWeight1 * BitBoard::bitCount(wPawns & shelter);
                safety -= kingSafetyWeight2 * BitBoard::bitCount(bPawns & (shelter | (shelter << 8)));
                shelter <<= 8;
                safety += kingSafetyWeight3 * BitBoard::bitCount(wPawns & shelter);
                shelter <<= 8;
                safety -= kingSafetyWeight4 * BitBoard::bitCount(bPawns & shelter);

                U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                if (wOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG1 * BitBoard::bitCount(wOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH1 * BitBoard::bitCount(wOpen & 0x18);
                }
                U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                if (bOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG2 * BitBoard::bitCount(bOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH2 * BitBoard::bitCount(bOpen & 0x18);
                }
                const int th = kingSafetyThreshold;
                safety = std::min(safety, th);

                const int xKing = Position::getX(pos.wKingSq());
                if (xKing >= 5)
                    score += evalKingPawnShelter<true, true>(pos);
                else if (xKing <= 2)
                    score += evalKingPawnShelter<true, false>(pos);
            }
            const int kSafety = safety - halfOpenFiles;
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
                safety += kingSafetyWeight1 * BitBoard::bitCount(bPawns & shelter);
                safety -= kingSafetyWeight2 * BitBoard::bitCount(wPawns & (shelter | (shelter >> 8)));
                shelter >>= 8;
                safety += kingSafetyWeight3 * BitBoard::bitCount(bPawns & shelter);
                shelter >>= 8;
                safety -= kingSafetyWeight4 * BitBoard::bitCount(wPawns & shelter);

                U64 bOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(bPawns)) & 0xff;
                if (bOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG1 * BitBoard::bitCount(bOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH1 * BitBoard::bitCount(bOpen & 0x18);
                }
                U64 wOpen = BitBoard::southFill(shelter) & (~BitBoard::southFill(wPawns)) & 0xff;
                if (wOpen != 0) {
                    halfOpenFiles += kingSafetyHalfOpenBCDEFG2 * BitBoard::bitCount(wOpen & 0xe7);
                    halfOpenFiles += kingSafetyHalfOpenAH2 * BitBoard::bitCount(wOpen & 0x18);
                }
                const int th = kingSafetyThreshold;
                safety = std::min(safety, th);

                const int xKing = Position::getX(pos.bKingSq());
                if (xKing >= 5)
                    score -= evalKingPawnShelter<false, true>(pos);
                else if (xKing <= 2)
                    score -= evalKingPawnShelter<false, false>(pos);
            }
            const int kSafety = safety - halfOpenFiles;
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
                int sq = BitBoard::extractSquare(m);
                score += pawnStormBonus * (Position::getY(sq)-5);
            }
            m = bPawns & pStormMask[wKingZone];
            while (m != 0) {
                int sq = BitBoard::extractSquare(m);
                score += pawnStormBonus * (Position::getY(sq)-2);
            }
        }

        ksh.key = key;
        ksh.score = score;
    }
    return ksh.score;
}

std::unique_ptr<Evaluate::EvalHashTables>
Evaluate::getEvalHashTables() {
    return make_unique<EvalHashTables>();
}

int
Evaluate::swindleScore(int evalScore, int distToWin) {
    using namespace SearchConst;
    if (distToWin == 0) {
        int sgn = evalScore >= 0 ? 1 : -1;
        int score = std::abs(evalScore) + 4;
        int lg = floorLog2(score);
        score = (lg - 3) * 4 + (score >> (lg - 2));
        score = std::min(score, minFrustrated - 1);
        return sgn * score;
    } else {
        int sgn = distToWin > 0 ? 1 : -1;
        return sgn * std::max(maxFrustrated + 1 - std::abs(distToWin), minFrustrated);
    }
}
