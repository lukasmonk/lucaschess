
// move_do.cpp

// includes

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "hash.h"
#include "move.h"
#include "move_do.h"
#include "pawn.h" // TODO: bit.h
#include "piece.h"
#include "pst.h"
#include "random.h"
#include "util.h"
#include "value.h"

// variables

static int CastleMask[SquareNb];

// prototypes

static void square_clear (board_t * board, int square, int piece, bool update);
static void square_set   (board_t * board, int square, int piece, int pos, bool update);
static void square_move  (board_t * board, int from, int to, int piece, bool update);

// functions

// move_do_init()

void move_do_init() {

   int sq;

   for (sq = 0; sq < SquareNb; sq++) CastleMask[sq] = 0xF;

   CastleMask[E1] &= ~FlagsWhiteKingCastle;
   CastleMask[H1] &= ~FlagsWhiteKingCastle;

   CastleMask[E1] &= ~FlagsWhiteQueenCastle;
   CastleMask[A1] &= ~FlagsWhiteQueenCastle;

   CastleMask[E8] &= ~FlagsBlackKingCastle;
   CastleMask[H8] &= ~FlagsBlackKingCastle;

   CastleMask[E8] &= ~FlagsBlackQueenCastle;
   CastleMask[A8] &= ~FlagsBlackQueenCastle;
}

// move_do()

void move_do(board_t * board, int move, undo_t * undo) {

   int me, opp;
   int from, to;
   int piece, pos, capture;
   int old_flags, new_flags;
   int delta;
   int sq;
   int pawn, rook;

   ASSERT(board!=NULL);
   ASSERT(move_is_ok(move));
   ASSERT(undo!=NULL);

   ASSERT(board_is_legal(board));

   // initialise undo

   undo->capture = false;

   undo->turn = board->turn;
   undo->flags = board->flags;
   undo->ep_square = board->ep_square;
   undo->ply_nb = board->ply_nb;

   undo->cap_sq = board->cap_sq;

   undo->opening = board->opening;
   undo->endgame = board->endgame;

   undo->key = board->key;
   undo->pawn_key = board->pawn_key;
   undo->material_key = board->material_key;

   // init

   me = board->turn;
   opp = COLOUR_OPP(me);

   from = MOVE_FROM(move);
   to = MOVE_TO(move);

   piece = board->square[from];
   ASSERT(COLOUR_IS(piece,me));

   // update key stack

   ASSERT(board->sp<StackSize);
   board->stack[board->sp++] = board->key;

   // update turn

   board->turn = opp;
   board->key ^= RANDOM_64(RandomTurn);

   // update castling rights

   old_flags = board->flags;
   new_flags = old_flags & CastleMask[from] & CastleMask[to];

   board->flags = new_flags;
   board->key ^= Castle64[new_flags^old_flags]; // HACK

   // update en-passant square

   if ((sq=board->ep_square) != SquareNone) {
      board->key ^= RANDOM_64(RandomEnPassant+SQUARE_FILE(sq)-FileA);
      board->ep_square = SquareNone;
   }

   if (PIECE_IS_PAWN(piece)) {

      delta = to - from;

      if (delta == +32 || delta == -32) {
         pawn = PAWN_MAKE(opp);
         if (board->square[to-1] == pawn || board->square[to+1] == pawn) {
            board->ep_square = (from + to) / 2;
            board->key ^= RANDOM_64(RandomEnPassant+SQUARE_FILE(to)-FileA);
         }
      }
   }

   // update move number (captures are handled later)

   board->ply_nb++;
   if (PIECE_IS_PAWN(piece)) board->ply_nb = 0; // conversion

   // update last square

   board->cap_sq = SquareNone;

   // remove the captured piece

   sq = to;
   if (MOVE_IS_EN_PASSANT(move)) sq = SQUARE_EP_DUAL(sq);

   if ((capture=board->square[sq]) != Empty) {

      ASSERT(COLOUR_IS(capture,opp));
      ASSERT(!PIECE_IS_KING(capture));

      undo->capture = true;
      undo->capture_square = sq;
      undo->capture_piece = capture;
      undo->capture_pos = board->pos[sq];

      square_clear(board,sq,capture,true);

      board->ply_nb = 0; // conversion
      board->cap_sq = to;
   }

   // move the piece

   if (MOVE_IS_PROMOTE(move)) {

      // promote

      undo->pawn_pos = board->pos[from];

      square_clear(board,from,piece,true);

      piece = move_promote(move);

      // insert the promote piece in MV order

      for (pos = board->piece_size[me]; pos > 0 && piece > board->square[board->piece[me][pos-1]]; pos--) // HACK
         ;

      square_set(board,to,piece,pos,true);

      board->cap_sq = to;

   } else {

      // normal move

      square_move(board,from,to,piece,true);
   }

   // move the rook in case of castling

   if (MOVE_IS_CASTLE(move)) {

      rook = Rook64 | COLOUR_FLAG(me); // HACK

      if (to == G1) {
         square_move(board,H1,F1,rook,true);
      } else if (to == C1) {
         square_move(board,A1,D1,rook,true);
      } else if (to == G8) {
         square_move(board,H8,F8,rook,true);
      } else if (to == C8) {
         square_move(board,A8,D8,rook,true);
      } else {
         ASSERT(false);
      }
   }

   // debug

   ASSERT(board_is_ok(board));
}

