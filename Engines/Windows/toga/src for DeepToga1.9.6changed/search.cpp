
// search.cpp

// includes

#include <windows.h>
#include <process.h>
#include <cstring>

//#include "attack.h"
//#include "board.h"
#include "book.h"
//#include "colour.h"
//#include "list.h"
#include "material.h"
//#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "move_legal.h"
#include "option.h"
//#include "pawn.h"
#include "protocol.h"
#include "pv.h"
#include "search.h"
#include "search_full.h"
#include "sort.h"
#include "trans.h"
//#include "util.h"
#include "value.h"

// constants

static const bool UseCpuTime = false; // false
static const bool UseEvent = true; // true

static const bool UseShortSearch = true;
static const int ShortSearchDepth = 1;

static const bool DispBest = true; // true
static const int  DispBestMinDepth = 7;
static const bool DispDepthStart = true; // WHM; was true; // true
static const bool DispDepthEnd = true; // true
static const bool DispRoot = true; // true
static const bool DispStat = true; // true
static const bool DispAllDebug = false; // true; // false to see what the GUI sees.

static const bool UseEasy = true; // singular move
static const int EasyThreshold = 150;
static const double EasyRatio = 0.125; // was 0.20

static const bool UseEarly = true; // early iteration end
static const double EarlyRatio = 0.60;

static const bool UseBad = true;
static const int BadThreshold = 35; // was 50;, new search hitches up way less... // 50
static const bool UseExtension = true;

static const int DeSync[MaxThreads] = {0, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 143, 149, 153, 157, 161, };

// variables

static HANDLE thread_handle[MaxThreads];
static int ThreadIds[MaxThreads]; // WHM moved this up here
static unsigned internalThreadIds[MaxThreads]; // WHM moved this up here
static volatile int last_depth_of_thread[MaxThreads]; // shared.
static volatile int Number_Tactical_Threads_Started[MaxThreads][DepthMax]; // shared.
static volatile int NumberPositionalThreads_Started[MaxThreads][DepthMax]; // shared.

// extern variables

/*UCI*/ bool Tactical[MaxThreads];
/*UCI*/ bool Mixed; // half threads Tactical, half Positional

volatile int NumberThreads; // UCI
volatile int NumberProcessors; // WHM & Vadim Dem
volatile int NumberThreadsInternal; // WHM
volatile int ThreadsPerDepth;

volatile int last_complete_depth; // shared.
volatile int last_tactical_depth; // shared.  // WHM for "Mixed" mode.
volatile int claimed_moves_array[DepthMax][256]; // shared.
volatile int claimed_moves_mixed[DepthMax][256]; // shared.
volatile int claimed_nb_array[DepthMax]; // shared.
volatile int root_alpha[DepthMax]; // shared.
volatile int last_displayed_depth; // shared.

// WCCC rules: need a converging mate once mate declared!
volatile bool last_best_was_mate;
volatile bool current_best_is_mate = false;
volatile bool SkipMoves    = false;

// WHM;  Decided to implement root_alpha[DepthMax]
//       with the breadcrumbs MP algorithm...

search_input_t SearchInput[1]; // read only shared.
search_info_t SearchInfo[MaxThreads][1]; // SearchInfo[ThreadId]->stop shared.
search_root_t SearchRoot[MaxThreads][1];
search_current_t SearchCurrent[MaxThreads][1];
search_best_t SearchReport[MaxThreads][DepthMax][1];
volatile search_best_t SearchBestAtDepth[DepthMax]; // shared.
volatile bool SearchBestAtDepthIsBusy[DepthMax][MaxThreads];
static volatile bool   search_thread_function_IsBusy[MaxThreads]; // shared.

static volatile bool SendNewBest; // shared.  Used to send only from thread 0
static volatile int NewBestDepth; // shared.  Used to send only from thread 0

// prototypes

static void search_send_stat                  (int ThreadId);
static unsigned __stdcall search_thread       (void *param);
static void search_smp                        (int ThreadId);
static int Get_Number_Tactical_Threads_Started(int depth);
static int Get_NumberPositionalThreads_Started(int depth);
static void redo_search_report_and_update_best();
static void get_last_complete_depth(int ThreadId);

// functions

// depth_is_ok()

bool depth_is_ok(int depth) {

   return depth > -128 && depth < DepthMax;
}

// height_is_ok()

bool height_is_ok(int height) {

   return height >= 0 && height < HeightMax;
}

// clear_SearchInput()

void clear_SearchInput() {

   // SearchInput

   SearchInput->infinite = false;
   SearchInput->depth_is_limited = false;
   SearchInput->depth_limit = 0;
   SearchInput->time_is_limited = false;
   SearchInput->time_limit_1 = 0.0;
   SearchInput->time_limit_M = 0.0;
   SearchInput->time_limit_2 = 0.0;
}

// search_clear()

