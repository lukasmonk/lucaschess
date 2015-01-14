
#include "daydreamer.h"

static const int trapped_bishop = 150;
// Note: the second luft value is an endgame score modifier,
// so the overall penalty is [10, 20].
static const int luft_penalty[2] = { 10, 10 };

/*
 * Find simple bad patterns that won't show up within reasonable search
 * depths. This is mostly trapped and blocked pieces.
 * TODO: trapped knight/rook patterns.
 * TODO: maybe merge this with eval_pieces so we have access to piece
 *       mobility information.
 */
score_t pattern_score(const position_t*pos)
{
    int s = 0;
    int eg_modifier = 0;
    if (pos->board[A2] == BB && pos->board[B3] == WP) s += trapped_bishop;
    if (pos->board[B1] == BB && pos->board[C2] == WP) s += trapped_bishop;
    if (pos->board[H2] == BB && pos->board[G3] == WP) s += trapped_bishop;
    if (pos->board[G1] == BB && pos->board[F2] == WP) s += trapped_bishop;

    if (pos->board[A7] == WB && pos->board[B6] == BP) s -= trapped_bishop;
    if (pos->board[B8] == WB && pos->board[C7] == BP) s -= trapped_bishop;
    if (pos->board[H7] == WB && pos->board[G6] == BP) s -= trapped_bishop;
    if (pos->board[G8] == WB && pos->board[F7] == BP) s -= trapped_bishop;

    square_t k = pos->pieces[WHITE][0];
    if (square_rank(k) == RANK_1) {
        bool luft = false;
        if (pos->board[k+N] != WP) luft = true;
        file_t f = square_file(k);
        if (!luft && f > FILE_A && pos->board[k+N-1] != WP) luft = true;
        if (!luft && f < FILE_H && pos->board[k+N+1] != WP) luft = true;
        if (!luft) {
            s -= luft_penalty[0];
            eg_modifier -= luft_penalty[1];
        }
    }
    k = pos->pieces[BLACK][0];
    if (square_rank(k) == RANK_8) {
        bool luft = false;
        if (pos->board[k+S] != BP) luft = true;
        file_t f = square_file(k);
        if (!luft && f > FILE_A && pos->board[k+S-1] != BP) luft = true;
        if (!luft && f < FILE_H && pos->board[k+S+1] != BP) luft = true;
        if (!luft) {
            s += luft_penalty[0];
            eg_modifier += luft_penalty[1];
        }
    }

    if (pos->side_to_move == BLACK) {
        s *= -1;
        eg_modifier *= -1;
    }
    score_t score;
    score.midgame = s;
    score.endgame = s + eg_modifier;
    return score;
}
