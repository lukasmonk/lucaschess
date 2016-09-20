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

//mobility increment values:
static const int qmobinc_mg = 3;
static const int rmobinc_mg = 1;
static const int bmobinc_mg = 3;
static const int nmobinc_mg = 4;
static const int qmobinc_eg = 2;
static const int rmobinc_eg = 3;
static const int bmobinc_eg = 3;
static const int nmobinc_eg = 2;

//attackers towards enemy king:
static const int qatk = 60;
static const int natk = 30;
static const int ratk = 25;
static const int batk = 20;
static const int atk_scale[16] =
{0, 0, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};

//threats:                  //  P   N   B   R   Q
static int pthreat_mg[6] = {0,  0, 12, 12, 18, 20};
static int pthreat_eg[6] = {0,  0, 20, 20, 30, 40};
static int nthreat_mg[6] = {0,  3,  0,  6, 10, 15};
static int nthreat_eg[6] = {0, 10,  0, 12, 20, 30};
static int bthreat_mg[6] = {0,  3,  6,  0, 10, 15};
static int bthreat_eg[6] = {0, 10, 12,  0, 20, 30};
static int rthreat_mg[6] = {0,  0,  4,  4,  0,  8};
static int rthreat_eg[6] = {0,  8, 12, 12,  0, 16};
static int qthreat_mg[6] = {0,  4,  4,  4,  4,  0};
static int qthreat_eg[6] = {0,  8,  8,  8,  8,  0};


