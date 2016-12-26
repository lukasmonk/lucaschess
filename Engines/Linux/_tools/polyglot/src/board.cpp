
// board.cpp

// includes

#include <cstdio>

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "fen.h"
#include "hash.h"
#include "list.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "move_legal.h"
#include "piece.h"
#include "util.h"

// constants

static const bool UseSlowDebug = false;

// functions

// board_is_ok()

bool board_is_ok(const board_t * board) {

   int sq, piece;
   int colour, pos;
   int king, rook;

   if (board == NULL) return false;

   // optional heavy DEBUG mode

   if (!UseSlowDebug) return true;

   // squares

   for (sq = 0; sq < SquareNb; sq++) {
      piece = board->square[sq];
      if (square_is_ok(sq)) {
         pos = board->pos[sq];
         if (piece == Empty) {
            if (pos != -1) return false;
         } else {
            if (pos < 0) return false;
            if (board->list[piece_colour(piece)][pos] != sq) return false;
         }
      } else {
         if (piece != Knight64) return false;
      }
   }

   // white piece list

   colour = White;
   pos = 0;

   if (board->list_size[colour] <= 0 || board->list_size[colour] > 16) return false;

   sq = board->list[colour][pos];
   if (sq == SquareNone) return false;
   if (board->pos[sq] != pos) return false;
   piece = board->square[sq];
   if (!colour_equal(piece,colour) || !piece_is_king(piece)) return false;

   for (pos++; pos < board->list_size[colour]; pos++) {
      sq = board->list[colour][pos];
      if (sq == SquareNone) return false;
      if (board->pos[sq] != pos) return false;
      if (!colour_equal(board->square[sq],colour)) return false;
   }

   sq = board->list[colour][pos];
   if (sq != SquareNone) return false;

   // black piece list

   colour = Black;
   pos = 0;

   if (board->list_size[colour] <= 0 || board->list_size[colour] > 16) return false;

   sq = board->list[colour][pos];
   if (sq == SquareNone) return false;
   if (board->pos[sq] != pos) return false;
   piece = board->square[sq];
   if (!colour_equal(piece,colour) || !piece_is_king(piece)) return false;

   for (pos++; pos < board->list_size[colour]; pos++) {
      sq = board->list[colour][pos];
      if (sq == SquareNone) return false;
      if (board->pos[sq] != pos) return false;
      if (!colour_equal(board->square[sq],colour)) return false;
   }

   sq = board->list[colour][pos];
   if (sq != SquareNone) return false;

   // TODO: material

   if (board->number[WhiteKing12] != 1) return false;
   if (board->number[BlackKing12] != 1) return false;

   if (!colour_is_ok(board->turn)) return false;

   // castling status

   if (board->castle[White][SideH] != SquareNone) {

      king = board->list[White][0];
      if (king < A1 || king > H1) return false;
      if (board->square[king] != WhiteKing256) return false;

      rook = board->castle[White][SideH];
      if (rook < A1 || rook > H1) return false;
      if (board->square[rook] != WhiteRook256) return false;

      if (rook <= king) return false;
   }

   if (board->castle[White][SideA] != SquareNone) {

      king = board->list[White][0];
      if (king < A1 || king > H1) return false;
      if (board->square[king] != WhiteKing256) return false;

      rook = board->castle[White][SideA];
      if (rook < A1 || rook > H1) return false;
      if (board->square[rook] != WhiteRook256) return false;

      if (rook >= king) return false;
   }

   if (board->castle[Black][SideH] != SquareNone) {

      king = board->list[Black][0];
      if (king < A8 || king > H8) return false;
      if (board->square[king] != BlackKing256) return false;

      rook = board->castle[Black][SideH];
      if (rook < A8 || rook > H8) return false;
      if (board->square[rook] != BlackRook256) return false;

      if (rook <= king) return false;
   }

   if (board->castle[Black][SideA] != SquareNone) {

      king = board->list[Black][0];
      if (king < A8 || king > H8) return false;
      if (board->square[king] != BlackKing256) return false;

      rook = board->castle[Black][SideA];
      if (rook < A8 || rook > H8) return false;
      if (board->square[rook] != BlackRook256) return false;

      if (rook >= king) return false;
   }

   return true;
}

