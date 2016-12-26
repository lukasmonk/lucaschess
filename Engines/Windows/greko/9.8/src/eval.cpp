//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  eval.cpp: static position evaluation
//  modified: 31-Dec-2012

#include "eval.h"
#include "utils.h"

EVAL VAL_P = 100;
EVAL VAL_N = 400;
EVAL VAL_B = 400;
EVAL VAL_R = 600;
EVAL VAL_Q = 1200;

EVAL LazyEval         = 150;

EVAL PawnDoubled      = -10;
EVAL PawnIsolated     = -10;
EVAL PawnBackwards    = -10;
EVAL PawnCenter       =  10;
EVAL PawnPassed       = 100;
EVAL PassedKingDist   =   5;
EVAL PawnPassedSquare =  50;
EVAL KnightMobility   =  10;
EVAL KnightCenter     =  10;
EVAL KnightOutpost    =  10;
EVAL BishopPair       =  50;
EVAL BishopCenter     =  10;
EVAL BishopMobility   =  40;
EVAL BishopTrapped    = -50;
EVAL RookMobility     =  20;
EVAL Rook7th          =  20;
EVAL RookOpen         =  10;
EVAL QueenKingTropism =  30;
EVAL KingCenterMid    = -40;
EVAL KingCenterEnd    =  40;
EVAL KingPawnShield   = 100;

EVAL PSQ_P[64];
EVAL PSQ_N[64];
EVAL PSQ_B[64];
EVAL PSQ_K_MID[64];
EVAL PSQ_K_END[64];
EVAL KNIGHT_MOBILITY[9];
EVAL BISHOP_MOBILITY[14];
EVAL ROOK_MOBILITY[15];
EVAL QUEEN_KING_TROPISM[8];
EVAL KING_PAWN_SHIELD[10];

struct PawnEntry
{
  U32  m_pawnHash;
  int  m_ranks[10][2];
  EVAL m_score;
  U64  m_passed[2];

  void Read(const Position& pos);
};

const int gm_pawnHashSize = 8192;
PawnEntry gm_pawnHash[gm_pawnHashSize];
EvalWeights g_evalWeights;

inline int Dist(FLD f1, FLD f2)
{
  int drow = Row(f1) - Row(f2);
  if (drow < 0) drow = -drow;
  int dcol = Col(f1) - Col(f2);
  if (dcol < 0) dcol = -dcol;

  return (drow > dcol)? drow : dcol;
}

int PawnShieldWhite(const PawnEntry& pentry, FLD K)
{
  int r = 0;
  int file = Col(K) + 1;
  for (int i = file - 1; i <= file + 1; ++i)
  {
    int rank = pentry.m_ranks[i][WHITE];
    if (rank == 6)
      ;
    else if (rank == 5)
      r += 1;
    else if (rank == 4)
      r += 2;
    else
      r += 3;
  }
  return r;
}

int PawnShieldBlack(const PawnEntry& pentry, FLD K)
{
  int r = 0;
  int file = Col(K) + 1;
  for (int i = file - 1; i <= file + 1; ++i)
  {
    int rank = pentry.m_ranks[i][BLACK];
    if (rank == 1)
      ;
    else if (rank == 2)
      r += 1;
    else if (rank == 3)
      r += 2;
    else
      r += 3;
  }
  return r;
}

