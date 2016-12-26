
// option.cpp

// includes

#include <cstdlib>

#include "option.h"
#include "protocol.h"
#include "util.h"

// types

struct option_t {
   const char * var;
   bool declare;
   const char * init;
   const char * type;
   const char * extra;
   const char * val;
};

// variables

static option_t Option[] = {

   // First 3 not displayed.  "OwnBook" not displayed by Shredder 9 GUI but displayed by Fritz GUI.  20-21 top.
   { "Hash",              true, "32",              "spin",   "min 16 max 4096", NULL }, // 11/25/08 up to 64 for today's machines.
   { "MultiPV",           true, "1",               "spin",   "min 1 max 40",    NULL }, // 64 * number_CPU's or so...
   { "Ponder",            true, "false",           "check",  "",                NULL },
   { "Number of Threads", true, "1",               "spin",   "min 1 max 16",    NULL }, // 11/25/08
   { "Threads Per Depth", true, "1",               "spin",   "min 1 max 16",    NULL },
   { "Skip Moves",        true, "false",           "check",  "",                NULL },
// { "Threads Per Move",  true, "1",               "spin",   "min 1 max 4",     NULL }, // TODO:
   { "Root Alpha",        true, "true",            "check",  "",                NULL },
   { "OwnBook",           true, "false",           "check",  "",                NULL },
   { "BookFile",          true, "performance.bin", "string", "",                NULL },
   { "Max Book Moves",    true, "30",              "spin",   "min 0 max 40",    NULL }, // WHM:
// { "Analyze Book",      true, "true",            "check",  "",                NULL }, // WHM: TODO do a MultiPV look at book
                                                                                        //      note Fritz/Chessbase GUI does this with .ctg books.
   { "Use Bitbases",        true, "false",    "check", "",               NULL }, // causes crashes, started re-design for MP.
   { "Bitbases Path",       true, "c:/egbb/", "string", "",              NULL },
   { "Bitbases Cache Size", true, "16",       "spin",   "min 4 max 1024", NULL },

   { "Playing Style",            true, "Tactical", "combo", "var Tactical var Positional var Mixed", NULL }, // 11/25/08 to "Tactical".  "Positional" good for Rybka.
   { "Positional History Value", true, "70",       "spin",  "min 50 max 100",                        NULL }, // matching Toga1.4beta5cWHM(31)
   { "WSD",                      true, "false",    "check", "",                                      NULL }, // Win or Save Draw by WHM, good for Junior?
   { "Razor Depth",              true, "1",        "spin",  "min 0 max 3",                           NULL }, // 1.4beta5c was 1, Toga1.4beta5cWHM(31) was 1.
   { "Futility Depth",           true, "5",        "spin",  "min 0 max 5",                           NULL }, // 1.4beta5c was 3, 1.4.1SE was 5, 1.2.1a was 2, Toga1.4beta5cWHM(31) was 5.
   { "History Drop Depth",       true, "6",        "spin",  "min 0 max 6",                           NULL },
   { "Mate Threat Depth",        true, "4",        "spin",  "min 0 max 64",                          NULL },
   { "JD's Hangers Depth",       true, "0",        "spin",  "min 0 max 14",                          NULL },
   { "Tactical Depth",           true, "64",       "spin",  "min 0 max 64",                          NULL }, // fruit21 <-> beta5c transition depth.


   { "King Attack",              true, "100",   "spin", "min 0 max 200", NULL },
   { "King Safety",              true, "100",   "spin", "min 0 max 200", NULL },
   { "Pawn Structure",           true, "100",   "spin", "min 0 max 200", NULL },
   { "Passed Pawns",             true, "100",   "spin", "min 0 max 200", NULL },
   { "Piece Square Tables",      true, "100",   "spin", "min 0 max 200", NULL },
   { "Piece Activity",           true, "100",   "spin", "min 0 max 200", NULL },
   { "Piece Invasion",           true, "100",   "spin", "min 0 max 200", NULL },
   { "Material",                 true, "100",   "spin", "min 0 max 200", NULL }, // TODO: replace this with q,r,b,n,p opening & endgame, 10 UCI guys.
   { "Material Imbalance",       true, "0",     "spin", "min 0 max 200", NULL }, // Thomas
   { "Toga Exchange Bonus",      true, "20",    "spin", "min 0 max 50",  NULL }, // matching Toga1.4beta5cWHM(31)
   { "Bishop Pair Opening",      true, "50",    "spin", "min 0 max 100", NULL },
   { "Bishop Pair Endgame",      true, "70",    "spin", "min 0 max 100", NULL }, // matching Toga1.4.1SE, Chris Formula's settings.
   { "Rook Pair Endgame",        true, "0",     "spin", "min 0 max 50",  NULL }, // matching Toga1.4beta5cWHM(31), let the learning figure this out.
   { "Pawn Holes",               true, "false", "check", "",             NULL }, // Good for Rybka.

   //-----------------------------------------------------------------------------

   // 21 items bottom.

   { "Debug", true, "false", "check", "", NULL }, // sets UCI_debug true, UCI_debug is extern in util.h, writes to "toga.log"

   // JAS
   // search X seconds for the best move, equal to "go movetime"
   { "Search Time",  true, "0",   "spin",  "min 0 max 3600000", NULL },
   // search X plies deep, equal to "go depth"
   { "Search Depth",  true, "0",   "spin",  "min 0 max 30", NULL },
   // JAS end

   { "MovesToGo Max",          true, "32", "spin", "min 15 max 120", NULL },//LC change 30-60 -> 15-120
   { "Quiescence Check Plies", true, "1",  "spin", "min 0 max 2",   NULL },

   { "Delta Pruning", true, "true", "check", "", NULL },
   { "Delta Margin",  true, "50",   "spin",  "min 0 max 500", NULL },

   { "Futility Margin 1", true, "100",  "spin",  "min 0 max 400", NULL }, // 1.4.1SE was 100,200,300,200,300, CFS==Chris Formula Setting
   { "Futility Margin 2", true, "300",  "spin",  "min 0 max 600", NULL }, // 1.4beta5c was 100,300,350, Futility Depth 3.
   { "Futility Margin 3", true, "300",  "spin",  "min 0 max 700", NULL },
   { "Futility Margin 4", true, "300",  "spin",  "min 0 max 800", NULL },
   { "Futility Margin 5", true, "300",  "spin",  "min 0 max 900", NULL },

   { "Max NPS",                  true, "999999", "spin", "min 1 max 999999",  NULL }, //LC change

   { NULL, false, NULL, NULL, NULL, NULL, },
};

