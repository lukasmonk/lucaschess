
// move_do.h

#ifndef MOVE_DO_H
#define MOVE_DO_H

// includes

#include "board.h"
#include "util.h"

// types

struct undo_t {

   bool capture;

   int capture_square;
   int capture_piece;
   int capture_pos;

   int pawn_pos;

   int turn;
   int flags;
   int ep_square;
   int ply_nb;

   int cap_sq;

   int opening;
   int endgame;

   uint64 key;
   uint64 pawn_key;
   uint64 material_key;
};

// functions

extern void move_do_init   ();

extern void move_do        (board_t * board, int move, undo_t * undo);
extern void move_undo      (board_t * board, int move, const undo_t * undo);

extern void move_do_null   (board_t * board, undo_t * undo);
extern void move_undo_null (board_t * board, const undo_t * undo);

#endif // !defined MOVE_DO_H

// end of move_do.h

