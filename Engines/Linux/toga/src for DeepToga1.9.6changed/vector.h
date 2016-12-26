
// vector.h

#ifndef VECTOR_H
#define VECTOR_H

// includes

#include "util.h"

// "constants"

const int IncNone = 0;
const int IncNb = 2 * 17 + 1;
const int IncOffset = 17;

const int DeltaNone = 0;
const int DeltaNb = 2 * 119 + 1;
const int DeltaOffset = 119;

// macros

#define DISTANCE(square_1,square_2) (Distance[DeltaOffset+((square_2)-(square_1))])
#define  TROPISM(square_1,square_2)  (Tropism[DeltaOffset+((square_2)-(square_1))]) // WHM

#define QUEEN_INDEX(delta) (QueenIndex[17 + delta])

// variables

extern int Distance[DeltaNb];
extern int  Tropism[DeltaNb]; // WHM
extern int  QueenIndex[35];

// functions

extern void vector_init ();

extern bool delta_is_ok (int delta);
extern bool inc_is_ok   (int inc);

#endif // !defined VECTOR_H

// end of vector.h

