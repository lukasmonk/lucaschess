
// move_check.h

#ifndef MOVE_CHECK_H
#define MOVE_CHECK_H

// includes

#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern void gen_quiet_checks (list_t * list, board_t * board);

extern bool move_is_check    (int move, board_t * board);

#endif // !defined MOVE_CHECK_H

// end of move_check.h

