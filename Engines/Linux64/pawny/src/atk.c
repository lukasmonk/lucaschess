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

bool is_sq_attacked_w(int sq)
{//is 'sq' attacked by any of the white pieces
  bitboard_t occ,atk;
	
  //pawn attacks:
  if(p_attacks[B][sq] & pos->occ[WP]) 
    return true;
  
  //diagonal attacks:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  atk = bmoves(sq, occ);
  if(atk & (pos->occ[WQ] | pos->occ[WB])) 
    return true;
	
  //orthogonal attacks:
  atk = rmoves(sq, occ);
  if(atk & (pos->occ[WQ] | pos->occ[WR]))
    return true;
		
  //knight attacks:
  if(n_moves[sq] & pos->occ[WN]) 
    return true;
	
  //king attacks:
  if(k_moves[sq] & pos->occ[WK]) 
    return true;
	
  return false;
}

bool is_sq_attacked_b(int sq)
{//is 'sq' attacked by any of the black pieces
  bitboard_t occ,atk;
	
  //pawn attacks:
  if(p_attacks[W][sq] & pos->occ[BP]) 
    return true;
	
  //diagonal attacks:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  atk = bmoves(sq, occ);
  if(atk & (pos->occ[BQ] | pos->occ[BB]))
    return true;
	
  //orthogonal attacks:
  atk = rmoves(sq, occ);
  if(atk & (pos->occ[BQ] | pos->occ[BR]))
    return true;
	
  //knight attacks:
  if(n_moves[sq] & pos->occ[BN]) 
    return true;
	
  //king attacks:
  if(k_moves[sq] & pos->occ[BK]) 
    return true;
	
  return false;
}

bool is_in_check_w()
{//is the white king in check
	
  bitboard_t occ,atk;
  int ksq = Ksq(W);
	
  //pawn attacks:
  if(p_attacks[W][ksq] & pos->occ[BP]) 
    return true;
	
  //diagonal attacks:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  atk = bmoves(ksq, occ);
  if(atk & (pos->occ[BQ] | pos->occ[BB])) 
    return true;
	
  //orthogonal attacks:
  atk = rmoves(ksq, occ);
  if(atk & (pos->occ[BQ] | pos->occ[BR]))
    return true;
	
  //knight attacks:
  if(n_moves[ksq] & pos->occ[BN]) 
    return true;
  
  //king attacks:
  if(k_moves[ksq] & pos->occ[BK]) 
    return true;
	
  return false;
}

bool is_in_check_b()
{//is the black king in check
	
  bitboard_t occ,atk;
  int ksq = Ksq(B);
		
  //pawn attacks:
  if(p_attacks[B][ksq] & pos->occ[WP]) 
    return true;
	
  //diagonal attacks:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  atk = bmoves(ksq, occ);
  if(atk & (pos->occ[WQ] | pos->occ[WB]))
    return true;
	
  //orthogonal attacks:
  atk = rmoves(ksq, occ);
  if(atk & (pos->occ[WQ]|pos->occ[WR]))
    return true;
	
  //knight attacks:
  if(n_moves[ksq] & pos->occ[WN]) 
    return true;
  
  //king attacks:
  if(k_moves[ksq] & pos->occ[WK]) 
    return true;
	
  return false;
}

bool is_in_check()
{
  if(pos->side == W) return is_in_check_w();
  return is_in_check_b();
}

bool is_pinned_w(int from, int to)
{//checking if the piece 'from' is pinned for the white king
 //this includes a capture that is illegal due to pinned piece
  int ksq = Ksq(W);
  bitboard_t atk, occ;
	
  occ = ((pos->occ[OCC_W] | pos->occ[OCC_B]) & notmask(from)) | (1ULL << to);

  atk = rmoves(ksq, occ);
  if(atk & ((pos->occ[BQ] | pos->occ[BR])) &  notmask(to))
    return true;

  atk = bmoves(ksq, occ);
  if(atk & ((pos->occ[BQ] | pos->occ[BB])) & notmask(to))
    return true;
	
  return false;
}

bool is_pinned_b(int from, int to)
{
  int ksq = Ksq(B);
  bitboard_t atk, occ;

  occ = ((pos->occ[OCC_W] | pos->occ[OCC_B]) & notmask(from)) | (1ULL << to);

  atk = rmoves(ksq, occ);
  if(atk & ((pos->occ[WQ] | pos->occ[WR])) & notmask(to))
    return true;

  atk = bmoves(ksq, occ);
  if(atk & ((pos->occ[WQ] | pos->occ[WB])) & notmask(to))
    return true;
  
  return false;
}

