
// list.h

#ifndef LIST_H
#define LIST_H

// includes

#include "board.h"
#include "move.h"
#include "util.h"

// constants

const int ListSize = 256;

// types

struct list_t {
   sint16 size;
   move_t move[ListSize];
   sint16 value[ListSize];
};

// functions

extern bool list_is_ok    (const list_t * list);

extern void list_clear    (list_t * list);
extern void list_add      (list_t * list, int move, int value = 0);
extern void list_remove   (list_t * list, int index);

extern bool list_is_empty (const list_t * list);
extern int  list_size     (const list_t * list);

extern int  list_move     (const list_t * list, int index);
extern int  list_value    (const list_t * list, int index);

extern void list_copy     (list_t * dst, const list_t * src);

extern void list_note     (list_t * list);
extern void list_sort     (list_t * list);

extern bool list_contain  (const list_t * list, int move);
extern bool list_equal    (list_t * list_1, list_t * list_2);

extern void list_disp     (const list_t * list, const board_t * board);

#endif // !defined LIST_H

// end of list.h

