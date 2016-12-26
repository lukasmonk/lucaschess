
// move_do.cpp

// includes

#include <cstdlib>

#include "board.h"
#include "colour.h"
#include "hash.h"
#include "move.h"
#include "move_do.h"
#include "move_legal.h"
#include "piece.h"
#include "random.h"
#include "util.h"

// prototypes

static void square_clear (board_t * board, int square, int piece);
static void square_set   (board_t * board, int square, int piece, int pos);
static void square_move  (board_t * board, int from, int to, int piece);

// functions

// move_do()

void move_do(board_t * board, int move) {

   int me, opp;
   int from, to;
   int piece, pos, capture;
   int old_flags, new_flags;
   int sq, ep_square;
   int pawn;

   ASSERT(board_is_ok(board));
   ASSERT(move_is_ok(move));

   ASSERT(move_is_pseudo(move,board));

   // init

   me = board->turn;
   opp = colour_opp(me);

   from = move_from(move);
   to = move_to(move);

   piece = board->square[from];
   ASSERT(colour_equal(piece,me));

   pos = board->pos[from];
   ASSERT(pos>=0);

   // update turn

   board->turn = opp;
   board->key ^= random_64(RandomTurn);

   // update castling rights

   old_flags = board_flags(board);

   if (piece_is_king(piece)) {
      board->castle[me][SideH] = SquareNone;
      board->castle[me][SideA] = SquareNone;
   }

   if (board->castle[me][SideH] == from) board->castle[me][SideH] = SquareNone;
   if (board->castle[me][SideA] == from) board->castle[me][SideA] = SquareNone;

   if (board->castle[opp][SideH] == to) board->castle[opp][SideH] = SquareNone;
   if (board->castle[opp][SideA] == to) board->castle[opp][SideA] = SquareNone;

   new_flags = board_flags(board);

   board->key ^= hash_castle_key(new_flags^old_flags); // HACK

   // update en-passant square

   ep_square = sq = board->ep_square;
   if (sq != SquareNone) {
      board->key ^= random_64(RandomEnPassant+square_file(sq));
      board->ep_square = SquareNone;
   }

   if (piece_is_pawn(piece) && abs(to-from) == 32) {
      pawn = piece_make_pawn(opp);
      if (board->square[to-1] == pawn || board->square[to+1] == pawn) {
         board->ep_square = sq = (from + to) / 2;
         board->key ^= random_64(RandomEnPassant+square_file(sq));
      }
   }

   // update ply number (captures are handled later)

   board->ply_nb++;
   if (piece_is_pawn(piece)) board->ply_nb = 0; // conversion

   // update move number

   if (me == Black) board->move_nb++;

   // castle

   if (colour_equal(board->square[to],me)) {

      int rank;
      int king_from, king_to;
      int rook_from, rook_to;
      int rook;

      rank = colour_is_white(me) ? Rank1 : Rank8;

      king_from = from;
      rook_from = to;

      if (to > from) { // h side
         king_to = square_make(FileG,rank);
         rook_to = square_make(FileF,rank);
      } else { // a side
         king_to = square_make(FileC,rank);
         rook_to = square_make(FileD,rank);
      }

      // remove the rook

      pos = board->pos[rook_from];
      ASSERT(pos>=0);

      rook = Rook64 | me; // HACK

      square_clear(board,rook_from,rook);

      // move the king

      square_move(board,king_from,king_to,piece);

      // put the rook back

      square_set(board,rook_to,rook,pos);

      ASSERT(board->key==hash_key(board));

      return;
   }

   // remove the captured piece

   if (piece_is_pawn(piece) && to == ep_square) {

      // en-passant capture

      sq = square_ep_dual(to);
      capture = board->square[sq];
      ASSERT(capture==piece_make_pawn(opp));

      square_clear(board,sq,capture);

      board->ply_nb = 0; // conversion

   } else {

      capture = board->square[to];

      if (capture != Empty) {

         // normal capture

         ASSERT(colour_equal(capture,opp));
         ASSERT(!piece_is_king(capture));

         square_clear(board,to,capture);

         board->ply_nb = 0; // conversion
      }
   }

   // move the piece

   if (move_is_promote(move)) {

      // promote

      square_clear(board,from,piece);
      piece = move_promote_hack(move) | me; // HACK
      square_set(board,to,piece,pos);

   } else {

      // normal move

      square_move(board,from,to,piece);
   }

   ASSERT(board->key==hash_key(board));
}