// board_clear()

void board_clear(board_t * board) {

   int file, rank, sq;
   int colour, pos;
   int piece;

   ASSERT(board!=NULL);

   // edge squares

   for (sq = 0; sq < SquareNb; sq++) {
      board->square[sq] = Knight64; // HACK: uncoloured knight
      board->pos[sq] = -1;
   }

   // empty squares

   for (rank = 0; rank < 8; rank++) {
      for (file = 0; file < 8; file++) {
         sq = square_make(file,rank);
         board->square[sq] = Empty;
      }
   }

   // piece lists

   for (colour = 0; colour < 3; colour++) {
      for (pos = 0; pos < 32; pos++) { // HACK
         board->list[colour][pos] = SquareNone;
      }
      board->list_size[colour] = 0;
   }

   // material

   for (piece = 0; piece < 12; piece++) {
      board->number[piece] = 0;
   }

   // rest

   board->turn = ColourNone;
   board->castle[White][SideH] = SquareNone;
   board->castle[White][SideA] = SquareNone;
   board->castle[Black][SideH] = SquareNone;
   board->castle[Black][SideA] = SquareNone;
   board->ep_square = SquareNone;

   board->ply_nb = 0;
   board->move_nb = 0;

   board->key = 0;
}

// board_start()

void board_start(board_t * board) {

   ASSERT(board!=NULL);

   if (!board_from_fen(board,StartFen)) ASSERT(false);
}

// board_copy()

void board_copy(board_t * dst, const board_t * src) {

   ASSERT(dst!=NULL);
   ASSERT(board_is_ok(src));

   *dst = *src;
}

// board_equal()

bool board_equal(const board_t * board_1, const board_t * board_2) {

   int sq_64, sq;

   ASSERT(board_is_ok(board_1));
   ASSERT(board_is_ok(board_2));

   // fast comparison

   if (board_1->key != board_2->key) return false;

   // slow comparison

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = square_from_64(sq_64);
      if (board_1->square[sq] != board_2->square[sq]) return false;
   }

   if (board_1->turn != board_2->turn) return false;
   if (board_1->castle[White][SideH] != board_2->castle[White][SideH]) return false;
   if (board_1->castle[White][SideA] != board_2->castle[White][SideA]) return false;
   if (board_1->castle[Black][SideH] != board_2->castle[Black][SideH]) return false;
   if (board_1->castle[Black][SideA] != board_2->castle[Black][SideA]) return false;
   if (board_1->ep_square != board_2->ep_square) return false;

   return true;
}

// board_init_list()

