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

static bool is_sq_attacked_w(sq_t target)
{//is the square attacked by whites:
  int sq, dest, delta;
  attack_t *a = &attack_vector[target][0];

  //pawns:
  if(PieceType(target - 15) == WP) return true;
  if(PieceType(target - 17) == WP) return true;
  
  //queens:
  for(sq = PLS(WQ); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & R_ATK || a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest) && (dest != target); dest += delta);
      if(dest == target) return true;
    }
  }
  
  //rooks:
  for(sq = PLS(WR); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & R_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest) && (dest != target); dest += delta);
      if(dest == target) return true;
    }
  }
  
  //bishops:
  for(sq = PLS(WB); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest) && (dest != target); dest += delta);
      if(dest == target) return true;
    }
  }
  
  //knights:
  for(sq = PLS(WN); sq <= H8; sq = PLN(sq))
    if((a[sq].flags & N_ATK) && (sq + a[sq].delta == target))
        return true;
   
  //king:
  sq = King_Square(W);
  if((a[sq].flags & R_ATK || a[sq].flags & B_ATK) && (sq + a[sq].delta == target))
    return true;

  return false;
}

static bool is_sq_attacked_b(sq_t target)
{//is the square attacked by blacks:
  
  int sq, dest, delta;
  attack_t *a = &attack_vector[target][0];

  //pawns:
  if(PieceType(target + 15) == BP) return true;
  if(PieceType(target + 17) == BP) return true;
  
  //queens:
  for(sq = PLS(BQ); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & R_ATK || a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest) && (dest != target); dest += delta);
      if(dest == target) return true;
    }
  }
  
  //rooks:
  for(sq = PLS(BR); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & R_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest) && (dest != target); dest += delta);
      if(dest == target) return true;
    }
  }
  
  //bishops:
  for(sq = PLS(BB); sq <= H8; sq = PLN(sq))
  { if(a[sq].flags & B_ATK)
    { delta = a[sq].delta;
      for(dest = sq + delta; IsEmpty(dest) && (dest != target); dest += delta);
      if(dest == target) return true;
    }
  }
  
  //knights:
  for(sq = PLS(BN); sq <= H8; sq = PLN(sq))
    if((a[sq].flags & N_ATK) && (sq + a[sq].delta == target))
      return true;
  
  //king:
  sq = King_Square(B);
  if((a[sq].flags & R_ATK || a[sq].flags & B_ATK) && (sq + a[sq].delta == target))
    return true;

  return false;
}

bool is_sq_attacked(sq_t target, int side)
{ if(side == W) return is_sq_attacked_w(target);
  return is_sq_attacked_b(target);
}

bool is_in_check(int side)
{ if(side == W) return is_sq_attacked_b(King_Square(W));
  return is_sq_attacked_w(King_Square(B));
}

void init_attack_vector()
{ 
  int i,j,p,dest;
  const int *d;
  
  memset(&attack_vector,0,sizeof(attack_vector));
  for(i = A1; i <= H8; i++)
  { if(IsOutside(i)) continue;
    for(j = A1; j <= H8; j++)
    { if(IsOutside(j) || j == i) continue;
      for(p = ROOK; p >= KNIGHT; p--)
        for(d = dir_vect[p]; *d != 0; d++)
          for(dest = i + *d; !IsOutside(dest); dest += *d)
            if(dest == j)
            { attack_vector[i][j].flags |= atk_masks[p];
              attack_vector[i][j].delta = -(sint8)*d;
            }
    }
  }
}

