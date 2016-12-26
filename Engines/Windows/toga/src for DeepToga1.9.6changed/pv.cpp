
// pv.cpp

// includes

#include <cstring>

//#include "board.h"
//#include "move.h"
//#include "move_do.h"
#include "pv.h"
//#include "util.h"

// functions

// pv_is_ok()

bool pv_is_ok(const mv_t pv[]) {

   int pos;
   int move;

   if (pv == NULL) return false;

   for (pos = 0; true; pos++) {

      if (pos >= 256) return false;
      move = pv[pos];

      if (move == MoveNone) return true;
      if (!move_is_ok(move)) return false;
   }

   return false; // WHM was true, gotta exit with the last MoveNone!  
}

// pv_is_ok()

bool pv_is_ok(const volatile mv_t pv[]) {

   int pos;
   int move;

   if (pv == NULL) return false;

   for (pos = 0; true; pos++) {

      if (pos >= 256) return false;
      move = pv[pos];

      if (move == MoveNone) return true;
      if (!move_is_ok(move)) return false;
   }

   return false; // WHM was true, gotta exit with the last MoveNone!  
}

// pv_copy()  SLOW now.

void pv_copy(mv_t dst[], const mv_t src[]) {

   ASSERT(pv_is_ok(src)); // a sequence of moves followed by MoveNone terminating the pv[].  
   ASSERT(dst!=NULL);
   ASSERT(src!=NULL);

   PV_CLEAR(dst); // init the pv

   while (move_is_ok(*src)  &&  (*dst++ = *src++) != MoveNone) // added move_is_ok(*src)
       ;

   *dst = MoveNone; // WHM end the pv for sure
}

// pv_copy()

void pv_copy(volatile mv_t dst[], const volatile mv_t src[]) {

// ASSERT(pv_is_ok(src));
   ASSERT(dst!=NULL);

   while ((*dst++ = *src++) != MoveNone)
      ;
}

// pv_cat()

void pv_cat(mv_t dst[], const mv_t src[], int move) {

   ASSERT(pv_is_ok(src));
   ASSERT(dst!=NULL);

   *dst++ = move;

   while ((*dst++ = *src++) != MoveNone)
      ;

#if DEBUG
      dst--;
      ASSERT(*dst == MoveNone);
#endif
}

// pv_cat()

void pv_cat(volatile mv_t dst[], const mv_t src[], int move) {

// ASSERT(pv_is_ok(src));
   ASSERT(dst!=NULL);

   *dst++ = move;

   while ((*dst++ = *src++) != MoveNone)
      ;

#if DEBUG
      dst--;
      ASSERT(*dst == MoveNone);
#endif
}

// pv_to_string()

bool pv_to_string(const mv_t pv[], char string[], int size) {

   int pos;
   int move;

   ASSERT(pv_is_ok(pv));
   ASSERT(string!=NULL);
   ASSERT(size>=512);

   // init

   if (size < 512) return false;

   pos = 0;

   // loop

   while ((move = *pv++) != MoveNone) {

      if (pos != 0) string[pos++] = ' ';

      move_to_string(move,&string[pos],size-pos);
      pos += strlen(&string[pos]);
   }

   string[pos] = '\0';

   return true;
}

// end of pv.cpp

