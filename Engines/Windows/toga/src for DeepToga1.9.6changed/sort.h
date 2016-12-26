
// sort.h

#ifndef SORT_H
#define SORT_H

// includes

#include "attack.h"
#include "board.h"
#include "list.h"
#include "search.h"
//#include "util.h"

// types

struct sort_t {
   bool in_pv; // in-pv queen promotes good, outta pv bad.  
   int height;
   int trans_killer;
   int killer_1;
   int killer_2;
   int killer_3;
   int killer_4;
   int gen;
   int test;
   int pos;
   int value;
   board_t * board;
   const attack_t * attack;
   list_t list[1];
   list_t bad[1];
};

// constants  WHM moved these here...

const int HistorySize = 12 * 64;

const  int   HistoryMax    = 16384;
const  int   HistoryKiller = 16383;
const  int   HistoryBadCap = 16382;

const uint32 HistTotMax    = 65535 * 32768; // large for "Adaptive Learning"

//externals

extern volatile uint32 HistHit[HistorySize];
extern volatile uint32 HistTot[HistorySize];

// functions

extern void sort_init    ();

extern void sort_init    (sort_t * sort, board_t * board, const attack_t * attack, int depth, int height, int trans_killer, bool in_pv, int ThreadId);
extern int  sort_next    (sort_t * sort, int ThreadId);

extern void sort_init_qs (sort_t * sort, board_t * board, const attack_t * attack, bool check);
extern int  sort_next_qs (sort_t * sort);

extern void good_move    (int move, const board_t * board, int depth, int height, int ThreadId);

extern void history_good (int move, const board_t * board, int ThreadId);
extern void history_bad  (int move, const board_t * board, int ThreadId);

extern void note_moves   (list_t * list, const board_t * board, int height, int trans_killer, int ThreadId);

#if DEBUG
extern bool capture_is_good   (int move, const board_t * board, bool in_pv);
#endif

#endif // !defined SORT_H

// end of sort.h

