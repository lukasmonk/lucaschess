#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "protos.h"
#include "globals.h"


int ST_PAWNPOS_W[64] =
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


int ST_KNIGHTPOS_W[64] =
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

int ST_BISHOPPOS_W[64] =
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

int ST_ROOKPOS_W[64] =
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

int ST_QUEENPOS_W[64] =
{
    -20, -10, -10,  -5,  -5, -10, -10, -20,
    -10,   0,   0,   0,   0,   0,   0, -10,
    -10,   0,  15,  15,  15,  15,   0, -10,
     -5,   0,  15,  15,  15,  15,   0,  -5,
      0,   0,  15,  15,  15,  15,   0,  -5,
    -10,  15,  15,  15,  15,  15,   0, -10,
    -10,   0,  15,   0,   0,   0,   0, -10,
    -20, -10, -10, -10, -10, -10, -10, -20
};

int ST_KINGPOS_W[64] =
{
    -50, -40, -30, -20, -20, -30, -40, -50,
    -30, -20, -10,   0,   0, -10, -20, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  30,  40,  40,  30, -10, -30,
    -30, -10,  20,  30,  30,  20, -10, -30,
    -30, -30,   0,   0,   0,   0, -30, -30,
    -50, -30, -10, -30,   0, -30,   0, -50
};

int ST_PAWNPOS_B[64];
int ST_KNIGHTPOS_B[64];
int ST_BISHOPPOS_B[64];
int ST_ROOKPOS_B[64];
int ST_QUEENPOS_B[64];
int ST_KINGPOS_B[64];
int ST_KINGPOS_ENDGAME_B[64];

void init_data_steven(void)
{
    int i;


    // Data is supplied as mirrored for WHITE, so it's ready for BLACK to use:
    for (i = 0; i < 64; i++)
    {
        ST_PAWNPOS_B[i] = ST_PAWNPOS_W[i];
        ST_KNIGHTPOS_B[i] = ST_KNIGHTPOS_W[i];
        ST_BISHOPPOS_B[i] = ST_BISHOPPOS_W[i];
        ST_ROOKPOS_B[i] = ST_ROOKPOS_W[i];
        ST_QUEENPOS_B[i] = ST_QUEENPOS_W[i];
        ST_KINGPOS_B[i] = ST_KINGPOS_W[i];
    }

    for (i = 0; i < 64; i++)
    {
        ST_PAWNPOS_W[i] = ST_PAWNPOS_B[MIRROR[i]];
        ST_KNIGHTPOS_W[i] = ST_KNIGHTPOS_B[MIRROR[i]];
        ST_BISHOPPOS_W[i] = ST_BISHOPPOS_B[MIRROR[i]];
        ST_ROOKPOS_W[i] = ST_ROOKPOS_B[MIRROR[i]];
        ST_QUEENPOS_W[i] = ST_QUEENPOS_B[MIRROR[i]];
        ST_KINGPOS_W[i] = ST_KINGPOS_B[MIRROR[i]];
    }

}

