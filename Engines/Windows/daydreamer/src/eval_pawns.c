
#include "daydreamer.h"
#include <string.h>

static const int isolation_penalty[2][8] = {
    { 6, 6, 6, 8, 8, 6, 6, 6 },
    { 8, 8, 8, 8, 8, 8, 8, 8 }
};
static const int open_isolation_penalty[2][8] = {
    { 14, 14, 15, 16, 16, 15, 14, 14 },
    { 16, 17, 18, 20, 20, 18, 17, 16 }
};
static const int doubled_penalty[2][8] = {
    { 5, 5, 5, 6, 6, 5, 5, 5 },
    { 6, 7, 8, 8, 8, 8, 7, 6 }
};
static const int passed_bonus[2][8] = {
    { 0,  5, 10, 20, 60, 120, 200, 0 },
    { 0, 10, 20, 25, 75, 135, 225, 0 },
};
static const int candidate_bonus[2][8] = {
    { 0, 5,  5, 10, 20, 30, 0, 0 },
    { 0, 5, 10, 15, 30, 45, 0, 0 },
};
static const int backward_penalty[2][8] = {
    { 6, 6, 6,  8,  8, 6, 6, 6 },
    { 8, 9, 9, 10, 10, 9, 9, 8 }
};
static const int unstoppable_passer_bonus[8] = {
    0, 500, 525, 550, 575, 600, 650, 0
};
static const int advanceable_passer_bonus[8] = {
    0, 20, 25, 30, 35, 40, 80, 0
};
static const int king_dist_bonus[8] = {
    0, 0, 5, 10, 15, 20, 25, 0
};
static const int connected_passer[2][8] = {
    { 0, 0, 1, 2,  5, 15, 20, 0},
    { 0, 0, 2, 5, 15, 40, 60, 0}
};
static const int connected_bonus[2] = { 5, 5 };
static const int passer_rook[2] = { 5, 15 };
static const int king_storm[0x80] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,-10,-10,-10,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0, -8, -8, -8,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  4,  4,  4,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  8,  8,  8,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0, 12, 12, 12,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0, 14, 16, 14,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int queen_storm[0x80] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  -10,-10,-10, -5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   -8, -8, -8, -4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    4,  4,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    8,  8,  8,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   12, 12, 12,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   14, 16, 14,  8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int central_space[0x80] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  2,  4,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  2,  4,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  1,  2,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

static pawn_data_t* pawn_table = NULL;
static int num_buckets;
static struct {
    int misses;
    int hits;
    int occupied;
    int evictions;
} pawn_hash_stats;

/*
 * Create a pawn hash table of the appropriate size.
 */
void init_pawn_table(const int max_bytes)
{
    assert(max_bytes >= 1024);
    int size = sizeof(pawn_data_t);
    num_buckets = 1;
    while (size <= max_bytes >> 1) {
        size <<= 1;
        num_buckets <<= 1;
    }
    if (pawn_table != NULL) free(pawn_table);
    pawn_table = malloc(size);
    assert(pawn_table);
    clear_pawn_table();
}

/*
 * Wipe the entire table.
 */
void clear_pawn_table(void)
{
    memset(pawn_table, 0, sizeof(pawn_data_t) * num_buckets);
    memset(&pawn_hash_stats, 0, sizeof(pawn_hash_stats));
}

/*
 * Look up the pawn data for the pawns in the given position.
 */
static pawn_data_t* get_pawn_data(const position_t* pos)
{
    pawn_data_t* pd = &pawn_table[pos->pawn_hash % num_buckets];
    if (pd->key == pos->pawn_hash) pawn_hash_stats.hits++;
    else if (pd->key != 0) pawn_hash_stats.evictions++;
    else {
        pawn_hash_stats.misses++;
        pawn_hash_stats.occupied++;
    }
    return pd;
}

/*
 * Print stats about the pawn hash.
 */
void print_pawn_stats(void)
{
    printf("info string pawn hash entries %d", num_buckets);
    printf(" filled %d (%.2f%%)", pawn_hash_stats.occupied,
            (float)pawn_hash_stats.occupied / (float)num_buckets*100.);
    printf(" evictions %d", pawn_hash_stats.evictions);
    printf(" hits %d (%.2f%%)", pawn_hash_stats.hits,
            (float)pawn_hash_stats.hits /
            (pawn_hash_stats.hits + pawn_hash_stats.misses)*100.);
    printf(" misses %d (%.2f%%)\n", pawn_hash_stats.misses,
            (float)pawn_hash_stats.misses /
            (pawn_hash_stats.hits + pawn_hash_stats.misses)*100.);
}

/*
 * Identify and record the position of all passed pawns. Analyze pawn structure
 * features, such as isolated and doubled pawns, and assign a pawn structure
 * score (which does not account for passers). This information is stored in
 * the pawn hash table, to prevent re-computation.
 */
