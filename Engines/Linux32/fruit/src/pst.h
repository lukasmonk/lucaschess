
// pst.h

#ifndef PST_H
#define PST_H

// includes

#include "util.h"

// constants

const int Opening = 0;
const int Endgame = 1;
const int StageNb = 2;

// macros

#define PST(piece_12,square_64,stage) (Pst[piece_12][square_64][stage])

// variables

extern sint16 Pst[12][64][StageNb];

// functions

extern void pst_init ();

#endif // !defined PST_H

// end of pst.h