int eval_steven(void) {
    int score, square;
    int whitepawns, whiteknights, whitebishops, whiterooks, whitequeens;
    int blackpawns, blackknights, blackbishops, blackrooks, blackqueens;
    int whitetotalmat, blacktotalmat;
    int whitekingsquare, blackkingsquare;
    int valpawn, valknight, valbishop, valrook, valqueen;
    Bitmap temp;


    whitepawns = bit_count(board.white_pawns);
    whiteknights = bit_count(board.white_knights);
    whitebishops = bit_count(board.white_bishops);
    whiterooks = bit_count(board.white_rooks);
    whitequeens = bit_count(board.white_queens);
    whitetotalmat = 3 * whiteknights + 3 * whitebishops + 5 * whiterooks + 10 * whitequeens;
    blackpawns = bit_count(board.black_pawns);
    blackknights = bit_count(board.black_knights);
    blackbishops = bit_count(board.black_bishops);
    blackrooks = bit_count(board.black_rooks);
    blackqueens = bit_count(board.black_queens);
    blacktotalmat = 3 * blackknights + 3 * blackbishops + 5 * blackrooks + 10 * blackqueens;


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Remember where the kings are
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (board.white_king) {
        whitekingsquare = first_one(board.white_king);
    } else {
        return (board.color) ? MATESCORE : -MATESCORE;
    }
    if (board.black_king) {
        blackkingsquare = first_one(board.black_king);
    } else {
        return (board.color) ? -MATESCORE : +MATESCORE;
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate for draws due to insufficient material:
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if (!whitepawns && !blackpawns) {
        // king versus king:
        if ((whitetotalmat == 0) && (blacktotalmat == 0)) {
            if (board.color) {
                return -DRAWSCORE;
            } else {
                return DRAWSCORE;
            }
        }

        // king and knight versus king:
        if (((whitetotalmat == 3) && (whiteknights == 1) && (blacktotalmat == 0)) ||
                ((blacktotalmat == 3) && (blackknights == 1) && (whitetotalmat == 0))) {
            if (board.color) {
                return -DRAWSCORE;
            } else {
                return DRAWSCORE;
            }
        }

        // 2 kings with one or more bishops, and all bishops on the same colour:
        if ((whitebishops + blackbishops) > 0) {
            if ((whiteknights == 0) && (whiterooks == 0) && (whitequeens == 0) &&
                    (blackknights == 0) && (blackrooks == 0) && (blackqueens == 0)) {
                if (!((board.white_bishops | board.black_bishops) & WHITE_SQUARES) ||
                        !((board.white_bishops | board.black_bishops) & BLACK_SQUARES)) {
                    return DRAWSCORE;
                }
            }
        }
    }

    // ----------------------------------------------------------------------------
    // Value of pieces
    // ----------------------------------------------------------------------------

    valqueen = QUEEN_VALUE*12/10;
    valrook = ROOK_VALUE*9/10;
    valbishop = BISHOP_VALUE*9/10;
    valknight = KNIGHT_VALUE*8/10;

    valpawn = PAWN_VALUE;


    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate MATERIAL
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    score = (whitepawns-blackpawns) * valpawn +
            (whiteknights-blackknights) * valknight +
            (whitebishops-blackbishops) * valbishop +
            (whiterooks-blackrooks) * valrook +
            (whitequeens-blackqueens) * valqueen;

    score *= 12/10;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate WHITE PIECES
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white pawns
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_pawns;
    while (temp) {
        square = first_one(temp);

        // - position on the board
        score += ST_PAWNPOS_W[square];

        // - distance from opponent king
        score += PAWN_OPPONENT_DISTANCE[DISTANCE[square][blackkingsquare]];

        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white knights
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_knights;
    while (temp) {
        square = first_one(temp);
        score += ST_KNIGHTPOS_W[square];
        score += KNIGHT_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white bishops
    // - having the pair
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_bishops;
    while (temp) {
        square = first_one(temp);
        score += ST_BISHOPPOS_W[square];
        score += BISHOP_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white rooks
    // - position on the board
    // - distance from opponent king
    // - on the same file as a passed pawn
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_rooks;
    while (temp) {
        square = first_one(temp);
        score += ST_ROOKPOS_W[square];
        score += ROOK_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate white queens
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.white_queens;
    while (temp) {
        square = first_one(temp);
        score += ST_QUEENPOS_W[square];
        score += QUEEN_DISTANCE[DISTANCE[square][blackkingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate the white king
    // - position on the board
    // - proximity to the pawns
    // - pawn shield (not in the endgame)
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    score += ST_KINGPOS_W[whitekingsquare];

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate BLACK PIECES
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black pawns
    // - position on the board
    // - distance from opponent king
    // - distance from own king
    // - passed, doubled, isolated or backward pawns
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_pawns;
    while (temp) {
        square = first_one(temp);
        score -= ST_PAWNPOS_B[square];
        temp ^= BITSET[square];

        // - distance from opponent king
        score += PAWN_OPPONENT_DISTANCE[DISTANCE[square][whitekingsquare]];

        square = first_one(temp);
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black knights
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_knights;
    while (temp) {
        square = first_one(temp);
        score -= ST_KNIGHTPOS_B[square];
        score -= KNIGHT_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black bishops
    // - having the pair
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_bishops;
    while (temp) {
        square = first_one(temp);
        score -= ST_BISHOPPOS_B[square];
        score -= BISHOP_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black rooks
    // - position on the board
    // - distance from opponent king
    // - on the same file as a passed pawn
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_rooks;
    while (temp) {
        square = first_one(temp);
        score -= ST_ROOKPOS_B[square];
        score -= ROOK_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate black queens
    // - position on the board
    // - distance from opponent king
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    temp = board.black_queens;
    while (temp) {
        square = first_one(temp);
        score -= ST_QUEENPOS_B[square];
        score -= QUEEN_DISTANCE[DISTANCE[square][whitekingsquare]];
        temp ^= BITSET[square];
    }

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Evaluate the black king
    // - position on the board
    // - proximity to the pawns
    // - pawn shield (not in the endgame)
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

     score -= ST_KINGPOS_B[blackkingsquare];
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Return the score
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    if (board.color) {
        return -score;
    } else {
        return +score;
    }
}
