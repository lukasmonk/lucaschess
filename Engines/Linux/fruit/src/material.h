
// material.h

#ifndef MATERIAL_H
#define MATERIAL_H

// includes

#include "board.h"
#include "colour.h"
#include "util.h"

// constants

enum mat_dummy_t {
   MAT_NONE,
   MAT_KK,
   MAT_KBK, MAT_KKB,
   MAT_KNK, MAT_KKN,
   MAT_KPK, MAT_KKP,
   MAT_KQKQ, MAT_KQKP, MAT_KPKQ,
   MAT_KRKR, MAT_KRKP, MAT_KPKR,
   MAT_KBKB, MAT_KBKP, MAT_KPKB, MAT_KBPK, MAT_KKBP,
   MAT_KNKN, MAT_KNKP, MAT_KPKN, MAT_KNPK, MAT_KKNP,
   MAT_KRPKR, MAT_KRKRP,
   MAT_KBPKB, MAT_KBKBP,
   MAT_NB
};

const int DrawNodeFlag    = 1 << 0;
const int DrawBishopFlag  = 1 << 1;

const int MatRookPawnFlag = 1 << 0;
const int MatBishopFlag   = 1 << 1;
const int MatKnightFlag   = 1 << 2;
const int MatKingFlag     = 1 << 3;

// types

struct material_info_t {
   uint32 lock;
   uint8 recog;
   uint8 flags;
   uint8 cflags[ColourNb];
   uint8 mul[ColourNb];
   sint16 phase;
   sint16 opening;
   sint16 endgame;
};

// functions

extern void material_init     ();

extern void material_alloc    ();
extern void material_clear    ();

extern void material_get_info (material_info_t * info, const board_t * board);

#endif // !defined MATERIAL_H

// end of material.h