void search_clear() {

    int ThreadId;
    int depth;

    SendNewBest = false; // shared.
    NewBestDepth = 0; // shared.

   for (ThreadId = 0; ThreadId < MaxThreads; ThreadId++) search_thread_function_IsBusy[ThreadId] = false;

   // SearchInfo

    for (ThreadId = 0; ThreadId < MaxThreads; ThreadId++){
        SearchInfo[ThreadId]->stop_command = false;
        SearchInfo[ThreadId]->is_stopped = false;
        SearchInfo[ThreadId]->stop = false;
        SearchInfo[ThreadId]->check_nb  = 10000 + DeSync[ThreadId]; // to de-synchronize threads, was 10000; // was 100000
        SearchInfo[ThreadId]->check_inc = 10000 + DeSync[ThreadId]; // to de-synchronize threads, was 10000; // was 100000
        SearchInfo[ThreadId]->last_time = 0.0;

        // SearchReport

        for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->move = MoveNone;
        for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->value = ValueNone;
        for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->flags = SearchUnknown;
        for (depth = 0; depth < DepthMax; depth++) PV_CLEAR(SearchReport[ThreadId][depth]->pv);

        // SearchRoot

        SearchRoot[ThreadId]->depth = 0;
        SearchRoot[ThreadId]->move = MoveNone;
        SearchRoot[ThreadId]->move_pos = 0;
        SearchRoot[ThreadId]->move_nb = 0;
        SearchRoot[ThreadId]->bad = false;
        SearchRoot[ThreadId]->change = false;
        SearchRoot[ThreadId]->easy = false;
        SearchRoot[ThreadId]->flag = false;

        // SearchCurrent

        SearchCurrent[ThreadId]->max_depth = 0;
        SearchCurrent[ThreadId]->node_nb = 0;
        SearchCurrent[ThreadId]->time = 0.0;
        SearchCurrent[ThreadId]->cpu = 0.0;
    }

    //  "Threads Per Depth"
    ThreadsPerDepth = option_get_int("Threads Per Depth");
    if (ThreadsPerDepth > NumberThreadsInternal) {
        ThreadsPerDepth = NumberThreadsInternal;
    }
    if (NumberThreadsInternal >= 2  &&  my_string_equal(option_get_string("Playing Style"),"Mixed")) {
        ThreadsPerDepth = (1 + ThreadsPerDepth) / 2;
    }

    // WCCC rules: need a converging mate once mate declared!
    if (SearchInput->plies_from_startpos <= 19 && SearchInput->input_is_startpos) {
        last_best_was_mate = false;
    }
    current_best_is_mate = false;

    // root move accounting
    last_complete_depth = 0;
    last_tactical_depth = 0;
    last_displayed_depth = 0;
    for (depth = 0; depth < DepthMax; depth++) {
       for (int i = 0; i < 256; i++) claimed_moves_array[depth][i] = MoveNone;
       for (int i = 0; i < 256; i++) claimed_moves_mixed[depth][i] = 0;
       claimed_nb_array[depth] = 0;
       root_alpha[depth] = ValueNone;
       for (int i = 0; i < MaxThreads; i++) Number_Tactical_Threads_Started[i][depth] = 0;
       for (int i = 0; i < MaxThreads; i++) NumberPositionalThreads_Started[i][depth] = 0;
    }

    for (depth = 0; depth < DepthMax; depth++) {
       SearchBestAtDepth[depth].move = MoveNone;
       SearchBestAtDepth[depth].value = ValueNone;
       SearchBestAtDepth[depth].flags = SearchUnknown;
       PV_CLEAR(SearchBestAtDepth[depth].pv);
       for (int i = 0; i < MaxThreads; i++) SearchBestAtDepthIsBusy[depth][i] = false;
    }

            if (UCI_debug && fp_debug != NULL) {
               fprintf(fp_debug,"search_clear().\n\r");
            }

   // WHM moved this here...
   for (int i = 0; i < MaxThreads; i++) Number_Tactical_Threads_Started[i][0] = 1;
   for (int i = 0; i < MaxThreads; i++) NumberPositionalThreads_Started[i][0] = 1;
}

void start_suspended_threads(){

   int i;

   // start and suspend extra threads

   for (i = 1; i < NumberThreadsInternal; i++) {

      ThreadIds[i-1] = i; // threads 1-3 have id's 0-2.

      if (thread_handle[i-1] <= NULL) { // For GUI's changing "Number of Threads" without engine re-load.
          thread_handle[i-1] = (HANDLE) _beginthreadex( NULL, 0, &search_thread, &ThreadIds[i-1], CREATE_SUSPENDED, &internalThreadIds[i-1]);
      }
   }
}

void resume_threads(){

   int i;

   // resume extra threads

   for (i = 1; i < NumberThreadsInternal; i++){
        ResumeThread(thread_handle[i-1]);
   }
            if (UCI_debug && fp_debug != NULL) {
               fprintf(fp_debug,"exit resume_threads().\n\r");
            }
}

//                 search_thread ()

static unsigned __stdcall search_thread (void *param) { // Thomas

    int ThreadId = *((int*)param);
    int i;

    while (!SearchInput->exit_engine){

        search_smp(ThreadId);

        i = 1 + ThreadId;
        if (i >= NumberThreadsInternal) {
            i  = 0;
        }

        search_thread_function_IsBusy[i] = false; // release another thread to get suspended.

        SearchInfo[ThreadId]->is_stopped = true; // This thread is both returned from search_smp() and suspended in the next line.

        SuspendThread(thread_handle[ThreadId-1]); // This has to be last.
    }

    _endthreadex( 0 );
    return 0;
}

//   get_NumberProcessors()

void get_NumberProcessors() {
   SYSTEM_INFO sys_info;
   GetSystemInfo(&sys_info);
   NumberProcessors = sys_info.dwNumberOfProcessors;
   if (NumberProcessors > MaxThreads) {
       NumberProcessors = MaxThreads;
   }
}



// search()

