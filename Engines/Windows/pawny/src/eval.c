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

#define BISHOP_PAIR_REWARD 50
#define DEVELOPMENT_REWARD 2
#define ROOK_ON_7TH_REWARD 20
#define QUEEN_ON_7TH_REWARD 15
#define ROOK_ON_OPEN_FILE_REWARD 5
#define ROOK_ON_SEMI_OPEN_FILE_REWARD 7
#define ROOKS_DOUBLED 4
#define ROOK_ON_E_FILE_REWARD 5
#define ROOK_ON_D_FILE_REWARD 5
#define TRAPPED_ROOK_PENALTY 50
#define TRAPPED_BISHOP_PENALTY 80
#define BLOCKED_BISHOP_PENALTY 50
#define OPCOLOURED_BISHOPS 15
#define QUEEN_EARLY_DEVELOPMENT_PENALTY 12
#define UNDEVELOPED_KING_PAWN_PENALTY 15
#define UNDEVELOPED_QUEEN_PAWN_PENALTY 14
#define UNDEVELOPED_KKNIGHT_PENALTY 12
#define UNDEVELOPED_QKNIGHT_PENALTY 11
#define UNDEVELOPED_KBISHOP_PENALTY 10
#define UNDEVELOPED_QBISHOP_PENALTY 9
#define KING_UNCASTLED_YET 15
#define KING_UNCASTLED_STILL 20
#define KING_EXPOSED 12

//mobility bonus table:
static const int mobility[7][30] = 
{ {0}, //empty
  {0}, //pawn
  {-4, -3,  -2, -1, 0, 1, 2, 3, 4}, //knight
  {-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8}, //bishop
  {-8, -7, -6, -5, -4, -1, 0, 1, 4, 5, 6, 7, 8, 9 ,10,11,12}, //rook
  {-3, -2, -1, -1, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3 ,3,
    3, 4, 4, 4, 4, 5, 5, 6, 6, 7, 7, 7}, //queen
  {0}  //king
};

//[color], [pawn first] rank
static const int king_shield[2][8] = 
{ {-12, 2, -4, -8, -12, -12, -12, -12},
  {-12, -12, -12, -12, -8, -4,  2, -12}
};

//material adjustment [own_piececount][own_pawns]
static const int rook_imbalance[10] = 
{60, 48, 36, 24, 12, 0,-12,-24,-36, 0};

static const int knight_imbalance[10] = 
{-30, -24, -18, -12, -6,  0,  6, 12, 18, 0};

static const int sq_ctrl[128] = 
{ 
  0,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  2,  2,  2,  2,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  2,  4,  4,  2,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  2,  4,  4,  2,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  2,  2,  2,  2,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0,
  0,  0,  0,  0,  0,  0,  0,  0, 0,0,0,0,0,0,0,0
};

static const int safety_kingside[2][128] =
{ { 
    0, 0, 0, 0, 0, 4,10, 4,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 4, 8, 4,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0
  },
  {
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 4, 8, 4,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 4,10, 4,  0,0,0,0,0,0,0,0
  }
};

static const int safety_queenside[2][128] =
{ {
    0, 8,10, 4, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    4, 6, 8, 4, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0
  },
  {
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    4, 6, 8, 4, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
    0, 8,10, 4, 0, 0, 0, 0,  0,0,0,0,0,0,0,0
  }
};

static const int empty[128] =
{
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0,
  0, 0, 0, 0, 0, 0, 0, 0,  0,0,0,0,0,0,0,0
};

static const int *safety[2];

