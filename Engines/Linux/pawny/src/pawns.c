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

#define PT_ALIGNMENT 64
#define PNTSIZE (1 << 16)
#define PMASK ((PNTSIZE) - 1)

//pawn table pointer:
pawn_entry_t *pnt = 0;

//king near passed pawn bonus:
static const int king_activity[8] = 
{0, 50, 40, 30, 20, 10, 8, 0};

//passer advancement:
static const int passer_bonus_mg[2][8] = //by rank
{{0, 0,  0, 15, 45, 90, 150, 0},
{0, 150, 90, 45, 15, 0, 0, 0}};

static const int passer_bonus_eg[2][8] = //by rank
{{0, 10, 10, 20, 40, 80, 120, 0},
{0, 120, 80, 40, 20, 10, 10, 0}};

static const int isolated_penalty_mg[8] = //by file
{12, 12, 12, 14, 14, 12, 12, 12};
static const int isolated_penalty_eg[8] =
{12, 12, 12, 14, 14, 12, 12, 12};

static const int backward_penalty_mg[8] = //by file
{8, 8, 8, 10, 10, 8, 8, 8};
static const int backward_penalty_eg[8] =
{10, 10, 10, 12, 12, 10, 10, 10};

static const int doubled_penalty_mg[8] = //by file
{8, 8, 8, 10, 10, 8, 8, 8};
static const int doubled_penalty_eg[8] =
{14, 15, 15, 16, 16, 15, 15, 14};

static const int candidate_mg[2][8] = //by rank
{{0, 2, 2, 5, 15, 30, 0, 0},
{0, 0, 30, 15, 5, 2, 2, 0}};
static const int candidate_eg[2][8] = 
{{0, 4, 4, 10, 30, 60, 0, 0},
{0, 0, 60, 30, 10, 4, 4, 0}};

static const int connected_passer_mg[2][8] = //by rank
{{0, 0, 2, 4, 6, 8, 10, 0},
{0, 10, 8, 6, 4, 2, 0, 0}};
static const int connected_passer_eg[2][8] = 
{{0, 0, 6, 14, 22, 30, 40, 0},
{0, 40, 30, 22, 14, 6, 0, 0}};

static const int pawn_chain_element_mg[8] = //by file
{4, 4, 4, 5, 5, 4, 4, 4};
static const int pawn_chain_element_eg[8] =
{2, 2, 2, 4, 4, 2, 2, 2};

static const int weak_square_mg = 8;
static const int backward_on_semi_op_mg = 10;
static const int isolated_on_semi_op_mg = 10;

//passed pawns bonuses/penalties:
static const int clear_path[2][8] = 
{{0, 0, 0, 1, 3, 5, 10, 0},
{0, 10, 5, 3, 1, 0, 0, 0}};
 
static const int rook_in_front[2][8] = 
{{0, 0, 0, 0, 0, 25, 50, 0},
{0, 50, 25, 0, 0, 0, 0, 0}};
 
static const int queen_in_front[2][8] = 
{{ 0, 0, 0, 0, 0, 0, 25,0},
{0, 25, 0, 0, 0, 0, 0, 0}};

static const int free_passer[2][8] =
{{0, 0, 0, 12, 35, 70, 110, 0},
{0, 110, 70, 35, 12, 0, 0, 0}};

static const int free_with_support[2][8] = 
{{0, 0, 0, 13, 40, 80, 120, 0},
{0, 120, 80, 40, 13, 0, 0, 0}};

static const int blocked_passer[2][8] = 
{{0, 0, 0, 6, 18, 36, 60, 0},
{0, 60, 36, 18, 6, 0, 0, 0}};
   
static const int blocked_with_support[2][8] =
{{0, 0, 0, 10, 30, 60, 100, 0},
{0, 100, 60, 30, 10, 0, 0, 0}};

//king shield related:
//L/R gets reversed when in O-O-O:
static const int wshelterR[8] = 
{40, 0, 15, 20, 25, 30, 35, 0};
static const int wshelterM[8] = 
{45, 0, 20, 25, 30, 35, 40, 0};
static const int wshelterL[8] = 
{40, 0, 15, 20, 25, 30, 35, 0};

static const int bshelterR[8] = 
{40, 35, 30, 25, 20, 15, 0, 0};
static const int bshelterM[8] = 
{45, 40, 35, 30, 25, 20, 0, 0};
static const int bshelterL[8] = 
{40, 35, 30, 25, 20, 15, 0, 0};

