
// hash.cpp

// includes

#include "board.h"
#include "hash.h"
#include "piece.h"
#include "random.h"
#include "square.h"
#include "util.h"

// variables

uint64 Castle64[16];

// prototypes

static uint64 hash_counter_key (int piece_12, int count);

// functions

// hash_init()

void hash_init() {

   int i;

   for (i = 0; i < 16; i++) Castle64[i] = hash_castle_key(i);
}

// hash_key()

uint64 hash_key(const board_t * board) {

   uint64 key;
   int colour;
   const sq_t * ptr;
   int sq, piece;

   ASSERT(board!=NULL);

   // init

   key = 0;

   // pieces

   for (colour = 0; colour < ColourNb; colour++) {

      for (ptr = &board->piece[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         key ^= hash_piece_key(piece,sq);
      }

      for (ptr = &board->pawn[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         key ^= hash_piece_key(piece,sq);
      }
   }

   // castle flags

   key ^= hash_castle_key(board->flags);

   // en-passant square

   sq = board->ep_square;
   if (sq != SquareNone) key ^= hash_ep_key(sq);

   // turn

   key ^= hash_turn_key(board->turn);

   return key;
}

// hash_pawn_key()

uint64 hash_pawn_key(const board_t * board) {

   uint64 key;
   int colour;
   const sq_t * ptr;
   int sq, piece;

   ASSERT(board!=NULL);

   // init

   key = 0;

   // pawns

   for (colour = 0; colour < ColourNb; colour++) {
      for (ptr = &board->pawn[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         key ^= hash_piece_key(piece,sq);
      }
   }

   return key;
}

// hash_material_key()

uint64 hash_material_key(const board_t * board) {

   uint64 key;
   int piece_12, count;

   ASSERT(board!=NULL);

   // init

   key = 0;

   // counters

   for (piece_12 = 0; piece_12 < 12; piece_12++) {
      count = board->number[piece_12];
      key ^= hash_counter_key(piece_12,count);
   }

   return key;
}

// hash_piece_key()

uint64 hash_piece_key(int piece, int square) {

   ASSERT(piece_is_ok(piece));
   ASSERT(SQUARE_IS_OK(square));

   return RANDOM_64(RandomPiece+(PIECE_TO_12(piece)^1)*64+SQUARE_TO_64(square)); // HACK: ^1 for PolyGlot book
}

// hash_castle_key()

uint64 hash_castle_key(int flags) {

   uint64 key;
   int i;

   ASSERT((flags&~0xF)==0);

   key = 0;

   for (i = 0; i < 4; i++) {
      if ((flags & (1<<i)) != 0) key ^= RANDOM_64(RandomCastle+i);
   }

   return key;
}

// hash_ep_key()

uint64 hash_ep_key(int square) {

   ASSERT(SQUARE_IS_OK(square));

   return RANDOM_64(RandomEnPassant+SQUARE_FILE(square)-FileA);
}

// hash_turn_key()

uint64 hash_turn_key(int colour) {

   ASSERT(COLOUR_IS_OK(colour));

   return (COLOUR_IS_WHITE(colour)) ? RANDOM_64(RandomTurn) : 0;
}

// hash_counter_key()

static uint64 hash_counter_key(int piece_12, int count) {

   uint64 key;
   int i, index;

   ASSERT(piece_12>=0&&piece_12<12);
   ASSERT(count>=0&&count<=10);

   // init

   key = 0;

   // counter

   index = piece_12 * 16;
   for (i = 0; i < count; i++) key ^= RANDOM_64(index+i);

   return key;
}

// end of hash.cpp

