
// vector.cpp

// includes

#include "piece.h"
#include "square.h"
#include "util.h"
#include "vector.h"

// variables

int Distance[DeltaNb];

// functions

void vector_init() {

   int delta;
   int x, y;
   int dist, tmp;

   // Distance[]

   for (delta = 0; delta < DeltaNb; delta++) Distance[delta] = -1;

   for (y = -7; y <= +7; y++) {

      for (x = -7; x <= +7; x++) {

         delta = y * 16 + x;
         ASSERT(delta_is_ok(delta));

         dist = 0;

         tmp = x;
         if (tmp < 0) tmp = -tmp;
         if (tmp > dist) dist = tmp;

         tmp = y;
         if (tmp < 0) tmp = -tmp;
         if (tmp > dist) dist = tmp;

         Distance[DeltaOffset+delta] = dist;
      }
   }
}

// delta_is_ok()

bool delta_is_ok(int delta) {

   if (delta < -119 || delta > +119) return false;

   if ((delta & 0xF) == 8) return false; // HACK: delta % 16 would be ill-defined for negative numbers

   return true;
}

// inc_is_ok()

bool inc_is_ok(int inc) {

   int dir;

   for (dir = 0; dir < 8; dir++) {
      if (KingInc[dir] == inc) return true;
   }

   return false;
}

// end of vector.cpp

