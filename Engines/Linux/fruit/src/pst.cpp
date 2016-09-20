
// pst.cpp

// includes

#include "option.h"
#include "piece.h"
#include "pst.h"
#include "util.h"

// macros

#define P(piece_12,square_64,stage) (Pst[(piece_12)][(square_64)][(stage)])

// constants

static const int A1=000, B1=001, C1=002, D1=003, E1=004, F1=005, G1=006, H1=007;
static const int A2=010, B2=011, C2=012, D2=013, E2=014, F2=015, G2=016, H2=017;
static const int A3=020, B3=021, C3=022, D3=023, E3=024, F3=025, G3=026, H3=027;
static const int A4=030, B4=031, C4=032, D4=033, E4=034, F4=035, G4=036, H4=037;
static const int A5=040, B5=041, C5=042, D5=043, E5=044, F5=045, G5=046, H5=047;
static const int A6=050, B6=051, C6=052, D6=053, E6=054, F6=055, G6=056, H6=057;
static const int A7=060, B7=061, C7=062, D7=063, E7=064, F7=065, G7=066, H7=067;
static const int A8=070, B8=071, C8=072, D8=073, E8=074, F8=075, G8=076, H8=077;

// constants and variables

static /* const */ int PieceActivityWeight = 256; // 100%
static /* const */ int KingSafetyWeight = 256; // 100%
static /* const */ int PawnStructureWeight = 256; // 100%

static const int PawnFileOpening = 5;
static const int KnightCentreOpening = 5;
static const int KnightCentreEndgame = 5;
static const int KnightRankOpening = 5;
static const int KnightBackRankOpening = 0;
static const int KnightTrapped = 100;
static const int BishopCentreOpening = 2;
static const int BishopCentreEndgame = 3;
static const int BishopBackRankOpening = 10;
static const int BishopDiagonalOpening = 4;
static const int RookFileOpening = 3;
static const int QueenCentreOpening = 0;
static const int QueenCentreEndgame = 4;
static const int QueenBackRankOpening = 5;
static const int KingCentreEndgame = 12;
static const int KingFileOpening = 10;
static const int KingRankOpening = 10;

// "constants"

