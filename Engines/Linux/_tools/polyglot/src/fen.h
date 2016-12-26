
// fen.h

#ifndef FEN_H
#define FEN_H

// includes

#include "board.h"
#include "util.h"

// "constants"

extern const char * StartFen;

// functions

extern bool board_from_fen (board_t * board, const char string[]);
extern bool board_to_fen   (const board_t * board, char string[], int size);

#endif // !defined FEN_H

// end of fen.h

