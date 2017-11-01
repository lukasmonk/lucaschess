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

//values:
static const int rook_on_7th_mg = 12;
static const int rook_on_7th_eg = 20;
static const int queen_on_7th_mg = 10;
static const int queen_on_7th_eg = 15;
static const int rook_opened_mg = 5;
static const int rook_opened_eg = 8;
static const int rook_semiopened_mg = 4;
static const int rook_semiopened_eg = 6;
static const int trapped_rook_mg = 50;
static const int trapped_rook_eg = 50;
static const int trapped_bishop_mg = 80;
static const int trapped_bishop_eg = 100;
static const int rooks_doubled_open_mg = 8;
static const int rooks_doubled_open_eg = 6;
static const int rooks_doubled_semi_mg = 6;
static const int rooks_doubled_semi_eg = 5;
static const int queen_early_development = 40;
static const int lights_undeveloped = 8;
static const int bishop_pair_value_mg = 50;
static const int bishop_pair_value_eg = 50;

//mobility increment values:
static const int qmobinc_mg = 3;
static const int rmobinc_mg = 1;
static const int bmobinc_mg = 3;
static const int nmobinc_mg = 4;
static const int qmobinc_eg = 2;
static const int rmobinc_eg = 3;
static const int bmobinc_eg = 3;
static const int nmobinc_eg = 2;

//threats:                        //  P   N   B   R   Q
static const int pthreat_mg[6] = {0,  0, 12, 12, 18, 20};
static const int pthreat_eg[6] = {0,  0, 20, 20, 30, 40};
static const int nthreat_mg[6] = {0,  3,  0,  6, 10, 15};
static const int nthreat_eg[6] = {0, 10,  0, 12, 20, 30};
static const int bthreat_mg[6] = {0,  3,  6,  0, 10, 15};
static const int bthreat_eg[6] = {0, 10, 12,  0, 20, 30};
static const int rthreat_mg[6] = {0,  0,  4,  4,  0,  8};
static const int rthreat_eg[6] = {0,  8, 12, 12,  0, 16};
static const int qthreat_mg[6] = {0,  4,  4,  4,  4,  0};
static const int qthreat_eg[6] = {0,  8,  8,  8,  8,  0};

#ifdef EVAL_WEIGHTS
//weights:
static int weight_material_mg = 100;
static int weight_material_eg = 100;
static int weight_pawn_structure_mg = 100;
static int weight_pawn_structure_eg = 100;
static int weight_mobility_mg = 100;
static int weight_mobility_eg = 100;
static int weight_psq_tables_mg = 100;
static int weight_psq_tables_eg = 100;
static int weight_outposts_mg = 100;
static int weight_outposts_eg = 100;
static int weight_threats_mg = 100;
static int weight_threats_eg = 100;
static int weight_development = 100;
static int weight_bishop_pair_mg = 100;
static int weight_bishop_pair_eg = 100;
static int weight_placement_mg = 100;
static int weight_placement_eg = 100;
static int weight_king_safety = 100;
static int weight_passed_pawns_mg = 100;
static int weight_passed_pawns_eg = 100;
#endif

static int interpolation(int scoremg, int scoreeg, int wmul, int bmul, int phase)
{ int v;
  v = ((scoremg * phase) + (scoreeg * (24 - phase))) / 24;
  if(v > 0 && wmul != 16) v = (v * wmul) / 16;
  else if(v < 0 && bmul != 16) v = (v * bmul) / 16;
  return((pos->side == W) ? (v) : (-v));
}

