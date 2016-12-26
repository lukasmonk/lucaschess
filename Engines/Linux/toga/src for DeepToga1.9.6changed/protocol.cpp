
// protocol.cpp

// includes

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>

#include "board.h"
#include "book.h"
#include "eval.h"
#include "fen.h"

#include "material.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "option.h"
#include "pawn.h"
#include "posix.h"
#include "protocol.h"
#include "pst.h"
#include "pv.h"
#include "search.h"
#include "sort.h"
#include "trans.h"
#include "util.h"
//#include "sort.h"
#include "probe.h"
#include "value.h"

// constants

#define VERSION "1.9.6nps"

static const double NormalRatio = 1.0;
static const double PonderRatio = 1.25;

// fen for opening lines
static const char D34_QueensGambitTarraschVariation[256] = "r2qk2r/pp2bppp/2n1bn2/2pp4/3P4/1PN2NP1/P3PPBP/R1BQ1RK1 b kq -"; // 17 ply
static const char E81_KingsIndianSamischVariation[256] = "1rbq1rk1/1pp1ppbp/p1np1np1/6B1/2PPP2P/2N2P2/PP1QN1P1/R3KB1R b KQ -"; // 17 ply
static const char B82_SicilianScheveningenVariation[256] = "rnb1kb1r/2q2ppp/p2ppn2/1p6/4PP2/1NNB1Q2/PPP3PP/R1B2RK1 b kq -"; // 19 ply
static const char D48_QueensGambitSemiSlavMeranVariation[256] = "r2qk2r/1b1nbppp/p1p1pn2/8/Pp1PN3/3BPN2/1P3PPP/R1BQ1RK1 w kq -"; // 22 ply
static const char B81_SicilianKeresAttack[256] = "2rqkb1r/1b1n1pp1/p2ppn1p/1p6/3NP1P1/P1N1B2P/1PP1QPB1/R4RK1 b k -"; // 23 ply
static const char D87_GrunfeildIndianClassicalExchangeVariation[256] = "r1br2k1/pp2ppbp/2n3p1/q7/2BpP3/2P1B3/P2QNPPP/2RR2K1 w - -"; // 26ply
static const char B87_SicilianSozinAttack[256] = "r3k1r1/1b2bp1p/p2ppn2/1pq3p1/3NP3/1B3PQ1/PPP3PP/R1B1R1K1 w q -"; // 30 ply
static const char C96_RuyLopezChigorinVariation[256] = "r1bq1rk1/4bppp/p2p4/1p2n3/3QP3/7P/PPB2PP1/R1BR1NK1 b - -"; // 33 ply
// fen for short book main variations only
static const char C96_RuyLopez[256] = "r1bqk2r/2ppbppp/p1n2n2/1p2p3/4P3/1B3N2/PPPP1PPP/RNBQR1K1 b kq -"; // 13 ply
static const char B40_Sicilian[256] = "rnbqkb1r/pp1p1ppp/4pn2/8/3NP3/2N5/PPP2PPP/R1BQKB1R b KQkq -"; // 9 ply
static const char A07_KingsIndianAttack[256] = "rnbqk1nr/pp2ppbp/2p3p1/3p4/2PP4/1Q3NP1/PP2PP1P/RNB1KB1R b KQkq -"; // 9 ply

static const bool UseBetterDeeper =  true;

// variables

static /* UCI */ int MovesToGoMax = 31;

static bool Init;

static bool Searching; // search in progress?
static bool Infinite; // infinite or ponder mode?
static bool Delay; // postpone "bestmove" in infinite/ponder mode?

// prototypes

static void init              ();
static void loop_step         ();

static void parse_go          (char string[]);
static void parse_position    (char string[]);
static void parse_setoption   (char string[]);

static void send_best_move    ();

static bool string_equal      (const char s1[], const char s2[]);
static bool string_start_with (const char s1[], const char s2[]);

// functions

void book_parameter() {

    // UCI options

    book_close();
    if (option_get_bool("OwnBook")) {
         book_open(option_get_string("BookFile"));
    }

   // opening book  WHM added the ability to limit number of book moves for tourney/testing compliance.
   SearchInput->max_book_moves = option_get_int("Max Book Moves");        // WHM
   SearchInput->max_book_plies = 2 * SearchInput->max_book_moves  -  1;   // WHM: moves -> ply, C++ style.
}

