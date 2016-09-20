/*
 * DiscoCheck, an UCI chess engine. Copyright (C) 2011-2013 Lucas Braesch.
 *
 * DiscoCheck is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * DiscoCheck is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include "types.h"

namespace bb {

const Bitboard FileA_bb = 0x0101010101010101ULL;
const Bitboard FileH_bb	= 0x8080808080808080ULL;
const Bitboard Rank1_bb	= 0x00000000000000FFULL;

extern Bitboard second_rank(int c);
extern Bitboard eighth_rank(int c);
extern Bitboard half_board(int c);

const Bitboard WhiteSquares = 0x55AA55AA55AA55AAULL;
const Bitboard BlackSquares = 0xAA55AA55AA55AA55ULL;

// Initialize: bitboards, magics
extern void init();
extern bool BitboardInitialized;

extern Key zob(int c, int p, int sq);
extern Key zob_ep(int sq);
extern Key zob_castle(int crights);
extern Key zob_turn();

extern Bitboard between(int s1, int s2);	// excludes s1 and includes s2
extern Bitboard direction(int s1, int s2);	// so through s2 to the edge of the board

extern Bitboard in_front(int c, int r);
extern Bitboard adjacent_files(int f);
extern Bitboard squares_in_front(int c, int sq);
extern Bitboard pawn_span(int c, int sq);
extern Bitboard shield(int c, int sq);

// Occupancy independant attacks
extern Bitboard kattacks(int sq);
extern Bitboard nattacks(int sq);
extern Bitboard battacks(int sq);
extern Bitboard rattacks(int sq);
extern Bitboard pattacks(int c, int sq);

// Squares attacked by a bishop/rook for a given board occupancy
extern Bitboard battacks(int sq, Bitboard occ);
extern Bitboard rattacks(int sq, Bitboard occ);

// squares attacked by piece on sq, for a given occupancy
extern Bitboard piece_attack(int piece, int sq, Bitboard occ);

// print bitboard (ASCII style)
extern void print(std::ostream& ostrm, Bitboard b);

extern int kdist(int s1, int s2);

extern void set_bit(Bitboard *b, unsigned sq);
extern void clear_bit(Bitboard *b, unsigned sq);
extern bool test_bit(Bitboard b, unsigned sq);
extern Bitboard shift_bit(Bitboard b, int i);
extern bool several_bits(Bitboard b);

extern int pawn_push(int color, int sq);
extern Bitboard rank_bb(int r);
extern Bitboard file_bb(int f);

extern int count_bit(Bitboard b);
extern int lsb(Bitboard b);
extern int msb(Bitboard b);
extern int pop_lsb(Bitboard *b);

}	// namespace bb
