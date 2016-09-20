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

#ifndef TABLEBASE_H
#define TABLEBASE_H
#include "gtb/gtb-probe.h"
#include "namespaces.h"
#include "ChessBoard.h"

class Tablebase: private Bits {
public:

    Tablebase();
    virtual ~Tablebase();
    void cacheInit ( int mb );
    bool getAvailable();
    int getCache();
    string getPath();
    string getSchema();
    bool setCacheSize ( int mb );
    void setPath ( string path ) ;
    bool setScheme ( string s );
    void restart();
    bool setProbeDepth ( int d );
    bool setInstalledPieces ( int n );

    bool isInstalledPieces ( int p ) {
        ASSERT ( p < 33 );
        return installedPieces[p];
    }

    int getProbeDepth() {
        return probeDepth;
    }

    template <int side, bool doPrint>
    int getDtm ( _Tchessboard& chessboard, uchar rightCastle, int depth ) {
        unsigned int  ws[17];   /* list of squares for white */
        unsigned int  bs[17];   /* list of squares for black */
        unsigned char wp[17];   /* what white pieces are on those squares */
        unsigned char bp[17];   /* what black pieces are on those squares */
        unsigned info = tb_UNKNOWN; /* default, no tbvalue */
        unsigned pliestomate;
        int count = 0;

        //white
        for ( int piece = 1; piece < 12; piece += 2 ) {
            u64 b = chessboard[piece];

            while ( b ) {
                int position = BITScanForward ( b );
                ws[count] = DECODE_POSITION[position];
                wp[count] = DECODE_PIECE[piece];
                count++;
                b &= NOTPOW2[position];
            }
        }

        ws[count] = tb_NOSQUARE;        /* it marks the end of list */
        wp[count] = tb_NOPIECE;         /* it marks the end of list */
        //black
        count = 0;

        for ( int piece = 0; piece < 12; piece += 2 ) {
            u64 b = chessboard[piece];

            while ( b ) {
                int position = BITScanForward ( b );
                bs[count] = DECODE_POSITION[position];
                bp[count] = DECODE_PIECE[piece];
                count++;
                b &= NOTPOW2[position];
            }
        }

        bs[count] = tb_NOSQUARE;
        bp[count] = tb_NOPIECE;
        unsigned int tb_castling = 0;
        tb_castling  = rightCastle & ChessBoard::RIGHT_QUEEN_CASTLE_WHITE_MASK ? tb_WOOO : 0;
        tb_castling |= rightCastle & ChessBoard::RIGHT_KING_CASTLE_WHITE_MASK ? tb_WOO : 0;
        tb_castling |= rightCastle & ChessBoard::RIGHT_KING_CASTLE_BLACK_MASK ? tb_BOO : 0;
        tb_castling |= rightCastle & ChessBoard::RIGHT_QUEEN_CASTLE_BLACK_MASK ? tb_BOOO : 0;
        int  tb_available = 0;

        if ( depth > 8 ) {
            tb_available = tb_probe_hard ( side ^ 1, tb_NOSQUARE, tb_castling, ws, bs, wp, bp, &info, &pliestomate );
        }
        else if ( depth >= probeDepth ) {
            tb_available = tb_probe_soft ( side ^ 1, tb_NOSQUARE, tb_castling, ws, bs, wp, bp, &info, &pliestomate );
        }

        return extractDtm< side ^ 1, doPrint> ( tb_available, info, pliestomate );
    }


private:

    const int DECODE_PIECE[13] = {
        tb_PAWN, tb_PAWN,
        tb_ROOK, tb_ROOK,
        tb_BISHOP, tb_BISHOP,
        tb_KNIGHT, tb_KNIGHT,
        tb_KING, tb_KING,
        tb_QUEEN, tb_QUEEN,
        tb_NOPIECE
    };

    const int DECODE_POSITION[64] = {
        tb_H1, tb_G1, tb_F1, tb_E1, tb_D1, tb_C1, tb_B1, tb_A1,
        tb_H2, tb_G2, tb_F2, tb_E2, tb_D2, tb_C2, tb_B2, tb_A2,
        tb_H3, tb_G3, tb_F3, tb_E3, tb_D3, tb_C3, tb_B3, tb_A3,
        tb_H4, tb_G4, tb_F4, tb_E4, tb_D4, tb_C4, tb_B4, tb_A4,
        tb_H5, tb_G5, tb_F5, tb_E5, tb_D5, tb_C5, tb_B5, tb_A5,
        tb_H6, tb_G6, tb_F6, tb_E6, tb_D6, tb_C6, tb_B6, tb_A6,
        tb_H7, tb_G7, tb_F7, tb_E7, tb_D7, tb_C7, tb_B7, tb_A7,
        tb_H8, tb_G8, tb_F8, tb_E8, tb_D8, tb_C8, tb_B8, tb_A8,
    };

    template <unsigned stm1, bool doPrint >
    int extractDtm ( int tb_available1, unsigned info1, unsigned pliestomate1 ) {
        if ( doPrint ) {
            print ( stm1, info1, pliestomate1 );
        }

        if ( tb_available1 ) {
            if ( info1 == tb_DRAW ) {
                return 0;
            }

            if ( info1 == tb_WMATE && stm1 == tb_WHITE_TO_MOVE ) {
                return pliestomate1 ;
            }

            if ( info1 == tb_BMATE && stm1 == tb_BLACK_TO_MOVE )  {
                return pliestomate1 ;
            }

            if ( info1 == tb_WMATE && stm1 == tb_BLACK_TO_MOVE )  {
                return -pliestomate1 ;
            }

            if ( info1 == tb_BMATE && stm1 == tb_WHITE_TO_MOVE ) {
                return -pliestomate1 ;
            }
        }

        return INT_MAX;
    }

    void print ( unsigned stm1, unsigned info1, unsigned pliestomate1 );
    void load();

    const int verbosity = 0;
    int cacheSize = 32;//mb
    const char ** paths = nullptr;
    string path = "gtb/gtb4"; int scheme = tb_CP4;
    int probeDepth = 0;
    bool installedPieces[33] ; // 3,4,5
    const int wdl_fraction = 96;

};

#endif

