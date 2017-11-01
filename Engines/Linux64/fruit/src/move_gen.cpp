
// move_gen.cpp

// includes

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "list.h"
#include "move.h"
#include "move_evasion.h"
#include "move_gen.h"
#include "move_legal.h"
#include "piece.h"
#include "util.h"

// prototypes

static void add_moves               (list_t * list, const board_t * board);
static void add_captures            (list_t * list, const board_t * board);
static void add_quiet_moves         (list_t * list, const board_t * board);

static void add_promotes            (list_t * list, const board_t * board);
static void add_en_passant_captures (list_t * list, const board_t * board);
static void add_castle_moves        (list_t * list, const board_t * board);

// functions

// gen_legal_moves()

void gen_legal_moves(list_t * list, board_t * board) {

   attack_t attack[1];

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   attack_set(attack,board);

   if (ATTACK_IN_CHECK(attack)) {
      gen_legal_evasions(list,board,attack);
   } else {
      gen_moves(list,board);
      list_filter(list,board,&pseudo_is_legal,true);
   }

   // debug

   ASSERT(list_is_ok(list));
}

// gen_moves()

void gen_moves(list_t * list, const board_t * board) {

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   ASSERT(!board_is_check(board));

   LIST_CLEAR(list);

   add_moves(list,board);

   add_en_passant_captures(list,board);
   add_castle_moves(list,board);

   // debug

   ASSERT(list_is_ok(list));
}

// gen_captures()

void gen_captures(list_t * list, const board_t * board) {

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   LIST_CLEAR(list);

   add_captures(list,board);
   add_en_passant_captures(list,board);

   // debug

   ASSERT(list_is_ok(list));
}

// gen_quiet_moves()

void gen_quiet_moves(list_t * list, const board_t * board) {

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   ASSERT(!board_is_check(board));

   LIST_CLEAR(list);

   add_quiet_moves(list,board);
   add_castle_moves(list,board);

   // debug

   ASSERT(list_is_ok(list));
}

// add_moves()

static void add_moves(list_t * list, const board_t * board) {

   int me, opp;
   int opp_flag;
   const sq_t * ptr;
   int from, to;
   int piece, capture;
   const inc_t * inc_ptr;
   int inc;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   me = board->turn;
   opp = COLOUR_OPP(me);

   opp_flag = COLOUR_FLAG(opp);

   // piece moves

   for (ptr = &board->piece[me][0]; (from=*ptr) != SquareNone; ptr++) {

      piece = board->square[from];
      inc_ptr = PIECE_INC(piece);

      if (PIECE_IS_SLIDER(piece)) {

         for (; (inc=*inc_ptr) != IncNone; inc_ptr++) {
            for (to = from+inc; (capture=board->square[to]) == Empty; to += inc) {
               LIST_ADD(list,MOVE_MAKE(from,to));
            }
            if (FLAG_IS(capture,opp_flag)) {
               LIST_ADD(list,MOVE_MAKE(from,to));
            }
         }

      } else {

         for (; (inc=*inc_ptr) != IncNone; inc_ptr++) {
            to = from + inc;
            capture = board->square[to];
            if (capture == Empty || FLAG_IS(capture,opp_flag)) {
               LIST_ADD(list,MOVE_MAKE(from,to));
            }
         }
      }
   }

   // pawn moves

   inc = PAWN_MOVE_INC(me);

   for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) {

      to = from + (inc-1);
      if (FLAG_IS(board->square[to],opp_flag)) {
         add_pawn_move(list,from,to);
      }

      to = from + (inc+1);
      if (FLAG_IS(board->square[to],opp_flag)) {
         add_pawn_move(list,from,to);
      }

      to = from + inc;
      if (board->square[to] == Empty) {
         add_pawn_move(list,from,to);
         if (PAWN_RANK(from,me) == Rank2) {
            to = from + (2*inc);
            if (board->square[to] == Empty) {
               ASSERT(!SQUARE_IS_PROMOTE(to));
               LIST_ADD(list,MOVE_MAKE(from,to));
            }
         }
      }
   }
}

// add_captures()

