
// book.h

#ifndef BOOK_H
#define BOOK_H

// includes

#include "board.h"
#include "util.h"

// functions

extern void book_init  ();

extern void book_open  (const char file_name[]);
extern void book_close ();

extern int  book_move  (board_t * board);

#endif // !defined BOOK_H

// end of book.h

