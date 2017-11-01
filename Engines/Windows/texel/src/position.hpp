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
 * position.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef POSITION_HPP_
#define POSITION_HPP_

#include <iosfwd>

#include "move.hpp"
#include "undoInfo.hpp"
#include "bitBoard.hpp"
#include "piece.hpp"
#include "material.hpp"
#include <algorithm>
#include <iostream>


namespace TBProbeData {
    extern int maxPieces;
}

/**
 * Stores the state of a chess position.
 * All required state is stored, except for all previous positions
 * since the last capture or pawn move. That state is only needed
 * for three-fold repetition draw detection, and is better stored
 * in a separate hash table.
 */
class Position {
public:
    /** Bit definitions for the castleMask bit mask. */
    static const int A1_CASTLE = 0; /** White long castle. */
    static const int H1_CASTLE = 1; /** White short castle. */
    static const int A8_CASTLE = 2; /** Black long castle. */
    static const int H8_CASTLE = 3; /** Black short castle. */

    /** Initialize board to empty position. */
    Position();

    bool equals(const Position& other) const;

    /**
     * Return Zobrist hash value for the current position.
     * Everything except the move counters are included in the hash value.
     */
    U64 zobristHash() const;
    U64 pawnZobristHash() const;
    U64 kingZobristHash() const;

    /** Zobrist hash including the halfMove clock.
     *  Only large halfMove clock values affect the hash. */
    U64 historyHash() const;
    /** Hash including halfMoveClock, to avoid opening book cycles. */
    U64 bookHash() const;

    /** Compute zobrist hash for position after "move" has been made.
     * May be incorrect in some cases, intended for prefetch. */
    U64 hashAfterMove(const Move& move) const;

    /** Return the material identifier. */
    int materialId() const;

    /** Return number of pieces, including kings and pawns. */
    int nPieces() const;

    /**
     * Decide if two positions are equal in the sense of the draw by repetition rule.
     * @return True if positions are equal, false otherwise.
     */
    bool drawRuleEquals(const Position& other) const;

    bool isWhiteMove() const;

    void setWhiteMove(bool whiteMove);

    /** Return piece occupying a square. */
    int getPiece(int square) const;

    /** Set a square to a piece value. */
    void setPiece(int square, int piece);

    /** Remove a piece from a square. */
    void clearPiece(int square);

    /**
     * Set a square to a piece value.
     * Special version that only updates enough of the state for the SEE function to be happy.
     */
    void setSEEPiece(int square, int piece);

    /** Return true if white long castling right has not been lost. */
    bool a1Castle() const;

    /** Return true if white short castling right has not been lost. */
    bool h1Castle() const;

    /** Return true if black long castling right has not been lost. */
    bool a8Castle() const;

    /** Return true if black short castling right has not been lost. */
    bool h8Castle() const;

    /** Bitmask describing castling rights. */
    int getCastleMask() const;
    void setCastleMask(int castleMask);

    /** En passant square, or -1 if no en passant possible. */
    int getEpSquare() const;

    void setEpSquare(int epSquare);

    int getKingSq(bool white) const;

    /** Apply a move to the current position. */
    void makeMove(const Move& move, UndoInfo& ui);

    void unMakeMove(const Move& move, const UndoInfo& ui);

    /** Special make move functions used by MoveGen::isLegal(). Does not update all data members. */
    void makeMoveB(const Move& move, UndoInfo& ui);
    void unMakeMoveB(const Move& move, const UndoInfo& ui);
    void setPieceB(int square, int piece);

    /**
     * Apply a move to the current position.
     * Special version that only updates enough of the state for the SEE function to be happy.
     */
    void makeSEEMove(const Move& move, UndoInfo& ui);

    void unMakeSEEMove(const Move& move, const UndoInfo& ui);

    int getFullMoveCounter() const;
    void setFullMoveCounter(int fm);
    int getHalfMoveClock() const;
    void setHalfMoveClock(int hm);