static void add_captures(list_t * list, const board_t * board) {

   int me, opp;
   int opp_flag;
   const sq_t * ptr;
   int from, to;
   int piece, capture;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   me = board->turn;
   opp = COLOUR_OPP(me);

   opp_flag = COLOUR_FLAG(opp);

   // piece captures

   for (ptr = &board->piece[me][0]; (from=*ptr) != SquareNone; ptr++) {

      piece = board->square[from];

      switch (PIECE_TYPE(piece)) {

      case Knight64:

         to = from - 33;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 31;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 18;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 14;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 14;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 18;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 31;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 33;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      case Bishop64:

         for (to = from-17; (capture=board->square[to]) == Empty; to -= 17)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from-15; (capture=board->square[to]) == Empty; to -= 15)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+15; (capture=board->square[to]) == Empty; to += 15)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+17; (capture=board->square[to]) == Empty; to += 17)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      case Rook64:

         for (to = from-16; (capture=board->square[to]) == Empty; to -= 16)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from-1; (capture=board->square[to]) == Empty; to -= 1)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+1; (capture=board->square[to]) == Empty; to += 1)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+16; (capture=board->square[to]) == Empty; to += 16)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      case Queen64:

         for (to = from-17; (capture=board->square[to]) == Empty; to -= 17)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from-16; (capture=board->square[to]) == Empty; to -= 16)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from-15; (capture=board->square[to]) == Empty; to -= 15)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from-1; (capture=board->square[to]) == Empty; to -= 1)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+1; (capture=board->square[to]) == Empty; to += 1)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+15; (capture=board->square[to]) == Empty; to += 15)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+16; (capture=board->square[to]) == Empty; to += 16)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         for (to = from+17; (capture=board->square[to]) == Empty; to += 17)
            ;
         if (FLAG_IS(capture,opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      case King64:

         to = from - 17;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 16;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 15;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 1;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 1;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 15;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 16;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 17;
         if (FLAG_IS(board->square[to],opp_flag)) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      default:

         ASSERT(false);
         break;
      }
   }

   // pawn captures

   if (COLOUR_IS_WHITE(me)) {

      for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) {

         to = from + 15;
         if (FLAG_IS(board->square[to],opp_flag)) add_pawn_move(list,from,to);

         to = from + 17;
         if (FLAG_IS(board->square[to],opp_flag)) add_pawn_move(list,from,to);

         // promote

         if (SQUARE_RANK(from) == Rank7) {
            to = from + 16;
            if (board->square[to] == Empty) {
               add_promote(list,MOVE_MAKE(from,to));
            }
         }
      }

   } else { // black

      for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) {

         to = from - 17;
         if (FLAG_IS(board->square[to],opp_flag)) add_pawn_move(list,from,to);

         to = from - 15;
         if (FLAG_IS(board->square[to],opp_flag)) add_pawn_move(list,from,to);

         // promote

         if (SQUARE_RANK(from) == Rank2) {
            to = from - 16;
            if (board->square[to] == Empty) {
               add_promote(list,MOVE_MAKE(from,to));
            }
         }
      }
   }
}

// add_quiet_moves()

static void add_quiet_moves(list_t * list, const board_t * board) {

   int me;
   const sq_t * ptr;
   int from, to;
   int piece;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   me = board->turn;

   // piece moves

   for (ptr = &board->piece[me][0]; (from=*ptr) != SquareNone; ptr++) {

      piece = board->square[from];

      switch (PIECE_TYPE(piece)) {

      case Knight64:

         to = from - 33;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 31;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 18;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 14;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 14;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 18;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 31;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 33;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      case Bishop64:

         for (to = from-17; board->square[to] == Empty; to -= 17) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from-15; board->square[to] == Empty; to -= 15) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+15; board->square[to] == Empty; to += 15) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+17; board->square[to] == Empty; to += 17) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         break;

      case Rook64:

         for (to = from-16; board->square[to] == Empty; to -= 16) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from-1; board->square[to] == Empty; to -= 1) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+1; board->square[to] == Empty; to += 1) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+16; board->square[to] == Empty; to += 16) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         break;

      case Queen64:

         for (to = from-17; board->square[to] == Empty; to -= 17) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from-16; board->square[to] == Empty; to -= 16) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from-15; board->square[to] == Empty; to -= 15) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from-1; board->square[to] == Empty; to -= 1) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+1; board->square[to] == Empty; to += 1) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+15; board->square[to] == Empty; to += 15) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+16; board->square[to] == Empty; to += 16) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         for (to = from+17; board->square[to] == Empty; to += 17) {
            LIST_ADD(list,MOVE_MAKE(from,to));
         }

         break;

      case King64:

         to = from - 17;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 16;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 15;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from - 1;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 1;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 15;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 16;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         to = from + 17;
         if (board->square[to] == Empty) LIST_ADD(list,MOVE_MAKE(from,to));

         break;

      default:

         ASSERT(false);
         break;
      }
   }

   // pawn moves

   if (COLOUR_IS_WHITE(me)) {

      for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) {

         // non promotes

         if (SQUARE_RANK(from) != Rank7) {
            to = from + 16;
            if (board->square[to] == Empty) {
               ASSERT(!SQUARE_IS_PROMOTE(to));
               LIST_ADD(list,MOVE_MAKE(from,to));
               if (SQUARE_RANK(from) == Rank2) {
                  to = from + 32;
                  if (board->square[to] == Empty) {
                     ASSERT(!SQUARE_IS_PROMOTE(to));
                     LIST_ADD(list,MOVE_MAKE(from,to));
                  }
               }
            }
         }
      }

   } else { // black

      for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) {

         // non promotes

         if (SQUARE_RANK(from) != Rank2) {
            to = from - 16;
            if (board->square[to] == Empty) {
               ASSERT(!SQUARE_IS_PROMOTE(to));
               LIST_ADD(list,MOVE_MAKE(from,to));
               if (SQUARE_RANK(from) == Rank7) {
                  to = from - 32;
                  if (board->square[to] == Empty) {
                     ASSERT(!SQUARE_IS_PROMOTE(to));
                     LIST_ADD(list,MOVE_MAKE(from,to));
                  }
               }
            }
         }
      }
   }
}