void search() {

   int move;
   bool all_stopped;
   int ThreadId;
   int depth;
   int ntries = 0;

   SearchInput->multipv = option_get_int("MultiPV") - 1;

   ASSERT(board_is_ok(SearchInput->board));

   // display just one thread so as not to confuse the GUI...
   last_complete_depth = 1;
   last_tactical_depth = 1;
   last_depth_of_thread[0] = 1;
   for (ThreadId = 1; ThreadId < NumberThreadsInternal; ThreadId++){
       last_depth_of_thread[ThreadId] = 1; // 11/24/08 up to one from zero.
   }

   // opening book

   if (option_get_bool("OwnBook") && !SearchInput->infinite) {

      move = book_move(SearchInput->board);

      if (move != MoveNone) {

         // play book move in every thread

            if (UCI_debug && fp_debug != NULL) {
               fprintf(fp_debug,"opening book\n\r");
            }

         for (ThreadId = 0; ThreadId < NumberThreadsInternal; ThreadId++){

            for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->move = move;
            for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->value = ThreadId * (1 - 2 * SearchInput->board->turn);
            for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->flags = SearchExact;
            for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->pv[0] = move;
            for (depth = 0; depth < DepthMax; depth++) SearchReport[ThreadId][depth]->pv[1] = MoveNone;
         }

         search_report_and_update_best(1,0); // depth 1, thread 0

         return;
      }
   }

   // SearchInput

   gen_legal_moves(SearchInput->list,SearchInput->board);

/* // WHM; 4-ply was way too fast for some of the GUI's
   if (LIST_SIZE(SearchInput->list) <= 1 && !SearchInput->GenLearn) {
      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = 7; // 4; // was 1
   } */


   // this search is one newer than last search
   trans_inc_date(Trans);
   sort_init(); // WHM here instead of search_full_init()

   // resume extra threads

   resume_threads();

   // thread  0  too.
   search_smp(0);
   SearchInfo[0]->is_stopped = true;

        if (UCI_debug && fp_debug != NULL) {
           search_update_current(0);
           fprintf(fp_debug,"search(), search_smp(0) exited, ThreadId = %d, time = %0.f\n\r", 0, SearchCurrent[0]->time*1000.0);
           if (!SearchInfo[0]->stop) {
               fprintf(fp_debug,"search(), search_smp(0) exited, SearchInfo[0]->stop is false.  , time = %0.f\n\r", SearchCurrent[0]->time*1000.0);
           }
        }

   // here thread 0 waits for all othes to finish.
   ntries = 0;
   all_stopped = false;
   while (!all_stopped){
       all_stopped = true;
       ntries++;
       if      (ntries >= 16 * 1024) ntries = 0;
       else if (ntries == 15 * 1024) SearchInfo[15]->stop = true;
       else if (ntries == 14 * 1024) SearchInfo[14]->stop = true;
       else if (ntries == 13 * 1024) SearchInfo[13]->stop = true;
       else if (ntries == 12 * 1024) SearchInfo[12]->stop = true;
       else if (ntries == 11 * 1024) SearchInfo[11]->stop = true;
       else if (ntries == 10 * 1024) SearchInfo[10]->stop = true;
       else if (ntries ==  9 * 1024) SearchInfo[ 9]->stop = true;
       else if (ntries ==  8 * 1024) SearchInfo[ 8]->stop = true;
       else if (ntries ==  7 * 1024) SearchInfo[ 7]->stop = true;
       else if (ntries ==  6 * 1024) SearchInfo[ 6]->stop = true;
       else if (ntries ==  5 * 1024) SearchInfo[ 5]->stop = true;
       else if (ntries ==  4 * 1024) SearchInfo[ 4]->stop = true;
       else if (ntries ==  3 * 1024) SearchInfo[ 3]->stop = true;
       else if (ntries ==  2 * 1024) SearchInfo[ 2]->stop = true;
       else if (ntries ==  1 * 1024) SearchInfo[ 1]->stop = true;

       for (ThreadId = 1; ThreadId < NumberThreadsInternal; ThreadId++){
           if (!SearchInfo[ThreadId]->is_stopped) {
                all_stopped = false;
           }
       }
   }

        if (UCI_debug && fp_debug != NULL) {
           search_update_current(0);
           fprintf(fp_debug,"search(), all_stopped, time = %0.f\n\r", SearchCurrent[0]->time*1000.0);
        }

   // all the threads are done so we gotta have a good answer.
   get_last_complete_depth(0);
   ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);

        if (UCI_debug && fp_debug != NULL) {
           search_update_current(0);
           fprintf(fp_debug,"search(), after get_last_complete_depth(), last_complete_depth = %d, time = %0.f\n\r", last_complete_depth, SearchCurrent[0]->time*1000.0);
        }

   // guarantee legal best_move and ponder_move.
   if (NumberThreadsInternal >= 2  &&  0 == best_and_ponder_legality(last_complete_depth)) {


      if (UCI_debug && fp_debug != NULL) {
           search_update_current(0);
           fprintf(fp_debug,"search(), 0 == best_and_ponder_legality(last_complete_depth), time = %0.f, last_complete_depth = %d\n\r", SearchCurrent[0]->time*1000.0, last_complete_depth);
           fprint_board(SearchInput->board, fp_debug);
       }

       redo_search_report_and_update_best();
       get_last_complete_depth(0);

       // guarantee legal best_move and ponder_move.
       if (NumberThreadsInternal >= 2  &&  0 == best_and_ponder_legality(last_complete_depth)) {

           depth = MAX(4,last_complete_depth);
           search_clear();
           SearchInput->depth_is_limited = true;
           SearchInput->depth_limit = depth;
           SearchInfo[0]->stop = false;
           depth = NumberThreadsInternal;
           NumberThreadsInternal = 1;
           search_smp(0);
           SearchInfo[0]->is_stopped = true;
           get_last_complete_depth(0);
           NumberThreadsInternal = depth;

           while(last_complete_depth >= 2  &&  0 == best_and_ponder_legality(last_complete_depth)) last_complete_depth--;

           if (0 == best_and_ponder_legality(last_complete_depth)) {

               if (UCI_debug && fp_debug != NULL) {
                   search_update_current(0);
                   fprintf(fp_debug,"search(), still 0 == best_and_ponder_legality(last_complete_depth), time = %0.f, last_complete_depth = %d\n\r", SearchCurrent[0]->time*1000.0, last_complete_depth);
                   fprint_board(SearchInput->board, fp_debug);
               }
           }
       }
   }
}

