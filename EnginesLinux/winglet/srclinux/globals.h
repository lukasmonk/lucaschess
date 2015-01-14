#ifndef WINGLET_GLOBALS_H
#define WINGLET_GLOBALS_H
 
#include <iostream>
#include "defines.h"
#include "board.h"
#include "hash.h"
 
char INIFILE[80];
char PATHNAME[80];
char CMD_BUFF[MAX_CMD_BUFF];
int CMD_BUFF_COUNT = 0;
 
Board board;

extern const int A8 = 56; extern const int B8 = 57; extern const int C8 = 58; extern const int D8 = 59;
extern const int E8 = 60; extern const int F8 = 61; extern const int G8 = 62; extern const int H8 = 63;
extern const int A7 = 48; extern const int B7 = 49; extern const int C7 = 50; extern const int D7 = 51;
extern const int E7 = 52; extern const int F7 = 53; extern const int G7 = 54; extern const int H7 = 55;
extern const int A6 = 40; extern const int B6 = 41; extern const int C6 = 42; extern const int D6 = 43;
extern const int E6 = 44; extern const int F6 = 45; extern const int G6 = 46; extern const int H6 = 47;
extern const int A5 = 32; extern const int B5 = 33; extern const int C5 = 34; extern const int D5 = 35;
extern const int E5 = 36; extern const int F5 = 37; extern const int G5 = 38; extern const int H5 = 39;
extern const int A4 = 24; extern const int B4 = 25; extern const int C4 = 26; extern const int D4 = 27;
extern const int E4 = 28; extern const int F4 = 29; extern const int G4 = 30; extern const int H4 = 31;
extern const int A3 = 16; extern const int B3 = 17; extern const int C3 = 18; extern const int D3 = 19;
extern const int E3 = 20; extern const int F3 = 21; extern const int G3 = 22; extern const int H3 = 23;
extern const int A2 =  8; extern const int B2 =  9; extern const int C2 = 10; extern const int D2 = 11;
extern const int E2 = 12; extern const int F2 = 13; extern const int G2 = 14; extern const int H2 = 15;
extern const int A1 =  0; extern const int B1 =  1; extern const int C1 =  2; extern const int D1 =  3;
extern const int E1 =  4; extern const int F1 =  5; extern const int G1 =  6; extern const int H1 =  7;
 
const char* SQUARENAME[64] = {"a1","b1","c1","d1","e1","f1","g1","h1",
                              "a2","b2","c2","d2","e2","f2","g2","h2",
                              "a3","b3","c3","d3","e3","f3","g3","h3",
                              "a4","b4","c4","d4","e4","f4","g4","h4",
                              "a5","b5","c5","d5","e5","f5","g5","h5",
                              "a6","b6","c6","d6","e6","f6","g6","h6",
                              "a7","b7","c7","d7","e7","f7","g7","h7",
                              "a8","b8","c8","d8","e8","f8","g8","h8"};
 
extern const int FILES[64] = {
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8,
       1, 2, 3, 4, 5, 6, 7, 8
};
 
extern const int RANKS[64] = {
       1, 1, 1, 1, 1, 1, 1, 1,
       2, 2, 2, 2, 2, 2, 2, 2,
       3, 3, 3, 3, 3, 3, 3, 3,
       4, 4, 4, 4, 4, 4, 4, 4,
       5, 5, 5, 5, 5, 5, 5, 5,
       6, 6, 6, 6, 6, 6, 6, 6,
       7, 7, 7, 7, 7, 7, 7, 7,
       8, 8, 8, 8, 8, 8, 8, 8
};
 
// Identifier of next move:
extern const unsigned char WHITE_MOVE  = 0; 
extern const unsigned char BLACK_MOVE  = 1; 
 
// Piece identifiers, 4 bits each.
// Usefull bitwise properties of this numbering scheme:
// white = 0..., black = 1..., sliding = .1.., nonsliding = .0..
// rank/file sliding pieces = .11., diagonally sliding pieces = .1.1
// pawns and kings (without color bits), are < 3
// major pieces (without color bits set), are > 5
// minor and major pieces (without color bits set), are > 2
extern const unsigned char EMPTY = 0;                //  0000
extern const unsigned char WHITE_PAWN = 1;           //  0001
extern const unsigned char WHITE_KING = 2;           //  0010
extern const unsigned char WHITE_KNIGHT = 3;         //  0011
extern const unsigned char WHITE_BISHOP =  5;        //  0101
extern const unsigned char WHITE_ROOK = 6;           //  0110
extern const unsigned char WHITE_QUEEN = 7;          //  0111
extern const unsigned char BLACK_PAWN = 9;           //  1001
extern const unsigned char BLACK_KING = 10;          //  1010
extern const unsigned char BLACK_KNIGHT = 11;        //  1011
extern const unsigned char BLACK_BISHOP = 13;        //  1101
extern const unsigned char BLACK_ROOK = 14;          //  1110
extern const unsigned char BLACK_QUEEN = 15;         //  1111
 