static int eval_score(eval_info_t *e, int wmul, int bmul, int phase)
{
  int scoremg, scoreeg;
  #ifdef EVAL_WEIGHTS
  //applying weights:
  e->material_mg = (e->material_mg * weight_material_mg) / 100;
  e->material_eg = (e->material_eg * weight_material_eg) / 100;
  e->pawn_structure_mg = (e->pawn_structure_mg * weight_pawn_structure_mg) / 100;
  e->pawn_structure_eg = (e->pawn_structure_eg * weight_pawn_structure_eg) / 100;
  e->mobility_mg = (e->mobility_mg * weight_mobility_mg) / 100;
  e->mobility_eg = (e->mobility_eg * weight_mobility_eg) / 100;
  e->psq_tables_mg = (e->psq_tables_mg * weight_psq_tables_mg) / 100;
  e->psq_tables_eg = (e->psq_tables_eg * weight_psq_tables_eg) / 100;
  e->outposts_mg = (e->outposts_mg * weight_outposts_mg) / 100;
  e->outposts_eg = (e->outposts_eg * weight_outposts_eg) / 100;
  e->threats_mg = (e->threats_mg * weight_threats_mg) / 100;
  e->threats_eg = (e->threats_eg * weight_threats_eg) / 100;
  e->development_mg = (e->development_mg * weight_development) / 100;
  e->bishop_pair_mg = (e->bishop_pair_mg * weight_bishop_pair_mg) / 100;
  e->bishop_pair_eg = (e->bishop_pair_eg * weight_bishop_pair_eg) / 100;
  e->placement_mg = (e->placement_mg * weight_placement_mg) / 100;
  e->placement_eg = (e->placement_eg * weight_placement_eg) / 100;
  e->king_safety_mg = (e->king_safety_mg * weight_king_safety) / 100;
  e->passed_pawns_mg = (e->passed_pawns_mg * weight_passed_pawns_mg) / 100;
  e->passed_pawns_eg = (e->passed_pawns_eg * weight_passed_pawns_eg) / 100;
  #endif
  //summary:
  scoremg = 
    (e->material_mg + e->pawn_structure_mg + e->mobility_mg + \
     e->psq_tables_mg + e->outposts_mg + e->threats_mg + \
     e->development_mg + e->bishop_pair_mg + e->placement_mg + e->king_safety_mg + e->passed_pawns_mg);
  
  scoreeg = 
    (e->material_eg + e->pawn_structure_eg + e->mobility_eg + e->psq_tables_eg + \
     e->outposts_eg + e->threats_eg + e->bishop_pair_eg + e->placement_eg + e->passed_pawns_eg);

  return(interpolation(scoremg, scoreeg, wmul, bmul, phase));
}

