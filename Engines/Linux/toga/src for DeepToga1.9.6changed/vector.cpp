
// vector.cpp

// includes

#include "piece.h"
#include "square.h"
#include "util.h"
#include "vector.h"

// variables

int Distance[DeltaNb];
int  Tropism[DeltaNb];
int  QueenIndex[35]; // WHM; get index [0, 7] from direction

// functions

void vector_init() {

   int delta;
   int x, y;
   int dist, tmp;
   int i; // WHM

   // Distance[] & Tropism[]

   for (delta = 0; delta < DeltaNb; delta++) Distance[delta] =  8; // -1;
   for (delta = 0; delta < DeltaNb; delta++)  Tropism[delta] = 15; // WHM

   for (y = -7; y <= +7; y++) {

      for (x = -7; x <= +7; x++) {

         delta = y * 16 + x; // sq1 - sq2 on 16x16 board
         ASSERT(delta_is_ok(delta));

         dist = 0;

         tmp = x;
         if (tmp < 0) tmp = -tmp;
         if (tmp > dist) dist = tmp;
         Tropism[DeltaOffset+delta] = tmp; // WHM

         tmp = y;
         if (tmp < 0) tmp = -tmp;
         if (tmp > dist) dist = tmp;
         Tropism[DeltaOffset+delta] += tmp; // WHM

         Distance[DeltaOffset+delta] = dist;
      }
   }

   for (i = 0; i < 35; i++) QueenIndex[i] = -1; // OB

   for (i = 0; i < 8; i++) {
      delta = QueenInc[i];
      ASSERT(17 + delta >=  0);
      ASSERT(17 + delta <= 34);
      QueenIndex[17 + delta] = i; // i is the index
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

