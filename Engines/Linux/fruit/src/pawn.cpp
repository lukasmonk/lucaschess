
// pawn.cpp

// includes

#include <cstring>

#include "board.h"
#include "colour.h"
#include "hash.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "square.h"
#include "util.h"

// constants

static const bool UseTable = true;
static const uint32 TableSize = 16384; // 256kB

// types

typedef pawn_info_t entry_t;

struct pawn_t {
   entry_t * table;
   uint32 size;
   uint32 mask;
   uint32 used;
   sint64 read_nb;
   sint64 read_hit;
   sint64 write_nb;
   sint64 write_collision;
};

// constants and variables

static /* const */ int PawnStructureWeight = 256; // 100%

static const int DoubledOpening = 10;
static const int DoubledEndgame = 20;

static const int IsolatedOpening = 10;
static const int IsolatedOpeningOpen = 20;
static const int IsolatedEndgame = 20;

static const int BackwardOpening = 8;
static const int BackwardOpeningOpen = 16;
static const int BackwardEndgame = 10;

static const int CandidateOpeningMin = 5;
static const int CandidateOpeningMax = 55;
static const int CandidateEndgameMin = 10;
static const int CandidateEndgameMax = 110;

// this was moved to eval.cpp

/*
static const int PassedOpeningMin = 10;
static const int PassedOpeningMax = 70;
static const int PassedEndgameMin = 20;
static const int PassedEndgameMax = 140;
*/

static /* const */ int Bonus[RankNb];

// variables

int BitEQ[16];
int BitLT[16];
int BitLE[16];
int BitGT[16];
int BitGE[16];

int BitFirst[0x100];
int BitLast[0x100];
int BitCount[0x100];
int BitRev[0x100];

static pawn_t Pawn[1];

static int BitRank1[RankNb];
static int BitRank2[RankNb];
static int BitRank3[RankNb];

// prototypes

static void pawn_comp_info (pawn_info_t * info, const board_t * board);

// functions

// pawn_init_bit()

void pawn_init_bit() {

   int rank;
   int first, last, count;
   int b, rev;

   // rank-indexed Bit*[]

   for (rank = 0; rank < RankNb; rank++) {

      BitEQ[rank] = 0;
      BitLT[rank] = 0;
      BitLE[rank] = 0;
      BitGT[rank] = 0;
      BitGE[rank] = 0;

      BitRank1[rank] = 0;
      BitRank2[rank] = 0;
      BitRank3[rank] = 0;
   }

   for (rank = Rank1; rank <= Rank8; rank++) {
      BitEQ[rank] = 1 << (rank - Rank1);
      BitLT[rank] = BitEQ[rank] - 1;
      BitLE[rank] = BitLT[rank] | BitEQ[rank];
      BitGT[rank] = BitLE[rank] ^ 0xFF;
      BitGE[rank] = BitGT[rank] | BitEQ[rank];
   }

   for (rank = Rank1; rank <= Rank8; rank++) {
      BitRank1[rank] = BitEQ[rank+1];
      BitRank2[rank] = BitEQ[rank+1] | BitEQ[rank+2];
      BitRank3[rank] = BitEQ[rank+1] | BitEQ[rank+2] | BitEQ[rank+3];
   }

   // bit-indexed Bit*[]

   for (b = 0; b < 0x100; b++) {

      first = Rank8; // HACK for pawn shelter
      last = Rank1; // HACK
      count = 0;
      rev = 0;

      for (rank = Rank1; rank <= Rank8; rank++) {
         if ((b & BitEQ[rank]) != 0) {
            if (rank < first) first = rank;
            if (rank > last) last = rank;
            count++;
            rev |= BitEQ[RANK_OPP(rank)];
         }
      }

      BitFirst[b] = first;
      BitLast[b] = last;
      BitCount[b] = count;
      BitRev[b] = rev;
   }
}

// pawn_init()

void pawn_init() {

   int rank;

   // UCI options

   PawnStructureWeight = (option_get_int("Pawn Structure") * 256 + 50) / 100;

   // bonus

   for (rank = 0; rank < RankNb; rank++) Bonus[rank] = 0;

   Bonus[Rank4] = 26;
   Bonus[Rank5] = 77;
   Bonus[Rank6] = 154;
   Bonus[Rank7] = 256;

   // pawn hash-table

   Pawn->size = 0;
   Pawn->mask = 0;
   Pawn->table = NULL;
}