// search_smp()

static void search_smp(int ThreadId) {

   int depth;
   int i, ithr;
   sint64 node_nb;
   double speed, time;
   bool last_depth_ok;
   int i1023 = 0;
   int ntries = 0;

   // LC max nps
   double minsecs;
   int maxnps, ms;

   ASSERT(NumberThreadsInternal >= ThreadsPerDepth);

   // SearchRoot
   list_copy(SearchRoot[ThreadId]->list,SearchInput->list);

   // SearchCurrent
   board_copy(SearchCurrent[ThreadId]->board,SearchInput->board);
   my_timer_reset(SearchCurrent[ThreadId]->timer);
   my_timer_start(SearchCurrent[ThreadId]->timer);

   // init

   search_full_init(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,ThreadId);

   // iterative deepening

// for (depth = 1; depth < DepthMax; depth++) {
// for (depth = START_DEPTH(ThreadId); depth < DepthMax; depth += DepthInc) {

   depth = 0;

   while (depth < DepthMax - 1) {

      depth++;

      while (depth < DepthMax &&  Tactical[ThreadId] && Get_Number_Tactical_Threads_Started(depth) >= MIN(ThreadsPerDepth,LIST_SIZE(SearchRoot[ThreadId]->list))) depth++;
      while (depth < DepthMax && !Tactical[ThreadId] && Get_NumberPositionalThreads_Started(depth) >= MIN(ThreadsPerDepth,LIST_SIZE(SearchRoot[ThreadId]->list))) depth++;

      if (depth >= DepthMax) {
         if (search_thread_function_IsBusy[ThreadId]) i1023 = 1023;
         goto get_out_clean;
      }

      if ( Tactical[ThreadId]) Number_Tactical_Threads_Started[ThreadId][depth]++;
      if (!Tactical[ThreadId]) NumberPositionalThreads_Started[ThreadId][depth]++;

      // 2 shots at this
      get_last_complete_depth(ThreadId);

//    ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);

      if (DispDepthStart && 0 == ThreadId) {
         if (depth == 1 + last_complete_depth  ||  DispAllDebug && DEBUG) {
            send("info depth %d",depth);
            last_displayed_depth = depth;
         }
      }

      SearchRoot[ThreadId]->bad = false;
      SearchRoot[ThreadId]->change = false;

      SearchCurrent[ThreadId]->act_iteration_depth = depth; // Thomas
      SearchCurrent[ThreadId]->egbb_height = MAX((1 + 2*depth) / 3, depth - 4); // WHM

//    ASSERT(last_complete_depth > depth - 6);
      ASSERT(last_complete_depth < depth || depth <= 7);
//    ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);

      board_copy(SearchCurrent[ThreadId]->board,SearchInput->board);

      if (UseShortSearch && depth <= ShortSearchDepth) {
         search_full_root(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,depth,SearchShort,ThreadId);
      } else {
         search_full_root(SearchRoot[ThreadId]->list,SearchCurrent[ThreadId]->board,depth,SearchNormal,ThreadId);
      }

      if (SearchInfo[ThreadId]->stop) { // SearchInfo[ThreadId]->stop shared.

          if (search_thread_function_IsBusy[ThreadId]) i1023 = 1023;
          goto get_out_clean;
      }


      last_depth_of_thread[ThreadId] = depth;

      search_update_current(ThreadId);

      // This whole new mp design
      // needs last_complete_depth to be correct!

      // 2 shots at this
      get_last_complete_depth(ThreadId);
      ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);

      if (last_complete_depth == depth  ||  Mixed && last_tactical_depth == depth) {

         node_nb = 0;
         for (i = 0; i < NumberThreadsInternal; i++){
            node_nb += SearchCurrent[ThreadId]->node_nb;
         }
         time = SearchCurrent[ThreadId]->time;

         // LC max NPS
         maxnps = option_get_int("Max NPS");
         if (maxnps < 999999 ) {
            minsecs = node_nb *1.0 / maxnps;
            if (minsecs > time) {
               ms = 1000*(minsecs-time);
               if (time > 0.5) speed = node_nb / time;
               else            speed = 0;
               while (true) {
                  // Send info each 1 sec then GUI knows it is alive
                  send( "info adjusting nps from %.0f to %d then pending to sleep %d ms",speed, maxnps, ms);
                  if(ms>1000) {
                     Sleep(1000);
                     ms -= 1000;
                  }
                  else {
                     Sleep(ms);
                     break;
                  }
               }
               time = minsecs;
               SearchCurrent[0]->time += minsecs-time;
            }
         }

         if (time > 0.5) speed = node_nb / time;
         else            speed = 0;

         if (DispDepthEnd && 0 == ThreadId && depth >= last_displayed_depth) {
            send(               "info depth %d seldepth %d time %.0f nodes " S64_FORMAT " nps %.0f",     depth,SearchCurrent[ThreadId]->max_depth,SearchCurrent[ThreadId]->time*1000.0,node_nb,speed);
            last_displayed_depth = depth;
         }
      }

      // update search info

      if (depth == 1) {

         if (   LIST_SIZE(SearchRoot[ThreadId]->list) >= 2
             && LIST_VALUE(SearchRoot[ThreadId]->list,0) >= LIST_VALUE(SearchRoot[ThreadId]->list,1) + EasyThreshold) {

            SearchRoot[ThreadId]->easy = true;
         }

      } else { // WHM; if the distance in centi-pawns between the best move and the rest shrinks, not easy.

         if (   LIST_SIZE(SearchRoot[ThreadId]->list) >= 2
             && LIST_VALUE(SearchRoot[ThreadId]->list,0) <  LIST_VALUE(SearchRoot[ThreadId]->list,1) + EasyThreshold) {

            SearchRoot[ThreadId]->easy = false;
         }
      }

      if (UseBad && depth > 1) {

         // MP FIX
         // This was moved here as the structure SearchBestAtDepth
         // has the information that we need to do this here.
         // Only after a depth has been completed can we compare to the previous depth.

         ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);

         if (SearchInput->multipv == 0 && depth == last_complete_depth) {

            last_depth_ok = true;
            for (ithr = 0; ithr < NumberThreadsInternal && last_depth_ok; ithr++) {
                if (last_depth_of_thread[ithr] < depth - 1) last_depth_ok = false;
            }

            if (last_depth_ok) {

               if (SearchBestAtDepth[depth].value <= SearchBestAtDepth[depth - 1].value - BadThreshold) {

                  SearchRoot[ThreadId]->bad = true;
               }
            }

            if (SearchRoot[ThreadId]->bad) {
                SearchRoot[ThreadId]->easy = false;
                SearchRoot[ThreadId]->flag = false;
            }
         } // MP FIX
      }

      // stop search?

      get_last_complete_depth(ThreadId);

      if (SearchInput->depth_is_limited) {
         if (last_complete_depth >= SearchInput->depth_limit) {
            SearchRoot[ThreadId]->flag = true;
         } else if (Mixed && last_tactical_depth >= SearchInput->depth_limit) {
            SearchRoot[ThreadId]->flag = true;
         }
      }

      ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);
