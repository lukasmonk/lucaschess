
// search_full.h

#ifndef SEARCH_FULL_H
#define SEARCH_FULL_H

// includes

#include "board.h"
#include "util.h"

// functions

extern void search_full_init (list_t * list, board_t * board);
extern int  search_full_root (list_t * list, board_t * board, int depth, int search_type);

#endif // !defined SEARCH_FULL_H

// end of search_full.h