EVAL Evaluate(const Position& pos, EVAL alpha, EVAL beta)
{
  EVAL_ESTIMATION ee = EstimateDraw(pos);
  if (ee == EVAL_THEORETICAL_DRAW || ee == EVAL_PRACTICAL_DRAW) return DRAWm_score;

  EVAL matScore = pos.Material(WHITE) - pos.Material(BLACK);
  EVAL posScore = 0;
  EVAL boardScore = 0;
  EVAL mobScore = 0; 
  EVAL pawnStructScore = 0;
  EVAL pawnPassedScore = 0;
  EVAL kingSafetyScore = 0;

  if (pos.Count(BW) == 2)
    matScore += BishopPair;
  if (pos.Count(BB) == 2)
    matScore -= BishopPair;

  matScore = matScore * g_evalWeights.Material / 50;

  EVAL lazy = (pos.Side() == WHITE)? matScore : -matScore;
  if (lazy <= alpha - LazyEval)
    return alpha;
  if (lazy >= beta + LazyEval)
    return beta;

  U64 x, y, occ = pos.BitsAll();
  FLD f;

  //
  //   PAWNS
  //

  int index = pos.PawnHash() % gm_pawnHashSize;
  PawnEntry& pentry = gm_pawnHash[index];
  if (pentry.m_pawnHash != pos.PawnHash())
    pentry.Read(pos);

  pawnStructScore += pentry.m_score;

  x = pentry.m_passed[WHITE];
  while (x)
  {
    f = PopLSB(x);
    pawnPassedScore += PawnPassed * (7 - Row(f)) / 6;
    if (pos.MatIndex(BLACK) == 0)
    {
      FLD f1 = f;
      if (pos.Side() == BLACK)
        f1 += 8;
      if ((BB_PAWN_SQUARE[f1][WHITE] & pos.Bits(KB)) == 0)
        pawnPassedScore  += PawnPassedSquare * (7 - Row(f1)) / 6;
    }
    else if (pos.MatIndex(BLACK) < 10)
      pawnPassedScore  += PassedKingDist * Dist(f - 8, pos.King(BLACK));
  }

  x = pentry.m_passed[BLACK];
  while (x)
  {
    f = PopLSB(x);
    pawnPassedScore  -= PawnPassed * Row(f) / 6;
    if (pos.MatIndex(WHITE) == 0)
    {
      FLD f1 = f;
      if (pos.Side() == WHITE)
        f1 -= 8;
      if ((BB_PAWN_SQUARE[f1][BLACK] & pos.Bits(KW)) == 0)
        pawnPassedScore  -= PawnPassedSquare * Row(f1) / 6;
    }
    else if (pos.MatIndex(WHITE) < 10)
      pawnPassedScore  -= PassedKingDist * Dist(f + 8, pos.King(WHITE));
  }

  //
  //   KNIGHTS
  //

  static const int outpost[64] =
  {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
  };

  x = pos.Bits(NW);
  while (x)
  {
    f = PopLSB(x);
    boardScore += PSQ_N[f];
    y = BB_KNIGHT_ATTACKS[f] & ~occ;
    mobScore += KNIGHT_MOBILITY[CountBits(y)];
    if (outpost[f])
    {
      if (BB_PAWN_ATTACKS[f][BLACK] & pos.Bits(PW))
        boardScore += KnightOutpost;
    }
  }

  x = pos.Bits(NB);
  while (x)
  {
    f = PopLSB(x);
    boardScore -= PSQ_N[FLIP[f]];
    y = BB_KNIGHT_ATTACKS[f] & ~occ;
    mobScore -= KNIGHT_MOBILITY[CountBits(y)];
    if (outpost[FLIP[f]])
    {
      if (BB_PAWN_ATTACKS[f][WHITE] & pos.Bits(PB))
        boardScore -= KnightOutpost;
    }
  }

  //
  //   BISHOPS
  //
  
  x = pos.Bits(BW);
  while (x)
  {
    f = PopLSB(x);
    boardScore += PSQ_B[f];
    if ((f == A7 || f == B8) && pos[B6] == PB && pos[C7] == PB)
      mobScore += BishopTrapped;
    if ((f == H7 || f == G8) && pos[G6] == PB && pos[F7] == PB)
      mobScore += BishopTrapped;
    y = BishopAttacks(f, occ);
    mobScore += BISHOP_MOBILITY[CountBits(y)];
  }

  x = pos.Bits(BB);
  while (x)
  {
    f = PopLSB(x);
    boardScore -= PSQ_B[FLIP[f]];
    if ((f == A2 || f == B1) && pos[B3] == PW && pos[C2] == PW)
      mobScore -= BishopTrapped;
    if ((f == H2 || f == G1) && pos[G3] == PW && pos[F2] == PW)
      mobScore -= BishopTrapped;
    y = BishopAttacks(f, occ);
    mobScore -= BISHOP_MOBILITY[CountBits(y)];
  }

  //
  //   ROOKS
  //

  x = pos.Bits(RW);
  while (x)
  {
    f = PopLSB(x);
    y = RookAttacks(f, occ ^ pos.Bits(RW));
    mobScore += ROOK_MOBILITY[CountBits(y)];
    if (Row(f) == 1)
      boardScore += Rook7th;
    int file = Col(f) + 1;
    if (pentry.m_ranks[file][WHITE] == 0)
      boardScore += RookOpen;
  }

  x = pos.Bits(RB);
  while (x)
  {
    f = PopLSB(x);
    y = RookAttacks(f, occ ^ pos.Bits(RB));
    mobScore -= ROOK_MOBILITY[CountBits(y)];
    if (Row(f) == 6)
      boardScore -= Rook7th;
    int file = Col(f) + 1;
    if (pentry.m_ranks[file][BLACK] == 7)
      boardScore -= RookOpen;
  }

  //
  //   QUEENS
  //

  x = pos.Bits(QW);
  while (x)
  {
    f = PopLSB(x);
    kingSafetyScore += QUEEN_KING_TROPISM[Dist(f, pos.King(BLACK))];
  }

  x = pos.Bits(QB);
  while (x)
  {
    f = PopLSB(x);
    kingSafetyScore -= QUEEN_KING_TROPISM[Dist(f, pos.King(WHITE))];
  }

  //
  //   KINGS
  //

  {
    f = pos.King(WHITE);
    kingSafetyScore += PSQ_K_MID[f] * pos.MatIndex(BLACK) / 32;
    boardScore += PSQ_K_END[f] * (32 - pos.MatIndex(BLACK)) / 32;

    int penalty = PawnShieldWhite(pentry, f);
    kingSafetyScore += KING_PAWN_SHIELD[penalty] * pos.MatIndex(BLACK) / 32;
  }

  {
    f = pos.King(BLACK);
    kingSafetyScore -= PSQ_K_MID[FLIP[f]] * pos.MatIndex(WHITE) / 32;
    boardScore -= PSQ_K_END[FLIP[f]] * (32 - pos.MatIndex(WHITE)) / 32;

    int penalty = PawnShieldBlack(pentry, f);
    kingSafetyScore -= KING_PAWN_SHIELD[penalty] * pos.MatIndex(WHITE) / 32;
  }

  posScore += mobScore * g_evalWeights.Mobility / 50;
  posScore += pawnStructScore * g_evalWeights.PawnStruct / 50;
  posScore += pawnPassedScore * g_evalWeights.PawnPassed / 50;
  posScore += kingSafetyScore * g_evalWeights.KingSafety / 50;
  posScore += boardScore * g_evalWeights.BoardControl / 50;

  EVAL e = matScore + posScore;
  if (e > 0 && ee == EVAL_WHITE_CANNOT_WIN) e = 0;
  if (e < 0 && ee == EVAL_BLACK_CANNOT_WIN) e = 0;

  // Random
  if (e > -VAL_Q && e < VAL_Q)
  {
    int N = g_evalWeights.RandomEval;
	if (N > 0)
    {
	  e += N - Rand32() % (2 * N);
    }
  }

  if (ee == EVAL_PROBABLE_DRAW) e /= 2;

  return (pos.Side() == WHITE)? e : -e;
}

