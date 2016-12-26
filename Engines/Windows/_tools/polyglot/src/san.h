
// san.h

#ifndef SAN_H
#define SAN_H

// includes

#include "board.h"
#include "util.h"

// functions

extern bool move_to_san         (int move, const board_t * board, char string[], int size);
extern int  move_from_san       (const char string[], const board_t * board);

extern int  move_from_san_debug (const char string[], const board_t * board);

#endif // !defined SAN_H

// end of san.h

