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

//piece value table
const int pval[16] = 
{ 0, P_VALUE, N_VALUE, B_VALUE, R_VALUE, Q_VALUE, K_VALUE, 0,
  0, P_VALUE, N_VALUE, B_VALUE, R_VALUE, Q_VALUE, K_VALUE, 0
};
const int slider[15] = {0,0,0,1,2,3,0,0,0,0,0,1,2,3,0};

//pawn advance:
const int pawn_delta[2][3]   = {{15,17,0},{-15,-17,0}};
const int pawn_push[2][3]    = {{16,32,0},{-16,-32,0}};
const int pawn_prom_rank[2]  = {RANK_7, RANK_2};
const int pawn_prom_rank_[2] = {RANK_8, RANK_1}; //evasions hack
const int pawn_dpush_rank[2] = {RANK_2, RANK_7};

int distance_table[128][128];
int direction[128][128]; 

//vector of attacked squares:
const int dir_vect[16][9] = 
{ {0},
  {15,17,0}, //white pawn's attacked squares
  {14,31,33,18,-14,-31,-33,-18,0}, //knight
  {17,-15,-17,15,0}, //bishop
  {16,-16,-1,1,0}, //rook
  {17,-15,-17,15,16,-16,-1,1,0}, //queen
  {1,-1,16,-16,15,-15,17,-17,0}, //king
  {0},{0},
  {-15,-17,0}, //black pawn's attacked squares
  {14,31,33,18,-14,-31,-33,-18,0},
  {17,-15,-17,15,0},
  {16,-16,-1,1,0},
  {17,-15,-17,15,16,-16,-1,1,0},
  {1,-1,16,-16,15,-15,17,-17,0},
  {0}
};

//attack interaction table (betw.[x]&[y])
attack_t attack_vector[128][128];

//this array have two purposes. 
//1. attack vector gets filled with diagonal, orthogonal and knight attack flags 
//(no queen since there is no square that can hold both R and B attack flags agains other square).
//2. a piece can be tested trough this array whether it attacks a particullar square,
//depending on it's type. Here comes the queen mix (like in SEE).
const int atk_masks[16] =
{ 0, 0, N_ATK, B_ATK, R_ATK, (B_ATK|R_ATK), 0, 0,
  0, 0, N_ATK, B_ATK, R_ATK, (B_ATK|R_ATK), 0, 0
};

//globals:
position_t board_;
searchinfo_t si_;
option_t opt_;

//global pointers:
ppos_t board = &board_;
sq_t *squares = &board_.square[0]; // all squares
sq_t *square = &board_.square[0x30]; //board startup square
list_t *piece = &board_.piece[0];
psi_t si = &si_;
hist_t *hs = &board_.hs[0];
option_t *opt = &opt_;
ptt tt = 0;

