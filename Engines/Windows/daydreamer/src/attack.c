
#include "daydreamer.h"
#include <string.h>

#define INVALID_DELTA   0xff

int distance_data_storage[256];
const int* distance_data = distance_data_storage+128;

// Data for each (from,to) pair on what pieces can attack there.
attack_data_t board_attack_data_storage[256];
const attack_data_t* board_attack_data = board_attack_data_storage + 128;

// For each (from, to) pair, which pieces can attack squares 1 away from to?
piece_flag_t near_attack_data_storage[256];
const piece_flag_t* near_attack_data = near_attack_data_storage + 128;
// For each piece and (from, to) pair, which squares need to be checked to
// determine if the piece is almost attacking the |to| square?
square_t near_attack_deltas[16][256][4];

#define near_attack(from, to, piece) \
    ((near_attack_data[(from)-(to)] & piece_flags[(piece)]) != 0)

/*
 * Set attack data for a given combination of source and destination squares
 * and piece types. This is a helper function for |generate_attack_data|.
 */
static void add_near_attack(square_t target,
        square_t from,
        int delta,
        piece_t piece)
{
    near_attack_data_storage[128+from-target] |= get_piece_flag(piece);
    int i = 0;
    for (; near_attack_deltas[piece][128+from-target][i] != INVALID_DELTA;
            ++i) {
        if (near_attack_deltas[piece][128+from-target][i] == delta) return;
    }
    assert(near_attack_deltas[piece][128+from-target][i] == INVALID_DELTA);
    if (i<3) near_attack_deltas[piece][128+from-target][i] = delta;
    assert(near_attack_deltas[piece][128+from-target][0] != INVALID_DELTA);
}

/*
 * Calculate which pieces can attack from a given square to another square
 * for each possible (from,to) pair.
 */
void generate_attack_data(void)
{
    memset((char*)distance_data_storage, -1, sizeof(int)*256);
    for (square_t s1=A1; s1<=H8; ++s1) {
        if (!valid_board_index(s1)) continue;
        for (square_t s2=A1; s2<=H8; ++s2) {
            if (!valid_board_index(s2)) continue;
            distance_data_storage[128+s2-s1] = MAX(
                    abs(square_file(s2) - square_file(s1)),
                    abs(square_rank(s2) - square_rank(s1)));
        }
    }

    memset((char*)board_attack_data_storage, 0, sizeof(attack_data_t)*256);
    attack_data_t* mutable_attack_data = (attack_data_t*)board_attack_data;
    for (square_t from=A1; from<=H8; ++from) {
        if (!valid_board_index(from)) continue;
        for (piece_t piece=WP; piece<=BK; ++piece) {
            for (const direction_t* dir=piece_deltas[piece]; *dir; ++dir) {
                for (square_t to=from+*dir; valid_board_index(to); to+=*dir) {
                    mutable_attack_data[from-to].possible_attackers |=
                        get_piece_flag(piece);
                    mutable_attack_data[from-to].relative_direction = *dir;
                    if (piece_slide_type(piece) == NO_SLIDE) break;
                }
            }
        }
    }

    memset((char*)near_attack_data_storage, 0, sizeof(piece_flag_t)*256);
    for (int i=0; i<16; ++i)
        for (int j=0; j<256; ++j)
            for (int k=0; k<4; ++k)
                near_attack_deltas[i][j][k] = INVALID_DELTA;
    for (square_t target=A1; target<=H8; ++target) {
        if (!valid_board_index(target)) continue;
        for (square_t from=A1; from<=H8; ++from) {
            if (!valid_board_index(from)) continue;
            for (piece_t piece=WP; piece<=BK; ++piece) {
                for (const direction_t* dir=piece_deltas[piece]; *dir; ++dir) {
                    for (square_t to=from+*dir;
                            valid_board_index(to); to+=*dir) {
                        if (distance(target, to) == 1) {
                            add_near_attack(target, from, to-from, piece);
                            break;
                        }
                        if (piece_slide_type(piece) == NO_SLIDE) break;
                    }
                }
            }
        }
    }
}

/*
 * Is the piece on |from| pinned, and if so what's the direction to the king?
 */
