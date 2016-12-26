#ifndef _WIN32

// search.h

#ifndef SEARCH_H
#define SEARCH_H

// includes

#include "board.h"
#include "util.h"

// constants

const int DepthMax = 63;

// functions

extern void search       (const board_t * board, int depth_max, double time_max);
extern void search_perft (const board_t * board, int depth_max);

#endif // !defined SEARCH_H

// end of search.h

#endif
