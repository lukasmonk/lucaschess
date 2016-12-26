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
#include "bits.h"

#define PT_ALIGNMENT 64
#define PAWN_CHAIN_ELEMENT 4
#define PAWN_CHAIN_ELEMENT_EG 5
#define BACKWARD_ON_SEMI_OP 2
#define BACKWARD_ON_SEMI_OP_EG 3
#define PROTECTED 10
#define PROTECTED_EG 15
#define CONNECTED 18
#define CONNECTED_EG 25
#define PNTSIZE (1 << 16) //pt entries (2mb)
#define PMASK ((PNTSIZE) - 1)

//pawn table entry. (32 bytes of size)
typedef struct pte_t_ 
{ short score[2];
  short score_eg[2];
  bitboard_t attacks[2];
  hashkey_t pkey;
}pte_t;

//pawn table pointer:
pte_t *pnt = 0;

//global attacks:
bitboard_t pawn_attacks[2];

//king near passed pawn bonus:
const int king_activity[8] = 
{0, 50, 40, 30, 20, 10, 8, 0};

//passer advancement:
const int passer_bonus[2][8] = 
{{0, 5, 10, 15, 20, 25, 30, 0},
{0, 30, 25, 20, 15, 10, 5, 0}};

const int passer_bonus_eg[2][8] = 
{{0, 5, 10, 20, 30, 40, 50, 0},
{0, 50, 40, 30, 20, 10, 5, 0}};

const int isolated_penalty[8] = 
{0, 6, 8, 10, 10, 8, 6, 0};

const int isolated_penalty_eg[8] = 
{0, 8, 10, 12, 12, 10, 8, 0};

const int doubled_penalty[8] =
{0, 8, 10, 12, 12, 10, 8, 0};

const int doubled_penalty_eg[8] =
{0, 10, 12, 15, 15, 12, 10, 0};

const int backwards_penalty[8] =
{0, 4, 6, 8, 8, 6, 4, 0};

const int backwards_penalty_eg[8] =
{0, 6, 8, 10, 10, 8, 6, 0};

const int hanging_penalty[8] =
{0, 1, 2, 3, 3, 2, 1, 0};

const int hanging_penalty_eg[8] =
{0, 2, 3, 4, 4, 3, 2, 0};

const int candidate[2][8] = 
{{0, 5, 10, 12, 15, 20, 0, 0},
{0, 0, 20, 15, 12, 10, 5, 0}};

const int candidate_eg[2][8] = 
{{0, 5, 10, 15, 18, 20, 0, 0},
{0, 0, 20, 18, 15, 10, 5, 0}};

void __inline pnt_free() {aligned_free((void *)pnt); pnt = 0;}
void __inline pnt_clear(){aligned_wipe_out((void *)pnt, (size_t)PNTSIZE*sizeof(pte_t), PT_ALIGNMENT);}

//initializing the pawn hash table
int pnt_init()
{ pnt_free();	
  pnt = (pte_t *)aligned_malloc((size_t)PNTSIZE*sizeof(pte_t), PT_ALIGNMENT);
  if(!pnt) return false;
  pnt_clear();
  return true;
}

static __inline bool is_passed(int color, int sq)
{ if(board->bb_pawns[color^1] & passer_mask[color][rsz[sq]])
    return false;
  return true;
}

static __inline bool is_isolated(int color, int sq)
{ if(board->bb_pawns[color] & isolated_mask[calc_file(sq)])
    return false;
  return true;
}

static __inline int eval_white_activity(int p_sq)
{ int sq, score = 0;
  //adding score for king near that pawn
  //and penalty for opking being near:
  score += king_activity[distance_table[King_Square(W)][p_sq]];
  score -= king_activity[distance_table[King_Square(B)][p_sq]];
  //is it backed up by a rook?:
  for(sq = PLS(WR); sq <= H8; sq = PLN(sq))
  { if(((calc_file(sq)) == (calc_file(p_sq))) && (sq < p_sq))
      score += 15;
  }
  return score;
}

