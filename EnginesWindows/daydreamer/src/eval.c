
#include "daydreamer.h"

#include "pst.inc"

#define pawn_scale      1024
#define pattern_scale   1024
#define pieces_scale    1024
#define safety_scale    1024

static const int tempo_bonus[2] = { 9, 2 };

/*
 * Initialize all static evaluation data structures.
 */
void init_eval(void)
{
    for (piece_t piece=WP; piece<=WK; ++piece) {
        for (square_t square=A1; square<=H8; ++square) {
            if (!valid_board_index(square)) continue;
            piece_square_values[piece][square] =
                piece_square_values[piece+BP-1][flip_square(square)];
            endgame_piece_square_values[piece][square] =
                endgame_piece_square_values[piece+BP-1][flip_square(square)];
        }
    }
    for (piece_t piece=WP; piece<=BK; ++piece) {
        if (piece > WK && piece < BP) continue;
        for (square_t square=A1; square<=H8; ++square) {
            if (!valid_board_index(square)) continue;
            piece_square_values[piece][square] += material_value(piece);
            endgame_piece_square_values[piece][square] +=
                eg_material_value(piece);
        }
    }
}

/*
 * Combine two scores, scaling |addend| by the given factor.
 */
static void add_scaled_score(score_t* score, score_t* addend, int scale)
{
    score->midgame += addend->midgame * scale / 1024;
    score->endgame += addend->endgame * scale / 1024;
}

/*
 * Blend endgame and midgame values linearly according to |phase|.
 */
static int blend_score(score_t* score, int phase)
{
    return (phase*score->midgame + (MAX_PHASE-phase)*score->endgame)/MAX_PHASE;
}

/*
 * Perform a simple position evaluation based just on material and piece
 * square bonuses.
 */
int simple_eval(const position_t* pos)
{
    color_t side = pos->side_to_move;
    eval_data_t ed;
    ed.md = get_material_data(pos);

    int score = 0;
    int endgame_scale[2] = { ed.md->scale[WHITE], ed.md->scale[BLACK] };
    if (endgame_scale[WHITE]==0 && endgame_scale[BLACK]==0) return DRAW_VALUE;

    score_t phase_score = ed.md->score;
    if (side == BLACK) {
        phase_score.midgame *= -1;
        phase_score.endgame *= -1;
    }
    phase_score.midgame += pos->piece_square_eval[side].midgame -
        pos->piece_square_eval[side^1].midgame;
    phase_score.endgame += pos->piece_square_eval[side].endgame -
        pos->piece_square_eval[side^1].endgame;

    phase_score.midgame += tempo_bonus[0];
    phase_score.endgame += tempo_bonus[1];

    score = blend_score(&phase_score, ed.md->phase);
    score = (score * endgame_scale[score > 0 ? side : side^1]) / 1024;

    if (!can_win(pos, side)) score = MIN(score, DRAW_VALUE);
    if (!can_win(pos, side^1)) score = MAX(score, DRAW_VALUE);
    return score;
}

/*
 * Do full, more expensive evaluation of the position. Not implemented yet,
 * so just return the simple evaluation.
 */
int full_eval(const position_t* pos, eval_data_t* ed)
{
    color_t side = pos->side_to_move;
    score_t phase_score, component_score;
    ed->md = get_material_data(pos);

    int score = 0;
    int endgame_scale[2] = { ed->md->scale[WHITE], ed->md->scale[BLACK] };
    if (endgame_scale[WHITE]==0 && endgame_scale[BLACK]==0) return DRAW_VALUE;

    phase_score = ed->md->score;
    if (side == BLACK) {
        phase_score.midgame *= -1;
        phase_score.endgame *= -1;
    }
    phase_score.midgame += pos->piece_square_eval[side].midgame -
        pos->piece_square_eval[side^1].midgame;
    phase_score.endgame += pos->piece_square_eval[side].endgame -
        pos->piece_square_eval[side^1].endgame;

    component_score = pawn_score(pos, &ed->pd);
    add_scaled_score(&phase_score, &component_score, pawn_scale);
    component_score = pattern_score(pos);
    add_scaled_score(&phase_score, &component_score, pattern_scale);
    component_score = pieces_score(pos, ed->pd);
    add_scaled_score(&phase_score, &component_score, pieces_scale);
    component_score = evaluate_king_safety(pos, ed);
    add_scaled_score(&phase_score, &component_score, safety_scale);

    phase_score.midgame += tempo_bonus[0];
    phase_score.endgame += tempo_bonus[1];

    score = blend_score(&phase_score, ed->md->phase);
    score = (score * endgame_scale[score > 0 ? side : side^1]) / 1024;

    if (!can_win(pos, side)) score = MIN(score, DRAW_VALUE);
    if (!can_win(pos, side^1)) score = MAX(score, DRAW_VALUE);
    return score;
}

