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
#include <cstdint>		// for portable fixed size int types
#include <cmath>		// for std::abs()
#include <cassert>
#include <iostream>

// Square, Rank, File

#define NB_SQUARE 64
enum {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	NO_SQUARE
};

enum { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, NB_RANK };
enum { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, NB_FILE };

extern bool rank_ok(int r);
extern bool file_ok(int f);
extern bool square_ok(int sq);
extern int rank(int sq);
extern int file(int sq);
extern int square(int r, int f);
extern int rank_mirror(int sq);
extern int file_mirror(int sq);

// Piece

#define NB_PIECE 6
enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE };

extern bool piece_ok(int piece);
extern bool is_slider(int piece);

// Color

#define NB_COLOR 2
enum { WHITE, BLACK, NO_COLOR };

extern bool color_ok(int color);
extern int opp_color(int color);
extern int color_of(int sq);

// game phase
enum { OPENING, ENDGAME, NB_PHASE };

// material values
enum {
	vOP = 78, vEP = 100,
	vN = 320, vB = 320,
	vR = 523, vQ = 990,
	vK = 20000 // only for SEE
};

// Eval

struct Eval {
	int op, eg;

	bool operator== (const Eval& e) { return op == e.op && eg == e.eg; }
	bool operator!= (const Eval& e) { return !(*this == e); }
	const Eval& operator+= (const Eval& e) { op += e.op; eg += e.eg; return *this; }
	const Eval& operator-= (const Eval& e) { op -= e.op; eg -= e.eg; return *this; }
};

typedef uint64_t Key, Bitboard;

const int MATE = 16000;
const int INF = 32767;

const int MAX_DEPTH = 127;	// search depth go from MAX_DEPTH down to 1
const int MIN_DEPTH = -8;	// qsearch depth go from 0 downto -MIN_DEPTH-1
const int MAX_PLY = MAX_DEPTH - MIN_DEPTH + 1;	// plies go from 0 to MAX_PLY

extern uint64_t dbg_cnt1, dbg_cnt2;