const char* PIECENAMES[16] = {"  ","P ","K ","N ","  ","B ","R ","Q ",
                              "  ","P*","K*","N*","  ","B*","R*","Q*"};
const char* PIECECHARS[16] = {" "," ","K","N"," ","B","R","Q"," "," ","K","N"," ","B","R","Q"};
 
BitMap BITSET[64];
int BOARDINDEX[9][9]; // index 0 is not used, only 1..8.
 
// Value of material, in centipawns:
extern const int PAWN_VALUE = 100;
extern const int KNIGHT_VALUE = 300;
extern const int BISHOP_VALUE = 300;
extern const int ROOK_VALUE = 500;
extern const int QUEEN_VALUE = 900;
extern const int KING_VALUE = 9999;
extern const int CHECK_MATE = KING_VALUE;
// used for MVV/LVA and SEE:
int PIECEVALUES[16];
 
// used in Eugene Nalimov's bitScanReverse
int MS1BTABLE[256];
 
// Attack tables:
BitMap WHITE_PAWN_ATTACKS[64];
BitMap WHITE_PAWN_MOVES[64];
BitMap WHITE_PAWN_DOUBLE_MOVES[64];
BitMap BLACK_PAWN_ATTACKS[64];
BitMap BLACK_PAWN_MOVES[64];
BitMap BLACK_PAWN_DOUBLE_MOVES[64];
BitMap KNIGHT_ATTACKS[64];
BitMap KING_ATTACKS[64];
BitMap RANK_ATTACKS[64][64];      // 32KB
BitMap FILE_ATTACKS[64][64];      // 32KB
BitMap DIAGA8H1_ATTACKS[64][64];  // 32KB
BitMap DIAGA1H8_ATTACKS[64][64];  // 32KB
 
// Move generator shift for ranks:
extern const int RANKSHIFT[64] = {
        1,  1,  1,  1,  1,  1,  1,  1,
        9,  9,  9,  9,  9,  9,  9,  9,
       17, 17, 17, 17, 17, 17, 17, 17,  
       25, 25, 25, 25, 25, 25, 25, 25,
       33, 33, 33, 33, 33, 33, 33, 33,
       41, 41, 41, 41, 41, 41, 41, 41,
       49, 49, 49, 49, 49, 49, 49, 49,
       57, 57, 57, 57, 57, 57, 57, 57
};
 
// Move generator magic multiplication numbers for files:
extern const BitMap _FILEMAGICS[8] = {
       0x8040201008040200,
       0x4020100804020100,
       0x2010080402010080,
       0x1008040201008040,
       0x0804020100804020,
       0x0402010080402010,
       0x0201008040201008,
       0x0100804020100804
};
 
// Move generator magic multiplication numbers for diagonals:
extern const BitMap _DIAGA8H1MAGICS[15] = {
       0x0,
       0x0,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0080808080808080,
       0x0040404040404040,
       0x0020202020202020,
       0x0010101010101010,
       0x0008080808080808,
       0x0,
       0x0
};
 
// Move generator magic multiplication numbers for diagonals:
extern const BitMap _DIAGA1H8MAGICS[15] = {
       0x0,
       0x0,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x0101010101010100,
       0x8080808080808000,
       0x4040404040400000,
       0x2020202020000000,
       0x1010101000000000,
       0x0808080000000000,
       0x0,
       0x0
};
 
// Move generator 6-bit masking and magic multiplication numbers:
BitMap RANKMASK[64];
BitMap FILEMASK[64];
BitMap FILEMAGIC[64];
BitMap DIAGA8H1MASK[64];
BitMap DIAGA8H1MAGIC[64];
BitMap DIAGA1H8MASK[64];
BitMap DIAGA1H8MAGIC[64];
 
// We use one generalized sliding attacks array: [8 squares][64 states]
// the unsigned char (=8 bits) contains the attacks for a rank, file or diagonal
unsigned char GEN_SLIDING_ATTACKS[8][64];
 