// loop()

void loop() {

   // init (to help debugging)

   Init = false;

   Searching = false;
   Infinite = false;
   Delay = false;

   // set up the search and board.

   clear_SearchInput();
   search_clear();

   board_from_fen(SearchInput->board,StartFen);

   // WHM here but not in search_clear()
   //     as search_clear() is called again after the "fen" input!
   SearchInput->input_is_startpos = true;

   // WHM; number of plies into the game counter.
   SearchInput->plies_from_startpos = 0;



   // loop

   while (true) loop_step();
}

// init()

static void init() {

   if (!Init) {

      // late initialisation

      Init = true;

      book_parameter();

      trans_alloc(Trans);

      pawn_init();
      pawn_alloc();

      material_init();
      material_alloc();

      pst_init();
      eval_init();

      SearchInput->first_search_not_done = true; // WHM

      SearchInput->exit_engine = false;
      start_suspended_threads();
   }
}

// event()

void event() {

   while (!SearchInfo[0]->stop_command && input_available()) loop_step();
}

// loop_step()

static void loop_step() {

   char string[65536];
   int ThreadId;

   // read a line

   get(string,65536);

   // parse

   if (false) {

   } else if (string_start_with(string,"debug ") ||
              string_start_with(string,"Debug ") ||
              string_start_with(string,"DEBUG ")    ) {

      UCI_debug = string_start_with(string,"debug on") ||
                  string_start_with(string,"debug ON") ||
                  string_start_with(string,"Debug on") ||
                  string_start_with(string,"Debug On") ||
                  string_start_with(string,"Debug ON") ||
                  string_start_with(string,"DEBUG ON")    ;

      if (!UCI_debug && option_get_bool("Debug")) {
           UCI_debug = true;
      }

      if (UCI_debug && fp_debug == NULL) {
         fp_debug=fopen("toga.log","a+");
         fprintf(fp_debug,"%s\n\r",string);
      }

   } else if (string_start_with(string,"go ")) {

      if (!Searching && !Delay) {
         init();
         parse_go(string);
      } else {
         ASSERT(false);
      }

   } else if (string_equal(string,"isready")) {

      if (!Searching && !Delay) {
         init();
      }

      last_best_was_mate = false;

      send("readyok"); // no need to wait when searching (dixit SMK)

   } else if (string_equal(string,"ponderhit")) {

      if (Searching) {

         ASSERT(Infinite);

         SearchInput->infinite = false;
         Infinite = false;

      } else if (Delay) {

         send_best_move();
        if (UCI_debug && fp_debug != NULL) {
           fprintf(fp_debug,"loop_step() after send_best_move() in 'ponderhit' case\n\r");
        }
         Delay = false;

      } else {

         ASSERT(false);
      }

   } else if (string_start_with(string,"position ")) {

      if (!Searching && !Delay) {
         init();
         parse_position(string);
      } else {
         ASSERT(false);
      }

   } else if (string_equal(string,"quit")) {

      ASSERT(!Searching);
      ASSERT(!Delay);

      if (UCI_debug && fp_debug != NULL) {
         fprintf(fp_debug,"%s\n\r",string);
         fprintf(fp_debug,"EXIT_SUCCESS\n\r");
         fclose(fp_debug);
      }

      SearchInput->exit_engine = true;
      resume_threads();
      exit(EXIT_SUCCESS);

   } else if (string_start_with(string,"setoption ")) {

      if (!Searching && !Delay) {

         parse_setoption(string);

         SearchInput->first_search_not_done = true; // WHM
         if (option_get_bool("Debug")) {
            UCI_debug = true;
            if (UCI_debug && fp_debug == NULL) {
               fp_debug=fopen("toga.log","a+");
            }
         }
         if (UCI_debug && fp_debug > NULL) {
            fprintf(fp_debug,"%s\n\r",string);
         }
         SearchInput->multipv = option_get_int("MultiPV") - 1;
         last_best_was_mate = false;

      } else {

         ASSERT(false);
      }

   } else if (string_equal(string,"stop")) {

      if (Searching) {

         for (ThreadId = 0; ThreadId < NumberThreadsInternal; ThreadId++){
             SearchInfo[ThreadId]->stop_command = true;
         }

         Infinite = false;

      } else if (Delay) {

         send_best_move();
        if (UCI_debug && fp_debug != NULL) {
           fprintf(fp_debug,"loop_step() after send_best_move() in 'stop' case\n\r");
        }
         Delay = false;
      }

      // WCCC rules: need a converging mate once mate is declared!
      last_best_was_mate = false;

   } else if (string_equal(string,"uci")) {

      ASSERT(!Searching);
      ASSERT(!Delay);

      send("id name DeepToga" VERSION);
      send("id author WHMoweryJr, Thomas Gaksch and Fabien Letouzey");

      option_list();

      send("uciok");

      // WCCC rules: need a converging mate once mate is declared!
      last_best_was_mate = false;

   } else if (string_equal(string,"ucinewgame")) {

      if (!Searching && !Delay && Init) {
         trans_clear(Trans);
         sort_init(); // WHM
         SearchInput->first_search_not_done = true; // WHM
         // WCCC rules: need a converging mate once mate is declared!
         last_best_was_mate = false;


      } else {

         ASSERT(false);
      }
   }
}

