
// game.h

#ifndef GAME_H
#define GAME_H

// includes

#include "board.h"
#include "move.h"
#include "util.h"

// constants

const int GameSize = 4096;

enum status_t {
   PLAYING,
   WHITE_MATES,
   BLACK_MATES,
   STALEMATE,
   DRAW_MATERIAL,
   DRAW_FIFTY,
   DRAW_REPETITION
};

// types

struct game_t {
   board_t start_board[1];
   board_t board[1];
   sint16 size;
   sint16 pos;
   sint8 status;
   move_t move[GameSize];
   uint64 key[GameSize];
};

// variables

extern game_t Game[1];

// functions

extern bool game_is_ok     (const game_t * game);

extern void game_clear     (game_t * game);
extern bool game_init      (game_t * game, const char fen[]);

extern int  game_status    (const game_t * game);

extern int  game_size      (const game_t * game);
extern int  game_pos       (const game_t * game);
extern int  game_move      (const game_t * game, int pos);

extern void game_get_board (const game_t * game, board_t * board, int pos = -1);
extern int  game_turn      (const game_t * game);
extern int  game_move_nb   (const game_t * game);

extern void game_add_move  (game_t * game, int move);
extern void game_rem_move  (game_t * game);

extern void game_goto      (game_t * game, int pos);

extern void game_disp      (const game_t * game);

#endif // !defined GAME_H

// end of game.h

