/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * tbgen.hpp
 *
 *  Created on: Apr 3, 2015
 *      Author: petero
 */

#ifndef TBGEN_HPP_
#define TBGEN_HPP_

#include "util/util.hpp"

#include <algorithm>
#include <cassert>


class Position;


/** Represent the game-theoretic or intermediate value of a position. */
class PositionValue {
    friend class TBGenTest;
    enum class State : S8 {
        MATE_IN_0 = 64,       // MATE_IN_N = MATE_IN_0 + N
        MATED_IN_0 = 63,      // MATED_IN_N = MATED_IN_0 - N
        DRAW = 0,
        INVALID = -1,
        UNINITIALIZED = -2,
        UNKNOWN = -3,
        REMAINING_0 = -3      // REMAINING_N = REMAINING_0 - N
    };
public:

    PositionValue();
    explicit PositionValue(U8 byte);

    void setMateInN(int n);
    void setMatedInN(int n);
    void setDraw();
    void setInvalid();
    void setUnknown();
    void setRemaining(int n);

    /** Decrement number of remaining moves. Return true if no more remaining moves. */
    bool decRemaining();

    bool isUnInitialized() const;
    bool isMatedInN(int n) const;
    bool isMateInN(int n) const;
    bool isDraw() const;
    bool isComputed() const;
    bool isUnknown() const;
    bool isRemainingN() const;

    /** If score is mate/mated in n, return true and set n to the distance to mate. */
    bool getMateInN(int& n) const;
    bool getMatedInN(int& n) const;

    State getState() const;

private:
    State state;
};


/** Information about number of pieces of each type. */
struct PieceCount {
    int nwq, nwr, nwb, nwn;
    int nbq, nbr, nbb, nbn;

    /** Total number of pieces including kings. */
    int nPieces() const;
};


/** A stack-allocated list of "moves". Moves are represented by
 * the TB index after the move has been made. */
class TbMoveList {
public:
    TbMoveList();

    void addMove(U32 idx);

    void sort();

    U32 operator[](int i) const;

    int getSize() const;

private:
    U32 buf[256];
    int size;
};


/**
 * Represents a chess position with at most N pieces using an index.
 * - Pawns, castling and half-move clock are not supported.
 * - A captured piece is represented by having the same square as the black king.
 *   When the black king is moved, all captured pieces automatically follows it.
 * - The white king is confined to the A1-D1-D4 triangle. Trying to set it to
 *   some other square automatically mirrors the board as required.
 * - The white king is piece 0 and the black king is piece nWhite,
 *   where nWhite is the number of white pieces.
 */
class TBIndex {
public:
    /** Constructor. Sets max number of pieces and initial index. */
    TBIndex(int nWhite, int nBlack, U32 index);

    /** Set index. */
    void setIndex(int index);

    /** Canonize index.
     * 1. Sort equal pieces so smaller squares come before larger squares.
     * 2. If white king is on the A1-D4 diagonal:
     *    a) Mirror position in A1-H8 diagonal.
     *    b) Sort equal pieces.
     *    c) If resulting index is smaller than the one in 1), use the smaller index.
     */
    void canonize(const std::vector<int>& pieceTypes, bool duplicatedPieces);

    /** Swap side to move. */
    void swapSide();

    /** Set the square for a piece.
     * When setting the white king the square is mapped to the A1-D1-D4 triangle. */
    void setSquare(int pieceNo, int square);


    /** Get index. */
    U32 getIndex() const;

    /** Return true if white to move. */
    bool whiteMove() const;

    /** Get the square for a piece. */
    int getSquare(int pieceNo) const;


    /** Initialize static data. */
    static void staticInitialize();

private:
    /** Get first bit position for piece pieceNo. */
    int pieceShift(int pieceNo) const;

    /** Mirror all pieces except the white king in the A1-H8 diagonal. */
    void mirrorD();

    /** Mirror X coordinate of all pieces except the white king. */
    void mirrorX();

    /** Mirror Y coordinate of all pieces except the white king. */
    void mirrorY();