// Used for castling:
unsigned char CANCASTLEOO = 1;
unsigned char CANCASTLEOOO = 2;
BitMap maskEG[2];
BitMap maskFG[2];
BitMap maskBD[2];
BitMap maskCE[2];
unsigned int WHITE_OOO_CASTL;
unsigned int BLACK_OOO_CASTL;
unsigned int WHITE_OO_CASTL;
unsigned int BLACK_OO_CASTL;

int ICAPT;
int IEP;
int IPROM;
int ICASTLOO;
int ICASTLOOO;
int ICHECK;

// Evaluation scores start here, all scores are in centipawns.
// If there are scoring data that is supposed to be symmetrical (i.e. same for BLACK & WHITE),
// then only the data for WHITE is supplied, and data for BLACK is calculated in dataInit().
// This is done to make data entry easier, eliminate typos and guarantuee symmetry.
//
// MIRRORED:
// Some scoring arrays are supplied MIRRORED, i.e. starting with the last rank (see the comments below).
// They are mirrored back in the right order in dataInit().
// This is only done to make data entry easier, because you can enter the scoring data as if you're
// looking at the chess board from White's point of perspective.
 
int PENALTY_DOUBLED_PAWN          = 10;
int PENALTY_ISOLATED_PAWN         = 20;
int PENALTY_BACKWARD_PAWN         =  8;
int BONUS_PASSED_PAWN             = 20;
int BONUS_BISHOP_PAIR             = 10;
int BONUS_ROOK_BEHIND_PASSED_PAWN = 20;
int BONUS_ROOK_ON_OPEN_FILE       = 20;
int BONUS_TWO_ROOKS_ON_OPEN_FILE  = 20;
 
int BONUS_PAWN_SHIELD_STRONG = 9;
int BONUS_PAWN_SHIELD_WEAK = 4;
 
int PAWN_OWN_DISTANCE[8] =           { 0,   8,  4,  2,  0,  0,  0,  0 };
int PAWN_OPPONENT_DISTANCE[8] =      { 0,   2,  1,  0,  0,  0,  0,  0 };
int KNIGHT_DISTANCE[8] =             { 0,   4,  4,  0,  0,  0,  0,  0 };
int BISHOP_DISTANCE[8] =             { 0,   5,  4,  3,  2,  1,  0,  0 };
int ROOK_DISTANCE[8] =               { 0,   7,  5,  4,  3,  0,  0,  0 };
int QUEEN_DISTANCE[8] =              { 0,  10,  8,  5,  4,  0,  0,  0 };
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int PAWNPOS_W[64] = {
         0,   0,   0,   0,   0,   0,   0,   0,
         5,  10,  15,  20,  20,  15,  10,   5,
         4,   8,  12,  16,  16,  12,   8,   4,
         3,   6,   9,  12,  12,   9,   6,   3,
         2,   4,   6,   8,   8,   6,   4,   2,
         1,   2,   3, -10, -10,   3,   2,   1,
         0,   0,   0, -40, -40,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0
};
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int KNIGHTPOS_W[64] = {
       -10, -10, -10, -10, -10, -10, -10, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10, -30, -10, -10, -10, -10, -30, -10
};
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int BISHOPPOS_W[64] = {
       -10, -10, -10, -10, -10, -10, -10, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10, -10, -20, -10, -10, -20, -10, -10
};
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int ROOKPOS_W[64] = {
      0,  0,  0,  0,   0,  0,  0,   0,
     15, 15, 15, 15,  15, 15, 15,  15,
      0,  0,  0,  0,   0,  0,  0,   0,
      0,  0,  0,  0,   0,  0,  0,   0,
      0,  0,  0,  0,   0,  0,  0,   0,
      0,  0,  0,  0,   0,  0,  0,   0,
      0,  0,  0,  0,   0,  0,  0,   0,
    -10,  0,  0, 10,  10,  0,  0, -10
};
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int QUEENPOS_W[64] = {
       -10, -10, -10, -10, -10, -10, -10, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   5,  10,  10,   5,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10, -10, -20, -10, -10, -20, -10, -10
};
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int KINGPOS_W[64]  = {
       -40, -40, -40, -40, -40, -40, -40, -40,
       -40, -40, -40, -40, -40, -40, -40, -40,
       -40, -40, -40, -40, -40, -40, -40, -40,
       -40, -40, -40, -40, -40, -40, -40, -40,
       -40, -40, -40, -40, -40, -40, -40, -40,
       -40, -40, -40, -40, -40, -40, -40, -40,
       -20, -20, -20, -20, -20, -20, -20, -20,
         0,  20,  40, -20,   0, -20,  40,  20
};
 
