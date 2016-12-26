
// search.h

#ifndef SEARCH_H
#define SEARCH_H

// includes

#include "board.h"
#include "list.h"
#include "move.h"
#include "util.h"

// macros

#define ABS(X)     ((X) <  0  ? -(X) : (X))
#define MIN(X, Y)  ((X) < (Y) ?  (X) : (Y))
#define MAX(X, Y)  ((X) > (Y) ?  (X) : (Y))

// constants

const int MaxThreads = 16;
const int MinEGBBHeight = 4;

const int DepthMax = 64;
const int HeightMax = 256;
const int GameMaxPlies = 1024; // WHM 512 move games never happen, 50 move rule, etc.

const int SearchNormal = 0;
const int SearchShort  = 1;

const int SearchUnknown = 0;
const int SearchUpper   = 1;
const int SearchLower   = 2;
const int SearchExact   = 3;

// types

struct search_param_t {
// int move;
// int best_move;
// int threat_move;
// bool reduced;
   bool in_check; // WHM
   bool free_queen_check; // WHM
   bool cap_extended; // Thomas idea
};

struct search_input_t {
   board_t board[1];
   list_t list[1];
   bool infinite;
   bool depth_is_limited;
   bool first_search_not_done; //                                  WHM
   int depth_limit;
   int multipv;
   int plies_from_startpos; //                                     WHM
   int max_book_moves; // = option_get_int("Max Book Moves")       WHM
   int max_book_plies; // = 2 * SearchInput.max_book_moves  -  1   WHM
   volatile bool exit_engine;
   bool time_is_limited;
   bool input_is_startpos; //                                      WHM
   bool UseLearn; //                                               WHM
   bool UseFPEGLearn; //                                           WHM
   bool GenLearn; //                                               WHM
   bool AdaptiveLearning; //                                       WHM
   bool we_have_learned_our_lesson; //                             WHM
   double time_limit_1;
   double time_limit_M; //                                         WHM
   double time_limit_2;
   mv_t moves_array[GameMaxPlies]; //                              WHM
   int  best_values_profile[GameMaxPlies]; //                      WHM
   char input_fen[256]; //                                         WHM
};

struct search_info_t {
   volatile bool stop_command; // for quit and any other engine shutdown
   volatile bool is_stopped; // thread is stopped and has been returned from search_smp()
   volatile bool stop; // thread needs to stop normal search...
   int check_nb;
   int check_inc;
   double last_time;
};

struct search_root_t {
   list_t list[1];
   int depth;
   int move;
   int move_pos;
   int move_nb;
   bool bad;
   bool change;
   bool easy;
   bool flag;
};

struct search_best_t {
   int move;
   int value;
   int flags;
   mv_t pv[HeightMax];
};

struct search_current_t {
   board_t board[1];
   my_timer_t timer[1];
   int max_depth;
   int act_iteration_depth; // Thomas
   int egbb_height; // WHM
   sint64 node_nb;
   double time;
   double cpu;
};

// variables

extern /*UCI*/ bool Tactical[MaxThreads];
extern /*UCI*/ bool Mixed; // half threads Tactical, half Positional

// WHM the whole mp design hinges on last_complete_depth being correct!  
extern volatile int last_complete_depth; // shared.  
extern volatile int last_tactical_depth; // shared.  
extern volatile int claimed_moves_array[DepthMax][256]; // shared.  
extern volatile int claimed_moves_mixed[DepthMax][256]; // shared.  
extern volatile int claimed_nb_array[DepthMax];  // shared.  
extern volatile int root_alpha[DepthMax]; // shared.  
extern volatile int last_displayed_depth; // shared.  

// WCCC rules: need a converging mate once mate declared!  
extern volatile bool last_best_was_mate; // shared.  
extern volatile bool current_best_is_mate; // shared.  
extern volatile bool SkipMoves; // shared.  

extern search_input_t SearchInput[1]; // read only shared.  
extern search_info_t SearchInfo[MaxThreads][1];
extern search_root_t SearchRoot[MaxThreads][1];
extern search_current_t SearchCurrent[MaxThreads][1];
extern search_best_t SearchReport[MaxThreads][DepthMax][1];
extern volatile search_best_t SearchBestAtDepth[DepthMax]; // shared.  
extern volatile bool SearchBestAtDepthIsBusy[DepthMax][MaxThreads]; // shared.  

extern int LearnThresh; // WHM learning algorithm threshold  UCI now.  

// functions

extern bool depth_is_ok           (int depth);
extern bool height_is_ok          (int height);

extern void clear_SearchInput     ();
extern void search_clear          ();
extern void search                ();

extern void search_report_and_update_best (int depth, int ThreadId);
extern void search_update_root            (           int ThreadId);
extern void search_update_current         (           int ThreadId);

extern void search_check          (int ThreadId);

extern void start_suspended_threads();
extern void resume_threads();
extern void get_NumberProcessors();

extern void search_send_SearchBestAtDepth(int depth);

extern int  best_and_ponder_legality(int depth);

#endif // !defined SEARCH_H

// end of search.h