static int eval_white_opening()
{
  int score = 0;
  bool queen_developed = false;
  bool all_pieces_developed = true;
  
  if(PieceType(D1) != WHITE_QUEEN)
    queen_developed = true;
  if(PieceType(E2) == WHITE_PAWN)
    score -= UNDEVELOPED_KING_PAWN_PENALTY;
  if(PieceType(D2) == WHITE_PAWN)
    score -= UNDEVELOPED_QUEEN_PAWN_PENALTY;
  if(PieceType(B1) == WHITE_KNIGHT)
  { score -= UNDEVELOPED_QKNIGHT_PENALTY;
    all_pieces_developed = false;
  }
  if(PieceType(G1) == WHITE_KNIGHT)
  { score -= UNDEVELOPED_KKNIGHT_PENALTY;
    all_pieces_developed = false;
  }
  if(PieceType(C1) == WHITE_BISHOP)
  { score -= UNDEVELOPED_QBISHOP_PENALTY;
    all_pieces_developed = false;
  }
  if(PieceType(F1) == WHITE_BISHOP)
  { score -= UNDEVELOPED_KBISHOP_PENALTY;
    all_pieces_developed = false;
  }
  if(!all_pieces_developed && queen_developed)
    score -= QUEEN_EARLY_DEVELOPMENT_PENALTY;
  if(all_pieces_developed && !queen_developed)
    score += DEVELOPMENT_REWARD;
  return score;
}

static int eval_black_opening()
{
  int score = 0;
  bool queen_developed = false;
  bool all_pieces_developed = true;
  
  if(PieceType(D8) != BLACK_QUEEN)
    queen_developed = true;
  if(PieceType(E7) == BLACK_PAWN)
    score -= UNDEVELOPED_KING_PAWN_PENALTY;
  if(PieceType(D7) == BLACK_PAWN)
    score -= UNDEVELOPED_QUEEN_PAWN_PENALTY;
  if(PieceType(B8) == BLACK_KNIGHT)
  { score -= UNDEVELOPED_QKNIGHT_PENALTY;
    all_pieces_developed = false;
  }
  if(PieceType(G8) == BLACK_KNIGHT)
  { score -= UNDEVELOPED_KKNIGHT_PENALTY;
    all_pieces_developed = false;
  }
  if(PieceType(C8) == BLACK_BISHOP)
  { score -= UNDEVELOPED_QBISHOP_PENALTY;
    all_pieces_developed = false;
  }
  if(PieceType(F8) == BLACK_BISHOP)
  { score -= UNDEVELOPED_KBISHOP_PENALTY;
    all_pieces_developed = false;
  }
  if(!all_pieces_developed && queen_developed)
    score -= QUEEN_EARLY_DEVELOPMENT_PENALTY;
  if(all_pieces_developed && !queen_developed)
    score += DEVELOPMENT_REWARD;
  return score;
}
  
static int eval_white_king(int sq)
{
  register int pawn_first;
  int score = 0;

  safety[W] = &empty[0];
  
  if(!(board->castle & (WHITE_OO|WHITE_OOO)))
  { if(sq > F1 && sq <= H1)
    { pawn_first = calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[FILE_F]));
      score += (king_shield[W][pawn_first]) * 3;
      pawn_first = calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[FILE_G]));
      score += (king_shield[W][pawn_first]) * 2;
      pawn_first = calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[FILE_H]));
      score += (king_shield[W][pawn_first]);
    
      score -= popcnt(board->bb_pawns[B] & king_storm_mask[W]) * 4;
      
      safety[W] = &safety_kingside[W][0];
    }	
    else if(sq < D1 && sq >= A1)
    { pawn_first = calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[FILE_C]));
      score += (king_shield[W][pawn_first]) * 3;
      pawn_first = calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[FILE_B]));
      score += (king_shield[W][pawn_first]) * 2;
      pawn_first = calc_rank64(bitscanf(board->bb_pawns[W] & file_mask[FILE_A]));
      score += (king_shield[W][pawn_first]);
    
      score -= popcnt(board->bb_pawns[B] & queen_storm_mask[W]) * 4;
      
      safety[W] = &safety_queenside[W][0];
    }
    else score -= KING_UNCASTLED_STILL;
  }
  else
  { score -= KING_UNCASTLED_YET;
    if(PieceType(sq + 16) != WP
    &&(PieceType(sq + 15) != WP
    || PieceType(sq + 17) != WP))
      score -= KING_EXPOSED;
  }
  return score;
}