// pawn_alloc()

void pawn_alloc() {

   ASSERT(sizeof(entry_t)==16);

   if (UseTable) {

      Pawn->size = TableSize;
      Pawn->mask = TableSize - 1;
      Pawn->table = (entry_t *) my_malloc(Pawn->size*sizeof(entry_t));

      pawn_clear();
   }
}

// pawn_clear()

void pawn_clear() {

   if (Pawn->table != NULL) {
      memset(Pawn->table,0,Pawn->size*sizeof(entry_t));
   }

   Pawn->used = 0;
   Pawn->read_nb = 0;
   Pawn->read_hit = 0;
   Pawn->write_nb = 0;
   Pawn->write_collision = 0;
}

// pawn_get_info()

void pawn_get_info(pawn_info_t * info, const board_t * board) {

   uint64 key;
   entry_t * entry;

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);

   // probe

   if (UseTable) {

      Pawn->read_nb++;

      key = board->pawn_key;
      entry = &Pawn->table[KEY_INDEX(key)&Pawn->mask];

      if (entry->lock == KEY_LOCK(key)) {

         // found

         Pawn->read_hit++;

         *info = *entry;

         return;
      }
   }

   // calculation

   pawn_comp_info(info,board);

   // store

   if (UseTable) {

      Pawn->write_nb++;

      if (entry->lock == 0) { // HACK: assume free entry
         Pawn->used++;
      } else {
         Pawn->write_collision++;
      }

      *entry = *info;
      entry->lock = KEY_LOCK(key);
   }
}

// pawn_comp_info()