// parse_go()

static void parse_go(char string[]) {

   const char * ptr;
   bool infinite, ponder;
   int depth, mate, movestogo;
   sint64 nodes;
   double binc, btime, movetime, winc, wtime;
   double time, inc;
   double time_max, alloc;
   double proj_time; // WHM; more explanatory than anything else.

   // init

   infinite = false;
   ponder = false;

   depth = -1;
   mate = -1;
   movestogo = -1;

   nodes = -1;

   binc = -1.0;
   btime = -1.0;
   movetime = -1.0;
   winc = -1.0;
   wtime = -1.0;

   // parse

   ptr = strtok(string," "); // skip "go"

   for (ptr = strtok(NULL," "); ptr != NULL; ptr = strtok(NULL," ")) {

      if (false) {

      } else if (string_equal(ptr,"binc")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         binc = double(atoi(ptr)) / 1000.0;
         ASSERT(binc>=0.0);

      } else if (string_equal(ptr,"btime")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         btime = double(atoi(ptr)) / 1000.0;
         ASSERT(btime>=0.0);

      } else if (string_equal(ptr,"depth")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         depth = atoi(ptr);
         ASSERT(depth>=0);

      } else if (string_equal(ptr,"infinite")) {

         infinite = true;

      } else if (string_equal(ptr,"mate")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         mate = atoi(ptr);
         ASSERT(mate>=0);

      } else if (string_equal(ptr,"movestogo")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         movestogo = atoi(ptr);
         ASSERT(movestogo>=0);

      } else if (string_equal(ptr,"movetime")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         movetime = double(atoi(ptr)) / 1000.0;
         ASSERT(movetime>=0.0);

      } else if (string_equal(ptr,"nodes")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         nodes = my_atoll(ptr);
         ASSERT(nodes>=0);

      } else if (string_equal(ptr,"ponder")) {

         ponder = true;

      } else if (string_equal(ptr,"searchmoves")) {

         // dummy

      } else if (string_equal(ptr,"winc")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         winc = double(atoi(ptr)) / 1000.0;
         ASSERT(winc>=0.0);

      } else if (string_equal(ptr,"wtime")) {

         ptr = strtok(NULL," ");
         if (ptr == NULL) my_fatal("parse_go(): missing argument\n");

         wtime = double(atoi(ptr)) / 1000.0;
         ASSERT(wtime>=0.0);
      }
   }

   // init

   // SearchInput

   clear_SearchInput();
   search_clear();

   // depth limit

   // JAS
   int option_depth = 0;
   option_depth = option_get_int("Search Depth");
   if (option_depth > 0) {
         depth = option_depth;
   }
   // JAS end

   if (depth >= 0) {
      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = depth;
   } else if (mate >= 0) {
      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = mate * 2 - 1; // HACK: move -> ply
   }

   // time limit

   if (COLOUR_IS_WHITE(SearchInput->board->turn)) {
      time = wtime;
      inc = winc;
   } else {
      time = btime;
      inc = binc;
   }

   MovesToGoMax = option_get_int("MovesToGo Max");

// if (movestogo <= 0 || movestogo > 30) movestogo = 30; // HACK was 30
   if (movestogo <= 0 || movestogo > MovesToGoMax) movestogo = MovesToGoMax; // WHM; for time_limit_M, UCI

   if (inc < 0.0) inc = 0.0;

   // JAS
   int option_movetime = 0;
   option_movetime = option_get_int("Search Time");
   if (option_movetime > 0) {
         movetime = option_movetime;
   }
   // JAS end

   if (movetime >= 0.0) {

      // fixed time

      SearchInput->time_is_limited = true;

//    SearchInput->time_limit_1 = movetime * 5.00; // HACK to avoid early exit
//    SearchInput->time_limit_M = movetime * 5.00; // WHM; to avoid early exit
//    SearchInput->time_limit_2 = movetime - 0.05; // WHM; UCI communication delay

      SearchInput->time_limit_1 = movetime / 0.75; // WHM; another possibility for
      SearchInput->time_limit_M = movetime / 0.50; //      an average of
      SearchInput->time_limit_2 = movetime / 0.15; //      about movetime.

   } else if (time >= 0.0) {

      // dynamic allocation

//    time_max = time * 0.95 - 1.0;
      time_max = time * 0.95 -  5.0; // depth 2 search plus 50 move rule a few times.
      if (time_max < 0.0) time_max = time / 5;
      if (time_max < 0.0) time_max = 0.0;

      SearchInput->time_is_limited = true;

      proj_time = time_max  +  inc * double(movestogo-1);

      alloc = proj_time / double(movestogo);
      alloc *= (ponder || option_get_bool("Ponder") ? PonderRatio : NormalRatio); // Fritz GUI does "go ponder moves e2e4 e7e5..."

      // first_search bool in SearchInput used to fill hash/material/pawn tables with a new game...
      if (SearchInput->first_search_not_done) alloc *= PonderRatio;
      SearchInput->first_search_not_done = false;

      SearchInput->time_limit_1 = alloc;
      SearchInput->time_limit_M = alloc * 1.5; // WHM; time at depths before d-1,
                                              //       time at depth d-1,
                                             //        time at first move depth d.

//WHM alloc = (time_max + inc * double(movestogo-1)) * 0.5;

      if (movestogo >= 7) alloc = proj_time / 6.6; // WHM: time is precious, cuts those big thinks to 15% from 50%
      else                alloc = proj_time / double(movestogo);
      if (alloc < SearchInput->time_limit_1) {
          alloc = SearchInput->time_limit_1;
      }
      SearchInput->time_limit_2 = alloc;

      // SMOOTH TRANSITION TO TIME PRESSURE IN BIG BOLD CAPITOL LETTERS FOR A VERY GOOD REASON.
      if (movestogo == MovesToGoMax) {
         if (SearchInput->time_limit_1 > time_max / 24  ) SearchInput->time_limit_1 = time_max / 24; // WHM
         if (SearchInput->time_limit_M > time_max / 16  ) SearchInput->time_limit_M = time_max / 16; // WHM
         if (SearchInput->time_limit_2 > time_max /  8  ) SearchInput->time_limit_2 = time_max /  8; // WHM
      } else {
         if (SearchInput->time_limit_1 > time_max / 4.5) SearchInput->time_limit_1 = time_max / 4.5; // WHM
         if (SearchInput->time_limit_M > time_max / 3  ) SearchInput->time_limit_M = time_max / 3  ; // WHM
         if (SearchInput->time_limit_2 > time_max      ) SearchInput->time_limit_2 = time_max      ; // WHM
      }
      // SMOOTH TRANSITION TO TIME PRESSURE IN BIG BOLD CAPITOL LETTERS FOR A VERY GOOD REASON.
   }

   if (infinite || ponder) SearchInput->infinite = true;

   // search

   ASSERT(!Searching);
   ASSERT(!Delay);

   Searching = true;
   Infinite = infinite || ponder;
   Delay = false;

   search();

   search_update_current(0); // WHM: the other threads are suspended now/here...

   ASSERT(Searching);
   ASSERT(!Delay);

   Searching = false;
   Delay = Infinite;

   // WHM begin SearchInput->best_values_profile[]
   ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);
   if (SearchInput->plies_from_startpos > 0) { // it has already been incremented...
       SearchInput->best_values_profile[SearchInput->plies_from_startpos - 1] = SearchBestAtDepth[last_complete_depth].value;
   } else {
       SearchInput->best_values_profile[                                 0  ] = SearchBestAtDepth[last_complete_depth].value;
   }

   // this needs to be last  WHM end
   if (!Delay) send_best_move();

        if (UCI_debug && fp_debug != NULL) {
           search_update_current(0);
           fprintf(fp_debug,"parse_go(), after send_best_move(), time = %0.f\n\r", SearchCurrent[0]->time*1000.0);
        }
}

