
// attack.h

#ifndef ATTACK_H
#define ATTACK_H

// includes

#include "board.h"
#include "util.h"

// constants

const int IncNone = 0;

// "constants"

extern const sint8 KnightInc[8+1];
extern const sint8 BishopInc[4+1];
extern const sint8 RookInc[4+1];
extern const sint8 QueenInc[8+1];
extern const sint8 KingInc[8+1];

// functions

extern void attack_init  ();

extern bool is_in_check  (const board_t * board, int colour);
extern bool is_attacked  (const board_t * board, int to, int colour);
extern bool piece_attack (const board_t * board, int piece, int from, int to);

extern bool is_pinned    (const board_t * board, int from, int to, int colour);

#endif // !defined ATTACK_H

// end of attack.h

