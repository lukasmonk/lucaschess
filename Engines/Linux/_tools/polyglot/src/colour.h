
// colour.h

#ifndef COLOUR_H
#define COLOUR_H

// includes

#include "util.h"

// constants

const int BlackFlag = 1 << 0;
const int WhiteFlag = 1 << 1;

const int ColourNone = 0;
const int Black      = BlackFlag;
const int White      = WhiteFlag;
const int ColourNb   = 3;

// functions

extern bool colour_is_ok    (int colour);

extern bool colour_is_white (int colour);
extern bool colour_is_black (int colour);
extern bool colour_equal    (int colour_1, int colour_2);

extern int  colour_opp      (int colour);

#endif // !defined COLOUR_H

// end of colour.h