//white pawns:
static const int wstormM[8] = 
{0,  0,  0,  10, 20, 30,  0,  0};
static const int wstormR[8] = 
{0,  0,  0,  10, 20, 30,  0,  0};
static const int wstormL[8] = 
{0,  0,  0,  10, 20, 30,  0,  0};
//black pawns:
static const int bstormM[8] = 
{0,  0, 30, 20, 10,  0,  0,  0};
static const int bstormR[8] = 
{0,  0, 30, 20, 10,  0,  0,  0};
static const int bstormL[8] = 
{0,  0, 30, 20, 10,  0,  0,  0};

int pnt_init()
{ aligned_free((void *)pnt); 
  pnt = 0;
  pnt = (pawn_entry_t *)aligned_malloc((size_t)PNTSIZE*sizeof(pawn_entry_t), PT_ALIGNMENT);
  if(!pnt) return false;
  aligned_wipe_out((void *)pnt, (size_t)PNTSIZE*sizeof(pawn_entry_t), PT_ALIGNMENT);
  return true;
}

static void pawn_shield_store(pawn_entry_t *p)
{ //white:
  int penalty = wshelterM[Rank(bitscanf(pos->occ[WP] & FMASK_G))];
  penalty += wshelterL[Rank(bitscanf(pos->occ[WP] & FMASK_F))];
  penalty += wshelterR[Rank(bitscanf(pos->occ[WP] & FMASK_H))];
  penalty += bstormM[Rank(bitscanf(pos->occ[BP] & FMASK_G))];
  penalty += bstormL[Rank(bitscanf(pos->occ[BP] & FMASK_F))];
  penalty += bstormR[Rank(bitscanf(pos->occ[BP] & FMASK_H))];
  p->wg1 = (uint8)penalty;
  
  penalty  = wshelterM[Rank(bitscanf(pos->occ[WP] & FMASK_B))];
  penalty += wshelterL[Rank(bitscanf(pos->occ[WP] & FMASK_C))];
  penalty += wshelterR[Rank(bitscanf(pos->occ[WP] & FMASK_A))];
  penalty += bstormM[Rank(bitscanf(pos->occ[BP] & FMASK_B))];
  penalty += bstormL[Rank(bitscanf(pos->occ[BP] & FMASK_C))];
  penalty += bstormR[Rank(bitscanf(pos->occ[BP] & FMASK_A))];
  p->wb1 = (uint8)penalty;
  
  penalty  = wshelterM[Rank(bitscanf(pos->occ[WP] & FMASK_E))];
  penalty += wshelterL[Rank(bitscanf(pos->occ[WP] & FMASK_D))];
  penalty += wshelterR[Rank(bitscanf(pos->occ[WP] & FMASK_F))];
  penalty += bstormM[Rank(bitscanf(pos->occ[BP] & FMASK_E))];
  penalty += bstormL[Rank(bitscanf(pos->occ[BP] & FMASK_D))];
  penalty += bstormR[Rank(bitscanf(pos->occ[BP] & FMASK_F))];
  p->we1 = (uint8)penalty;
  
  penalty = wshelterM[Rank(bitscanf(pos->occ[WP] & FMASK_D))];
  penalty += wshelterL[Rank(bitscanf(pos->occ[WP] & FMASK_C))];
  penalty += wshelterR[Rank(bitscanf(pos->occ[WP] & FMASK_E))];
  penalty += bstormM[Rank(bitscanf(pos->occ[BP] & FMASK_D))];
  penalty += bstormL[Rank(bitscanf(pos->occ[BP] & FMASK_C))];
  penalty += bstormR[Rank(bitscanf(pos->occ[BP] & FMASK_E))];
  p->wd1 = (uint8)penalty;
  
  //black:
  penalty  = bshelterM[Rank(bitscanr(pos->occ[BP] & FMASK_G))];
  penalty += bshelterL[Rank(bitscanr(pos->occ[BP] & FMASK_F))];
  penalty += bshelterR[Rank(bitscanr(pos->occ[BP] & FMASK_H))];
  penalty += wstormM[Rank(bitscanr(pos->occ[WP] & FMASK_G))];
  penalty += wstormL[Rank(bitscanr(pos->occ[WP] & FMASK_F))];
  penalty += wstormR[Rank(bitscanr(pos->occ[WP] & FMASK_H))];
  p->bg8 = (uint8)penalty;
  
  penalty  = bshelterM[Rank(bitscanr(pos->occ[BP] & FMASK_B))];
  penalty += bshelterL[Rank(bitscanr(pos->occ[BP] & FMASK_C))];
  penalty += bshelterR[Rank(bitscanr(pos->occ[BP] & FMASK_A))];
  penalty += wstormM[Rank(bitscanr(pos->occ[WP] & FMASK_B))];
  penalty += wstormL[Rank(bitscanr(pos->occ[WP] & FMASK_C))];
  penalty += wstormR[Rank(bitscanr(pos->occ[WP] & FMASK_A))];
  p->bb8 = (uint8)penalty;
  
  penalty  = bshelterM[Rank(bitscanr(pos->occ[BP] & FMASK_E))];
  penalty += bshelterL[Rank(bitscanr(pos->occ[BP] & FMASK_D))];
  penalty += bshelterR[Rank(bitscanr(pos->occ[BP] & FMASK_F))];
  penalty += wstormM[Rank(bitscanr(pos->occ[WP] & FMASK_E))];
  penalty += wstormL[Rank(bitscanr(pos->occ[WP] & FMASK_D))];
  penalty += wstormR[Rank(bitscanr(pos->occ[WP] & FMASK_F))];
  p->be8 = (uint8)penalty;
  
  penalty  = bshelterM[Rank(bitscanr(pos->occ[BP] & FMASK_D))];
  penalty += bshelterL[Rank(bitscanr(pos->occ[BP] & FMASK_E))];
  penalty += bshelterR[Rank(bitscanr(pos->occ[BP] & FMASK_C))];
  penalty += wstormM[Rank(bitscanr(pos->occ[WP] & FMASK_D))];
  penalty += wstormL[Rank(bitscanr(pos->occ[WP] & FMASK_E))];
  penalty += wstormR[Rank(bitscanr(pos->occ[WP] & FMASK_C))];
  p->bd8 = (uint8)penalty;
}

