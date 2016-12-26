
// san.cpp

// includes

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "attack.h"
#include "board.h"
#include "list.h"
#include "move.h"
#include "move_gen.h"
#include "move_legal.h"
#include "piece.h"
#include "san.h"
#include "square.h"
#include "util.h"

// constants

static const bool UseSlowDebug = false;

enum ambiguity_t {
   AMBIGUITY_NONE,
   AMBIGUITY_FILE,
   AMBIGUITY_RANK,
   AMBIGUITY_SQUARE
};

// functions

static bool san_to_lan    (const char san[], const board_t * board, char string[], int size);
static int  move_from_lan (const char string[], const board_t * board);

static int  ambiguity     (int move, const board_t * board);

// move_to_san()

bool move_to_san(int move, const board_t * board, char string[], int size) {

   int from, to, piece;
   char tmp_string[256];

   ASSERT(move_is_ok(move));
   ASSERT(board_is_ok(board));
   ASSERT(string!=NULL);
   ASSERT(size>=8);

   ASSERT(move_is_legal(move,board));

   if (size < 8) return false;

   // init

   from = move_from(move);
   to = move_to(move);

   string[0] = '\0';

   // castle

   if (move_is_castle(move,board)) {

      if (to > from) {
         strcat(string,"O-O");
      } else {
         strcat(string,"O-O-O");
      }

      goto check;
   }

   // from

   piece = board->square[from];

   if (piece_is_pawn(piece)) {

      // pawn

      if (move_is_capture(move,board)) {
         sprintf(tmp_string,"%c",file_to_char(square_file(from)));
         strcat(string,tmp_string);
      }

   } else {

      // piece

      sprintf(tmp_string,"%c",toupper(piece_to_char(piece)));
      strcat(string,tmp_string);

      // ambiguity

      switch (ambiguity(move,board)) {
      case AMBIGUITY_NONE:
         break;
      case AMBIGUITY_FILE:
         sprintf(tmp_string,"%c",file_to_char(square_file(from)));
         strcat(string,tmp_string);
         break;
      case AMBIGUITY_RANK:
         sprintf(tmp_string,"%c",rank_to_char(square_rank(from)));
         strcat(string,tmp_string);
         break;
      case AMBIGUITY_SQUARE:
         if (!square_to_string(from,tmp_string,256)) return false;
         strcat(string,tmp_string);
         break;
      default:
         ASSERT(false);
         break;
      }
   }

   // capture

   if (move_is_capture(move,board)) strcat(string,"x");

   // to

   if (!square_to_string(to,tmp_string,sizeof(tmp_string))) return false;
   strcat(string,tmp_string);

   // promote

   if (move_is_promote(move)) {
      sprintf(tmp_string,"=%c",toupper(piece_to_char(move_promote(move,board))));
      strcat(string,tmp_string);
   }

   // check

check:

   if (move_is_mate(move,board)) {
      strcat(string,"#");
   } else if (move_is_check(move,board)) {
      strcat(string,"+");
   }

   return true;
}

// move_from_san()

int move_from_san(const char string[], const board_t * board) {

   char s[256];
   int move;

   ASSERT(string!=NULL);
   ASSERT(board_is_ok(board));

   san_to_lan(string,board,s,sizeof(s));
   move = move_from_lan(s,board);

   ASSERT(!UseSlowDebug||move==move_from_san_debug(string,board));

   return move;
}

// move_from_san_debug()

int move_from_san_debug(const char string[], const board_t * board) {

   list_t list[1];
   int i, move;
   char move_string[256];

   ASSERT(string!=NULL);
   ASSERT(board_is_ok(board));

   gen_legal_moves(list,board);

   for (i = 0; i < list_size(list); i++) {
      move = list_move(list,i);
#pragma warning(disable:4390 4127)
      if (!move_to_san(move,board,move_string,sizeof(move_string))) ASSERT(false);
      if (my_string_equal(move_string,string)) return move;
   }

   return MoveNone;
}

// san_to_lan()

static bool san_to_lan(const char san[], const board_t * board, char string[], int size) {

   int len;
   int left, right;
   int c;
   int king, rook;
   char king_string[3], rook_string[3];

   ASSERT(san!=NULL);
   ASSERT(board_is_ok(board));
   ASSERT(string!=NULL);
   ASSERT(size>=8);

   // init

   if (size < 8) return false;
   strcpy(string,"???????");

   len = (int)strlen(san);

   left = 0;
   right = len;

   // skip trailing '+' or '#'

   if (left < right) {
      c = san[right-1];
      if (c == '+' || c == '#') right--;
   }

   // castling

   ASSERT(left==0);

   if (false) {

   } else if (right == 3 && strncmp(san,"O-O",3) == 0) {

      if (board->castle[board->turn][SideH] == SquareNone) return false;

      king = king_pos(board,board->turn);
      rook = board->castle[board->turn][SideH];

      square_to_string(king,king_string,3);
      square_to_string(rook,rook_string,3);

      sprintf(string,"K%s?%s?",king_string,rook_string);

   } else if (right == 5 && strncmp(san,"O-O-O",5) == 0) {

      if (board->castle[board->turn][SideA] == SquareNone) return false;

      king = king_pos(board,board->turn);
      rook = board->castle[board->turn][SideA];

      square_to_string(king,king_string,3);
      square_to_string(rook,rook_string,3);

      sprintf(string,"K%s?%s?",king_string,rook_string);

   } else {

      // moved piece

      if (left < right) {

         c = san[left];

         if (char_is_piece(c)) {
            string[0] = c;
            left++;
         }
      }

      // promotion

      if (left < right) {

         c = toupper(san[right-1]);

         if (char_is_piece(c)) {

            string[6] = c;
            right--;

            // skip '='

            if (left < right && san[right-1] == '=') right--;
         }
      }

      // to-square rank

      if (left < right) {

         c = san[right-1];

         if (char_is_rank(c)) {
            string[5] = c;
            right--;
         }
      }

      // to-square file

      if (left < right) {

         c = san[right-1];

         if (char_is_file(c)) {
            string[4] = c;
            right--;
         }
      }

      // captured piece

      if (left < right) {

         c = san[right-1];

         if (char_is_piece(c)) {
            string[3] = c;
            right--;
         }
      }

      // skip middle '-' or 'x'

      if (left < right) {
         c = san[right-1];
         if (c == '-' || c == 'x') right--;
      }

      // from-square file

      if (left < right) {

         c = san[left];

         if (char_is_file(c)) {
            string[1] = c;
            left++;
         }
      }

      // from-square rank

      if (left < right) {

         c = san[left];

         if (char_is_rank(c)) {
            string[2] = c;
            left++;
         }
      }

      if (left != right) return false;
   }

   // end

   return true;
}

