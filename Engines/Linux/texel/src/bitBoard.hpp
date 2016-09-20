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
 * bitBoard.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BITBOARD_HPP_
#define BITBOARD_HPP_

#include "util/util.hpp"
#include "util/alignedAlloc.hpp"

class BitBoard {
public:
    /** Squares attacked by a king on a given square. */
    static U64 kingAttacks[64];
    static U64 knightAttacks[64];
    static U64 wPawnAttacks[64], bPawnAttacks[64];

    // Squares preventing a pawn from being a passed pawn, if occupied by enemy pawn
    static U64 wPawnBlockerMask[64], bPawnBlockerMask[64];

    static const U64 maskAToGFiles = 0x7F7F7F7F7F7F7F7FULL;
    static const U64 maskBToHFiles = 0xFEFEFEFEFEFEFEFEULL;
    static const U64 maskAToFFiles = 0x3F3F3F3F3F3F3F3FULL;
    static const U64 maskCToHFiles = 0xFCFCFCFCFCFCFCFCULL;

    static const U64 maskFile[8];

    // Masks for squares where enemy pawn can capture en passant, indexed by file
    static U64 epMaskW[8], epMaskB[8];

    static const U64 maskRow1      = 0x00000000000000FFULL;
    static const U64 maskRow2      = 0x000000000000FF00ULL;
    static const U64 maskRow3      = 0x0000000000FF0000ULL;
    static const U64 maskRow4      = 0x00000000FF000000ULL;
    static const U64 maskRow5      = 0x000000FF00000000ULL;
    static const U64 maskRow6      = 0x0000FF0000000000ULL;
    static const U64 maskRow7      = 0x00FF000000000000ULL;
    static const U64 maskRow8      = 0xFF00000000000000ULL;
    static const U64 maskRow1Row8  = 0xFF000000000000FFULL;

    static const U64 maskDarkSq    = 0xAA55AA55AA55AA55ULL;
    static const U64 maskLightSq   = 0x55AA55AA55AA55AAULL;

    static const U64 maskCorners   = 0x8100000000000081ULL;


    static U64 bishopAttacks(int sq, U64 occupied) {
        return bTables[sq][(int)(((occupied & bMasks[sq]) * bMagics[sq]) >> (64 - bBits[sq]))];
    }

    static U64 rookAttacks(int sq, U64 occupied) {
        return rTables[sq][(int)(((occupied & rMasks[sq]) * rMagics[sq]) >> (64 - rBits[sq]))];
    }

    static U64 squaresBetween[64][64];

    static int getDirection(int from, int to) {
        int offs = to + (to|7) - from - (from|7) + 0x77;
        return dirTable[offs];
    }

    static int getKingDistance(int from, int to) {
        int offs = to + (to|7) - from - (from|7) + 0x77;
        return kingDistTable[offs];
    }

    static int getTaxiDistance(int from, int to) {
        int offs = to + (to|7) - from - (from|7) + 0x77;
        return taxiDistTable[offs];
    }

    static U64 southFill(U64 mask) {
        mask |= (mask >> 8);
        mask |= (mask >> 16);
        mask |= (mask >> 32);
        return mask;
    }

    static U64 northFill(U64 mask) {
        mask |= (mask << 8);
        mask |= (mask << 16);
        mask |= (mask << 32);
        return mask;
    }

    static int numberOfTrailingZeros(U64 mask) {
#ifdef HAVE_CTZ
        if (sizeof(U64) == sizeof(long))
            return __builtin_ctzl(mask);
        else if (sizeof(U64) == sizeof(long long))
            return __builtin_ctzll(mask);
#endif
        return trailingZ[(int)(((mask & -mask) * 0x07EDD5E59A4E28C2ULL) >> 58)];
    }

    /** Return number of 1 bits in mask. */
    static int bitCount(U64 mask) {
#ifdef HAVE_POPCNT
        if (sizeof(U64) == sizeof(long))
            return __builtin_popcountl(mask);
        else if (sizeof(U64) == sizeof(long long))
            return __builtin_popcountl(mask >> 32) +
                   __builtin_popcountl(mask & 0xffffffffULL);
#endif
        const U64 k1 = 0x5555555555555555ULL;
        const U64 k2 = 0x3333333333333333ULL;
        const U64 k4 = 0x0f0f0f0f0f0f0f0fULL;
        const U64 kf = 0x0101010101010101ULL;
        U64 t = mask;
        t -= (t >> 1) & k1;
        t = (t & k2) + ((t >> 2) & k2);
        t = (t + (t >> 4)) & k4;
        t = (t * kf) >> 56;
        return (int)t;
    }

    /** Initialize static data. */
    static void staticInitialize();

private:
    static U64* rTables[64];
    static U64 rMasks[64];
    static int rBits[64];
    static const U64 rMagics[64];

    static U64* bTables[64];
    static U64 bMasks[64];
    static const int bBits[64];
    static const U64 bMagics[64];

    static vector_aligned<U64> tableData;

    static const byte dirTable[];
    static const byte kingDistTable[];
    static const byte taxiDistTable[];
    static const int trailingZ[64];
};


#endif /* BITBOARD_HPP_ */