pawn_entry_t *eval_pawn_struct()
{
  int sq, atk_sq, file, rank;
  bitboard_t t, sq_mask, front, wpawns, bpawns;
  bool passed, backward, doubled, isolated, connected;
  pawn_entry_t *p = pnt + (pos->phash & PMASK);
  
  if(p->pkey == pos->phash) return (p);
  
  p->scoremg = 0;
  p->scoreeg = 0;
  p->passers = 0ULL;
  p->pkey = pos->phash;
  
  wpawns = pos->occ[WP];
  bpawns = pos->occ[BP];
  
  //storing pawn attacks:
  p->attacks[W]  = ((pos->occ[WP] & ~FMASK_A) << 7);
  p->attacks[W] |= ((pos->occ[WP] & ~FMASK_H) << 9);
  p->attacks[B]  = ((pos->occ[BP] & ~FMASK_A) >> 9);
  p->attacks[B] |= ((pos->occ[BP] & ~FMASK_H) >> 7);

  t = pos->occ[WP];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    file = File(sq);
    rank = Rank(sq);
  
    //psq_values:
    p->scoremg += psq_pawn_mg[W][sq];
    
    //collecting the pawn type info:
    doubled   = (true && (wpawns & rook_backward[B][sq]));
    isolated  = (true && (!(wpawns & isolated_mask[file])));
    connected = (true && (wpawns & connected_mask[W][sq]));
    passed    = (!doubled && (!(bpawns & passer_mask[W][sq])));
    backward  = (!isolated && !connected && !passed && (!(wpawns & backward_mask[W][sq])));
  
    //if pawn can advance (not blocked by any other pawn), 
    //but cannot advance safely, it's backward:
    if(backward)
    { front = rook_backward[B][sq];
      backward = false;
      while(front)
      { atk_sq = bitscanf(front);
        sq_mask = (1ULL << atk_sq);
        if((sq_mask & (wpawns | bpawns)) 
        || (sq_mask & p->attacks[W]))
          break;
        if(sq_mask & p->attacks[B])
        { backward = true;
          break;
        }
        bitclear(front, atk_sq);
      }
    }
    //penalties and bonuses:
    if(backward)
    { p->scoremg -= backward_penalty_mg[file];
      p->scoreeg -= backward_penalty_eg[file];
      if(!(bpawns & rook_backward[B][sq]))
        p->scoremg -= backward_on_semi_op_mg;
       
      if(((1ULL << (sq+8)) & p->attacks[B])
      && ((1ULL << (sq+8)) & boutposts))
        p->scoremg -= weak_square_mg;
    }
    if(doubled)
    { p->scoremg -= doubled_penalty_mg[file];
      p->scoreeg -= doubled_penalty_eg[file];
    }
    if(isolated)
    { p->scoremg -= isolated_penalty_mg[file];
      p->scoreeg -= isolated_penalty_eg[file];
      if(!(bpawns & rook_backward[B][sq]))
        p->scoremg -= isolated_on_semi_op_mg;
    }
    if(connected)
    { p->scoremg += pawn_chain_element_mg[file];
      p->scoreeg += pawn_chain_element_eg[file];
    }
    
    if(passed)
    { p->passers |= (1ULL << sq);
      p->scoremg += passer_bonus_mg[W][rank];
     if(connected) p->scoremg += connected_passer_mg[W][rank];
    }
    else
    { if(!(bpawns & rook_backward[B][sq]))
      { if((popcnt(wpawns & backward_mask[W][sq])) >= \
           (popcnt(bpawns & backward_mask[B][sq+8]))){
              p->scoremg += candidate_mg[W][rank];
              p->scoreeg += candidate_eg[W][rank];
        }
      }
    }
  }
  
  t = pos->occ[BP];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    file = File(sq);
    rank = Rank(sq);

    //psq_values:
    p->scoremg -= psq_pawn_mg[B][sq];
    
    //collecting the pawn type info:
    doubled   = (true && (bpawns & rook_backward[W][sq]));
    isolated  = (true && (!(bpawns & isolated_mask[file])));
    connected = (true && (bpawns & connected_mask[B][sq]));
    passed    = (!doubled && (!(wpawns & passer_mask[B][sq])));
    backward  = (!isolated && !connected && !passed && (!(bpawns & backward_mask[B][sq])));
    
    if(backward)
    { front = rook_backward[W][sq];
      backward = false;
      while(front)
      { atk_sq = bitscanr(front);
        sq_mask = (1ULL << atk_sq);
        if((sq_mask & (wpawns | bpawns)) 
        || (sq_mask & p->attacks[B]))
          break;
        if(sq_mask & p->attacks[W])
        { backward = true;
          break;
        }
        bitclear(front, atk_sq);
      }
    }
    //penalties and bonuses:        
    if(backward)
    { p->scoremg += backward_penalty_mg[file];
      p->scoreeg += backward_penalty_eg[file];
      if(!(wpawns & rook_backward[W][sq]))
        p->scoremg += backward_on_semi_op_mg;
        
      if(((1ULL << (sq-8)) & p->attacks[W])
      && ((1ULL << (sq-8)) & woutposts))
        p->scoremg += weak_square_mg;
    }
    if(doubled)
    { p->scoremg += doubled_penalty_mg[file];
      p->scoreeg += doubled_penalty_eg[file];
    }

    if(isolated)
    { p->scoremg += isolated_penalty_mg[file];
      p->scoreeg += isolated_penalty_eg[file];
      if(!(wpawns & rook_backward[W][sq]))
        p->scoremg += isolated_on_semi_op_mg;
    }
    if(connected)
    { p->scoremg -= pawn_chain_element_mg[file];
      p->scoreeg -= pawn_chain_element_eg[file];
    }
    if(passed)
    { p->passers |= (1ULL << sq);
      p->scoremg -= passer_bonus_mg[B][rank];
      if(connected) p->scoremg -= connected_passer_mg[B][rank];
    }
    else
    { if(!(wpawns & rook_backward[W][sq]))
      { if((popcnt(bpawns & backward_mask[B][sq])) >= \
           (popcnt(wpawns & backward_mask[W][sq-8]))){
              p->scoremg -= candidate_mg[B][rank];
              p->scoreeg -= candidate_eg[B][rank];
        }
      }
    }
  }
  
  //saving the pawn shield/storms:
  pawn_shield_store(p);
  return (p);
}