const int center[64] =
{
  -2, -1,  0,  1,  1,  0, -1, -2,
  -1,  0,  1,  2,  2,  1,  0, -1,
   0,  1,  2,  3,  3,  2,  1,  0,
   0,  1,  2,  3,  3,  2,  1,  0,
  -1,  0,  1,  2,  2,  1,  0, -1,
  -2, -1,  0,  1,  1,  0, -1, -2,
  -3, -2, -1,  0,  0, -1, -2, -3,
  -4, -3, -2, -1, -1, -2, -3, -4
};

const int center_p[64] =
{
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  2,  2,  0,  0,  0,
   0,  0,  1,  2,  2,  1,  0,  0,
   0,  0,  1,  2,  2,  1,  0,  0,
   0,  0,  1,  1,  1,  1,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0
};

const int center_k[64] =
{
   4,  4,  4,  4,  4,  4,  4,  4, 
   4,  4,  4,  4,  4,  4,  4,  4,
   4,  4,  4,  4,  4,  4,  4,  4,
   4,  4,  4,  4,  4,  4,  4,  4,
   4,  4,  4,  4,  4,  4,  4,  4,
   4,  4,  4,  4,  4,  4,  4,  4,
   2,  2,  2,  2,  2,  2,  2,  2,
   1,  0,  0,  1,  0,  1,  0,  1
};