direction_t pin_direction(const position_t* pos,
        square_t from,
        square_t king_sq)
{
    direction_t pin_dir = 0;
    if (!possible_attack(from, king_sq, WQ)) return 0;
    direction_t king_dir = direction(from, king_sq);
    square_t sq;
    for (sq = from + king_dir; pos->board[sq] == EMPTY; sq += king_dir) {}
    if (sq == king_sq) {
        // Nothing between us and the king. Is there anything
        // behind us that's doing the pinning?
        for (sq = from - king_dir; pos->board[sq] == EMPTY; sq -= king_dir) {}
        if (can_capture(pos->board[sq], pos->board[from]) &&
                possible_attack(from, king_sq, pos->board[sq]) &&
                (piece_slide_type(pos->board[sq]) != NONE)) {
            pin_dir = king_dir;
        }
    }
    return pin_dir;
}

/*
 * Is |sq| being directly attacked by any pieces on |side|? Works on both
 * occupied and unoccupied squares.
 */
bool is_square_attacked(const position_t* pos, square_t sq, color_t side)
{
    // For every opposing piece, look up the attack data for its square.
    // Special-case pawns for efficiency.
    piece_t opp_pawn = create_piece(side, PAWN);
    if (pos->board[sq - piece_deltas[opp_pawn][0]] == opp_pawn) return true;
    if (pos->board[sq - piece_deltas[opp_pawn][1]] == opp_pawn) return true;

    square_t from;
    for (const square_t* pfrom = &pos->pieces[side][0];
            (from = *pfrom) != INVALID_SQUARE;
            ++pfrom) {
        piece_t p = pos->board[from];
        if (possible_attack(from, sq, p)) {
            if (piece_slide_type(p) == NO_SLIDE) return true;
            direction_t att_dir = direction(from, sq);
            while (from != sq) {
                from += att_dir;
                if (from == sq) return true;
                if (pos->board[from] != EMPTY) break;
            }
        }
    }
    return false;
}

/*
 * Is a the piece on |from| attacking a square adjacent to |target|?
 */
bool piece_attacks_near(const position_t* pos, square_t from, square_t target)
{
    piece_t p = pos->board[from];
    if (near_attack(from, target, p)) {
        if (piece_slide_type(p) == NO_SLIDE) return true;
        int delta;
        for (int i=0; (delta = near_attack_deltas[p][128+from-target][i]) !=
                INVALID_DELTA; ++i) {
            square_t sq = from + delta;
            if (pos->board[sq] == OUT_OF_BOUNDS) continue;
            direction_t att_dir = direction(from, sq);
            square_t x = from;
            while (x != sq) {
                x += att_dir;
                if (x == sq) return true;
                if (pos->board[x] != EMPTY) break;
            }
        }
    }
    return false;
}

/*
 * Set |pos->check_square| to the location of a checking piece, and return 0
 * if |pos| is not check, 1 if there is exactly 1 checker, or 2 if there are
 * multiple checkers.
 */
uint8_t find_checks(position_t* pos)
{
    // For every opposing piece, look up the attack data for its square.
    uint8_t attackers = 0;
    pos->check_square = EMPTY;
    color_t side = pos->side_to_move^1;
    square_t sq = pos->pieces[side^1][0];

    // Special-case pawns for efficiency.
    piece_t opp_pawn = create_piece(side, PAWN);
    if (pos->board[sq - piece_deltas[opp_pawn][0]] == opp_pawn) {
        pos->check_square = sq - piece_deltas[opp_pawn][0];
        if (++attackers > 1) return attackers;
    }
    if (pos->board[sq - piece_deltas[opp_pawn][1]] == opp_pawn) {
        pos->check_square = sq - piece_deltas[opp_pawn][1];
        if (++attackers > 1) return attackers;
    }

    square_t from;
    for (square_t* pfrom = &pos->pieces[side][1];
            (from = *pfrom) != INVALID_SQUARE;
            ++pfrom) {
        piece_t p = pos->board[from];
        if (possible_attack(from, sq, p)) {
            if (piece_slide_type(p) == NO_SLIDE) {
                pos->check_square = from;
                if (++attackers > 1) return attackers;
                continue;
            }
            square_t att_sq = from;
            direction_t att_dir = direction(from, sq);
            while (att_sq != sq) {
                att_sq += att_dir;
                if (att_sq == sq) {
                    pos->check_square = from;
                    if (++attackers > 1) return attackers;
                }
                if (pos->board[att_sq]) break;
            }
        }
    }
    return attackers;
}

