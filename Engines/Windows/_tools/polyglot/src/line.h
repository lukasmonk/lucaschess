
// line.h

#ifndef LINE_H
#define LINE_H

// includes

#include "board.h"
#include "move.h"
#include "util.h"

// constants

const int LineSize = 256;

// functions

extern bool line_is_ok    (const move_t line[]);

extern void line_clear    (move_t line[]);
extern void line_copy     (move_t dst[], const move_t src[]);

extern bool line_from_can (move_t line[], const board_t * board, const char string[], int size);

extern bool line_to_can   (const move_t line[], const board_t * board, char string[], int size);
extern bool line_to_san   (const move_t line[], const board_t * board, char string[], int size);

#endif // !defined LINE_H

// end of line.h

