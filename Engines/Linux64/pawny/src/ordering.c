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

void ordering(move_t *ms, int count, uint32 bestmove)
{ int i, p_from, p_to, ply = pos->ply;
  
  for (i = 0; i < count; i++)
  { if(ms[i].p  == bestmove) 
      ms[i].score = 100000;
    else if(ms[i].type & PROM)
    { ms[i].score = 50000 + pval[ms[i].promoted];
      if(ms[i].type & CAP) 
        ms[i].score += pval[Piece(ms[i].to)];
    }
    else if(ms[i].type & CAP)
    { if(ms[i].type & EP)
        ms[i].score = 30000 - P_VALUE;
      else
      { p_from = pval[Piece(ms[i].from)];
        p_to = pval[Piece(ms[i].to)];
        if(p_to > p_from)//good caps
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
    else if(ms[i].p == si.killer[0][ply])
      ms[i].score = 20000;
    else if(ms[i].p == si.killer[1][ply])
      ms[i].score = 10000;
    else if(ms[i].type & CASTLE) 
      ms[i].score = 5000;
    else ms[i].score = si.history[pos->square[ms[i].from]][ms[i].to];
  }
}

void caps_ordering(move_t *ms, int count)
{
  int i, p_from, p_to;
  for (i = 0; i < count; i++)
  { if(ms[i].type & PROM)
    { ms[i].score = 50000 + pval[ms[i].promoted];
      if(ms[i].type & CAP) 
        ms[i].score += pval[Piece(ms[i].to)];
    }
    else if(ms[i].type & CAP)
    { if(ms[i].type & EP)
        ms[i].score = 30000 - P_VALUE;
      else
      { p_from = pval[Piece(ms[i].from)];
        p_to = pval[Piece(ms[i].to)];
        if(p_to > p_from)//good caps
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

/*-----------------------------
   1.hashmove/pvmove
   2.promotions+cap
   3.promotions
   4.good captures
   5.equal captures
   6.castlings
   7.checks
   8.nodes and history counts
   9.bad captures (see() < 0)
-------------------------------*/
void root_ordering()
{ int i, j, size, p_from, p_to, check;
  rootmove_t tmove;
  size = root_list_size;

  for (i = 0; i < size; i++)
  { p_from = pval[Piece(rms[i].m.from)];
    p_to = pval[Piece(rms[i].m.to)];
    if(rms[i].m.p  == si.rootmove.p) 
      rms[i].score = 0x7FFFFFFFFFFEFFFF;
    else if(rms[i].m.type & PROM)
    { rms[i].score = 0x7FFFFFFFFFFDFFFF + pval[rms[i].m.promoted];
      if(rms[i].m.type & CAP) 
        rms[i].score += pval[Piece(rms[i].m.to)];
    }
    else if(rms[i].m.type & CAP)
    { if(rms[i].m.type & EP)
        rms[i].score = 0x7FFFFFFFFFFBFFFF - P_VALUE;
      else
      { if(p_to > p_from)//good caps
          rms[i].score = 0x7FFFFFFFFFFCFFFF + (p_to - p_from);
        else if(p_to == p_from) //equal caps
          rms[i].score = 0x7FFFFFFFFFFBFFFF - p_from;
        else if(p_to < p_from) //presumably bad caps
        { rms[i].score = see(rms[i].m);
          if(rms[i].score == 0) rms[i].score = 0x7FFFFFFFFFFBFFFF - p_from;//it's equal.
          else if(rms[i].score > 0) rms[i].score += 0x7FFFFFFFFFFCFFFF; //appears to be a good sequence.
        }
      }
    }
    else if(rms[i].m.type & CASTLE) 
      rms[i].score = 0x7FFFFFFFFFFAFFFF;
    else
    { move_make(rms[i].m);
      check = is_in_check(pos->side);
      move_unmake();
      if(check && see_squares(rms[i].m.from, rms[i].m.to) >= 0)
        rms[i].score = 0x7FFFFFFFFFF9FFFF - p_from;
  
      rms[i].score += si.history[pos->square[rms[i].m.from]][rms[i].m.to];
    }
  }
  
  //sorting the moves:
  for(i = 0; i < size; i++)
    for(j = size - 1; j > i; j--)
      if(rms[j - 1].score < rms[j].score)
      { tmove = rms[j - 1];
        rms[j - 1] = rms[j];
        rms[j] = tmove;
      }
}

void init_rootlist()
{ int i;
  move_t ms[MSSIZE];
  root_list_size = move_gen_legal(ms);
  for(i = 0; i < root_list_size; i++)
  { rms[i].m.p = ms[i].p;
    rms[i].score = 0ULL;
  }
}
