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


int endeval(int score[])
{
  register int mvl_w = board->state.material_value[W];
  register int mvl_b = board->state.material_value[B];

  ///endgame type scalling:
  if(!board->state.pawns)
  { 
    //insufficient material check:
    if((mvl_w < N_VALUE + B_VALUE) 
    && (mvl_b < N_VALUE + B_VALUE))
    { if(mvl_w != R_VALUE && mvl_b != R_VALUE) 
        return 0;
    }
    
    //other trivial draws:
    if(mvl_w == R_VALUE && mvl_b == R_VALUE)
      return 0;
    if(mvl_w == 2*R_VALUE && mvl_b == 2*R_VALUE)
      return 0;
    if(mvl_w == N_VALUE+B_VALUE && mvl_b == N_VALUE+B_VALUE)
      return 0;
    if(mvl_w == B_VALUE+B_VALUE && mvl_b == B_VALUE+B_VALUE)
      return 0;
  
    //KNBK
    if(mvl_w == N_VALUE + B_VALUE && mvl_b == K_VALUE)
    { if(square_color[PLS(WB)] == W)
        //keep the attacked king away from the center and close to the corners:
        score[W] -= min(distance_table[King_Square(B)][A8], \
          distance_table[King_Square(B)][H1]) * 10;
      else score[W] -= min(distance_table[King_Square(B)][A1], \
        distance_table[King_Square(B)][H8]) * 10;
      score[W] += distance_table[King_Square(B)][E4] * 2;
    }
    if(mvl_b == N_VALUE + B_VALUE && mvl_w == K_VALUE)
    { if(square_color[PLS(BB)] == W)
        score[B] -= min(distance_table[King_Square(W)][A8],\
          distance_table[King_Square(W)][H1]) * 10;
      else score[B] -= min(distance_table[King_Square(W)][A1],\
        distance_table[King_Square(W)][H8]) * 10;
      score[B] += distance_table[King_Square(W)][E4] * 2;
    }
  }
  ///final results:
  score[W] += (mvl_w + board->state.pawn_value[W]);
  score[B] += (mvl_b + board->state.pawn_value[B]);
  if(board->side == W) return  (score[W]-score[B]);
  else return -(score[W]-score[B]);
}


