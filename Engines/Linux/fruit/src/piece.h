
// piece.h

#ifndef PIECE_H
#define PIECE_H

// includes

#include "colour.h"
#include "util.h"

// constants

const int WhitePawnFlag = 1 << 2;
const int BlackPawnFlag = 1 << 3;
const int KnightFlag    = 1 << 4;
const int BishopFlag    = 1 << 5;
const int RookFlag      = 1 << 6;
const int KingFlag      = 1 << 7;

const int PawnFlags  = WhitePawnFlag | BlackPawnFlag;
const int QueenFlags = BishopFlag | RookFlag;

const int PieceNone64 = 0;
const int WhitePawn64 = WhitePawnFlag;
const int BlackPawn64 = BlackPawnFlag;
const int Knight64    = KnightFlag;
const int Bishop64    = BishopFlag;
const int Rook64      = RookFlag;
const int Queen64     = QueenFlags;
const int King64      = KingFlag;

const int PieceNone256   = 0;
const int WhitePawn256   = WhitePawn64 | WhiteFlag;
const int BlackPawn256   = BlackPawn64 | BlackFlag;
const int WhiteKnight256 = Knight64    | WhiteFlag;
const int BlackKnight256 = Knight64    | BlackFlag;
const int WhiteBishop256 = Bishop64    | WhiteFlag;
const int BlackBishop256 = Bishop64    | BlackFlag;
const int WhiteRook256   = Rook64      | WhiteFlag;
const int BlackRook256   = Rook64      | BlackFlag;
const int WhiteQueen256  = Queen64     | WhiteFlag;
const int BlackQueen256  = Queen64     | BlackFlag;
const int WhiteKing256   = King64      | WhiteFlag;
const int BlackKing256   = King64      | BlackFlag;
const int PieceNb        = 256;

const int WhitePawn12   =  0;
const int BlackPawn12   =  1;
const int WhiteKnight12 =  2;
const int BlackKnight12 =  3;
const int WhiteBishop12 =  4;
const int BlackBishop12 =  5;
const int WhiteRook12   =  6;
const int BlackRook12   =  7;
const int WhiteQueen12  =  8;
const int BlackQueen12  =  9;
const int WhiteKing12   = 10;
const int BlackKing12   = 11;

// macros

#define PAWN_MAKE(colour)        (PawnMake[colour])
#define PAWN_OPP(pawn)           ((pawn)^(WhitePawn256^BlackPawn256))

#define PIECE_COLOUR(piece)      (((piece)&3)-1)
#define PIECE_TYPE(piece)        ((piece)&~3)

#define PIECE_IS_PAWN(piece)     (((piece)&PawnFlags)!=0)
#define PIECE_IS_KNIGHT(piece)   (((piece)&KnightFlag)!=0)
#define PIECE_IS_BISHOP(piece)   (((piece)&QueenFlags)==BishopFlag)
#define PIECE_IS_ROOK(piece)     (((piece)&QueenFlags)==RookFlag)
#define PIECE_IS_QUEEN(piece)    (((piece)&QueenFlags)==QueenFlags)
#define PIECE_IS_KING(piece)     (((piece)&KingFlag)!=0)
#define PIECE_IS_SLIDER(piece)   (((piece)&QueenFlags)!=0)

#define PIECE_TO_12(piece)       (PieceTo12[piece])

#define PIECE_ORDER(piece)       (PieceOrder[piece])

#define PAWN_MOVE_INC(colour)    (PawnMoveInc[colour])
#define PIECE_INC(piece)         (PieceInc[piece])

// types

typedef int inc_t;

// "constants"

extern const int PawnMake[ColourNb];
extern const int PieceFrom12[12];

extern const inc_t PawnMoveInc[ColourNb];

extern const inc_t KnightInc[8+1];
extern const inc_t BishopInc[4+1];
extern const inc_t RookInc[4+1];
extern const inc_t QueenInc[8+1];
extern const inc_t KingInc[8+1];

// variables

extern int PieceTo12[PieceNb];
extern int PieceOrder[PieceNb];

extern const inc_t * PieceInc[PieceNb];

// functions

extern void piece_init      ();

extern bool piece_is_ok     (int piece);

extern int  piece_from_12   (int piece_12);

extern int  piece_to_char   (int piece);
extern int  piece_from_char (int c);

#endif // !defined PIECE_H

// end of piece.h

