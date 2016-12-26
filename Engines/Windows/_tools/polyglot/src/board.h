
// board.h

#ifndef BOARD_H
#define BOARD_H

// includes

#include "colour.h"
#include "square.h"
#include "util.h"

// constants

const int Empty = 0;

const int SideH = 0;
const int SideA = 1;
const int SideNb = 2;

// types

struct board_t {

   uint8 square[SquareNb];
   sint8 pos[SquareNb];

   uint8 list[ColourNb][32];
   sint8 list_size[ColourNb];

   sint8 number[12];

   sint8 turn;
   uint8 castle[ColourNb][SideNb];
   uint8 ep_square;

   sint16 ply_nb;
   sint16 move_nb;

   uint64 key;
};

// functions

extern bool board_is_ok        (const board_t * board);

extern void board_clear        (board_t * board);
extern void board_start        (board_t * board);

extern void board_copy         (board_t * dst, const board_t * src);
extern bool board_equal        (const board_t * board_1, const board_t * board_2);

extern void board_init_list    (board_t * board);

extern int  board_flags        (const board_t * board);

extern bool board_can_play     (const board_t * board);
extern int  board_mobility     (const board_t * board);

extern bool board_is_check     (const board_t * board);
extern bool board_is_mate      (const board_t * board);
extern bool board_is_stalemate (const board_t * board);

extern int  king_pos           (const board_t * board, int colour);

extern void board_disp         (const board_t * board);

#endif // !defined BOARD_H

// end of board.h