//    ASSERT(last_tactical_depth >= 1 && last_tactical_depth < DepthMax);

      if (         depth <= last_complete_depth                       ||     // if depth > last_complete_depth then
          Mixed && depth <= last_tactical_depth && Tactical[ThreadId]    ) { // another thread is finishing up moves at this depth.

         if (SearchInput->time_is_limited
          && (SearchBestAtDepth[depth].value > +ValueEvalInf || !last_best_was_mate)
          && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1
          && !SearchRoot[ThreadId]->bad) {

            SearchRoot[ThreadId]->flag = true;
         }

         if (UseEasy
          && SearchInput->time_is_limited
          && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1 * EasyRatio
          && SearchRoot[ThreadId]->easy
          // WCCC rules: need a converging mate once mate declared!
          && (SearchBestAtDepth[depth].value > +ValueEvalInf || !last_best_was_mate) // Only to avoid snatching LOSS from WIN!!!!!!!
          && !SearchRoot[ThreadId]->bad       // moved these outta the ASSERTS
          && !SearchRoot[ThreadId]->change) { // moved these outta the ASSERTS

            SearchRoot[ThreadId]->flag = true;
         }

         if (UseEarly
          && SearchInput->time_is_limited
          && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_1 * EarlyRatio
          // WCCC rules: need a converging mate once mate declared!
          && (SearchBestAtDepth[depth].value > +ValueEvalInf || !last_best_was_mate)
          && !SearchRoot[ThreadId]->bad
          && !SearchRoot[ThreadId]->change
          && !SearchInput->first_search_not_done) { // do not use early on the first search!

            if (SearchBestAtDepth[depth].move == SearchBestAtDepth[depth+1].move) { // agreement with the next depth also added by WHM
               SearchRoot[ThreadId]->flag = true;
            } else if               (MoveNone == SearchBestAtDepth[depth+1].move) {
               SearchRoot[ThreadId]->flag = true;
            }
         }
      }

      if (SearchRoot[ThreadId]->flag && !SearchInput->infinite) {
          if (search_thread_function_IsBusy[ThreadId]) i1023 = 1023;
          goto get_out_clean;
      }
   }
   // end of depth loop.


get_out_clean: // search_thread() does not seem to throw another instance, so we only want one thread at a time in there.

   ntries++;
   while(i1023 > 0) i1023--;

   if (search_thread_function_IsBusy[ThreadId] && ntries < 1024) {
       i1023 = 1023;
       goto get_out_clean;
   }

              if (UCI_debug && fp_debug != NULL && search_thread_function_IsBusy[ThreadId]) {
                 search_update_current(ThreadId);
                 fprintf(fp_debug,"search_thread_function_IsBusy == TRUE, and trying to exit from search_smp(int ThreadId), ThreadId == %d  depth = %d time = %0.f\n\r", ThreadId, depth, SearchCurrent[ThreadId]->time*1000.0);
              }

   // shared.
   search_thread_function_IsBusy[ThreadId] = true;
   i = 1 + ThreadId;
   if (i >= NumberThreadsInternal) i = 0;
   SearchInfo[i]->stop = true; // WHM; tell the next thread to stop.  1.8.11 let the threads exit one at a time...
}