    /** Return incrementally updated piece square table score for middle game and endgame. */
    int psScore1(int piece) const;
    int psScore2(int piece) const;

    /** BitBoard for all squares occupied by a piece type. */
    U64 pieceTypeBB(Piece::Type piece) const;
    /** BitBoard for all squares occupied by several piece types. */
    template <typename Piece0, typename... Pieces> U64 pieceTypeBB(Piece0 piece0, Pieces... pieces) const;

    /** BitBoard for all squares occupied by white pieces. */
    U64 whiteBB() const;
    /** BitBoard for all squares occupied by black pieces. */
    U64 blackBB() const;
    /** BitBoard for all squares occupied by white or black pieces. */
    U64 colorBB(int wtm) const;

    /** BitBoard for all squares occupied by white and black pieces. */
    U64 occupiedBB() const;

    int wKingSq() const;
    int bKingSq() const;

    /** Total white/black material value. */
    int wMtrl() const;
    int bMtrl() const;
    /** White/black material value for all pawns. */
    int wMtrlPawns() const;
    int bMtrlPawns() const;


    /** Return index in squares[] vector corresponding to (x,y). */
    static int getSquare(int x, int y);

    /** Return x position (file) corresponding to a square. */
    static int getX(int square);

    /** Return y position (rank) corresponding to a square. */
    static int getY(int square);

    /** Return getSquare(getX(square),7-getY(square)). */
    static int mirrorY(int square);

    /** Return true if (x,y) is a dark square. */
    static bool darkSquare(int x, int y);

    /** Compute the Zobrist hash value non-incrementally. Only useful for testing. */
    U64 computeZobristHash();

    /** Initialize static data. */
    static void staticInitialize();

    /** Get hash key for a piece at a square. */
    static U64 getHashKey(int piece, int square);


    /** Serialization. Used by tree logging code. */
    struct SerializeData {
        U64 v[5];
    };
    void serialize(SerializeData& data) const;
    void deSerialize(const SerializeData& data);

private:
    /** Move a non-pawn piece to an empty square. */
    void movePieceNotPawn(int from, int to);
    void movePieceNotPawnB(int from, int to);


    int wMtrl_;              // Total value of all white pieces and pawns
    int bMtrl_;              // Total value of all black pieces and pawns
    int wMtrlPawns_;         // Total value of all white pawns
    int bMtrlPawns_;         // Total value of all black pawns

    int squares[64];

    // Piece square table scores
    short psScore1_[Piece::nPieceTypes];
    short psScore2_[Piece::nPieceTypes];

    // Bitboards
    U64 pieceTypeBB_[Piece::nPieceTypes];
    U64 whiteBB_, blackBB_;

    bool whiteMove;

    /** Number of half-moves since last 50-move reset. */
    int halfMoveClock;

    /** Game move number, starting from 1. */
    int fullMoveCounter;

    int castleMask;
    int epSquare;

    U64 hashKey;           // Cached Zobrist hash key
    U64 pHashKey;          // Cached Zobrist pawn hash key
    MatId matId;           // Cached material identifier

    static U8 castleSqMask[64]; // Castle masks retained for each square

    const static U64 psHashKeys[Piece::nPieceTypes][64];    // [piece][square]

    const static U64 whiteHashKey = 0xc98143a7869aa213ULL;
    const static U64 castleHashKeys[16];   // [castleMask]
    const static U64 epHashKeys[9];        // [epFile + 1] (epFile==-1 for no ep)
    const static U64 moveCntKeys[101];     // [min(halfMoveClock, 100)]
};

/** For debugging. */
std::ostream& operator<<(std::ostream& os, const Position& pos);