    /** Reorder equal pieces so that larger squares come after smaller squares. */
    void sortPieces(const std::vector<int>& pieceTypes);


    int p;      // Number of pieces
    int nWhite; // Number of white pieces

    // Index bit definitions:
    // p = Number of pieces, including both kings
    // Start  size
    // 0      6    : Square of (p-1):th piece
    // 6      6    : Square of (p-2):th piece
    // ...    6    : Square of ...:th piece
    // 6(p-2) 6    : Square of 1:st piece
    // 6(p-1) 1    : Side to move, 1 = white
    // 6p-5   4    : Position of white king (0:th piece), value 0-9
    U32 idx;

    U32 colBits; // Bits corresponding to square columns, 0,1,2, 6,7,8, ...
    U32 rowBits; // Bits corresponding to square rows,    3,4,5, 9,10,11, ...

    static int symType[64];        // Mirror operations needed to map square to a1-d1-d4 triangle
                                   // Bit 0: Mirror X, Bit 1: Mirror Y, Bit 2: Mirror Diag
    static int kingMap[64];        // Map white king square to king index, 0-9
    static int kingMapInverse[10]; // Map king index to square in a1-d1-d4 triangle
};


/** Handle forward and backward move generation and validity checks. */
class TBPosition {
    friend class TBGenTest;
public:
    explicit TBPosition(const PieceCount& pc);

    /** Return the number of possible positions. Not all positions have to be valid. */
    U32 nPositions() const;

    /** Set TB position from pos. Return true if successful. */
    bool setPosition(const Position& pos);

    /** Set position from index.
     * @pre 0 <= idx < nPositions(). */
    void setIndex(U32 idx);

    /** Return true if current index represents a canonical position with non-overlapping pieces.
     * Can modify the current index when returning false. */
    bool indexValid();

    /** Get TB index for current position. */
    U32 getIndex() const;

    /** Swap side to move. */
    void swapSide();


    /** Return true if the oppoent king can be taken. */
    bool canTakeKing();

    /** Generate all pseudo-legal moves. canTakeKing() must be false. */
    void getMoves(TbMoveList& lst);

    /** Generate all pseudo-legal takeback moves. */
    void getUnMoves(TbMoveList& lst);

private:
    /** Convert to Position object. This method is slow. */
    void getPos(Position& pos) const;

    /** Compute bitmask of occupied squares. */
    U64 getOccupied() const;

    std::vector<int> pieceTypes;
    int nWhite; // Number of white pieces
    U32 nPos;   // Number of possible positions
    bool duplicatedPieces; // True if there are at least two equal pieces

    TBIndex currIdx;        // Table index.
};


/** TB storage type that stores data in a private vector. */
class VectorStorage {
public:
    void resize(U32 size) { table.resize(size); }
    const PositionValue operator[](U32 idx) const { return table[idx]; }
    void store(U32 idx, PositionValue pv) { table[idx] = pv; }
private:
    std::vector<PositionValue> table;
};


/** Generate endgame tablebase. */
template <typename TBStorage>
class TBGenerator {
    friend class TBGenTest;
public:
    /** TB generator for positions with the given number of queens,
     * rooks, bishops and knights. Pawns are not supported. */
    TBGenerator(TBStorage& storage, const PieceCount& pc);

    /** Generate the tablebase. */
    bool generate(RelaxedShared<S64>& maxTimeMillis, bool verbose);

    /** Probe tablebase.
     * @param pos  The position to probe.
     * @param ply  The ply value used to adjust mate scores.
     * @param score The tablebase score. Only modified for tablebase hits.
     * @return True if pos was found in the tablebases, false otherwise.
     */
    bool probeDTM(const Position& pos, int ply, int& score) const;

private:
    /** Get the TB PositionValue as an integer for the current position. */
    int getValue(TBPosition& tbPos) const;

    PieceCount pieceCount;
    TBStorage& table;
};



inline
PositionValue::PositionValue()
    : state(State::UNINITIALIZED) {
}