bool is_pinned_ep_w(int from, int to)
{//White pawn e.p. move:
 //the only difference here is that instead of eventual
 //piece capture being excluded, 
 //the pawn taken by e.p. is being exluded from the logical op test.
  int ksq = Ksq(W);
  bitboard_t atk, occ;
  
  occ = ((pos->occ[OCC_W] | pos->occ[OCC_B]) & notmask(from) & notmask(to-8)) | (1ULL << to);
	
  atk = rmoves(ksq, occ);
  if(atk & ((pos->occ[BQ] | pos->occ[BR])))
    return true;

  atk = bmoves(ksq, occ);
  if(atk & ((pos->occ[BQ] | pos->occ[BB])))
    return true;

  return false;
}

bool is_pinned_ep_b(int from, int to)
{//black pawn e.p. move
  int ksq = Ksq(B);
  bitboard_t atk, occ;

  occ = ((pos->occ[OCC_W] | pos->occ[OCC_B]) & (notmask(from) & notmask(to+8))) | (1ULL << to);
  
  atk = rmoves(ksq, occ);
  if(atk & ((pos->occ[WQ] | pos->occ[WR])))
    return true;
  
  atk = bmoves(ksq, occ);
  if(atk & ((pos->occ[WQ] | pos->occ[WB])))
    return true;
  
  return false;
}


int checkers_w(bitboard_t *attackers, bitboard_t *front, bitboard_t *across)
{/*Returns the count of white pieces, that attack the black king.
  'attackers' receives the squares of the attacking pieces.
  'front' receives the attack rays between the king and the attackers
  'across' - the attack rays from the attackers and beyound the king.
  'across' is used in king evasions to determine if the square to move on is attacked 
   without actually moving the king.
  'front' is used in full evasions to determine if piece is able to cover the king (if single check).
   Basically, these are two partial attack maps (only formed by the attackers against the king),
   that are used in the evasions generation.*/
  bitboard_t occ, moves, temp;
  int x = 0, ksq = Ksq(B);
	
  *front = 0; *across = 0;
  *attackers  = (p_attacks[B][ksq] & pos->occ[WP]) | (n_moves[ksq] & pos->occ[WN]);
  
  occ = (pos->occ[OCC_W] | pos->occ[OCC_B]);
  moves = bmoves(ksq, occ);
  temp = moves & (pos->occ[WB] | pos->occ[WQ]);
  if(temp)
  { *attackers |= temp;
    while(temp)
    { x = bitscanf(temp);
      bitclear(temp, x);
      *across |= bmoves(x, occ & ~pos->occ[BK]);// & moves - not needed for across - using extended ray.;
    }
    *front = (bmoves(x, occ)) & moves;
  }
  moves = rmoves(ksq, occ);
  temp = moves & (pos->occ[WR] | pos->occ[WQ]);
  if(temp)
  { *attackers |= temp;
    while(temp)
    { x = bitscanf(temp);
      bitclear(temp, x);
      *across |= rmoves(x, occ & ~pos->occ[BK]);
    }
    *front = (rmoves(x, occ)) & moves;
  }
  return popcnt(*attackers);
}

int checkers_b(bitboard_t *attackers, bitboard_t *front, bitboard_t *across)
{
  bitboard_t occ, moves, temp;
  int x = 0, ksq = Ksq(W);
	
  *front = 0; *across = 0;
  *attackers  = (p_attacks[W][ksq] & pos->occ[BP]) | (n_moves[ksq] & pos->occ[BN]);
	
  occ = (pos->occ[OCC_W] | pos->occ[OCC_B]);
  moves = bmoves(ksq, occ);
  temp = moves & (pos->occ[BB] | pos->occ[BQ]);
  if(temp)
  { *attackers |= temp;
    while(temp)
    { x = bitscanf(temp);
      bitclear(temp, x);
      *across |= bmoves(x, occ & ~pos->occ[WK]);
    }
    *front = (bmoves(x, occ)) & moves;
  }
  moves = rmoves(ksq, occ);
  temp = moves & (pos->occ[BR] | pos->occ[BQ]);
  if(temp)
  { *attackers |= temp;
    while(temp)
    { x = bitscanf(temp);
      bitclear(temp, x);
      *across |= rmoves(x, occ & ~pos->occ[WK]);
    }
    *front = (rmoves(x, occ)) & moves;
  }
  return popcnt(*attackers);
}