// *** This array is MIRRORED                          ***
// *** You can enter the scoring data as if you're     ***
// *** looking at the chess board from white's point   ***
// *** of perspective. Lower left corner is square a1: ***
int KINGPOS_ENDGAME_W[64] = {
         0,  10,  20,  30,  30,  20,  10,   0,
        10,  20,  30,  40,  40,  30,  20,  10,
        20,  30,  40,  50,  50,  40,  30,  20,
        30,  40,  50,  60,  60,  50,  40,  30,
        30,  40,  50,  60,  60,  50,  40,  30,
        20,  30,  40,  50,  50,  40,  30,  20,
        10,  20,  30,  40,  40,  30,  20,  10,
         0,  10,  20,  30,  30,  20,  10,   0
}; 
 
int MIRROR[64] = {
        56,  57,  58,  59,  60,  61,  62,  63,
        48,  49,  50,  51,  52,  53,  54,  55,
        40,  41,  42,  43,  44,  45,  46,  47,
        32,  33,  34,  35,  36,  37,  38,  39,
        24,  25,  26,  27,  28,  29,  30,  31,
        16,  17,  18,  19,  20,  21,  22,  23,
         8,   9,  10,  11,  12,  13,  14,  15,  
         0,   1,   2,   3,   4,   5,   6,   7
};
 
int DISTANCE[64][64];
int PAWNPOS_B[64];
int KNIGHTPOS_B[64];
int BISHOPPOS_B[64];
int ROOKPOS_B[64];
int QUEENPOS_B[64];
int KINGPOS_B[64];
int KINGPOS_ENDGAME_B[64];
BitMap PASSED_WHITE[64];
BitMap PASSED_BLACK[64];
BitMap ISOLATED_WHITE[64];
BitMap ISOLATED_BLACK[64];
BitMap BACKWARD_WHITE[64];
BitMap BACKWARD_BLACK[64];
BitMap KINGSHIELD_STRONG_W[64];
BitMap KINGSHIELD_STRONG_B[64];
BitMap KINGSHIELD_WEAK_W[64];
BitMap KINGSHIELD_WEAK_B[64];
BitMap WHITE_SQUARES;
BitMap BLACK_SQUARES;

//
// Search parameters start here:
//
int LARGE_NUMBER = KING_VALUE + 1;
int CHECKMATESCORE = KING_VALUE;
int STALEMATESCORE = 0;
int DRAWSCORE = 0;
Move NOMOVE;
HashKeys KEY;

// variables for quiescence and SEE:
int OFFSET = 128;
int MINCAPTVAL = 1;
int WEST = -1;
int NORTHWEST = +7;
int NORTH = +8;
int NORTHEAST = +9;
int EAST = +1;
int SOUTHEAST = -7;
int SOUTH = -8;
int SOUTHWEST = -9;
BitMap RAY_W[64];
BitMap RAY_NW[64];
BitMap RAY_N[64];
BitMap RAY_NE[64];
BitMap RAY_E[64];
BitMap RAY_SE[64];
BitMap RAY_S[64];
BitMap RAY_SW[64];
int HEADINGS[64][64];

// Nullmove parameters:
int NULLMOVE_REDUCTION = 4;  // equivalent to R=3
// Material (excluding pawns) must be > NULLMOVE_LIMIT to allow a nullmove:
// Setting this value to 299 ensures that null moves are not done if the 
// side to move has no major or minor pieces
int NULLMOVE_LIMIT = KNIGHT_VALUE - 1;

// peek interval in searched node units
int UPDATEINTERVAL = 100000; 
// don't start a new iteration if STOPFRAC fraction of our max search time has passed:
double STOPFRAC = 0.6;

// keep track of stdout (writing to a file or to the console):
int TO_CONSOLE;

// Winboard constants & variables:
BOOLTYPE XB_MODE;
BOOLTYPE XB_PONDER;
BOOLTYPE XB_POST;
BOOLTYPE XB_DO_PENDING;
BOOLTYPE XB_NO_TIME_LIMIT;
unsigned char XB_NONE = 2;
unsigned char XB_ANALYZE = 3;
unsigned char XB_COMPUTER_SIDE;
int XB_MIN;
int XB_SEC;
int XB_MPS;
int XB_INC;
int XB_OTIM;
int XB_CTIM;


#endif