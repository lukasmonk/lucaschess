
// move_legal.h

#ifndef MOVE_LEGAL_H
#define MOVE_LEGAL_H

// includes

#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern bool move_is_pseudo  (int move, const board_t * board);
extern bool pseudo_is_legal (int move, const board_t * board);
extern bool move_is_legal   (int move, const board_t * board);

extern void filter_legal    (list_t * list, const board_t * board);

#endif // !defined MOVE_LEGAL_H

// end of move_legal.h

