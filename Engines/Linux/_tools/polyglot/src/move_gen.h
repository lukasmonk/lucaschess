
// move_gen.h

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

// includes

#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern void gen_legal_moves (list_t * list, const board_t * board);
extern void gen_moves       (list_t * list, const board_t * board);

#endif // !defined MOVE_GEN_H

// end of move_gen.h

