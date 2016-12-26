
// board.cpp

// includes

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "fen.h"
#include "hash.h"
#include "list.h"
#include "move.h"
#include "move_do.h"
#include "move_evasion.h"
#include "move_gen.h"
#include "move_legal.h"
#include "pawn.h" // TODO: bit.h
#include "piece.h"
#include "pst.h"
#include "util.h"
#include "value.h"

// constants

static const bool UseSlowDebug = false;

// functions

// board_is_ok()

bool board_is_ok(const board_t * board) {

   int sq, piece, colour;
   int size, pos;

   if (board == NULL) return false;

   // optional heavy DEBUG mode

   if (!UseSlowDebug) return true;

   // squares

   for (sq = 0; sq < SquareNb; sq++) {

      piece = board->square[sq];
      pos = board->pos[sq];

      if (SQUARE_IS_OK(sq)) {

         // inside square

         if (piece == Empty) {

            if (pos != -1) return false;

         } else {

            if (!piece_is_ok(piece)) return false;

            if (!PIECE_IS_PAWN(piece)) {

               colour = PIECE_COLOUR(piece);
               if (pos < 0 || pos >= board->piece_size[colour]) return false;
               if (board->piece[colour][pos] != sq) return false;

            } else { // pawn

               if (SQUARE_IS_PROMOTE(sq)) return false;

               colour = PIECE_COLOUR(piece);
               if (pos < 0 || pos >= board->pawn_size[colour]) return false;
               if (board->pawn[colour][pos] != sq) return false;
            }
         }

      } else {

         // edge square

         if (piece != Edge) return false;
         if (pos != -1) return false;
      }
   }

   // piece lists

   for (colour = 0; colour < ColourNb; colour++) {

      // piece list

      size = board->piece_size[colour];
      if (size < 1 || size > 16) return false;

      for (pos = 0; pos < size; pos++) {

         sq = board->piece[colour][pos];
         if (!SQUARE_IS_OK(sq)) return false;

         if (board->pos[sq] != pos) return false;

         piece = board->square[sq];
         if (!COLOUR_IS(piece,colour)) return false;
         if (pos == 0 && !PIECE_IS_KING(piece)) return false;
         if (pos != 0 && PIECE_IS_KING(piece)) return false;

         if (pos != 0 && PIECE_ORDER(piece) > PIECE_ORDER(board->square[board->piece[colour][pos-1]])) {
            return false;
         }
      }

      sq = board->piece[colour][size];
      if (sq != SquareNone) return false;

      // pawn list

      size = board->pawn_size[colour];
      if (size < 0 || size > 8) return false;

      for (pos = 0; pos < size; pos++) {

         sq = board->pawn[colour][pos];
         if (!SQUARE_IS_OK(sq)) return false;
         if (SQUARE_IS_PROMOTE(sq)) return false;

         if (board->pos[sq] != pos) return false;

         piece = board->square[sq];
         if (!COLOUR_IS(piece,colour)) return false;
         if (!PIECE_IS_PAWN(piece)) return false;
      }

      sq = board->pawn[colour][size];
      if (sq != SquareNone) return false;

      // piece total

      if (board->piece_size[colour] + board->pawn_size[colour] > 16) return false;
   }

   // material

   if (board->piece_nb != board->piece_size[White] + board->pawn_size[White]
                        + board->piece_size[Black] + board->pawn_size[Black]) {
      return false;
   }

   if (board->number[WhitePawn12] != board->pawn_size[White]) return false;
   if (board->number[BlackPawn12] != board->pawn_size[Black]) return false;
   if (board->number[WhiteKing12] != 1) return false;
   if (board->number[BlackKing12] != 1) return false;

   // misc

   if (!COLOUR_IS_OK(board->turn)) return false;

   if (board->ply_nb < 0) return false;
   if (board->sp < board->ply_nb) return false;

   if (board->cap_sq != SquareNone && !SQUARE_IS_OK(board->cap_sq)) return false;

   if (board->opening != board_opening(board)) return false;
   if (board->endgame != board_endgame(board)) return false;
   if (board->key != hash_key(board)) return false;
   if (board->pawn_key != hash_pawn_key(board)) return false;
   if (board->material_key != hash_material_key(board)) return false;

   return true;
}