// move_undo()

void move_undo(board_t * board, int move, const undo_t * undo) {

   int me;
   int from, to;
   int piece, pos;
   int rook;

   ASSERT(board!=NULL);
   ASSERT(move_is_ok(move));
   ASSERT(undo!=NULL);

   // init

   me = undo->turn;

   from = MOVE_FROM(move);
   to = MOVE_TO(move);

   piece = board->square[to];
   ASSERT(COLOUR_IS(piece,me));

   // castle

   if (MOVE_IS_CASTLE(move)) {

      rook = Rook64 | COLOUR_FLAG(me); // HACK

      if (to == G1) {
         square_move(board,F1,H1,rook,false);
      } else if (to == C1) {
         square_move(board,D1,A1,rook,false);
      } else if (to == G8) {
         square_move(board,F8,H8,rook,false);
      } else if (to == C8) {
         square_move(board,D8,A8,rook,false);
      } else {
         ASSERT(false);
      }
   }

   // move the piece backward

   if (MOVE_IS_PROMOTE(move)) {

      // promote

      ASSERT(piece==move_promote(move));
      square_clear(board,to,piece,false);

      piece = PAWN_MAKE(me);
      pos = undo->pawn_pos;

      square_set(board,from,piece,pos,false);

   } else {

      // normal move

      square_move(board,to,from,piece,false);
   }

   // put the captured piece back

   if (undo->capture) {
      square_set(board,undo->capture_square,undo->capture_piece,undo->capture_pos,false);
   }

   // update board info

   board->turn = undo->turn;
   board->flags = undo->flags;
   board->ep_square = undo->ep_square;
   board->ply_nb = undo->ply_nb;

   board->cap_sq = undo->cap_sq;

   board->opening = undo->opening;
   board->endgame = undo->endgame;

   board->key = undo->key;
   board->pawn_key = undo->pawn_key;
   board->material_key = undo->material_key;

   // update key stack

   ASSERT(board->sp>0);
   board->sp--;

   // debug

   ASSERT(board_is_ok(board));
   ASSERT(board_is_legal(board));
}

// move_do_null()

void move_do_null(board_t * board, undo_t * undo) {

   int sq;

   ASSERT(board!=NULL);
   ASSERT(undo!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board));

   // initialise undo

   undo->turn = board->turn;
   undo->ep_square = board->ep_square;
   undo->ply_nb = board->ply_nb;
   undo->cap_sq = board->cap_sq;
   undo->key = board->key;

   // update key stack

   ASSERT(board->sp<StackSize);
   board->stack[board->sp++] = board->key;

   // update turn

   board->turn = COLOUR_OPP(board->turn);
   board->key ^= RANDOM_64(RandomTurn);

   // update en-passant square

   sq = board->ep_square;
   if (sq != SquareNone) {
      board->key ^= RANDOM_64(RandomEnPassant+SQUARE_FILE(sq)-FileA);
      board->ep_square = SquareNone;
   }

   // update move number

   board->ply_nb = 0; // HACK: null move is considered as a conversion

   // update last square

   board->cap_sq = SquareNone;

   // debug

   ASSERT(board_is_ok(board));
}

