#ifndef WINGLET_EXTGLOBALS_H
#define WINGLET_EXTGLOBALS_H
 
#include <iostream>
#include "defines.h"
#include "board.h"
#include "hash.h"
 
extern char INIFILE[];
extern char PATHNAME[];
extern char CMD_BUFF[];
extern int CMD_BUFF_COUNT;
 
extern Board board;
 
extern const int A8; extern const int B8; extern const int C8; extern const int D8;
extern const int E8; extern const int F8; extern const int G8; extern const int H8;
extern const int A7; extern const int B7; extern const int C7; extern const int D7;
extern const int E7; extern const int F7; extern const int G7; extern const int H7;
extern const int A6; extern const int B6; extern const int C6; extern const int D6;
extern const int E6; extern const int F6; extern const int G6; extern const int H6;
extern const int A5; extern const int B5; extern const int C5; extern const int D5;
extern const int E5; extern const int F5; extern const int G5; extern const int H5;
extern const int A4; extern const int B4; extern const int C4; extern const int D4;
extern const int E4; extern const int F4; extern const int G4; extern const int H4;
extern const int A3; extern const int B3; extern const int C3; extern const int D3;
extern const int E3; extern const int F3; extern const int G3; extern const int H3;
extern const int A2; extern const int B2; extern const int C2; extern const int D2;
extern const int E2; extern const int F2; extern const int G2; extern const int H2;
extern const int A1; extern const int B1; extern const int C1; extern const int D1;
extern const int E1; extern const int F1; extern const int G1; extern const int H1;
 
extern const char* SQUARENAME[];
 
extern const int FILES[];
extern const int RANKS[];
 
extern const unsigned char WHITE_MOVE; 
extern const unsigned char BLACK_MOVE; 
 
extern const unsigned char EMPTY;
extern const unsigned char WHITE_KNIGHT;
extern const unsigned char WHITE_PAWN;
extern const unsigned char WHITE_KING;
extern const unsigned char WHITE_BISHOP;
extern const unsigned char WHITE_ROOK;
extern const unsigned char WHITE_QUEEN;
extern const unsigned char BLACK_KNIGHT;
extern const unsigned char BLACK_PAWN;
extern const unsigned char BLACK_KING;
extern const unsigned char BLACK_BISHOP;
extern const unsigned char BLACK_ROOK;
extern const unsigned char BLACK_QUEEN;
 
extern const char* PIECENAMES[];
extern const char* PIECECHARS[];
 
extern BitMap BITSET[];
extern int BOARDINDEX[9][9];
 
extern const int PAWN_VALUE;
extern const int KNIGHT_VALUE;
extern const int BISHOP_VALUE;
extern const int ROOK_VALUE;
extern const int QUEEN_VALUE;
extern const int KING_VALUE;
extern const int CHECK_MATE;
extern int PIECEVALUES[];

extern int MS1BTABLE[];
 
extern BitMap WHITE_PAWN_ATTACKS[];
extern BitMap WHITE_PAWN_MOVES[];
extern BitMap WHITE_PAWN_DOUBLE_MOVES[];
extern BitMap BLACK_PAWN_ATTACKS[];
extern BitMap BLACK_PAWN_MOVES[];
extern BitMap BLACK_PAWN_DOUBLE_MOVES[];
extern BitMap KNIGHT_ATTACKS[];
extern BitMap KING_ATTACKS[];
extern BitMap RANK_ATTACKS[64][64];
extern BitMap FILE_ATTACKS[64][64];
extern BitMap DIAGA8H1_ATTACKS[64][64];
extern BitMap DIAGA1H8_ATTACKS[64][64];
 
extern const int RANKSHIFT[];
extern const BitMap _FILEMAGICS[];
extern const BitMap _DIAGA8H1MAGICS[];
extern const BitMap _DIAGA1H8MAGICS[];
 
extern BitMap RANKMASK[];
extern BitMap FILEMAGIC[];
extern BitMap FILEMASK[];
extern BitMap DIAGA8H1MASK[];
extern BitMap DIAGA8H1MAGIC[];
extern BitMap DIAGA1H8MASK[];
extern BitMap DIAGA1H8MAGIC[];
 
extern unsigned char GEN_SLIDING_ATTACKS[8][64];
 
