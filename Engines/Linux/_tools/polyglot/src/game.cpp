
// game.cpp

// includes

#include "attack.h"
#include "board.h"
#include "fen.h"
#include "game.h"
#include "list.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "piece.h"
#include "square.h"
#include "util.h"

// constants

static const bool UseSlowDebug = false;

// variables

game_t Game[1];

// prototypes

static void game_update      (game_t * game);
static int  game_comp_status (const game_t * game);

// functions

// game_is_ok()

bool game_is_ok(const game_t * game) {

   board_t board[1];
   int pos, move;

   if (game == NULL) return false;

   if (game->size < 0 || game->size > GameSize) return false;
   if (game->pos < 0 || game->pos > game->size) return false;

   // optional heavy DEBUG mode

   if (!UseSlowDebug) return true;

   if (!board_is_ok(game->start_board)) return false;

   board_copy(board,game->start_board);

   for (pos = 0; pos <= game->size; pos++) {

      if (pos == game->pos) {
         if (!board_equal(game->board,board)) return false;
      }

      if (pos >= game->size) break;

      if (game->key[pos] != board->key) return false;

      move = game->move[pos];
      //if (!move_is_legal(move,board));  //huh??
	  if (!move_is_legal(move,board)) return false;  

      move_do(board,move);
   }

   if (game->status != game_comp_status(game)) return false;

   return true;
}

// game_clear()

void game_clear(game_t * game) {

   ASSERT(game!=NULL);

   game_init(game,StartFen);
}

// game_init()

bool game_init(game_t * game, const char fen[]) {

   ASSERT(game!=NULL);
   ASSERT(fen!=NULL);

   if (!board_from_fen(game->start_board,fen)) return false;

   game->size = 0;

   board_copy(game->board,game->start_board);
   game->pos = 0;

   game_update(game);

   return true;
}

// game_status()

int game_status(const game_t * game) {

   ASSERT(game!=NULL);

   return game->status;
}

// game_size()

int game_size(const game_t * game) {

   ASSERT(game!=NULL);

   return game->size;
}

// game_pos()

int game_pos(const game_t * game) {

   ASSERT(game!=NULL);

   return game->pos;
}

// game_move()

int game_move(const game_t * game, int pos) {

   ASSERT(game!=NULL);
   ASSERT(pos>=0&&pos<game->pos);

   return game->move[pos];
}

// game_get_board()

void game_get_board(const game_t * game, board_t * board, int pos) {

   int start;
   int i;

   ASSERT(game!=NULL);
   ASSERT(board!=NULL);
   ASSERT(pos==-1||(pos>=0&&pos<=game->size)); // HACK

   if (pos < 0) pos = game->pos;

   if (pos >= game->pos) { // forward from current position
      start = game->pos;
      board_copy(board,game->board);
   } else { // backward => replay the whole game
      start = 0;
      board_copy(board,game->start_board);
   }

   for (i = start; i < pos; i++) move_do(board,game->move[i]);
}

// game_turn()

int game_turn(const game_t * game) {

   ASSERT(game!=NULL);

   return game->board->turn;
}

// game_move_nb()

int game_move_nb(const game_t * game) {

   ASSERT(game!=NULL);

   return game->board->move_nb;
}

// game_add_move()

void game_add_move(game_t * game, int move) {

   ASSERT(game!=NULL);
   ASSERT(move_is_ok(move));

   ASSERT(move_is_legal(move,game->board));

   if (game->pos >= GameSize) my_fatal("game_add_move(): game overflow\n");

   game->move[game->pos] = move;
   game->key[game->pos] = game->board->key;

   move_do(game->board,move);
   game->pos++;

   game->size = game->pos; // truncate game, HACK: before calling game_is_ok() in game_update()

   game_update(game);
}

// game_rem_move()

void game_rem_move(game_t * game) {

   ASSERT(game!=NULL);

   game_goto(game,game->pos-1);

   game->size = game->pos; // truncate game
}

// game_goto()

void game_goto(game_t * game, int pos) {

   int i;

   ASSERT(game!=NULL);
   ASSERT(pos>=0&&pos<=game->size);

   if (pos < game->pos) { // going backward => replay the whole game
      board_copy(game->board,game->start_board);
      game->pos = 0;
   }

   for (i = game->pos; i < pos; i++) move_do(game->board,game->move[i]);
   ASSERT(i==pos);

   game->pos = pos;

   game_update(game);
}

// game_disp()

void game_disp(const game_t * game) {

   board_t board[1];
   int i, move;

   ASSERT(game_is_ok(game));

   board_copy(board,game->start_board);

   board_disp(board);

   for (i = 0; i < game->pos; i++) {

      move = game->move[i];
      move_disp(move,board);

      move_do(board,move);
   }

   my_log("POLYGLOT\n");

   board_disp(board);
}

// game_update()

static void game_update(game_t * game) {

   ASSERT(game!=NULL);

   game->status = game_comp_status(game);

   ASSERT(game_is_ok(game));
}

// game_comp_status()

static int game_comp_status(const game_t * game) {

   int i, n;
   int wb, bb;
   const board_t * board;
   uint64 key;
   int start;

   ASSERT(game!=NULL);

   // init

   board = game->board;

   // mate and stalemate

   if (!board_can_play(board)) {
      if (false) {
      } else if (is_in_check(board,Black)) { // HACK
         return WHITE_MATES;
      } else if (is_in_check(board,White)) { // HACK
         return BLACK_MATES;
      } else {
         return STALEMATE;
      }
   }

   // insufficient material

   if (board->number[WhitePawn12]  == 0
    && board->number[BlackPawn12]  == 0
    && board->number[WhiteQueen12] == 0
    && board->number[BlackQueen12] == 0
    && board->number[WhiteRook12]  == 0
    && board->number[BlackRook12]  == 0) {

      if (board->number[WhiteBishop12]
        + board->number[BlackBishop12]
        + board->number[WhiteKnight12]
        + board->number[BlackKnight12] <= 1) { // KK, KBK and KNK

         return DRAW_MATERIAL;

      } else if (board->number[WhiteBishop12] == 1
              && board->number[BlackBishop12] == 1
              && board->number[WhiteKnight12] == 0
              && board->number[BlackKnight12] == 0) {

         wb = board->list[White][1]; // HACK
         bb = board->list[Black][1]; // HACK

         if (square_colour(wb) == square_colour(bb)) { // KBKB
            return DRAW_MATERIAL;
         }
      }
   }

   // 50-move rule

   if (board->ply_nb >= 100) return DRAW_FIFTY;

   // position repetition

   key = board->key;
   n = 0;

   start = game->pos - board->ply_nb;
   if (start < 0) start = 0;

   for (i = game->pos-4; i >= start; i -= 2) {
      if (game->key[i] == key) {
         if (++n == 2) return DRAW_REPETITION;
      }
   }

   return PLAYING;
}

// end of game.cpp

