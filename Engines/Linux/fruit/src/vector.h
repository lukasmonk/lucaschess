
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

// variables

extern int Distance[DeltaNb];

// functions

extern void vector_init ();

extern bool delta_is_ok (int delta);
extern bool inc_is_ok   (int inc);

#endif // !defined VECTOR_H

// end of vector.h