static int eval_black_king(int sq)
{
  register int pawn_first;
  int score = 0;
  
  safety[B] = &empty[0];
  
  if(!(board->castle & (BLACK_OO|BLACK_OOO)))
  { if(sq > F8 && sq <= H8)
    { pawn_first = calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[FILE_F]));
      score += (king_shield[B][pawn_first]) * 3;
      pawn_first = calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[FILE_G]));
      score += (king_shield[B][pawn_first]) * 2;
      pawn_first = calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[FILE_H]));
      score += (king_shield[B][pawn_first]);
    
      score -= popcnt(board->bb_pawns[W] & king_storm_mask[B]) * 4;
      
      safety[B] = &safety_kingside[B][0];
    }	
    else if(sq < D8 && sq >= A8)
    { pawn_first = calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[FILE_C]));
      score += (king_shield[B][pawn_first]) * 3;
      pawn_first = calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[FILE_B]));
      score += (king_shield[B][pawn_first]) * 2;
      pawn_first = calc_rank64(bitscanr(board->bb_pawns[B] & file_mask[FILE_A]));
      score += (king_shield[B][pawn_first]);
  
      score -= popcnt(board->bb_pawns[W] & queen_storm_mask[B]) * 4;

      safety[B] = &safety_queenside[B][0];
    }
    else score -= KING_UNCASTLED_STILL;
  }
  else 
  { score -= KING_UNCASTLED_YET;
    if(PieceType(sq - 16) != BP
    &&(PieceType(sq - 15) != BP
    || PieceType(sq - 17) != BP))
      score -= KING_EXPOSED;
  }
  return score;
}

static int eval_white_rook(int sq)
{
  int score = 0;
  int file = calc_file(sq);
  int rank = calc_rank(sq);
  int k_sq = King_Square(W);
  bool wf,bf;
  
  wf = (bool)(true && (file_mask[file] & board->bb_pawns[W]));
  bf = (bool)(true && (file_mask[file] & board->bb_pawns[B]));

  if(rank == RANK_7) score += ROOK_ON_7TH_REWARD;
  if(!wf && !bf)
  { score += ROOK_ON_OPEN_FILE_REWARD;
    //opened file with doubled rooks 
    //(backward rook examined if exists):
    if(rank > RANK_1)
    { if(PieceType(sq-16) == WHITE_ROOK)
        score+= ROOKS_DOUBLED;
    }
  }
  if(!wf && bf) score += ROOK_ON_SEMI_OPEN_FILE_REWARD;	
  //white trapped rook:
  if(sq == H1 || sq == G1)
  { if(k_sq > E1 && k_sq < H1)
      score -= TRAPPED_ROOK_PENALTY;
  }
  if(sq >= A1 && sq <= C1)
  { if(k_sq <= D1) score -= TRAPPED_ROOK_PENALTY;
  }
  if(file == FILE_E)
    score += ROOK_ON_E_FILE_REWARD;
  if(file == FILE_D)
    score += ROOK_ON_D_FILE_REWARD;	
  return score;
}


static int eval_black_rook(int sq)
{
  int score = 0;
  int file = calc_file(sq);
  int rank = calc_rank(sq);
  int k_sq = King_Square(B);
  bool wf,bf;
  
  wf = (bool)(true && (file_mask[file] & board->bb_pawns[W]));
  bf = (bool)(true && (file_mask[file] & board->bb_pawns[B]));
  
  if(rank == RANK_2) score += ROOK_ON_7TH_REWARD;
    
  if(!bf && !wf)
  { score += ROOK_ON_OPEN_FILE_REWARD;
    if(rank < RANK_8)
    { if(PieceType(sq + 16) == BLACK_ROOK)
        score+= ROOKS_DOUBLED;
    }
  }
  if(!bf && wf) score += ROOK_ON_SEMI_OPEN_FILE_REWARD;
  //black trapped rook:
  if(sq == H8 || sq == G8)
  { if(k_sq > E8 && k_sq < H8) score -= TRAPPED_ROOK_PENALTY;
  }
  if(sq >= A8 && sq <= C8)
  { if(k_sq <= D8) score -= TRAPPED_ROOK_PENALTY;
  }
  if(file == FILE_E)
    score += ROOK_ON_E_FILE_REWARD;
  if(file == FILE_D)
    score += ROOK_ON_D_FILE_REWARD;
  return score;
}

