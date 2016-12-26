#ifndef _WIN32

// search.cpp

// includes

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "engine.h"
#include "fen.h"
#include "line.h"
#include "list.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "move_legal.h"
#include "option.h"
#include "parse.h"
#include "san.h"
#include "search.h"
#include "uci.h"
#include "util.h"

// variables

static int Depth;

static int BestMove;
static int BestValue;
static move_t BestPV[LineSize];

static sint64 NodeNb;
static sint64 LeafNb;
static double Time;

static int Move;
static int MovePos;
static int MoveNb;

// prototypes

#if DEBUG
static bool depth_is_ok (int depth);
#endif

static void perft       (const board_t * board, int depth);

// functions

// depth_is_ok()
#if DEBUG

static bool depth_is_ok(int depth) {

   return depth >= 0 && depth < DepthMax;
}

#endif
// search()

void search(const board_t * board, int depth_max, double time_max) {

   char string[256];

   ASSERT(board_is_ok(board));
   ASSERT(depth_max>=1&&depth_max<DepthMax);
   ASSERT(time_max>=0.0);

   // engine

   Depth = 0;

   BestMove = MoveNone;
   BestValue = 0;
   line_clear(BestPV);

   NodeNb = 0;
   LeafNb = 0;
   Time = 0.0;

   Move = MoveNone;
   MovePos = 0;
   MoveNb = 0;

   // init

   uci_send_ucinewgame(Uci);
   uci_send_isready_sync(Uci);

   // position

   if (!board_to_fen(board,string,256)) ASSERT(false);
   engine_send(Engine,"position fen %s",string);

   // search

   engine_send_queue(Engine,"go");

   engine_send_queue(Engine," movetime %.0f",time_max*1000.0);
   engine_send_queue(Engine," depth %d",depth_max);

   engine_send(Engine,""); // newline

   // wait for feed-back

   while (true) {

      engine_get(Engine,string,256);

      if (false) {

      } else if (match(string,"bestmove * ponder *")) {

         BestMove = move_from_can(Star[0],board);
         ASSERT(BestMove!=MoveNone&&move_is_legal(BestMove,board));

         break;

      } else if (match(string,"bestmove *")) {

         BestMove = move_from_can(Star[0],board);
         ASSERT(BestMove!=MoveNone&&move_is_legal(BestMove,board));

         break;
      }
   }

   printf("\n");
}

// search_perft()

void search_perft(const board_t * board, int depth_max) {

   int depth;
   my_timer_t timer[1];
   double time, speed;

   ASSERT(board_is_ok(board));
   ASSERT(depth_max>=1&&depth_max<DepthMax);

   // init

   board_disp(board);

   // iterative deepening

   for (depth = 1; depth <= depth_max; depth++) {

      // init

      NodeNb = 0;
      LeafNb = 0;

      my_timer_reset(timer);

      my_timer_start(timer);
      perft(board,depth);
      my_timer_stop(timer);

      time = my_timer_elapsed_real(timer);//my_timer_elapsed_cpu(timer);
      speed = (time < 0.01) ? 0.0 : double(NodeNb) / time;

      printf("%2d " S64_FORMAT_10 " " S64_FORMAT_10 " %7.2f %7.0f\n",depth,NodeNb,LeafNb,time,speed);
   }

   printf("\n");
}

// perft()

static void perft(const board_t * board, int depth) {

   int me;
   list_t list[1];
   int i, move;
   board_t new_board[1];

   ASSERT(board_is_ok(board));
   ASSERT(depth_is_ok(depth));

   ASSERT(!is_in_check(board,colour_opp(board->turn)));

   // init

   NodeNb++;

   // leaf

   if (depth == 0) {
      LeafNb++;
      return;
   }

   // more init

   me = board->turn;

   // move loop

   gen_moves(list,board);

   for (i = 0; i < list_size(list); i++) {

      move = list_move(list,i);

      board_copy(new_board,board);
      move_do(new_board,move);

      if (!is_in_check(new_board,me)) perft(new_board,depth-1);
   }
}

// end of search.cpp

#endif
