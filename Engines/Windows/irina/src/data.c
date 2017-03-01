#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "protos.h"

Board board;
Bitmap BITSET[64];
Bitmap FREEWAY[64][64];
Bitmap WHITE_PAWN_ATTACKS[64];
Bitmap WHITE_PAWN_POSTATTACKS[64];
Bitmap WHITE_PAWN_MOVES[64];
Bitmap WHITE_PAWN_DOUBLE_MOVES[64];
Bitmap BLACK_PAWN_ATTACKS[64];
Bitmap BLACK_PAWN_POSTATTACKS[64];
Bitmap BLACK_PAWN_MOVES[64];
Bitmap BLACK_PAWN_DOUBLE_MOVES[64];
Bitmap KNIGHT_ATTACKS[64];
Bitmap KING_ATTACKS[64];
Bitmap LINE_ATTACKS[64];
Bitmap DIAG_ATTACKS[64];
int PAWN_VALUE = 100;
int KNIGHT_VALUE = 300;
int BISHOP_VALUE = 300;
int ROOK_VALUE = 500;
int QUEEN_VALUE = 900;
int KING_VALUE = 9999;
int CHECK_MATE = 9999;

int DISTANCE[64][64];

// used in Eugene Nalimov's bitScanReverse
int MS1BTABLE[256];

Bitmap BLACK_SQUARES;
Bitmap WHITE_SQUARES;

Bitmap FILA_MASK[64];
Bitmap COLUMNA_MASK[64];

Bitmap inodes;


