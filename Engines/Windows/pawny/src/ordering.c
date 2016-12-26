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

/*-----------------------------
  1.hashmove/pvmove
  2.promotions+cap
  3.promotions
  4.good captures
  5.equal captures
  6.killers
  7.castlings
  8.history counters
  9.bad captures (see() < 0)
-------------------------------*/

void ordering(move_t ms[], int count, uint32 bestmove)
{
  int i,ply = board->ply;
  
  for (i = 0; i < count; i++)
  { if(ms[i].p  == bestmove) 
      ms[i].score = 1000000;
    else if(ms[i].type & PROM)
    { ms[i].score = 500000 + pval[ms[i].promoted];
      if(ms[i].type & CAP) 
        ms[i].score += pval[PieceType(ms[i].to)];
    }
    else if(ms[i].type & CAP)
    { if(ms[i].type & EP)
        ms[i].score = 30000 - P_VALUE;
      else
      { int p_from = pval[PieceType(ms[i].from)];
        int p_to = pval[PieceType(ms[i].to)];
        //bishop/knight correction
        if(abs(p_to - p_from) == B_N_DIFF) 
          ms[i].score = 30000 - p_from;
        else if(p_to > p_from)//good caps
          ms[i].score = 40000 + (p_to - p_from);
        else if(p_to == p_from) //equal caps
          ms[i].score = 30000 - p_from;
        else if(p_to < p_from) //presumably bad caps
        { ms[i].score = see(ms[i]);
          if(ms[i].score == 0) ms[i].score = 30000 - p_from;//it's equal.
          else if(ms[i].score > 0) ms[i].score += 40000; //appears to be a good sequence.
        }
      }
    }
    else if(ms[i].p == si->killer[0][ply])
      ms[i].score = 20000;
    else if(ms[i].p == si->killer[1][ply])
      ms[i].score = 10000;
    else if(ms[i].type & CASTLE) 
      ms[i].score = 5000 - (ms[i].type ^ CASTLE);
    else ms[i].score += si->history[ms[i].from][ms[i].to];
  }
}

void caps_ordering(move_t ms[], int count)
{
  int i;
  for (i = 0; i < count; i++)
  { if(ms[i].type & PROM)
    { ms[i].score = 500000 + pval[ms[i].promoted];
      if(ms[i].type & CAP) 
        ms[i].score += pval[PieceType(ms[i].to)];
    }
    else if(ms[i].type & CAP)
    { if(ms[i].type & EP)
        ms[i].score = 30000 - P_VALUE;
      else
      { int p_from = pval[PieceType(ms[i].from)];
        int p_to = pval[PieceType(ms[i].to)];
        //bishop/knight correction
        if(abs(p_to - p_from) == B_N_DIFF) 
          ms[i].score = 30000 - p_from;
        else if(p_to > p_from)//good caps
          ms[i].score = 40000 + (p_to - p_from);
        else if(p_to == p_from) //equal caps
          ms[i].score = 30000 - p_from;
        else if(p_to < p_from) //presumably bad caps
        { ms[i].score = see(ms[i]);
          if(ms[i].score == 0) ms[i].score = 30000 - p_from;//it's equal.
          else if(ms[i].score > 0) ms[i].score += 40000; //appears to be a good sequence.
        }
      }
    }
  }
}
