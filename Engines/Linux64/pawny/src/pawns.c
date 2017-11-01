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

#define PT_ALIGNMENT 64
#define PNTSIZE (1 << 16)
#define PMASK ((PNTSIZE) - 1)

//pawn table pointer:
pawn_entry_t *pnt = 0;

static const int isolated_penalty_mg[8] = //by file
{12, 14, 14, 18, 18, 14, 14, 12};
static const int isolated_penalty_eg[8] =
{10, 12, 12, 14, 14, 12, 12, 10};

static const int backward_penalty_mg[8] = //by file
{10, 10, 10, 12, 12, 10, 10, 10};
static const int backward_penalty_eg[8] =
{8, 8, 8, 10, 10, 8, 8, 8};

static const int doubled_penalty_mg[8] = //by file
{8, 8, 8, 10, 10, 8, 8, 8};
static const int doubled_penalty_eg[8] =
{14, 15, 15, 16, 16, 15, 15, 14};

static const int candidate_mg[8] = //by rank
{0, 2, 4, 6, 12, 30, 0, 0};
static const int candidate_eg[8] = 
{0, 4, 8, 12, 24, 60, 0, 0};

static const int pawn_chain_element_mg[8] = //by file
{4, 4, 4, 5, 5, 4, 4, 4};
static const int pawn_chain_element_eg[8] =
{2, 2, 2, 4, 4, 2, 2, 2};

static const int weak_square_mg = 8;
static const int backward_on_semi_op_mg = 8;
static const int isolated_on_semi_op_mg = 8;

int pnt_init()
{ aligned_free((void *)pnt); 
  pnt = 0;
  pnt = (pawn_entry_t *)aligned_malloc((size_t)PNTSIZE*sizeof(pawn_entry_t), PT_ALIGNMENT);
  if(!pnt) return false;
  aligned_wipe_out((void *)pnt, (size_t)PNTSIZE*sizeof(pawn_entry_t), PT_ALIGNMENT);
  return true;
}

