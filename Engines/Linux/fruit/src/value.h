
// value.h

#ifndef VALUE_H
#define VALUE_H

// includes

#include "piece.h"
#include "util.h"

// constants

const int ValuePawn   = 100;   // was 100
const int ValueKnight = 325;   // was 300
const int ValueBishop = 325;   // was 300
const int ValueRook   = 500;   // was 500
const int ValueQueen  = 1000;  // was 900
const int ValueKing   = 10000; // was 10000

const int ValueNone    = -32767;
const int ValueDraw    = 0;
const int ValueMate    = 30000;
const int ValueInf     = ValueMate;
const int ValueEvalInf = ValueMate - 256; // handle mates upto 255 plies

// macros

#define VALUE_MATE(height) (-ValueMate+(height))
#define VALUE_PIECE(piece) (ValuePiece[piece])

// variables

extern int ValuePiece[PieceNb];

// functions

extern void value_init       ();

extern bool value_is_ok      (int value);
extern bool range_is_ok      (int min, int max);

extern bool value_is_mate    (int value);

extern int  value_to_trans   (int value, int height);
extern int  value_from_trans (int value, int height);

extern int  value_to_mate    (int value);

#endif // !defined VALUE_H

// end of value.h