int eval()
{
  int x, sq, scoremg, scoreeg;
  bitboard_t t, m, bm, rm, occ;
  eval_info_t *pei;
  pawn_entry_t *pt;
  material_entry_t *mt;
  int wdeveloped = 0, bdeveloped = 0;
  int wksq = pos->ksq[W], bksq = pos->ksq[B];
  int wlights = pos->pcount[WB] + pos->pcount[WN];
  int blights = pos->pcount[BB] + pos->pcount[BN];
  bitboard_t atk_map[2];
  int wmul, bmul;

  pei = &ei;
  pei->material_mg = 0;
  pei->material_eg = 0;
  pei->pawn_structure_mg = 0;
  pei->pawn_structure_eg = 0;
  pei->mobility_mg = 0;
  pei->mobility_eg = 0;
  pei->psq_tables_mg = 0;
  pei->psq_tables_eg = 0;
  pei->outposts_mg = 0;
  pei->outposts_eg = 0;
  pei->threats_mg = 0;
  pei->threats_eg = 0;
  pei->development_mg = 0;
  pei->bishop_pair_mg = 0;
  pei->bishop_pair_eg = 0;
  pei->placement_mg = 0;
  pei->placement_eg = 0;
  pei->king_safety_mg = 0;
  pei->passed_pawns_mg = 0;
  pei->passed_pawns_eg = 0;

  //get material score and info:
  mt = eval_material();
  pei->material_mg = mt->scoremg;
  pei->material_eg = mt->scoreeg;
  //using local multipliers in case
  //of eventual alteration (exmpl: OPPC_BISHOPS)
  wmul = mt->wmul;
  bmul = mt->bmul;
  
  //special endgame:
  if(mt->flags & SPECIAL_EG)
  { scoremg = pei->material_mg;
    scoreeg = pei->material_eg;
    if(eval_endgame(mt, &scoremg, &scoreeg))
      return (interpolation(scoremg, scoreeg, wmul, bmul, mt->phase));
  }
  
  //evaluating pawn structure:
  pt = eval_pawn_struct();
  pei->pawn_structure_mg = pt->scoremg;
  pei->pawn_structure_eg = pt->scoreeg;
  atk_map[W] = pt->attacks[W];
  atk_map[B] = pt->attacks[B];

  //board full occupancy:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  
  //evaluate white pieces
  t = pos->occ[WN];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != B1) && (sq != G1)) wdeveloped++;
    pei->psq_tables_mg += psq_knight_mg[W][sq];
    pei->psq_tables_eg += psq_knight_eg[W][sq];
    
    //outposts:
    if(((1ULL << sq) & woutposts)
    && ((1ULL << sq) & pt->attacks[W])
    && (!((1ULL << sq) & pt->attacks[B])))
    { pei->outposts_mg += psq_outposts_mg[W][sq];
      pei->outposts_eg += psq_outposts_eg[W][sq];
    }
    
    //moves:
    m = n_moves[sq];
    atk_map[W] |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[BQ])
    { pei->threats_mg += nthreat_mg[QUEEN];
      pei->threats_eg += nthreat_eg[QUEEN];
    }
    if(m & pos->occ[BR])
    { pei->threats_mg += nthreat_mg[ROOK];
      pei->threats_eg += nthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { pei->threats_mg -= pthreat_mg[KNIGHT];
      pei->threats_eg -= pthreat_eg[KNIGHT];
    }
    
    //mobility:
    m &= ~pos->occ[OCC_W];
    x = popcnt(m);
    pei->mobility_mg += (x * nmobinc_mg);
    pei->mobility_eg += (x * nmobinc_eg);
    
    //general threats, excluding the protected by enemy pawns:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BB])
    { pei->threats_mg += nthreat_mg[BISHOP];
      pei->threats_eg += nthreat_eg[BISHOP];
    }
    if(m & pos->occ[BP])
    { pei->threats_mg += nthreat_mg[PAWN];
      pei->threats_eg += nthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[WB];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != C1) && (sq != F1)) wdeveloped++;
    pei->psq_tables_mg += psq_bishop_mg[W][sq];
    pei->psq_tables_eg += psq_bishop_eg[W][sq];
    
    //outposts:
    if(((1ULL << sq) & woutposts)
    && ((1ULL << sq) & pt->attacks[W])
    && (!((1ULL << sq) & pt->attacks[B])))
    { pei->outposts_mg += (psq_outposts_mg[W][sq]/2);
      pei->outposts_eg += (psq_outposts_eg[W][sq]/2);
    }
    
    //trapped on 7th:
    if((sq == A7) && (pos->square[B6] == BP))
    { if(see_squares(A7, B6) < 0) 
      { pei->placement_mg -= trapped_bishop_mg;
        pei->placement_eg -= trapped_bishop_eg;
      }
    }
    if((sq == H7) && (pos->square[G6] == BP))
    { if(see_squares(H7, G6) < 0) 
      { pei->placement_mg -= trapped_bishop_mg;
        pei->placement_eg -= trapped_bishop_eg;
      }
    }
    //trapped on 6th:
    if((sq == A6) && (pos->square[B5] == BP))
    { if(see_squares(A6, B5) < 0) 
      { pei->placement_mg -= (trapped_bishop_mg / 2);
        pei->placement_eg -= (trapped_bishop_eg / 2);
      }
    }
    if((sq == H6) && (pos->square[G5] == BP))
    { if(see_squares(H6, G5) < 0) 
      { pei->placement_mg -= (trapped_bishop_mg / 2);
        pei->placement_eg -= (trapped_bishop_eg / 2);
      }
    }
    
    //moves:
    m = (bmoves(sq, occ));
    atk_map[W] |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[BQ])
    { pei->threats_mg += bthreat_mg[QUEEN];
      pei->threats_eg += bthreat_eg[QUEEN];
    }
    if(m & pos->occ[BR])
    { pei->threats_mg += bthreat_mg[ROOK];
      pei->threats_eg += bthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { pei->threats_mg -= pthreat_mg[BISHOP];
      pei->threats_eg -= pthreat_eg[BISHOP];
    }
    
    //mobility:
    m &= ~pos->occ[OCC_W];
    x = popcnt(m);
    pei->mobility_mg += (x * bmobinc_mg);
    pei->mobility_eg += (x * bmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BN])
    { pei->threats_mg += bthreat_mg[KNIGHT];
      pei->threats_eg += bthreat_eg[KNIGHT];
    }
    if(m & pos->occ[BP])
    { pei->threats_mg += bthreat_mg[PAWN];
      pei->threats_eg += bthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[WR];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    pei->psq_tables_mg += psq_rook_mg[W][sq];
    pei->psq_tables_eg += psq_rook_eg[W][sq];
    
    if((Rank(sq) == RANK_7) 
    &&((Rank(bksq) == RANK_8) || (pos->occ[BP] & RMASK_7)))
    { pei->placement_mg += rook_on_7th_mg;
      pei->placement_eg += rook_on_7th_eg;
    }
    
    //moves:
    m = (rmoves(sq, occ));
    atk_map[W] |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[BQ])
    { pei->threats_mg += rthreat_mg[QUEEN];
      pei->threats_eg += rthreat_eg[QUEEN];
    }
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { pei->threats_mg -= pthreat_mg[ROOK];
      pei->threats_eg -= pthreat_eg[ROOK];
    }
    
    //update the moves map:
    m &= ~pos->occ[OCC_W];

    //trapped by own king:
    if(distance[wksq][sq] < 3)
    { if(File(wksq) > FILE_E && File(wksq) < File(sq))
      { if(popcnt(m) <= 5)
        { pei->placement_mg -= trapped_rook_mg;
          pei->placement_eg -= trapped_rook_eg;
        }
      }
      else if(File(wksq) < FILE_E && File(wksq) > File(sq))
      { if(popcnt(m) <= 5)
        { pei->placement_mg -= trapped_rook_mg;
          pei->placement_eg -= trapped_rook_eg;
        }
      }
    }
    
    //mobility:
    x = popcnt(m);
    pei->mobility_mg += (x * rmobinc_mg);
    pei->mobility_eg += (x * rmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BN])
    { pei->threats_mg += rthreat_mg[KNIGHT];
      pei->threats_eg += rthreat_eg[KNIGHT];
    }
    if(m & pos->occ[BB])
    { pei->threats_mg += rthreat_mg[BISHOP];
      pei->threats_eg += rthreat_eg[BISHOP];
    }
    if(m & pos->occ[BP])
    { pei->threats_mg += rthreat_mg[PAWN];
      pei->threats_eg += rthreat_eg[PAWN];
    }
    
    //rook on opened/semiopened file:
    if(!(pos->occ[WP] & file_mask[File(sq)]))
    { if(!(pos->occ[BP] & file_mask[File(sq)]))
      { pei->placement_mg += rook_opened_mg;
        pei->placement_eg += rook_opened_eg;
        if((rook_backward[W][sq] & pos->occ[WR])
        || (rook_backward[W][sq] & pos->occ[WQ]))
        { pei->placement_mg += rooks_doubled_open_mg;
          pei->placement_eg += rooks_doubled_open_eg;
        }
      }
      else
      { pei->placement_mg += rook_semiopened_mg;
        pei->placement_eg += rook_semiopened_eg;
        if((rook_backward[W][sq] & pos->occ[WR])
        || (rook_backward[W][sq] & pos->occ[WQ]))
        { pei->placement_mg += rooks_doubled_semi_mg;
          pei->placement_eg += rooks_doubled_semi_eg;
        }
      }
    }
  }
  
  t = pos->occ[WQ];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != D1) && (mt->phase >= 20) && (wdeveloped < wlights))
      pei->development_mg -= queen_early_development;
    
    pei->psq_tables_mg += psq_queen_mg[W][sq];
    pei->psq_tables_eg += psq_queen_eg[W][sq];
    
    if((Rank(sq) == RANK_7) 
    &&((Rank(bksq) == RANK_8) || (pos->occ[BP] & RMASK_7)))
    { pei->placement_mg += queen_on_7th_mg;
      pei->placement_eg += queen_on_7th_eg;
    }
    
    //moves:
    bm = bmoves(sq, occ);
    rm = rmoves(sq, occ);
    m = bm | rm;
    atk_map[W] |= m;
    
    //lva threats:
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { pei->threats_mg -= pthreat_mg[QUEEN];
      pei->threats_eg -= pthreat_eg[QUEEN];
    }
    
    //mobility:
    m &= ~pos->occ[OCC_W];
    x = popcnt(m);
    pei->mobility_mg += (x * qmobinc_mg);
    pei->mobility_eg += (x * qmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BR])
    { pei->threats_mg += qthreat_mg[ROOK];
      pei->threats_eg += qthreat_eg[ROOK];
    }
    if(m & pos->occ[BN])
    { pei->threats_mg += qthreat_mg[KNIGHT];
      pei->threats_eg += qthreat_eg[KNIGHT];
    }
    if(m & pos->occ[BB])
    { pei->threats_mg += qthreat_mg[BISHOP];
      pei->threats_eg += qthreat_eg[BISHOP];
    }
    if(m & pos->occ[BP])
    { pei->threats_mg += qthreat_mg[PAWN];
      pei->threats_eg += qthreat_eg[PAWN];
    }
  }

  //evaluate black pieces
  t = pos->occ[BN];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != G8) && (sq != B8)) bdeveloped++;
    pei->psq_tables_mg -= psq_knight_mg[B][sq];
    pei->psq_tables_eg -= psq_knight_eg[B][sq];
    
    //outposts:
    if(((1ULL << sq) & boutposts)
    && ((1ULL << sq) & pt->attacks[B])
    && (!((1ULL << sq) & pt->attacks[W])))
    { pei->outposts_mg -= psq_outposts_mg[B][sq];
      pei->outposts_eg -= psq_outposts_eg[B][sq];
    }
    
    //moves:
    m = n_moves[sq];
    atk_map[B] |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[WQ])
    { pei->threats_mg -= nthreat_mg[QUEEN];
      pei->threats_eg -= nthreat_eg[QUEEN];
    }
    if(m & pos->occ[WR])
    { pei->threats_mg -= nthreat_mg[ROOK];
      pei->threats_eg -= nthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { pei->threats_mg += pthreat_mg[KNIGHT];
      pei->threats_eg += pthreat_eg[KNIGHT];
    }
    
    //mobility:
    m &= ~pos->occ[OCC_B];
    x = popcnt(m);
    pei->mobility_mg -= (x * nmobinc_mg);
    pei->mobility_eg -= (x * nmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WB])
    { pei->threats_mg -= nthreat_mg[BISHOP];
      pei->threats_eg -= nthreat_eg[BISHOP];
    }
    if(m & pos->occ[WP])
    { pei->threats_mg -= nthreat_mg[PAWN];
      pei->threats_eg -= nthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[BB];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != F8) && (sq != C8)) bdeveloped++;
    pei->psq_tables_mg -= psq_bishop_mg[B][sq];
    pei->psq_tables_eg -= psq_bishop_eg[B][sq];
    
    //outposts - twice as low than a knight outpost:
    if(((1ULL << sq) & boutposts)
    && ((1ULL << sq) & pt->attacks[B])
    && (!((1ULL << sq) & pt->attacks[W])))
    { pei->outposts_mg -= (psq_outposts_mg[B][sq]/2);
      pei->outposts_eg -= (psq_outposts_eg[B][sq]/2);
    }
    
    //trapped on 2nd:
    if((sq == A2) && (pos->square[B3] == WP))
    { if(see_squares(A2, B3) < 0) 
      { pei->placement_mg += trapped_bishop_mg;
        pei->placement_eg += trapped_bishop_eg;
      }
    }
    if((sq == H2) && (pos->square[G3] == WP))
    { if(see_squares(H2, G3) < 0) 
      { pei->placement_mg += trapped_bishop_mg;
        pei->placement_eg += trapped_bishop_eg;
      }
    }
    //trapped on 6th:
    if((sq == A3) && (pos->square[B4] == WP))
    { if(see_squares(A3, B4) < 0) 
      { pei->placement_mg += (trapped_bishop_mg / 2);
        pei->placement_eg += (trapped_bishop_eg / 2);
      }
    }
    if((sq == H3) && (pos->square[G4] == WP))
    { if(see_squares(H3, G4) < 0) 
      { pei->placement_mg += (trapped_bishop_mg / 2);
        pei->placement_eg += (trapped_bishop_eg / 2);
      }
    }
    
    //moves:
    m = (bmoves(sq, occ));
    atk_map[B] |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[WQ])
    { pei->threats_mg -= bthreat_mg[QUEEN];
      pei->threats_eg -= bthreat_eg[QUEEN];
    }
    if(m & pos->occ[WR])
    { pei->threats_mg -= bthreat_mg[ROOK];
      pei->threats_eg -= bthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { pei->threats_mg += pthreat_mg[BISHOP];
      pei->threats_eg += pthreat_eg[BISHOP];
    }
    
    //mobility:
    m &= ~pos->occ[OCC_B];
    x = popcnt(m);
    pei->mobility_mg -= (x * bmobinc_mg);
    pei->mobility_eg -= (x * bmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WN])
    { pei->threats_mg -= bthreat_mg[KNIGHT];
      pei->threats_eg -= bthreat_eg[KNIGHT];
    }
    if(m & pos->occ[WP])
    { pei->threats_mg -= bthreat_mg[PAWN];
      pei->threats_eg -= bthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[BR];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    pei->psq_tables_mg -= psq_rook_mg[B][sq];
    pei->psq_tables_eg -= psq_rook_eg[B][sq];
    
    if((Rank(sq) == RANK_2)
    &&((Rank(wksq) == RANK_1) || (pos->occ[WP] & RMASK_2)))
    { pei->placement_mg -= rook_on_7th_mg;
      pei->placement_eg -= rook_on_7th_eg;
    }
    
    //moves:
    m = (rmoves(sq, occ));
    atk_map[B] |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[WQ])
    { pei->threats_mg -= rthreat_mg[QUEEN];
      pei->threats_eg -= rthreat_eg[QUEEN];
    }
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { pei->threats_mg += pthreat_mg[ROOK];
      pei->threats_eg += pthreat_eg[ROOK];
    }
    
    //update the moves map:
    m &= ~pos->occ[OCC_B];
    
    //trapped by own king:
    if(distance[bksq][sq] < 3)
    { if(File(bksq) > FILE_E && File(bksq) < File(sq))
      { if(popcnt(m) <= 5)
        { pei->placement_mg += trapped_rook_mg;
          pei->placement_eg += trapped_rook_eg;
        }
      }
      else if(File(bksq) < FILE_E && File(bksq) > File(sq))
      { if(popcnt(m) <= 5)
        { pei->placement_mg += trapped_rook_mg;
          pei->placement_eg += trapped_rook_eg;
        }
      }
    }
    
    //mobility:
    x = popcnt(m);
    pei->mobility_mg -= (x * rmobinc_mg);
    pei->mobility_eg -= (x * rmobinc_eg);
  
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WB])
    { pei->threats_mg -= rthreat_mg[BISHOP];
      pei->threats_eg -= rthreat_eg[BISHOP];
    }
    if(m & pos->occ[WN])
    { pei->threats_mg -= rthreat_mg[KNIGHT];
      pei->threats_eg -= rthreat_eg[KNIGHT];
    }
    if(m & pos->occ[WP])
    { pei->threats_mg -= rthreat_mg[PAWN];
      pei->threats_eg -= rthreat_eg[PAWN];
    }
    
    //rook on opened/semiopened file:
    if(!(pos->occ[BP] & file_mask[File(sq)]))
    { if(!(pos->occ[WP] & file_mask[File(sq)]))
      { pei->placement_mg -= rook_opened_mg;
        pei->placement_eg -= rook_opened_eg;
        if((rook_backward[B][sq] & pos->occ[BR])
        || (rook_backward[B][sq] & pos->occ[BQ]))
        { pei->placement_mg -= rooks_doubled_open_mg;
          pei->placement_eg -= rooks_doubled_open_eg;
        }
      }
      else
      { pei->placement_mg -= rook_semiopened_mg;
        pei->placement_eg -= rook_semiopened_eg;
        if((rook_backward[B][sq] & pos->occ[BR])
        || (rook_backward[B][sq] & pos->occ[BQ]))
        { pei->placement_mg -= rooks_doubled_semi_mg;
          pei->placement_eg -= rooks_doubled_semi_eg;
        }
      }
    }
  }
  
  t = pos->occ[BQ];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != D8) && (mt->phase >= 20) && (bdeveloped < blights))
      pei->development_mg += queen_early_development;
    
    pei->psq_tables_mg -= psq_queen_mg[B][sq];
    pei->psq_tables_eg -= psq_queen_eg[B][sq];
    
    if((Rank(sq) == RANK_2)
    &&((Rank(wksq) == RANK_1) || (pos->occ[WP] & RMASK_2)))
    { pei->placement_mg -= queen_on_7th_mg;
      pei->placement_eg -= queen_on_7th_eg;
    }
    
    //moves:
    bm = bmoves(sq, occ);
    rm = rmoves(sq, occ);
    m =  bm | rm;
    atk_map[B] |= m;
    
    //lva threats:
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { pei->threats_mg += pthreat_mg[QUEEN];
      pei->threats_eg += pthreat_eg[QUEEN];
    }
    
    //mobility:
    m &= ~pos->occ[OCC_B];
    x = popcnt(m);
    pei->mobility_mg -= (x * qmobinc_mg);
    pei->mobility_eg -= (x * qmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WR])
    { pei->threats_mg -= qthreat_mg[ROOK];
      pei->threats_eg -= qthreat_eg[ROOK];
    }
    if(m & pos->occ[WB])
    { pei->threats_mg -= qthreat_mg[BISHOP];
      pei->threats_eg -= qthreat_eg[BISHOP];
    }
    if(m & pos->occ[WN])
    { pei->threats_mg -= qthreat_mg[KNIGHT];
      pei->threats_eg -= qthreat_eg[KNIGHT];
    }
    if(m & pos->occ[WP])
    { pei->threats_mg -= qthreat_mg[PAWN];
      pei->threats_eg -= qthreat_eg[PAWN];
    }
  }
 
  //bishop pair scoring:
  if(mt->flags & TRY_BISHOP_PAIR_W)
  { if((pos->occ[WB] & light_squares) && (pos->occ[WB] & dark_squares))
    { pei->bishop_pair_mg += (bishop_pair_value_mg - (pos->pcount[WP]+pos->pcount[BP]));
      pei->bishop_pair_eg += (bishop_pair_value_eg - (pos->pcount[WP]+pos->pcount[BP]));
    }
  }
  if(mt->flags & TRY_BISHOP_PAIR_B)
  { if((pos->occ[BB] & light_squares) && (pos->occ[BB] & dark_squares))
    { pei->bishop_pair_mg -= (bishop_pair_value_mg - (pos->pcount[WP]+pos->pcount[BP]));
      pei->bishop_pair_eg -= (bishop_pair_value_eg - (pos->pcount[WP]+pos->pcount[BP]));
    }
  }
  
  //opposite colored bishops:
  if(mt->flags == TRY_OPPC_BISHOPS)
  { if(((pos->occ[WB] & light_squares) && (pos->occ[BB] & dark_squares))
    || ((pos->occ[WB] & dark_squares)  && (pos->occ[BB] & light_squares)))
    { wmul = 8;
      bmul = 8;
    }
  }
  
  ///kings:
  pei->psq_tables_mg += (psq_king_mg[W][wksq] - psq_king_mg[B][bksq]);
  pei->psq_tables_eg += (psq_king_eg[W][wksq] - psq_king_eg[B][bksq]);
  
  //opening/midgame:
  if(mt->phase >= 10)
  { 
    //king safety:
    pei->king_safety_mg -= evaluate_king_safety(mt->phase, wksq, WHITE, atk_map);
    pei->king_safety_mg += evaluate_king_safety(mt->phase, bksq, BLACK, atk_map);
    
    //development of the light pieces:
    pei->development_mg -= (wlights - wdeveloped) * lights_undeveloped;
    pei->development_mg += (blights - bdeveloped) * lights_undeveloped;
  }

  //passed pawns:
  atk_map[W] |= k_moves[wksq];
  atk_map[B] |= k_moves[bksq];
  if(pt->passers) eval_passers(&pei->passed_pawns_mg, &pei->passed_pawns_eg, pt->passers, atk_map[W], atk_map[B], occ);
  
  //final results:
  return (eval_score(pei, wmul, bmul, mt->phase));
}
