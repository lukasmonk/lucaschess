
// hash.cpp

// includes

#include "board.h"
#include "hash.h"
#include "piece.h"
#include "random.h"
#include "square.h"
#include "util.h"

// variables

static uint64 Castle64[16];

// prototypes

static uint64 hash_castle_key_debug (int flags);

// functions

// hash_init()

void hash_init() {

   int i;

   for (i = 0; i < 16; i++) Castle64[i] = hash_castle_key_debug(i);
}

// hash_key()

uint64 hash_key(const board_t * board) {

   uint64 key;
   int colour;
   const uint8 * ptr;
   int sq, piece;

   ASSERT(board_is_ok(board));

   // init

   key = 0;

   // pieces

   for (colour = 1; colour <= 2; colour++) { // HACK
      for (ptr = board->list[colour]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         key ^= hash_piece_key(piece,sq);
      }
   }

   // castle flags

   key ^= hash_castle_key(board_flags(board));

   // en-passant square

   sq = board->ep_square;
   if (sq != SquareNone) key ^= hash_ep_key(sq);

   // turn

   key ^= hash_turn_key(board->turn);

   return key;
}

// hash_piece_key()

uint64 hash_piece_key(int piece, int square) {

   ASSERT(piece_is_ok(piece));
   ASSERT(square_is_ok(square));

   return random_64(RandomPiece+piece_to_12(piece)*64+square_to_64(square));
}

// hash_castle_key()

uint64 hash_castle_key(int flags) {

   ASSERT((flags&~0xF)==0);

   return Castle64[flags];
}

// hash_castle_key_debug()

static uint64 hash_castle_key_debug(int flags) {

   uint64 key;
   int i;

   ASSERT((flags&~0xF)==0);

   key = 0;

   for (i = 0; i < 4; i++) {
      if ((flags & (1<<i)) != 0) key ^= random_64(RandomCastle+i);
   }

   return key;
}

// hash_ep_key()

uint64 hash_ep_key(int square) {

   ASSERT(square_is_ok(square));

   return random_64(RandomEnPassant+square_file(square));
}

// hash_turn_key()

uint64 hash_turn_key(int colour) {

   ASSERT(colour_is_ok(colour));

   return (colour_is_white(colour)) ? random_64(RandomTurn) : 0;
}

// end of hash.cpp