static int eval_white_bishop(int sq)
{
  int score = 0;
    
  //trapped bishop on 7th rank
  if(((sq == A7) && (PieceType(B6) == BLACK_PAWN))
  || ((sq == B8) && (PieceType(C7) == BLACK_PAWN)))
    score -= TRAPPED_BISHOP_PENALTY;
    
  if(((sq == H7) && (PieceType(G6) == BLACK_PAWN))
  || ((sq == G8) && (PieceType(F7) == BLACK_PAWN)))
    score -= TRAPPED_BISHOP_PENALTY;
  
  //trapped on 6th:
  if((sq == A6) && (PieceType(B5) == BLACK_PAWN))
    score -= (TRAPPED_BISHOP_PENALTY / 2);
    
  if((sq == H6) && (PieceType(G5) == BLACK_PAWN))
    score -= (TRAPPED_BISHOP_PENALTY / 2);
    
  //bishop, blocked  by it's own pawns:
  if(sq == C1 && PieceType(D2) == WHITE_PAWN && !IsEmpty(D3))
    score -= BLOCKED_BISHOP_PENALTY;
    
  if(sq == F1 && PieceType(E2) == WHITE_PAWN && !IsEmpty(E3))
    score -= BLOCKED_BISHOP_PENALTY;
  
  return score;
}

static int eval_black_bishop(int sq)
{
  int score = 0;
    
  //trapped bishop on 7th
  if(((sq == A2) && (PieceType(B3) == WHITE_PAWN))
  || ((sq == B1) && (PieceType(C2) == WHITE_PAWN)))
    score -= TRAPPED_BISHOP_PENALTY;
    
  if(((sq == H2) && (PieceType(G3) == WHITE_PAWN))
  || ((sq == G1) && (PieceType(F2) == WHITE_PAWN)))
    score -= TRAPPED_BISHOP_PENALTY;
  
  //trapped on 6th:
  if((sq == A3) && (PieceType(B4) == WHITE_PAWN))
    score -= (TRAPPED_BISHOP_PENALTY / 2);
    
  if((sq == H3) && (PieceType(G4) == WHITE_PAWN))
    score -= (TRAPPED_BISHOP_PENALTY / 2);
    
  //bishop, blocked  by it's own pawns:
  if(sq == C8 && PieceType(D7) == BLACK_PAWN && !IsEmpty(D6))
    score -= BLOCKED_BISHOP_PENALTY;
    
  if(sq == F8 && PieceType(E7) == BLACK_PAWN && !IsEmpty(E6))
    score -= BLOCKED_BISHOP_PENALTY;
  
  return score;
}

static __inline int eval_q_mobility(int sq,int color)
{
  int dest,r = 0,score = 0;
  int opcolor = color ^ 1;
  
  dest = sq + 17;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 17;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 17;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 17;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 15;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 15;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 15;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 15;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 16;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 16;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 16;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 16;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 1;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 1;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 1;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 1;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  score += (mobility[QUEEN][r]);
  return score;
}

static __inline int eval_r_mobility(int sq, int color)
{
  int dest,r = 0,score = 0;
  int opcolor = color ^ 1;
  
  dest = sq + 16;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 16;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 16;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 16;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 1;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 1;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 1;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 1;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  score += (mobility[ROOK][r]);
  return score;
}