// parse_position()

static void parse_position(char string[]) {

   const char * fen;
   char * moves;
   const char * ptr;
   char move_string[256];
   int move;
   undo_t undo[1];

   // init

   fen = strstr(string,"fen ");
   moves = strstr(string,"moves ");

   // start position

   if (fen != NULL) { // "fen" present

      if (moves != NULL) { // "moves" present
         ASSERT(moves>fen);
         moves[-1] = '\0'; // dirty, but so is UCI
      }

//    board_from_fen(SearchInput->board,fen+4); // CHANGE ME, OK.

      fen += 4; //                                                              WHM
      board_from_fen(SearchInput->board,fen); // changed.                       WHM

      int i = 0; //                                                             WHM
      while(*fen != '\0'  &&  i < 256) SearchInput->input_fen[i++] = *fen++; // WHM
      SearchInput->input_fen[i] = '\0';ASSERT(*fen == '\0');ASSERT(i < 256); // WHM
      SearchInput->input_is_startpos = false; //                                WHM

   } else {

      // HACK: assumes startpos

      board_from_fen(SearchInput->board,StartFen);

      SearchInput->input_is_startpos = true; // WHM
   }

   // moves

   SearchInput->plies_from_startpos = 0; // WHM; number of plies into the game counter.

   if (moves != NULL) { // "moves" present

      ptr = moves + 6;

      while (*ptr != '\0') {

         move_string[0] = *ptr++;
         move_string[1] = *ptr++;
         move_string[2] = *ptr++;
         move_string[3] = *ptr++;

         if (*ptr == '\0' || *ptr == ' ') {
            move_string[4] = '\0';
         } else { // promote
            move_string[4] = *ptr++;
            move_string[5] = '\0';
         }

         move = move_from_string(move_string,SearchInput->board);

         move_do(SearchInput->board,move,undo);

         SearchInput->moves_array[SearchInput->plies_from_startpos] = move;    // WHM store the input move
         SearchInput->plies_from_startpos++;              // WHM; increment number of plies counter.

         while (*ptr == ' ') ptr++;
      }
   }
}

