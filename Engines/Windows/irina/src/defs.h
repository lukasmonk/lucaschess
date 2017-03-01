#ifndef IRINA_DEFS_H
#define IRINA_DEFS_H
#include <stdio.h>

typedef signed char bool;
#define true 1
#define false 0

typedef signed char int8_t;
typedef unsigned char   uint8_t;
typedef short  int16_t;
typedef unsigned short  uint16_t;
typedef int  int32_t;
typedef unsigned   uint32_t;
typedef long long  int64_t;
typedef unsigned long long   uint64_t;


typedef uint64_t   Bitmap;

#define WHITE               false
#define BLACK               true

#define CASTLE_OO           1
#define CASTLE_OOO          2

#define CASTLE_OO_WHITE     1
#define CASTLE_OO_BLACK     2
#define CASTLE_OOO_WHITE    4
#define CASTLE_OOO_BLACK    8

#define EMPTY               0      //  0000
#define WHITE_PAWN          1      //  0001
#define WHITE_KING          2      //  0010
#define WHITE_KNIGHT        3      //  0011
#define WHITE_BISHOP        5      //  0101
#define WHITE_ROOK          6      //  0110
#define WHITE_QUEEN         7      //  0111
#define BLACK_PAWN          9      //  1001
#define BLACK_KING          10     //  1010
#define BLACK_KNIGHT        11     //  1011
#define BLACK_BISHOP        13     //  1101
#define BLACK_ROOK          14     //  1110
#define BLACK_QUEEN         15     //  1111

#define DRAWSCORE           0
#define MATESCORE           10000

#define IS_BLACK_PIECE(piece)    ((piece) & 24)

#define MAX_MOVES     16384   // Max number of moves that we can store (all plies)
#define MAX_PLY         512   // Max search depth
#define MAX_HISTORY    1024   // Max number of moves in the (game + search) line that we can store

typedef struct
{
   unsigned from      : 6;
   unsigned to        : 6;
   unsigned piece     : 4;
   unsigned capture   : 4;
   unsigned promotion : 4;
   unsigned is_ep     : 1;
   unsigned is_2p     : 1;
   unsigned is_castle : 2;
   unsigned none      : 4;
} Move;


typedef struct
{
   unsigned castle;
   unsigned ep;
   unsigned fifty;
   Move     move;
   Bitmap   hashkey;
} History;

typedef struct
{
   Bitmap   white_king, white_queens, white_rooks, white_bishops, white_knights, white_pawns;
   Bitmap   black_king, black_queens, black_rooks, black_bishops, black_knights, black_pawns;
   Bitmap   white_pieces, black_pieces, all_pieces;
   bool     color;
   unsigned castle;
   unsigned ep;
   unsigned fifty;
   unsigned pz[64];
   unsigned fullmove;
   Bitmap   hashkey;
   unsigned ply;
   unsigned idx_moves;
   Move     moves[MAX_MOVES];
   unsigned ply_moves[MAX_HISTORY];
   History  history[MAX_HISTORY];
} Board;

#define FILA(x)       ((x) / 8)
#define COLUMNA(x)    ((x) % 8)

#define A1          0
#define B1          1
#define C1          2
#define D1          3
#define E1          4
#define F1          5
#define G1          6
#define H1          7
#define A8          56
#define B8          57
#define C8          58
#define D8          59
#define E8          60
#define F8          61
#define G8          62
#define H8          63

#define BIGNUMBER    9999999
#define HASH_DEFAULT 32

#endif