pawn_data_t* analyze_pawns(const position_t* pos)
{
    pawn_data_t* pd = get_pawn_data(pos);
    if (pd->key == pos->pawn_hash) return pd;

    // Zero everything out and create pawn bitboards.
    memset(pd, 0, sizeof(pawn_data_t));
    pd->key = pos->pawn_hash;
    square_t sq, to;
    for (color_t color=WHITE; color<=BLACK; ++color) {
        for (int i=0; pos->pawns[color][i] != INVALID_SQUARE; ++i) {
            set_sq_bit(pd->pawns_bb[color], pos->pawns[color][i]);
        }
    }

    // Create outpost bitboard and analyze pawns.
    for (color_t color=WHITE; color<=BLACK; ++color) {
        pd->num_passed[color] = 0;
        int push = pawn_push[color];
        const piece_t pawn = create_piece(color, PAWN);
        const piece_t opp_pawn = create_piece(color^1, PAWN);
        bitboard_t our_pawns = pd->pawns_bb[color];
        bitboard_t their_pawns = pd->pawns_bb[color^1];

        for (int ind=0; ind<64; ++ind) {
            // Fill in mask of outpost squares.
            sq = index_to_square(ind);
            if (!(outpost_mask[color][ind] & their_pawns)) {
                set_bit(pd->outposts_bb[color], ind);
            }
            if (pos->board[sq] != pawn) continue;

            file_t file = square_file(sq);
            rank_t rank = square_rank(sq);
            rank_t rrank = relative_rank[color][rank];

            // Passed pawns and passed pawn candidates.
            bool passed = !(passed_mask[color][ind] & their_pawns);
            if (passed) {
                set_bit(pd->passed_bb[color], ind);
                pd->passed[color][pd->num_passed[color]++] = sq;
                pd->score[color].midgame += passed_bonus[0][rrank];
                pd->score[color].endgame += passed_bonus[1][rrank];
            } else {
                // Candidate passed pawns (one enemy pawn one file away).
                // TODO: this condition could be more sophisticated.
                int blockers = 0;
                for (to = sq + push;
                        pos->board[to] != OUT_OF_BOUNDS; to += push) {
                    if (pos->board[to-1] == opp_pawn) ++blockers;
                    if (pos->board[to] == opp_pawn) blockers = 2;
                    if (pos->board[to+1] == opp_pawn) ++blockers;
                }
                if (blockers < 2) {
                    pd->score[color].midgame += candidate_bonus[0][rrank];
                    pd->score[color].endgame += candidate_bonus[1][rrank];
                }
            }

            // Isolated pawns.
            bool isolated = (neighbor_file_mask[file] & our_pawns) == 0;
            bool open = (in_front_mask[color][ind] & their_pawns) == 0;
            if (isolated) {
                if (open) {
                    pd->score[color].midgame -= open_isolation_penalty[0][file];
                    pd->score[color].endgame -= open_isolation_penalty[1][file];
                } else {
                    pd->score[color].midgame -= isolation_penalty[0][file];
                    pd->score[color].endgame -= isolation_penalty[1][file];
                }
            }

            // Pawn storm scores. Only used in opposite-castling positions.
            int storm = 1.5*king_storm[sq ^ (0x70*color)];
            if (storm && (passed_mask[color][ind] &
                        (~file_mask[file]) & their_pawns)) storm += storm/2;
            if (storm && open) storm += storm/2;
            pd->kingside_storm[color] += storm;
            storm = 1.5*queen_storm[sq ^ (0x70*color)];
            if (storm && (passed_mask[color][ind] &
                        (~file_mask[file]) & their_pawns)) storm += storm/2;
            if (storm && open) storm += storm/2;
            pd->queenside_storm[color] += storm;

            // Doubled pawns.
            bool doubled = (in_front_mask[color^1][ind] & our_pawns) != 0;
            if (doubled) {
                pd->score[color].midgame -= doubled_penalty[0][file];
                pd->score[color].endgame -= doubled_penalty[1][file];
            }

            // Connected pawns.
            bool connected = neighbor_file_mask[file] & our_pawns &
                (rank_mask[rank] | rank_mask[rank + (color == WHITE ? 1:-1)]);
            if (connected) {
                pd->score[color].midgame += connected_bonus[0];
                pd->score[color].endgame += connected_bonus[1];
            }

            // Space bonus for connected advanced central pawns.
            if (connected) pd->score[color].midgame +=
                central_space[sq ^ (0x70*color)];

            // Backward pawns (unsupportable by pawns, can't advance).
            // TODO: a simpler formulation would be nice.
            if (!passed && !isolated && !connected &&
                    pos->board[sq+push-1] != opp_pawn &&
                    pos->board[sq+push+1] != opp_pawn) {
                bool backward = true;
                for (to = sq; pos->board[to] != OUT_OF_BOUNDS; to -= push) {
                    if (pos->board[to-1] == pawn || pos->board[to+1] == pawn) {
                        backward = false;
                        break;
                    }
                }
                if (backward) {
                    for (to = sq + 2*push; pos->board[to] != OUT_OF_BOUNDS;
                            to += push) {
                        if (pos->board[to-1] == opp_pawn ||
                                pos->board[to+1] == opp_pawn) break;
                        if (pos->board[to-1] == pawn ||
                                pos->board[to+1] == pawn) {
                            backward = false;
                            break;
                        }
                    }
                    if (backward) {
                        pd->score[color].midgame -= backward_penalty[0][file];
                        pd->score[color].endgame -= backward_penalty[1][file];
                    }
                }
            }
        }

        // Penalty for multiple pawn islands.
        int islands = 0;
        bool on_island = false;
        for (file_t f = FILE_A; f <= FILE_H; ++f) {
            if (!file_is_half_open(pd, f, color)) {
                if (!on_island) {
                    on_island = true;
                    islands++;
                }
            } else on_island = false;
        }
        if (islands) --islands;
        pd->score[color].midgame -= 2 * islands;
        pd->score[color].endgame -= 4 * islands;
    }
    return pd;
}