// board_clear()

void board_clear(board_t * board) {

   int sq, sq_64;

   ASSERT(board!=NULL);

   // edge squares

   for (sq = 0; sq < SquareNb; sq++) {
      board->square[sq] = Edge;
   }

   // empty squares

   for (sq_64 = 0; sq_64 < 64; sq_64++) {
      sq = SQUARE_FROM_64(sq_64);
      board->square[sq] = Empty;
   }

   // misc

   board->turn = ColourNone;
   board->flags = FlagsNone;
   board->ep_square = SquareNone;
   board->ply_nb = 0;
}

// board_copy()

void board_copy(board_t * dst, const board_t * src) {

   ASSERT(dst!=NULL);
   ASSERT(board_is_ok(src));

   *dst = *src;
}

// board_init_list()

void board_init_list(board_t * board) {

   int sq_64, sq, piece;
   int colour, pos;
   int i, size;
   int square;
   int order;
   int file;

   ASSERT(board!=NULL);

   // init

   for (sq = 0; sq < SquareNb; sq++) {
      board->pos[sq] = -1;
   }

   board->piece_nb = 0;
   for (piece = 0; piece < 12; piece++) board->number[piece] = 0;

   // piece lists

   for (colour = 0; colour < ColourNb; colour++) {

      // piece list

      pos = 0;

      for (sq_64 = 0; sq_64 < 64; sq_64++) {

         sq = SQUARE_FROM_64(sq_64);
         piece = board->square[sq];
         if (piece != Empty && !piece_is_ok(piece)) my_fatal("board_init_list(): illegal position\n");

         if (COLOUR_IS(piece,colour) && !PIECE_IS_PAWN(piece)) {

            if (pos >= 16) my_fatal("board_init_list(): illegal position\n");
            ASSERT(pos>=0&&pos<16);

            board->pos[sq] = pos;
            board->piece[colour][pos] = sq;
            pos++;

            board->piece_nb++;
            board->number[PIECE_TO_12(piece)]++;
         }
      }

      if (board->number[COLOUR_IS_WHITE(colour)?WhiteKing12:BlackKing12] != 1) my_fatal("board_init_list(): illegal position\n");

      ASSERT(pos>=1&&pos<=16);
      board->piece[colour][pos] = SquareNone;
      board->piece_size[colour] = pos;

      // MV sort

      size = board->piece_size[colour];

      for (i = 1; i < size; i++) {

         square = board->piece[colour][i];
         piece = board->square[square];
         order = PIECE_ORDER(piece);

         for (pos = i; pos > 0 && order > PIECE_ORDER(board->square[(sq=board->piece[colour][pos-1])]); pos--) {
            ASSERT(pos>0&&pos<size);
            board->piece[colour][pos] = sq;
            ASSERT(board->pos[sq]==pos-1);
            board->pos[sq] = pos;
         }

         ASSERT(pos>=0&&pos<size);
         board->piece[colour][pos] = square;
         ASSERT(board->pos[square]==i);
         board->pos[square] = pos;
      }

      // debug

      if (DEBUG) {

         for (i = 0; i < board->piece_size[colour]; i++) {

            sq = board->piece[colour][i];
            ASSERT(board->pos[sq]==i);

            if (i == 0) { // king
               ASSERT(PIECE_IS_KING(board->square[sq]));
            } else {
               ASSERT(!PIECE_IS_KING(board->square[sq]));
               ASSERT(PIECE_ORDER(board->square[board->piece[colour][i]])<=PIECE_ORDER(board->square[board->piece[colour][i-1]]));
            }
         }
      }

      // pawn list

      for (file = 0; file < FileNb; file++) {
         board->pawn_file[colour][file] = 0;
      }

      pos = 0;

      for (sq_64 = 0; sq_64 < 64; sq_64++) {

         sq = SQUARE_FROM_64(sq_64);
         piece = board->square[sq];

         if (COLOUR_IS(piece,colour) && PIECE_IS_PAWN(piece)) {

            if (pos >= 8 || SQUARE_IS_PROMOTE(sq)) my_fatal("board_init_list(): illegal position\n");
            ASSERT(pos>=0&&pos<8);

            board->pos[sq] = pos;
            board->pawn[colour][pos] = sq;
            pos++;

            board->piece_nb++;
            board->number[PIECE_TO_12(piece)]++;
            board->pawn_file[colour][SQUARE_FILE(sq)] |= BIT(PAWN_RANK(sq,colour));
         }
      }

      ASSERT(pos>=0&&pos<=8);
      board->pawn[colour][pos] = SquareNone;
      board->pawn_size[colour] = pos;

      if (board->piece_size[colour] + board->pawn_size[colour] > 16) my_fatal("board_init_list(): illegal position\n");
   }

   // last square

   board->cap_sq = SquareNone;

   // PST

   board->opening = board_opening(board);
   board->endgame = board_endgame(board);

   // hash key

   for (i = 0; i < board->ply_nb; i++) board->stack[i] = 0; // HACK
   board->sp = board->ply_nb;

   board->key = hash_key(board);
   board->pawn_key = hash_pawn_key(board);
   board->material_key = hash_material_key(board);

   // legality

   if (!board_is_legal(board)) my_fatal("board_init_list(): illegal position\n");

   // debug

   ASSERT(board_is_ok(board));
}