// move_undo_null()

void move_undo_null(board_t * board, const undo_t * undo) {

   ASSERT(board!=NULL);
   ASSERT(undo!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board));

   // update board info

   board->turn = undo->turn;
   board->ep_square = undo->ep_square;
   board->ply_nb = undo->ply_nb;
   board->cap_sq = undo->cap_sq;
   board->key = undo->key;

   // update key stack

   ASSERT(board->sp>0);
   board->sp--;

   // debug

   ASSERT(board_is_ok(board));
}

// square_clear()

static void square_clear(board_t * board, int square, int piece, bool update) {

   int pos, piece_12, colour;
   int sq;
   int i, size;
   int sq_64;
   uint64 hash_xor;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(square));
   ASSERT(piece_is_ok(piece));
   ASSERT(update==true||update==false);

   // init

   pos = board->pos[square];
   ASSERT(pos>=0);

   piece_12 = PIECE_TO_12(piece);
   colour = PIECE_COLOUR(piece);

   // square

   ASSERT(board->square[square]==piece);
   board->square[square] = Empty;

   // piece list

   if (!PIECE_IS_PAWN(piece)) {

      // init

      size = board->piece_size[colour];
      ASSERT(size>=1);

      // stable swap

      ASSERT(pos>=0&&pos<size);

      ASSERT(board->pos[square]==pos);
      board->pos[square] = -1;

      for (i = pos; i < size-1; i++) {

         sq = board->piece[colour][i+1];

         board->piece[colour][i] = sq;

         ASSERT(board->pos[sq]==i+1);
         board->pos[sq] = i;
      }

      // size

      size--;

      board->piece[colour][size] = SquareNone;
      board->piece_size[colour] = size;

   } else {

      // init

      size = board->pawn_size[colour];
      ASSERT(size>=1);

      // stable swap

      ASSERT(pos>=0&&pos<size);

      ASSERT(board->pos[square]==pos);
      board->pos[square] = -1;

      for (i = pos; i < size-1; i++) {

         sq = board->pawn[colour][i+1];

         board->pawn[colour][i] = sq;

         ASSERT(board->pos[sq]==i+1);
         board->pos[sq] = i;
      }

      // size

      size--;

      board->pawn[colour][size] = SquareNone;
      board->pawn_size[colour] = size;

      // pawn "bitboard"

      board->pawn_file[colour][SQUARE_FILE(square)] ^= BIT(PAWN_RANK(square,colour));
   }

   // material

   ASSERT(board->piece_nb>0);
   board->piece_nb--;

   ASSERT(board->number[piece_12]>0);
   board->number[piece_12]--;

   // update

   if (update) {

      // init

      sq_64 = SQUARE_TO_64(square);

      // PST

      board->opening -= PST(piece_12,sq_64,Opening);
      board->endgame -= PST(piece_12,sq_64,Endgame);

      // hash key

      hash_xor = RANDOM_64(RandomPiece+(piece_12^1)*64+sq_64); // HACK: ^1 for PolyGlot book

      board->key ^= hash_xor;
      if (PIECE_IS_PAWN(piece)) board->pawn_key ^= hash_xor;

      // material key

      board->material_key ^= RANDOM_64(piece_12*16+board->number[piece_12]);
   }
}

// square_set()