char *POS_AH[64] =
{
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

char NAMEPZ[16] =
{
    '\0', 'P', 'K', 'N', '\0', 'B', 'R', 'Q',
    '\0', 'p', 'k', 'n', '\0', 'b', 'r', 'q'
};

int PAWNPOS_W[64] =
{
    0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
    5,   5,  10,  25,  25,  10,   5,   5,
    0,   0,   0,  20,  20,   0,   0,   0,
    5,  -5, -10,   0,   0, -10,  -5,   5,
    5,  10,  10, -20, -20,  10,  10,   5,
    0,   0,   0,   0,   0,   0,   0,   0
};


int KNIGHTPOS_W[64] =
{
    -50, -40, -30, -30, -30, -30, -40, -50,
    -40, -20,   0,   0,   0,   0, -20, -40,
    -30,   0,  10,  15,  15,  10,   0, -30,
    -30,   5,  15,  20,  20,  15,   5, -30,
    -30,   0,  15,  20,  20,  15,   0, -30,
    -30,   5,  10,  15,  15,  10,   5, -30,
    -40, -20,   0,   5,   5,   0, -20, -40,
    -50, -40, -30, -30, -30, -30, -40, -50,
};

int BISHOPPOS_W[64] =
{
    -20, -10, -10, -10, -10, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,  10,  10,   5,   0, -10,
    -10,   5,   5,  10,  10,   5,   5, -10,
    -10,   0,  10,  10,  10,  10,   0, -10,
    -10,  10,  10,  10,  10,  10,  10, -10,
    -10,   5,   0,   0,   0,   0,   5, -10,
    -20, -10, -10, -10, -10, -10, -10, -20,
};

int ROOKPOS_W[64] =
{
    0,   0,   0,   0,   0,   0,   0,   0,
    5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    0,   0,   0,   5,   5,   0,   0,   0
};

int QUEENPOS_W[64] =
{
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,   5,   5,   5,   5,   0, -10,
    -5,   0,   5,   5,   5,   5,   0,  -5,
    0,   0,   5,   5,   5,   5,   0,  -5,
    -10,   5,   5,   5,   5,   5,   0, -10,
    -10,   0,   5,   0,   0,   0,   0, -10,
    -20, -10, -10,  -5,  -5, -10, -10, -20
};

int KINGPOS_W[64] =
{
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -30, -40, -40, -50, -50, -40, -40, -30,
    -20, -30, -30, -40, -40, -30, -30, -20,
    -10, -20, -20, -20, -20, -20, -20, -10,
    20,  20,   0,   0,   0,   0,  20,  20,
    20,  30,  10,   0,   0,  10,  30,  20
};

int KINGPOS_ENDGAME_W[64] =
{
    -50, -40, -30, -20, -20, -30, -40, -50,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -50, -30, -30, -30, -30, -30, -30, -50
};

int MIRROR[64] =
{
    56, 57, 58, 59, 60, 61, 62, 63,
    48, 49, 50, 51, 52, 53, 54, 55,
    40, 41, 42, 43, 44, 45, 46, 47,
    32, 33, 34, 35, 36, 37, 38, 39,
    24, 25, 26, 27, 28, 29, 30, 31,
    16, 17, 18, 19, 20, 21, 22, 23,
    8,  9, 10, 11, 12, 13, 14, 15,
    0,  1,  2,  3,  4,  5,  6,  7
};

int PAWNPOS_B[64];
int KNIGHTPOS_B[64];
int BISHOPPOS_B[64];
int ROOKPOS_B[64];
int QUEENPOS_B[64];
int KINGPOS_B[64];
int KINGPOS_ENDGAME_B[64];

int PENALTY_DOUBLED_PAWN          = 10;
int PENALTY_ISOLATED_PAWN         = 20;
int PENALTY_BACKWARD_PAWN         =  8;
int BONUS_PASSED_PAWN             = 20;
int BONUS_BISHOP_PAIR             = 10;
int BONUS_ROOK_BEHIND_PASSED_PAWN = 20;
int BONUS_ROOK_ON_OPEN_FILE       = 20;
int BONUS_TWO_ROOKS_ON_OPEN_FILE  = 5;

int BONUS_PAWN_SHIELD_STRONG = 9;
int BONUS_PAWN_SHIELD_WEAK = 4;

int PAWN_OWN_DISTANCE[8] =           { 0,   8,  4,  2,  0,  0,  0,  0 };
int PAWN_OPPONENT_DISTANCE[8] =      { 0,   2,  1,  0,  0,  0,  0,  0 };
int KNIGHT_DISTANCE[8] =             { 0,   4,  4,  0,  0,  0,  0,  0 };
int BISHOP_DISTANCE[8] =             { 0,   5,  4,  3,  2,  1,  0,  0 };
int ROOK_DISTANCE[8] =               { 0,   7,  5,  4,  3,  0,  0,  0 };
int QUEEN_DISTANCE[8] =              { 0,  10,  8,  5,  4,  0,  0,  0 };

Bitmap PASSED_WHITE[64];
Bitmap PASSED_BLACK[64];
Bitmap ISOLATED_WHITE[64];
Bitmap ISOLATED_BLACK[64];
Bitmap BACKWARD_WHITE[64];
Bitmap BACKWARD_BLACK[64];
Bitmap KINGSHIELD_STRONG_W[64];
Bitmap KINGSHIELD_STRONG_B[64];
Bitmap KINGSHIELD_WEAK_W[64];
Bitmap KINGSHIELD_WEAK_B[64];

void init_data(void)
{
    int i, square;
    int from, to, col_from, col_to, fil_from, fil_to, dif_fil, dif_col;
    Bitmap tmp, fb, cb;

    BITSET[0] = 1;
    for (i = 1; i < 64; i++)
    {
        BITSET[i] = BITSET[i - 1] << 1;
    }

    for (from = 0; from < 64; from++)
    {
        fil_from = FILA(from);
        col_from = COLUMNA(from);

        fb = 0;
        cb = 0;
        for(i=0; i < 8; i++)
        {
            fb |= BITSET[fil_from*8+i];
            cb |= BITSET[col_from+i*8];
        }
        FILA_MASK[from] = fb;
        COLUMNA_MASK[from] = cb;

        for (to = 0; to < 64; to++)
        {
            fil_to = FILA(to);
            col_to = COLUMNA(to);
            tmp = 0;
            if (from != to)
            {
                if (fil_from == fil_to)
                {
                    dif_col = (col_from > col_to) ? -1 : +1;
                    for (i = col_from + dif_col; i != col_to; i += dif_col)
                    {
                        tmp |= BITSET[fil_from * 8 + i];
                    }
                }
                else if (col_from == col_to)
                {
                    dif_fil = (fil_from > fil_to) ? -1 : +1;
                    for (i = fil_from + dif_fil; i != fil_to; i += dif_fil)
                    {
                        tmp |= BITSET[i * 8 + col_from];
                    }
                }
                else if (abs(col_from - col_to) == abs(fil_from - fil_to))
                {
                    dif_fil = (fil_from > fil_to) ? -1 : +1;
                    dif_col = (col_from > col_to) ? -1 : +1;
                    for (i = 1; i < abs(col_from - col_to); i++)
                    {
                        tmp |= BITSET[(fil_from + i * dif_fil) * 8 + col_from + i * dif_col];
                    }
                }
            }
            FREEWAY[from][to] = tmp;
        }
    }

    for (from = 0; from < 64; from++)
    {
        fil_from = FILA(from);
        col_from = COLUMNA(from);

        // BLACK_PAWN_MOVES
        if (fil_from > 0)
        {
            fil_to = fil_from - 1;
            to = fil_to * 8 + col_from;
            BLACK_PAWN_MOVES[from] = BITSET[to];
        }
        else
        {
            BLACK_PAWN_MOVES[from] = 0;
        }

        // BLACK_PAWN_DOUBLE_MOVES
        if (fil_from == 6)
        {
            fil_to = 4;
            to = fil_to * 8 + col_from;
            BLACK_PAWN_DOUBLE_MOVES[from] = BITSET[to];
        }
        else
        {
            BLACK_PAWN_DOUBLE_MOVES[from] = 0;
        }

        // BLACK_PAWN_ATTACKS
        BLACK_PAWN_ATTACKS[from] = 0;
        if (fil_from > 0)
        {
            fil_to = fil_from - 1;
            col_to = col_from - 1;
            if (col_to >= 0)
            {
                to = fil_to * 8 + col_to;
                BLACK_PAWN_ATTACKS[from] |= BITSET[to];
            }
            col_to = col_from + 1;
            if (col_to <= 7)
            {
                to = fil_to * 8 + col_to;
                BLACK_PAWN_ATTACKS[from] |= BITSET[to];
            }
        }

        // BLACK_PAWN_POSTATTACKS
        BLACK_PAWN_POSTATTACKS[from] = 0;
        if (fil_from < 6)
        {
            fil_to = fil_from + 1;
            col_to = col_from - 1;
            if (col_to >= 0)
            {
                to = fil_to * 8 + col_to;
                BLACK_PAWN_POSTATTACKS[from] |= BITSET[to];
            }
            col_to = col_from + 1;
            if (col_to <= 7)
            {
                to = fil_to * 8 + col_to;
                BLACK_PAWN_POSTATTACKS[from] |= BITSET[to];
            }
        }

        // WHITE_PAWN_MOVES
        if (fil_from < 7)
        {
            fil_to = fil_from + 1;
            to = fil_to * 8 + col_from;
            WHITE_PAWN_MOVES[from] = BITSET[to];
        }
        else
        {
            WHITE_PAWN_MOVES[from] = 0;
        }

        // WHITE_PAWN_DOUBLE_MOVES
        if (fil_from == 1)
        {
            fil_to = 3;
            to = fil_to * 8 + col_from;
            WHITE_PAWN_DOUBLE_MOVES[from] = BITSET[to];
        }
        else
        {
            WHITE_PAWN_DOUBLE_MOVES[from] = 0;
        }

        // WHITE_PAWN_ATTACKS
        WHITE_PAWN_ATTACKS[from] = 0;
        if (fil_from < 7)
        {
            fil_to = fil_from + 1;
            col_to = col_from - 1;
            if (col_to >= 0)
            {
                to = fil_to * 8 + col_to;
                WHITE_PAWN_ATTACKS[from] |= BITSET[to];
            }
            col_to = col_from + 1;
            if (col_to <= 7)
            {
                to = fil_to * 8 + col_to;
                WHITE_PAWN_ATTACKS[from] |= BITSET[to];
            }
        }

        WHITE_PAWN_POSTATTACKS[from] = 0;
        if (fil_from > 1)
        {
            fil_to = fil_from - 1;
            col_to = col_from + 1;
            if (col_to <= 7)
            {
                to = fil_to * 8 + col_to;
                WHITE_PAWN_POSTATTACKS[from] |= BITSET[to];
            }
            col_to = col_from - 1;
            if (col_to >= 0)
            {
                to = fil_to * 8 + col_to;
                WHITE_PAWN_POSTATTACKS[from] |= BITSET[to];
            }
        }

        // KNIGHT attacks;
        KNIGHT_ATTACKS[from] = 0;
        fil_to = fil_from + 2;
        col_to = col_from + 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from + 2;
        col_to = col_from - 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 2;
        col_to = col_from + 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 2;
        col_to = col_from - 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from + 1;
        col_to = col_from + 2;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from + 1;
        col_to = col_from - 2;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 1;
        col_to = col_from + 2;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 1;
        col_to = col_from - 2;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KNIGHT_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }

        // KING attacks;
        KING_ATTACKS[from] = 0;
        fil_to = fil_from + 1;
        col_to = col_from + 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from + 1;
        col_to = col_from;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from + 1;
        col_to = col_from - 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 1;
        col_to = col_from + 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 1;
        col_to = col_from;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 1;
        col_to = col_from - 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 0;
        col_to = col_from + 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }
        fil_to = fil_from - 0;
        col_to = col_from - 1;
        if ((fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
        {
            KING_ATTACKS[from] |= BITSET[fil_to * 8 + col_to];
        }

        // LINE_ATTACKS
        LINE_ATTACKS[from] = 0;
        for (col_to = col_from + 1; col_to <= 7; col_to++)
        {
            LINE_ATTACKS[from] |= BITSET[fil_from * 8 + col_to];
        }
        for (col_to = col_from - 1; col_to >= 0; col_to--)
        {
            LINE_ATTACKS[from] |= BITSET[fil_from * 8 + col_to];
        }
        for (fil_to = fil_from + 1; fil_to <= 7; fil_to++)
        {
            LINE_ATTACKS[from] |= BITSET[fil_to * 8 + col_from];
        }
        for (fil_to = fil_from - 1; fil_to >= 0; fil_to--)
        {
            LINE_ATTACKS[from] |= BITSET[fil_to * 8 + col_from];
        }

        // DIAG_ATTACKS
        DIAG_ATTACKS[from] = 0;
        for (i = 1; i < 8; i++)
        {
            col_to = col_from + i;
            fil_to = fil_from + i;
            to = fil_to * 8 + col_to;
            if ((from != to) && (fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
            {
                DIAG_ATTACKS[from] |= BITSET[to];
            }
            col_to = col_from + i;
            fil_to = fil_from - i;
            to = fil_to * 8 + col_to;
            if ((from != to) && (fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
            {
                DIAG_ATTACKS[from] |= BITSET[to];
            }
            col_to = col_from - i;
            fil_to = fil_from + i;
            to = fil_to * 8 + col_to;
            if ((from != to) && (fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
            {
                DIAG_ATTACKS[from] |= BITSET[to];
            }
            col_to = col_from - i;
            fil_to = fil_from - i;
            to = fil_to * 8 + col_to;
            if ((from != to) && (fil_to >= 0) && (fil_to <= 7) && (col_to >= 0) && (col_to <= 7))
            {
                DIAG_ATTACKS[from] |= BITSET[to];
            }
        }


        PASSED_WHITE[from] = 0;
        for (fil_to = fil_from + 1; fil_to <= 7; fil_to++) PASSED_WHITE[from] |= BITSET[fil_to * 8 + col_from];

        PASSED_BLACK[from] = 0;
        for (fil_to = fil_from - 1; fil_to >= 0; fil_to--) PASSED_BLACK[from] |= BITSET[fil_to * 8 + col_from];

        ISOLATED_WHITE[from] = 0;
        for (fil_to = fil_from; fil_to <= 7; fil_to++)
        {
            if( col_from ) ISOLATED_WHITE[from] |= BITSET[fil_to * 8 + col_from-1];
            if( col_from < 7 ) ISOLATED_WHITE[from] |= BITSET[fil_to * 8 + col_from+1];
        }

        ISOLATED_BLACK[from] = 0;
        for (fil_to = fil_from; fil_to >= 0; fil_to--)
        {
            if( col_from ) ISOLATED_BLACK[from] |= BITSET[fil_to * 8 + col_from-1];
            if( col_from < 7 ) ISOLATED_BLACK[from] |= BITSET[fil_to * 8 + col_from+1];
        }

        KINGSHIELD_STRONG_W[from] = 0;
        KINGSHIELD_STRONG_B[from] = 0;
        KINGSHIELD_WEAK_W[from] = 0;
        KINGSHIELD_WEAK_B[from] = 0;
    }

    // Pawn shield bitmaps for king safety, only if the king is on the first 3 ranks:
    for (i = 0; i < 24; i++)
    {
        //  KINGSHIELD_STRONG_W & KINGSHIELD_WEAK_W:
        KINGSHIELD_STRONG_W[i] ^= BITSET[i + 8];
        KINGSHIELD_WEAK_W[i] ^= BITSET[i + 16];
        if (COLUMNA(i) > 1)
        {
            KINGSHIELD_STRONG_W[i] ^= BITSET[i + 7];
            KINGSHIELD_WEAK_W[i] ^= BITSET[i + 15];
        }
        if (COLUMNA(i) < 8)
        {
            KINGSHIELD_STRONG_W[i] ^= BITSET[i + 9];
            KINGSHIELD_WEAK_W[i] ^= BITSET[i + 17];
        }
        if (COLUMNA(i)== 1)
        {
            KINGSHIELD_STRONG_W[i] ^= BITSET[i + 10];
            KINGSHIELD_WEAK_W[i] ^= BITSET[i + 18];
        }
        if (COLUMNA(i)== 8)
        {
            KINGSHIELD_STRONG_W[i] ^= BITSET[i + 6];
            KINGSHIELD_WEAK_W[i] ^= BITSET[i + 14];
        }
    }

    // Data is supplied as mirrored for WHITE, so it's ready for BLACK to use:
    for (i = 0; i < 64; i++)
    {
        PAWNPOS_B[i] = PAWNPOS_W[i];
        KNIGHTPOS_B[i] = KNIGHTPOS_W[i];
        BISHOPPOS_B[i] = BISHOPPOS_W[i];
        ROOKPOS_B[i] = ROOKPOS_W[i];
        QUEENPOS_B[i] = QUEENPOS_W[i];
        KINGPOS_B[i] = KINGPOS_W[i];
        KINGPOS_ENDGAME_B[i] = KINGPOS_ENDGAME_W[i];
    }

    for (i = 0; i < 64; i++)
    {
        PAWNPOS_W[i] = PAWNPOS_B[MIRROR[i]];
        KNIGHTPOS_W[i] = KNIGHTPOS_B[MIRROR[i]];
        BISHOPPOS_W[i] = BISHOPPOS_B[MIRROR[i]];
        ROOKPOS_W[i] = ROOKPOS_B[MIRROR[i]];
        QUEENPOS_W[i] = QUEENPOS_B[MIRROR[i]];
        KINGPOS_W[i] = KINGPOS_B[MIRROR[i]];
        KINGPOS_ENDGAME_W[i] = KINGPOS_ENDGAME_B[MIRROR[i]];

        for (square = 0; square < 64; square ++)
        {
            //  KINGSHIELD_STRONG_B bitmaps (mirror of KINGSHIELD_STRONG_W bitmaps):
            if (KINGSHIELD_STRONG_W[i] & BITSET[square]) KINGSHIELD_STRONG_B[MIRROR[i]] |= BITSET[MIRROR[square]];

            //  KINGSHIELD_WEAK_B bitmaps (mirror of KINGSHIELD_WEAK_W bitmaps):
            if (KINGSHIELD_WEAK_W[i] & BITSET[square]) KINGSHIELD_WEAK_B[MIRROR[i]] |= BITSET[MIRROR[square]];
        }

    }



    for (from = 0 ; from < 64; from++)
    {
        for (to = 0 ; to < 64; to++)
        {
            if (abs(FILA(from) - FILA(to)) > abs(COLUMNA(from) - COLUMNA(to))) DISTANCE[from][to] = abs(FILA(from) - FILA(to));
            else DISTANCE[from][to] = abs(COLUMNA(from) - COLUMNA(to));
        }
    }


    //     ===========================================================================
    //     Initialize MS1BTABLE, used in last_one
    //     ===========================================================================
    for (i = 0; i < 256; i++)
    {
        MS1BTABLE[i] = (
            (i > 127) ? 7 :
            (i >  63) ? 6 :
            (i >  31) ? 5 :
            (i >  15) ? 4 :
            (i >   7) ? 3 :
            (i >   3) ? 2 :
            (i >   1) ? 1 : 0 );
    }


    init_data_steven();

}