// parse_setoption()

static void parse_setoption(char string[]) {

   const char * name;
   char * value;

   // init

   name = strstr(string,"name ");
   value = strstr(string,"value ");

   if (name == NULL || value == NULL || name >= value) return; // ignore buttons

   value[-1] = '\0'; // HACK
   name += 5;
   value += 6;

   // update

   option_set(name,value);

    // update bitbases if needed, variables are 'extern's in module probe.

    if (my_string_equal(name,"Bitbases Path")       ||
        my_string_equal(name,"Bitbases Cache Size")    ){

        egbb_cache_size = (option_get_int("Bitbases Cache Size") * 1024 * 1024);
        egbb_cache_size /= NumberThreadsInternal; // WHM_egbb
        egbb_path = (char *) option_get_string("Bitbases Path");
        egbb_is_loaded = false;
        if (option_get_bool("Use Bitbases")) {
           if (1 == LoadEgbbLibrary(egbb_path)) egbb_is_loaded = true;
        }
        if (egbb_is_loaded) printf("EgbbProbe IS Loaded!\n");
        else                printf("EgbbProbe NOT Loaded!\n");
    }

   // update transposition-table size if needed

   if (Init && my_string_equal(name,"Hash")) { // Init => already allocated

      ASSERT(!Searching);

      if (option_get_int("Hash") >= 16) {
         trans_free(Trans);
         trans_alloc(Trans);
      }
   }

   // WHM added book change options here...
   if (my_string_equal(name,"OwnBook")       ||
       my_string_equal(name,"BookFile")      ||
       my_string_equal(name,"Max Book Moves")  ) {

      book_parameter();
   }

         pawn_parameter();
         material_parameter();
//       book_parameter();
         pst_init();
         eval_parameter();
         sort_init(); // WHM

   if (my_string_equal(name,          "Number of Threads")) {
       NumberThreads = option_get_int("Number of Threads");
       NumberThreadsInternal = MIN(NumberThreads, NumberProcessors);
       sort_init();
       pawn_init();
       pawn_alloc();
       material_init();
       material_alloc();
       start_suspended_threads();
       // this is after option_init() so lets use options for conformity.
       // all probe variables here are 'extern's in module probe.
       egbb_cache_size = (option_get_int("Bitbases Cache Size") * 1024 * 1024);
       egbb_cache_size /= NumberThreadsInternal; // WHM_egbb
       egbb_path = (char *) option_get_string("Bitbases Path");
       egbb_is_loaded = false;
       if (option_get_bool("Use Bitbases")) {
          if (1 == LoadEgbbLibrary(egbb_path)) egbb_is_loaded = true;
       }
       if (egbb_is_loaded) printf("EgbbProbe IS Loaded!\n");
       else                printf("EgbbProbe NOT Loaded!\n");
   }

   // WCCC rules: need a converging mate once mate is declared!
   last_best_was_mate = false;
}