static void square_set(board_t * board, int square, int piece, int pos, bool update) {

   int piece_12, colour;
   int sq;
   int i, size;
   int sq_64;
   uint64 hash_xor;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(square));
   ASSERT(piece_is_ok(piece));
   ASSERT(pos>=0);
   ASSERT(update==true||update==false);

   // init

   piece_12 = PIECE_TO_12(piece);
   colour = PIECE_COLOUR(piece);

   // square

   ASSERT(board->square[square]==Empty);
   board->square[square] = piece;

   // piece list

   if (!PIECE_IS_PAWN(piece)) {

      // init

      size = board->piece_size[colour];
      ASSERT(size>=0);

      // size

      size++;

      board->piece[colour][size] = SquareNone;
      board->piece_size[colour] = size;

      // stable swap

      ASSERT(pos>=0&&pos<size);

      for (i = size-1; i > pos; i--) {

         sq = board->piece[colour][i-1];

         board->piece[colour][i] = sq;

         ASSERT(board->pos[sq]==i-1);
         board->pos[sq] = i;
      }

      board->piece[colour][pos] = square;

      ASSERT(board->pos[square]==-1);
      board->pos[square] = pos;

   } else {

      // init

      size = board->pawn_size[colour];
      ASSERT(size>=0);

      // size

      size++;

      board->pawn[colour][size] = SquareNone;
      board->pawn_size[colour] = size;

      // stable swap

      ASSERT(pos>=0&&pos<size);

      for (i = size-1; i > pos; i--) {

         sq = board->pawn[colour][i-1];

         board->pawn[colour][i] = sq;

         ASSERT(board->pos[sq]==i-1);
         board->pos[sq] = i;
      }

      board->pawn[colour][pos] = square;

      ASSERT(board->pos[square]==-1);
      board->pos[square] = pos;

      // pawn "bitboard"

      board->pawn_file[colour][SQUARE_FILE(square)] ^= BIT(PAWN_RANK(square,colour));
   }

   // material

   ASSERT(board->piece_nb<32);
   board->piece_nb++;;

   ASSERT(board->number[piece_12]<9);
   board->number[piece_12]++;

   // update

   if (update) {

      // init

      sq_64 = SQUARE_TO_64(square);

      // PST

      board->opening += PST(piece_12,sq_64,Opening);
      board->endgame += PST(piece_12,sq_64,Endgame);

      // hash key

      hash_xor = RANDOM_64(RandomPiece+(piece_12^1)*64+sq_64); // HACK: ^1 for PolyGlot book

      board->key ^= hash_xor;
      if (PIECE_IS_PAWN(piece)) board->pawn_key ^= hash_xor;

      // material key

      board->material_key ^= RANDOM_64(piece_12*16+(board->number[piece_12]-1));
   }
}

// square_move()

static void square_move(board_t * board, int from, int to, int piece, bool update) {

   int colour;
   int pos;
   int from_64, to_64;
   int piece_12;
   int piece_index;
   uint64 hash_xor;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(from));
   ASSERT(SQUARE_IS_OK(to));
   ASSERT(piece_is_ok(piece));
   ASSERT(update==true||update==false);

   // init

   colour = PIECE_COLOUR(piece);

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

   if (!PIECE_IS_PAWN(piece)) {

      ASSERT(board->piece[colour][pos]==from);
      board->piece[colour][pos] = to;

   } else {

      ASSERT(board->pawn[colour][pos]==from);
      board->pawn[colour][pos] = to;

      // pawn "bitboard"

      board->pawn_file[colour][SQUARE_FILE(from)] ^= BIT(PAWN_RANK(from,colour));
      board->pawn_file[colour][SQUARE_FILE(to)]   ^= BIT(PAWN_RANK(to,colour));
   }

   // update

   if (update) {

      // init

      from_64 = SQUARE_TO_64(from);
      to_64 = SQUARE_TO_64(to);
      piece_12 = PIECE_TO_12(piece);

      // PST

      board->opening += PST(piece_12,to_64,Opening) - PST(piece_12,from_64,Opening);
      board->endgame += PST(piece_12,to_64,Endgame) - PST(piece_12,from_64,Endgame);

      // hash key

      piece_index = RandomPiece + (piece_12^1) * 64; // HACK: ^1 for PolyGlot book

      hash_xor = RANDOM_64(piece_index+to_64) ^ RANDOM_64(piece_index+from_64);

      board->key ^= hash_xor;
      if (PIECE_IS_PAWN(piece)) board->pawn_key ^= hash_xor;
   }
}

// end of move_do.cpp

