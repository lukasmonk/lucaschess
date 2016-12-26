
// sort.h

#ifndef SORT_H
#define SORT_H

// includes

#include "attack.h"
#include "board.h"
#include "list.h"
#include "util.h"

// types

struct sort_t {
   int depth;
   int height;
   int trans_killer;
   int killer_1;
   int killer_2;
   int gen;
   int test;
   int pos;
   int value;
   board_t * board;
   const attack_t * attack;
   list_t list[1];
   list_t bad[1];
};

// functions

extern void sort_init    ();

extern void sort_init    (sort_t * sort, board_t * board, const attack_t * attack, int depth, int height, int trans_killer);
extern int  sort_next    (sort_t * sort);

extern void sort_init_qs (sort_t * sort, board_t * board, const attack_t * attack, bool check);
extern int  sort_next_qs (sort_t * sort);

extern void good_move    (int move, const board_t * board, int depth, int height);

extern void history_good (int move, const board_t * board);
extern void history_bad  (int move, const board_t * board);

extern void note_moves   (list_t * list, const board_t * board, int height, int trans_killer);

#endif // !defined SORT_H

// end of sort.h

