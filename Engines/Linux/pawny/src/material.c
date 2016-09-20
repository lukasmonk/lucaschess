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

static const int exchange_mg = 2;
static const int exchange_eg = 2;

material_entry_t *mtable;

uint32 hash_material()
{ return 
   (zobrist_material[WP][pos->pcount[WP]] ^
    zobrist_material[WN][pos->pcount[WN]] ^
    zobrist_material[WB][pos->pcount[WB]] ^
    zobrist_material[WR][pos->pcount[WR]] ^
    zobrist_material[WQ][pos->pcount[WQ]] ^
    zobrist_material[BP][pos->pcount[BP]] ^
    zobrist_material[BN][pos->pcount[BN]] ^
    zobrist_material[BB][pos->pcount[BB]] ^
    zobrist_material[BR][pos->pcount[BR]] ^
    zobrist_material[BQ][pos->pcount[BQ]]);
}

void material_setup(material_entry_t *p, 
int wp, int wn, int wb, int wr, int wq, 
int bp, int bn, int bb, int br, int bq)
{
  int wminor, bminor, wmajor, bmajor, wtotal, btotal;
  int wmaterial = (sint16)((N_VALUE * wn) + (B_VALUE * wb) + (R_VALUE * wr) + (Q_VALUE * wq));
  int bmaterial = (sint16)((N_VALUE * bn) + (B_VALUE * bb) + (R_VALUE * br) + (Q_VALUE * bq));
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
  { p->flags = PAWNLESS_EG;
    if((wtotal == 2) && !btotal && (wn == 1) && (wb == 1)) p->flags |= KNBK;
    if((btotal == 2) && !wtotal && (bn == 1) && (bb == 1)) p->flags |= KKNB;
  }
  if((wp + bp) && !wtotal && !btotal)
  { p->flags = PAWN_EG;
    p->wmul = 24;
    p->bmul = 24;
  }
  if((wtotal == 1) && !btotal && (wb == 1) && (wp > 0)) p->flags |= KBPK;
  if((btotal == 1) && !wtotal && (bb == 1) && (bp > 0)) p->flags |= KBPK;
 
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
    p->flags |= TRY_OPPC_BISHOPS;

  if(wb > 1) p->flags |= (TRY_BISHOP_PAIR | TRY_BISHOP_PAIR_W);
  if(bb > 1) p->flags |= (TRY_BISHOP_PAIR | TRY_BISHOP_PAIR_B);  
  
  //storing the "scalling" values:
  p->material[W] = (sint16)(wmaterial);
  p->material[B] = (sint16)(bmaterial);
  
  //material ballance score storing:
  p->scoremg  = (wp * p_value_mg) + (wn * n_value_mg) + 
                (wb * b_value_mg) + (wr * r_value_mg) + (wq * q_value_mg);
  p->scoreeg  = (wp * p_value_eg) + (wn * n_value_eg) + 
                (wb * b_value_eg) + (wr * r_value_eg) + (wq * q_value_eg);
  p->scoremg -= (bp * p_value_mg) + (bn * n_value_mg) + 
                (bb * b_value_mg) + (br * r_value_mg) + (bq * q_value_mg);
  p->scoreeg -= (bp * p_value_eg) + (bn * n_value_eg) + 
                (bb * b_value_eg) + (br * r_value_eg) + (bq * q_value_eg);
  
  //a piece of code to encourage exchanges
  //in case of material or pawn advantage /penalty form/:
  if(p->scoremg > 0 && p->wmul == 16)
  { p->scoremg -= ((wminor + bminor + wq + bq + wr + br) * exchange_mg);
    p->scoreeg -= ((wminor + bminor + wq + bq + wr + br) * exchange_eg);
  }
  if(p->scoremg < 0 && p->bmul == 16)
  { p->scoremg += ((wminor + bminor + wq + bq + wr + br) * exchange_mg);
    p->scoreeg += ((wminor + bminor + wq + bq + wr + br) * exchange_eg);
  }

  //calculate basic knight and rook material imbalances:
  p->scoremg += ((wr * (5 - wp)) * scale_major_mg);
  p->scoremg -= ((br * (5 - bp)) * scale_major_mg);
  p->scoremg += ((wn * (wp - 6)) * scale_minor_mg);
  p->scoremg -= ((bn * (bp - 6)) * scale_minor_mg);
  
  p->scoreeg += ((wr * (5 - wp)) * scale_major_eg);
  p->scoreeg -= ((br * (5 - bp)) * scale_major_eg);
  p->scoreeg += ((wn * (wp - 6)) * scale_minor_eg);
  p->scoreeg -= ((bn * (bp - 6)) * scale_minor_eg);
  
  if(wr)//redundancy:
  { p->scoremg -= (((wr - 1) * scale_major_mg) + (wq * scale_minor_mg));
    p->scoreeg -= (((wr - 1) * scale_major_eg) + (wq * scale_minor_eg));
  }
  if(br)
  { p->scoremg += (((br - 1) * scale_major_mg) + (bq * scale_minor_mg));
    p->scoreeg += (((br - 1) * scale_major_eg) + (bq * scale_minor_eg));
  }
  
  p->phase = ((wn + wb + bn + bb) + ((wr + br) * 2) + ((wq + bq) * 4));
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
  pos->mhash = hash_material();
  
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
  { key = hash_material();
    p = mtable + (key & MTMASK);
    if(p->mkey == key) continue;
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
  
  material_setup(p, wp,wn,wb,wr,wq,bp,bn,bb,br,bq);
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
    return (p->material[W]+p->material[B]);
  return
   ((N_VALUE * pos->pcount[WN]) + \
    (B_VALUE * pos->pcount[WB]) + \
    (R_VALUE * pos->pcount[WR]) + \
    (Q_VALUE * pos->pcount[WQ]) + \
    (N_VALUE * pos->pcount[BN]) + \
    (B_VALUE * pos->pcount[BB]) + \
    (R_VALUE * pos->pcount[BR]) + \
    (Q_VALUE * pos->pcount[BQ]));
}

int material_gain(move_t m)
{ int result = 0;
  if(m.promoted)
  { result -= P_VALUE;
    result += pval[m.promoted];
  }
  if(m.type & CAP)
    result += pval[pos->stack[pos->sply - 1].captured];
  return result;
}
