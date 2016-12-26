
// material.h

#ifndef MATERIAL_H
#define MATERIAL_H

#define MAX_MATERIAL            2*2*3*3*3*3*3*3*9*9

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

// matching Toga1.4.1SE, Chris Formula's values...
const int PawnOpening   =  70; // was 100   TogaII1.2.1 == TogaII1.4.1SE == 70    fruit 2.1 == TogaII1.4beta5c == 80
const int PawnEndgame   =  90; // was 100   TogaII1.2.1 == TogaII1.4.1SE == 90 == fruit 2.1 == TogaII1.4beta5c == 90
const int KnightOpening = 325;
const int KnightEndgame = 315;
const int BishopOpening = 325;
const int BishopEndgame = 315;
const int RookOpening   = 500; //  500 for both fruit21 and Toga
const int RookEndgame   = 500; //  500 for both fruit21 and Toga
const int QueenOpening  = 975; // 1000==fruit21 975==Toga.
const int QueenEndgame  = 975; // 1000==fruit21 975==Toga.

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
   int fast_phase; // WHM
};

// functions

extern void material_init     ();
extern void material_parameter();

extern void material_alloc    ();
extern void material_clear    (int ThreadId);

extern void material_get_info (material_info_t * info, const board_t * board, int ThreadId);

#endif // !defined MATERIAL_H

// end of material.h