// square_clear()

static void square_clear(board_t * board, int square, int piece) {

   int pos, piece_12, colour;
   int sq, size;

   ASSERT(board!=NULL);
   ASSERT(square_is_ok(square));
   ASSERT(piece_is_ok(piece));

   // init

   pos = board->pos[square];
   ASSERT(pos>=0);

   colour = piece_colour(piece);
   piece_12 = piece_to_12(piece);

   // square

   ASSERT(board->square[square]==piece);
   board->square[square] = Empty;

   ASSERT(board->pos[square]==pos);
   board->pos[square] = -1; // not needed

   // piece list

   ASSERT(board->list_size[colour]>=2);
   size = --board->list_size[colour];
   ASSERT(pos<=size);

   if (pos != size) {

      sq = board->list[colour][size];
      ASSERT(square_is_ok(sq));
      ASSERT(sq!=square);

      ASSERT(board->pos[sq]==size);
      board->pos[sq] = pos;

      ASSERT(board->list[colour][pos]==square);
      board->list[colour][pos] = sq;
   }

   board->list[colour][size] = SquareNone;

   // material

   ASSERT(board->number[piece_12]>=1);
   board->number[piece_12]--;

   // hash key

   board->key ^= random_64(RandomPiece+piece_12*64+square_to_64(square));
}

// square_set()

static void square_set(board_t * board, int square, int piece, int pos) {

   int piece_12, colour;
   int sq, size;

   ASSERT(board!=NULL);
   ASSERT(square_is_ok(square));
   ASSERT(piece_is_ok(piece));
   ASSERT(pos>=0);

   // init

   colour = piece_colour(piece);
   piece_12 = piece_to_12(piece);

   // square

   ASSERT(board->square[square]==Empty);
   board->square[square] = piece;

   ASSERT(board->pos[square]==-1);
   board->pos[square] = pos;

   // piece list

   size = board->list_size[colour]++;
   ASSERT(board->list[colour][size]==SquareNone);
   ASSERT(pos<=size);

   if (pos != size) {

      sq = board->list[colour][pos];
      ASSERT(square_is_ok(sq));
      ASSERT(sq!=square);

      ASSERT(board->pos[sq]==pos);
      board->pos[sq] = size;

      ASSERT(board->list[colour][size]==SquareNone);
      board->list[colour][size] = sq;
   }

   board->list[colour][pos] = square;

   // material

   ASSERT(board->number[piece_12]<=8);
   board->number[piece_12]++;

   // hash key

   board->key ^= random_64(RandomPiece+piece_12*64+square_to_64(square));
}

// square_move()

static void square_move(board_t * board, int from, int to, int piece) {

   int colour, pos;
   int piece_index;

   ASSERT(board!=NULL);
   ASSERT(square_is_ok(from));
   ASSERT(square_is_ok(to));
   ASSERT(piece_is_ok(piece));

   // init

   colour = piece_colour(piece);

   pos = board->pos[from];
   ASSERT(pos>=0);

   // from

   ASSERT(board->square[from]==piece);
   board->square[from] = Empty;

   ASSERT(board->pos[from]==pos);
   board->pos[from] = -1; // not needed

   // to

   ASSERT(board->square[to]==Empty);
   board->square[to] = piece;

   ASSERT(board->pos[to]==-1);
   board->pos[to] = pos;

   // piece list

   ASSERT(board->list[colour][pos]==from);
   board->list[colour][pos] = to;

   // hash key

   piece_index = RandomPiece + piece_to_12(piece) * 64;

   board->key ^= random_64(piece_index+square_to_64(from))
               ^ random_64(piece_index+square_to_64(to));
}

// end of move_do.cpp

