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

#ifndef _BITS_H_
#define _BITS_H_
#include "namespaces.h"

class Bits {
public:

    Bits();
    virtual ~Bits();
#ifdef HAS_POPCNT
    static int bitCount ( u64 bits ) {
        return __builtin_popcountll ( bits );
    }
#else
    static int bitCount ( u64 bits ) {
        int count = 0;

        while ( bits ) {
            count++;
            bits &= bits - 1;
        }

        return count;
    }
#endif
protected:

    u64 MASK_BIT_SET_NOBOUND[64][64];
    uchar DISTANCE[64][64];
    char MASK_BIT_SET_COUNT[64][64];
    char MASK_BIT_SET_NOBOUND_COUNT[64][64];

    u64** LINK_ROOKS;

    template <int side, int shift>  static u64 shiftForward ( const u64 bits ) {
        return side == WHITE ? bits << shift : bits>>shift;
    }



#ifdef HAS_BSF
#if UINTPTR_MAX == 0xffffffffffffffff
    static int BITScanForward ( u64 bits ) {
        return __builtin_ffsll ( bits ) - 1;
    }

    static int BITScanReverse ( u64 bits ) {
        return 63 - __builtin_clzll ( bits ) ;
    }
#else
    static int BITScanForward ( u64 bits ) {
        return ( ( unsigned ) bits ) ? __builtin_ffs ( bits ) - 1 : __builtin_ffs ( bits >> 32 ) + 31;
    }

    static int BITScanReverse ( u64 bits ) {
        return ( ( unsigned ) ( bits >> 32 ) ) ? 63 - __builtin_clz ( bits >> 32 ) : 31 - __builtin_clz ( bits ) ;
    }
#endif
#else



    static int BITScanForward ( u64 bb ) {
        //  @author Matt Taylor (2003)
        static const  int lsb_64_table[64] = {
            63, 30,  3, 32, 59, 14, 11, 33,
            60, 24, 50,  9, 55, 19, 21, 34,
            61, 29,  2, 53, 51, 23, 41, 18,
            56, 28,  1, 43, 46, 27,  0, 35,
            62, 31, 58,  4,  5, 49, 54,  6,
            15, 52, 12, 40,  7, 42, 45, 16,
            25, 57, 48, 13, 10, 39,  8, 44,
            20, 47, 38, 22, 17, 37, 36, 26
        };
        unsigned int folded;
        bb ^= bb - 1;
        folded = ( int ) bb ^ ( bb >> 32 );
        return lsb_64_table[folded * 0x78291ACF >> 26];
    }


    static int BITScanReverse ( u64 bb ) {
        // authors Kim Walisch, Mark Dickinson
        static const int index64[64] = {
            0, 47,  1, 56, 48, 27,  2, 60,
            57, 49, 41, 37, 28, 16,  3, 61,
            54, 58, 35, 52, 50, 42, 21, 44,
            38, 32, 29, 23, 17, 11,  4, 62,
            46, 55, 26, 59, 40, 36, 15, 53,
            34, 51, 20, 43, 31, 22, 10, 45,
            25, 39, 14, 33, 19, 30,  9, 24,
            13, 18,  8, 12,  7,  6,  5, 63
        };
        static const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
        bb |= bb >> 1;
        bb |= bb >> 2;
        bb |= bb >> 4;
        bb |= bb >> 8;
        bb |= bb >> 16;
        bb |= bb >> 32;
        return index64[ ( bb * debruijn64 ) >> 58];
    }

#endif

};
#endif

