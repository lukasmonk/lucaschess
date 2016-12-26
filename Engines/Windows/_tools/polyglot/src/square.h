
// square.h

#ifndef SQUARE_H
#define SQUARE_H

// includes

#include "util.h"

// constants

const int SquareNb = 16 * 12;

const int FileA = 0;
const int FileB = 1;
const int FileC = 2;
const int FileD = 3;
const int FileE = 4;
const int FileF = 5;
const int FileG = 6;
const int FileH = 7;

const int Rank1 = 0;
const int Rank2 = 1;
const int Rank3 = 2;
const int Rank4 = 3;
const int Rank5 = 4;
const int Rank6 = 5;
const int Rank7 = 6;
const int Rank8 = 7;

const int SquareNone = 0;

const int A1=0x24, B1=0x25, C1=0x26, D1=0x27, E1=0x28, F1=0x29, G1=0x2A, H1=0x2B;
const int A2=0x34, B2=0x35, C2=0x36, D2=0x37, E2=0x38, F2=0x39, G2=0x3A, H2=0x3B;
const int A3=0x44, B3=0x45, C3=0x46, D3=0x47, E3=0x48, F3=0x49, G3=0x4A, H3=0x4B;
const int A4=0x54, B4=0x55, C4=0x56, D4=0x57, E4=0x58, F4=0x59, G4=0x5A, H4=0x5B;
const int A5=0x64, B5=0x65, C5=0x66, D5=0x67, E5=0x68, F5=0x69, G5=0x6A, H5=0x6B;
const int A6=0x74, B6=0x75, C6=0x76, D6=0x77, E6=0x78, F6=0x79, G6=0x7A, H6=0x7B;
const int A7=0x84, B7=0x85, C7=0x86, D7=0x87, E7=0x88, F7=0x89, G7=0x8A, H7=0x8B;
const int A8=0x94, B8=0x95, C8=0x96, D8=0x97, E8=0x98, F8=0x99, G8=0x9A, H8=0x9B;

const int Dark  = 0;
const int Light = 1;

// functions

extern void square_init        ();

extern bool square_is_ok       (int square);

extern int  square_make        (int file, int rank);

extern int  square_file        (int square);
extern int  square_rank        (int square);
extern int  square_side_rank   (int square, int colour);

extern int  square_from_64     (int square);
extern int  square_to_64       (int square);

extern bool square_is_promote  (int square);
extern int  square_ep_dual     (int square);

extern int  square_colour      (int square);

extern bool char_is_file       (int c);
extern bool char_is_rank       (int c);

extern int  file_from_char     (int c);
extern int  rank_from_char     (int c);

extern int  file_to_char       (int file);
extern int  rank_to_char       (int rank);

extern bool square_to_string   (int square, char string[], int size);
extern int  square_from_string (const char string[]);

#endif // !defined SQUARE_H

// end of square.h

