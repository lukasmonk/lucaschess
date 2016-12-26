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

void hash_board()
{//hashing the whole position and pawn structure -
//only at position initialization,
//the rest of hashing is done incrementally.
  int i;
  uint64 key = 0,pkey = 0;
    
  for(i = A1; i <= H8; i++)
  { 
    if(IsOutside(i) || IsEmpty(i)) continue;
    key ^= zobrist_psq[PieceType(i)][i];
  }
  
  key ^= zobrist_ep[board->en_passant];
  key ^= zobrist_castle[board->castle];
  
  //xor side - only current, because no old state yet:
  key ^= zobrist_side[board->side];
  board->hash = key;
  
  //hashing the pawn structure:
  for(i = PLS(WP); i <= H8; i = PLN(i)) pkey ^= zobrist_psq[WP][i];
  for(i = PLS(BP); i <= H8; i = PLN(i)) pkey ^= zobrist_psq[BP][i];
  board->phash = pkey;
}
