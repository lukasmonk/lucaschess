
// trans.h

#ifndef TRANS_H
#define TRANS_H

// includes

#include "util.h"

// constants

const int TransUpperFlag = 1 << 0;
const int TransLowerFlag = 1 << 1;

const int TransUnknown = 0;
const int TransUpper   = TransUpperFlag;
const int TransLower   = TransLowerFlag;
const int TransExact   = TransUpperFlag | TransLowerFlag;
const int TransFlags   = 3;

const int DepthNone = -128; // WHM moved

// macros

#define TRANS_IS_UPPER(flags) (((flags)&TransUpperFlag)!=0)
#define TRANS_IS_LOWER(flags) (((flags)&TransLowerFlag)!=0)
#define TRANS_IS_EXACT(flags) ((flags)==TransExact)

struct entry_t {
   uint64 key;
   unsigned short move;
   unsigned char depth;
   unsigned char date_flags;
   signed short value;
   unsigned short nhits; // WHM to count the number of reads...
};

// types

typedef struct trans trans_t;

// variables

extern volatile trans_t Trans[1];

// functions

extern bool trans_is_ok    (const volatile trans_t * trans);

extern void trans_init     (volatile trans_t * trans);
extern void trans_alloc    (volatile trans_t * trans);
extern void trans_free     (volatile trans_t * trans);

extern void trans_clear    (volatile trans_t * trans);
extern void trans_inc_date (volatile trans_t * trans);

// WHM entry_t ** entry args removed, minimizing args passed max's nps...  Book, material and pawn (entry_t)'s re-named...  
#if DEBUG
#include "move_legal.h"
#include "board.h"
extern void trans_store    (volatile trans_t * trans, uint64 key, int   move, int   depth, int   flags, int   value, board_t * board);
#else
extern void trans_store    (volatile trans_t * trans, uint64 key, int   move, int   depth, int   flags, int   value);
#endif
extern bool trans_retrieve (volatile trans_t * trans, uint64 key, int * move, int * depth, int * flags, int * value);

extern void trans_stats    (const volatile trans_t * trans);

#endif // !defined TRANS_H

// end of trans.h

