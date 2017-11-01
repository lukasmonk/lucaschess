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
 * textio.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef TEXTIO_HPP_
#define TEXTIO_HPP_

#include <string>
#include <vector>

#include "chessParseError.hpp"
#include "position.hpp"
#include "util/util.hpp"


class MoveGen;

/**
 * Conversion between text and binary formats.
 */
class TextIO {
public:
    static const std::string startPosFEN;

    /** Parse a FEN string and return a chess Position object. */
    static Position readFEN(const std::string& fen);

    /** Remove pseudo-legal EP square if it is not legal, ie would leave king in check. */
    static void fixupEPSquare(Position& pos);

    /** Return a FEN string corresponding to a chess Position object. */
    static std::string toFEN(const Position& pos);

    /**
     * Convert a chess move to human readable form.
     * @param pos      The chess position.
     * @param move     The executed move.
     * @param longForm If true, use long notation, eg Ng1-f3.
     *                 Otherwise, use short notation, eg Nf3
     */
    static std::string moveToString(const Position& pos, const Move& move, bool longForm);

    /** Convert a move object to UCI string format. */
    static std::string moveToUCIString(const Move& m);

    /**
     * Convert a string to a Move object.
     * @return A move object, or empty move if move has invalid syntax.
     */
    static Move uciStringToMove(const std::string& move);

    /**
     * Convert a chess move string to a Move object.
     * The string may specify any combination of piece/source/target/promotion
     * information as long as it matches exactly one valid move.
     */
    static Move stringToMove(Position& pos, const std::string& strMove);

    /**
     * Convert a string, such as "e4" to a square number.
     * @return The square number, or -1 if not a legal square.
     */
    static int getSquare(const std::string& s);

    /**
     * Convert a square number to a string, such as "e4".
     */
    static std::string squareToString(int square);

    /**
     * Create an ASCII representation of a position.
     */
    static std::string asciiBoard(const Position& pos);

    /** Create an ASCII representation of a bitmask. */
    static std::string asciiBoard(U64 mask);

    /** Create a comma separated list of squares corresponding to mask. */
    static std::string squareList(U64 mask);

private:
    static void safeSetPiece(Position& pos, int col, int row, int p);

    static int charToPiece(bool white, char c);
};

inline int
TextIO::getSquare(const std::string& s)
{
    int x = s[0] - 'a';
    int y = s[1] - '1';
    if ((x < 0) || (x > 7) || (y < 0) || (y > 7))
        return -1;

    return Position::getSquare(x, y);
}

inline std::string
TextIO::squareToString(int square)
{
    std::string ret;
    int x = Position::getX(square);
    int y = Position::getY(square);
    ret += (char)(((x + 'a')));
    ret += (char)(((y + '1')));
    return ret;
}

inline void
TextIO::safeSetPiece(Position& pos, int col, int row, int p) {
    if (col > 7) throw ChessParseError("Too many columns");
    if ((p == Piece::WPAWN) || (p == Piece::BPAWN))
        if ((row == 0) || (row == 7))
            throw ChessParseError("Pawn on first/last rank");
    pos.setPiece(Position::getSquare(col, row), p);
}

inline int
TextIO::charToPiece(bool white, char c) {
    switch (c) {
    case 'Q': case 'q': return white ? Piece::WQUEEN  : Piece::BQUEEN;
    case 'R': case 'r': return white ? Piece::WROOK   : Piece::BROOK;
    case 'B':           return white ? Piece::WBISHOP : Piece::BBISHOP;
    case 'N': case 'n': return white ? Piece::WKNIGHT : Piece::BKNIGHT;
    case 'K': case 'k': return white ? Piece::WKING   : Piece::BKING;
    case 'P': case 'p': return white ? Piece::WPAWN   : Piece::BPAWN;
    }
    return -1;
}

inline
std::ostream& operator<<(std::ostream& os, const Move& m) {
    os << TextIO::moveToUCIString(m);
    return os;
}

#endif /* TEXTIO_HPP_ */
