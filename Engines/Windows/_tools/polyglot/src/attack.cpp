
// attack.cpp

// includes

#include "board.h"
#include "colour.h"
#include "move.h"
#include "attack.h"
#include "piece.h"
#include "util.h"

// macros

#define DELTA_INC(delta)  (DeltaInc[128+(delta)])
#define DELTA_MASK(delta) (DeltaMask[128+(delta)])

// "constants"

const sint8 KnightInc[8+1] = {
   -33, -31, -18, -14, +14, +18, +31, +33, 0
};

const sint8 BishopInc[4+1] = {
   -17, -15, +15, +17, 0
};

const sint8 RookInc[4+1] = {
   -16, -1, +1, +16, 0
};

const sint8 QueenInc[8+1] = {
   -17, -16, -15, -1, +1, +15, +16, +17, 0
};

const sint8 KingInc[8+1] = {
   -17, -16, -15, -1, +1, +15, +16, +17, 0
};

// variables

static sint8 DeltaInc[256];
static uint8 DeltaMask[256];

// prototypes

#if DEBUG
static bool delta_is_ok (int delta);
static bool inc_is_ok   (int inc);
#endif

// functions

// attack_init()

void attack_init() {

   int delta;
   int dir, inc, dist;

   for (delta = -128; delta < +128; delta++) {
      DeltaInc[128+delta] = IncNone;
      DeltaMask[128+delta] = 0;
   }

   DeltaMask[128-17] |= BlackPawnFlag;
   DeltaMask[128-15] |= BlackPawnFlag;

   DeltaMask[128+15] |= WhitePawnFlag;
   DeltaMask[128+17] |= WhitePawnFlag;

   for (dir = 0; dir < 8; dir++) {
      delta = KnightInc[dir];
      ASSERT(delta_is_ok(delta));
      DeltaMask[128+delta] |= KnightFlag;
   }

   for (dir = 0; dir < 4; dir++) {
      inc = BishopInc[dir];
      ASSERT(inc!=IncNone);
      for (dist = 1; dist < 8; dist++) {
         delta = inc*dist;
         ASSERT(delta_is_ok(delta));
         ASSERT(DeltaInc[128+delta]==IncNone);
         DeltaInc[128+delta] = inc;
         DeltaMask[128+delta] |= BishopFlag;
      }
   }

   for (dir = 0; dir < 4; dir++) {
      inc = RookInc[dir];
      ASSERT(inc!=IncNone);
      for (dist = 1; dist < 8; dist++) {
         delta = inc*dist;
         ASSERT(delta_is_ok(delta));
         ASSERT(DeltaInc[128+delta]==IncNone);
         DeltaInc[128+delta] = inc;
         DeltaMask[128+delta] |= RookFlag;
      }
   }

   for (dir = 0; dir < 8; dir++) {
      delta = KingInc[dir];
      ASSERT(delta_is_ok(delta));
      DeltaMask[128+delta] |= KingFlag;
   }
}

// delta_is_ok()
#if DEBUG
static bool delta_is_ok(int delta) {

   if (delta < -119 || delta > +119) return false;

   return true;
}

// inc_is_ok()

static bool inc_is_ok(int inc) {

   int dir;

   for (dir = 0; dir < 8; dir++) {
      if (KingInc[dir] == inc) return true;
   }

   return false;
}
#endif
// is_in_check()

bool is_in_check(const board_t * board, int colour) {

   ASSERT(board_is_ok(board));
   ASSERT(colour_is_ok(colour));

   return is_attacked(board,king_pos(board,colour),colour_opp(colour));
}

// is_attacked()

bool is_attacked(const board_t * board, int to, int colour) {

   const uint8 * ptr;
   int from, piece;

   ASSERT(board_is_ok(board));
   ASSERT(square_is_ok(to));
   ASSERT(colour_is_ok(colour));

   for (ptr = board->list[colour]; (from=*ptr) != SquareNone; ptr++) {

      piece = board->square[from];
      ASSERT(colour_equal(piece,colour));

      if (piece_attack(board,piece,from,to)) return true;
   }

   return false;
}

// piece_attack()

bool piece_attack(const board_t * board, int piece, int from, int to) {

   int delta;
   int inc, sq;

   ASSERT(board_is_ok(board));
   ASSERT(piece_is_ok(piece));
   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));

   delta = to - from;
   ASSERT(delta_is_ok(delta));

   if ((piece & DELTA_MASK(delta)) == 0) return false; // no pseudo-attack

   if (!piece_is_slider(piece)) return true;

   inc = DELTA_INC(delta);
   ASSERT(inc_is_ok(inc));

   for (sq = from+inc; sq != to; sq += inc) {
      ASSERT(square_is_ok(sq));
      if (board->square[sq] != Empty) return false; // blocker
   }

   return true;
}

// is_pinned()

bool is_pinned(const board_t * board, int from, int to, int colour) {

   int king;
   int inc;
   int sq, piece;

   ASSERT(board!=NULL);
   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));
   ASSERT(colour_is_ok(colour));

   king = king_pos(board,colour);

   inc = DELTA_INC(king-from);
   if (inc == IncNone) return false; // not a line

   sq = from;
   do sq += inc; while (board->square[sq] == Empty);

   if (sq != king) return false; // blocker

   sq = from;
   do sq -= inc; while ((piece=board->square[sq]) == Empty);

   return square_is_ok(sq)
       && (piece & DELTA_MASK(king-sq)) != 0
       && piece_colour(piece) == colour_opp(colour)
       && DELTA_INC(king-to) != inc;
}

// end of attack.cpp