//   search_report_and_update_best()

void search_report_and_update_best(int depth, int ThreadId) {

   int move, value, flags, max_depth;
   mv_t pv[HeightMax];
   int move_pos;
   double time;
   sint64 node_nb;
   int mate;
   char move_string[256], pv_string[512];
   bool new_best;

   search_update_current(ThreadId);

   move = SearchReport[ThreadId][depth]->move;
   value = SearchReport[ThreadId][depth]->value;
   flags = SearchReport[ThreadId][depth]->flags; ASSERT(SearchExact == flags);
   pv_copy(pv, SearchReport[ThreadId][depth]->pv);
   ASSERT(pv_is_ok(pv));

   new_best = false;

   if (SearchBestAtDepth[depth].value <= value) { // shared.   // at the same value, the later guy used deeper trans data!

       if (SearchBestAtDepth[depth].move != move  ||  ValueNone >= SearchBestAtDepth[depth].value) new_best = true; // shared.

       SearchBestAtDepth[depth].move  = move; // shared.
       SearchBestAtDepth[depth].value = value; // shared.
       SearchBestAtDepth[depth].flags = flags; // shared.
       pv_copy(SearchBestAtDepth[depth].pv, pv); // shared.
       ASSERT(pv_is_ok(SearchBestAtDepth[depth].pv)); // shared.
   }

   get_last_complete_depth(ThreadId);
   ASSERT(last_complete_depth >= 0 && last_complete_depth < DepthMax);

// if (depth == 1 + last_complete_depth  ||  DispAllDebug && DEBUG) {
//    if (new_best || (SearchRoot[ThreadId]->move_pos <= SearchInput->multipv && SearchInput->multipv >= 1)  ||  DispAllDebug && DEBUG) { // multipv is back

         ASSERT(flags == SearchExact);

         if (DispBest) {

            if (0 == ThreadId) {

               if (depth >= DispBestMinDepth && depth >= last_displayed_depth) {

                  max_depth = SearchCurrent[ThreadId]->max_depth;
                  time = SearchCurrent[ThreadId]->time; // double seconds, need to send integer milli-seconds.
                  node_nb = SearchCurrent[0]->node_nb;
                  for (int i = 1; i < NumberThreadsInternal; i++) node_nb += SearchCurrent[i]->node_nb;

                  move_to_string(move,move_string,256);
                  if (pv_is_ok(pv)) {
                     pv_to_string(pv,pv_string,512);
                  } else {
                     pv_string[0] = ' ';
                     pv_string[1] = '\0';
                     if (UCI_debug && fp_debug != NULL) {
                        search_update_current(ThreadId);
                        fprintf(fp_debug,"search_report_and_update_best() pv[] was not ok, time = %0.f\n\r", ThreadId, SearchCurrent[ThreadId]->time*1000.0);
                     }
                  }

                  mate = value_to_mate(value);

                  move_pos = SearchRoot[ThreadId]->move_pos;
                  if (move_pos > SearchInput->multipv) {
                      if (new_best) move_pos = 0; // new best best, overwrite prev best.
                      else          move_pos = SearchInput->multipv;
                  }

                  if (last_displayed_depth <= depth) { // shared.

                     if (mate == 0) {

                        // normal evaluation

                        if (false) {
                        } else if (flags == SearchExact) {
                           send("info multipv %d depth %d seldepth %d score cp %d time %.0f nodes " S64_FORMAT " pv %s",           1+move_pos,depth,max_depth,value,time*1000.0,node_nb,pv_string);
                        } else if (flags == SearchLower) {
                           ASSERT(false);
                           send("info multipv %d depth %d seldepth %d score cp %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",1+move_pos,depth,max_depth,value,time*1000.0,node_nb,pv_string);
                        } else if (flags == SearchUpper) {
                           ASSERT(false);
                           send("info multipv %d depth %d seldepth %d score cp %d upperbound time %.0f nodes " S64_FORMAT " pv %s",1+move_pos,depth,max_depth,value,time*1000.0,node_nb,pv_string);
                        }

                        if (UCI_debug && fp_debug != NULL) {
                           search_update_current(ThreadId);
                           fprintf(fp_debug,"search_report_and_update_best() pv's being sent, ThreadId = %d, time = %0.f value = %d\n\r", ThreadId, SearchCurrent[ThreadId]->time*1000.0, value);
                        }

                     } else {

                        // mate announcement

                        if (false) {
                        } else if (flags == SearchExact) {
                           send("info multipv %d depth %d seldepth %d score mate %d time %.0f nodes " S64_FORMAT " pv %s",           1+move_pos,depth,max_depth,mate,time*1000.0,node_nb,pv_string);
                        } else if (flags == SearchLower) {
                           ASSERT(false);
                           send("info multipv %d depth %d seldepth %d score mate %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",1+move_pos,depth,max_depth,mate,time*1000.0,node_nb,pv_string);
                        } else if (flags == SearchUpper) {
                           ASSERT(false);
                           send("info multipv %d depth %d seldepth %d score mate %d upperbound time %.0f nodes " S64_FORMAT " pv %s",1+move_pos,depth,max_depth,mate,time*1000.0,node_nb,pv_string);
                        }
                     }
                  }

                  last_displayed_depth = depth; // shared.

               } // if (depth >= DispBestMinDepth)

            } else if (new_best && depth >= last_displayed_depth) { ASSERT(0 != ThreadId);

               SendNewBest = true; // shared.
               NewBestDepth = depth; // shared.
            }
         }
//    }
// }
}