/*
 * Print a breakdown of the static evaluation of |pos|.
 */
void report_eval(const position_t* pos)
{
    eval_data_t ed_storage;
    eval_data_t* ed = &ed_storage;
    color_t side = pos->side_to_move;
    score_t phase_score, component_score;
    ed->md = get_material_data(pos);

    int score = 0;
    int endgame_scale[2] = { ed->md->scale[WHITE], ed->md->scale[BLACK] };
    printf("scale\t\t(%5d, %5d)\n", endgame_scale[WHITE], endgame_scale[BLACK]);

    phase_score = ed->md->score;
    if (side == BLACK) {
        phase_score.midgame *= -1;
        phase_score.endgame *= -1;
    }
    printf("md_score\t(%5d, %5d)\n", phase_score.midgame, phase_score.endgame);
    phase_score.midgame += pos->piece_square_eval[side].midgame -
        pos->piece_square_eval[side^1].midgame;
    phase_score.endgame += pos->piece_square_eval[side].endgame -
        pos->piece_square_eval[side^1].endgame;
    printf("psq_score\t(%5d, %5d)\n", phase_score.midgame, phase_score.endgame);

    component_score = pawn_score(pos, &ed->pd);
    add_scaled_score(&phase_score, &component_score, pawn_scale);
    printf("pawn_score\t(%5d, %5d)\n", phase_score.midgame, phase_score.endgame);
    component_score = pattern_score(pos);
    add_scaled_score(&phase_score, &component_score, pattern_scale);
    printf("pattern_score\t(%5d, %5d)\n", phase_score.midgame, phase_score.endgame);
    component_score = pieces_score(pos, ed->pd);
    add_scaled_score(&phase_score, &component_score, pieces_scale);
    printf("pieces_score\t(%5d, %5d)\n", phase_score.midgame, phase_score.endgame);
    component_score = evaluate_king_safety(pos, ed);
    add_scaled_score(&phase_score, &component_score, safety_scale);
    printf("safety_score\t(%5d, %5d)\n", phase_score.midgame, phase_score.endgame);

    phase_score.midgame += tempo_bonus[0];
    phase_score.endgame += tempo_bonus[1];

    score = blend_score(&phase_score, ed->md->phase);
    score = (score * endgame_scale[score > 0 ? side : side^1]) / 1024;

    if (!can_win(pos, side)) score = MIN(score, DRAW_VALUE);
    if (!can_win(pos, side^1)) score = MAX(score, DRAW_VALUE);
    printf("final_score\t%5d\n", score);
}

/*
 * Is there enough material left for either side to conceivably win?
 */
bool insufficient_material(const position_t* pos)
{
    return (pos->num_pawns[WHITE] == 0 &&
        pos->num_pawns[BLACK] == 0 &&
        pos->material_eval[WHITE] < ROOK_VAL + KING_VAL &&
        pos->material_eval[BLACK] < ROOK_VAL + KING_VAL);
}

/*
 * Is it still possible for |side| to win the game?
 */
bool can_win(const position_t* pos, color_t side)
{
    return !(pos->num_pawns[side] == 0 &&
            pos->material_eval[side] < ROOK_VAL + KING_VAL);
}

/*
 * Is the given position definitely a draw. This considers only draws by
 * rule, not positions that are known to be drawable with best play.
 */
bool is_draw(const position_t* pos)
{
    return pos->fifty_move_counter > 100 ||
        (pos->fifty_move_counter == 100 && !is_check(pos)) ||
        insufficient_material(pos) ||
        is_repetition(pos);
}