static __inline int eval_b_mobility(int sq, int color)
{
  int dest,r = 0,score = 0;
  int opcolor = color ^ 1;
  
  dest = sq + 17;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 17;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 17;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 17;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 15;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest += 15;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 15;
  while(IsEmpty(dest))
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
    dest -= 15;
  }
  if(PieceColor(dest) == opcolor)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }

  score += (mobility[BISHOP][r]);
  return score;
}

static __inline int eval_n_mobility(int sq, int color)
{
  int dest,r = 0,score = 0;
  int opcolor = color ^ 1;
  
  dest = sq + 14;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }	
  dest = sq - 14;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 18;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 18;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 31;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 31;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq + 33;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }
  dest = sq - 33;
  if(!IsOutside(dest) && PieceColor(dest) != color)
  { r++;
    score += sq_ctrl[dest];
    score += *(safety[opcolor] + dest);
  }

  score += (mobility[KNIGHT][r]);
  return score;
}

static int evaluate()
{
  int sq;
  int score[2];
  sq_t king_square[2];
  state_t *state = &board->state;
  
  //clearing data:
  score[W] = 0; score[B] = 0;

  ///pawn evaluation:
  eval_pawn_struct(&score[0]);
  
  ///piece evaluation
  //kings
  king_square[W] = King_Square(W);
  king_square[B] = King_Square(B);
  score[W] += psq_table[WK][king_square[W]];
  score[W] += eval_white_king(king_square[W]);
  score[B] += psq_table[BK][king_square[B]];
  score[B] += eval_black_king(king_square[B]);
  
  //queens:
  for(sq = PLS(WQ); sq <= H8; sq = PLN(sq))
  { score[W] += psq_table[WQ][sq];
    if(calc_rank(sq) == RANK_7
    && calc_rank(king_square[BLACK]) == RANK_8)
      score[W] += QUEEN_ON_7TH_REWARD;
    score[W] += eval_q_mobility(sq,W);
  }
  for(sq = PLS(BQ); sq <= H8; sq = PLN(sq))
  { score[B] += psq_table[BQ][sq];
    if(calc_rank(sq) == RANK_2
    && calc_rank(king_square[WHITE]) == RANK_1)
      score[B] += QUEEN_ON_7TH_REWARD;
    score[B] += eval_q_mobility(sq,B);
  }
  
  //rooks:
  for(sq = PLS(WR); sq <= H8; sq = PLN(sq))
  { score[W] += psq_table[WR][sq];
    score[W] += eval_white_rook(sq);
    score[W] += eval_r_mobility(sq,W);
  }
  for(sq = PLS(BR); sq <= H8; sq = PLN(sq))
  { score[B] += psq_table[BR][sq];
    score[B] += eval_black_rook(sq);
    score[B] += eval_r_mobility(sq,B);
  }
  
  //bishops:
  for(sq = PLS(WB); sq <= H8; sq = PLN(sq))
  { score[W] += psq_table[WB][sq];
    score[W] += eval_white_bishop(sq);
    score[W] += eval_b_mobility(sq,W);
    
    //outposts - twice as low than a knight outpost:
    if(!((1ULL << rsz[sq]) & pawn_attacks[B]))
    { if(psq_outposts[W][sq])
      { //if not attacked by an opponent's pawn,
        //this could be a weak square that matters,
        //in accordance with the outposts table:
        score[W] += (psq_outposts[W][sq]) / 2;
        
        //additional bonus if it's reinforced by own pawns:
        if((1ULL << rsz[sq]) & pawn_attacks[W])
          score[W] += (psq_outposts[W][sq]) / 2;				
      }
    }
  }
  for(sq = PLS(BB); sq <= H8; sq = PLN(sq))
  { score[B] += psq_table[BB][sq];
    score[B] += eval_black_bishop(sq);
    score[B] += eval_b_mobility(sq,B);
    
    //outposts:
    if(!((1ULL << rsz[sq]) & pawn_attacks[W]))
    { if(psq_outposts[B][sq])
      { score[B] += (psq_outposts[B][sq]) / 2;
        if((1ULL << rsz[sq]) & pawn_attacks[B])
          score[B] += (psq_outposts[B][sq]) / 2;
      }
    }
  }
  
  //knights:
  for(sq = PLS(WN); sq <= H8; sq = PLN(sq))
  { score[W] += psq_table[WN][sq];
    score[W] += eval_n_mobility(sq,W);
    
    //outposts:
    if(!((1ULL << rsz[sq]) & pawn_attacks[B]))
    { if(psq_outposts[W][sq])
      { score[W] += psq_outposts[W][sq];
        if((1ULL << rsz[sq]) & pawn_attacks[W])
          score[W] += psq_outposts[W][sq];				
      }
    }
  }
  for(sq = PLS(BN); sq <= H8; sq = PLN(sq))
  { score[B] += psq_table[BN][sq];
    score[B] += eval_n_mobility(sq,B);
    
    //outposts:
    if(!((1ULL << rsz[sq]) & pawn_attacks[W]))
    { if(psq_outposts[B][sq])
      { score[B] += psq_outposts[B][sq];
        if((1ULL << rsz[sq]) & pawn_attacks[B])
          score[B] += psq_outposts[B][sq];
      }
    }
  }
  
  //bishop pair bonus depending on pawns on board:
  if(Bishops(W) >= 2) score[W] += BISHOP_PAIR_REWARD - state->pawns;
  if(Bishops(B) >= 2) score[B] += BISHOP_PAIR_REWARD - state->pawns;
  
  //calculate basic knight and rook material imbalances:
  /**********************************************************
      quote from GM L.Kaufman:
   "A further refinement would be to raise the knight's value 
   by 1/16 and lower the rook's value by 1/8 for each pawn 
   above five of the side being valued, with the opposite 
   adjustment for each pawn short of five".
  ***********************************************************/	
  score[W] += Rooks(W)   * rook_imbalance[Pawns(W)];
  score[W] += Knights(W) * knight_imbalance[Pawns(W)];
  score[B] += Rooks(B)   * rook_imbalance[Pawns(B)];
  score[B] += Knights(B) * knight_imbalance[Pawns(B)];
    
  if(opening)
  { score[W] += eval_white_opening();
    score[B] += eval_black_opening();
  }

  ///final results:
  score[W] += state->material_value[WHITE] + \
    state->pawn_value[WHITE];
  score[B] += state->material_value[BLACK] + \
    state->pawn_value[BLACK];
  
  if(board->side == W) return (score[W]-score[B]);
  else return -(score[W]-score[B]);
}

