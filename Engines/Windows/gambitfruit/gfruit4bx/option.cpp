
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

    // JAS
    // search X seconds for the best move, equal to "go movetime"
    { "Search Time",  true, "0",   "spin",  "min 0 max 3600", NULL },
    // search X plies deep, equal to "go depth"
    { "Search Depth",  true, "0",   "spin",  "min 0 max 20", NULL },
    // JAS end

    { "Ponder", true, "false", "check", "", NULL },

    { "OwnBook",  true, "true",           "check",  "", NULL },
    { "BookFile", true, "book_small.bin", "string", "", NULL },
    { "MultiPV", true, "1", "spin",  "min 1 max 10", NULL },

    { "Bitbase pieces", true, "4", "spin",  "min 3 max 4", NULL },

    { "NullMove Pruning",       true, "Always", "combo", "var Always var Fail High var Never", NULL },
    { "NullMove Reduction",     true, "3",         "spin",  "min 1 max 4", NULL },
    { "Verification Search",    true, "Always",   "combo", "var Always var Endgame var Never", NULL },
    { "Verification Reduction", true, "5",         "spin",  "min 1 max 6", NULL },

    { "History Pruning",     true, "true", "check", "", NULL },
    { "History Threshold",   true, "70",   "spin",  "min 0 max 100", NULL },
    { "History Research on Beta",     true, "true", "check", "", NULL },

    { "Rebel Reductions",     true, "true", "check", "", NULL },

    { "Futility Pruning", true, "true", "check", "", NULL },
    { "Quick Futility eval", true, "false", "check", "", NULL },
    { "Futility Margin",  true, "125",   "spin",  "min 0 max 500", NULL },
    { "Extended Futility Margin",  true, "325",   "spin",  "min 0 max 900", NULL },

    { "Delta Pruning", true, "true", "check", "", NULL },
    { "Delta Margin",  true, "50",    "spin",  "min 0 max 500", NULL },

    { "Quiescence Check Plies", true, "1", "spin", "min 0 max 5", NULL },

    { "Alt Pawn SQT", true, "false", "check", "", NULL },
    { "Alt Knight SQT", true, "false", "check", "", NULL },
    { "Alt Bishop SQT", true, "false", "check", "", NULL },

    { "Chess Knowledge", true, "100", "spin", "min 0 max 500", NULL },
    { "Piece Activity",  true, "100", "spin", "min 0 max 500", NULL },
    { "Pawn Shelter",    true, "100", "spin", "min 0 max 500", NULL },
    { "Pawn Storm",      true, "100", "spin", "min 0 max 500", NULL },
    { "King Attack",     true, "100", "spin", "min 0 max 500", NULL },
    { "Pawn Structure",  true, "100", "spin", "min 0 max 500", NULL },
    { "Passed Pawns",    true, "100", "spin", "min 0 max 500", NULL },
    /*
       { "knight tropism opening",  	true, "4",    "spin",  "min 0 max 10", NULL },
       { "bishop tropism opening",  	true, "2",    "spin",  "min 0 max 10", NULL },
       { "rook tropism opening",  		true, "2",    "spin",  "min 0 max 10", NULL },
       { "queen tropism opening",  		true, "3",    "spin",  "min 0 max 10", NULL },

       { "knight tropism endgame",  	true, "2",    "spin",  "min 0 max 10", NULL },
       { "bishop tropism endgame",  	true, "1",    "spin",  "min 0 max 10", NULL },
       { "rook tropism endgame",  		true, "1",    "spin",  "min 0 max 10", NULL },
       { "queen tropism endgame",  		true, "3",    "spin",  "min 0 max 10", NULL },
    */
    { "Opening Pawn Value",	true, "80", "spin", "min 0 max 10000", NULL },
    { "Opening Knight Value",	true, "320", "spin", "min 0 max 10000", NULL },
    { "Opening Bishop Value",	true, "325", "spin", "min 0 max 10000", NULL },
    { "Opening Rook Value",	true, "500", "spin", "min 0 max 10000", NULL },
    { "Opening Queen Value",	true, "975", "spin", "min 0 max 10000", NULL },

    { "Endgame Pawn Value",	true, "90", "spin", "min 0 max 10000", NULL },
    { "Endgame Knight Value",	true, "320", "spin", "min 0 max 10000", NULL },
    { "Endgame Bishop Value",	true, "325", "spin", "min 0 max 10000", NULL },
    { "Endgame Rook Value",	true, "500", "spin", "min 0 max 10000", NULL },
    { "Endgame Queen Value",	true, "975", "spin", "min 0 max 10000", NULL },

    { "Bishop Pair Opening",	true, "50", "spin", "min 0 max 1000", NULL },
    { "Bishop Pair Endgame",	true, "50", "spin", "min 0 max 1000", NULL },

    { "Queen Knight combo", 	true, "15", "spin", "min 0 max 1000", NULL },
    { "Rook Bishop combo",	true, "15", "spin", "min 0 max 1000", NULL },

    { "Bad Trade Value",  true, "50",    "spin",  "min 0 max 1000", NULL },

    { "Contempt Factor",	true, "0", "spin", "min -1000 max 1000", NULL },

    { "Max NPS", true, "999999", "spin min 1 max 999999",  NULL }, //LC change

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

