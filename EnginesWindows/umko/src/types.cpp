/***************************************************************************
 *   Copyright (C) 2009 by Borko Bošković                                  *
 *   borko.boskovic@gmail.com                                              *
 *                                                                         *
 *   This program is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include <iostream>

#include "types.h"

const char PieceChar[15] = {
    'W', 'b', 'P', 'p',
    'N', 'n', 'B', 'b',
    'R', 'r', 'Q', 'q',
    'K', 'k', 'x'
};


const char SanPieceChar[15] = {
    'W', 'B', 'P', 'P',
    'N', 'N', 'B', 'B',
    'R', 'R', 'Q', 'Q',
    'K', 'K', 'x'
};

const std::string SquareString[65] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "xx"
};

char piece_to_char(const Piece p){ return PieceChar[p]; }

char piece_to_san_char(const Piece p){ return SanPieceChar[p]; }

std::string square_to_string(const Square sq){ return SquareString[sq]; }

void bb_print(Bitboard x){
    int i;
    Square sq;
    char stype[72];
    for(i=0; i<71; i++) stype[i] = '0';
    for(i=8; i<71; i+=9) stype[i] = '\n';
    stype[71] = '\0';
    while(x != 0){
        sq = first_bit_clear(x);
        stype[70-rank(sq)*9-(7-file(sq))] = '1';
    }
    std::cout<<stype<<std::endl;
}

#if !defined(__i386__) && !defined(__x86_64__)

static const Square BitTable[64] = {
  Square(0), Square(1), Square(2), Square(7),
  Square(3), Square(13), Square(8), Square(19),
  Square(4), Square(25), Square(14), Square(28),
  Square(9), Square(34), Square(20), Square(40),
  Square(5), Square(17), Square(26), Square(38),
  Square(15), Square(46), Square(29), Square(48),
  Square(10), Square(31), Square(35), Square(54),
  Square(21), Square(50), Square(41), Square(57),
  Square(63), Square(6), Square(12), Square(18),
  Square(24), Square(27), Square(33), Square(39),
  Square(16), Square(37), Square(45), Square(47),
  Square(30), Square(53), Square(49), Square(56),
  Square(62), Square(11), Square(23), Square(32),
  Square(36), Square(44), Square(52), Square(55),
  Square(61), Square(22), Square(43), Square(51),
  Square(60), Square(42), Square(59), Square(58)
};

Square first_bit(Bitboard b) {
    return BitTable[((b & -b) * 0x218a392cd3d5dbfULL) >> 58];
}

Square last_bit(Bitboard b){
    Square sq = 63;
    if (!(b & 0xffffffff00000000ull)){b <<= 32; sq -= 32;}
    if (!(b & 0xffff000000000000ull)){b <<= 16; sq -= 16;}
    if (!(b & 0xff00000000000000ull)){b <<= 8; sq -= 8;}
    if (!(x & 0xf000000000000000ull)){b <<= 4; sq -= 4;}
    if (!(x & 0xc000000000000000ull)){b <<= 2; sq -= 2;}
    if (!(x & 0x8000000000000000ull)){b <<= 1; sq -= 1;}
    return sq;

    /*
    Bitboard x = bb & ~(bb >>> 32);
    int exp = x >> 52;
    int sign = (exp >> 11) & 63;
    exp = (exp & 2047)-1023;
    return exp | sign;
    */
}

Square first_bit_clear(Bitboard& b) {
  Bitboard bb = b;
  b &= (b - 1);
  return BitTable[((bb & -bb) * 0x218a392cd3d5dbfULL) >> 58];
}

#endif

Piece char_to_piece(const char c){
    switch(c){
        case 'P': return WP;
        case 'N': return WN;
        case 'B': return WB;
        case 'R': return WR;
        case 'Q': return WQ;
        case 'K': return WK;
        case 'p': return BP;
        case 'n': return BN;
        case 'b': return BB;
        case 'r': return BR;
        case 'q': return BQ;
        case 'k': return BK;
        default: return NO_PIECE;
    }
}

Square string_to_sq(const std::string s){
    int f=-1, r=-1;
    switch (s[0]){
        case 'a':{f=0; break;}
        case 'b':{f=1; break;}
        case 'c':{f=2; break;}
        case 'd':{f=3; break;}
        case 'e':{f=4; break;}
        case 'f':{f=5; break;}
        case 'g':{f=6; break;}
        case 'h':{f=7; break;}
    };
    switch (s[1]){
        case '1':{r=0; break;}
        case '2':{r=1; break;}
        case '3':{r=2; break;}
        case '4':{r=3; break;}
        case '5':{r=4; break;}
        case '6':{r=5; break;}
        case '7':{r=6; break;}
        case '8':{r=7; break;}
    };
    return Square((r*8)+f);
}

std::string move_to_string(const Move m){
    Square to = move_to(m);
    int type = move_type(m);
    if(type >= MovePN && type <= MovePQ)
        return square_to_string(move_from(m)) + square_to_string(to) + piece_to_char(Piece(type+Black));
    return square_to_string(move_from(m)) + square_to_string(to);
}

