
// square.cpp

// includes

#include "colour.h"
#include "square.h"
#include "util.h"

// "constants"

const int SquareFrom64[64] = {
   A1, B1, C1, D1, E1, F1, G1, H1,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A3, B3, C3, D3, E3, F3, G3, H3,
   A4, B4, C4, D4, E4, F4, G4, H4,
   A5, B5, C5, D5, E5, F5, G5, H5,
   A6, B6, C6, D6, E6, F6, G6, H6,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A8, B8, C8, D8, E8, F8, G8, H8,
};

const int RankMask[ColourNb] = { 0, 0xF };
const int PromoteRank[ColourNb] = { 0xB0, 0x40 };

// variables

int SquareTo64[SquareNb];
bool SquareIsPromote[SquareNb];

// functions

// square_init()

void square_init() {

   int sq;

   // SquareTo64[]

   for (sq = 0; sq < SquareNb; sq++) SquareTo64[sq] = -1;

   for (sq = 0; sq < 64; sq++) {
      SquareTo64[SquareFrom64[sq]] = sq;
   }

   // SquareIsPromote[]

   for (sq = 0; sq < SquareNb; sq++) {
      SquareIsPromote[sq] = SQUARE_IS_OK(sq) && (SQUARE_RANK(sq) == Rank1 || SQUARE_RANK(sq) == Rank8);
   }
}

// file_from_char()

int file_from_char(int c) {

   ASSERT(c>='a'&&c<='h');

   return FileA + (c - 'a');
}

// rank_from_char()

int rank_from_char(int c) {

   ASSERT(c>='1'&&c<='8');

   return Rank1 + (c - '1');
}

// file_to_char()

int file_to_char(int file) {

   ASSERT(file>=FileA&&file<=FileH);

   return 'a' + (file - FileA);
}

// rank_to_char()

int rank_to_char(int rank) {

   ASSERT(rank>=Rank1&&rank<=Rank8);

   return '1' + (rank - Rank1);
}

// square_to_string()

bool square_to_string(int square, char string[], int size) {

   ASSERT(SQUARE_IS_OK(square));
   ASSERT(string!=NULL);
   ASSERT(size>=3);

   if (size < 3) return false;

   string[0] = file_to_char(SQUARE_FILE(square));
   string[1] = rank_to_char(SQUARE_RANK(square));
   string[2] = '\0';

   return true;
}

// square_from_string()

int square_from_string(const char string[]) {

   int file, rank;

   ASSERT(string!=NULL);

   if (string[0] < 'a' || string[0] > 'h') return SquareNone;
   if (string[1] < '1' || string[1] > '8') return SquareNone;
   if (string[2] != '\0') return SquareNone;

   file = file_from_char(string[0]);
   rank = rank_from_char(string[1]);

   return SQUARE_MAKE(file,rank);
}

// end of square.cpp

