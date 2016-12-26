
// square.cpp

// includes

#include "colour.h"
#include "square.h"
#include "util.h"

// "constants"

static const uint8 SquareFrom64[64] = {
   A1, B1, C1, D1, E1, F1, G1, H1,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A3, B3, C3, D3, E3, F3, G3, H3,
   A4, B4, C4, D4, E4, F4, G4, H4,
   A5, B5, C5, D5, E5, F5, G5, H5,
   A6, B6, C6, D6, E6, F6, G6, H6,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A8, B8, C8, D8, E8, F8, G8, H8,
};

// variables

static sint8 SquareTo64[SquareNb];

// functions

// square_init()

void square_init() {

   int sq;

   for (sq = 0; sq < SquareNb; sq++) SquareTo64[sq] = -1;

   for (sq = 0; sq < 64; sq++) {
      SquareTo64[SquareFrom64[sq]] = (sint8)sq;
   }
}

// square_is_ok()

bool square_is_ok(int square) {

   if (square < 0 || square >= SquareNb) return false;

   if (SquareTo64[square] < 0) return false;

   return true;
}

// square_make()

int square_make(int file, int rank) {

   int sq_64;

   ASSERT(file>=0&&file<8);
   ASSERT(rank>=0&&rank<8);

   sq_64 = (rank << 3) | file;

   return square_from_64(sq_64);
}

// square_file()

int square_file(int square) {

   int file;

   ASSERT(square_is_ok(square));

   file = (square - 4) & 7;
   ASSERT(file==(square_to_64(square)&7));

   return file;
}

// square_rank()

int square_rank(int square) {

   int rank;

   ASSERT(square_is_ok(square));

   rank = (square >> 4) - 2;
   ASSERT(rank==square_to_64(square)>>3);

   return rank;
}

// square_side_rank()

int square_side_rank(int square, int colour) {

   int rank;

   ASSERT(square_is_ok(square));
   ASSERT(colour_is_ok(colour));

   rank = square_rank(square);
   if (colour_is_black(colour)) rank = 7-rank;

   return rank;
}

// square_from_64()

int square_from_64(int square) {

   ASSERT(square>=0&&square<64);

   return SquareFrom64[square];
}

// square_to_64()

int square_to_64(int square) {

   ASSERT(square_is_ok(square));

   return SquareTo64[square];
}

// square_is_promote()

bool square_is_promote(int square) {

   int rank;

   ASSERT(square_is_ok(square));

   rank = square_rank(square);

   return rank == Rank1 || rank == Rank8;
}

// square_ep_dual()

int square_ep_dual(int square) {

   ASSERT(square_is_ok(square));
   ASSERT(square_rank(square)>=2&&square_rank(square)<=5);

   return square ^ 16;
}

// square_colour()

int square_colour(int square) {

   ASSERT(square_is_ok(square));

   return (square ^ (square >> 4)) & 1;
}

// file_from_char()

int file_from_char(int c) {

   ASSERT(c>='a'&&c<='h');

   return c - 'a';
}

// rank_from_char()

int rank_from_char(int c) {

   ASSERT(c>='1'&&c<='8');

   return c - '1';
}

// file_to_char()

int file_to_char(int file) {

   ASSERT(file>=0&&file<8);

   return 'a' + file;
}

// rank_to_char()

int rank_to_char(int rank) {

   ASSERT(rank>=0&&rank<8);

   return '1' + rank;
}

// char_is_file()

bool char_is_file(int c) {

   return c >= 'a' && c <= 'h';
}

// char_is_rank()

bool char_is_rank(int c) {

   return c >= '1' && c <= '8';
}

// square_to_string()

bool square_to_string(int square, char string[], int size) {

   ASSERT(square_is_ok(square));
   ASSERT(string!=NULL);
   ASSERT(size>=3);

   if (size < 3) return false;

   string[0] = (char)('a' + square_file(square));
   string[1] = (char)('1' + square_rank(square));
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

   return square_make(file,rank);
}

// end of square.cpp

