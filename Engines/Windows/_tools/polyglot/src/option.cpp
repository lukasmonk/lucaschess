
// option.cpp

// includes

#include <cstdlib>
#include <cstring>

#include "option.h"
#include "util.h"

// constants

static const bool UseDebug = false;

// types

struct option_t {
   const char * var;
   const char * val;
};

// variables

static option_t Option[] = {

   { "OptionFile",    NULL, }, // string

   // options

   { "EngineName",    NULL, }, // string
   { "EngineDir",     NULL, }, // string
   { "EngineCommand", NULL, }, // string

   { "Log",           NULL, }, // true/false
   { "LogFile",       NULL, }, // string

 //  { "UCI",           NULL, }, // true/false

   { "UseNice",       NULL, }, // true/false

   { "NiceValue",     NULL, }, // true/false

   { "Chess960",      NULL, }, // true/false

   { "Resign",        NULL, }, // true/false
   { "ResignMoves",   NULL, }, // move number
   { "ResignScore",   NULL, }, // centipawns

   { "MateScore",     NULL, }, // centipawns

   { "Book",          NULL, }, // true/false
   { "BookFile",      NULL, }, // string

   { "BookRandom",    NULL, }, // true/false
   { "BookLearn",     NULL, }, // true/false

   { "KibitzMove",    NULL, }, // true/false
   { "KibitzPV",      NULL, }, // true/false

   { "KibitzCommand", NULL, }, // string
   { "KibitzDelay",   NULL, }, // seconds

   { "ShowPonder",    NULL, }, // true/false
   { "ScoreWhite",    NULL, }, // true/false
   { "NoGlobals",     NULL, }, // true/false
   { "InfoStrings",   NULL, }, // true/false
   { "MultiPVall",    NULL, }, // true/false

   // work-arounds

   { "UCIVersion",    NULL, }, // 1-
   { "CanPonder",     NULL, }, // true/false
   { "SyncStop",      NULL, }, // true/false
   { "Affinity",	  NULL, }, // -1 else 0-32 //won't do much on *nix systems
   { "RepeatPV",	  NULL, },
   { "PromoteWorkAround", NULL, }, // true/false
   { "PostDelay", NULL, } , // seconds

   // { "",              NULL, },

   { NULL,            NULL, },
};

// prototypes

static option_t * option_find (const char var[]);

// functions

// option_init()

void option_init() {

   option_set("OptionFile","polyglot.ini");

   // options

   option_set("EngineName","<empty>");
   option_set("EngineDir",".");
   option_set("EngineCommand","<empty>");

   option_set("Log","false");
   option_set("LogFile","polyglot.log");

 //  option_set("UCI","false");

   option_set("UseNice","false");
   option_set("NiceValue","5");
   
   option_set("Chess960","false");

   option_set("Resign","false");
   option_set("ResignMoves","3");
   option_set("ResignScore","600");

   option_set("MateScore","10000");

   option_set("Book","false");
   option_set("BookFile","book.bin");

   option_set("BookRandom","true");
   option_set("BookLearn","false");

   option_set("KibitzMove","false");
   option_set("KibitzPV","false");

   option_set("KibitzCommand","tellall");
   option_set("KibitzDelay","5");

   option_set("ShowPonder","true");
   option_set("ScoreWhite","false");
   option_set("NoGlobals","false");
   option_set("InfoStrings","false");
   option_set("MultiPVall","false");

   // work-arounds

   option_set("UCIVersion","2");
   option_set("CanPonder","false");
   option_set("SyncStop","false");
   option_set("Affinity","-1");
   option_set("PromoteWorkAround","false");
   option_set("RepeatPV","true");
   option_set("PostDelay","0");
   // option_set("","");
}

// option_set()

bool option_set(const char var[], const char val[]) {

   option_t * opt;

   ASSERT(var!=NULL);
   ASSERT(val!=NULL);

   opt = option_find(var);
   if (opt == NULL) return false;

   my_string_set(&opt->val,val);

   if (UseDebug) my_log("POLYGLOT OPTION SET \"%s\" -> \"%s\"\n",opt->var,opt->val);

   return true;
}

// option_get()

const char * option_get(const char var[]) {

   option_t * opt;

   ASSERT(var!=NULL);

   opt = option_find(var);
   if (opt == NULL) my_fatal("option_get(): unknown option \"%s\"\n",var);

   if (UseDebug) my_log("POLYGLOT OPTION GET \"%s\" -> \"%s\"\n",opt->var,opt->val);

   return opt->val;
}

// option_get_bool()

bool option_get_bool(const char var[]) {

   const char * val;

   val = option_get(var);

   if (false) {
   } else if (my_string_case_equal(val,"true") || my_string_case_equal(val,"yes") || my_string_equal(val,"1")) {
      return true;
   } else if (my_string_case_equal(val,"false") || my_string_case_equal(val,"no") || my_string_equal(val,"0")) {
      return false;
   }

   ASSERT(false);

   return false;
}

// option_get_double()

double option_get_double(const char var[]) {

   const char * val;

   val = option_get(var);

   return atof(val);
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
      if (my_string_case_equal(opt->var,var)) return opt;
   }

   return NULL;
}

// end of option.cpp