// send_best_move()

static void send_best_move() {

   double time, speed, cpu;
   sint64 node_nb;
   char move_string[256];
   char ponder_string[256];
   int move;
// mv_t * pv;
   mv_t   pv[HeightMax];
   int ThreadId;

   int idepth;
   int best_depth;
   int legality;
   int best_value;
   int best_move;

   // LC max nps
   double minsecs;
   int maxnps, ms;

   bool already_claimed;
   int j;

   // info

   // HACK: should be in search.cpp

   Sleep( 1);


   time = SearchCurrent[0]->time;
   speed = 0;
   cpu = SearchCurrent[0]->cpu;

   node_nb = 0;
   for (ThreadId = 0; ThreadId < NumberThreadsInternal; ThreadId++){
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

   send("info time %.0f nodes " S64_FORMAT " nps %.0f cpuload %.0f",time*1000.0,node_nb,speed,cpu*1000.0);

   trans_stats(Trans);
   // pawn_stats();
   // material_stats();

   // best move

   ASSERT(last_complete_depth >= 1 && last_complete_depth < DepthMax);
   move = SearchBestAtDepth[last_complete_depth].move;
// pv = SearchBestAtDepth[last_complete_depth]->pv;
   pv_copy(pv, SearchBestAtDepth[last_complete_depth].pv);
   ASSERT(pv[0] == move);

   legality = best_and_ponder_legality(last_complete_depth);
   best_depth = last_complete_depth;
   best_value = SearchBestAtDepth[last_complete_depth].value;
   best_move  = SearchBestAtDepth[last_complete_depth].move;

   // deeper is better, provided value >= prev value...  TODO: Mixed mode.
   if (UseBetterDeeper  &&  NumberThreadsInternal >= 2) {

       for (idepth = 1 + last_complete_depth; idepth <= MIN(DepthMax-1, 16 + last_complete_depth); idepth++) {

           for (ThreadId = 0; ThreadId < NumberThreadsInternal; ThreadId++) {
              if (best_move == SearchReport[ThreadId][idepth]->move) {
                  best_value = SearchReport[ThreadId][idepth]->value; // use this depth's value if this move was completed at this depth.
                  best_depth = idepth;
              }
           }

           // if this depth processed best_move, use this depth's result...
           already_claimed = false;
           if (best_depth < idepth) {
               for (j = 0; j < claimed_nb_array[idepth] && !already_claimed; j++) {
                   if (claimed_moves_array[idepth][j] == best_move) {
                       already_claimed = true;
                   }
               }
           }

           if (already_claimed || best_value <= SearchBestAtDepth[idepth].value) {
               if (best_and_ponder_legality(idepth) >  legality) {
                   legality = best_and_ponder_legality(idepth);
                   best_depth = idepth;
                   best_value = SearchBestAtDepth[idepth].value;
                   best_move = SearchBestAtDepth[idepth].move;
			   } else if (best_and_ponder_legality(idepth) >= legality) {
                   best_depth = idepth;
                   best_value = SearchBestAtDepth[idepth].value;
                   best_move = SearchBestAtDepth[idepth].move;
               } else {
                   if (UCI_debug && fp_debug != NULL) {
                       fprintf(fp_debug," \n\r");
                       fprintf(fp_debug,"UseBetterDeeper, best_and_ponder_legality(idepth) = %d \n\r", best_and_ponder_legality(idepth));
                       fprintf(fp_debug,"UseBetterDeeper, idepth == %d\n\r", idepth);
                       fprintf(fp_debug,"UseBetterDeeper, SearchBestAtDepth[idepth]->move  == %d\n\r", SearchBestAtDepth[idepth].move );
                       fprintf(fp_debug,"UseBetterDeeper, SearchBestAtDepth[idepth]->pv[0] == %d\n\r", SearchBestAtDepth[idepth].pv[0]);
                       fprintf(fp_debug,"UseBetterDeeper, SearchBestAtDepth[idepth]->pv[1] == %d\n\r", SearchBestAtDepth[idepth].pv[1]);
                   }
                   ASSERT(false);
               }
           }
       }
   }

   search_send_SearchBestAtDepth(best_depth); // GUI's get confused

   // WCCC rules: need a converging mate once mate declared!
   if (SearchBestAtDepth[best_depth].value > +ValueEvalInf) { // Only to avoid snatching LOSS from WIN!!!!!!!
      last_best_was_mate = true;
   } else {
           if (UCI_debug && fp_debug != NULL && last_best_was_mate) {
              fprintf(fp_debug,"after search_send_SearchBestAtDepth(best_depth), last_best_was_mate going from true to false...\n\r");
           }
      last_best_was_mate = false;
   }

   search_update_current(0);

   // get best move and ponder move
   move = SearchBestAtDepth[best_depth].move;
// pv = SearchBestAtDepth[best_depth]->pv;
   pv_copy(pv, SearchBestAtDepth[best_depth].pv);

   move_to_string(move,move_string,256);

   if (pv[0] == move && move_is_ok(pv[1]) && best_and_ponder_legality(best_depth) == 2) {
      move_to_string(pv[1],ponder_string,256);
      send("bestmove %s ponder %s",move_string,ponder_string);
   } else {
      send("bestmove %s",move_string);
        if (UCI_debug && fp_debug != NULL) {
           fprintf(fp_debug,"send_best_move() only best sent, bestmove %s\n\r",move_string);
           fprintf(fp_debug,"best_depth = %d, time = %0.f\n\r", best_depth, SearchCurrent[0]->time*1000.0);
        }
   }

   if (0 == best_and_ponder_legality(best_depth)) {
           if (UCI_debug && fp_debug != NULL) {
              search_update_current(0);
              fprintf(fp_debug,"send_best_move(), ILLEGAL bestmove %s time = %0.f\n\r",move_string,SearchCurrent[0]->time*1000.0);
           }
   }

   // Marking that we did a search
   SearchInput->first_search_not_done = false;
}

// get()

void get(char string[], int size) {

   ASSERT(string!=NULL);
   ASSERT(size>=65536);

   if (!my_file_read_line(stdin,string,size)) { // EOF
      exit(EXIT_SUCCESS);
   }
}

// send()

void send(const char format[], ...) {

   va_list arg_list;
   char string[4096];

   ASSERT(format!=NULL);

   va_start(arg_list,format);
   vsprintf(string,format,arg_list);
   va_end(arg_list);

   fprintf(stdout,"%s\n",string);
}

// string_equal()

static bool string_equal(const char s1[], const char s2[]) {

   ASSERT(s1!=NULL);
   ASSERT(s2!=NULL);

   return strcmp(s1,s2) == 0;
}

// string_start_with()

static bool string_start_with(const char s1[], const char s2[]) {

   ASSERT(s1!=NULL);
   ASSERT(s2!=NULL);

   return strstr(s1,s2) == s1;
}

// end of protocol.cpp

