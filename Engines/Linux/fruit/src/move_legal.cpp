
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

static bool move_is_pseudo_debug (int move, board_t * board);

// functions

// move_is_pseudo()

bool move_is_pseudo(int move, board_t * board) {

   int me, opp;
   int from, to;
   int piece, capture;
   int inc, delta;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!board_is_check(board));

   // special cases

   if (MOVE_IS_SPECIAL(move)) {
      return move_is_pseudo_debug(move,board);
   }

   ASSERT((move&~07777)==0);

   // init

   me = board->turn;
   opp = COLOUR_OPP(board->turn);

   // from

   from = MOVE_FROM(move);
   ASSERT(SQUARE_IS_OK(from));

   piece = board->square[from];
   if (!COLOUR_IS(piece,me)) return false;

   ASSERT(piece_is_ok(piece));

   // to

   to = MOVE_TO(move);
   ASSERT(SQUARE_IS_OK(to));

   capture = board->square[to];
   if (COLOUR_IS(capture,me)) return false;

   // move

   if (PIECE_IS_PAWN(piece)) {

      if (SQUARE_IS_PROMOTE(to)) return false;

      inc = PAWN_MOVE_INC(me);
      delta = to - from;
      ASSERT(delta_is_ok(delta));

      if (capture == Empty) {

         // pawn push

         if (delta == inc) return true;

         if (delta == (2*inc)
          && PAWN_RANK(from,me) == Rank2
          && board->square[from+inc] == Empty) {
            return true;
         }

      } else {

         // pawn capture

         if (delta == (inc-1) || delta == (inc+1)) return true;
      }

   } else {

      if (PIECE_ATTACK(board,piece,from,to)) return true;
   }

   return false;
}

// quiet_is_pseudo()

bool quiet_is_pseudo(int move, board_t * board) {

   int me, opp;
   int from, to;
   int piece;
   int inc, delta;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!board_is_check(board));

   // special cases

   if (MOVE_IS_CASTLE(move)) {
      return move_is_pseudo_debug(move,board);
   } else if (MOVE_IS_SPECIAL(move)) {
      return false;
   }

   ASSERT((move&~07777)==0);

   // init

   me = board->turn;
   opp = COLOUR_OPP(board->turn);

   // from

   from = MOVE_FROM(move);
   ASSERT(SQUARE_IS_OK(from));

   piece = board->square[from];
   if (!COLOUR_IS(piece,me)) return false;

   ASSERT(piece_is_ok(piece));

   // to

   to = MOVE_TO(move);
   ASSERT(SQUARE_IS_OK(to));

   if (board->square[to] != Empty) return false; // capture

   // move

   if (PIECE_IS_PAWN(piece)) {

      if (SQUARE_IS_PROMOTE(to)) return false;

      inc = PAWN_MOVE_INC(me);
      delta = to - from;
      ASSERT(delta_is_ok(delta));

      // pawn push

      if (delta == inc) return true;

      if (delta == (2*inc)
       && PAWN_RANK(from,me) == Rank2
       && board->square[from+inc] == Empty) {
         return true;
      }

   } else {

      if (PIECE_ATTACK(board,piece,from,to)) return true;
   }

   return false;
}

// pseudo_is_legal()

bool pseudo_is_legal(int move, board_t * board) {

   int me, opp;
   int from, to;
   int piece;
   bool legal;
   int king;
   undo_t undo[1];

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   // init

   me = board->turn;
   opp = COLOUR_OPP(me);

   from = MOVE_FROM(move);
   to = MOVE_TO(move);

   piece = board->square[from];
   ASSERT(COLOUR_IS(piece,me));

   // slow test for en-passant captures

   if (MOVE_IS_EN_PASSANT(move)) {

      move_do(board,move,undo);
      legal = !IS_IN_CHECK(board,me);
      move_undo(board,move,undo);

      return legal;
   }

   // king moves (including castle)

   if (PIECE_IS_KING(piece)) {

      legal = !is_attacked(board,to,opp);

      if (DEBUG) {
         ASSERT(board->square[from]==piece);
         board->square[from] = Empty;
         ASSERT(legal==!is_attacked(board,to,opp));
         board->square[from] = piece;
      }

      return legal;
   }

   // pins

   if (is_pinned(board,from,me)) {
      king = KING_POS(board,me);
      return DELTA_INC_LINE(king-to) == DELTA_INC_LINE(king-from); // does not discover the line
   }

   return true;
}

// move_is_pseudo_debug()

static bool move_is_pseudo_debug(int move, board_t * board) {

   list_t list[1];

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!board_is_check(board));

   gen_moves(list,board);

   return list_contain(list,move);
}

// end of move_legal.cpp