// add_promotes()

static void add_promotes(list_t * list, const board_t * board) {

   int me;
   int inc;
   const sq_t * ptr;
   int from, to;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   me = board->turn;

   inc = PAWN_MOVE_INC(me);

   for (ptr = &board->pawn[me][0]; (from=*ptr) != SquareNone; ptr++) {
      if (PAWN_RANK(from,me) == Rank7) {
         to = from + inc;
         if (board->square[to] == Empty) {
            add_promote(list,MOVE_MAKE(from,to));
         }
      }
   }
}

// add_en_passant_captures()

static void add_en_passant_captures(list_t * list, const board_t * board) {

   int from, to;
   int me;
   int inc;
   int pawn;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   to = board->ep_square;

   if (to != SquareNone) {

      me = board->turn;

      inc = PAWN_MOVE_INC(me);
      pawn = PAWN_MAKE(me);

      from = to - (inc-1);
      if (board->square[from] == pawn) {
         ASSERT(!SQUARE_IS_PROMOTE(to));
         LIST_ADD(list,MOVE_MAKE_FLAGS(from,to,MoveEnPassant));
      }

      from = to - (inc+1);
      if (board->square[from] == pawn) {
         ASSERT(!SQUARE_IS_PROMOTE(to));
         LIST_ADD(list,MOVE_MAKE_FLAGS(from,to,MoveEnPassant));
      }
   }
}

// add_castle_moves()

static void add_castle_moves(list_t * list, const board_t * board) {

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);

   ASSERT(!board_is_check(board));

   if (COLOUR_IS_WHITE(board->turn)) {

      if ((board->flags & FlagsWhiteKingCastle) != 0
       && board->square[F1] == Empty
       && board->square[G1] == Empty
       && !is_attacked(board,F1,Black)) {
         LIST_ADD(list,MOVE_MAKE_FLAGS(E1,G1,MoveCastle));
      }

      if ((board->flags & FlagsWhiteQueenCastle) != 0
       && board->square[D1] == Empty
       && board->square[C1] == Empty
       && board->square[B1] == Empty
       && !is_attacked(board,D1,Black)) {
         LIST_ADD(list,MOVE_MAKE_FLAGS(E1,C1,MoveCastle));
      }

   } else { // black

      if ((board->flags & FlagsBlackKingCastle) != 0
       && board->square[F8] == Empty
       && board->square[G8] == Empty
       && !is_attacked(board,F8,White)) {
         LIST_ADD(list,MOVE_MAKE_FLAGS(E8,G8,MoveCastle));
      }

      if ((board->flags & FlagsBlackQueenCastle) != 0
       && board->square[D8] == Empty
       && board->square[C8] == Empty
       && board->square[B8] == Empty
       && !is_attacked(board,D8,White)) {
         LIST_ADD(list,MOVE_MAKE_FLAGS(E8,C8,MoveCastle));
      }
   }
}

// add_pawn_move()

void add_pawn_move(list_t * list, int from, int to) {

   int move;

   ASSERT(list!=NULL);
   ASSERT(SQUARE_IS_OK(from));
   ASSERT(SQUARE_IS_OK(to));

   move = MOVE_MAKE(from,to);

   if (SQUARE_IS_PROMOTE(to)) {
      LIST_ADD(list,move|MovePromoteQueen);
      LIST_ADD(list,move|MovePromoteKnight);
      LIST_ADD(list,move|MovePromoteRook);
      LIST_ADD(list,move|MovePromoteBishop);
   } else {
      LIST_ADD(list,move);
   }
}

// add_promote()

void add_promote(list_t * list, int move) {

   ASSERT(list!=NULL);
   ASSERT(move_is_ok(move));

   ASSERT((move&~07777)==0); // HACK
   ASSERT(SQUARE_IS_PROMOTE(MOVE_TO(move)));

   LIST_ADD(list,move|MovePromoteQueen);
   LIST_ADD(list,move|MovePromoteKnight);
   LIST_ADD(list,move|MovePromoteRook);
   LIST_ADD(list,move|MovePromoteBishop);
}

// end of move_gen.cpp

