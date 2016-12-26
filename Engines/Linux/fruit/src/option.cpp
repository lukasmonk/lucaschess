
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

   { "Hash", true, "16", "spin", "min 4 max 1024", NULL },

   { "Ponder", true, "false", "check", "", NULL },

   { "OwnBook",  true, "true",           "check",  "", NULL },
   { "BookFile", true, "book_small.bin", "string", "", NULL },

   { "NullMove Pruning",       true, "Fail High", "combo", "var Always var Fail High var Never", NULL },
   { "NullMove Reduction",     true, "3",         "spin",  "min 1 max 3", NULL },
   { "Verification Search",    true, "Endgame",   "combo", "var Always var Endgame var Never", NULL },
   { "Verification Reduction", true, "5",         "spin",  "min 1 max 6", NULL },

   { "History Pruning",     true, "true", "check", "", NULL },
   { "History Threshold",   true, "60",   "spin",  "min 0 max 100", NULL },

   { "Futility Pruning", true, "false", "check", "", NULL },
   { "Futility Margin",  true, "100",   "spin",  "min 0 max 500", NULL },

   { "Delta Pruning", true, "false", "check", "", NULL },
   { "Delta Margin",  true, "50",    "spin",  "min 0 max 500", NULL },

   { "Quiescence Check Plies", true, "1", "spin", "min 0 max 2", NULL },

   { "Material",        true, "100", "spin", "min 0 max 400", NULL },
   { "Piece Activity",  true, "100", "spin", "min 0 max 400", NULL },
   { "King Safety",     true, "100", "spin", "min 0 max 400", NULL },
   { "Pawn Structure",  true, "100", "spin", "min 0 max 400", NULL },
   { "Passed Pawns",    true, "100", "spin", "min 0 max 400", NULL },

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

