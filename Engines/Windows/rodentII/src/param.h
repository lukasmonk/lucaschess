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

#pragma once

typedef class {
public:
  int elo;
  int fl_weakening;
  int pc_value[7];            // these values might be changed via UCI options
  int keep_pc[7];
  int mg_pst[2][6][64];       // midgame piece-square tables (initialized depending on pst_style, pst_perc and mat_perc)
  int eg_pst[2][6][64];       // endgame piece-square tables (initialized depending on pst_style, pst_perc and mat_perc)
  int sp_pst_data[2][6][64];  // special piece/square tables (outposts etc.)
  int danger[512];            // table for evaluating king safety
  int dist[64][64];           // table for evaluating king tropism
  int chebyshev_dist[64][64]; // table for unstoppable passer detection
  int phalanx[2][64];
  int defended[2][64];
  int pst_style;
  int mob_style;
  int mat_perc;
  int pst_perc;
  int shield_perc;
  int storm_perc;
  int bish_pair;
  int knight_pair;
  int exchange_imbalance;
  int np_bonus;
  int rp_malus;
  int rook_pair_malus;
  int doubled_malus_mg;
  int doubled_malus_eg;
  int isolated_malus_mg;
  int isolated_malus_eg;
  int isolated_open_malus;
  int backward_malus_base;
  int backward_malus_mg[8];
  int backward_malus_eg;
  int backward_open_malus;
  int minorBehindPawn;
  int minorVsQueen;
  int bishConfined;
  int rookOn7thMg;
  int rookOn7thEg;
  int twoRooksOn7thMg;
  int twoRooksOn7thEg;
  int rookOnQueen;
  int rookOnOpenMg;
  int rookOnOpenEg;
  int rookOnBadHalfOpenMg;
  int rookOnBadHalfOpenEg;
  int rookOnGoodHalfOpenMg;
  int rookOnGoodHalfOpenEg;
  int queenOn7thMg;
  int queenOn7thEg;
  int imbalance[9][9];
  int np_table[9];
  int rp_table[9];
  int draw_score;
  int forwardness;
  int book_filter;
  int eval_blur;
  int n_mob_mg[9];
  int n_mob_eg[9];
  int b_mob_mg[14];
  int b_mob_eg[14];
  int r_mob_mg[15];
  int r_mob_eg[15];
  int q_mob_mg[28];
  int q_mob_eg[28];
  void DynamicInit(void);
  void Default(void);
} cParam; 

extern cParam Param;