// search_update_root()

void search_update_root(int ThreadId) {

   int move, move_pos;
// int move_nb;
   double time;
// sint64 node_nb;
   char move_string[256];
   int depth;

   if (DispRoot && ThreadId == 0) { // TODO: buffer this up, then send from thread 0.

      search_update_current(ThreadId);

      move = SearchRoot[ThreadId]->move;
      move_pos = SearchRoot[ThreadId]->move_pos;

      if (SearchCurrent[ThreadId]->time >= 2.0                  ||
          SearchCurrent[ThreadId]->time >= 0.5 && move_pos == 0    ) {

         time = SearchCurrent[ThreadId]->time;

         move_to_string(move,move_string,256);

         depth = SearchCurrent[ThreadId]->act_iteration_depth;

         get_last_complete_depth(ThreadId);

         if (0 == ThreadId) {
            if (depth == 1 + last_complete_depth  ||   // shared.
                DispAllDebug && DEBUG                ) {

               send("info currmove %s currmovenumber %d",move_string,move_pos+1);
            }
         }
      }
   }
}

// search_update_current()

void search_update_current(int ThreadId) {

   my_timer_t *timer;
   sint64 node_nb;
   double time, cpu;

   timer = SearchCurrent[ThreadId]->timer;

   node_nb = SearchCurrent[ThreadId]->node_nb;
   time = (UseCpuTime) ? my_timer_elapsed_cpu(timer) : my_timer_elapsed_real(timer);
   cpu = my_timer_cpu_usage(timer);

   SearchCurrent[ThreadId]->time = time;
   SearchCurrent[ThreadId]->cpu = cpu;
}

// search_check()

void search_check(int ThreadId) {

   search_send_stat(ThreadId);

   if (UseEvent) event();

   if (SearchInput->time_is_limited
    && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_M // WHM; 1.5 * time_limit_1
    && !SearchRoot[ThreadId]->bad
    && SearchRoot[ThreadId]->depth == 1 + last_complete_depth
    && (current_best_is_mate || !last_best_was_mate)
    && (!UseExtension || SearchRoot[ThreadId]->move_pos == 0)) {

      SearchRoot[ThreadId]->flag = true;
   }

   if (SearchInput->time_is_limited
    && SearchCurrent[ThreadId]->time >= SearchInput->time_limit_2) {

      SearchRoot[ThreadId]->flag = true;
   }

   if (SearchRoot[ThreadId]->flag && !SearchInput->infinite  ||
       SearchInfo[ThreadId]->stop_command                       ) {
       SearchInfo[ThreadId]->stop = true; // SearchInfo[ThreadId]->stop shared.   // tells this thread to halt search.
   }


   // WHM  If an off-thread had a new best, send it from thread 0.
   if (0 == ThreadId  &&  SendNewBest) { // shared.
      ASSERT(NewBestDepth <= DepthMax); // shared.
      if (NewBestDepth >= last_complete_depth  && // shared.
          NewBestDepth >= last_displayed_depth    ) { // shared.
          search_send_SearchBestAtDepth(NewBestDepth); // shared.
          SendNewBest = false; // shared.
          last_displayed_depth = NewBestDepth; // shared.
          NewBestDepth = 0; // shared.
      }
   }
}

// search_send_stat()

static void search_send_stat(int ThreadId) {

   double time, speed, cpu;
   sint64 node_nb;
   int i;

   // LC max nps
   double minsecs;
   int maxnps, ms;

   search_update_current(ThreadId);

   if (DispStat && ThreadId == 0 && SearchCurrent[ThreadId]->time >= SearchInfo[ThreadId]->last_time + 1.0) { // at least one-second gap

      SearchInfo[ThreadId]->last_time = SearchCurrent[ThreadId]->time;

      time = SearchCurrent[ThreadId]->time;
      cpu = SearchCurrent[ThreadId]->cpu;
      node_nb = 0;
      for (i = 0; i < NumberThreadsInternal; i++){
        node_nb += SearchCurrent[ThreadId]->node_nb;
      }

      // LC max NPS
      maxnps = option_get_int("Max NPS");
      if (maxnps < 999999 ) {
         minsecs = node_nb *1.0 / maxnps;
         if (minsecs > time) {
            ms = 1000*(minsecs-time);
            if (time > 0.5) speed = node_nb / time;
            else            speed = 0;
            while (true) {
               // Send info each 1 sec then GUI knows it is alive
               send( "info adjusting nps from %.0f to %d then pending to sleep %d ms",speed, maxnps, ms);
               if(ms>1000) {
                  Sleep(1000);
                  ms -= 1000;
               }
               else {
                  Sleep(ms);
                  break;
               }
            }
            time = minsecs;
            SearchCurrent[0]->time += minsecs-time;
         }
      }

      if (time > 0.5) speed = node_nb / time;
      else            speed = 0;

      if (0 == ThreadId) {
         send("info time %.0f nodes " S64_FORMAT " nps %.0f cpuload %.0f",time*1000.0,node_nb,speed,cpu*1000.0);
         trans_stats(Trans);
      }
   }
}

//   search_send_SearchBestAtDepth()