static const int PawnFile[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int KnightLine[8] = {
   -4, -2, +0, +1, +1, +0, -2, -4,
};

static const int KnightRank[8] = {
   -2, -1, +0, +1, +2, +3, +2, +1,
};

static const int BishopLine[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int RookFile[8] = {
   -2, -1, +0, +1, +1, +0, -1, -2,
};

static const int QueenLine[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int KingLine[8] = {
   -3, -1, +0, +1, +1, +0, -1, -3,
};

static const int KingFile[8] = {
   +3, +4, +2, +0, +0, +2, +4, +3,
};

static const int KingRank[8] = {
   +1, +0, -2, -3, -4, -5, -6, -7,
};

// variables

sint16 Pst[12][64][StageNb];

// prototypes

static int square_make (int file, int rank);

static int square_file (int square);
static int square_rank (int square);
static int square_opp  (int square);

// functions

// pst_init()

void pst_init() {

   int i;
   int piece, sq, stage;

   // UCI options

   PieceActivityWeight = (option_get_int("Piece Activity") * 256 + 50) / 100;
   KingSafetyWeight    = (option_get_int("King Safety")    * 256 + 50) / 100;
   PawnStructureWeight = (option_get_int("Pawn Structure") * 256 + 50) / 100;

   // init

   for (piece = 0; piece < 12; piece++) {
      for (sq = 0; sq < 64; sq++) {
         for (stage = 0; stage < StageNb; stage++) {
            P(piece,sq,stage) = 0;
         }
      }
   }

   // pawns

   piece = WhitePawn12;

   // file

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += PawnFile[square_file(sq)] * PawnFileOpening;
   }

   // centre control

   P(piece,D3,Opening) += 10;
   P(piece,E3,Opening) += 10;

   P(piece,D4,Opening) += 20;
   P(piece,E4,Opening) += 20;

   P(piece,D5,Opening) += 10;
   P(piece,E5,Opening) += 10;

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PawnStructureWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PawnStructureWeight) / 256;
   }

   // knights

   piece = WhiteKnight12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KnightLine[square_file(sq)] * KnightCentreOpening;
      P(piece,sq,Opening) += KnightLine[square_rank(sq)] * KnightCentreOpening;
      P(piece,sq,Endgame) += KnightLine[square_file(sq)] * KnightCentreEndgame;
      P(piece,sq,Endgame) += KnightLine[square_rank(sq)] * KnightCentreEndgame;
   }

   // rank

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KnightRank[square_rank(sq)] * KnightRankOpening;
   }

   // back rank

   for (sq = A1; sq <= H1; sq++) { // HACK: only first rank
      P(piece,sq,Opening) -= KnightBackRankOpening;
   }

   // "trapped"

   P(piece,A8,Opening) -= KnightTrapped;
   P(piece,H8,Opening) -= KnightTrapped;

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceActivityWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceActivityWeight) / 256;
   }

   // bishops

   piece = WhiteBishop12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += BishopLine[square_file(sq)] * BishopCentreOpening;
      P(piece,sq,Opening) += BishopLine[square_rank(sq)] * BishopCentreOpening;
      P(piece,sq,Endgame) += BishopLine[square_file(sq)] * BishopCentreEndgame;
      P(piece,sq,Endgame) += BishopLine[square_rank(sq)] * BishopCentreEndgame;
   }

   // back rank

   for (sq = A1; sq <= H1; sq++) { // HACK: only first rank
      P(piece,sq,Opening) -= BishopBackRankOpening;
   }

   // main diagonals

   for (i = 0; i < 8; i++) {
      sq = square_make(i,i);
      P(piece,sq,Opening) += BishopDiagonalOpening;
      P(piece,square_opp(sq),Opening) += BishopDiagonalOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceActivityWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceActivityWeight) / 256;
   }

   // rooks

   piece = WhiteRook12;

   // file

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += RookFile[square_file(sq)] * RookFileOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceActivityWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceActivityWeight) / 256;
   }

   // queens

   piece = WhiteQueen12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += QueenLine[square_file(sq)] * QueenCentreOpening;
      P(piece,sq,Opening) += QueenLine[square_rank(sq)] * QueenCentreOpening;
      P(piece,sq,Endgame) += QueenLine[square_file(sq)] * QueenCentreEndgame;
      P(piece,sq,Endgame) += QueenLine[square_rank(sq)] * QueenCentreEndgame;
   }

   // back rank

   for (sq = A1; sq <= H1; sq++) { // HACK: only first rank
      P(piece,sq,Opening) -= QueenBackRankOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * PieceActivityWeight) / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceActivityWeight) / 256;
   }

   // kings

   piece = WhiteKing12;

   // centre

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Endgame) += KingLine[square_file(sq)] * KingCentreEndgame;
      P(piece,sq,Endgame) += KingLine[square_rank(sq)] * KingCentreEndgame;
   }

   // file

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KingFile[square_file(sq)] * KingFileOpening;
   }

   // rank

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) += KingRank[square_rank(sq)] * KingRankOpening;
   }

   // weight

   for (sq = 0; sq < 64; sq++) {
      P(piece,sq,Opening) = (P(piece,sq,Opening) * KingSafetyWeight)    / 256;
      P(piece,sq,Endgame) = (P(piece,sq,Endgame) * PieceActivityWeight) / 256;
   }

   // symmetry copy for black

   for (piece = 0; piece < 12; piece += 2) { // HACK
      for (sq = 0; sq < 64; sq++) {
         for (stage = 0; stage < StageNb; stage++) {
            P(piece+1,sq,stage) = -P(piece,square_opp(sq),stage); // HACK
         }
      }
   }
}

// square_make()

static int square_make(int file, int rank) {

   ASSERT(file>=0&&file<8);
   ASSERT(rank>=0&&rank<8);

   return (rank << 3) | file;
}

// square_file()

static int square_file(int square) {

   ASSERT(square>=0&&square<64);

   return square & 7;
}

// square_rank()

static int square_rank(int square) {

   ASSERT(square>=0&&square<64);

   return square >> 3;
}

// square_opp()

static int square_opp(int square) {

   ASSERT(square>=0&&square<64);

   return square ^ 070;
}

// end of pst.cpp

