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

#include "Bits.h"
#include <iostream>
Bits::Bits() {
    //LINK_ROOKS
    LINK_ROOKS = ( u64** ) malloc ( 64 * sizeof ( u64* ) );

    for ( int i = 0; i < 64; i++ ) {
        LINK_ROOKS[i] = ( u64* ) malloc ( 64 * sizeof ( u64 ) );
    }

    int from, to;

    for ( int i = 0; i < 64; i++ ) {
        for ( int j = 0; j < 64; j++ ) {
            u64 t = 0;

            if ( RANK[i] & RANK[j] ) { //rank
                from = min ( i, j );
                to = max ( i, j );

                for ( int k = from + 1; k <= to - 1; k++ ) {
                    t |= POW2[k];
                }
            }
            else if ( FILE_[i] & FILE_[j] ) { //file
                from = min ( i, j );
                to = max ( i, j );

                for ( int k = from + 8; k <= to - 8; k += 8 ) {
                    t |= POW2[k];
                }
            }

            if ( !t ) { t = 0xffffffffffffffffULL; }

            LINK_ROOKS[i][j] = t;
        }
    }

    //DISTANCE
    for ( int i = 0; i < 64; i++ ) {
        for ( int j = 0; j < 64; j++ ) {
            DISTANCE[i][j] = max ( abs ( RANK_AT[i] - FILE_AT[j] ), abs ( RANK_AT[j] - FILE_AT[i] ) );
        }
    }

    ///
    u64 MASK_BIT_SET[64][64];
    memset ( MASK_BIT_SET, 0, sizeof ( MASK_BIT_SET ) );

    for ( int i = 0; i < 64; i++ ) {
        for ( int j = 0; j < 64; j++ ) {
            int a = min ( i, j );
            int b = max ( i, j );
            MASK_BIT_SET[i][i] = 0;

            for ( int e = a; e <= b; e++ ) {
                u64 r = ( RANK[i] | POW2[i] ) & ( RANK[j] | POW2[j] );

                if ( r ) { MASK_BIT_SET[i][j] |= POW2[e] & r; }
                else {
                    r = ( FILE_[i] | POW2[i] ) & ( FILE_[j] | POW2[j] );

                    if ( r ) { MASK_BIT_SET[i][j] |= POW2[e] & r; }
                    else {
                        r = ( LEFT_DIAG[i] | POW2[i] ) & ( LEFT_DIAG[j] | POW2[j] );

                        if ( r ) {
                            MASK_BIT_SET[i][j] |= POW2[e] & r;
                        }
                        else {
                            r = ( RIGHT_DIAG[i] | POW2[i] ) & ( RIGHT_DIAG[j] | POW2[j] );

                            if ( r ) { MASK_BIT_SET[i][j] |= POW2[e] & r; }
                        }
                    }
                }
            }

            if ( i == j ) { MASK_BIT_SET[i][i] &= NOTPOW2[i]; }
        }
    }

    for ( int i = 0; i < 64; i++ ) {
        for ( int j = 0; j < 64; j++ ) {
            MASK_BIT_SET_NOBOUND[i][j] = MASK_BIT_SET[i][j];
            MASK_BIT_SET_NOBOUND[i][j] &= NOTPOW2[i];
            MASK_BIT_SET_NOBOUND[i][j] &= NOTPOW2[j];
            MASK_BIT_SET[i][j] &= NOTPOW2[i];
        }
    }

    for ( int i = 0; i < 64; i++ ) {
        for ( int j = 0; j < 64; j++ ) {
            MASK_BIT_SET_COUNT[i][j] = bitCount ( MASK_BIT_SET[i][j] );
            MASK_BIT_SET_NOBOUND_COUNT[i][j] = bitCount ( MASK_BIT_SET_NOBOUND[i][j] );
        }
    }
}

Bits::~Bits() {
    for ( int i = 0; i < 64; i++ ) {
        free ( LINK_ROOKS[i] );
    }

    free ( LINK_ROOKS );
}