static void pawn_comp_info(pawn_info_t * info, const board_t * board) {

   int colour;
   int file, rank;
   int me, opp;
   const sq_t * ptr;
   int sq;
   bool backward, candidate, doubled, isolated, open, passed;
   int t1, t2;
   int n;
   int bits;
   int opening[ColourNb], endgame[ColourNb];
   int flags[ColourNb];
   int file_bits[ColourNb];
   int passed_bits[ColourNb];
   int single_file[ColourNb];

   ASSERT(info!=NULL);
   ASSERT(board!=NULL);

   // pawn_file[]

#if DEBUG
   for (colour = 0; colour < ColourNb; colour++) {

      int pawn_file[FileNb];

      me = colour;

      for (file = 0; file < FileNb; file++) {
         pawn_file[file] = 0;
      }

      for (ptr = &board->pawn[me][0]; (sq=*ptr) != SquareNone; ptr++) {

         file = SQUARE_FILE(sq);
         rank = PAWN_RANK(sq,me);

         ASSERT(file>=FileA&&file<=FileH);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         pawn_file[file] |= BIT(rank);
      }

      for (file = 0; file < FileNb; file++) {
         if (board->pawn_file[colour][file] != pawn_file[file]) my_fatal("board->pawn_file[][]\n");
      }
   }
#endif

   // init

   for (colour = 0; colour < ColourNb; colour++) {

      opening[colour] = 0;
      endgame[colour] = 0;

      flags[colour] = 0;
      file_bits[colour] = 0;
      passed_bits[colour] = 0;
      single_file[colour] = SquareNone;
   }

   // features and scoring

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      for (ptr = &board->pawn[me][0]; (sq=*ptr) != SquareNone; ptr++) {

         // init

         file = SQUARE_FILE(sq);
         rank = PAWN_RANK(sq,me);

         ASSERT(file>=FileA&&file<=FileH);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         // flags

         file_bits[me] |= BIT(file);
         if (rank == Rank2) flags[me] |= BackRankFlag;

         // features

         backward = false;
         candidate = false;
         doubled = false;
         isolated = false;
         open = false;
         passed = false;

         t1 = board->pawn_file[me][file-1] | board->pawn_file[me][file+1];
         t2 = board->pawn_file[me][file] | BitRev[board->pawn_file[opp][file]];

         // doubled

         if ((board->pawn_file[me][file] & BitLT[rank]) != 0) {
            doubled = true;
         }

         // isolated and backward

         if (t1 == 0) {

            isolated = true;

         } else if ((t1 & BitLE[rank]) == 0) {

            backward = true;

            // really backward?

            if ((t1 & BitRank1[rank]) != 0) {

               ASSERT(rank+2<=Rank8);

               if (((t2 & BitRank1[rank])
                  | ((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitRank2[rank])) == 0) {

                  backward = false;
               }

            } else if (rank == Rank2 && ((t1 & BitEQ[rank+2]) != 0)) {

               ASSERT(rank+3<=Rank8);

               if (((t2 & BitRank2[rank])
                  | ((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitRank3[rank])) == 0) {

                  backward = false;
               }
            }
         }

         // open, candidate and passed

         if ((t2 & BitGT[rank]) == 0) {

            open = true;

            if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) {

               passed = true;
               passed_bits[me] |= BIT(file);

            } else {

               // candidate?

               n = 0;

               n += BIT_COUNT(board->pawn_file[me][file-1]&BitLE[rank]);
               n += BIT_COUNT(board->pawn_file[me][file+1]&BitLE[rank]);

               n -= BIT_COUNT(BitRev[board->pawn_file[opp][file-1]]&BitGT[rank]);
               n -= BIT_COUNT(BitRev[board->pawn_file[opp][file+1]]&BitGT[rank]);

               if (n >= 0) {

                  // safe?

                  n = 0;

                  n += BIT_COUNT(board->pawn_file[me][file-1]&BitEQ[rank-1]);
                  n += BIT_COUNT(board->pawn_file[me][file+1]&BitEQ[rank-1]);

                  n -= BIT_COUNT(BitRev[board->pawn_file[opp][file-1]]&BitEQ[rank+1]);
                  n -= BIT_COUNT(BitRev[board->pawn_file[opp][file+1]]&BitEQ[rank+1]);

                  if (n >= 0) candidate = true;
               }
            }
         }

         // score

         if (doubled) {
            opening[me] -= DoubledOpening;
            endgame[me] -= DoubledEndgame;
         }

         if (isolated) {
            if (open) {
               opening[me] -= IsolatedOpeningOpen;
               endgame[me] -= IsolatedEndgame;
            } else {
               opening[me] -= IsolatedOpening;
               endgame[me] -= IsolatedEndgame;
            }
         }

         if (backward) {
            if (open) {
               opening[me] -= BackwardOpeningOpen;
               endgame[me] -= BackwardEndgame;
            } else {
               opening[me] -= BackwardOpening;
               endgame[me] -= BackwardEndgame;
            }
         }

         if (candidate) {
            opening[me] += quad(CandidateOpeningMin,CandidateOpeningMax,rank);
            endgame[me] += quad(CandidateEndgameMin,CandidateEndgameMax,rank);
         }

         // this was moved to the dynamic evaluation

/*
         if (passed) {
            opening[me] += quad(PassedOpeningMin,PassedOpeningMax,rank);
            endgame[me] += quad(PassedEndgameMin,PassedEndgameMax,rank);
         }
*/
      }
   }

   // store info

   info->opening = ((opening[White] - opening[Black]) * PawnStructureWeight) / 256;
   info->endgame = ((endgame[White] - endgame[Black]) * PawnStructureWeight) / 256;

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      // draw flags

      bits = file_bits[me];

      if (bits != 0 && (bits & (bits-1)) == 0) { // one set bit

         file = BIT_FIRST(bits);
         rank = BIT_FIRST(board->pawn_file[me][file]);
         ASSERT(rank>=Rank2);

         if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) {
            rank = BIT_LAST(board->pawn_file[me][file]);
            single_file[me] = SQUARE_MAKE(file,rank);
         }
      }

      info->flags[colour] = flags[colour];
      info->passed_bits[colour] = passed_bits[colour];
      info->single_file[colour] = single_file[colour];
   }
}

// quad()

int quad(int y_min, int y_max, int x) {

   int y;

   ASSERT(y_min>=0&&y_min<=y_max&&y_max<=+32767);
   ASSERT(x>=Rank2&&x<=Rank7);

   y = y_min + ((y_max - y_min) * Bonus[x] + 128) / 256;
   ASSERT(y>=y_min&&y<=y_max);

   return y;
}

// end of pawn.cpp

