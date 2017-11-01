
// recog.cpp

// includes

#include "board.h"
#include "colour.h"
#include "material.h"
#include "piece.h"
#include "recog.h"
#include "util.h"
#include "vector.h"

// prototypes

static bool kpk_draw  (int wp, int wk, int bk, int turn);
static bool kbpk_draw (int wp, int wb, int bk);

// functions

// recog_draw()

bool recog_draw(const board_t * board) {

   material_info_t mat_info[1];

   ASSERT(board!=NULL);

   // material

   if (board->piece_nb > 4) return false;

   material_get_info(mat_info,board);

   if ((mat_info->flags & DrawNodeFlag) == 0) return false;

   // recognisers

   if (false) {

   } else if (mat_info->recog == MAT_KK) {

      // KK

      return true;

   } else if (mat_info->recog == MAT_KBK) {

      // KBK (white)

      return true;

   } else if (mat_info->recog == MAT_KKB) {

      // KBK (black)

      return true;

   } else if (mat_info->recog == MAT_KNK) {

      // KNK (white)

      return true;

   } else if (mat_info->recog == MAT_KKN) {

      // KNK (black)

      return true;

   } else if (mat_info->recog == MAT_KPK) {

      // KPK (white)

      int me, opp;
      int wp, wk, bk;

      me = White;
      opp = COLOUR_OPP(me);

      wp = board->pawn[me][0];
      wk = KING_POS(board,me);
      bk = KING_POS(board,opp);

      if (SQUARE_FILE(wp) >= FileE) {
         wp = SQUARE_FILE_MIRROR(wp);
         wk = SQUARE_FILE_MIRROR(wk);
         bk = SQUARE_FILE_MIRROR(bk);
      }

      if (kpk_draw(wp,wk,bk,board->turn)) {
         return true;
      }

   } else if (mat_info->recog == MAT_KKP) {

      // KPK (black)

      int me, opp;
      int wp, wk, bk;

      me = Black;
      opp = COLOUR_OPP(me);

      wp = SQUARE_RANK_MIRROR(board->pawn[me][0]);
      wk = SQUARE_RANK_MIRROR(KING_POS(board,me));
      bk = SQUARE_RANK_MIRROR(KING_POS(board,opp));

      if (SQUARE_FILE(wp) >= FileE) {
         wp = SQUARE_FILE_MIRROR(wp);
         wk = SQUARE_FILE_MIRROR(wk);
         bk = SQUARE_FILE_MIRROR(bk);
      }

      if (kpk_draw(wp,wk,bk,COLOUR_OPP(board->turn))) {
         return true;
      }

   } else if (mat_info->recog == MAT_KBKB) {

      // KBKB

      int wb, bb;

      wb = board->piece[White][1];
      bb = board->piece[Black][1];

      if (SQUARE_COLOUR(wb) == SQUARE_COLOUR(bb)) { // bishops on same colour
         return true;
      }

   } else if (mat_info->recog == MAT_KBPK) {

      // KBPK (white)

      int me, opp;
      int wp, wb, bk;

      me = White;
      opp = COLOUR_OPP(me);

      wp = board->pawn[me][0];
      wb = board->piece[me][1];
      bk = KING_POS(board,opp);

      if (SQUARE_FILE(wp) >= FileE) {
         wp = SQUARE_FILE_MIRROR(wp);
         wb = SQUARE_FILE_MIRROR(wb);
         bk = SQUARE_FILE_MIRROR(bk);
      }

      if (kbpk_draw(wp,wb,bk)) return true;

   } else if (mat_info->recog == MAT_KKBP) {

      // KBPK (black)

      int me, opp;
      int wp, wb, bk;

      me = Black;
      opp = COLOUR_OPP(me);

      wp = SQUARE_RANK_MIRROR(board->pawn[me][0]);
      wb = SQUARE_RANK_MIRROR(board->piece[me][1]);
      bk = SQUARE_RANK_MIRROR(KING_POS(board,opp));

      if (SQUARE_FILE(wp) >= FileE) {
         wp = SQUARE_FILE_MIRROR(wp);
         wb = SQUARE_FILE_MIRROR(wb);
         bk = SQUARE_FILE_MIRROR(bk);
      }

      if (kbpk_draw(wp,wb,bk)) return true;

   } else {

      ASSERT(false);
   }

   return false;
}

// kpk_draw()

static bool kpk_draw(int wp, int wk, int bk, int turn) {

   int wp_file, wp_rank;
   int wk_file;
   int bk_file, bk_rank;

   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_IS_OK(wk));
   ASSERT(SQUARE_IS_OK(bk));
   ASSERT(COLOUR_IS_OK(turn));

   ASSERT(SQUARE_FILE(wp)<=FileD);

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   wk_file = SQUARE_FILE(wk);

   bk_file = SQUARE_FILE(bk);
   bk_rank = SQUARE_RANK(bk);

   if (false) {

   } else if (bk == wp+16) {

      if (wp_rank <= Rank6) {

         return true;

      } else {

         ASSERT(wp_rank==Rank7);

         if (COLOUR_IS_WHITE(turn)) {
            if (wk == wp-15 || wk == wp-17) return true;
         } else {
            if (wk != wp-15 && wk != wp-17) return true;
         }
      }

   } else if (bk == wp+32) {

      if (wp_rank <= Rank5) {

         return true;

      } else {

         ASSERT(wp_rank==Rank6);

         if (COLOUR_IS_WHITE(turn)) {
            if (wk != wp-1 && wk != wp+1) return true;
         } else {
            return true;
         }
      }

   } else if (wk == wp-1 || wk == wp+1) {

      if (bk == wk+32 && COLOUR_IS_WHITE(turn)) { // opposition
         return true;
      }

   } else if (wk == wp+15 || wk == wp+16 || wk == wp+17) {

      if (wp_rank <= Rank4) {
         if (bk == wk+32 && COLOUR_IS_WHITE(turn)) { // opposition
            return true;
         }
      }
   }

   // rook pawn

   if (wp_file == FileA) {

      if (DISTANCE(bk,A8) <= 1) return true;

      if (wk_file == FileA) {
         if (wp_rank == Rank2) wp_rank++; // HACK
         if (bk_file == FileC && bk_rank > wp_rank) return true;
      }
   }

   return false;
}

// kbpk_draw()

static bool kbpk_draw (int wp, int wb, int bk) {

   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_IS_OK(wb));
   ASSERT(SQUARE_IS_OK(bk));

   if (SQUARE_FILE(wp) == FileA
    && DISTANCE(bk,A8) <= 1
    && SQUARE_COLOUR(wb) != SQUARE_COLOUR(A8)) {
      return true;
   }

   return false;
}

// end of recog.cpp

