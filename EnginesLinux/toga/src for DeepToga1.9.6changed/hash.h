
// hash.h

#ifndef HASH_H
#define HASH_H

// includes

#include "board.h"
#include "util.h"

// macros

#define KEY_INDEX(key) ((uint32)(key))
#define KEY_LOCK(key)  ((uint32)((key)>>32))

// constants

const int RandomPiece     =   0; // 12 * 64
const int RandomCastle    = 768; // 4
const int RandomEnPassant = 772; // 8
const int RandomTurn      = 780; // 1

// variables

extern uint64 Castle64[16];

// functions

extern void   hash_init         ();

extern uint64 hash_key          (const board_t * board);
extern uint64 hash_pawn_key     (const board_t * board);
extern uint64 hash_material_key (const board_t * board);

extern uint64 hash_piece_key    (int piece, int square);
extern uint64 hash_castle_key   (int flags);
extern uint64 hash_ep_key       (int square);
extern uint64 hash_turn_key     (int colour);

#endif // !defined HASH_H

// end of hash.h