void search_send_SearchBestAtDepth(int depth) {

   int move, value, flags, max_depth;
   mv_t pv[HeightMax];
   double time;
   sint64 node_nb;
   int mate, i;
   char move_string[256], pv_string[512];


   move = SearchBestAtDepth[depth].move;
   value = SearchBestAtDepth[depth].value;
   flags = SearchBestAtDepth[depth].flags; ASSERT(SearchExact == flags);
   pv_copy(pv, SearchBestAtDepth[depth].pv);
   ASSERT(pv_is_ok(pv));
   ASSERT(pv[0] == move);

   max_depth = SearchCurrent[0]->max_depth;
   time = SearchCurrent[0]->time;
   node_nb = SearchCurrent[0]->node_nb;
   for (i = 1; i < NumberThreadsInternal; i++) node_nb += SearchCurrent[i]->node_nb;
   for (i = 1; i < NumberThreadsInternal; i++) if (max_depth < SearchCurrent[i]->max_depth)
                                                   max_depth = SearchCurrent[i]->max_depth;
   move_to_string(move,move_string,256);
   pv_to_string(pv,pv_string,512);

   mate = value_to_mate(value);

   if (mate == 0) {

      // normal evaluation

      if (false) {
      } else if (flags == SearchExact) {
         send("info depth %d seldepth %d score cp %d time %.0f nodes " S64_FORMAT " pv %s",           depth,max_depth,value,time*1000.0,node_nb,pv_string);
      } else if (flags == SearchLower) {
         send("info depth %d seldepth %d score cp %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",depth,max_depth,value,time*1000.0,node_nb,pv_string);
      } else if (flags == SearchUpper) {
         send("info depth %d seldepth %d score cp %d upperbound time %.0f nodes " S64_FORMAT " pv %s",depth,max_depth,value,time*1000.0,node_nb,pv_string);
      }

   } else {

      // mate announcement

      if (false) {
      } else if (flags == SearchExact) {
         send("info depth %d seldepth %d score mate %d time %.0f nodes " S64_FORMAT " pv %s",           depth,max_depth,mate,time*1000.0,node_nb,pv_string);
      } else if (flags == SearchLower) {
         send("info depth %d seldepth %d score mate %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",depth,max_depth,mate,time*1000.0,node_nb,pv_string);
      } else if (flags == SearchUpper) {
         send("info depth %d seldepth %d score mate %d upperbound time %.0f nodes " S64_FORMAT " pv %s",depth,max_depth,mate,time*1000.0,node_nb,pv_string);
      }
   }

   last_displayed_depth = depth;
}

//          get_last_complete_depth()

static void get_last_complete_depth(int ThreadId) {

   int i;

   if (SkipMoves && ThreadsPerDepth >= 2) {
       last_complete_depth = DepthMax;
       for (i = 0; i < NumberThreadsInternal; i++) {
           if (last_complete_depth > last_depth_of_thread[i]) {
               last_complete_depth = last_depth_of_thread[i];
           }
       }
   } else {
       last_complete_depth = 0;
       for (i = 0; i < NumberThreadsInternal; i++) {
           if (last_complete_depth < last_depth_of_thread[i]) {
               last_complete_depth = last_depth_of_thread[i];
           }
       }
   }
}

static int Get_Number_Tactical_Threads_Started(int depth) {
   int sum = 0;
   for (int i = 0; i < NumberThreadsInternal; i++) sum += Number_Tactical_Threads_Started[i][depth];
   return sum;
}

static int Get_NumberPositionalThreads_Started(int depth) {
   int sum = 0;
   for (int i = 0; i < NumberThreadsInternal; i++) sum += NumberPositionalThreads_Started[i][depth];
   return sum;
}
// best_and_ponder_legality()

       int best_and_ponder_legality(int depth) {

   int move;
   mv_t pv[HeightMax];
   board_t board[1];
   undo_t undo[1];

   int bp_legal = 0;

   // best move

   move = SearchBestAtDepth[depth].move;
   pv_copy(pv, SearchBestAtDepth[depth].pv);

   board_copy(board, SearchInput->board);

   if (move == pv[0]) {
      if (move_is_ok(move)) {
         if (pseudo_is_legal (move, board)) {
            move_do(board,move,undo);
            if (board_is_legal(board)) {
               bp_legal++;
               if (move_is_ok(pv[1])) {
                  if (pseudo_is_legal(pv[1], board)) {
                     bp_legal++;
                  }
               }
               move_undo(board,move,undo); // for DEBUG
            }
         }
      }
   }


   return bp_legal;
}

void redo_search_report_and_update_best() { // TODO: time tag every entry into SearchReport[][], use later time in a tie-breaker...

   int move, value, flags;
   mv_t pv[HeightMax];

   int depth, ThreadId;

   for (depth = 1; depth < DepthMax; depth++) {
       for (ThreadId = 0; ThreadId < NumberThreadsInternal; ThreadId++) {

           value = SearchReport[ThreadId][depth]->value;

           if (value > ValueNone) {
               move = SearchReport[ThreadId][depth]->move;
               flags = SearchReport[ThreadId][depth]->flags; ASSERT(SearchExact == flags);
               pv_copy(pv, SearchReport[ThreadId][depth]->pv);
               ASSERT(pv_is_ok(pv));
               if (SearchBestAtDepth[depth].value < value) { // at the same value, the later guy used deeper trans data!
                   SearchBestAtDepth[depth].move  = move;   // TODO: do a later time tag tiebreaker for equal values...
                   SearchBestAtDepth[depth].value = value;
                   SearchBestAtDepth[depth].flags = flags;
                   pv_copy(SearchBestAtDepth[depth].pv, pv);
                   ASSERT(pv_is_ok(SearchBestAtDepth[depth].pv));
               }
           }
       }
   }

   get_last_complete_depth(0);
   ASSERT(last_complete_depth >= 0 && last_complete_depth < DepthMax);
}

// end of search.cpp