void eval_passers(int *score, bitboard_t passers, bitboard_t watk_map, bitboard_t batk_map, bitboard_t occ)
{
  int sq, result, rank;
  bitboard_t t, front, back;
  bitboard_t wmajors = pos->occ[WR]|pos->occ[WQ];
  bitboard_t bmajors = pos->occ[BR]|pos->occ[BQ];
  bool attacked;
  
  t = pos->occ[WP] & passers;
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    rank  = Rank(sq);
    front = rook_backward[B][sq];
    back  = rook_backward[W][sq];
    attacked = false;
    
    //basic bonuses by rank:
    result = passer_bonus_eg[W][rank];
    if(pos->occ[WP] & connected_mask[W][sq])
      result += connected_passer_eg[W][rank];
    
    //general case - don't put your major piece in front:
    if(front & pos->occ[WR]) result -= rook_in_front[W][rank];
    if(front & pos->occ[WQ]) result -= queen_in_front[W][rank];
    
    //if not immediately blocked, try to see whether it's supported:
    if(pos->square[sq + 8] == EMPTY)
    {  
      //don't block your own pawn's path:
      if(!(front & pos->occ[OCC_W])) result += clear_path[W][rank];
      
      if(bmajors & back)//is it attacked from behind?:
      { if(Rank(bitscanr(occ & back)) <=/*!*/ Rank(bitscanr(bmajors & back)))
          attacked = true;
      }
      
      //attacked/blocked passer with or without own support:
      if(!attacked && (!(front & (batk_map | pos->occ[OCC_B]))))
      { if(front & watk_map) 
          result += free_with_support[W][rank];
        else result += free_passer[W][rank];
      }
      else
      { //all squares to queening covered?:
        if((front & watk_map) == front)
          result += blocked_with_support[W][rank];
        else result += blocked_passer[W][rank];
      }
    }

    //king activity:
    result += (king_activity[distance[pos->ksq[W]][sq+8]] - 
               king_activity[distance[pos->ksq[B]][sq+8]]);
    
    if(result > 0)
    { if((File(sq) != FILE_A) && (File(sq) != FILE_H))
      { if(pos->occ[BQ] | pos->occ[BR]) result -= (result / 4);
        else if(!pos->pcount[BB]) result += (result / 4);
      }
      *(score) += result;
    }
  }
  
  
  t = pos->occ[BP] & passers;
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    rank = Rank(sq);
    front = rook_backward[W][sq];
    back  = rook_backward[B][sq];
    attacked = false;
    
    result = passer_bonus_eg[B][rank];
    if(pos->occ[BP] & connected_mask[B][sq])
      result += connected_passer_eg[B][rank];
    
    if(front & pos->occ[BR]) result -= rook_in_front[B][rank];
    if(front & pos->occ[BQ]) result -= queen_in_front[B][rank];
    
    if(pos->square[sq - 8] == EMPTY)
    {
      if(!(front & pos->occ[OCC_B])) result += clear_path[B][rank];
      if(wmajors & back)
      {
        if(Rank(bitscanf(occ & back)) >=/*!*/ Rank(bitscanf(wmajors & back)))
          attacked = true;
      }
      if(!attacked && (!(front & (watk_map | pos->occ[OCC_W]))))
      {
        if(front & batk_map)
          result += free_with_support[B][rank];
        else result += free_passer[B][rank];
      }
      else
      { if((front & batk_map) == front)
          result += blocked_with_support[B][rank];
        else result += blocked_passer[B][rank];
      }
    }

    result += (king_activity[distance[pos->ksq[B]][sq-8]] - 
               king_activity[distance[pos->ksq[W]][sq-8]]);
    
    if(result > 0)
    { if((File(sq) != FILE_A) && (File(sq) != FILE_H))
      { if(pos->occ[WQ] | pos->occ[WR]) result -= (result / 4);
        else if(!pos->pcount[WB]) result += (result / 4);
      }
      *(score) -= result;
    }
  }
}

