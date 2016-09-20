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
#include "types.h"

bool rank_ok(int r)			{ return RANK_1 <= r && r <= RANK_8; }
bool file_ok(int f)			{ return FILE_A <= f && f <= FILE_H; }
bool square_ok(int sq)		{ return A1 <= sq && sq <= H8; }

int rank(int sq)			{ assert(square_ok(sq)); return sq / NB_FILE; }
int file(int sq)			{ assert(square_ok(sq)); return sq % NB_FILE; }
int square(int r, int f)	{ assert(rank_ok(r) && file_ok(f)); return NB_FILE * r + f; }

int rank_mirror(int sq)		{ assert(square_ok(sq)); return sq ^ 070; }
int file_mirror(int sq)		{ assert(square_ok(sq)); return sq ^ 7; }

bool piece_ok(int piece)	{ return PAWN <= piece && piece < NO_PIECE; }
bool is_slider(int piece)	{ assert(piece_ok(piece)); return BISHOP <= piece && piece <= QUEEN; }

bool color_ok(int color)	{ return color == WHITE || color == BLACK; }
int opp_color(int color)	{ assert(color_ok(color)); return color ^ 1; }
int color_of(int sq)		{ assert(square_ok(sq)); return (sq + rank(sq)) & 1; }