void InitEval()
{
  for (int f = 0; f < 64; ++f)
  {
    PSQ_P[f] = PawnCenter * center_p[f] / 2;
    PSQ_N[f] = KnightCenter * center[f] / 3;
    PSQ_B[f] = BishopCenter * center[f] / 3;
    PSQ_K_MID[f] = KingCenterMid * center_k[f] / 4;
    PSQ_K_END[f] = KingCenterEnd * center[f] / 3;
  }

  for (int m = 0; m < 9; ++m)
    KNIGHT_MOBILITY[m] = (EVAL) (KnightMobility * (m - 4) / 4);
  for (int m = 0; m < 14; ++m)
    BISHOP_MOBILITY[m] = (EVAL) (BishopMobility * (m - 6.5) / 6.5);
  for (int m = 0; m < 15; ++m)    
    ROOK_MOBILITY[m] = (EVAL) (RookMobility * (m - 7) / 7);
  for (int d = 0; d < 8; ++d)
    QUEEN_KING_TROPISM[d] = QueenKingTropism * (7 - d) / 7;
  for (int p = 0; p < 10; ++p)
    KING_PAWN_SHIELD[p] = -KingPawnShield * p / 9;
}

void PawnEntry::Read(const Position& pos)
{
  U64 x;
  FLD f;
  int file, rank;

  m_pawnHash = pos.PawnHash();
  m_score = 0;
  m_passed[WHITE] = m_passed[BLACK] = 0;

  for (file = 0; file < 10; ++file)
  {
    m_ranks[file][WHITE] = 0;
    m_ranks[file][BLACK] = 7;
  }

  x = pos.Bits(PW);
  while (x)
  {
    f = PopLSB(x);
    file = Col(f) + 1;
    rank = Row(f);
    if (rank > m_ranks[file][WHITE])
      m_ranks[file][WHITE] = rank;
  }

  x = pos.Bits(PB);
  while (x)
  {
    f = PopLSB(x);
    file = Col(f) + 1;
    rank = Row(f);
    if (rank < m_ranks[file][BLACK])
      m_ranks[file][BLACK] = rank;
  }

  x = pos.Bits(PW);
  while (x)
  {
    f = PopLSB(x);
    file = Col(f) + 1;
    rank = Row(f);

    m_score += PSQ_P[f];
    if (m_ranks[file][BLACK] == 7)
    {
      if (m_ranks[file - 1][BLACK] >= rank && m_ranks[file + 1][BLACK] >= rank)
        m_passed[WHITE] |= BB_SINGLE[f];
    }
    if (rank != m_ranks[file][WHITE])
      m_score += PawnDoubled;
    if (m_ranks[file - 1][WHITE] == 0 && m_ranks[file + 1][WHITE] == 0)
      m_score += PawnIsolated;
    else if (m_ranks[file - 1][WHITE] < rank && m_ranks[file + 1][WHITE] < rank)
      m_score += PawnBackwards;
  }

  x = pos.Bits(PB);
  while (x)
  {
    f = PopLSB(x);
    file = Col(f) + 1;
    rank = Row(f);

    m_score -= PSQ_P[FLIP[f]];
    if (m_ranks[file][WHITE] == 0)
    {
      if (m_ranks[file - 1][WHITE] <= rank && m_ranks[file + 1][WHITE] <= rank)
        m_passed[BLACK] |= BB_SINGLE[f];
    }
    if (rank != m_ranks[file][BLACK])
      m_score -= PawnDoubled;
    if (m_ranks[file - 1][BLACK] == 7 && m_ranks[file + 1][BLACK] == 7)
      m_score -= PawnIsolated;
    else if (m_ranks[file - 1][BLACK] > rank && m_ranks[file + 1][BLACK] > rank)
      m_score -= PawnBackwards;
  }
}