inline
PositionValue::PositionValue(U8 byte)
    : state((State)byte) {
}

inline void
PositionValue::setMateInN(int n) {
    state = (State)((int)State::MATE_IN_0 + n);
}

inline void
PositionValue::setMatedInN(int n) {
    state = (State)((int)State::MATED_IN_0 - n);
}

inline void
PositionValue::setDraw() {
    state = State::DRAW;
}

inline void
PositionValue::setInvalid() {
    state = State::INVALID;
}

inline void
PositionValue::setUnknown() {
    state = State::UNKNOWN;
}

inline void
PositionValue::setRemaining(int n) {
    state = (State)((int)State::REMAINING_0 - n);
}

inline bool
PositionValue::decRemaining() {
    state = (State)((int)state + 1);
    return state == State::REMAINING_0;
}

inline bool
PositionValue::isUnInitialized() const {
    return state == State::UNINITIALIZED;
}

inline bool
PositionValue::isMatedInN(int n) const {
    return (int)state == (int)State::MATED_IN_0 - n;
}

inline bool
PositionValue::isMateInN(int n) const {
    return (int)state == (int)State::MATE_IN_0 + n;
}

inline bool
PositionValue::isDraw() const {
    return state == State::DRAW;
}

inline bool
PositionValue::isComputed() const {
    return state >= State::INVALID;
}

inline bool
PositionValue::isUnknown() const {
    return state == State::UNKNOWN;
}

inline bool
PositionValue::isRemainingN() const {
    return state < State::REMAINING_0;
}

inline bool
PositionValue::getMateInN(int& n) const {
    int s = (int)state;
    if (s <= (int)State::MATE_IN_0)
        return false;
    n = s - (int)State::MATE_IN_0;
    return true;
}

inline bool
PositionValue::getMatedInN(int& n) const {
    int s = (int)state;
    if (s > (int)State::MATED_IN_0 || s <= (int)State::DRAW)
        return false;
    n = (int)State::MATED_IN_0 - s;
    return true;
}

inline PositionValue::State
PositionValue::getState() const {
    return state;
}


inline int
PieceCount::nPieces() const {
    return 2 + nwq + nwr + nwb + nwn + nbq + nbr + nbb + nbn;
}


inline
TbMoveList::TbMoveList()
    : size(0) {
}

inline void
TbMoveList::addMove(U32 idx) {
    buf[size++] = idx;
}

inline void
TbMoveList::sort() {
    std::sort(&buf[0], &buf[size]);
}

inline U32
TbMoveList::operator[](int i) const {
    return buf[i];
}

inline int
TbMoveList::getSize() const {
    return size;
}


inline void
TBIndex::setIndex(int index) {
    idx = index;
}

inline void
TBIndex::swapSide() {
    idx ^= 1ULL << (6*p-6);
}

inline U32
TBIndex::getIndex() const {
    return idx;
}

inline bool
TBIndex::whiteMove() const {
    return (idx >> (6*p-6)) & 1;
}

inline int
TBIndex::pieceShift(int pieceNo) const {
    return 6*(p-1-pieceNo);
}

inline void
TBIndex::mirrorD() {
    idx = ((idx & colBits) << 3) | ((idx & rowBits) >> 3) | (idx & ~(colBits | rowBits));
}

inline void
TBIndex::mirrorX() {
    idx ^= colBits;
}

inline void
TBIndex::mirrorY() {
    idx ^= rowBits;
}


inline U32
TBPosition::nPositions() const {
    return nPos;
}

inline void
TBPosition::setIndex(U32 idx) {
    currIdx.setIndex(idx);
}

inline U32
TBPosition::getIndex() const {
    return currIdx.getIndex();
}

inline void
TBPosition::swapSide() {
    currIdx.swapSide();
}


template <typename TBStorage>
inline int
TBGenerator<TBStorage>::getValue(TBPosition& tbPos) const {
    return (int)table[tbPos.getIndex()].getState();
}

#endif /* TBGEN_HPP_ */
