/*--------------------------------------------------------------------------
    Pawny 1.0, chess engine (source code).
    Copyright (C) 2009 - 2013 by Mincho Georgiev.
    
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


static bitboard_t see_attackers(int sq, bitboard_t occ, bitboard_t exclude, int side)
{ //pawns
  bitboard_t atk = p_attacks[side^1][sq] & pos->occ[Coloured(PAWN,side)];
  //diagonal attackers:
  atk |= (bmoves(sq, occ) & (pos->occ[Coloured(QUEEN,side)] | pos->occ[Coloured(BISHOP,side)]));
  //orthogonal attackers:
  atk |= (rmoves(sq, occ) & (pos->occ[Coloured(QUEEN,side)] | pos->occ[Coloured(ROOK,side)]));
  //knights:
  atk |= (n_moves[sq] & pos->occ[Coloured(KNIGHT,side)]);
  //king:
  atk |= (k_moves[sq] & pos->occ[Coloured(KING,side)]);
  //remove already processed attackers from the list:
  atk &= ~exclude;
  return atk;
}

static bool see_pinned(int side, bitboard_t occ)
{//determines whether the virtually erased piece 
 //was pinned for the king of 'side'.
  bitboard_t moves;
  int ksq = Ksq(side);
  int opside = side ^ 1;
  
  moves = rmoves(ksq, occ);
  if(moves & ((pos->occ[Coloured(QUEEN, opside)] | pos->occ[Coloured(ROOK, opside)])))
    return true;
  moves = bmoves(ksq, occ);
  if(moves & ((pos->occ[Coloured(QUEEN, opside)] | pos->occ[Coloured(BISHOP, opside)])))
    return true;
  return false;
}

int see(move_t m)
{
  int i,x,n,victim,side;
  bitboard_t occ,exclude,atk[2];
  int smallest[2];
  int score[32];
  
  //see() handles only captures:
  if(!(m.type & CAP) || (m.promoted) || (m.type & EP)) 
    return m.score;
  
  //clearing:
  n = 0; side = pos->side;
  for(i = 0, x = 16; i < 16; i++, x++) 
    score[i] = score[x] = 0;

  //initial capture result:
  score[n++] = pval[pos->square[m.to]];
  //saving the capturing piece:
  victim = pval[pos->square[m.from]];
   
  ///pass 1: initial capture.
  //getting attackers without the initial one,
  //occupation, excluding the initial attacker:
  occ = (pos->occ[OCC_W] | pos->occ[OCC_B]) & notmask(m.from);
  exclude = (1ULL << m.from);
  
  //stm attackers without the initial capturing piece:
  atk[side] = see_attackers(m.to, occ, exclude, side);
  
  //collect op-side's attackers without the initial piece:
  atk[side^1] = see_attackers(m.to, occ, exclude, side^1);
  
  //is it a discovery check?
  if(see_pinned(side^1, occ))
  { //is the op-king attacks and the square is undefended:
    if(atk[side^1] & pos->occ[Coloured(KING, side^1)])
    { //square is defended:
      if(atk[side] != 0) return score[0];
      //square is undefended:
      return score[0] - victim;
    }
    //the op-king can't capture:
    return score[0];
  }
  
  //find smallest atk
  for(x = 0, i = Coloured(PAWN,side); i <= Coloured(KING,side); x = 0, i++)
  { if((pos->occ[i] & atk[side]))
    { x = bitscanf(pos->occ[i] & atk[side]);
      if(i != Coloured(KING,side))
      { //is this piece pinned for the own king:
        if(see_pinned(side, (occ & notmask(x)))) continue;
        else break;
      }
      else break;
    }
  }
  if(x) smallest[side] = pos->square[x];
  else atk[side] = 0;
   
  //switching sides:
  side ^= 1;
	
  ///pass2 - recapturing:
  while(atk[side])
  { 
    //find smallest atk
    for(x = 0, i = Coloured(PAWN,side); i <= Coloured(KING,side); x = 0, i++)
    { if((pos->occ[i] & atk[side]))
      { x = bitscanf(pos->occ[i] & atk[side]);
        if(i != Coloured(KING,side))
        { //is this piece pinned for the own king:
          if(see_pinned(side, (occ & notmask(x)))) continue;
          else break;
        }
        else break;
      }
    }
    if(x == 0) break;
    smallest[side] = pos->square[x];
    
    //if the king is the only one left:
    if(smallest[side] == Coloured(KING, side))
    { //but the square is still protected:
      if(atk[side^1] != 0) break;
      //if not, this is the last capture:
      score[n++] = -score[n-1] + victim;
      break;
    }
    
    //update without current smallest:
    bitclear(occ, x);
    
    //discovery?
    if(see_pinned(side^1, occ))
    { //is the op-king attacks and the square is undefended:
      if(atk[side^1] & pos->occ[Coloured(KING, side^1)])
      { if((atk[side] & notmask(x)) != 0) break;
        //updating both sides scores:
        score[n++] = -score[n-1] + victim;
        score[n++] = -score[n-1] + pval[smallest[side]];
        break;
      }//otherwise the king could not move onto the capture square, so:
      score[n++] = -score[n-1] + victim;
      break;
    }
    
    //update score for the current stm:
    score[n++] = -score[n-1] + victim;
    victim = pval[smallest[side]];
    
    //update exclusion and attacks:
    exclude |= (1ULL << x);
    atk[side] = see_attackers(m.to, occ, exclude, side);
	  
    //switching sides:
    side ^= 1;
  }
  while(--n) score[n-1] = min(-score[n], score[n-1]);
  return (score[0]);
}

int see_squares(int from, int to)
{
  int i,x,n,victim,side;
  bitboard_t occ,exclude,atk[2];
  int smallest[2];
  int score[32];
  
  //clearing:
  n = 0; side = GetColor(pos->square[from]);
  for(i = 0, x = 16; i < 16; i++, x++) 
    score[i] = score[x] = 0;

  //initial capture result:
  score[n++] = pval[pos->square[to]];
  //saving the capturing piece:
  victim = pval[pos->square[from]];
   
  ///pass 1: initial capture.
  //getting attackers without the initial one,
  //occupation, excluding the initial attacker:
  occ = (pos->occ[OCC_W] | pos->occ[OCC_B]) & notmask(from);
  exclude = (1ULL << from);
  
  //stm attackers without the initial capturing piece:
  atk[side] = see_attackers(to, occ, exclude, side);
  
  //collect op-side's attackers without the initial piece:
  atk[side^1] = see_attackers(to, occ, exclude, side^1);
  
  //is it a discovery check?
  if(see_pinned(side^1, occ))
  { //is the op-king attacks and the square is undefended:
    if(atk[side^1] & pos->occ[Coloured(KING, side^1)])
    { //square is defended:
      if(atk[side] != 0) return score[0];
      //square is undefended:
      return score[0] - victim;
    }
    //the op-king can't capture:
    return score[0];
  }
  
  //find smallest atk
  for(x = 0, i = Coloured(PAWN,side); i <= Coloured(KING,side); x = 0, i++)
  { if((pos->occ[i] & atk[side]))
    { x = bitscanf(pos->occ[i] & atk[side]);
      if(i != Coloured(KING,side))
      { //is this piece pinned for the own king:
        if(see_pinned(side, (occ & notmask(x)))) continue;
        else break;
      }
      else break;
    }
  }
  if(x) smallest[side] = pos->square[x];
  else atk[side] = 0;
   
  //switching sides:
  side ^= 1;
	
  ///pass2 - recapturing:
  while(atk[side])
  { 
    //find smallest atk
    for(x = 0, i = Coloured(PAWN,side); i <= Coloured(KING,side); x = 0, i++)
    { if((pos->occ[i] & atk[side]))
      { x = bitscanf(pos->occ[i] & atk[side]);
        if(i != Coloured(KING,side))
        { //is this piece pinned for the own king:
          if(see_pinned(side, (occ & notmask(x)))) continue;
          else break;
        }
        else break;
      }
    }
    if(x == 0) break;
    smallest[side] = pos->square[x];
    
    //if the king is the only one left:
    if(smallest[side] == Coloured(KING, side))
    { //but the square is still protected:
      if(atk[side^1] != 0) break;
      //if not, this is the last capture:
      score[n++] = -score[n-1] + victim;
      break;
    }
    
    //update without current smallest:
    bitclear(occ, x);
    
    //discovery?
    if(see_pinned(side^1, occ))
    { //is the op-king attacks and the square is undefended:
      if(atk[side^1] & pos->occ[Coloured(KING, side^1)])
      { if((atk[side] & notmask(x)) != 0) break;
        //updating both sides scores:
        score[n++] = -score[n-1] + victim;
        score[n++] = -score[n-1] + pval[smallest[side]];
        break;
      }//otherwise the king could not move onto the capture square, so:
      score[n++] = -score[n-1] + victim;
      break;
    }
    
    //update score for the current stm:
    score[n++] = -score[n-1] + victim;
    victim = pval[smallest[side]];
    
    //update exclusion and attacks:
    exclude |= (1ULL << x);
    atk[side] = see_attackers(to, occ, exclude, side);
	  
    //switching sides:
    side ^= 1;
  }
  while(--n) score[n-1] = min(-score[n], score[n-1]);
  return (score[0]);
}