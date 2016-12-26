
// piece.h

#ifndef PIECE_H
#define PIECE_H

// includes

#include "colour.h"
#include "util.h"

// constants

const int BlackPawnFlag = 1 << 2;
const int WhitePawnFlag = 1 << 3;
const int KnightFlag    = 1 << 4;
const int BishopFlag    = 1 << 5;
const int RookFlag      = 1 << 6;
const int KingFlag      = 1 << 7;

const int PawnFlags  = BlackPawnFlag | WhitePawnFlag;
const int QueenFlags = BishopFlag | RookFlag;

const int PieceNone64 = 0;
const int BlackPawn64 = BlackPawnFlag;
const int WhitePawn64 = WhitePawnFlag;
const int Knight64    = KnightFlag;
const int Bishop64    = BishopFlag;
const int Rook64      = RookFlag;
const int Queen64     = QueenFlags;
const int King64      = KingFlag;

const int PieceNone256   = 0;
const int BlackPawn256   = BlackPawn64 | Black;
const int WhitePawn256   = WhitePawn64 | White;
const int BlackKnight256 = Knight64    | Black;
const int WhiteKnight256 = Knight64    | White;
const int BlackBishop256 = Bishop64    | Black;
const int WhiteBishop256 = Bishop64    | White;
const int BlackRook256   = Rook64      | Black;
const int WhiteRook256   = Rook64      | White;
const int BlackQueen256  = Queen64     | Black;
const int WhiteQueen256  = Queen64     | White;
const int BlackKing256   = King64      | Black;
const int WhiteKing256   = King64      | White;

const int BlackPawn12   =  0;
const int WhitePawn12   =  1;
const int BlackKnight12 =  2;
const int WhiteKnight12 =  3;
const int BlackBishop12 =  4;
const int WhiteBishop12 =  5;
const int BlackRook12   =  6;
const int WhiteRook12   =  7;
const int BlackQueen12  =  8;
const int WhiteQueen12  =  9;
const int BlackKing12   = 10;
const int WhiteKing12   = 11;

// functions

extern void piece_init      ();

extern bool piece_is_ok     (int piece);

extern int  piece_make_pawn (int colour);
extern int  piece_pawn_opp  (int piece);

extern int  piece_colour    (int piece);
extern int  piece_type      (int piece);

extern bool piece_is_pawn   (int piece);
extern bool piece_is_knight (int piece);
extern bool piece_is_bishop (int piece);
extern bool piece_is_rook   (int piece);
extern bool piece_is_queen  (int piece);
extern bool piece_is_king   (int piece);

extern bool piece_is_slider (int piece);

extern int  piece_to_12     (int piece);
extern int  piece_from_12   (int piece);

extern int  piece_to_char   (int piece);
extern int  piece_from_char (int c);

extern bool char_is_piece   (int c);

#endif // !defined PIECE_H

// end of piece.h