inline bool
Position::equals(const Position& other) const {
    if (!drawRuleEquals(other))
        return false;
    if (halfMoveClock != other.halfMoveClock)
        return false;
    if (fullMoveCounter != other.fullMoveCounter)
        return false;
    if (hashKey != other.hashKey)
        return false;
    if (pHashKey != other.pHashKey)
        return false;
    if (matId() != other.matId())
        return false;
    return true;
}

inline U64
Position::zobristHash() const {
    return hashKey;
}

inline U64
Position::pawnZobristHash() const {
    return pHashKey;
}

inline U64
Position::kingZobristHash() const {
    return psHashKeys[Piece::WKING][wKingSq()] ^
           psHashKeys[Piece::BKING][bKingSq()];
}

inline U64
Position::historyHash() const {
    U64 ret = hashKey;
    if (nPieces() <= TBProbeData::maxPieces) {
        ret ^= moveCntKeys[std::min(halfMoveClock, 100)];
    } else if (halfMoveClock >= 40) {
        if (halfMoveClock < 80)
            ret ^= moveCntKeys[halfMoveClock / 10];
        else
            ret ^= moveCntKeys[std::min(halfMoveClock, 100)];
    }
    return ret;
}

inline U64
Position::bookHash() const {
    U64 ret = hashKey;
    ret ^= moveCntKeys[std::min(halfMoveClock, 100)];
    return ret;
}

inline int
Position::materialId() const {
    return matId();
}

inline int
Position::nPieces() const {
    return BitBoard::bitCount(occupiedBB());
}

inline bool
Position::drawRuleEquals(const Position& other) const {
    for (int i = 0; i < 64; i++)
        if (squares[i] != other.squares[i])
            return false;
    if (whiteMove != other.whiteMove)
        return false;
    if (castleMask != other.castleMask)
        return false;
    if (epSquare != other.epSquare)
        return false;
    return true;
}

inline bool
Position::isWhiteMove() const {
    return whiteMove;
}

inline void
Position::setWhiteMove(bool whiteMove) {
    if (whiteMove != this->whiteMove) {
        hashKey ^= whiteHashKey;
        this->whiteMove = whiteMove;
    }
}

inline int
Position::getPiece(int square) const {
    return squares[square];
}

inline void
Position::setSEEPiece(int square, int piece) {
    int removedPiece = squares[square];

    // Update board
    squares[square] = piece;

    // Update bitboards
    U64 sqMask = 1ULL << square;
    pieceTypeBB_[removedPiece] &= ~sqMask;
    pieceTypeBB_[piece] |= sqMask;
    if (removedPiece != Piece::EMPTY) {
        if (Piece::isWhite(removedPiece))
            whiteBB_ &= ~sqMask;
        else
            blackBB_ &= ~sqMask;
    }
    if (piece != Piece::EMPTY) {
        if (Piece::isWhite(piece))
            whiteBB_ |= sqMask;
        else
            blackBB_ |= sqMask;
    }
}

inline bool
Position::a1Castle() const {
    return (castleMask & (1 << A1_CASTLE)) != 0;
}

inline bool
Position::h1Castle() const {
    return (castleMask & (1 << H1_CASTLE)) != 0;
}

inline bool
Position::a8Castle() const {
    return (castleMask & (1 << A8_CASTLE)) != 0;
}

inline bool
Position::h8Castle() const {
    return (castleMask & (1 << H8_CASTLE)) != 0;
}

inline int
Position::getCastleMask() const {
    return castleMask;
}

inline void
Position::setCastleMask(int castleMask) {
    if (castleMask != this->castleMask) {
        hashKey ^= castleHashKeys[this->castleMask];
        hashKey ^= castleHashKeys[castleMask];
        this->castleMask = castleMask;
    }
}

inline int
Position::getEpSquare() const {
    return epSquare;
}

inline void
Position::setEpSquare(int epSquare) {
    if (this->epSquare != epSquare) {
        hashKey ^= epHashKeys[(this->epSquare >= 0) ? getX(this->epSquare) + 1 : 0];
        hashKey ^= epHashKeys[(epSquare >= 0) ? getX(epSquare) + 1 : 0];
        this->epSquare = epSquare;
    }
}

