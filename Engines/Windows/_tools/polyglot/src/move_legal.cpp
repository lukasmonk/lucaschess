
// move_legal.cpp

// includes

#include "attack.h"
#include "colour.h"
#include "fen.h"
#include "list.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "move_legal.h"
#include "piece.h"
#include "square.h"
#include "util.h"

// prototypes

#if DEBUG
static bool move_is_legal_debug (int move, const board_t * board);
#endif

// functions

// move_is_pseudo()

bool move_is_pseudo(int move, const board_t * board) {

   list_t list[1];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   gen_moves(list,board);

   return list_contain(list,move);
}

// pseudo_is_legal()

bool pseudo_is_legal(int move, const board_t * board) {

   board_t new_board[1];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   ASSERT(move_is_pseudo(move,board));

   board_copy(new_board,board);
   move_do(new_board,move);

   return !is_in_check(new_board,colour_opp(new_board->turn));
}

// move_is_legal()

bool move_is_legal(int move, const board_t * board) {

   bool legal;

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   legal = move_is_pseudo(move,board) && pseudo_is_legal(move,board);
   ASSERT(legal==move_is_legal_debug(move,board));

   return legal;
}

// filter_legal()

void filter_legal(list_t * list, const board_t * board) {

   int pos;
   int i, move, value;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));

   pos = 0;

   for (i = 0; i < list_size(list); i++) {

      ASSERT(pos>=0&&pos<=i);

      move = list_move(list,i);
      value = list_value(list,i);

      if (pseudo_is_legal(move,board)) {
         list->move[pos] = move;
         list->value[pos] = value;
         pos++;
      }
   }

   ASSERT(pos>=0&&pos<=list_size(list));
   list->size = pos;
}

// move_is_legal_debug()
#if DEBUG
static bool move_is_legal_debug(int move, const board_t * board) {

   list_t list[1];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   gen_legal_moves(list,board);

   return list_contain(list,move);
}
#endif
// end of move_legal.cpp

