/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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

class EvaluateTest;

/** Position evaluation routines. */
class Evaluate {
    friend class EvaluateTest;
private:
    struct PawnHashData {
        PawnHashData();
        U64 key;
        S16 current;        // For hash replacement policy
        S16 score;          // Positive score means good for white
        S16 passedBonusW;
        S16 passedBonusB;
        U64 passedPawns;    // The most advanced passed pawns for each file
                            // Contains both white and black pawns
        U64 outPostsW;      // Possible outpost squares for white
        U64 outPostsB;
        U64 stalePawns;     // Pawns that can not be used for "pawn breaks"
    };

    struct MaterialHashData {
        MaterialHashData();
        int id;
        int score;
        S16 wPawnIPF, bPawnIPF;
        S16 wKnightIPF, bKnightIPF;
        S16 castleIPF, queenIPF;
        S16 wPassedPawnIPF, bPassedPawnIPF;
        S16 kingSafetyIPF;
        S16 diffColorBishopIPF;
        S16 wKnightOutPostIPF, bKnightOutPostIPF;
        U8 endGame;
    };

    struct KingSafetyHashData {
        KingSafetyHashData();
        U64 key;
        int score;
        S16 current;        // For hash replacement policy
    };

public:
    struct EvalHashTables {
        EvalHashTables();
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

    /** Compute "swindle" score corresponding to an evaluation score when
     * the position is a known TB draw. */
    static int swindleScore(int evalScore);

    /**
     * Interpolate between (x1,y1) and (x2,y2).
     * If x < x1, return y1, if x > x2 return y2. Otherwise, use linear interpolation.
     */
    static int interpolate(int x, int x1, int y1, int x2, int y2);

    static const int IPOLMAX = 1024;

    /** Compute v1 + (v2-v1)*k/IPOLMAX */
    static int interpolate(int v1, int v2, int k);

    static void staticInitialize();
    static void updateEvalParams();

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

    PawnHashData& getPawnHashEntry(std::vector<PawnHashData>& pawnHash, U64 key);
    int pawnBonus(const Position& pos);

    /** Compute set of pawns that can not participate in "pawn breaks". */
    static U64 computeStalePawns(const Position& pos);

    /** Compute pawn hash data for pos. */
    void computePawnHashData(const Position& pos, PawnHashData& ph);

    /** Compute rook bonus. Rook on open/half-open file. */
    int rookBonus(const Position& pos);

    /** Compute bishop evaluation. */
    int bishopEval(const Position& pos, int oldScore);

    /** Compute knight evaluation. */
    int knightEval(const Position& pos);

    /** Bonus for threatening opponent pieces. */
    int threatBonus(const Position& pos);

    /** Bonus for own pieces protected by pawns. */
    int protectBonus(const Position& pos);

    /** Compute king safety for both kings. */
    int kingSafety(const Position& pos);

    KingSafetyHashData& getKingSafetyHashEntry(vector_aligned<KingSafetyHashData>& ksHash, U64 key);
    int kingSafetyKPPart(const Position& pos);

    static int castleMaskFactor[256];
    static int knightMobScoreA[64][9];
    static U64 knightKingProtectPattern[64];
    static U64 bishopKingProtectPattern[64];

    std::vector<PawnHashData>& pawnHash;
    const PawnHashData* phd;

    std::vector<MaterialHashData>& materialHash;
    const MaterialHashData* mhd;

    vector_aligned<KingSafetyHashData>& kingSafetyHash;

     // King safety variables
    U64 wKingZone, bKingZone;       // Squares close to king that are worth attacking
    int wKingAttacks, bKingAttacks; // Number of attacks close to white/black king
    U64 wAttacksBB, bAttacksBB;
    U64 wPawnAttacks, bPawnAttacks; // Squares attacked by white/black pawns
};


inline
Evaluate::PawnHashData::PawnHashData()
    : key((U64)-1), // Non-zero to avoid collision for positions with no pawns
      current(0), score(0),
      passedBonusW(0),
      passedBonusB(0),
      passedPawns(0) {
}

inline
Evaluate::MaterialHashData::MaterialHashData()
    : id(-1), score(0) {
}

inline
Evaluate::KingSafetyHashData::KingSafetyHashData()
    : key((U64)-1), score(0), current(0) {
}

inline
Evaluate::EvalHashTables::EvalHashTables() {
    pawnHash.resize(1<<16);
    kingSafetyHash.resize(1 << 15);
    materialHash.resize(1 << 14);
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

inline Evaluate::PawnHashData&
Evaluate::getPawnHashEntry(std::vector<Evaluate::PawnHashData>& pawnHash, U64 key) {
    int e0 = (int)key & (pawnHash.size() - 2);
    int e1 = e0 + 1;
    if (pawnHash[e0].key == key) {
        pawnHash[e0].current = 1;
        pawnHash[e1].current = 0;
        return pawnHash[e0];
    }
    if (pawnHash[e1].key == key) {
        pawnHash[e1].current = 1;
        pawnHash[e0].current = 0;
        return pawnHash[e1];
    }
    if (pawnHash[e0].current) {
        pawnHash[e1].current = 1;
        pawnHash[e0].current = 0;
        return pawnHash[e1];
    } else {
        pawnHash[e0].current = 1;
        pawnHash[e1].current = 0;
        return pawnHash[e0];
    }
}

inline Evaluate::KingSafetyHashData&
Evaluate::getKingSafetyHashEntry(vector_aligned<Evaluate::KingSafetyHashData>& ksHash, U64 key) {
    int e0 = (int)key & (ksHash.size() - 2);
    int e1 = e0 + 1;
    if (ksHash[e0].key == key) {
        ksHash[e0].current = 1;
        ksHash[e1].current = 0;
        return ksHash[e0];
    }
    if (ksHash[e1].key == key) {
        ksHash[e1].current = 1;
        ksHash[e0].current = 0;
        return ksHash[e1];
    }
    if (ksHash[e0].current) {
        ksHash[e1].current = 1;
        ksHash[e0].current = 0;
        return ksHash[e1];
    } else {
        ksHash[e0].current = 1;
        ksHash[e1].current = 0;
        return ksHash[e0];
    }
}

#endif /* EVALUATE_HPP_ */
