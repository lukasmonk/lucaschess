
// move_gen.cpp

// includes

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "move_legal.h"
#include "piece.h"
#include "util.h"

// prototypes

static void add_all_moves    (list_t * list, const board_t * board);
static void add_castle_moves (list_t * list, const board_t * board);

static void add_pawn_move    (list_t * list, int from, int to);

// functions

// gen_legal_moves()

void gen_legal_moves(list_t * list, const board_t * board) {

   ASSERT(list!=NULL);
   ASSERT(board_is_ok(board));

   gen_moves(list,board);
   filter_legal(list,board);
}

// gen_moves()

void gen_moves(list_t * list, const board_t * board) {

   ASSERT(list!=NULL);
   ASSERT(board_is_ok(board));

   list_clear(list);

   add_all_moves(list,board);
   if (!is_in_check(board,board->turn)) add_castle_moves(list,board);
}

// add_all_moves()

static void add_all_moves(list_t * list, const board_t * board) {

   int me, opp;
   const uint8 * ptr;
   const sint8 * ptr_inc;
   int from, to;
   int inc;
   int piece, capture;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));

   me = board->turn;
   opp = colour_opp(me);

   for (ptr = board->list[me]; (from=*ptr) != SquareNone; ptr++) {

      piece = board->square[from];
      ASSERT(colour_equal(piece,me));

      switch (piece_type(piece)) {

      case WhitePawn64:

         to = from + 15;
         if (to == board->ep_square || colour_equal(board->square[to],opp)) {
            add_pawn_move(list,from,to);
         }

         to = from + 17;
         if (to == board->ep_square || colour_equal(board->square[to],opp)) {
            add_pawn_move(list,from,to);
         }

         to = from + 16;
         if (board->square[to] == Empty) {
            add_pawn_move(list,from,to);
            if (square_rank(from) == Rank2) {
               to = from + 32;
               if (board->square[to] == Empty) {
                  ASSERT(!square_is_promote(to));
                  list_add(list,move_make(from,to));
               }
            }
         }

         break;

      case BlackPawn64:

         to = from - 17;
         if (to == board->ep_square || colour_equal(board->square[to],opp)) {
            add_pawn_move(list,from,to);
         }

         to = from - 15;
         if (to == board->ep_square || colour_equal(board->square[to],opp)) {
            add_pawn_move(list,from,to);
         }

         to = from - 16;
         if (board->square[to] == Empty) {
            add_pawn_move(list,from,to);
            if (square_rank(from) == Rank7) {
               to = from - 32;
               if (board->square[to] == Empty) {
                  ASSERT(!square_is_promote(to));
                  list_add(list,move_make(from,to));
               }
            }
         }

         break;

      case Knight64:

         for (ptr_inc = KnightInc; (inc=*ptr_inc) != IncNone; ptr_inc++) {
            to = from + inc;
            capture = board->square[to];
            if (capture == Empty || colour_equal(capture,opp)) {
               list_add(list,move_make(from,to));
            }
         }

         break;

      case Bishop64:

         for (ptr_inc = BishopInc; (inc=*ptr_inc) != IncNone; ptr_inc++) {
            for (to = from+inc; (capture=board->square[to]) == Empty; to += inc) {
               list_add(list,move_make(from,to));
            }
            if (colour_equal(capture,opp)) {
               list_add(list,move_make(from,to));
            }
         }

         break;

      case Rook64:

         for (ptr_inc = RookInc; (inc=*ptr_inc) != IncNone; ptr_inc++) {
            for (to = from+inc; (capture=board->square[to]) == Empty; to += inc) {
               list_add(list,move_make(from,to));
            }
            if (colour_equal(capture,opp)) {
               list_add(list,move_make(from,to));
            }
         }

         break;

      case Queen64:

         for (ptr_inc = QueenInc; (inc=*ptr_inc) != IncNone; ptr_inc++) {
            for (to = from+inc; (capture=board->square[to]) == Empty; to += inc) {
               list_add(list,move_make(from,to));
            }
            if (colour_equal(capture,opp)) {
               list_add(list,move_make(from,to));
            }
         }

         break;

      case King64:

         for (ptr_inc = KingInc; (inc=*ptr_inc) != IncNone; ptr_inc++) {
            to = from + inc;
            capture = board->square[to];
            if (capture == Empty || colour_equal(capture,opp)) {
               list_add(list,move_make(from,to));
            }
         }

         break;

      default:

         ASSERT(false);
         break;
      }
   }
}