EVAL VALUE[14] =
{ 0, 0, VAL_P, VAL_P, VAL_N, VAL_N, VAL_B, VAL_B, VAL_R, VAL_R, VAL_Q, VAL_Q, 0, 0 };

/*
enum EVAL_ESTIMATION
{
  EVAL_EXACT_DRAW = 0,
  EVAL_POSSIBLE_DRAW = 1,
  EVAL_WHITE_CANNOT_WIN = 2,
  EVAL_BLACK_CANNOT_WIN = 3,
  EVAL_UNKNOWN = 4
};
*/

EVAL_ESTIMATION EstimateDraw(const Position& pos)
{
  static const int FIELD_COLOR[64] =
  {
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0
  };

  // KBP-K

  if (pos.MatIndex(WHITE) == 3 && pos.Count(PW) == 1)
  {
    if (pos.MatIndex(BLACK) == 0 && pos.Count(PB) == 0)
    {
      FLD pawn = LSB(pos.Bits(PW));
      FLD bishop = LSB(pos.Bits(BW));
      FLD king = pos.King(BLACK);
      if (Col(pawn) == 0)
      {
        if (FIELD_COLOR[A8] != FIELD_COLOR[bishop])
        {
          if (king == A8 || king == A7 || king == B8 || king == B7) return EVAL_PRACTICAL_DRAW;
        }
      }
      if (Col(pawn) == 7)
      {        
        if (FIELD_COLOR[H8] != FIELD_COLOR[bishop])
        {
          if (king == H8 || king == H7 || king == G8 || king == G7) return EVAL_PRACTICAL_DRAW;
        }
      }
    }
  }

  if (pos.MatIndex(BLACK) == 3 && pos.Count(PB) == 1)
  {
    if (pos.MatIndex(WHITE) == 0 && pos.Count(PW) == 0)
    {
      FLD pawn = LSB(pos.Bits(PB));
      FLD bishop = LSB(pos.Bits(BB));
      FLD king = pos.King(WHITE);
      if (Col(pawn) == 0)
      {
        if (FIELD_COLOR[A1] != FIELD_COLOR[bishop])
        {
          if (king == A1 || king == A2 || king == B1 || king == B2) return EVAL_PRACTICAL_DRAW;
        }
      }
      if (Col(pawn) == 7)
      {
        if (FIELD_COLOR[H1] != FIELD_COLOR[bishop])
        {
          if (king == H1 || king == H2 || king == G1 || king == G2) return EVAL_PRACTICAL_DRAW;
        }
      }
    }
  }

  // KB-KB

  if (pos.MatIndex(WHITE) == 3 && pos.MatIndex(BLACK) == 3)
  {
    if (pos.Count(BW) == 1 && pos.Count(BB) == 1)
    {
      FLD bw = LSB(pos.Bits(BW));
      FLD bb = LSB(pos.Bits(BB));
      if (FIELD_COLOR[bw] != FIELD_COLOR[bb]) return EVAL_PROBABLE_DRAW;
    }
  }

  // Low material

  if (pos.Count(PW) == 0 && pos.MatIndex(WHITE) < 5)
  {
    if (pos.Count(PB) == 0 && pos.MatIndex(BLACK) < 5) 
      return EVAL_THEORETICAL_DRAW;
    else
      return EVAL_WHITE_CANNOT_WIN;
  }

  if (pos.Count(PB) == 0 && pos.MatIndex(BLACK) < 5)
  {
    if (pos.Count(PW) == 0 && pos.MatIndex(WHITE) < 5) 
      return EVAL_THEORETICAL_DRAW;
    else
      return EVAL_BLACK_CANNOT_WIN;
  }

  return EVAL_UNKNOWN;
}
