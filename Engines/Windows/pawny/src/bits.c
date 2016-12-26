/*--------------------------------------------------------------------------
    Pawny 0.3.1, chess engine (source code).
    Copyright (C) 2009 - 2011 by Mincho Georgiev.
    
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
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#include "data.h"

const bitboard_t file_mask[8] = {FMASK_A, FMASK_B, FMASK_C, FMASK_D, FMASK_E, FMASK_F, FMASK_G, FMASK_H};

const bitboard_t isolated_mask[8] = 
{ 0x202020202020202ULL, 0x505050505050505ULL, 0xa0a0a0a0a0a0a0aULL, 0x1414141414141414ULL, 
  0x2828282828282828ULL, 0x5050505050505050ULL, 0xa0a0a0a0a0a0a0a0ULL, 0x4040404040404040ULL
};

const bitboard_t passer_mask[2][64] = 
{ { 0,0,0,0,0,0,0,0,
    0x3030303030000ULL,  0x7070707070000ULL,  0xe0e0e0e0e0000ULL,  0x1c1c1c1c1c0000ULL,
    0x38383838380000ULL, 0x70707070700000ULL, 0xe0e0e0e0e00000ULL, 0xc0c0c0c0c00000ULL,
    0x3030303000000ULL,  0x7070707000000ULL,  0xe0e0e0e000000ULL,  0x1c1c1c1c000000ULL,
    0x38383838000000ULL, 0x70707070000000ULL, 0xe0e0e0e0000000ULL, 0xc0c0c0c0000000ULL,
    0x3030300000000ULL,  0x7070700000000ULL,  0xe0e0e00000000ULL,  0x1c1c1c00000000ULL,
    0x38383800000000ULL, 0x70707000000000ULL, 0xe0e0e000000000ULL, 0xc0c0c000000000ULL,
    0x3030000000000ULL,  0x7070000000000ULL,  0xe0e0000000000ULL,  0x1c1c0000000000ULL,
    0x38380000000000ULL, 0x70700000000000ULL, 0xe0e00000000000ULL, 0xc0c00000000000ULL,
    0x3000000000000ULL,  0x7000000000000ULL,  0xe000000000000ULL,  0x1c000000000000ULL,
    0x38000000000000ULL, 0x70000000000000ULL, 0xe0000000000000ULL, 0xc0000000000000ULL,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0x300ULL,         0x700ULL,         0xe00ULL,         0x1c00ULL,
    0x3800ULL,        0x7000ULL,        0xe000ULL,        0xc000ULL,
    0x30300ULL,       0x70700ULL,       0xe0e00ULL,       0x1c1c00ULL,
    0x383800ULL,      0x707000ULL,      0xe0e000ULL,      0xc0c000ULL,
    0x3030300ULL,     0x7070700ULL,     0xe0e0e00ULL,     0x1c1c1c00ULL,
    0x38383800ULL,    0x70707000ULL,    0xe0e0e000ULL,    0xc0c0c000ULL,
    0x303030300ULL,   0x707070700ULL,   0xe0e0e0e00ULL,   0x1c1c1c1c00ULL,
    0x3838383800ULL,  0x7070707000ULL,  0xe0e0e0e000ULL,  0xc0c0c0c000ULL,
    0x30303030300ULL, 0x70707070700ULL, 0xe0e0e0e0e00ULL, 0x1c1c1c1c1c00ULL,
    0x383838383800ULL,0x707070707000ULL,0xe0e0e0e0e000ULL,0xc0c0c0c0c000ULL,
    0,0,0,0,0,0,0,0
  }
};

const bitboard_t pawn_front[2][64] = 
{ { 0, 0, 0, 0, 0, 0, 0, 0,
    0x3030303030000ULL, 0x7070707070000ULL, 0xe0e0e0e0e0000ULL, 0x1c1c1c1c1c0000ULL, 0x38383838380000ULL, 0x70707070700000ULL, 0xe0e0e0e0e00000ULL, 0xc0c0c0c0c00000ULL,
    0x3030303000000ULL, 0x7070707000000ULL, 0xe0e0e0e000000ULL, 0x1c1c1c1c000000ULL, 0x38383838000000ULL, 0x70707070000000ULL, 0xe0e0e0e0000000ULL, 0xc0c0c0c0000000ULL,
    0x3030300000000ULL, 0x7070700000000ULL, 0xe0e0e00000000ULL, 0x1c1c1c00000000ULL, 0x38383800000000ULL, 0x70707000000000ULL, 0xe0e0e000000000ULL, 0xc0c0c000000000ULL,
    0x3030000000000ULL, 0x7070000000000ULL, 0xe0e0000000000ULL, 0x1c1c0000000000ULL, 0x38380000000000ULL, 0x70700000000000ULL, 0xe0e00000000000ULL, 0xc0c00000000000ULL,
    0x3000000000000ULL, 0x7000000000000ULL, 0xe000000000000ULL, 0x1c000000000000ULL, 0x38000000000000ULL, 0x70000000000000ULL, 0xe0000000000000ULL, 0xc0000000000000ULL,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
  },
  { 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0x300ULL, 0x700ULL, 0xe00ULL, 0x1c00ULL, 0x3800ULL, 0x7000ULL, 0xe000ULL, 0xc000ULL,
    0x30300ULL, 0x70700ULL, 0xe0e00ULL, 0x1c1c00ULL, 0x383800ULL, 0x707000ULL, 0xe0e000ULL, 0xc0c000ULL,
    0x3030300ULL, 0x7070700ULL, 0xe0e0e00ULL, 0x1c1c1c00ULL, 0x38383800ULL, 0x70707000ULL, 0xe0e0e000ULL, 0xc0c0c000ULL,
    0x303030300ULL, 0x707070700ULL, 0xe0e0e0e00ULL, 0x1c1c1c1c00ULL, 0x3838383800ULL, 0x7070707000ULL, 0xe0e0e0e000ULL, 0xc0c0c0c000ULL,
    0x30303030300ULL, 0x70707070700ULL, 0xe0e0e0e0e00ULL, 0x1c1c1c1c1c00ULL, 0x383838383800ULL, 0x707070707000ULL, 0xe0e0e0e0e000ULL, 0xc0c0c0c0c000ULL,
    0, 0, 0, 0, 0, 0, 0, 0
  }
};

//king safety related:
const bitboard_t king_storm_mask[2] = {0xe0e0e000ULL, 0xe0e0e000000000ULL};
const bitboard_t queen_storm_mask[2] = {0x7070700ULL, 0x7070700000000ULL};

//bit index conversion:
const int rsz[128] = /*real_square_zip*/
{  0,  1,  2,  3,  4,  5,  6,  7, 0,0,0,0,0,0,0,0,
   8,  9, 10, 11, 12, 13, 14, 15, 0,0,0,0,0,0,0,0,
  16, 17, 18, 19, 20, 21, 22, 23, 0,0,0,0,0,0,0,0,
  24, 25, 26, 27, 28, 29, 30, 31, 0,0,0,0,0,0,0,0,
  32, 33, 34, 35, 36, 37, 38, 39, 0,0,0,0,0,0,0,0,
  40, 41, 42, 43, 44, 45, 46, 47, 0,0,0,0,0,0,0,0,
  48, 49, 50, 51, 52, 53, 54, 55, 0,0,0,0,0,0,0,0,
  56, 57, 58, 59, 60, 61, 62, 63, 0,0,0,0,0,0,0,0
};

const int rsu[64] =  /*real_square_unzip*/
{ A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2 ,D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8
};