static __inline int eval_black_activity(int p_sq)
{ int sq, score = 0;
  //adding score for king near that pawn
  //and penalty for opking being near:
  score += king_activity[distance_table[King_Square(B)][p_sq]];
  score -= king_activity[distance_table[King_Square(W)][p_sq]];
  //is it backed up by a rook?:
  for(sq = PLS(BR); sq <= H8; sq = PLN(sq))
  { if(((calc_file(sq)) == (calc_file(p_sq))) && (sq > p_sq))
      score += 15;
  }
  return score;
}

static __inline void eval_white_pawn(int sq, short *t_score, short *t_score_eg)
{
  int score, score_eg;
  int back = 0;
  int file = calc_file(sq);
  int rank = calc_rank(sq);
  bool passer = false;
  int atk_sq, atk = 0, def = 0;
  bitboard_t left,right;
  
  //psq
  score = psq_table[WP][sq];
  score_eg = psq_table[WP][sq];
  
  //mark if passed pawn:
  if(is_passed(W, sq)) 
  { score += passer_bonus[W][rank];
    score_eg += passer_bonus_eg[W][rank];
    passer = true;
  }
  
  //doubled pawn:
  if(calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[file])) < rank)
  { score -= doubled_penalty[rank];
    score_eg -= doubled_penalty_eg[rank];
  }
  
  if(file == FILE_A)
  { 
    //pawn chain element/proteced passer:
    if(PieceType(sq - 15) == WP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    //isolated pawn:
    if(is_isolated(W,sq)) 
    { score -= isolated_penalty[rank];
      score_eg -= isolated_penalty_eg[rank];
    }
    else
    { 
      //backward pawn:
      right = board->bb_pawns[W] & file_mask[file + 1];
      if(calc_rank64(bitscanf(right)) > rank)
      { score -= backwards_penalty[rank];
        score_eg -= backwards_penalty_eg[rank];
      }
      
      if(passer)//connected passer
      { if(is_passed(W, rsu[bitscanf(right)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
      }
      else//candidate passed pawn:
      { if(!(board->bb_pawns[B] & (pawn_front[W][rsz[sq]] & file_mask[FILE_A])))
        { atk_sq = rsu[bitscanf(pawn_attacks[B] & (pawn_front[W][rsz[sq]] & file_mask[FILE_A]))];
          if(atk_sq && (PieceType(atk_sq - 15) == WP))
          { score += candidate[W][rank];
            score_eg += candidate_eg[W][rank];
          }
        }
      }
    }
  }
  
  else if(file == FILE_H)
  { 
    //pawn chain element/proteced passer:
    if(PieceType(sq - 17) == WP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    
    //isolated pawn:
    if(is_isolated(W,sq))
    { score -= isolated_penalty[rank];
      score_eg -= isolated_penalty_eg[rank];
    }
    else
    { 
      //backward pawn:
      left = board->bb_pawns[W] & file_mask[file - 1];
      if(calc_rank64(bitscanf(left)) > rank)
      { score -= backwards_penalty[rank];
        score_eg -= backwards_penalty_eg[rank];
      }
      
      if(passer)//connected passer
      { if(is_passed(W, rsu[bitscanf(left)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
      }
      else//candidate passed pawn:
      { if(!(board->bb_pawns[B] & (pawn_front[W][rsz[sq]] & file_mask[FILE_H])))
        { atk_sq = rsu[bitscanf(pawn_attacks[B] & (pawn_front[W][rsz[sq]] & file_mask[FILE_H]))];
          if(atk_sq && (PieceType(atk_sq - 17) == WP))
          { score += candidate[W][rank];
            score_eg += candidate_eg[W][rank];
          }
        }
      }
    }
  }
  
  else //not a rook pawn:
  { 
    //pawn chain element/proteced passer:
    if(PieceType(sq - 15) == WP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    if(PieceType(sq - 17) == WP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
   
    //isolated pawn:
    if(is_isolated(W,sq)) 
    { score -= isolated_penalty[rank];
      score_eg -= isolated_penalty_eg[rank];
    }
    else
    { 
      left  = board->bb_pawns[W] & file_mask[file - 1];
      right = board->bb_pawns[W] & file_mask[file + 1];
      
      if(passer)//connected passer
      { if(left && is_passed(W, rsu[bitscanf(left)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
        if(right && is_passed(W, rsu[bitscanf(right)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
      }
      else//candidate:
      { if(!(board->bb_pawns[B] & (pawn_front[W][rsz[sq]] & file_mask[file])))
        { atk_sq = rsu[bitscanf(pawn_attacks[B] & (pawn_front[W][rsz[sq]] & file_mask[file]))];
          if(atk_sq)
          { if(PieceType(atk_sq + 17) == BP) atk++;
            if(PieceType(atk_sq + 15) == BP) atk++;
            if(PieceType(atk_sq - 17) == WP) def++;
            if(PieceType(atk_sq - 15) == WP) def++;
            if(atk < def) 
            { score += candidate[W][rank];
              score_eg += candidate_eg[W][rank];
            }
          }
        }
      }
      if(left)//backward / hanging pawn:
      { if(calc_rank64(bitscanf(left)) > rank) back++;
        if((right) && (back) && (calc_rank64(bitscanf(right)) > rank))
        { if(!(file_mask[file] & board->bb_pawns[B]))
          { score -= BACKWARD_ON_SEMI_OP;
            score_eg -= BACKWARD_ON_SEMI_OP_EG;
          }
          score -= backwards_penalty[rank];
          score_eg -= backwards_penalty_eg[rank];
        }
        else if(!right) 
        { score -= hanging_penalty[rank];
          score_eg -= hanging_penalty_eg[rank];
        }
      }
      else 
      { score -= hanging_penalty[rank];
        score_eg -= hanging_penalty_eg[rank];
        if(calc_rank64(bitscanf(right)) > rank)
        { score -= backwards_penalty[rank];
          score_eg -= backwards_penalty_eg[rank];
        }
      }
    }
  }
  *t_score = (short)(*t_score + score);
  *t_score_eg = (short)(*t_score_eg + score_eg);
}
static __inline void eval_black_pawn(int sq, short *t_score, short *t_score_eg)
{
  int score, score_eg;
  int back = 0;
  int file = calc_file(sq);
  int rank = calc_rank(sq);
  bool passer = false;
  int atk_sq, atk = 0, def = 0;
  bitboard_t left,right;
  
  //psq:
  score = psq_table[BP][sq];
  score_eg = psq_table[BP][sq];
  
  //mark if passed pawn:
  if(is_passed(B, sq))
  { score += passer_bonus[B][rank];
    score_eg += passer_bonus_eg[B][rank];
    passer = true;
  }
  
  //doubled pawn:
  if(calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[file])) > rank)
  { score -= doubled_penalty[rank];
    score_eg -= doubled_penalty_eg[rank];
  }
  
  if(file == FILE_A)
  { 
    //pawn chain element/proteced passer:
    if(PieceType(sq + 17) == BP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    
    //isolated pawn:
    if(is_isolated(B,sq)) 
    { score -= isolated_penalty[rank];
      score_eg -= isolated_penalty_eg[rank];
    }
    else
    { 
      //backward pawn:
      right = board->bb_pawns[B] & file_mask[file + 1];
      if(calc_rank64(bitscanr(right)) < rank)
      { score -= backwards_penalty[rank];
        score_eg -= backwards_penalty_eg[rank];
      }
      
      if(passer)//connected passer:
      { if(is_passed(B, rsu[bitscanf(right)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
      }
      else//candidate passed pawn:
      { if(!(board->bb_pawns[W] & (pawn_front[B][rsz[sq]] & file_mask[FILE_A])))
        { atk_sq = rsu[bitscanr(pawn_attacks[W] & (pawn_front[B][rsz[sq]] & file_mask[FILE_A]))];
          if(atk_sq && (PieceType(atk_sq + 17) == BP))
          { score += candidate[B][rank];
            score_eg += candidate_eg[B][rank];
          }
        }
      }
    }
  }
  else if(file == FILE_H)
  { 
    //pawn chain element/protected passer:
    if(PieceType(sq + 15) == BP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    
    //isolated pawn:
    if(is_isolated(B,sq)) 
    { score -= isolated_penalty[rank];
      score_eg -= isolated_penalty_eg[rank];
    }
    else
    { 
      //backward pawn:
      left = board->bb_pawns[B] & file_mask[file - 1];
      if(calc_rank64(bitscanr(left)) < rank)
      { score -= backwards_penalty[rank];
        score_eg -= backwards_penalty_eg[rank];
      }
      
      if(passer)//connected passer:
      { if(is_passed(B, rsu[bitscanf(left)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
      }
      else//candidate passer:
      { if(!(board->bb_pawns[W] & (pawn_front[B][rsz[sq]] & file_mask[FILE_H])))
        { atk_sq = rsu[bitscanr(pawn_attacks[W] & (pawn_front[B][rsz[sq]] & file_mask[FILE_H]))];
          if(atk_sq && (PieceType(atk_sq + 15) == BP))
          { score += candidate[B][rank];
            score_eg += candidate_eg[B][rank];
          }
        }
      }
    }
  }
  else //not a rook pawn:
  { 
    //pawn chain element/protected passer:
    if(PieceType(sq + 15) == BP) 
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    if(PieceType(sq + 17) == BP)
    { if(passer)
      { score += PROTECTED;
        score_eg += PROTECTED_EG;
      }
      else
      { score += PAWN_CHAIN_ELEMENT;
        score_eg += PAWN_CHAIN_ELEMENT_EG;
      }
    }
    
    //isolated pawn
    if(is_isolated(B,sq))
    { score -= isolated_penalty[rank];
      score_eg -= isolated_penalty_eg[rank];
    }
    else
    { 
      left  = board->bb_pawns[B] & file_mask[file - 1];
      right = board->bb_pawns[B] & file_mask[file + 1];
      
      if(passer)//connected passer:
      { if(left && is_passed(B, rsu[bitscanf(left)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
        if(right && is_passed(B, rsu[bitscanf(right)]))
        { score += CONNECTED;
          score_eg += CONNECTED_EG;
        }
      }
      else//candidate passed pawn:
      { if(!(board->bb_pawns[W] & (pawn_front[B][rsz[sq]] & file_mask[file])))
        { atk_sq = rsu[bitscanr(pawn_attacks[W] & (pawn_front[B][rsz[sq]] & file_mask[file]))];
          if(atk_sq)
          { if(PieceType(atk_sq + 17) == BP) def++;
            if(PieceType(atk_sq + 15) == BP) def++;
            if(PieceType(atk_sq - 17) == WP) atk++;
            if(PieceType(atk_sq - 15) == WP) atk++;
            if(atk < def)
            { score += candidate[B][rank];
              score_eg += candidate_eg[B][rank];
            }
          }
        }
      }
      if(left)//backward / hanging pawn:
      { if(calc_rank64(bitscanr(left)) < rank)
          back++;
        if(right && back && (calc_rank64(bitscanr(right)) < rank))
        { if(!(file_mask[file] & board->bb_pawns[W]))
          { score -= BACKWARD_ON_SEMI_OP;
            score_eg -= BACKWARD_ON_SEMI_OP_EG;
          }
          score -= backwards_penalty[rank];
          score_eg -= backwards_penalty_eg[rank];
        }
        else if(!right)
        { score -= hanging_penalty[rank];
          score_eg -= hanging_penalty_eg[rank];
        }
      }
      else 
      { score -= hanging_penalty[rank];
        score_eg -= hanging_penalty_eg[rank];
        if(calc_rank64(bitscanr(right)) < rank)
        { score -= backwards_penalty[rank];
          score_eg -= backwards_penalty_eg[rank];
        }
      }
    }
  }
  *t_score = (short)(*t_score + score);
  *t_score_eg = (short)(*t_score_eg + score_eg);
}

void eval_pawn_struct(int score[])
{
  int sq;
  pte_t *p = pnt + (board->phash & PMASK);
  
  if(p->pkey == board->phash)
  { pawn_attacks[W] = p->attacks[W];
    pawn_attacks[B] = p->attacks[B];
  }
  else
  { p->score[W] = 0;
    p->score[B] = 0;
    p->score_eg[W] = 0;
    p->score_eg[B] = 0;
    p->pkey = board->phash;
    
    //getting pawn attacks (left/right):
    pawn_attacks[W]  = ((board->bb_pawns[W] & ~FMASK_A) << 7);
    pawn_attacks[W] |= ((board->bb_pawns[W] & ~FMASK_H) << 9);
    pawn_attacks[B]  = ((board->bb_pawns[B] & ~FMASK_A) >> 9);
    pawn_attacks[B] |= ((board->bb_pawns[B] & ~FMASK_H) >> 7);
    p->attacks[W] = pawn_attacks[W]; p->attacks[B] = pawn_attacks[B];
    
    for(sq = PLS(WP); sq <= H8; sq = PLN(sq))
      eval_white_pawn(sq, &p->score[W], &p->score_eg[W]);
    for(sq = PLS(BP); sq <= H8; sq = PLN(sq))
      eval_black_pawn(sq, &p->score[B], &p->score_eg[B]);
  }
  score[W] += p->score[W];
  score[B] += p->score[B];
}

void eval_pawn_struct_endgame(int score[])
{	
  int sq;
  pte_t *p = pnt + (board->phash & PMASK);
  
  if(p->pkey == board->phash)
  { pawn_attacks[W] = p->attacks[W];
    pawn_attacks[B] = p->attacks[B];
    
    for(sq = PLS(WP); sq <= H8; sq = PLN(sq))
      if(is_passed(W, sq)) score[W] += eval_white_activity(sq);
    for(sq = PLS(BP); sq <= H8; sq = PLN(sq))
      if(is_passed(B, sq)) score[B] += eval_black_activity(sq);
  }
  else
  { p->score[W] = 0;
    p->score[B] = 0;
    p->score_eg[W] = 0;
    p->score_eg[B] = 0;
    p->pkey = board->phash;
    
    //getting pawn attacks (left/right):
    pawn_attacks[W]  = ((board->bb_pawns[W] & ~FMASK_A) << 7);
    pawn_attacks[W] |= ((board->bb_pawns[W] & ~FMASK_H) << 9);
    pawn_attacks[B]  = ((board->bb_pawns[B] & ~FMASK_A) >> 9);
    pawn_attacks[B] |= ((board->bb_pawns[B] & ~FMASK_H) >> 7);
    p->attacks[W] = pawn_attacks[W]; p->attacks[B] = pawn_attacks[B];
       
    for(sq = PLS(WP); sq <= H8; sq = PLN(sq))
    { eval_white_pawn(sq, &p->score[W], &p->score_eg[W]);
      if(is_passed(W, sq)) score[W] += eval_white_activity(sq);
    }
    for(sq = PLS(BP); sq <= H8; sq = PLN(sq))
    { eval_black_pawn(sq, &p->score[B], &p->score_eg[B]);
      if(is_passed(B, sq)) score[B] += eval_black_activity(sq);
    }
  }
  score[W] += p->score_eg[W];
  score[B] += p->score_eg[B];
}