int eval()
{
  int x, sq, score, scoremg, scoreeg;
  bitboard_t t, m, bm, rm, occ;
  pawn_entry_t *pt;
  material_entry_t *mt;
  bitboard_t wsafety, bsafety;
  int wattackers, battackers;
  int w_atk_value = 0, b_atk_value = 0;
  int wdevelopment = 0, bdevelopment = 0;
  int wksq = pos->ksq[W], bksq = pos->ksq[B];
  int wlights = pos->pcount[WB] + pos->pcount[WN];
  int blights = pos->pcount[BB] + pos->pcount[BN];
  bitboard_t watk_map, batk_map;

  //get material score and info:
  mt = eval_material();
  scoremg = mt->scoremg;
  scoreeg = mt->scoreeg;
  
  //endgame:
  if(mt->flags & SPECIAL_EG)
  { if(eval_endgame(mt, &scoremg, &scoreeg))
    { score = ((scoremg * mt->phase) + (24 - mt->phase) * scoreeg) / 24;
      if((score > 0) && (mt->wmul != 16)) score = (score * mt->wmul) / 16;
      else if((score < 0) && (mt->bmul != 16)) score = (score * mt->bmul) / 16;
      if(pos->side == W) return (score);
      return (-score);
    }
  }
  
  //preparing king safety data - surrounding squares:
  wsafety = (1ULL << wksq) | k_moves[wksq];
  bsafety = (1ULL << bksq) | k_moves[bksq];
  watk_map = k_moves[wksq];
  batk_map = k_moves[bksq];
  
  //evaluating pawn structure:
  pt = eval_pawn_struct();
  scoremg += pt->scoremg;
  scoreeg += pt->scoreeg;
  wattackers = popcnt(pt->attacks[W] & bsafety);
  battackers = popcnt(pt->attacks[B] & wsafety);
  watk_map |= pt->attacks[W];
  batk_map |= pt->attacks[B];
  
  //evaluate kings - placement, pawn shield:
  eval_shield(pt, &scoremg);
  scoremg += (psq_king_mg[W][wksq] - psq_king_mg[B][bksq]);
  scoreeg += (psq_king_eg[W][wksq] - psq_king_eg[B][bksq]);
  if(k_moves[wksq] & bsafety) wattackers++;
  if(k_moves[bksq] & wsafety) battackers++;

  //board full occupancy:
  occ = pos->occ[OCC_W] | pos->occ[OCC_B];
  
  //evaluate white pieces
  t = pos->occ[WN];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != B1) && (sq != G1)) wdevelopment++;
    scoremg += psq_knight_mg[W][sq];
    scoreeg += psq_knight_eg[W][sq];
    
    //outposts:
    if(((1ULL << sq) & woutposts)
    && ((1ULL << sq) & pt->attacks[W])
    && (!((1ULL << sq) & pt->attacks[B])))
    { scoremg += psq_outposts_mg[W][sq];
      scoreeg += psq_outposts_eg[W][sq];
    }
    
    //moves:
    m = n_moves[sq];
    watk_map |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[BQ])
    { scoremg += nthreat_mg[QUEEN];
      scoreeg += nthreat_eg[QUEEN];
    }
    if(m & pos->occ[BR])
    { scoremg += nthreat_mg[ROOK];
      scoreeg += nthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { scoremg -= pthreat_mg[KNIGHT];
      scoreeg -= pthreat_eg[KNIGHT];
    }
    
    //enemy king attack:
    if(m & bsafety)
    { wattackers++;
      w_atk_value += natk;
    }
    
    //mobility:
    m &= ~pos->occ[OCC_W];
    x = popcnt(m);
    scoremg += (x * nmobinc_mg);
    scoreeg += (x * nmobinc_eg);
    
    //general threats, excluding the protected by enemy pawns:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BB])
    { scoremg += nthreat_mg[BISHOP];
      scoreeg += nthreat_eg[BISHOP];
    }
    if(m & pos->occ[BP])
    { scoremg += nthreat_mg[PAWN];
      scoreeg += nthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[WB];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != C1) && (sq != F1)) wdevelopment++;
    scoremg += psq_bishop_mg[W][sq];
    scoreeg += psq_bishop_eg[W][sq];
    
    //outposts:
    if(((1ULL << sq) & woutposts)
    && ((1ULL << sq) & pt->attacks[W])
    && (!((1ULL << sq) & pt->attacks[B])))
    { scoremg += (psq_outposts_mg[W][sq]/2);
      scoreeg += (psq_outposts_eg[W][sq]/2);
    }
    
    //trapped on 7th:
    if((sq == A7) && (pos->square[B6] == BP))
    { if(see_squares(A7, B6) < 0) 
      { scoremg -= trapped_bishop_mg;
        scoreeg -= trapped_bishop_eg;
      }
    }
    if((sq == H7) && (pos->square[G6] == BP))
    { if(see_squares(H7, G6) < 0) 
      { scoremg -= trapped_bishop_mg;
        scoreeg -= trapped_bishop_eg;
      }
    }
    //trapped on 6th:
    if((sq == A6) && (pos->square[B5] == BP))
    { if(see_squares(A6, B5) < 0) 
      { scoremg -= (trapped_bishop_mg / 2);
        scoreeg -= (trapped_bishop_eg / 2);
      }
    }
    if((sq == H6) && (pos->square[G5] == BP))
    { if(see_squares(H6, G5) < 0) 
      { scoremg -= (trapped_bishop_mg / 2);
        scoreeg -= (trapped_bishop_eg / 2);
      }
    }
    
    //moves:
    m = (bmoves(sq, occ));
    watk_map |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[BQ])
    { scoremg += bthreat_mg[QUEEN];
      scoreeg += bthreat_eg[QUEEN];
    }
    if(m & pos->occ[BR])
    { scoremg += bthreat_mg[ROOK];
      scoreeg += bthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { scoremg -= pthreat_mg[BISHOP];
      scoreeg -= pthreat_eg[BISHOP];
    }
  
    //enemy king attack:
    if(m & bsafety)
    { wattackers++;
      w_atk_value += batk;
    }
    
    //mobility:
    m &= ~pos->occ[OCC_W];
    x = popcnt(m);
    scoremg += (x * bmobinc_mg);
    scoreeg += (x * bmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BN])
    { scoremg += bthreat_mg[KNIGHT];
      scoreeg += bthreat_eg[KNIGHT];
    }
    if(m & pos->occ[BP])
    { scoremg += bthreat_mg[PAWN];
      scoreeg += bthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[WR];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    scoremg += psq_rook_mg[W][sq];
    scoreeg += psq_rook_eg[W][sq];
    
    if((Rank(sq) == RANK_7) 
    &&((Rank(bksq) == RANK_8) || (pos->occ[BP] & RMASK_7)))
    { scoremg += rook_on_7th_mg;
      scoreeg += rook_on_7th_eg;
    }
    
    //moves:
    m = (rmoves(sq, occ));
    watk_map |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[BQ])
    { scoremg += rthreat_mg[QUEEN];
      scoreeg += rthreat_eg[QUEEN];
    }
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { scoremg -= pthreat_mg[ROOK];
      scoreeg -= pthreat_eg[ROOK];
    }

    //enemy king attack:
    if(m & bsafety) 
    { wattackers++;
      w_atk_value += ratk;
    }
    
    //update the moves map:
    m &= ~pos->occ[OCC_W];

    //trapped by own king:
    if(distance[wksq][sq] < 3)
    { if(File(wksq) > FILE_E && File(wksq) < File(sq))
      { if(popcnt(m) <= 5)
        { scoremg -= trapped_rook_mg;
          scoreeg -= trapped_rook_eg;
        }
      }
      else if(File(wksq) < FILE_E && File(wksq) > File(sq))
      { if(popcnt(m) <= 5)
        { scoremg -= trapped_rook_mg;
          scoreeg -= trapped_rook_eg;
        }
      }
    }
    
    //mobility:
    x = popcnt(m);
    scoremg += (x * rmobinc_mg);
    scoreeg += (x * rmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BN])
    { scoremg += rthreat_mg[KNIGHT];
      scoreeg += rthreat_eg[KNIGHT];
    }
    if(m & pos->occ[BB])
    { scoremg += rthreat_mg[BISHOP];
      scoreeg += rthreat_eg[BISHOP];
    }
    if(m & pos->occ[BP])
    { scoremg += rthreat_mg[PAWN];
      scoreeg += rthreat_eg[PAWN];
    }
    
    //rook on opened/semiopened file:
    if(!(pos->occ[WP] & file_mask[File(sq)]))
    { if(!(pos->occ[BP] & file_mask[File(sq)]))
      { scoremg += rook_opened_mg;
        scoreeg += rook_opened_eg;
        if((rook_backward[W][sq] & pos->occ[WR])
        || (rook_backward[W][sq] & pos->occ[WQ]))
        { scoremg += rooks_doubled_open_mg;
          scoreeg += rooks_doubled_open_eg;
        }
      }
      else
      { scoremg += rook_semiopened_mg;
        scoreeg += rook_semiopened_eg;
        if((rook_backward[W][sq] & pos->occ[WR])
        || (rook_backward[W][sq] & pos->occ[WQ]))
        { scoremg += rooks_doubled_semi_mg;
          scoreeg += rooks_doubled_semi_eg;
        }
      }
    }
  }
  
  t = pos->occ[WQ];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != D1) && (mt->phase >= 20) && (wdevelopment < wlights))
      scoremg -= queen_early_development;
    
    scoremg += psq_queen_mg[W][sq];
    scoreeg += psq_queen_eg[W][sq];
    
    if((Rank(sq) == RANK_7) 
    &&((Rank(bksq) == RANK_8) || (pos->occ[BP] & RMASK_7)))
    { scoremg += queen_on_7th_mg;
      scoreeg += queen_on_7th_eg;
    }
    
    //moves:
    bm = bmoves(sq, occ);
    rm = rmoves(sq, occ);
    m = bm | rm;
    watk_map |= m;
    
    //lva threats:
    //contrariwise pawn threat:
    if(pt->attacks[B] & (1ULL << sq))
    { scoremg -= pthreat_mg[QUEEN];
      scoreeg -= pthreat_eg[QUEEN];
    }

    //attack towards enemy king:
    if(m & bsafety) 
    { wattackers++;
      w_atk_value += qatk;
    }
    
    //mobility:
    m &= ~pos->occ[OCC_W];
    x = popcnt(m);
    scoremg += (x * qmobinc_mg);
    scoreeg += (x * qmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[B];
    if(m & pos->occ[BR])
    { scoremg += qthreat_mg[ROOK];
      scoreeg += qthreat_eg[ROOK];
    }
    if(m & pos->occ[BN])
    { scoremg += qthreat_mg[KNIGHT];
      scoreeg += qthreat_eg[KNIGHT];
    }
    if(m & pos->occ[BB])
    { scoremg += qthreat_mg[BISHOP];
      scoreeg += qthreat_eg[BISHOP];
    }
    if(m & pos->occ[BP])
    { scoremg += qthreat_mg[PAWN];
      scoreeg += qthreat_eg[PAWN];
    }
  }

  //evaluate black pieces
  t = pos->occ[BN];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != G8) && (sq != B8)) bdevelopment++;
    scoremg -= psq_knight_mg[B][sq];
    scoreeg -= psq_knight_eg[B][sq];
    
    //outposts:
    if(((1ULL << sq) & boutposts)
    && ((1ULL << sq) & pt->attacks[B])
    && (!((1ULL << sq) & pt->attacks[W])))
    { scoremg -= psq_outposts_mg[B][sq];
      scoreeg -= psq_outposts_eg[B][sq];
    }
    
    //moves:
    m = n_moves[sq];
    batk_map |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[WQ])
    { scoremg -= nthreat_mg[QUEEN];
      scoreeg -= nthreat_eg[QUEEN];
    }
    if(m & pos->occ[WR])
    { scoremg -= nthreat_mg[ROOK];
      scoreeg -= nthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { scoremg += pthreat_mg[KNIGHT];
      scoreeg += pthreat_eg[KNIGHT];
    }
   
    //enemy king attack:
    if(m & wsafety)
    { battackers++;
      b_atk_value += natk;
    }
    
    //mobility:
    m &= ~pos->occ[OCC_B];
    x = popcnt(m);
    scoremg -= (x * nmobinc_mg);
    scoreeg -= (x * nmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WB])
    { scoremg -= nthreat_mg[BISHOP];
      scoreeg -= nthreat_eg[BISHOP];
    }
    if(m & pos->occ[WP])
    { scoremg -= nthreat_mg[PAWN];
      scoreeg -= nthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[BB];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != F8) && (sq != C8)) bdevelopment++;
    scoremg -= psq_bishop_mg[B][sq];
    scoreeg -= psq_bishop_eg[B][sq];
    
    //outposts - twice as low than a knight outpost:
    if(((1ULL << sq) & boutposts)
    && ((1ULL << sq) & pt->attacks[B])
    && (!((1ULL << sq) & pt->attacks[W])))
    { scoremg -= (psq_outposts_mg[B][sq]/2);
      scoreeg -= (psq_outposts_eg[B][sq]/2);
    }
    
    //trapped on 2nd:
    if((sq == A2) && (pos->square[B3] == WP))
    { if(see_squares(A2, B3) < 0) 
      { scoremg += trapped_bishop_mg;
        scoreeg += trapped_bishop_eg;
      }
    }
    if((sq == H2) && (pos->square[G3] == WP))
    { if(see_squares(H2, G3) < 0) 
      { scoremg += trapped_bishop_mg;
        scoreeg += trapped_bishop_eg;
      }
    }
    //trapped on 6th:
    if((sq == A3) && (pos->square[B4] == WP))
    { if(see_squares(A3, B4) < 0) 
      { scoremg += (trapped_bishop_mg / 2);
        scoreeg += (trapped_bishop_eg / 2);
      }
    }
    if((sq == H3) && (pos->square[G4] == WP))
    { if(see_squares(H3, G4) < 0) 
      { scoremg += (trapped_bishop_mg / 2);
        scoreeg += (trapped_bishop_eg / 2);
      }
    }
    
    //moves:
    m = (bmoves(sq, occ));
    batk_map |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[WQ])
    { scoremg -= bthreat_mg[QUEEN];
      scoreeg -= bthreat_eg[QUEEN];
    }
    if(m & pos->occ[WR])
    { scoremg -= bthreat_mg[ROOK];
      scoreeg -= bthreat_eg[ROOK];
    }
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { scoremg += pthreat_mg[BISHOP];
      scoreeg += pthreat_eg[BISHOP];
    }
      
    //enemy king attack:
    if(m & wsafety)
    { battackers++;
      b_atk_value += batk;
    }
    
    //mobility:
    m &= ~pos->occ[OCC_B];
    x = popcnt(m);
    scoremg -= (x * bmobinc_mg);
    scoreeg -= (x * bmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WN])
    { scoremg -= bthreat_mg[KNIGHT];
      scoreeg -= bthreat_eg[KNIGHT];
    }
    if(m & pos->occ[WP])
    { scoremg -= bthreat_mg[PAWN];
      scoreeg -= bthreat_eg[PAWN];
    }
  }
  
  t = pos->occ[BR];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    scoremg -= psq_rook_mg[B][sq];
    scoreeg -= psq_rook_eg[B][sq];
    
    if((Rank(sq) == RANK_2)
    &&((Rank(wksq) == RANK_1) || (pos->occ[WP] & RMASK_2)))
    { scoremg -= rook_on_7th_mg;
      scoreeg -= rook_on_7th_eg;
    }
    
    //moves:
    m = (rmoves(sq, occ));
    batk_map |= m;
    
    //lva type of threats - 
    //it doesn't need to test for a pawn protector:
    if(m & pos->occ[WQ])
    { scoremg -= rthreat_mg[QUEEN];
      scoreeg -= rthreat_eg[QUEEN];
    }
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { scoremg += pthreat_mg[ROOK];
      scoreeg += pthreat_eg[ROOK];
    }
  
    //enemy king attack:
    if(m & wsafety) 
    { battackers++;
      b_atk_value += ratk;
    }
    
    //update the moves map:
    m &= ~pos->occ[OCC_B];
    
    //trapped by own king:
    if(distance[bksq][sq] < 3)
    { if(File(bksq) > FILE_E && File(bksq) < File(sq))
      { if(popcnt(m) <= 5)
        { scoremg += trapped_rook_mg;
          scoreeg += trapped_rook_eg;
        }
      }
      else if(File(bksq) < FILE_E && File(bksq) > File(sq))
      { if(popcnt(m) <= 5)
        { scoremg += trapped_rook_mg;
          scoreeg += trapped_rook_eg;
        }
      }
    }
    
    //mobility:
    x = popcnt(m);
    scoremg -= (x * rmobinc_mg);
    scoreeg -= (x * rmobinc_eg);
  
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WB])
    { scoremg -= rthreat_mg[BISHOP];
      scoreeg -= rthreat_eg[BISHOP];
    }
    if(m & pos->occ[WN])
    { scoremg -= rthreat_mg[KNIGHT];
      scoreeg -= rthreat_eg[KNIGHT];
    }
    if(m & pos->occ[WP])
    { scoremg -= rthreat_mg[PAWN];
      scoreeg -= rthreat_eg[PAWN];
    }
    
    //rook on opened/semiopened file:
    if(!(pos->occ[BP] & file_mask[File(sq)]))
    { if(!(pos->occ[WP] & file_mask[File(sq)]))
      { scoremg -= rook_opened_mg;
        scoreeg -= rook_opened_eg;
        if((rook_backward[B][sq] & pos->occ[BR])
        || (rook_backward[B][sq] & pos->occ[BQ]))
        { scoremg -= rooks_doubled_open_mg;
          scoreeg -= rooks_doubled_open_eg;
        }
      }
      else
      { scoremg -= rook_semiopened_mg;
        scoreeg -= rook_semiopened_eg;
        if((rook_backward[B][sq] & pos->occ[BR])
        || (rook_backward[B][sq] & pos->occ[BQ]))
        { scoremg -= rooks_doubled_semi_mg;
          scoreeg -= rooks_doubled_semi_eg;
        }
      }
    }
  }
  
  t = pos->occ[BQ];
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    
    if((sq != D8) && (mt->phase >= 20) && (bdevelopment < blights))
      scoremg += queen_early_development;
    
    scoremg -= psq_queen_mg[B][sq];
    scoreeg -= psq_queen_eg[B][sq];
    
    if((Rank(sq) == RANK_2)
    &&((Rank(wksq) == RANK_1) || (pos->occ[WP] & RMASK_2)))
    { scoremg -= queen_on_7th_mg;
      scoreeg -= queen_on_7th_eg;
    }
    
    //moves:
    bm = bmoves(sq, occ);
    rm = rmoves(sq, occ);
    m =  bm | rm;
    batk_map |= m;
    
    //lva threats:
    //contrariwise pawn threat:
    if(pt->attacks[W] & (1ULL << sq))
    { scoremg += pthreat_mg[QUEEN];
      scoreeg += pthreat_eg[QUEEN];
    }
    
    //attack towards enemy king:
    if(m & wsafety) 
    { battackers++;
      b_atk_value += qatk;
    }
    
    //mobility:
    m &= ~pos->occ[OCC_B];
    x = popcnt(m);
    scoremg -= (x * qmobinc_mg);
    scoreeg -= (x * qmobinc_eg);
    
    //general threats:
    m &= ~pt->attacks[W];
    if(m & pos->occ[WR])
    { scoremg -= qthreat_mg[ROOK];
      scoreeg -= qthreat_eg[ROOK];
    }
    if(m & pos->occ[WB])
    { scoremg -= qthreat_mg[BISHOP];
      scoreeg -= qthreat_eg[BISHOP];
    }
    if(m & pos->occ[WN])
    { scoremg -= qthreat_mg[KNIGHT];
      scoreeg -= qthreat_eg[KNIGHT];
    }
    if(m & pos->occ[WP])
    { scoremg -= qthreat_mg[PAWN];
      scoreeg -= qthreat_eg[PAWN];
    }
  }
  
  //bishop pair scoring:
  if(mt->flags & TRY_BISHOP_PAIR)
  { if(mt->flags & TRY_BISHOP_PAIR_W)
    { if((pos->occ[WB] & light_squares)
      && (pos->occ[WB] & dark_squares))
      { scoremg += 50 - (pos->pcount[WP]+pos->pcount[BP]);
        scoreeg += 50 - (pos->pcount[WP]+pos->pcount[BP]);
      }
    }
    if(mt->flags & TRY_BISHOP_PAIR_B)
    { if((pos->occ[BB] & light_squares)
      && (pos->occ[BB] & dark_squares))
      { scoremg -= 50 - (pos->pcount[WP]+pos->pcount[BP]);
        scoreeg -= 50 - (pos->pcount[WP]+pos->pcount[BP]);
      }
    }
  }
  
  //opposite colored bishops:
  if(mt->flags & TRY_OPPC_BISHOPS)
  { if(((pos->occ[WB] & light_squares) && (pos->occ[BB] & dark_squares))
    || ((pos->occ[WB] & dark_squares)  && (pos->occ[BB] & light_squares)))
    { mt->wmul = 8;
      mt->bmul = 8;
    }
  }
  
  //king safety summary:
  if(w_atk_value)
  { if(wattackers > 15) wattackers = 15;
    x = (atk_scale[wattackers] * w_atk_value) / 8;
    if(!pos->pcount[WQ]) x /= 2;
    scoremg += x;
  }
  if(b_atk_value) 
  { if(battackers > 15) battackers = 15;
    x = (atk_scale[battackers] * b_atk_value) / 8;
    if(!pos->pcount[BQ]) x /= 2;
    scoremg -= x;
  }
  
  //development of the light pieces:
  scoremg -= (wlights - wdevelopment) * lights_undeveloped;
  scoremg += (blights - bdevelopment) * lights_undeveloped;

  //passed pawns:
  if(pt->passers) eval_passers(&scoreeg, pt->passers, watk_map, batk_map, occ);
  
  //final results:
  score = (scoremg * mt->phase + (24-mt->phase) * scoreeg) / 24;
  if((score > 0) && (mt->wmul != 16)) score = (score * mt->wmul) / 16; 
  else if((score < 0) && (mt->bmul != 16)) score = (score * mt->bmul) / 16;
  if(pos->side == W) return (score);
  return (-score);
}
  