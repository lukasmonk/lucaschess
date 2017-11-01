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

#define MTSIZE (4 << 16)
#define MTMASK ((MTSIZE) - 1)
#define MT_ALIGNMENT 64

static const int p_value_mg = 100;
static const int p_value_eg = 100;
static const int n_value_mg = 325;
static const int n_value_eg = 325;
static const int b_value_mg = 325;
static const int b_value_eg = 325;
static const int r_value_mg = 500;
static const int r_value_eg = 500;
static const int q_value_mg = 975;
static const int q_value_eg = 975;
static const int scale_minor_mg =  6;
static const int scale_minor_eg =  6;
static const int scale_major_mg = 12;
static const int scale_major_eg = 12;

material_entry_t *mtable;

static uint32 hash_material
(int wp, int wn, int wb, int wr, int wq, 
 int bp, int bn, int bb, int br, int bq)
 
{ return 
   (zobrist_material[WP][wp] ^
    zobrist_material[WN][wn] ^
    zobrist_material[WB][wb] ^
    zobrist_material[WR][wr] ^
    zobrist_material[WQ][wq] ^
    zobrist_material[BP][bp] ^
    zobrist_material[BN][bn] ^
    zobrist_material[BB][bb] ^
    zobrist_material[BR][br] ^
    zobrist_material[BQ][bq]);
}

static void material_setup(material_entry_t *p, 
int wp, int wn, int wb, int wr, int wq, 
int bp, int bn, int bb, int br, int bq)
{ 
  int wminor, bminor, wmajor, bmajor, wtotal, btotal;
  int scoremg = 0, scoreeg = 0;
  wminor = (wn + wb);
  bminor = (bn + bb);
  wmajor = (wq * 2) + wr;
  bmajor = (bq * 2) + br;
  wtotal = (wmajor * 2) + wminor;
  btotal = (bmajor * 2) + bminor;
  p->wmul = p->bmul = 16;
  p->flags = 0;
  
  //storing scale multipliers and flags:
  if(!wp && !bp)
  { if((wtotal == 2) && !btotal && (wn == 1) && (wb == 1)) p->flags = KNBK;
    if((btotal == 2) && !wtotal && (bn == 1) && (bb == 1)) p->flags = KKNB;
  }
  if(((wtotal == 1) && !btotal && (wb == 1) && (wp > 0))
  || ((btotal == 1) && !wtotal && (bb == 1) && (bp > 0)))
    p->flags = KBPK;
 
  if(!wp)
  { if(wtotal <= 1) p->wmul = 0;
    else if((wtotal == 2) && (wn == 2))
    { if((btotal > 0) || !bp) p->wmul = 0;
      else p->wmul = 4;
    }
    else if((wtotal == 2) && (wb == 2) && (btotal == 1) && (bn == 1)) p->wmul = 8;
    else if((wtotal - btotal) < 2 && (wmajor <= 2)) p->wmul = 4;
  }
  else if(wp == 1)
  { if(bminor > 0) 
    { if(wtotal == 1) p->wmul = 8; 
      else if((wtotal == 2) && (wn == 2)) p->wmul = 8; 
      else if((wtotal + 1 - btotal) < 2 && (wmajor <= 2)) p->wmul = 12;
    }
    else if(br > 0)
    { if(wtotal == 1) p->wmul =4;
      else if((wtotal == 2) && (wn == 2)) p->wmul = 4;
      else if((wtotal + 2 - btotal) < 2 && (wmajor <= 2)) p->wmul = 8;
    }
  }
    
  if(!bp)
  { if(btotal <= 1) p->bmul = 0;
    else if((btotal == 2) && (bn == 2))
    { if((wtotal > 0) || !wp) p->bmul = 0;
      else p->bmul = 4;
    }
    else if((btotal == 2) && (bb == 2) && (wtotal == 1) && (wn == 1)) p->bmul = 8;
    else if((btotal - wtotal) < 2 && (bmajor <= 2)) p->bmul = 4;
  }
  else if(bp == 1)
  { if(wminor > 0)
    { if(btotal == 1) p->bmul = 8;
      else if((btotal == 2) && (bn == 2)) p->bmul = 8;
      else if((btotal + 1 - wtotal) < 2 && (bmajor <= 2)) p->bmul = 12;
    }
    else if(wr > 0)
    { if(btotal == 1) p->bmul = 4;
      else if((btotal == 2) && (bn == 2)) p->bmul = 4;
      else if((btotal + 2 - wtotal) < 2 && (bmajor <= 2)) p->bmul = 8;
    }
  }
  
  if((wtotal == 1) && (btotal == 1) && (wb == 1) && (bb == 1))
    p->flags = TRY_OPPC_BISHOPS;
  
  if(wb > 1) p->flags  = TRY_BISHOP_PAIR_W;
  if(bb > 1) p->flags |= TRY_BISHOP_PAIR_B;
  
  //material balance:
  scoremg  = ((wp - bp) * p_value_mg);
  scoreeg  = ((wp - bp) * p_value_eg);
  scoremg += ((wn - bn) * n_value_mg);
  scoreeg += ((wn - bn) * n_value_eg);
  scoremg += ((wb - bb) * b_value_mg);
  scoreeg += ((wb - bb) * b_value_eg);
  scoremg += ((wr - br) * r_value_mg);
  scoreeg += ((wr - br) * r_value_eg);
  scoremg += ((wq - bq) * q_value_mg);
  scoreeg += ((wq - bq) * q_value_eg);
  
  ///Imbalance calculations, proposed by GM Kaufman
  //basic knight and rook material imbalances:
  scoremg += ((wr * (5 - wp)) * scale_major_mg);
  scoremg -= ((br * (5 - bp)) * scale_major_mg);
  scoremg += ((wn * (wp - 6)) * scale_minor_mg);
  scoremg -= ((bn * (bp - 6)) * scale_minor_mg);
  
  scoreeg += ((wr * (5 - wp)) * scale_major_eg);
  scoreeg -= ((br * (5 - bp)) * scale_major_eg);
  scoreeg += ((wn * (wp - 6)) * scale_minor_eg);
  scoreeg -= ((bn * (bp - 6)) * scale_minor_eg);
  
  if(wr)//redundancy:
  { scoremg -= (((wr - 1) * scale_major_mg) + (wq * scale_minor_mg));
    scoreeg -= (((wr - 1) * scale_major_eg) + (wq * scale_minor_eg));
  }
  if(br)
  { scoremg += (((br - 1) * scale_major_mg) + (bq * scale_minor_mg));
    scoreeg += (((br - 1) * scale_major_eg) + (bq * scale_minor_eg));
  }

  p->scoremg = scoremg;
  p->scoreeg = scoreeg;
  p->phase = (uint8)((wn + wb + bn + bb) + ((wr + br) * 2) + ((wq + bq) * 4));
  if(p->phase > 24) p->phase = 24;
}

