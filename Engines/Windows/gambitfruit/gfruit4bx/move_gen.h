
// move_gen.h

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

// includes

#include "attack.h"
#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern void gen_legal_moves (list_t * list, board_t * board);

extern void gen_moves       (list_t * list, const board_t * board);
extern void gen_captures    (list_t * list, const board_t * board);
extern void gen_quiet_moves (list_t * list, const board_t * board);

extern void add_pawn_move   (list_t * list, int from, int to);
extern void add_promote     (list_t * list, int move);

#endif // !defined MOVE_GEN_H

// end of move_gen.h