static int evaluate_endgame()
{
  int sq;
  int score[2];
  sq_t king_square[2];
  state_t *state = &board->state;
  
  //clearing data:
  score[W] = 0; score[B] = 0;
  
  ///pawn evaluation:
  eval_pawn_struct_endgame(&score[0]);

  ///piece evaluation
  //kings
  king_square[W] = King_Square(W);
  king_square[B] = King_Square(B);
  score[W] += endgame_king_psq[W][king_square[W]];
  score[B] += endgame_king_psq[B][king_square[B]];
  
  if(state->material)
  {
    //queens:
    for(sq = PLS(WQ); sq <= H8; sq = PLN(sq))
    { score[W] += psq_table[WQ][sq];
      if(calc_rank(sq) == RANK_7
      && calc_rank(king_square[BLACK]) == RANK_8)
        score[W] += QUEEN_ON_7TH_REWARD;
    }
    for(sq = PLS(BQ); sq <= H8; sq = PLN(sq))
    { score[B] += psq_table[BQ][sq];
      if(calc_rank(sq) == RANK_2
      && calc_rank(king_square[WHITE]) == RANK_1)
        score[B] += QUEEN_ON_7TH_REWARD;
    }
    
    //rooks:
    for(sq = PLS(WR); sq <= H8; sq = PLN(sq))
    { score[W] += psq_table[WR][sq];
      score[W] += eval_white_rook(sq);
    }
    for(sq = PLS(BR); sq <= H8; sq = PLN(sq))
    { score[B] += psq_table[BR][sq];
      score[B] += eval_black_rook(sq);
    }
    
    //bishops:
    for(sq = PLS(WB); sq <= H8; sq = PLN(sq))
    { score[W] += psq_table[WB][sq];
      score[W] += eval_white_bishop(sq);
    
      //outposts
      if(!((1ULL << rsz[sq]) & pawn_attacks[B]))
      { if(psq_outposts[W][sq])
        { score[W] += (psq_outposts[W][sq]) / 2;
          if((1ULL << rsz[sq]) & pawn_attacks[W])
            score[W] += (psq_outposts[W][sq]) / 2;				
        }
      }
    }
    for(sq = PLS(BB); sq <= H8; sq = PLN(sq))
    { score[B] += psq_table[BB][sq];
      score[B] += eval_black_bishop(sq);
    
      //outposts:
      if(!((1ULL << rsz[sq]) & pawn_attacks[W]))
      { if(psq_outposts[B][sq])
        { score[B] += (psq_outposts[B][sq]) / 2;
          if((1ULL << rsz[sq]) & pawn_attacks[B])
            score[B] += (psq_outposts[B][sq]) / 2;
        }
      }
    }
    
    //knights:
    for(sq = PLS(WN); sq <= H8; sq = PLN(sq))
    { score[W] += psq_table[WN][sq];
    
      //outposts:
      if(!((1ULL << rsz[sq]) & pawn_attacks[B]))
      { if(psq_outposts[W][sq])
        { score[W] += psq_outposts[W][sq];
          if((1ULL << rsz[sq]) & pawn_attacks[W])
            score[W] += psq_outposts[W][sq];				
        }
      }
    }
    for(sq = PLS(BN); sq <= H8; sq = PLN(sq))
    { score[B] += psq_table[BN][sq];
    
      //outposts:
      if(!((1ULL << rsz[sq]) & pawn_attacks[W]))
      { if(psq_outposts[B][sq])
        { score[B] += psq_outposts[B][sq];
          if((1ULL << rsz[sq]) & pawn_attacks[B])
            score[B] += psq_outposts[B][sq];
        }
      }
    }
  
    //bishop pair bonus depending on pawns on board:
    if(Bishops(W) >= 2) score[W] += BISHOP_PAIR_REWARD - state->pawns;
    if(Bishops(B) >= 2) score[B] += BISHOP_PAIR_REWARD - state->pawns;
  
    //calculate basic knight and rook material imbalances:
    score[W] += Rooks(W)   * rook_imbalance[Pawns(W)];
    score[W] += Knights(W) * knight_imbalance[Pawns(W)];
    score[B] += Rooks(B)   * rook_imbalance[Pawns(B)];
    score[B] += Knights(B) * knight_imbalance[Pawns(B)];
    
    //a piece of code to encourage exchanges
    //in case of material advantage:
    if(state->material_value[W] > state->material_value[B]
    || state->pawn_value[W] > state->pawn_value[B])
      score[W] -= (state->piece_count[W] + state->piece_count[B]) * 2;
  
    if(state->material_value[B] > state->material_value[W]
    || state->pawn_value[B] > state->pawn_value[W])
      score[B] -= (state->piece_count[B] + state->piece_count[W]) * 2;
  }		
  return endeval(&score[0]);
}

int full_material_ballance(int color)
{
  int result = 0;
  state_t *state = &board->state;
  
  result += state->material_value[color];
  result -= state->material_value[color ^ 1];
  result += state->pawn_value[color];
  result -= state->pawn_value[color ^ 1];
  return result;
}

int material_gain(move_t m)
{
  int result = 0;
  if(m.promoted)
  { result -= P_VALUE;
    result += pval[m.promoted];
  }
  if(m.type & CAP)
    result += pval[hs[board->hply - 1].captured];
  return result;
}

int eval()
{
  if(!endgame) return evaluate();
  return evaluate_endgame();
}