inline int
Position::getKingSq(bool white) const {
    return white ? wKingSq() : bKingSq();
}

inline void
Position::unMakeMove(const Move& move, const UndoInfo& ui) {
    hashKey ^= whiteHashKey;
    whiteMove = !whiteMove;
    int p = squares[move.to()];
    setPiece(move.from(), p);
    setPiece(move.to(), ui.capturedPiece);
    setCastleMask(ui.castleMask);
    setEpSquare(ui.epSquare);
    halfMoveClock = ui.halfMoveClock;
    bool wtm = whiteMove;
    if (move.promoteTo() != Piece::EMPTY) {
        p = wtm ? Piece::WPAWN : Piece::BPAWN;
        setPiece(move.from(), p);
    }
    if (!wtm)
        fullMoveCounter--;

    // Handle castling
    int king = wtm ? Piece::WKING : Piece::BKING;
    if (p == king) {
        int k0 = move.from();
        if (move.to() == k0 + 2) { // O-O
            movePieceNotPawn(k0 + 1, k0 + 3);
        } else if (move.to() == k0 - 2) { // O-O-O
            movePieceNotPawn(k0 - 1, k0 - 4);
        }
    }

    // Handle en passant
    if (move.to() == epSquare) {
        if (p == Piece::WPAWN) {
            setPiece(move.to() - 8, Piece::BPAWN);
        } else if (p == Piece::BPAWN) {
            setPiece(move.to() + 8, Piece::WPAWN);
        }
    }
}

inline void
Position::unMakeMoveB(const Move& move, const UndoInfo& ui) {
    int p = squares[move.to()];
    setPieceB(move.from(), p);
    setPieceB(move.to(), ui.capturedPiece);
    bool wtm = whiteMove;
    if (move.promoteTo() != Piece::EMPTY) {
        p = wtm ? Piece::WPAWN : Piece::BPAWN;
        setPieceB(move.from(), p);
    }

    // Handle castling
    int king = wtm ? Piece::WKING : Piece::BKING;
    if (p == king) {
        int k0 = move.from();
        if (move.to() == k0 + 2) { // O-O
            movePieceNotPawnB(k0 + 1, k0 + 3);
        } else if (move.to() == k0 - 2) { // O-O-O
            movePieceNotPawnB(k0 - 1, k0 - 4);
        }
    }

    // Handle en passant
    if (move.to() == epSquare) {
        if (p == Piece::WPAWN) {
            setPieceB(move.to() - 8, Piece::BPAWN);
        } else if (p == Piece::BPAWN) {
            setPieceB(move.to() + 8, Piece::WPAWN);
        }
    }
}

inline void
Position::setPieceB(int square, int piece) {
    int removedPiece = squares[square];
    squares[square] = piece;

    // Update bitboards
    const U64 sqMask = 1ULL << square;
    pieceTypeBB_[removedPiece] &= ~sqMask;
    pieceTypeBB_[piece] |= sqMask;

    if (removedPiece != Piece::EMPTY) {
        if (Piece::isWhite(removedPiece))
            whiteBB_ &= ~sqMask;
        else
            blackBB_ &= ~sqMask;
    }

    if (piece != Piece::EMPTY) {
        if (Piece::isWhite(piece)) {
            whiteBB_ |= sqMask;
        } else {
            blackBB_ |= sqMask;
        }
    }
}

inline void
Position::movePieceNotPawnB(int from, int to) {
    const int piece = squares[from];

    squares[from] = Piece::EMPTY;
    squares[to] = piece;

    const U64 sqMaskF = 1ULL << from;
    const U64 sqMaskT = 1ULL << to;
    pieceTypeBB_[piece] &= ~sqMaskF;
    pieceTypeBB_[piece] |= sqMaskT;
    if (Piece::isWhite(piece)) {
        whiteBB_ &= ~sqMaskF;
        whiteBB_ |= sqMaskT;
    } else {
        blackBB_ &= ~sqMaskF;
        blackBB_ |= sqMaskT;
    }
}