pawn_entry_t *eval_pawn_struct()
{
  int sq, atk_sq, file, rank;
  bitboard_t t, sq_mask, front, wpawns, bpawns;
  bool passed, backward, doubled, isolated, connected;
  pawn_entry_t *p = pnt + (pos->phash & PMASK);
  int scoremg, scoreeg;
  
  if(p->lock == pos->phash >> 32) return (p);
  
  scoremg = 0;
  scoreeg = 0;
  p->passers = 0ULL;
  p->lock = (uint32)(pos->phash >> 32);
  
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
    scoremg += psq_pawn_mg[W][sq];
    
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
    { scoremg -= backward_penalty_mg[file];
      scoreeg -= backward_penalty_eg[file];
      if(!(bpawns & rook_backward[B][sq]))
        scoremg -= backward_on_semi_op_mg;
       
      if(((1ULL << (sq+8)) & p->attacks[B])
      && ((1ULL << (sq+8)) & boutposts))
        scoremg -= weak_square_mg;
    }
    if(doubled)
    { scoremg -= doubled_penalty_mg[file];
      scoreeg -= doubled_penalty_eg[file];
    }
    if(isolated)
    { scoremg -= isolated_penalty_mg[file];
      scoreeg -= isolated_penalty_eg[file];
      if(!(bpawns & rook_backward[B][sq]))
        scoremg -= isolated_on_semi_op_mg;
    }
    if(connected)
    { scoremg += pawn_chain_element_mg[file];
      scoreeg += pawn_chain_element_eg[file];
    }
    
    if(passed) p->passers |= (1ULL << sq);
    
    else
    { if(!(bpawns & rook_backward[B][sq]))
      { if((popcnt(wpawns & backward_mask[W][sq])) >= \
           (popcnt(bpawns & backward_mask[B][sq+8]))){
              scoremg += candidate_mg[rank];
              scoreeg += candidate_eg[rank];
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
    rank = 7 - Rank(sq);

    //psq_values:
    scoremg -= psq_pawn_mg[B][sq];
    
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
    { scoremg += backward_penalty_mg[file];
      scoreeg += backward_penalty_eg[file];
      if(!(wpawns & rook_backward[W][sq]))
        scoremg += backward_on_semi_op_mg;
        
      if(((1ULL << (sq-8)) & p->attacks[W])
      && ((1ULL << (sq-8)) & woutposts))
        scoremg += weak_square_mg;
    }
    if(doubled)
    { scoremg += doubled_penalty_mg[file];
      scoreeg += doubled_penalty_eg[file];
    }

    if(isolated)
    { scoremg += isolated_penalty_mg[file];
      scoreeg += isolated_penalty_eg[file];
      if(!(wpawns & rook_backward[W][sq]))
        scoremg += isolated_on_semi_op_mg;
    }
    if(connected)
    { scoremg -= pawn_chain_element_mg[file];
      scoreeg -= pawn_chain_element_eg[file];
    }
    
    if(passed) p->passers |= (1ULL << sq);

    else
    { if(!(wpawns & rook_backward[W][sq]))
      { if((popcnt(bpawns & backward_mask[B][sq])) >= \
           (popcnt(wpawns & backward_mask[W][sq-8]))){
              scoremg -= candidate_mg[rank];
              scoreeg -= candidate_eg[rank];
        }
      }
    }
  }
  p->scoremg = (sint16)scoremg;
  p->scoreeg = (sint16)scoreeg;
  return (p);
}

//passed pawns evaluation:
static const int major_in_front = 4;
static const int clear_path = 6;
static const int free_passer = 16;
static const int supported = 6;
static const int coverage = 8;
static const int connected_passer = 4;
static const int king_activity[8] = {50, 30, 15, 5, 0, 0, 0, 0};
static const int bonus_rank[8] = {0, 0, 0, 8, 24, 48, 86, 0};
static const int unstoppable = 100;

void eval_passers(int *scoremg, int *scoreeg, bitboard_t passers, bitboard_t watk_map, bitboard_t batk_map, bitboard_t occ)
{
  int mg, eg, sq, queening_sq, rank;
  bitboard_t t, m, front, back;
  bitboard_t wmajors = pos->occ[WR]|pos->occ[WQ];
  bitboard_t bmajors = pos->occ[BR]|pos->occ[BQ];
  bool attacked;
  int pieces[2];
  
  pieces[W] = get_piece_count(W);
  pieces[B] = get_piece_count(B);
  
  t = pos->occ[WP] & passers;
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    rank  = Rank(sq);
    front = rook_backward[B][sq];
    back  = rook_backward[W][sq];
    attacked = false;
    queening_sq = bitscanr(file_mask[File(sq)]);
    
    //advancement bonuses:
    mg = bonus_rank[rank];
    eg = bonus_rank[rank] * 2;

    //connected passer:
    if(pos->occ[WP] & connected_mask[W][sq])
    { mg += connected_passer * rank;
      eg += connected_passer * rank * 2;
    }
    
    //Don't put your major piece in front in the Endgame:
    if(front & (pos->occ[WR]|pos->occ[WQ]))
      eg -= major_in_front * rank;
    
    //if not immediately blocked, see if it's supported:
    if(pos->square[sq + 8] == EMPTY)
    { 
      //check if the frontspan is clear:
      if(!(front & (pos->occ[OCC_W]|pos->occ[OCC_B])))
      { eg += clear_path * rank;
      
        //The opponent has no pieces left:
        if(pieces[B] == 0 && distance[sq][queening_sq] < distance[pos->ksq[B]][queening_sq])
        { if(distance[pos->ksq[B]][queening_sq] - distance[sq][queening_sq] > 1
          || pos->side == W)
          { eg += unstoppable;
            //In case both sides might have 'unstoppable pawns', 
            //double the bonus if the opponent's king might be in check after promotion:
            m = bmoves(pos->ksq[B], occ) | rmoves(pos->ksq[B], occ);
            if(m & (1ULL << queening_sq))
              eg += unstoppable;
          }
        }
      }
      
      //is it attacked from the back:
      if(bmajors & back)
      { if(Rank(bitscanr(occ & back)) <= Rank(bitscanr(bmajors & back)))
          attacked = true;
      }
      
      //uncontested:
      if(!attacked && !(front & (batk_map | pos->occ[OCC_B])))
        eg += free_passer * rank;
      else
      { //all squares to queening covered?:
        if((front & watk_map) == front)
          eg += coverage * rank;
        //if not, check if the immediate front square is covered:
        else if(watk_map & (1ULL << (sq + 8)))
          eg += supported * rank;
      }
    }
    //king activity - endgame:
    eg += (king_activity[distance[pos->ksq[W]][sq+8]] - 
           king_activity[distance[pos->ksq[B]][sq+8]]);
    
    *(scoremg) += mg;
    *(scoreeg) += eg;
  }
  
  
  t = pos->occ[BP] & passers;
  while(t)
  {
    sq = bitscanf(t);
    bitclear(t, sq);
    rank = 7 - Rank(sq); //relative rank
    front = rook_backward[W][sq];
    back  = rook_backward[B][sq];
    attacked = false;
    queening_sq = bitscanf(file_mask[File(sq)]);
    
    //advancement bonuses:
    mg = bonus_rank[rank];
    eg = bonus_rank[rank] * 2;

    //connected passer:
    if(pos->occ[BP] & connected_mask[B][sq])
    { mg += connected_passer * rank;
      eg += connected_passer * rank * 2;
    }
    
    //general case - don't put your major piece in front:
    if(front & (pos->occ[BR]|pos->occ[BQ]))
      eg -= major_in_front * rank;
    
    //if not immediately blocked, see if it's supported:
    if(pos->square[sq - 8] == EMPTY)
    { 
      //check if the frontspan is clear:
      if(!(front & (pos->occ[OCC_W]|pos->occ[OCC_B])))
      { eg += clear_path * rank;
      
        //The opponent has no pieces left:
        if(pieces[W] == 0 && distance[sq][queening_sq] < distance[pos->ksq[W]][queening_sq])
        { if(distance[pos->ksq[W]][queening_sq] - distance[sq][queening_sq] > 1
          || pos->side == B)
          { eg += unstoppable;
            //In case both sides might have 'unstoppable pawns', 
            //double the bonus if the opponent's king might be in check after promotion:
            m = bmoves(pos->ksq[W], occ) | rmoves(pos->ksq[W], occ);
            if(m & (1ULL << queening_sq))
              eg += unstoppable;
          }
        }
      }
      
      //is it attacked from the back:
      if(wmajors & back)
      { if(Rank(bitscanf(occ & back)) >= Rank(bitscanf(wmajors & back)))
          attacked = true;
      }
      
      //uncontested:
      if(!attacked && !(front & (watk_map | pos->occ[OCC_W])))
        eg += free_passer * rank;
      else
      { //all squares to queening covered?:
        if((front & batk_map) == front)
          eg += coverage * rank;
        //if not, check if the immediate front square is covered:
        else if(batk_map & (1ULL << (sq - 8)))
          eg += supported * rank;
      }
    }
    //king activity - endgame:
    eg += (king_activity[distance[pos->ksq[B]][sq-8]] - 
           king_activity[distance[pos->ksq[W]][sq-8]]);
    
    *(scoremg) -= mg;
    *(scoreeg) -= eg;
  }
}

int passer_push(move_t m) //after it's made
{ if(pos->square[m.to] == WP && Rank(m.to) >= RANK_6
  && !(pos->occ[BP] & passer_mask[W][m.to]))
    return 1;
  if(pos->square[m.to] == BP && Rank(m.to) <= RANK_3
  && !(pos->occ[WP] & passer_mask[B][m.to]))
    return 1;
  return 0;
}
