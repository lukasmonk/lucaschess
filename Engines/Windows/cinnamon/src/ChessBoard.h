/*
    Cinnamon is a UCI chess engine
    Copyright (C) 2011-2014 Giuseppe Cannella

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

#ifndef CHESSBOARD_H_
#define CHESSBOARD_H_
#include <iostream>
#include <string.h>
#include <sstream>
#include "String.h"
#include "Bits.h"
#include "namespaces.h"

using namespace _board;

class ChessBoard : protected Bits {
public:

    ChessBoard();
    virtual ~ChessBoard();
    static const uchar RIGHT_KING_CASTLE_WHITE_MASK = 0x10;
    static const uchar RIGHT_QUEEN_CASTLE_WHITE_MASK = 0x20;
    static const uchar RIGHT_KING_CASTLE_BLACK_MASK = 0x40;
    static const uchar RIGHT_QUEEN_CASTLE_BLACK_MASK = 0x80;
    static const u64 CENTER_MASK = 0x1818000000ULL;
    static const u64 BIG_DIAG_LEFT = 0x102040810204080ULL;
    static const u64 BIG_DIAG_RIGHT = 0x8040201008040201ULL;
    static const int SQUARE_FREE = 12;
    static const int PAWN_BLACK = 0;
    static const int PAWN_WHITE = 1;
    static const int ROOK_BLACK = 2;
    static const int ROOK_WHITE = 3;
    static const int BISHOP_BLACK = 4;
    static const int BISHOP_WHITE = 5;
    static const int KNIGHT_BLACK = 6;
    static const int KNIGHT_WHITE = 7;
    static const int KING_BLACK = 8;
    static const int KING_WHITE = 9;
    static const int QUEEN_BLACK = 10;
    static const int QUEEN_WHITE = 11;
    static const int NO_ENPASSANT = -1;
    void display();
    string getFen();
    char decodeBoard ( string ) ;
    virtual int loadFen ( string );
    int getPieceByChar ( char ) ;
#ifdef DEBUG_MODE
    u64 getBitBoard ( int side );
#endif

    template <int side> u64 getBitBoard() {
        return chessboard[PAWN_BLACK + side] | chessboard[ROOK_BLACK + side] | chessboard[BISHOP_BLACK + side] | chessboard[KNIGHT_BLACK + side]
               | chessboard[KING_BLACK + side] | chessboard[QUEEN_BLACK + side];
    }

    void setSide ( bool b ) {
        sideToMove = b;
    }

    int getSide() {
        return sideToMove;
    }

    template <int side> u64 getBitBoardNoPawns() {
        return chessboard[ROOK_BLACK + side] | chessboard[BISHOP_BLACK + side] | chessboard[KNIGHT_BLACK + side]
               | chessboard[KING_BLACK + side] | chessboard[QUEEN_BLACK + side];
    }

    template <int side> int getPieceAt ( u64 bitmapPos ) {
        return ( ( chessboard[PAWN_BLACK + side] & bitmapPos ) ? PAWN_BLACK + side :
                 ( ( chessboard[ROOK_BLACK + side] & bitmapPos ) ? ROOK_BLACK + side :
                   ( ( chessboard[BISHOP_BLACK + side] & bitmapPos ) ? BISHOP_BLACK + side :
                     ( ( chessboard[KNIGHT_BLACK + side] & bitmapPos ) ? KNIGHT_BLACK + side :
                       ( ( chessboard[QUEEN_BLACK + side] & bitmapPos ) ? QUEEN_BLACK + side :
                         ( ( chessboard[KING_BLACK + side] & bitmapPos ) ? KING_BLACK + side : SQUARE_FREE ) ) ) ) ) );
    }

protected:

    typedef struct {
        u64 allPieces;
        u64 kingAttackers[2];
        u64 allPiecesSide[2];
        u64 pawns[2];
        u64 rooks[2];
        u64 openColumn;
        u64 semiOpenColumn[2];
        u64 isolated[2];
        u64 allPiecesNoPawns[2];
        int kingSecurityDistance[2];
        uchar posKing[2];
    } _Tboard;

    static const u64 A7bit = 0x80000000000000ULL;
    static const u64 B7bit = 0x40000000000000ULL;
    static const u64 C6bit = 0x200000000000ULL;
    static const u64 H7bit = 0x1000000000000ULL;
    static const u64 G7bit = 0x2000000000000ULL;
    static const u64 F6bit = 0x40000000000ULL;
    static const u64 A8bit = 0x8000000000000000ULL;
    static const u64 H8bit = 0x100000000000000ULL;
    static const u64 A2bit = 0x8000ULL;
    static const u64 B2bit = 0x4000ULL;
    static const u64 H2bit = 0x100ULL;
    static const u64 G2bit = 0x200ULL;
    static const u64 A1bit = 0x80ULL;
    static const u64 H1bit = 0x1ULL;
    static const u64 F1G1bit = 0x6ULL;
    static const u64 H1H2G1bit = 0x103ULL;
    static const u64 C1B1bit = 0x60ULL;
    static const u64 A1A2B1bit = 0x80c0ULL;
    static const u64 F8G8bit = 0x600000000000000ULL;
    static const u64 H8H7G8bit = 0x301000000000000ULL;
    static const u64 C8B8bit = 0x6000000000000000ULL;
    static const u64 A8A7B8bit = 0xc080000000000000ULL;
    static const u64 C6A6bit = 0xa00000000000ULL;
    static const u64 F6H6bit = 0x50000000000ULL;
    static const u64 A7C7bit = 0xa0000000000000ULL;
    static const u64 H7G7bit = 0x3000000000000ULL;
    static const u64 C3A3bit = 0xa00000ULL;
    static const u64 F3H3bit = 0x50000ULL;
    static const u64 A2C2bit = 0xa000ULL;
    static const u64 H2G2bit = 0x300ULL;

    static const int E1 = 3;
    static const int E8 = 59;
    static const int C1 = 5;
    static const int F1 = 2;
    static const int C8 = 61;
    static const int F8 = 58;
    static const int D8 = 60;
    static const int A8 = 63;
    static const int H8 = 56;
    static const int G8 = 57;
    static const u64 BLACK_SQUARES = 0x55AA55AA55AA55AAULL;
    static const u64 WHITE_SQUARES = 0xAA55AA55AA55AA55ULL;
    static const uchar KING_SIDE_CASTLE_MOVE_MASK = 0x4;
    static const uchar QUEEN_SIDE_CASTLE_MOVE_MASK = 0x8;

    u64 zobristKey;
    int enpassantPosition;
    uchar rightCastle;
    _Tchessboard chessboard;
    _Tboard structure;
    bool sideToMove;
    int friendKing[2];
    string boardToFen();
    string decodeBoardinv ( const uchar type, const int a, const int side );
    void makeZobristKey();

    template <int side> int getNpiecesNoPawnNoKing() {
        return bitCount ( chessboard[ROOK_BLACK + side] | chessboard[BISHOP_BLACK + side] | chessboard[KNIGHT_BLACK + side] | chessboard[QUEEN_BLACK + side] );
    }
#ifdef DEBUG_MODE
    void updateZobristKey ( int piece, int position ) {
        ASSERT_RANGE ( position, 0, 63 );
        ASSERT ( piece != 12 );
        ASSERT_RANGE ( piece, 0, 14 );
        zobristKey ^= _random::RANDOM_KEY[piece][position];
    }

    int getPieceAt ( int side, u64 bitmapPos );
#else
#define updateZobristKey(piece,  position) (zobristKey ^= _random::RANDOM_KEY[piece][position])

#endif
private:
    string fenString;
    void setRightCastle ( uchar r );
    int loadFen();
    uchar getRightCastle();

};
#endif

