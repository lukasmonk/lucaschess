
// fen.cpp

// includes

#include <cctype>
#include <cstdio>
#include <cstdlib>

#include "board.h"
#include "colour.h"
#include "fen.h"
#include "piece.h"
#include "square.h"
#include "util.h"

// "constants"

const char * const StartFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// variables

static const bool Strict = false;

// functions

// board_from_fen()

void board_from_fen(board_t * board, const char fen[]) {

   int pos;
   int file, rank, sq;
   int c;
   int i, len;
   int piece;
   int pawn;

   ASSERT(board!=NULL);
   ASSERT(fen!=NULL);

   board_clear(board);

   pos = 0;
   c = fen[pos];

   // piece placement

   for (rank = Rank8; rank >= Rank1; rank--) {

      for (file = FileA; file <= FileH;) {

         if (c >= '1' && c <= '8') { // empty square(s)

            len = c - '0';

            for (i = 0; i < len; i++) {
               if (file > FileH) my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
               board->square[SQUARE_MAKE(file,rank)] = Empty;
               file++;
            }

         } else { // piece

            piece = piece_from_char(c);
            if (piece == PieceNone256) my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);

            board->square[SQUARE_MAKE(file,rank)] = piece;
            file++;
         }

         c = fen[++pos];
      }

      if (rank > Rank1) {
         if (c != '/') my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
         c = fen[++pos];
     }
   }

   // active colour

   if (c != ' ') my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
   c = fen[++pos];

   switch (c) {
   case 'w':
      board->turn = White;
      break;
   case 'b':
      board->turn = Black;
      break;
   default:
      my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
      break;
   }

   c = fen[++pos];

   // castling

   if (c != ' ') my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
   c = fen[++pos];

   board->flags = FlagsNone;

   if (c == '-') { // no castling rights

      c = fen[++pos];

   } else {

      if (c == 'K') {
         if (board->square[E1] == WK && board->square[H1] == WR) board->flags |= FlagsWhiteKingCastle;
         c = fen[++pos];
      }

      if (c == 'Q') {
         if (board->square[E1] == WK && board->square[A1] == WR) board->flags |= FlagsWhiteQueenCastle;
         c = fen[++pos];
      }

      if (c == 'k') {
         if (board->square[E8] == BK && board->square[H8] == BR) board->flags |= FlagsBlackKingCastle;
         c = fen[++pos];
      }

      if (c == 'q') {
         if (board->square[E8] == BK && board->square[A8] == BR) board->flags |= FlagsBlackQueenCastle;
         c = fen[++pos];
      }
   }

   // en-passant

   if (c != ' ') my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
   c = fen[++pos];

   if (c == '-') { // no en-passant

      sq = SquareNone;
      c = fen[++pos];

   } else {

      if (c < 'a' || c > 'h') my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
      file = file_from_char(c);
      c = fen[++pos];

      if (c != (COLOUR_IS_WHITE(board->turn) ? '6' : '3')) my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
      rank = rank_from_char(c);
      c = fen[++pos];

      sq = SQUARE_MAKE(file,rank);
      pawn = SQUARE_EP_DUAL(sq);

      if (board->square[sq] != Empty
       || board->square[pawn] != PAWN_MAKE(COLOUR_OPP(board->turn))
       || (board->square[pawn-1] != PAWN_MAKE(board->turn)
        && board->square[pawn+1] != PAWN_MAKE(board->turn))) {
         sq = SquareNone;
      }
   }

   board->ep_square = sq;

   // halfmove clock

   board->ply_nb = 0;

   if (c != ' ') {
      if (!Strict) goto update;
      my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
   }
   c = fen[++pos];

   if (!isdigit(c)) {
      if (!Strict) goto update;
      my_fatal("board_from_fen(): bad FEN (pos=%d)\n",pos);
   }

   board->ply_nb = atoi(&fen[pos]);

   // board update

update:
   board_init_list(board);
}

// board_to_fen()

bool board_to_fen(const board_t * board, char fen[], int size) {

   int pos;
   int file, rank;
   int sq, piece;
   int c;
   int len;

   ASSERT(board!=NULL);
   ASSERT(fen!=NULL);
   ASSERT(size>=92);

   // init

   if (size < 92) return false;

   pos = 0;

   // piece placement

   for (rank = Rank8; rank >= Rank1; rank--) {

      for (file = FileA; file <= FileH;) {

         sq = SQUARE_MAKE(file,rank);
         piece = board->square[sq];
         ASSERT(piece==Empty||piece_is_ok(piece));

         if (piece == Empty) {

            len = 0;
            for (; file <= FileH && board->square[SQUARE_MAKE(file,rank)] == Empty; file++) {
               len++;
            }

            ASSERT(len>=1&&len<=8);
            c = '0' + len;

         } else {

            c = piece_to_char(piece);
            file++;
         }

         fen[pos++] = c;
      }

      fen[pos++] = '/';
   }

   fen[pos-1] = ' '; // HACK: remove the last '/'

   // active colour

   fen[pos++] = (COLOUR_IS_WHITE(board->turn)) ? 'w' : 'b';
   fen[pos++] = ' ';

   // castling

   if (board->flags == FlagsNone) {
      fen[pos++] = '-';
   } else {
      if ((board->flags & FlagsWhiteKingCastle)  != 0) fen[pos++] = 'K';
      if ((board->flags & FlagsWhiteQueenCastle) != 0) fen[pos++] = 'Q';
      if ((board->flags & FlagsBlackKingCastle)  != 0) fen[pos++] = 'k';
      if ((board->flags & FlagsBlackQueenCastle) != 0) fen[pos++] = 'q';
   }

   fen[pos++] = ' ';

   // en-passant

   if (board->ep_square == SquareNone) {
      fen[pos++] = '-';
   } else {
      square_to_string(board->ep_square,&fen[pos],3);
      pos += 2;
   }

   fen[pos++] = ' ';

   // halfmove clock

   sprintf(&fen[pos],"%d 1",board->ply_nb);

   return true;
}

// end of fen.cpp