// add_castle_moves()

static void add_castle_moves(list_t * list, const board_t * board) {

   int me, opp;
   int rank;
   int king_from, king_to;
   int rook_from, rook_to;
   bool legal;
   int inc;
   int sq;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));

   ASSERT(!is_in_check(board,board->turn));

   me = board->turn;
   opp = colour_opp(me);

   rank = colour_is_white(me) ? Rank1 : Rank8;

   // h-side castling

   if (board->castle[me][SideH] != SquareNone) {

      king_from = king_pos(board,me);
      king_to = square_make(FileG,rank);
      rook_from = board->castle[me][SideH];
      rook_to = square_make(FileF,rank);

      ASSERT(square_rank(king_from)==rank);
      ASSERT(square_rank(rook_from)==rank);
      ASSERT(board->square[king_from]==(King64|me)); // HACK
      ASSERT(board->square[rook_from]==(Rook64|me)); // HACK
      ASSERT(rook_from>king_from);

      legal = true;

      if (king_to != king_from) {

         inc = (king_to > king_from) ? +1 : -1;

         for (sq = king_from+inc; true; sq += inc) {

            if (sq != rook_from && board->square[sq] != Empty) legal = false;
            if (is_attacked(board,sq,opp)) legal = false;

            if (sq == king_to) break;
         }
      }

      if (rook_to != rook_from) {

         inc = (rook_to > rook_from) ? +1 : -1;

         for (sq = rook_from+inc; true; sq += inc) {
            if (sq != king_from && board->square[sq] != Empty) legal = false;
            if (sq == rook_to) break;
         }
      }

      if (legal) list_add(list,move_make(king_from,rook_from));
   }

   // a-side castling

   if (board->castle[me][SideA] != SquareNone) {

      king_from = king_pos(board,me);
      king_to = square_make(FileC,rank);
      rook_from = board->castle[me][SideA];
      rook_to = square_make(FileD,rank);

      ASSERT(square_rank(king_from)==rank);
      ASSERT(square_rank(rook_from)==rank);
      ASSERT(board->square[king_from]==(King64|me)); // HACK
      ASSERT(board->square[rook_from]==(Rook64|me)); // HACK
      ASSERT(rook_from<king_from);

      legal = true;

      if (king_to != king_from) {

         inc = (king_to > king_from) ? +1 : -1;

         for (sq = king_from+inc; true; sq += inc) {

            if (sq != rook_from && board->square[sq] != Empty) legal = false;
            if (is_attacked(board,sq,opp)) legal = false;

            if (sq == king_to) break;
         }
      }

      if (rook_to != rook_from) {

         inc = (rook_to > rook_from) ? +1 : -1;

         for (sq = rook_from+inc; true; sq += inc) {
            if (sq != king_from && board->square[sq] != Empty) legal = false;
            if (sq == rook_to) break;
         }
      }

      if (legal) list_add(list,move_make(king_from,rook_from));
   }
}

// add_pawn_move()

static void add_pawn_move(list_t * list, int from, int to) {

   int move;

   ASSERT(list_is_ok(list));
   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));

   move = move_make(from,to);

   if (square_is_promote(to)) {
      list_add(list,move|MovePromoteKnight);
      list_add(list,move|MovePromoteBishop);
      list_add(list,move|MovePromoteRook);
      list_add(list,move|MovePromoteQueen);
   } else {
      list_add(list,move);
   }
}

// end of move_gen.cpp

