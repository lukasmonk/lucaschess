/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rodent.h"

int castle_mask[64];
const int bit_table[64] = {
   0,  1,  2,  7,  3, 13,  8, 19,
   4, 25, 14, 28,  9, 34, 20, 40,
   5, 17, 26, 38, 15, 46, 29, 48,
  10, 31, 35, 54, 21, 50, 41, 57,
  63,  6, 12, 18, 24, 27, 33, 39,
  16, 37, 45, 47, 30, 53, 49, 56,
  62, 11, 23, 32, 36, 44, 52, 55,
  61, 22, 43, 51, 60, 42, 59, 58
};
//                         P    N    B    R    Q
const int tp_value[7] = { 100, 325, 325, 500, 1000, 0, 0 }; // immutable, used in Swap()
int history[12][64];
int refutation[64][64];
int killer[MAX_PLY][2];
U64 zob_piece[12][64];
U64 zob_castle[16];
U64 zob_ep[8];

int pondering;
int root_depth;
int fl_elo_slider;
int time_percentage;
int use_book;
int book_filter;
U64 nodes;
int abort_search;

ENTRY *tt;
int tt_size;
int tt_mask;
int tt_date;

int weights[N_OF_FACTORS];
int dyn_weights[5];
int curr_weights[2][2];
int hist_limit;
int hist_perc;
int panel_style;
int verbose;
int fl_reading_personality;
int fl_separate_books;