void board_init_list(board_t * board) {

   int sq_64, sq, piece;
   int colour, pos;

   ASSERT(board!=NULL);

   // init

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = square_from_64(sq_64);
      board->pos[sq] = -1;
   }

   for (piece = 0; piece < 12; piece++) board->number[piece] = 0;

   // white piece list

   colour = White;
   pos = 0;

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = square_from_64(sq_64);
      piece = board->square[sq];
      ASSERT(pos>=0&&pos<=16);
      if (colour_equal(piece,colour) && piece_is_king(piece)) {
         board->pos[sq] = (sint8)pos;
         board->list[colour][pos] = (uint8)sq;
         pos++;
         board->number[piece_to_12(piece)]++;
      }
   }
   ASSERT(pos==1);

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = square_from_64(sq_64);
      piece = board->square[sq];
      ASSERT(pos>=0&&pos<=16);
      if (colour_equal(piece,colour) && !piece_is_king(piece)) {
         board->pos[sq] = (sint8)pos;
         board->list[colour][pos] = (uint8)sq;
         pos++;
         board->number[piece_to_12(piece)]++;
      }
   }

   ASSERT(pos>=1&&pos<=16);
   board->list[colour][pos] = SquareNone;
   board->list_size[colour] = (sint8)pos;

   // black piece list

   colour = Black;
   pos = 0;

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = square_from_64(sq_64);
      piece = board->square[sq];
      ASSERT(pos>=0&&pos<=16);
      if (colour_equal(piece,colour) && piece_is_king(piece)) {
         board->pos[sq] = (sint8)pos;
         board->list[colour][pos] = (uint8)sq;
         pos++;
         board->number[piece_to_12(piece)]++;
      }
   }
   ASSERT(pos==1);

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = square_from_64(sq_64);
      piece = board->square[sq];
      ASSERT(pos>=1&&pos<=16);
      if (colour_equal(piece,colour) && !piece_is_king(piece)) {
         board->pos[sq] = (sint8)pos;
         board->list[colour][pos] = (uint8)sq;
         pos++;
         board->number[piece_to_12(piece)]++;
      }
   }

   ASSERT(pos>=1&&pos<=16);
   board->list[colour][pos] = SquareNone;
   board->list_size[colour] = (sint8)pos;

   // hash key

   board->key = hash_key(board);
}

// board_flags()

int board_flags(const board_t * board) {

   int flags;

   flags = 0;

   if (board->castle[White][SideH] != SquareNone) flags |= 1 << 0;
   if (board->castle[White][SideA] != SquareNone) flags |= 1 << 1;
   if (board->castle[Black][SideH] != SquareNone) flags |= 1 << 2;
   if (board->castle[Black][SideA] != SquareNone) flags |= 1 << 3;

   return flags;
}

// board_can_play()

bool board_can_play(const board_t * board) {

   list_t list[1];
   int i, move;

   ASSERT(board_is_ok(board));

   gen_moves(list,board);

   for (i = 0; i < list_size(list); i++) {
      move = list_move(list,i);
      if (pseudo_is_legal(move,board)) return true;
   }

   return false; // no legal move
}

// board_mobility()

int board_mobility(const board_t * board) {

   list_t list[1];

   ASSERT(board_is_ok(board));

   gen_legal_moves(list,board);

   return list_size(list);
}

// board_is_check()

bool board_is_check(const board_t * board) {

   ASSERT(board_is_ok(board));

   return is_in_check(board,board->turn);
}

// board_is_mate()

bool board_is_mate(const board_t * board) {

   ASSERT(board_is_ok(board));

   if (!board_is_check(board)) return false;
   if (board_can_play(board)) return false;

   return true;
}

// board_is_stalemate()

bool board_is_stalemate(const board_t * board) {

   ASSERT(board_is_ok(board));

   if (board_is_check(board)) return false;
   if (board_can_play(board)) return false;

   return true;
}

// king_pos()

int king_pos(const board_t * board, int colour) {

   ASSERT(board_is_ok(board));
   ASSERT(colour_is_ok(colour));

   return board->list[colour][0];
}

// board_disp()

void board_disp(const board_t * board) {

   int file, rank, sq;
   int piece, c;
   char fen[256];

   ASSERT(board!=NULL);

   if (!board_to_fen(board,fen,256)) ASSERT(false);
   my_log("POLYGLOT %s\n",fen);
   my_log("POLYGLOT\n");

   for (rank = 7; rank >= 0; rank--) {

      my_log("POLYGLOT ");

      for (file = 0; file < 8; file++) {

         sq = square_make(file,rank);
         piece = board->square[sq];

         c = (piece != Empty) ? piece_to_char(piece) : '-';
         my_log("%c ",c);
      }

      my_log("\n");
   }

   my_log("POLYGLOT\n");

   my_log("POLYGLOT %s to play\n",(colour_is_black(board->turn))?"black":"white");
   my_log("POLYGLOT\n");
}

// end of board.cpp