inline void
Position::makeSEEMove(const Move& move, UndoInfo& ui) {
    ui.capturedPiece = squares[move.to()];
    int p = squares[move.from()];

    // Handle en passant
    if (move.to() == epSquare) {
        if (p == Piece::WPAWN) {
            setSEEPiece(move.to() - 8, Piece::EMPTY);
        } else if (p == Piece::BPAWN) {
            setSEEPiece(move.to() + 8, Piece::EMPTY);
        }
    }

    // Perform move
    setSEEPiece(move.from(), Piece::EMPTY);
    setSEEPiece(move.to(), p);
    whiteMove = !whiteMove;
}

inline void
Position::unMakeSEEMove(const Move& move, const UndoInfo& ui) {
    whiteMove = !whiteMove;
    int p = squares[move.to()];
    setSEEPiece(move.from(), p);
    setSEEPiece(move.to(), ui.capturedPiece);

    // Handle en passant
    if (move.to() == epSquare) {
        if (p == Piece::WPAWN) {
            setSEEPiece(move.to() - 8, Piece::BPAWN);
        } else if (p == Piece::BPAWN) {
            setSEEPiece(move.to() + 8, Piece::WPAWN);
        }
    }
}

inline int
Position::getSquare(int x, int y) {
    return y * 8 + x;
}

/** Return x position (file) corresponding to a square. */
inline int
Position::getX(int square) {
    return square & 7;
}

/** Return y position (rank) corresponding to a square. */
inline int
Position::getY(int square) {
    return square >> 3;
}

inline int
Position::mirrorY(int square) {
    return square ^ 0x38;
}

/** Return true if (x,y) is a dark square. */
inline bool
Position::darkSquare(int x, int y) {
    return (x & 1) == (y & 1);
}

inline int Position::getFullMoveCounter() const {
    return fullMoveCounter;
}

inline void Position::setFullMoveCounter(int fm) {
    fullMoveCounter = fm;
}

inline int Position::getHalfMoveClock() const {
    return halfMoveClock;
}

inline void Position::setHalfMoveClock(int hm) {
    halfMoveClock = hm;
}

inline int Position::psScore1(int piece) const {
    return psScore1_[piece];
}

inline int Position::psScore2(int piece) const {
    return psScore2_[piece];
}

inline U64 Position::pieceTypeBB(Piece::Type piece) const {
    return pieceTypeBB_[piece];
}

template <typename Piece0, typename... Pieces>
inline U64 Position::pieceTypeBB(Piece0 piece0, Pieces... pieces) const {
    return pieceTypeBB(piece0) | pieceTypeBB(pieces...);
}

inline U64 Position::whiteBB() const {
    return whiteBB_;
}

inline U64 Position::blackBB() const {
    return blackBB_;
};

inline U64 Position::colorBB(int wtm) const {
    return wtm ? whiteBB_ : blackBB_;
}

inline U64 Position::occupiedBB() const {
    return whiteBB() | blackBB();
}

inline int Position::wKingSq() const {
    return BitBoard::firstSquare(pieceTypeBB_[Piece::WKING]);
}

inline int Position::bKingSq() const {
    return BitBoard::firstSquare(pieceTypeBB_[Piece::BKING]);
}

inline int Position::wMtrl() const {
    return wMtrl_;
}

inline int Position::bMtrl() const {
    return bMtrl_;
}

inline int Position::wMtrlPawns() const {
    return wMtrlPawns_;
}

inline int Position::bMtrlPawns() const {
    return bMtrlPawns_;
}

inline U64 Position::getHashKey(int piece, int square) {
    return psHashKeys[piece][square];
}

#endif /* POSITION_HPP_ */
