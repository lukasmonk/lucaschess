
#include "daydreamer.h"

/*
 * Count all attackers and defenders of a square to determine whether or not
 * a capture is advantageous. Captures with a positive static eval are
 * favorable. Note: this implementation does not account for pinned pieces.
 */
int static_exchange_eval(const position_t* pos, move_t move)
{
    square_t attacker_sq = get_move_from(move);
    square_t attacked_sq = get_move_to(move);
    piece_t attacker = get_move_piece(move);
    piece_t captured = get_move_capture(move);
    square_t attacker_sqs[2][16];
    int num_attackers[2] = { 0, 0 };
    int initial_attacker[2] = { 0, 0 };
    int index;

    // Find all the pieces that could be attacking.
    // Pawns are handled separately, because there are potentially a lot of
    // them and only a few squares they could attack from.
    if (pos->board[attacked_sq + NW] == BP && attacked_sq + NW != attacker_sq) {
        index = num_attackers[BLACK]++;
        attacker_sqs[BLACK][index] = attacked_sq + NW;
    }
    if (pos->board[attacked_sq + NE] == BP && attacked_sq + NE != attacker_sq) {
        index = num_attackers[BLACK]++;
        attacker_sqs[BLACK][index] = attacked_sq + NE;
    }
    if (pos->board[attacked_sq + SW] == WP && attacked_sq + SW != attacker_sq) {
        index = num_attackers[WHITE]++;
        attacker_sqs[WHITE][index] = attacked_sq + SW;
    }
    if (pos->board[attacked_sq + SE] == WP && attacked_sq + SE != attacker_sq) {
        index = num_attackers[WHITE]++;
        attacker_sqs[WHITE][index] = attacked_sq + SE;
    }
    
    for (color_t side=WHITE; side<=BLACK; ++side) {
        square_t from, att_sq;
        piece_t piece;
        square_t att_dir;
        for (int i=0; pos->pieces[side][i] != INVALID_SQUARE; ++i) {
            from = pos->pieces[side][i];
            piece = pos->board[from];
            if (from == attacker_sq) continue;
            if (!possible_attack(from, attacked_sq, piece)) continue;
            piece_type_t type = piece_type(piece);
            switch (type) {
                case KING:
                case KNIGHT:
                    attacker_sqs[side][num_attackers[side]++] = from;
                    break;
                case BISHOP:
                case ROOK:
                case QUEEN:
                    att_dir = (square_t)direction(from, attacked_sq);
                    for (att_sq = from + att_dir; att_sq != attacked_sq &&
                            pos->board[att_sq] == EMPTY; att_sq += att_dir) {}
                    if (att_sq == attacked_sq) {
                        attacker_sqs[side][num_attackers[side]++] = from;
                    }
                    break;
                default: assert(false);
            }
        }
    }

    // At this point, all unblocked attackers other than |attacker| have been
    // added to |attackers|. Now play out all possible captures in order of
    // increasing piece value (but starting with |attacker|) while alternating
    // colors. Whenever a capture is made, add any x-ray attackers that removal
    // of the piece would reveal.
    color_t side = piece_color(attacker);
    initial_attacker[side] = 1;
    int gain[32] = { material_value(captured) };
    int gain_index = 1;
    int capture_value = material_value(attacker);
    bool initial_attack = true;
    while (num_attackers[side] + initial_attacker[side] + 1 || initial_attack) {
        initial_attack = false;
        // add in any new x-ray attacks
        const attack_data_t* att_data =
            &get_attack_data(attacker_sq, attacked_sq);
        if (att_data->possible_attackers & Q_FLAG) {
            square_t sq;
            for (sq=attacker_sq - att_data->relative_direction;
                    pos->board[sq] == EMPTY;
                    sq -= att_data->relative_direction) {}
            if (pos->board[sq] != OUT_OF_BOUNDS) {
                const piece_t xray_piece = pos->board[sq];
                if (att_data->possible_attackers & 
                        get_piece_flag(xray_piece)) {
                    const color_t xray_color = piece_color(xray_piece);
                    index = num_attackers[xray_color]++;
                    attacker_sqs[xray_color][index] = sq;
                }
            }
        }

        // score the capture under the assumption that it's defended.
        gain[gain_index] = capture_value - gain[gain_index - 1];
        ++gain_index;
        side ^= 1;

        // find the next lowest valued attacker
        int least_value = material_value(KING)+1;
        int att_index = -1;
        for (int i=0; i<num_attackers[side]; ++i) {
            capture_value = material_value(pos->board[attacker_sqs[side][i]]);
            if (capture_value < least_value) {
                least_value = capture_value;
                att_index = i;
            }
        }
        if (att_index == -1) {
            assert(num_attackers[side] == 0);
            break;
        }
        attacker = pos->board[attacker_sqs[side][att_index]];
        attacker_sq = attacker_sqs[side][att_index];
        index = --num_attackers[side];
        attacker_sqs[side][att_index] = attacker_sqs[side][index];
        capture_value = least_value;
        if (piece_type(attacker) == KING && num_attackers[side^1]) break;
    }

    // Now that gain array is set up, scan back through to get score.
    --gain_index;
    while (--gain_index) {
        gain[gain_index-1] = -gain[gain_index-1] < gain[gain_index] ?
            -gain[gain_index] : gain[gain_index-1];
    }
    return gain[0];
}

/*
 * If we only care about whether or not a move is losing, sometimes we don't
 * need a full static exchange eval and can bail out early.
 */
int static_exchange_sign(const position_t* pos, move_t move)
{
    piece_type_t attacker_type = piece_type(get_move_piece(move));
    piece_type_t captured_type = piece_type(get_move_capture(move));
    if (attacker_type == KING || attacker_type <= captured_type) return 1;
    return static_exchange_eval(pos, move);
}
