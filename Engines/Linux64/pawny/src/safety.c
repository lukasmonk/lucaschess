/*--------------------------------------------------------------------------
    Pawny 1.2, chess engine (source code).
    Copyright (C) 2009 - 2016 by Mincho Georgiev.
    
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
#include "inline.h"

int discovery_checks[8][8][2];
static const int shield_defect_value = 12;
static const int attack_value[8] = {0, 0, 3, 3, 6, 9, 0, 0};
static const int castling_penalty = 25;
static const bitboard_t king_files[8] = 
{ 0x303030303030303ULL,   0x707070707070707ULL,  0xe0e0e0e0e0e0e0eULL, 0x1c1c1c1c1c1c1c1cULL,
  0x3838383838383838ULL, 0x7070707070707070ULL, 0xe0e0e0e0e0e0e0e0ULL, 0xc0c0c0c0c0c0c0c0ULL
};

#define SHIELD 9
static int shield_penalty(int ksq, int color)
{ //returns count of shield defects:
  bitboard_t pawns;
  int v = 0;
  int rank = Rank(ksq);
 
  if(color == W)
  { pawns = pos->occ[WP] & king_files[File(ksq)];
    if(rank <= RANK_4)
    { v += (popcnt(pawns & rank_mask[rank + 1]) * 3);
      v += (popcnt(pawns & rank_mask[rank + 2]) * 2);
      v += (popcnt(pawns & rank_mask[rank + 3]));
    }
  }
  else
  { pawns = pos->occ[BP] & king_files[File(ksq)];
    if(rank >= RANK_5) 
    { v += (popcnt(pawns & rank_mask[rank - 1]) * 3);
      v += (popcnt(pawns & rank_mask[rank - 2]) * 2);
      v += (popcnt(pawns & rank_mask[rank - 3]));
    }
  }
  if(v > SHIELD) v = SHIELD;
  return (SHIELD - v);
}

static int castling_rights(int phase, int ksq, int color)
{//Returns penalty if the king of 'color'
 //have lost his castling abillity.
  if((pos->occ[WQ] | pos->occ[BQ]) && phase > 15)
  { if(color == WHITE)
    { if(!(pos->castle & (WHITE_OO|WHITE_OOO)) && ksq != C1 && ksq != G1)
        return (castling_penalty);
    }
    else
    { if(!(pos->castle & (BLACK_OO|BLACK_OOO)) && ksq != C8 && ksq != G8)
        return castling_penalty;
    }
  }
  return 0;
}

static int eval_discovery_checks(int ksq, int xside)
{ 
  bitboard_t pcs, pc_betw, dir;
  bitboard_t occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  int sq, penalty = 0;

  //diagonals:
  pcs = bmoves(ksq, 1ULL << ksq) & (pos->occ[Coloured(QUEEN, xside)] | pos->occ[Coloured(BISHOP, xside)]);
  while(pcs)
  { sq = bitscanf(pcs);
    bitclear(pcs, sq);
    dir = direction[ksq][sq] & occ;
    if(popcnt(dir) == 1)
    { pc_betw = Piece(bitscanf(dir));
      if(GetColor(pc_betw) == xside)
      {	if(!(atk_ability[GetType(pc_betw)] & atk_flags[ksq][sq]))
          penalty += discovery_checks[GetType(Piece(sq))][GetType(pc_betw)][pos->side == xside];
      }
    }
  }
  //orthogonals:	
  pcs = rmoves(ksq, 1ULL << ksq) & (pos->occ[Coloured(QUEEN, xside)] | pos->occ[Coloured(ROOK, xside)]);
  while(pcs)
  { sq = bitscanf(pcs);
    bitclear(pcs, sq);
    dir = direction[ksq][sq] & occ;
    if(popcnt(dir) == 1)
    { pc_betw = Piece(bitscanf(dir));
      if(GetColor(pc_betw) == xside)
      { pc_betw = GetType(pc_betw);
        if((pc_betw == PAWN && Rank(sq) == Rank(ksq))
        || (pc_betw != PAWN && !(atk_ability[pc_betw] & atk_flags[ksq][sq])))
          penalty += discovery_checks[GetType(Piece(sq))][pc_betw][pos->side == xside];
      }
    }
  }
  return penalty;
}

static int get_attackers(int sq, int color, int *atk_value)
{//Returns the attackers count of 'color' 
 //and stores weight per square.
  int x, sq2, count;
  bitboard_t occ, p, t, exclude;
  
  //pawns
  count = popcnt(p_attacks[color ^ 1][sq] & pos->occ[Coloured(PAWN, color)]);
    
  //knights:
  x = popcnt(n_moves[sq] & pos->occ[Coloured(KNIGHT, color)]);
  if(x)
  { count += x;
    *atk_value = x * attack_value[KNIGHT];
  }
    
  //orthogonal attackers:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  exclude = 0ULL;
  x = 0;
  p = (rmoves(sq, occ) & ((pos->occ[Coloured(QUEEN, color)]) | (pos->occ[Coloured(ROOK, color)])));
  while(p)
  { count += popcnt(p);
    t = p;
    while(x == 0 && t)
    { sq2 = bitscanf(t);
      bitclear(t, sq2);
      //attackers from 2nd+ line are just getting counted
      //the weight remains the one from the 1st line attacker
      //example: queen, powered by bishop on 2nd line is not the same as bishop, powered by queen from behind:
      *atk_value += attack_value[GetType(pos->square[sq2])];
    }
    //exclude the processed and continue:
    exclude |= p;
    occ &= ~exclude;
    p = (rmoves(sq, occ) & ((pos->occ[Coloured(QUEEN, color)]) | (pos->occ[Coloured(ROOK, color)])) & ~exclude);
    x++; //attack line count increment.
  }
    
  //diagonal attackers:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  exclude = 0ULL;
  x = 0;
  p = (bmoves(sq, occ) & ((pos->occ[Coloured(QUEEN, color)]) | (pos->occ[Coloured(BISHOP, color)])));
  while(p)
  { count += popcnt(p);
    t = p;
    while(x == 0 && t)
    { sq2 = bitscanf(t);
      bitclear(t, sq2);
      *atk_value += attack_value[GetType(pos->square[sq2])];
    }
    exclude |= p;
    occ &= ~exclude;
    p = (bmoves(sq, occ) & ((pos->occ[Coloured(QUEEN, color)]) | pos->occ[Coloured(BISHOP, color)]) & ~exclude);
    x++;
  }
  return count;
}

int evaluate_king_safety(int phase, int ksq, int side, bitboard_t atk_map[])
{//Returns penalty for 'side'
  bitboard_t undefended;
  int sq, attackers, atk_value;
  int xside = side ^ 1;
  int sum = 0, total = 0;
  int shield;
  int penalty;

  undefended = (k_moves[ksq] & atk_map[xside]) & ~atk_map[side];
  while(undefended)
  { sq = bitscanf(undefended);
    bitclear(undefended, sq);
    atk_value = 0;
    attackers = get_attackers(sq, xside, &atk_value);
    sum += (attackers * atk_value);
    total += attackers;
  }
  if((1ULL << ksq) & atk_map[xside])
  { atk_value = 0;
    attackers = get_attackers(ksq, xside, &atk_value);
    sum += (attackers * atk_value);
    total += attackers;
  }
  shield = shield_penalty(ksq, side);
  penalty  = shield * shield_defect_value;
  penalty += castling_rights(phase, ksq, side);
  penalty += eval_discovery_checks(ksq, xside);
  if(sum && total > 1) penalty += (sum * (1 + shield));
  penalty = min(WIN, penalty);
  return(penalty);
}

void init_safety()
{
  memset(discovery_checks, 0, sizeof(discovery_checks));
  
  //Static discovery checks penalties in the form of:
  //[2ND Line Attacker][Piece Between][stm == attacking_side (true/false)]:
  discovery_checks[BISHOP][PAWN][0]   = discovery_checks[ROOK][PAWN][0]   = 8;
  discovery_checks[BISHOP][PAWN][1]   = discovery_checks[ROOK][PAWN][1]   = 12;
  discovery_checks[BISHOP][KNIGHT][0] = discovery_checks[ROOK][KNIGHT][0] = 12;
  discovery_checks[BISHOP][KNIGHT][1] = discovery_checks[ROOK][KNIGHT][1] = 15;
  discovery_checks[BISHOP][ROOK][0]   = discovery_checks[ROOK][BISHOP][0] = 15;
  discovery_checks[BISHOP][ROOK][1]   = discovery_checks[ROOK][BISHOP][1] = 20;
  
  discovery_checks[QUEEN][PAWN][0] = 12;
  discovery_checks[QUEEN][PAWN][1] = 15;
  discovery_checks[QUEEN][KNIGHT][0] = 18;
  discovery_checks[QUEEN][KNIGHT][1] = 25;
  discovery_checks[QUEEN][ROOK][0] = 18;
  discovery_checks[QUEEN][ROOK][1] = 25;
}