extern unsigned char CANCASTLEOO;
extern unsigned char CANCASTLEOOO;
extern BitMap maskEG[];
extern BitMap maskFG[];
extern BitMap maskBD[];
extern BitMap maskCE[];
extern unsigned int WHITE_OOO_CASTL;
extern unsigned int BLACK_OOO_CASTL;
extern unsigned int WHITE_OO_CASTL;
extern unsigned int BLACK_OO_CASTL;

extern int ICAPT;
extern int IEP;
extern int IPROM;
extern int ICASTLOO;
extern int ICASTLOOO;
extern int ICHECK;

extern int PENALTY_DOUBLED_PAWN;
extern int PENALTY_ISOLATED_PAWN;
extern int PENALTY_BACKWARD_PAWN;
extern int BONUS_PASSED_PAWN;
extern int BONUS_BISHOP_PAIR;
extern int BONUS_ROOK_BEHIND_PASSED_PAWN;
extern int BONUS_ROOK_ON_OPEN_FILE;
extern int BONUS_PAWN_SHIELD_STRONG;
extern int BONUS_PAWN_SHIELD_WEAK;
extern int PAWN_OWN_DISTANCE[];
extern int PAWN_OPPONENT_DISTANCE[];
extern int KNIGHT_DISTANCE[];
extern int BISHOP_DISTANCE[];
extern int ROOK_DISTANCE[];
extern int QUEEN_DISTANCE[];
extern int PAWNPOS_W[];
extern int KNIGHTPOS_W[];
extern int BISHOPPOS_W[];
extern int ROOKPOS_W[];
extern int QUEENPOS_W[];
extern int KINGPOS_W[];
extern int KINGPOS_ENDGAME_W[];
extern int MIRROR[];
extern int DISTANCE[64][64];
extern int PAWNPOS_B[];
extern int KNIGHTPOS_B[];
extern int BISHOPPOS_B[];
extern int ROOKPOS_B[];
extern int QUEENPOS_B[];
extern int KINGPOS_B[];
extern int KINGPOS_ENDGAME_B[];
extern BitMap PASSED_WHITE[];
extern BitMap PASSED_BLACK[];
extern BitMap ISOLATED_WHITE[];
extern BitMap ISOLATED_BLACK[];
extern BitMap BACKWARD_WHITE[];
extern BitMap BACKWARD_BLACK[];
extern BitMap KINGSHIELD_STRONG_W[];
extern BitMap KINGSHIELD_STRONG_B[];
extern BitMap KINGSHIELD_WEAK_W[];
extern BitMap KINGSHIELD_WEAK_B[];
extern BitMap WHITE_SQUARES;
extern BitMap BLACK_SQUARES;

extern int LARGE_NUMBER;
extern int CHECKMATESCORE;
extern int STALEMATESCORE;
extern int DRAWSCORE;
extern Move NOMOVE;
extern HashKeys KEY;

extern int OFFSET;
extern int MINCAPTVAL;
extern int WEST;
extern int NORTHWEST;
extern int NORTH;
extern int NORTHEAST;
extern int EAST;
extern int SOUTHEAST;
extern int SOUTH;
extern int SOUTHWEST;
extern BitMap RAY_W[];
extern BitMap RAY_NW[];
extern BitMap RAY_N[];
extern BitMap RAY_NE[];
extern BitMap RAY_E[];
extern BitMap RAY_SE[];
extern BitMap RAY_S[];
extern BitMap RAY_SW[];
extern int HEADINGS[64][64];

extern int NULLMOVE_REDUCTION;
extern int NULLMOVE_LIMIT;
extern int UPDATEINTERVAL;
extern double STOPFRAC;

extern int TO_CONSOLE;

extern BOOLTYPE XB_MODE;
extern BOOLTYPE XB_PONDER;
extern BOOLTYPE XB_POST;
extern BOOLTYPE XB_DO_PENDING;
extern BOOLTYPE XB_NO_TIME_LIMIT;
extern unsigned char XB_NONE;
extern unsigned char XB_ANALYZE;
extern unsigned char XB_COMPUTER_SIDE;
extern int XB_MIN;
extern int XB_SEC;
extern int XB_MPS;
extern int XB_INC;
extern int XB_OTIM;
extern int XB_CTIM;

#endif