// move_from_lan()

static int move_from_lan(const char string[], const board_t * board) {

   int len;
   int move;
   int promote;
   char s[256];
   int from, to;
   int colour;
   int inc;
   int piece_char;
   int n;
   const uint8 * ptr;
   int piece;
   int side;

   ASSERT(string!=NULL);
   ASSERT(board_is_ok(board));

   // init

   len = (int)strlen(string);
   if (len != 7) return MoveNone;

   move = MoveNone;
   colour = board->turn;

   // promote

   promote = 0;

   switch (string[6]) {
   case '?': // not a promotion
      break;
   case 'N':
      promote = MovePromoteKnight;
      break;
   case 'B':
      promote = MovePromoteBishop;
      break;
   case 'R':
      promote = MovePromoteRook;
      break;
   case 'Q':
      promote = MovePromoteQueen;
      break;
   default:
      return MoveNone;
      break;
   }

   // to square

   s[0] = string[4];
   s[1] = string[5];
   s[2] = '\0';

   to = square_from_string(s);
   if (to == SquareNone) return MoveNone;

   // known from square?

   if (string[1] != '?' && string[2] != '?') {

      // from square

      s[0] = string[1];
      s[1] = string[2];
      s[2] = '\0';

      from = square_from_string(s);
      if (from == SquareNone) return MoveNone;

      // convert "king slide" castling to KxR

      if (piece_is_king(board->square[from])
       && square_rank(to) == square_rank(from)
       && abs(to-from) > 1) {
         side = (to > from) ? SideH : SideA;
         to = board->castle[colour][side];
         if (to == SquareNone) return MoveNone;
      }

      // move

      move = move_make(from,to) | promote;

      return move;
   }

   // pawn non-capture?

   if (string[0] == '?' && string[1] == '?') {

      if (board->square[to] != Empty) return MoveNone; // useful?

      inc = (colour_is_white(colour)) ? +16 : -16;

      from = to - inc;
      if (board->square[from] == Empty && square_side_rank(to,colour) == Rank4) {
         from -= inc;
      }

      if (board->square[from] != piece_make_pawn(colour)) { // useful?
         return MoveNone;
      }

      // move

      move = move_make(from,to) | promote;

      return move;
   }

   // pawn capture?

   piece_char = string[0];

   if (piece_char == '?' && string[1] != '?') {
      piece_char = 'P';
   }

   // attack loop

   n = 0;

   for (ptr = board->list[colour]; (from=*ptr) != SquareNone; ptr++) {

      piece = board->square[from];

      if (toupper(piece_to_char(piece)) == piece_char) {
         if (piece_attack(board,piece,from,to)) {
            if (true
             && (string[1] == '?' || file_to_char(square_file(from)) == string[1])
             && (string[2] == '?' || rank_to_char(square_rank(from)) == string[2])) {
               if (!is_pinned(board,from,to,colour)) {
                  move = move_make(from,to) | promote;
                  n++;
               }
            }
         }
      }
   }

   if (n != 1) move = MoveNone;

   return move;
}

// ambiguity()

static int ambiguity(int move, const board_t * board) {

   int from, to, piece;
   list_t list[1];
   int i, n, m;

   // init

   from = move_from(move);
   to = move_to(move);
   piece = move_piece(move,board);

   gen_legal_moves(list,board);

   // no ambiguity?

   n = 0;

   for (i = 0; i < list_size(list); i++) {
      m = list_move(list,i);
      if (move_piece(m,board) == piece && move_to(m) == to) {
         n++;
      }
   }

   if (n == 1) return AMBIGUITY_NONE;

   // file ambiguity?

   n = 0;

   for (i = 0; i < list_size(list); i++) {
      m = list_move(list,i);
      if (move_piece(m,board) == piece && move_to(m) == to) {
         if (square_file(move_from(m)) == square_file(from)) n++;
      }
   }

   if (n == 1) return AMBIGUITY_FILE;

   // rank ambiguity?

   n = 0;

   for (i = 0; i < list_size(list); i++) {
      m = list_move(list,i);
      if (move_piece(m,board) == piece && move_to(m) == to) {
         if (square_rank(move_from(m)) == square_rank(from)) n++;
      }
   }

   if (n == 1) return AMBIGUITY_RANK;

   // square ambiguity

   return AMBIGUITY_SQUARE;
}

// end of san.cpp

