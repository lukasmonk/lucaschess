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

bool eval_endgame(material_entry_t *mt, int *scoremg, int *scoreeg)
{
  if(mt->flags & KNBK)
  { if(pos->occ[WB] & light_squares)
      //keep the attacked king away from the center and close to the corners:
      *(scoremg) -= min(distance[pos->ksq[B]][A8], distance[pos->ksq[B]][H1]) * 10;
    else *(scoremg) -= min(distance[pos->ksq[B]][A1], distance[pos->ksq[B]][H8]) * 10;
    *(scoremg) += distance[pos->ksq[B]][E4] * 2;
    *(scoreeg) = *(scoremg);
    return true;
  }
  if(mt->flags & KKNB)
  { if(pos->occ[BB] & light_squares)
      *(scoremg) += min(distance[pos->ksq[W]][A8], distance[pos->ksq[W]][H1]) * 10;
    else *(scoremg) += min(distance[pos->ksq[W]][A1], distance[pos->ksq[W]][H8]) * 10;
    *(scoremg) -= distance[pos->ksq[W]][E4] * 2;
    *(scoreeg) = *(scoremg);
    return true;
  }
  
  if(mt->flags & PAWN_EG)
  { if(pos->pcount[WP])
    { if(((!(pos->occ[WP] & ~FMASK_A)) && distance[pos->ksq[B]][A8] <= 1)
      || ((!(pos->occ[WP] & ~FMASK_H)) && distance[pos->ksq[B]][H8] <= 1))
      { *(scoremg) = 0;
        *(scoreeg) = 0;
        return true;
      }
    }
    if(pos->pcount[BP])
    { if(((!(pos->occ[BP] & ~FMASK_A)) && distance[pos->ksq[W]][A1] <= 1)
      || ((!(pos->occ[BP] & ~FMASK_H)) && distance[pos->ksq[W]][H1] <= 1))
      { *(scoremg) = 0;
        *(scoreeg) = 0;
        return true;
      }
    }
  }

  if(mt->flags & KBPK)
  { if(pos->pcount[WP])
    { if(((!(pos->occ[WP] & ~FMASK_A)) && (pos->occ[WB] & dark_squares)  && distance[pos->ksq[B]][A8] <= 1)
      || ((!(pos->occ[WP] & ~FMASK_H)) && (pos->occ[WB] & light_squares) && distance[pos->ksq[B]][H8] <= 1))
      { *(scoremg) = 0;
        *(scoreeg) = 0;
        return true;
      }
    }
    if(pos->pcount[BP])
    { if(((!(pos->occ[BP] & ~FMASK_A)) && (pos->occ[BB] & light_squares) && distance[pos->ksq[W]][A1] <= 1)
      || ((!(pos->occ[BP] & ~FMASK_H)) && (pos->occ[BB] && dark_squares) && distance[pos->ksq[W]][H1] <= 1))
      { *(scoremg) = 0;
        *(scoreeg) = 0;
        return true;
      }
    }
  }
  return false;
}

