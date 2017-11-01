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
 * bitBoard.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef BITBOARD_HPP_
#define BITBOARD_HPP_

#include "util/util.hpp"
#include "util/alignedAlloc.hpp"


#ifdef HAS_BMI2
#include <immintrin.h>
inline U64 pext(U64 value, U64 mask) {
    return _pext_u64(value, mask);
}
#endif

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

class BitBoard {
public:
    /** Squares attacked by a king on a given square. */
    static U64 kingAttacks[64];
    static U64 knightAttacks[64];
    static U64 wPawnAttacks[64], bPawnAttacks[64];

    // Squares preventing a pawn from being a passed pawn, if occupied by enemy pawn
    static U64 wPawnBlockerMask[64], bPawnBlockerMask[64];

    static const U64 maskFileA = 0x0101010101010101ULL;
    static const U64 maskFileB = 0x0202020202020202ULL;
    static const U64 maskFileC = 0x0404040404040404ULL;
    static const U64 maskFileD = 0x0808080808080808ULL;
    static const U64 maskFileE = 0x1010101010101010ULL;
    static const U64 maskFileF = 0x2020202020202020ULL;
    static const U64 maskFileG = 0x4040404040404040ULL;
    static const U64 maskFileH = 0x8080808080808080ULL;

    static const U64 maskAToGFiles = 0x7F7F7F7F7F7F7F7FULL;
    static const U64 maskBToHFiles = 0xFEFEFEFEFEFEFEFEULL;
    static const U64 maskAToFFiles = 0x3F3F3F3F3F3F3F3FULL;
    static const U64 maskCToHFiles = 0xFCFCFCFCFCFCFCFCULL;

    static const U64 maskAToDFiles = 0x0F0F0F0F0F0F0F0FULL;
    static const U64 maskEToHFiles = 0xF0F0F0F0F0F0F0F0ULL;

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

    /** Convert one or more squares to a bitmask. */
    static U64 sqMask(Square sq) { return 1ULL << sq; }
    template <typename Sq0, typename... Squares> static U64 sqMask(Sq0 sq0, Squares... squares) {
        return sqMask(sq0) | sqMask(squares...);
    }

    /** Mirror a bitmask in the X or Y direction. */
    static U64 mirrorX(U64 mask);
    static U64 mirrorY(U64 mask);


    static U64 bishopAttacks(int sq, U64 occupied);
    static U64 rookAttacks(int sq, U64 occupied);

    /** Shift mask in the NW and NE directions. */
    static U64 wPawnAttacksMask(U64 mask);
    /** Shift mask in the SW and SE directions. */
    static U64 bPawnAttacksMask(U64 mask);

    static U64 squaresBetween[64][64];

    /** Get direction between two squares, 8*sign(dy) + sign(dx) */
    static int getDirection(int from, int to);

    /** Get the max norm distance between two squares. */
    static int getKingDistance(int from, int to);

    /** Get the L1 norm distance between two squares. */
    static int getTaxiDistance(int from, int to);

    static U64 southFill(U64 mask);

    static U64 northFill(U64 mask);

    /** Get the lowest square from mask. */
    static int firstSquare(U64 mask);

    /** Get the lowest square from mask and remove the corresponding bit in mask. */
    static int extractSquare(U64& mask);

    /** Return number of 1 bits in mask. */
    static int bitCount(U64 mask);

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

    static const S8 dirTable[];
    static const S8 kingDistTable[];
    static const S8 taxiDistTable[];
    static const int trailingZ[64];
};

inline U64
BitBoard::mirrorX(U64 mask) {
    U64 k1 = 0x5555555555555555ULL;
    U64 k2 = 0x3333333333333333ULL;
    U64 k3 = 0x0f0f0f0f0f0f0f0fULL;
    U64 t = mask;
    t = ((t >> 1) & k1) | ((t & k1) << 1);
    t = ((t >> 2) & k2) | ((t & k2) << 2);
    t = ((t >> 4) & k3) | ((t & k3) << 4);
    return t;
}

inline U64
BitBoard::mirrorY(U64 mask) {
    U64 k1 = 0x00ff00ff00ff00ffULL;
    U64 k2 = 0x0000ffff0000ffffULL;
    U64 t = mask;
    t = ((t >>  8) & k1) | ((t & k1) <<  8);
    t = ((t >> 16) & k2) | ((t & k2) << 16);
    t = ((t >> 32)     ) | ((t     ) << 32);
    return t;
}

inline U64
BitBoard::bishopAttacks(int sq, U64 occupied) {
#ifdef HAS_BMI2
    return bTables[sq][pext(occupied, bMasks[sq])];
#else
    return bTables[sq][(int)(((occupied & bMasks[sq]) * bMagics[sq]) >> bBits[sq])];
#endif
}

inline U64
BitBoard::rookAttacks(int sq, U64 occupied) {
#ifdef HAS_BMI2
    return rTables[sq][pext(occupied, rMasks[sq])];
#else
    return rTables[sq][(int)(((occupied & rMasks[sq]) * rMagics[sq]) >> rBits[sq])];
#endif
}

inline U64
BitBoard::wPawnAttacksMask(U64 mask) {
    return ((mask & maskBToHFiles) << 7) |
           ((mask & maskAToGFiles) << 9);
}

inline U64
BitBoard::bPawnAttacksMask(U64 mask) {
    return ((mask & maskBToHFiles) >> 9) |
           ((mask & maskAToGFiles) >> 7);
}

inline int
BitBoard::getDirection(int from, int to) {
    int offs = to + (to|7) - from - (from|7) + 0x77;
    return dirTable[offs];
}

inline int
BitBoard::getKingDistance(int from, int to) {
    int offs = to + (to|7) - from - (from|7) + 0x77;
    return kingDistTable[offs];
}

inline int
BitBoard::getTaxiDistance(int from, int to) {
    int offs = to + (to|7) - from - (from|7) + 0x77;
    return taxiDistTable[offs];
}

inline U64
BitBoard::southFill(U64 mask) {
    mask |= (mask >> 8);
    mask |= (mask >> 16);
    mask |= (mask >> 32);
    return mask;
}

inline U64
BitBoard::northFill(U64 mask) {
    mask |= (mask << 8);
    mask |= (mask << 16);
    mask |= (mask << 32);
    return mask;
}

inline int
BitBoard::firstSquare(U64 mask) {
#ifdef HAS_CTZ
    if (sizeof(U64) == sizeof(long))
        return __builtin_ctzl(mask);
    else if (sizeof(U64) == sizeof(long long))
        return __builtin_ctzll(mask);
#endif
    return trailingZ[(int)(((mask & -mask) * 0x07EDD5E59A4E28C2ULL) >> 58)];
}

inline int
BitBoard::extractSquare(U64& mask) {
    int ret = firstSquare(mask);
    mask &= mask - 1;
    return ret;
}

inline int
BitBoard::bitCount(U64 mask) {
#ifdef HAS_POPCNT
    if (sizeof(U64) == sizeof(long))
        return __builtin_popcountl(mask);
    else if (sizeof(U64) == 2*sizeof(long))
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

#endif /* BITBOARD_HPP_ */
