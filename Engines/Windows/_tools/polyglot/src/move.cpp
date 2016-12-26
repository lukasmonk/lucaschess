
// move.cpp

// includes

#include <cstdlib>
#include <cstring>

#include "attack.h"
#include "colour.h"
#include "list.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "move_legal.h"
#include "option.h"
#include "piece.h"
#include "square.h"
#include "util.h"

// "constants"

static const uint8 PromotePiece[5] = { PieceNone64, Knight64, Bishop64, Rook64, Queen64 };

// functions

// move_is_ok()

bool move_is_ok(int move) {

   if (move < 0 || move >= 65536) return false;

   if (move == MoveNone) return false;

   return true;
}

// move_make()

int move_make(int from, int to) {

   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));

   return (square_to_64(from) << 6) | square_to_64(to);
}

// move_make_flags()

int move_make_flags(int from, int to, int flags) {

   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));
   ASSERT((flags&~0xF000)==0);

   ASSERT(to!=from);

   return (square_to_64(from) << 6) | square_to_64(to) | flags;
}

// move_from()

int move_from(int move) {

   int from_64;

   ASSERT(move_is_ok(move));

   from_64 = (move >> 6) & 077;

   return square_from_64(from_64);
}

// move_to()

int move_to(int move) {

   int to_64;

   ASSERT(move_is_ok(move));

   to_64 = move & 077;

   return square_from_64(to_64);
}

// move_promote_hack()

int move_promote_hack(int move) {

   int code;

   ASSERT(move_is_ok(move));

   ASSERT(move_is_promote(move));

   code = move >> 12;
   ASSERT(code>=1&&code<=4);

   return PromotePiece[code];
}

// move_is_capture()

bool move_is_capture(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   if (move_is_en_passant(move,board)) return true;
   if (board->square[move_to(move)] != Empty) return true;

   return false;
}

// move_is_promote()

bool move_is_promote(int move) {

   ASSERT(move_is_ok(move));

   return (move & MoveFlags) != 0;
}

// move_is_en_passant()

bool move_is_en_passant(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   return piece_is_pawn(move_piece(move,board))
       && move_to(move) == board->ep_square;
}

// move_is_castle()

bool move_is_castle(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   return colour_equal(board->square[move_to(move)],board->turn);
}

// move_piece()

int move_piece(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   return board->square[move_from(move)];
}

// move_capture()

int move_capture(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   if (move_is_en_passant(move,board)) {
      return piece_pawn_opp(move_piece(move,board));
   }

   return board->square[move_to(move)];
}

// move_promote()

int move_promote(int move, const board_t * board) {

   int code;

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   if (move_is_promote(move)) {
      code = move >> 12;
      ASSERT(code>=1&&code<=4);
      return PromotePiece[code] | board->turn;
   }

   return Empty;
}

// move_is_check()

bool move_is_check(int move, const board_t * board) {

   board_t new_board[1];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   board_copy(new_board,board);
   move_do(new_board,move);
   ASSERT(!is_in_check(new_board,colour_opp(new_board->turn)));

   return board_is_check(new_board);
}

// move_is_mate()

bool move_is_mate(int move, const board_t * board) {

   board_t new_board[1];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   board_copy(new_board,board);
   move_do(new_board,move);
   ASSERT(!is_in_check(new_board,colour_opp(new_board->turn)));

   return board_is_mate(new_board);
}

// move_to_can()

bool move_to_can(int move, const board_t * board, char string[], int size) {

   int from, to;

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));
   ASSERT(string!=NULL);
   ASSERT(size>=6);

   ASSERT(move_is_legal(move,board));

   if (size < 6) return false;

   // init

   from = move_from(move);
   to = move_to(move);

   // king-slide castling

   if (move_is_castle(move,board) && !option_get_bool("Chess960")) {
      if (false) {
      } else if (from == E1 && to == H1) {
         to = G1;
      } else if (from == E1 && to == A1) {
         to = C1;
      } else if (from == E8 && to == H8) {
         to = G8;
      } else if (from == E8 && to == A8) {
         to = C8;
      }
   }

   // normal moves

   if (!square_to_string(from,&string[0],3)) ASSERT(false);
   if (!square_to_string(to,&string[2],3)) ASSERT(false);
   ASSERT(strlen(string)==4);

   // promotes

   if (move_is_promote(move)) {
      string[4] = piece_to_char(move_promote_hack(move)|Black); // HACK: black => lower-case
      string[5] = '\0';
   }

   // debug

   ASSERT(move_from_can(string,board)==move);

   return true;
}

// move_from_can()

int move_from_can(const char string[], const board_t * board) {

   char tmp_string[256];
   int from, to;
   int side;
   int move;

   ASSERT(string!=NULL);
   ASSERT(board_is_ok(board));

   // from

   tmp_string[0] = string[0];
   tmp_string[1] = string[1];
   tmp_string[2] = '\0';

   from = square_from_string(tmp_string);
   if (from == SquareNone) return MoveNone;

   // to

   tmp_string[0] = string[2];
   tmp_string[1] = string[3];
   tmp_string[2] = '\0';

   to = square_from_string(tmp_string);
   if (to == SquareNone) return MoveNone;

   // convert "king slide" castling to KxR

   if (piece_is_king(board->square[from])
    && square_rank(to) == square_rank(from)
    && abs(to-from) > 1) {
      side = (to > from) ? SideH : SideA;
      to = board->castle[board->turn][side];
      if (to == SquareNone) return MoveNone;
   }

   // move

   move = move_make(from,to);

   // promote

   switch (string[4]) {
   case '\0': // not a promotion
      if (piece_is_pawn(board->square[from])
       && square_side_rank(to,board->turn) == Rank8
       && option_get_bool("PromoteWorkAround")) {
         move |= MovePromoteQueen;
      }
      break;
   case 'N':
   case 'n':
      move |= MovePromoteKnight;
      break;
   case 'B':
   case 'b':
      move |= MovePromoteBishop;
      break;
   case 'R':
   case 'r':
      move |= MovePromoteRook;
      break;
   case 'Q':
   case 'q':
      move |= MovePromoteQueen;
      break;
   default:
      return MoveNone;
      break;
   }

   // debug

   ASSERT(move_is_legal(move,board));

   return move;
}

// move_order()

int move_order(int move) {

   ASSERT(move_is_ok(move));

   return ((move & 07777) << 3) | (move >> 12); // from, to, promote
}

// move_disp()

void move_disp(int move, const board_t * board) {

   char string[256];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));

   if (!move_to_can(move,board,string,sizeof(string))) ASSERT(false);
   my_log("POLYGLOT %s\n",string);
}

// end of move.cpp

