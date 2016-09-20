
#include "daydreamer.h"

static const int mobility_score_table[2][8][32] = {
    { // midgame
        {0},
        {0, 4},
        {-8, -4, 0, 4, 8, 12, 16, 18, 20},
        {-15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 40, 40, 40, 40},
        {-10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20},
        {-20, -19, -18, -17, -16, -15, -14, -13, -12, -11, -10, -9, -8, -7,
            -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
        {0}, {0}
    },
    { // endgame
        {0},
        {0, 12},
        {-8, -4, 0, 4, 8, 12, 16, 18, 20},
        {-15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 40, 40, 40, 40},
        {-10, -6, -2, 2, 6, 10, 14, 18, 22, 26, 30, 34, 38, 42, 46, 50},
        {-20, -18, -16, -14, -12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12,
            14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42},
        {0}, {0}
    },
};

static const int color_table[2][17] = {
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0}, // white
    {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // black
};

static const int knight_outpost[0x80] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  1,  4,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  2,  4,  5,  5,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  3,  6,  9,  9,  6,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  1,  3,  4,  4,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int bishop_outpost[0x80] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  2,  2,  2,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    3,  5,  6,  6,  6,  5,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    3,  5,  6,  6,  6,  5,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  2,  2,  2,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// TODO: these really need to be tuned
static const int rook_on_7[2] = { 20, 40 };
static const int rook_open_file_bonus[2] = { 20, 10 };
static const int rook_half_open_file_bonus[2] = { 10, 10 };

/*
 * Score a weak square that's occupied by a minor piece. The basic bonus
 * is given by a score table, and additional points are awarded for being
 * defended by a friendly pawn and for being difficult to take with an
 * opponent's minor piece.
 */
static int outpost_score(const position_t* pos, square_t sq, piece_type_t type)
{
    color_t side = piece_color(pos->board[sq]);
    int bonus = type == KNIGHT ? knight_outpost[sq ^ (0x70*side)] : bishop_outpost[sq ^ (0x70*side)];
    int score = bonus;
    if (bonus) {
        // An outpost is better when supported by pawns.
        piece_t our_pawn = create_piece(side, PAWN);
        if (pos->board[sq - pawn_push[side] - 1] == our_pawn ||
                pos->board[sq - pawn_push[side] + 1] == our_pawn) {
            score += bonus/2;
            // Even better if an opposing knight/bishop can't capture it.
            // TODO: take care of the case where there's one opposing bishop
            // that's the wrong color. The position data structure needs to
            // be modified a little to make this efficient, or I need to pull
            // out bishop color info before doing outposts.
            piece_t their_knight = create_piece(side^1, KNIGHT);
            piece_t their_bishop = create_piece(side^1, BISHOP);
            if (pos->piece_count[their_knight] == 0 &&
                    pos->piece_count[their_bishop] == 0) {
                score += bonus;
            }
        }
    }
    return score;
}

/*
 * Compute the number of squares each non-pawn, non-king piece could move to,
 * and assign a bonus or penalty accordingly. Also assign miscellaneous
 * bonuses based on outpost squares, open files, etc.
 */
score_t pieces_score(const position_t* pos, pawn_data_t* pd)
{
    score_t score;
    int mid_score[2] = {0, 0};
    int end_score[2] = {0, 0};
    rank_t king_rank[2] = { relative_rank[WHITE]
                                [square_rank(pos->pieces[WHITE][0])],
                            relative_rank[BLACK]
                                [square_rank(pos->pieces[BLACK][0])] };
    color_t side;
    for (side=WHITE; side<=BLACK; ++side) {
        const int* mobile = color_table[side];
        square_t from, to;
        piece_t piece;
        for (int i=1; pos->pieces[side][i] != INVALID_SQUARE; ++i) {
            from = pos->pieces[side][i];
            piece = pos->board[from];
            piece_type_t type = piece_type(piece);
            int ps = 0;
            switch (type) {
                case KNIGHT:
                    ps += mobile[pos->board[from-33]];
                    ps += mobile[pos->board[from-31]];
                    ps += mobile[pos->board[from-18]];
                    ps += mobile[pos->board[from-14]];
                    ps += mobile[pos->board[from+14]];
                    ps += mobile[pos->board[from+18]];
                    ps += mobile[pos->board[from+31]];
                    ps += mobile[pos->board[from+33]];
                    if (square_is_outpost(pd, from, side)) {
                        int bonus = outpost_score(pos, from, KNIGHT);
                        mid_score[side] += bonus;
                        end_score[side] += bonus;
                    }
                    break;
                case BISHOP:
                    for (to=from-17; pos->board[to]==EMPTY; to-=17, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from-15; pos->board[to]==EMPTY; to-=15, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+15; pos->board[to]==EMPTY; to+=15, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+17; pos->board[to]==EMPTY; to+=17, ++ps) {}
                    ps += mobile[pos->board[to]];
                    if (square_is_outpost(pd, from, side)) {
                        int bonus = outpost_score(pos, from, BISHOP);
                        mid_score[side] += bonus;
                        end_score[side] += bonus;
                    }
                    break;
                case ROOK:
                    for (to=from-16; pos->board[to]==EMPTY; to-=16, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from-1; pos->board[to]==EMPTY; to-=1, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+1; pos->board[to]==EMPTY; to+=1, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+16; pos->board[to]==EMPTY; to+=16, ++ps) {}
                    ps += mobile[pos->board[to]];
                    int rrank = relative_rank[side][square_rank(from)];
                    if (rrank == RANK_7 && king_rank[side^1] == RANK_8) {
                        mid_score[side] += rook_on_7[0];
                        end_score[side] += rook_on_7[1];
                    }
                    file_t file = square_file(from);
                    if (file_is_half_open(pd, file, side)) {
                        mid_score[side] += rook_half_open_file_bonus[0];
                        end_score[side] += rook_half_open_file_bonus[1];
                        if (file_is_half_open(pd, file, side^1)) {
                            mid_score[side] += rook_open_file_bonus[0];
                            end_score[side] += rook_open_file_bonus[1];
                        }
                    }
                    break;
                case QUEEN:
                    for (to=from-17; pos->board[to]==EMPTY; to-=17, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from-15; pos->board[to]==EMPTY; to-=15, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+15; pos->board[to]==EMPTY; to+=15, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+17; pos->board[to]==EMPTY; to+=17, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from-16; pos->board[to]==EMPTY; to-=16, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from-1; pos->board[to]==EMPTY; to-=1, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+1; pos->board[to]==EMPTY; to+=1, ++ps) {}
                    ps += mobile[pos->board[to]];
                    for (to=from+16; pos->board[to]==EMPTY; to+=16, ++ps) {}
                    ps += mobile[pos->board[to]];
                    if (relative_rank[side][square_rank(from)] == RANK_7 &&
                            king_rank[side^1] == RANK_8) {
                        mid_score[side] += rook_on_7[0] / 2;
                        end_score[side] += rook_on_7[1] / 2;
                    }
                    break;
                default: assert(false);
            }
            mid_score[side] += mobility_score_table[0][type][ps];
            end_score[side] += mobility_score_table[1][type][ps];
        }
    }
    side = pos->side_to_move;
    score.midgame = mid_score[side] - mid_score[side^1];
    score.endgame = end_score[side] - end_score[side^1];
    return score;
}

