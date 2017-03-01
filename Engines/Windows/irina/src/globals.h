#ifndef IRINA_GLOBALS_H
#define IRINA_GLOBALS_H

extern Board  board;
extern Bitmap BITSET[64];
extern Bitmap FREEWAY[64][64];
extern Bitmap WHITE_PAWN_ATTACKS[64];
extern Bitmap WHITE_PAWN_POSTATTACKS[64];
extern Bitmap WHITE_PAWN_MOVES[64];
extern Bitmap WHITE_PAWN_DOUBLE_MOVES[64];
extern Bitmap BLACK_PAWN_ATTACKS[64];
extern Bitmap BLACK_PAWN_POSTATTACKS[64];
extern Bitmap BLACK_PAWN_MOVES[64];
extern Bitmap BLACK_PAWN_DOUBLE_MOVES[64];
extern Bitmap KNIGHT_ATTACKS[64];
extern Bitmap KING_ATTACKS[64];
extern Bitmap LINE_ATTACKS[64];
extern Bitmap DIAG_ATTACKS[64];
extern Bitmap BLACK_SQUARES;
extern Bitmap WHITE_SQUARES;
extern int    PAWN_VALUE;
extern int    KNIGHT_VALUE;
extern int    BISHOP_VALUE;
extern int    ROOK_VALUE;
extern int    QUEEN_VALUE;
extern int    KING_VALUE;
extern int    CHECK_MATE;

extern Bitmap inodes;

extern Bitmap HASH_keys[64][16];
extern Bitmap HASH_ep[64];
extern Bitmap HASH_wk;
extern Bitmap HASH_wq;
extern Bitmap HASH_bk;
extern Bitmap HASH_bq;
extern Bitmap HASH_side;

extern char *POS_AH[64];
extern char NAMEPZ[16];

extern Bitmap BLACK_SQUARES;
extern Bitmap WHITE_SQUARES;

extern Bitmap FILA_MASK[64];
extern Bitmap COLUMNA_MASK[64];

extern int PAWNPOS_W[64];
extern int KNIGHTPOS_W[64];
extern int BISHOPPOS_W[64];
extern int ROOKPOS_W[64];
extern int QUEENPOS_W[64];
extern int KINGPOS_W[64];
extern int KINGPOS_ENDGAME_W[64];
extern int PAWNPOS_B[64];
extern int KNIGHTPOS_B[64];
extern int BISHOPPOS_B[64];
extern int ROOKPOS_B[64];
extern int QUEENPOS_B[64];
extern int KINGPOS_B[64];
extern int KINGPOS_ENDGAME_B[64];

extern int PENALTY_DOUBLED_PAWN;
extern int PENALTY_ISOLATED_PAWN;
extern int PENALTY_BACKWARD_PAWN;
extern int BONUS_PASSED_PAWN;
extern int BONUS_BISHOP_PAIR;
extern int BONUS_ROOK_BEHIND_PASSED_PAWN;
extern int BONUS_ROOK_ON_OPEN_FILE;
extern int BONUS_TWO_ROOKS_ON_OPEN_FILE;

extern int BONUS_PAWN_SHIELD_STRONG;
extern int BONUS_PAWN_SHIELD_WEAK;

extern int DISTANCE[64][64];
extern Bitmap PASSED_WHITE[64];
extern Bitmap PASSED_BLACK[64];
extern Bitmap ISOLATED_WHITE[64];
extern Bitmap ISOLATED_BLACK[64];
extern Bitmap BACKWARD_WHITE[64];
extern Bitmap BACKWARD_BLACK[64];
extern Bitmap KINGSHIELD_STRONG_W[64];
extern Bitmap KINGSHIELD_STRONG_B[64];
extern Bitmap KINGSHIELD_WEAK_W[64];
extern Bitmap KINGSHIELD_WEAK_B[64];

extern int PAWN_OWN_DISTANCE[8];
extern int PAWN_OPPONENT_DISTANCE[8];
extern int KNIGHT_DISTANCE[8];
extern int BISHOP_DISTANCE[8];
extern int ROOK_DISTANCE[8];
extern int QUEEN_DISTANCE[8];

extern int MIRROR[64];

typedef int (*FunctionEval)(void);
extern FunctionEval functionEval;

extern int min_time;
extern int max_time;

extern bool is_personality;

extern bool use_book;


extern int ST_PAWNPOS_W[64];
extern int ST_KNIGHTPOS_W[64];
extern int ST_BISHOPPOS_W[64];
extern int ST_ROOKPOS_W[64];
extern int ST_QUEENPOS_W[64];
extern int ST_KINGPOS_W[64];
extern int ST_KINGPOS_ENDGAME_W[64];
extern int ST_PAWNPOS_B[64];
extern int ST_KNIGHTPOS_B[64];
extern int ST_BISHOPPOS_B[64];
extern int ST_ROOKPOS_B[64];
extern int ST_QUEENPOS_B[64];
extern int ST_KINGPOS_B[64];
extern int ST_KINGPOS_ENDGAME_B[64];

extern int MS1BTABLE[256];

extern FILE * flog;



#endif
