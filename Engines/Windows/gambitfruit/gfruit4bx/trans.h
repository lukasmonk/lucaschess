
// trans.h

#ifndef TRANS_H
#define TRANS_H

// includes

#include "util.h"

// types

typedef struct trans trans_t;

// variables

extern trans_t Trans[1];
extern bool trans_endgame;

// functions

extern bool trans_is_ok    (const trans_t * trans);

extern void trans_init     (trans_t * trans);
extern void trans_alloc    (trans_t * trans);
extern void trans_free     (trans_t * trans);

extern void trans_clear    (trans_t * trans);
extern void trans_inc_date (trans_t * trans);

extern void trans_store    (trans_t * trans, uint64 key, int move, int depth, int min_value, int max_value);
extern bool trans_retrieve (trans_t * trans, uint64 key, int * move, int * min_depth, int * max_depth, int * min_value, int * max_value);

extern void trans_stats    (const trans_t * trans);

#endif // !defined TRANS_H

// end of trans.h

