
#include "daydreamer.h"

#define shield_scale    1024
#define attack_scale    1024

static void evaluate_king_shield(const position_t* pos, int score[2]);
static void evaluate_king_attackers(const position_t* pos,
        int shield_score[2],
        int score[2]);

const int shield_value[2][17] = {
    { 0, 8, 2, 4, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 2, 4, 1, 1, 0, 0, 0 },
};

const int king_attack_score[16] = {
    0, 0, 16, 16, 32, 64, 0, 0, 0, 0, 16, 16, 32, 64, 0, 0
};
const int num_king_attack_scale[16] = {
    0, 0, 640, 800, 1120, 1200, 1280, 1280,
    1344, 1344, 1408, 1408, 1472, 1472, 1536, 1536
};

score_t evaluate_king_safety(const position_t* pos, eval_data_t* ed)
{
    (void)ed;
    int shield_score[2], attack_score[2];

    evaluate_king_shield(pos, shield_score);
    evaluate_king_attackers(pos, shield_score, attack_score);

    score_t phase_score;
    color_t side = pos->side_to_move;
    phase_score.midgame =
        (attack_score[side] - attack_score[side^1])*attack_scale/1024 +
        (shield_score[side] - shield_score[side^1])*shield_scale/1024;
    phase_score.endgame = 0;
    return phase_score;
}

/*
 * Give some points for pawns directly in front of your king.
 */
static int king_shield_score(const position_t* pos, color_t side, square_t king)
{
    int s = 0;
    int push = pawn_push[side];
    s += shield_value[side][pos->board[king-1]] * 2;
    s += shield_value[side][pos->board[king+1]] * 2;
    s += shield_value[side][pos->board[king+push-1]] * 4;
    s += shield_value[side][pos->board[king+push]] * 6;
    s += shield_value[side][pos->board[king+push+1]] * 4;
    s += shield_value[side][pos->board[king+2*push-1]];
    s += shield_value[side][pos->board[king+2*push]] * 2;
    s += shield_value[side][pos->board[king+2*push+1]];
    return s;
}

/*
 * Compute the overall balance of king safety offered by pawn shields.
 */
static void evaluate_king_shield(const position_t* pos, int score[2])
{
    score[WHITE] = score[BLACK] = 0;
    int oo_score[2] = {0, 0};
    int ooo_score[2] = {0, 0};
    int castle_score[2] = {0, 0};
    square_t wk = pos->pieces[WHITE][0];
    if (pos->piece_count[BQ]) {
        score[WHITE] = king_shield_score(pos, WHITE, wk);
        if (has_oo_rights(pos, WHITE)) {
            oo_score[WHITE] = king_shield_score(pos, WHITE, G1);
        }
        if (has_ooo_rights(pos, WHITE)) {
            ooo_score[WHITE] = king_shield_score(pos, WHITE, C1);
        }
        castle_score[WHITE] = MAX(score[WHITE],
            MAX(oo_score[WHITE], ooo_score[WHITE]));
    }
    square_t bk = pos->pieces[BLACK][0];
    if (pos->piece_count[WQ]) {
        score[BLACK] = king_shield_score(pos, BLACK, bk);
        if (has_oo_rights(pos, BLACK)) {
            oo_score[BLACK] = king_shield_score(pos, BLACK, G8);
        }
        if (has_ooo_rights(pos, BLACK)) {
            ooo_score[BLACK] = king_shield_score(pos, BLACK, C8);
        }
        castle_score[BLACK] = MAX(score[BLACK],
                MAX(oo_score[BLACK], ooo_score[BLACK]));
    }
    score[WHITE] = (score[WHITE] + castle_score[WHITE])/2;
    score[BLACK] = (score[BLACK] + castle_score[BLACK])/2;
}

/*
 * Compute a measure of king safety given by the number and type of pieces
 * attacking a square adjacent to the king.
 */
static void evaluate_king_attackers(const position_t* pos,
        int shield_score[2],
        int score[2])
{
    static const int bad_shield = 28;
    for (color_t side = WHITE; side <= BLACK; ++side) {
        score[side] = 0;
        if (pos->piece_count[create_piece(side, QUEEN)] == 0) continue;
        const square_t opp_king = pos->pieces[side^1][0];
        int num_attackers = 0;
        for (int i=1; i<pos->num_pieces[side]; ++i) {
            const square_t attacker = pos->pieces[side][i];
            if (piece_attacks_near(pos, attacker, opp_king)) {
                score[side] += king_attack_score[pos->board[attacker]];
                num_attackers++;
            }
        }
        if (shield_score[side^1] <= bad_shield) num_attackers += 2;
        score[side] = score[side]*num_king_attack_scale[num_attackers]/1024;
    }
}

