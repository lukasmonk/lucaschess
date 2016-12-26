
// pawn.h

#ifndef PAWN_H
#define PAWN_H

// includes

#include "board.h"
#include "colour.h"
#include "util.h"

// macros

#define BIT(n)       (BitEQ[n])

#define BIT_FIRST(b) (BitFirst[b])
#define BIT_LAST(b)  (BitLast[b])
#define BIT_COUNT(b) (BitCount[b])

// constants

const int BackRankFlag = 1 << 0;

const int BadPawnFileA = 1 << 0;
const int BadPawnFileB = 1 << 1;
const int BadPawnFileC = 1 << 2;
const int BadPawnFileD = 1 << 3;
const int BadPawnFileE = 1 << 4;
const int BadPawnFileF = 1 << 5;
const int BadPawnFileG = 1 << 6;
const int BadPawnFileH = 1 << 7;

// types

struct pawn_info_t {
   uint32 lock;
   sint16 opening;
   sint16 endgame;
   uint8 flags[ColourNb];
   uint8 passed_bits[ColourNb];
   uint8 single_file[ColourNb];
   uint8 badpawns[ColourNb]; // Ryan
   uint8 wsp[ColourNb]; // Ryan
};

// variables

extern int BitEQ[16];
extern int BitLT[16];
extern int BitLE[16];
extern int BitGT[16];
extern int BitGE[16];

extern int BitFirst[0x100];
extern int BitLast[0x100];
extern int BitCount[0x100];
extern int BitRev[0x100];

// functions

extern void pawn_init_bit ();
extern void pawn_init     ();

extern void pawn_alloc    ();
extern void pawn_clear    ();

extern void pawn_get_info (pawn_info_t * info, const board_t * board);

extern int  quad          (int y_min, int y_max, int x);

#endif // !defined PAWN_H

// end of pawn.h

