
// book.h

#ifndef BOOK_H
#define BOOK_H

// includes

#include "board.h"
#include "util.h"

// functions

extern void book_clear      ();

extern int book_open       (const char file_name[]);
extern void book_close      ();

extern bool is_in_book      (const board_t * board);
extern int  book_move       (const board_t * board, bool random);
extern void book_disp       (const board_t * board);

extern void book_learn_move (const board_t * board, int move, int result);
extern void book_flush      ();

#endif // !defined BOOK_H

// end of book.h