// prototypes

static option_t * option_find (const char var[]);

// functions

// option_init()

void option_init() {

   option_t * opt;

   for (opt = &Option[0]; opt->var != NULL; opt++) {
      option_set(opt->var,opt->init);
   }
}

// option_list()

void option_list() {

   option_t * opt;

   for (opt = &Option[0]; opt->var != NULL; opt++) {
      if (opt->declare) {
         if (opt->extra != NULL && *opt->extra != '\0') {
            send("option name %s type %s default %s %s",opt->var,opt->type,opt->val,opt->extra);
         } else {
            send("option name %s type %s default %s",opt->var,opt->type,opt->val);
         }
      }
   }
}

// option_set()

bool option_set(const char var[], const char val[]) {

   option_t * opt;

   ASSERT(var!=NULL);
   ASSERT(val!=NULL);

   opt = option_find(var);
   if (opt == NULL) return false;

   my_string_set(&opt->val,val);

   return true;
}

// option_get()

const char * option_get(const char var[]) {

   option_t * opt;

   ASSERT(var!=NULL);

   opt = option_find(var);
   if (opt == NULL) my_fatal("option_get(): unknown option \"%s\"\n",var);

   return opt->val;
}

// option_get_bool()

bool option_get_bool(const char var[]) {

   const char * val;

   val = option_get(var);

   if (false) {
   } else if (my_string_equal(val,"true") || my_string_equal(val,"yes") || my_string_equal(val,"1")) {
      return true;
   } else if (my_string_equal(val,"false") || my_string_equal(val,"no") || my_string_equal(val,"0")) {
      return false;
   }

   ASSERT(false);

   return false;
}

// option_get_int()

int option_get_int(const char var[]) {

   const char * val;

   val = option_get(var);

   return atoi(val);
}

// option_get_string()

const char * option_get_string(const char var[]) {

   const char * val;

   val = option_get(var);

   return val;
}

// option_find()

static option_t * option_find(const char var[]) {

   option_t * opt;

   ASSERT(var!=NULL);

   for (opt = &Option[0]; opt->var != NULL; opt++) {
      if (my_string_equal(opt->var,var)) return opt;
   }

   return NULL;
}

// end of option.cpp