/*
 * Retrieve (and calculate if necessary) the pawn data associated with |pos|,
 * and use it to determine the overall pawn score for the given position. The
 * pawn data is also used as an input to other evaluation functions.
 */
score_t pawn_score(const position_t* pos, pawn_data_t** pawn_data)
{
    pawn_data_t* pd = analyze_pawns(pos);
    if (pawn_data) *pawn_data = pd;
    int passer_bonus[2] = {0, 0};
    int eg_passer_bonus[2] = {0, 0};
    int storm_score[2] = {0, 0};
    file_t king_file[2] = { square_file(pos->pieces[WHITE][0]),
                            square_file(pos->pieces[BLACK][0]) };
    for (color_t side=WHITE; side<=BLACK; ++side) {
        const square_t push = pawn_push[side];
        piece_t our_pawn = create_piece(side, PAWN);
        for (int i=0; i<pd->num_passed[side]; ++i) {
            square_t passer = pd->passed[side][i];
            assert(pos->board[passer] == create_piece(side, PAWN));
            square_t target = passer + push;
            rank_t rank = relative_rank[side][square_rank(passer)];
            if (pos->num_pieces[side^1] == 1) {
                // Other side is down to king+pawns. Is this passer stoppable?
                // This measure is conservative, which is fine.
                int prom_dist = 8 - rank;
                if (rank == RANK_2) --prom_dist;
                if (pos->side_to_move == side) --prom_dist;
                square_t prom_sq = square_file(passer) + A8*side;
                if (distance(pos->pieces[side^1][0], prom_sq) > prom_dist) {
                    passer_bonus[side] += unstoppable_passer_bonus[rank];
                }
            }

            // Adjust endgame bonus based on king proximity
            eg_passer_bonus[side] += king_dist_bonus[rank] *
                (distance(target, pos->pieces[side^1][0]) -
                 distance(target, pos->pieces[side][0]));

            // Is the passer connected to another friendly pawn?
            if (pos->board[passer-1] == our_pawn ||
                    pos->board[passer+1] == our_pawn ||
                    pos->board[passer-push-1] == our_pawn ||
                    pos->board[passer-push+1] == our_pawn) {
                passer_bonus[side] += connected_passer[0][rank];
                eg_passer_bonus[side] += connected_passer[1][rank];
            }

            // Find rooks behind the passer.
            square_t sq;
            for (sq = passer - push; pos->board[sq] == EMPTY; sq -= push) {}
            if (pos->board[sq] == create_piece(side, ROOK)) {
                passer_bonus[side] += passer_rook[0];
                eg_passer_bonus[side] += passer_rook[1];
            } else if (pos->board[sq] == create_piece(side^1, ROOK)) {
                passer_bonus[side] -= passer_rook[0];
                eg_passer_bonus[side] -= passer_rook[1];
            }

            // Can the pawn advance without being captured?
            if (pos->board[target] == EMPTY) {
                move_t push = rank == RANK_7 ?
                    create_move_promote(passer, target,
                            create_piece(side, PAWN), EMPTY, QUEEN) :
                    create_move(passer, target,
                            create_piece(side, PAWN), EMPTY);
                if (static_exchange_sign(pos, push) >= 0) {
                    passer_bonus[side] += advanceable_passer_bonus[rank];
                }
            }
        }
        // Apply pawn storm bonuses
        if (king_file[side] < FILE_E && king_file[side^1] > FILE_E) {
            storm_score[side] = pd->kingside_storm[side];
        } else if (king_file[side] > FILE_E && king_file[side^1] < FILE_E) {
            storm_score[side] = pd->queenside_storm[side];
        }
    }

    color_t side = pos->side_to_move;
    score_t score;
    score.midgame = pd->score[side].midgame + passer_bonus[side] -
        (pd->score[side^1].midgame + passer_bonus[side^1]) +
        storm_score[side] - storm_score[side^1];
    score.endgame = pd->score[side].endgame +
        passer_bonus[side] + eg_passer_bonus[side] -
        (pd->score[side^1].endgame +
         passer_bonus[side^1] + eg_passer_bonus[side^1]);
    return score;
}