// board_is_legal()

bool board_is_legal(const board_t * board) {

   ASSERT(board!=NULL);

   return !IS_IN_CHECK(board,COLOUR_OPP(board->turn));
}

// board_is_check()

bool board_is_check(const board_t * board) {

   ASSERT(board!=NULL);

   return IS_IN_CHECK(board,board->turn);
}

// board_is_mate()

bool board_is_mate(const board_t * board) {

   attack_t attack[1];

   ASSERT(board!=NULL);

   attack_set(attack,board);

   if (!ATTACK_IN_CHECK(attack)) return false; // not in check => not mate
   if (legal_evasion_exist(board,attack)) return false; // legal move => not mate

   return true; // in check and no legal move => mate
}

// board_is_stalemate()

bool board_is_stalemate(board_t * board) {

   list_t list[1];
   int i, move;

   ASSERT(board!=NULL);

   // init

   if (IS_IN_CHECK(board,board->turn)) return false; // in check => not stalemate

   // move loop

   gen_moves(list,board);

   for (i = 0; i < LIST_SIZE(list); i++) {
      move = LIST_MOVE(list,i);
      if (pseudo_is_legal(move,board)) return false; // legal move => not stalemate
   }

   return true; // in check and no legal move => mate
}

// board_is_repetition()

bool board_is_repetition(const board_t * board) {

   int i;

   ASSERT(board!=NULL);

   // 50-move rule

   if (board->ply_nb >= 100) { // potential draw

      if (board->ply_nb > 100) return true;

      ASSERT(board->ply_nb==100);
      return !board_is_mate(board);
   }

   // position repetition

   ASSERT(board->sp>=board->ply_nb);

   for (i = 4; i <= board->ply_nb; i += 2) {
      if (board->stack[board->sp-i] == board->key) return true;
   }

   return false;
}

// board_opening()

int board_opening(const board_t * board) {

   int opening;
   int colour;
   const sq_t * ptr;
   int sq, piece;

   ASSERT(board!=NULL);

   opening = 0;

   for (colour = 0; colour < ColourNb; colour++) {

      for (ptr = &board->piece[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         opening += PST(PIECE_TO_12(piece),SQUARE_TO_64(sq),Opening);
      }

      for (ptr = &board->pawn[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         opening += PST(PIECE_TO_12(piece),SQUARE_TO_64(sq),Opening);
      }
   }

   return opening;
}

// board_endgame()

int board_endgame(const board_t * board) {

   int endgame;
   int colour;
   const sq_t * ptr;
   int sq, piece;

   ASSERT(board!=NULL);

   endgame = 0;

   for (colour = 0; colour < ColourNb; colour++) {

      for (ptr = &board->piece[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         endgame += PST(PIECE_TO_12(piece),SQUARE_TO_64(sq),Endgame);
      }

      for (ptr = &board->pawn[colour][0]; (sq=*ptr) != SquareNone; ptr++) {
         piece = board->square[sq];
         endgame += PST(PIECE_TO_12(piece),SQUARE_TO_64(sq),Endgame);
      }
   }

   return endgame;
}

// end of board.cpp

