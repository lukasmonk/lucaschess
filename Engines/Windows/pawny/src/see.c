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

#define SEE_P_VALUE 100
#define SEE_N_VALUE 321
#define SEE_B_VALUE 325
#define SEE_R_VALUE 500
#define SEE_Q_VALUE 950
#define SEE_K_VALUE 1000

static const int see_pval[16] = 
{ 0, SEE_P_VALUE, SEE_N_VALUE, SEE_B_VALUE, SEE_R_VALUE, SEE_Q_VALUE, SEE_K_VALUE, 0,
  0, SEE_P_VALUE, SEE_N_VALUE, SEE_B_VALUE, SEE_R_VALUE, SEE_Q_VALUE, SEE_K_VALUE, 0
};

int see(move_t m)
{	
  int i,v,sq,delta;
  int dest;	//delta leads to sq.
  int from;	//source square
  int to;		//target (dest) square
  int victim;	//captured piece value;
  int score[32];	//results of capturing
  int nscore;	//score count
  int side;	//side to "move"
  int attackers[2][16];//attackers squares
  int atk_count[2];//total count of attackers per side
  int smallest;	//the smallest attacker value
  int sm_index;	//the index of the smallest atk
  int sm_sq;	//the smallest attacker square
  attack_t *a;
  bool pinned;
  bool discovery;
  int ksq[2];
  bitboard_t played = 0; //already "played" attackers
  
  //see() handles only captures:
  if(!(m.type & CAP) || (m.promoted) || (m.type & EP)) return m.score;
  
  //init data:
  from = m.from; to = m.to;
  
  for(i = 0, v = 16; i < 16; i++,v++)
    attackers[0][i] = attackers[1][i] = \
    score[i] = score[v] = 0;
  
  atk_count[W] = atk_count[B] = 0;
  nscore = 0;
  side = board->side;//initial side to move:
  a = &attack_vector[to][0];
  
  //white pawns
  if(PieceType(to-15) == WP && from != (to-15))
  { attackers[W][atk_count[W]] = (to-15);
    atk_count[W]++;
  }
  if(PieceType(to-17) == WP && from != (to-17))
  { attackers[W][atk_count[W]] = (to-17);
    atk_count[W]++;
  }
  //black pawns:
  if(PieceType(to+15) == BP && from != (to+15))
  { attackers[B][atk_count[B]] = (to+15);
    atk_count[B]++;
  }
  if(PieceType(to + 17) == BP && from != (to+17))
  { attackers[B][atk_count[B]] = (to+17);
    atk_count[B]++;
  }

  //white queens:
  for(sq = PLS(WQ); sq <= H8; sq = PLN(sq))
  { if(sq != from && (a[sq].flags & R_ATK || a[sq].flags & B_ATK))
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest);dest += delta);
      if(dest == to)
      { attackers[W][atk_count[W]] = sq;
        atk_count[W]++;
      }
    }
  }
  //black queens:
  for(sq = PLS(BQ); sq <= H8; sq = PLN(sq))
  { if(sq != from && (a[sq].flags & R_ATK || a[sq].flags & B_ATK))
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest);dest += delta);
      if(dest == to)
      { attackers[B][atk_count[B]] = sq;
        atk_count[B]++;
      }
    }
  }
  
  //white rooks:
  for(sq = PLS(WR); sq <= H8; sq = PLN(sq))
  { if(sq != from && a[sq].flags & R_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest);dest += delta);
      if(dest == to)
      { attackers[W][atk_count[W]] = sq;
        atk_count[W]++;
      }
    }
  }
  //black rooks:
  for(sq = PLS(BR); sq <= H8; sq = PLN(sq))
  { if(sq != from && a[sq].flags & R_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest);dest += delta);
      if(dest == to)
      { attackers[B][atk_count[B]] = sq;
        atk_count[B]++;
      }
    }
  }
  
  //white bishops:
  for(sq = PLS(WB); sq <= H8; sq = PLN(sq))
  { if(sq != from && a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest);dest += delta);
      if(dest == to)
      { attackers[W][atk_count[W]] = sq;
        atk_count[W]++;
      }
    }
  }
  //black bishops:
  for(sq = PLS(BB); sq <= H8; sq = PLN(sq))
  { if(sq != from && a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest);dest += delta);
      if(dest == to)
      { attackers[B][atk_count[B]] = sq;
        atk_count[B]++;
      }
    }
  }
  
  //white knights:
  for(sq = PLS(WN); sq <= H8; sq = PLN(sq))
  { if(sq != from && a[sq].flags & N_ATK)
    { if(sq + a[sq].delta == to)
      { attackers[W][atk_count[W]] = sq;
        atk_count[W]++;
      }
    }
  }
  //black knights:
  for(sq = PLS(BN); sq <= H8; sq = PLN(sq))
  { if(sq != from && a[sq].flags & N_ATK)
    { if(sq + a[sq].delta == to)
      { attackers[B][atk_count[B]] = sq;
        atk_count[B]++;
      }
    }
  }
  
  //whether the king is trying to capture onto an attacked square:
  if((GetType(PieceType(m.from)) == KING) && atk_count[side^1])
    return -INF;
  
  //white king:
  ksq[W] = King_Square(W);
  if(ksq[W] != from && (a[ksq[W]].flags & R_ATK || a[ksq[W]].flags & B_ATK))
  { if(ksq[W] + a[ksq[W]].delta == to)
    { attackers[W][atk_count[W]] = ksq[W];
      atk_count[W]++;
    }
  }
  
  //black king:
  ksq[B] = King_Square(B);
  if(ksq[B] != from && (a[ksq[B]].flags & R_ATK || a[ksq[B]].flags & B_ATK))
  { if(ksq[B] + a[ksq[B]].delta == to)
    { attackers[B][atk_count[B]] = ksq[B];
      atk_count[B]++;
    }
  }
  
  //hidden attacker (slider) behind the initial one:
  if(direction[to][from])
  { i = direction[to][from];
    for(dest = from + i; IsEmpty(dest);dest += i);
    if(!IsOutside(dest))
    { if(a[dest].flags & atk_masks[PieceType(dest)])
      { attackers[side][atk_count[side]] = dest;
        atk_count[side]++;
      }
    }
  }
  
  //testing whether the initial piece is pinned for the king,
  //these tests (along with king attacking protected square) are necessary,
  //since pseudo-legal move generator is used.
  if(direction[ksq[side]][from])
  { i = direction[ksq[side]][from];
    for(dest = ksq[side] + i; IsEmpty(dest) ; dest += i);
    if(dest == from) //there is no piece between the king and this one!
    { pinned = false;
      for(dest = from + i; !IsOutside(dest); dest += i)
      { if(!IsEmpty(dest))
        { //we reach a friendly piece?:
          if(GetColor(PieceType(dest)) == (side)) break;
          else//enemy piece
          { if((dest != m.to) && (attack_vector[dest][ksq[side]].flags & atk_masks[PieceType(dest)]))
              pinned = true;
            break;
          }
        }
      }
      if(pinned == true) return -INF;
    }
  }
  
  //excluding the initial attacker:
  played = (1ULL << rsz[from]);
  
  discovery = false;
  if(direction[ksq[side^1]][from])
  { i = direction[ksq[side^1]][from];
    for(dest = ksq[side^1] + i;IsEmpty(dest) || (((1ULL << rsz[dest]) & played) != 0); dest += i);
    if(!IsOutside(dest))
    { if(GetColor(PieceType(dest)) == side)
      { if(attack_vector[dest][ksq[side^1]].flags & atk_masks[PieceType(dest)])
          discovery = true;
      }
    }
  }
  
  //initial capture:
  score[0] = see_pval[square[m.to]];
  nscore++;
  victim = see_pval[PieceType(from)];
  side ^= 1;

  //capture sequence:
  while(atk_count[side])
  {	
    //preparing data.
    smallest = SEE_K_VALUE + 1;
    sm_index = 0;
    sm_sq = 0;
    
    //find smallest attacker for 'side'
    for(i = 0; i < atk_count[side]; i++)
    { v = see_pval[PieceType(attackers[side][i])];
      if(v < smallest)
      { smallest = v;
        sm_index = i;
      }
    }
    
    //if king is exposed and it's not able to capture on the square:
    if(discovery)
    { if(distance_table[ksq[side]][to] != 1) break;
      if(atk_count[side^1]) break;
      score[nscore] = -score[nscore-1] + victim;
      nscore ++;
      break;
    }
    
    //if the only attacker left is the king,
    //is the square still attacked?:
    if(smallest == SEE_K_VALUE)
    { if(atk_count[side^1]) break;
      else
      { score[nscore] = -score[nscore-1] + victim;
        nscore ++;
        break;
      }
    }
    
    sm_sq = attackers[side][sm_index];
    
    //pinned?:
    if(direction[ksq[side]][sm_sq])
    { i = direction[ksq[side]][sm_sq];
      for(dest = ksq[side] + i;IsEmpty(dest) || (((1ULL << rsz[dest]) & played) != 0); dest += i);
      if(!IsOutside(dest))
      { if(dest == sm_sq) //there is no piece between the king and this one!
        {	pinned = false;
          for(dest = sm_sq + i;!IsOutside(dest);dest += i)
          { if(!IsEmpty(dest) && GetColor(PieceType(dest)) == (side^1))
            { //exclude if already played:
              if(((1ULL << rsz[dest]) & played) != 0) continue;
              //friendly piece?:
              if(GetColor(PieceType(dest)) == (side)) break;
              else
              {	//is the found piece able to attack king and it's not a pice that gets re-captured:
                if((dest != m.to) && (attack_vector[dest][ksq[side]].flags & atk_masks[PieceType(dest)]))
                  pinned = true;
                break;
              }
            }
          }//break the main loop if pinned:
          if(pinned == true) break;
        }
      }
    }
    
    //excluding the piece now:
    played |= (1ULL << rsz[sm_sq]);
    
    //discovery check?
    discovery = false;
    if(direction[ksq[side^1]][sm_sq])
    { i = direction[ksq[side^1]][sm_sq];
      for(dest = ksq[side^1] + i; IsEmpty(dest) || (((1ULL << rsz[dest]) & played) != 0); dest += i);
      if(!IsOutside(dest))
      { if(GetColor(PieceType(dest)) == side)
        { if(attack_vector[dest][ksq[side^1]].flags & atk_masks[PieceType(dest)])
            discovery = true;
        }
      }
    }
    
    attackers[side][sm_index] = attackers[side][--atk_count[side]];
    attackers[side][atk_count[side]] = sm_sq;//swap
  
    //now,if we have sliding direction 'target<->sm_sq'.
    //continue and find the next hidden attacker (slider or pawn):
    if(direction[to][sm_sq])
    { i = direction[to][sm_sq];
      for(dest = sm_sq + i;IsEmpty(dest);dest += i);
      if(!IsOutside(dest))
      { if(a[dest].flags & atk_masks[PieceType(dest)])
        { attackers[side][atk_count[side]] = dest;
          atk_count[side]++;
        }
      }
    }

    score[nscore] = -score[nscore-1] + victim;
    nscore++;
    victim = smallest;
    side ^= 1;
  }
  while(--nscore)
    score[nscore - 1] = min(-score[nscore], score[nscore-1]);
  return (score[0]);
}