//pawn shield:
static int wcurrent(pawn_entry_t *pt, int sq)
{ int file = File(sq);
  if(file > FILE_E) return pt->wg1;
  else if(file < FILE_D) return pt->wb1;
  else if(file == FILE_E) return pt->we1;
  return pt->wd1;
}

static int bcurrent(pawn_entry_t *pt, int sq)
{ int file = File(sq);
  if(file > FILE_E) return pt->bg8;
  else if(file < FILE_D) return pt->bb8;
  else if(file == FILE_E) return pt->be8;
  return pt->bd8;
}

void eval_shield(pawn_entry_t *p, int *scoremg)
{ int current, min, penalty;
  uint8 castle;
  if(pos->pcount[BQ])
  { min = INF;
    current = wcurrent(p, pos->ksq[W]);
    castle = pos->castle & ~(BLACK_OO|BLACK_OOO);
    if(castle  & WHITE_OO) min = p->wg1;
    if((castle & WHITE_OOO) && (min > p->wb1)) min = p->wb1;    
    if(castle && (min < current)) penalty = (min + current) / 2;
    else penalty = current;
    *(scoremg) -= penalty;
  }
  if(pos->pcount[WQ])
  { min = INF;
    current = bcurrent(p, pos->ksq[B]);
    castle = pos->castle & ~(WHITE_OO|WHITE_OOO);
    if(castle  & BLACK_OO) min = p->bg8;
    if((castle & BLACK_OOO) && (min > p->bb8)) min = p->bb8;
    if(castle && (min < current)) penalty = (min + current) / 2;
    else penalty = current;
    *(scoremg) += penalty;
  }
}