int init_material()
{
  int wp,wn,wb,wr,wq;
  int bp,bn,bb,br,bq;
  material_entry_t *p;
  uint32 key;
  
  aligned_free((void *)mtable); 
  mtable = (material_entry_t *)aligned_malloc((size_t)MTSIZE*sizeof(material_entry_t), MT_ALIGNMENT);
  if(!mtable) return false;
  aligned_wipe_out((void *)mtable, (size_t)MTSIZE*sizeof(material_entry_t), MT_ALIGNMENT);
  
  for(wp = 8; wp >= 0; wp--)
  for(bp = 8; bp >= 0; bp--)
  for(wn = 2; wn >= 0; wn--)
  for(bn = 2; bn >= 0; bn--)
  for(wb = 2; wb >= 0; wb--)
  for(bb = 2; bb >= 0; bb--)
  for(wr = 2; wr >= 0; wr--)
  for(br = 2; br >= 0; br--)
  for(wq = 1; wq >= 0; wq--)
  for(bq = 1; bq >= 0; bq--)
  { key = hash_material(wp, wn, wb, wr, wq, bp, bn, bb, br, bq);
    p = mtable + (key & MTMASK);
    p->mkey = key;
    material_setup(p, wp, wn, wb, wr, wq, bp, bn, bb, br, bq);
  }
  return true;
}

material_entry_t *eval_material()
{
  int wp, bp, wn, bn, wb, bb, wr, br, wq, bq;
  
  material_entry_t *p = mtable + (pos->mhash & MTMASK);
  
  if(p->mkey == pos->mhash) return (p);
  
  wp = pos->pcount[WP];
  bp = pos->pcount[BP];
  wn = pos->pcount[WN];
  bn = pos->pcount[BN];
  wb = pos->pcount[WB];
  bb = pos->pcount[BB];
  wr = pos->pcount[WR];
  br = pos->pcount[BR];
  wq = pos->pcount[WQ];
  bq = pos->pcount[BQ];
  
  material_setup(p, wp, wn, wb, wr, wq, bp, bn, bb, br, bq);
  p->mkey = pos->mhash;
  return (p);
}

bool is_pawn_endgame()
{ if(pos->occ[OCC_W] == (pos->occ[WP] | pos->occ[WK])
  && pos->occ[OCC_B] == (pos->occ[BP] | pos->occ[BK]))
    return true;
  return false;
}

bool is_king_alone()
{ if(pos->occ[OCC_W] == pos->occ[WK]
  || pos->occ[OCC_B] == pos->occ[BK])
    return true;
  return false;
}

int get_piece_count(int color)
{
  return (pos->pcount[KNIGHT | (color << 3)] + \
          pos->pcount[BISHOP | (color << 3)] + \
          pos->pcount[ROOK   | (color << 3)] + \
          pos->pcount[QUEEN  | (color << 3)]);
}

int get_phase()
{
  material_entry_t *p = mtable + (pos->mhash & MTMASK);
  if(p->mkey == pos->mhash)
    return (p->phase);
  return
   ((pos->pcount[WN] + pos->pcount[BN]) + \
    (pos->pcount[WB] + pos->pcount[BB]) + \
    ((pos->pcount[WR] + pos->pcount[BR]) * 2) +
    ((pos->pcount[WQ] + pos->pcount[BQ]) * 4));
}

int material_value()
{ material_entry_t *p = eval_material();
  return (pos->side == WHITE) ? (p->scoremg) : (-p->scoremg);
}

