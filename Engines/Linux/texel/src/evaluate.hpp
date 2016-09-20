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
 * evaluate.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef EVALUATE_HPP_
#define EVALUATE_HPP_

#include "parameters.hpp"
#include "piece.hpp"
#include "position.hpp"
#include "util/alignedAlloc.hpp"

/** Position evaluation routines. */
class Evaluate {
private:
    struct PawnHashData {
        PawnHashData();
        U64 key;
        int score;            // Positive score means good for white
        short passedBonusW;
        short passedBonusB;
        U64 passedPawnsW;     // The most advanced passed pawns for each file
        U64 passedPawnsB;
        U64 outPostsW;        // Possible outpost squares for white
        U64 outPostsB;
    };

    struct MaterialHashData {
        MaterialHashData() : id(-1), score(0) { }
        int id;
        int score;
        short wPawnIPF, bPawnIPF;
        short wKnightIPF, bKnightIPF;
        short castleIPF;
        short wPassedPawnIPF, bPassedPawnIPF;
        short kingSafetyIPF;
        short diffColorBishopIPF;
        short wKnightOutPostIPF, bKnightOutPostIPF;
        U8 endGame;
    };

    struct KingSafetyHashData {
        KingSafetyHashData() : key((U64)-1), score(0) { }
        U64 key;
        int score;
    };

public:
    struct EvalHashTables {
        EvalHashTables() {
            pawnHash.resize(1<<16);
            kingSafetyHash.resize(1 << 15);
            materialHash.resize(1 << 14);
        }
        std::vector<PawnHashData> pawnHash;
        std::vector<MaterialHashData> materialHash;
        vector_aligned<KingSafetyHashData> kingSafetyHash;
    };

    /** Constructor. */
    Evaluate(EvalHashTables& et);

    static int pieceValueOrder[Piece::nPieceTypes];

    static const int* psTab1[Piece::nPieceTypes];
    static const int* psTab2[Piece::nPieceTypes];

    /** Get evaluation hash tables. */
    static std::shared_ptr<EvalHashTables> getEvalHashTables();

    /**
     * Static evaluation of a position.
     * @param pos The position to evaluate.
     * @return The evaluation score, measured in centipawns.
     *         Positive values are good for the side to make the next move.
     */
    int evalPos(const Position& pos);
    int evalPosPrint(const Position& pos);

    /**
     * Interpolate between (x1,y1) and (x2,y2).
     * If x < x1, return y1, if x > x2 return y2. Otherwise, use linear interpolation.
     */
    static int interpolate(int x, int x1, int y1, int x2, int y2);

    static const int IPOLMAX = 1024;

    /** Compute v1 + (v2-v1)*k/IPOLMAX */
    static int interpolate(int v1, int v2, int k);

    static void staticInitialize();

private:
    template <bool print> int evalPos(const Position& pos);

    /** Compute score based on piece square tables. Positive values are good for white. */
    int pieceSquareEval(const Position& pos);

    /** Get material score */
    int materialScore(const Position& pos, bool print);

    /** Compute material score. */
    void computeMaterialScore(const Position& pos, MaterialHashData& mhd, bool print) const;

    /** Implement the "when ahead trade pieces, when behind trade pawns" rule. */
    int tradeBonus(const Position& pos, int wCorr, int bCorr) const;

    /** Score castling ability. */
    int castleBonus(const Position& pos);

    int pawnBonus(const Position& pos);

    /** Compute pawn hash data for pos. */
    void computePawnHashData(const Position& pos, PawnHashData& ph);

    /** Compute rook bonus. Rook on open/half-open file. */
    int rookBonus(const Position& pos);

    /** Compute bishop evaluation. */
    int bishopEval(const Position& pos, int oldScore);

    /** Compute knight evaluation. */
    int knightEval(const Position& pos);

    int threatBonus(const Position& pos);

    /** Compute king safety for both kings. */
    int kingSafety(const Position& pos);

    int kingSafetyKPPart(const Position& pos);

    /** Implements special knowledge for some endgame situations.
     * If doEval is false the position is not evaluated. Instead 1 is returned if
     * this function has special knowledge about the current material balance, and 0
     * is returned otherwise. */
    template <bool doEval> int endGameEval(const Position& pos, int oldScore) const;

    static int kqkpEval(int wKing, int wQueen, int bKing, int bPawn, bool whiteMove, int score);

    static int kpkEval(int wKing, int bKing, int wPawn, bool whiteMove);

    static int krkpEval(int wKing, int bKing, int bPawn, bool whiteMove, int score);
    static int krpkrEval(int wKing, int bKing, int wPawn, int wRook, int bRook, bool whiteMove);
    static int krpkrpEval(int wKing, int bKing, int wPawn, int wRook, int bRook, int bPawn, bool whiteMove, int score);

    static int kbnkEval(int wKing, int bKing, bool darkBishop);

    static int kbpkbEval(int wKing, int wBish, int wPawn, int bKing, int bBish, int score);
    static int kbpknEval(int wKing, int wBish, int wPawn, int bKing, int bKnight, int score);
    static int knpkbEval(int wKing, int wKnight, int wPawn, int bKing, int bBish, int score, bool wtm);
    static int knpkEval(int wKing, int wKnight, int wPawn, int bKing, int score, bool wtm);

    static int castleFactor[256];
    static const int distToH1A8[8][8];
    static int knightMobScore[64][9];

    std::vector<PawnHashData>& pawnHash;
    const PawnHashData* phd;

    std::vector<MaterialHashData>& materialHash;
    const MaterialHashData* mhd;

    vector_aligned<KingSafetyHashData>& kingSafetyHash;

    static const ubyte kpkTable[2*32*64*48/8];
    static const ubyte krkpTable[2*32*48*8];
    static const U64 krpkrTable[2*24*64];

    // King safety variables
    U64 wKingZone, bKingZone;       // Squares close to king that are worth attacking
    int wKingAttacks, bKingAttacks; // Number of attacks close to white/black king
    U64 wAttacksBB, bAttacksBB;
    U64 wPawnAttacks, bPawnAttacks; // Squares attacked by white/black pawns
};


inline
Evaluate::PawnHashData::PawnHashData()
    : key((U64)-1), // Non-zero to avoid collision for positions with no pawns
      score(0),
      passedBonusW(0),
      passedBonusB(0),
      passedPawnsW(0),
      passedPawnsB(0) {
}

inline int
Evaluate::interpolate(int x, int x1, int y1, int x2, int y2) {
    if (x > x2) {
        return y2;
    } else if (x < x1) {
        return y1;
    } else {
        return (x - x1) * (y2 - y1) / (x2 - x1) + y1;
    }
}

inline int
Evaluate::interpolate(int v1, int v2, int k) {
    return v1 + (v2 - v1) * k / IPOLMAX;
}

inline int
Evaluate::materialScore(const Position& pos, bool print) {
    int mId = pos.materialId();
    int key = (mId >> 16) * 40507 + mId;
    MaterialHashData& newMhd = materialHash[key & (materialHash.size() - 1)];
    if ((newMhd.id != mId) || print)
        computeMaterialScore(pos, newMhd, print);
    mhd = &newMhd;
    return newMhd.score;
}

#endif /* EVALUATE_HPP_ */
