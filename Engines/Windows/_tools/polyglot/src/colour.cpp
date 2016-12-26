
// colour.cpp

// includes

#include "colour.h"
#include "util.h"

// functions

// colour_is_ok()

bool colour_is_ok(int colour) {

   return colour == Black || colour == White;
}

// colour_is_white()

bool colour_is_white(int colour) {

   ASSERT(colour_is_ok(colour));

   return colour == White;
}

// colour_is_black()

bool colour_is_black(int colour) {

   ASSERT(colour_is_ok(colour));

   return colour == Black;
}

// colour_equal()

bool colour_equal(int colour_1, int colour_2) {

   ASSERT(colour_is_ok(colour_2));

   return (colour_1 & colour_2) != 0;
}

// colour_opp()

int colour_opp(int colour) {

   ASSERT(colour_is_ok(colour));

   return colour ^ (BlackFlag^WhiteFlag);
}

// end of colour.cpp

