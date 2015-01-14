
#include "daydreamer.h"

static void generate_pawn_captures(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves);
static void generate_piece_captures(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves);
static void generate_piece_noncaptures(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves);
static int generate_promotions(const position_t* pos, move_t* moves);
static void generate_pawn_quiet_moves(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves_head);
static int generate_pseudo_captures(const position_t* pos, move_t* moves);

/*
 * Push a new move onto a stack of moves, first doing some sanity checks.
 */
static move_t* add_move(const position_t* pos,
        move_t move,
        move_t* moves)
{
    (void)pos; // avoid warning when NDEBUG is defined
    check_move_validity(pos, move);
    *(moves++) = move;
    return moves;
}

/*
 * Fill the provided list with all legal moves in the given position.
 */
int generate_legal_moves(position_t* pos, move_t* moves)
{
    if (is_check(pos)) return generate_evasions(pos, moves);
    int num_pseudo = generate_pseudo_moves(pos, moves);
    move_t* moves_tail = moves+num_pseudo;
    move_t* moves_curr = moves;
    while (moves_curr < moves_tail) {
        check_pseudo_move_legality(pos, *moves_curr);
        if (!is_pseudo_move_legal(pos, *moves_curr)) {
            *moves_curr = *(--moves_tail);
            *moves_tail = 0;
        } else {
            ++moves_curr;
        }
    }
    return moves_tail-moves;
}

/*
 * Generate pseudo-legal captures and promotions. These moves are considered
 * first by the move selection algorithm.
 */
int generate_pseudo_tactical_moves(const position_t* pos, move_t* moves)
{
    move_t* moves_head = moves;
    moves += generate_promotions(pos, moves);
    moves += generate_pseudo_captures(pos, moves);
    return moves - moves_head;
}

/*
 * Fill the provided list with all pseudolegal moves in the given position.
 * Pseudolegal moves are moves which would be legal if we didn't have to worry
 * about leaving our king in check.
 */
int generate_pseudo_moves(const position_t* pos, move_t* moves)
{
    if (is_check(pos)) return generate_evasions(pos, moves);
    move_t* moves_head = moves;
    moves += generate_pseudo_tactical_moves(pos, moves);
    moves += generate_pseudo_quiet_moves(pos, moves);
    return moves-moves_head;
}

/*
 * Generate all pseudolegal moves that should be considered during quiescence
 * search. Currently this is just captures and promotions to queen.
 */
int generate_quiescence_moves(const position_t* pos,
        move_t* moves,
        bool generate_checks)
{
    if (is_check(pos)) return generate_evasions(pos, moves);
    move_t* moves_head = moves;
    moves += generate_pseudo_tactical_moves(pos, moves);
    if (generate_checks) moves += generate_pseudo_checks(pos, moves);
    return moves-moves_head;
}

/*
 * Fill the provided list with all pseudolegal captures in the given position.
 */
static int generate_pseudo_captures(const position_t* pos, move_t* moves)
{
    move_t* moves_head = moves;
    color_t side = pos->side_to_move;
    piece_t piece;
    square_t from;
    for (int i = 0; i < pos->num_pieces[side]; ++i) {
        from = pos->pieces[side][i];
        piece = pos->board[from];
        generate_piece_captures(pos, from, piece, &moves);
    }
    piece = create_piece(side, PAWN);
    for (int i=0; i<pos->num_pawns[side]; ++i) {
        from = pos->pawns[side][i];
        generate_pawn_captures(pos, from, piece, &moves);
    }
    *moves = 0;
    return moves-moves_head;
}

/*
 * Generate pseudo-legal moves which are neither captures nor promotions.
 */
int generate_pseudo_quiet_moves(const position_t* pos, move_t* moves)
{
    move_t* moves_head = moves;
    color_t side = pos->side_to_move;
    piece_t piece;
    square_t from;

    // Castling. Castles are considered pseudo-legal if we have appropriate
    // castling rights, the squares between king and rook are unoccupied,
    // and the intermediate square is unattacked. Therefore checking for
    // legality just requires seeing if we're in check afterwards.
    // This is messy for Chess960, so it's separated into separate cases.
    square_t my_king_home = king_home + side*A8;
    if (!options.chess960) {
        if (has_oo_rights(pos, side) &&
                pos->board[my_king_home+1] == EMPTY &&
                pos->board[my_king_home+2] == EMPTY &&
                !is_square_attacked((position_t*)pos,my_king_home+1,side^1)) {
            assert(pos->board[my_king_home] ==
                    create_piece(pos->side_to_move, KING));
            moves = add_move(pos,
                    create_move_castle(my_king_home, my_king_home+2,
                        create_piece(side, KING)),
                    moves);
        }
        if (has_ooo_rights(pos, side) &&
                pos->board[my_king_home-1] == EMPTY &&
                pos->board[my_king_home-2] == EMPTY &&
                pos->board[my_king_home-3] == EMPTY &&
                !is_square_attacked((position_t*)pos,my_king_home-1,side^1)) {
            assert(pos->board[my_king_home] ==
                    create_piece(pos->side_to_move, KING));
            moves = add_move(pos,
                    create_move_castle(my_king_home, my_king_home-2,
                        create_piece(side, KING)),
                    moves);
        }
    } else {
        if (has_oo_rights(pos, side)) {
            square_t my_f1 = F1 + side*A8;
            square_t my_g1 = G1 + side*A8;
            square_t my_kr = king_rook_home + side*A8;
            bool castle_ok = true;
            // Check that rook is unimpeded.
            for (square_t sq = MIN(my_kr, my_f1);
                    sq <= MAX(my_kr, my_f1); ++sq) {
                if (sq != my_kr  && sq != my_king_home &&
                        pos->board[sq] != EMPTY) {
                    castle_ok = false;
                    break;
                }
            }
            // Check that the king is unimpeded and unattacked
            if (castle_ok) {
                for (square_t sq = MIN(my_king_home, my_g1);
                        sq <= my_g1; ++sq) {
                    if (sq != my_king_home && sq != my_kr &&
                            pos->board[sq] != EMPTY) {
                        castle_ok = false;
                        break;
                    }
                    if (sq != my_g1 &&
                            is_square_attacked((position_t*)pos, sq, side^1)) {
                        castle_ok = false;
                        break;
                    }
                }
            }
            if (castle_ok) moves = add_move(pos,
                    create_move_castle(my_king_home, my_g1,
                        create_piece(side, KING)), moves);
        }
        if (has_ooo_rights(pos, side)) {
            square_t my_d1 = D1 + side*A8;
            square_t my_c1 = C1 + side*A8;
            square_t my_qr = queen_rook_home + side*A8;
            bool castle_ok = true;
            // Check that rook is unimpeded.
            for (square_t sq = MIN(my_qr, my_d1);
                    sq <= MAX(my_qr, my_d1); ++sq) {
                if (sq != my_qr && sq != my_king_home &&
                        pos->board[sq] != EMPTY) {
                    castle_ok = false;
                    break;
                }
            }
            // Check that the king is unimpeded and unattacked
            if (castle_ok) {
                for (square_t sq = MIN(my_king_home, my_c1);
                        sq <= MAX(my_king_home, my_c1); ++sq) {
                    if (sq != my_king_home && sq != my_qr &&
                            pos->board[sq] != EMPTY) {
                        castle_ok = false;
                        break;
                    }
                    if (sq != my_c1 &&
                            is_square_attacked((position_t*)pos, sq, side^1)) {
                        castle_ok = false;
                        break;
                    }
                }
            }
            if (castle_ok) moves = add_move(pos,
                    create_move_castle(my_king_home, my_c1,
                        create_piece(side, KING)),
                    moves);
        }
    }

    for (int i = 0; i < pos->num_pieces[side]; ++i) {
        from = pos->pieces[side][i];
        piece = pos->board[from];
        assert(piece_color(piece) == side && piece_type(piece) != PAWN);
        generate_piece_noncaptures(pos, from, piece, &moves);
    }
    piece = create_piece(side, PAWN);
    for (int i=0; i<pos->num_pawns[side]; ++i) {
        from = pos->pawns[side][i];
        assert(pos->board[from] == piece);
        generate_pawn_quiet_moves(pos, from, piece, &moves);
    }

    *moves = 0;
    return (moves-moves_head);
}

/*
 * Add all pseudo-legal non-capturing promotions.
 */
static int generate_promotions(const position_t* pos, move_t* moves)
{
    move_t* moves_head = moves;
    color_t side = pos->side_to_move;
    piece_t piece = create_piece(side, PAWN);
    for (int i = 0; i < pos->num_pawns[side]; ++i) {
        square_t from = pos->pawns[side][i];
        rank_t r_rank = relative_rank[side][square_rank(from)];
        if (r_rank < RANK_7) continue;
        square_t to = from + pawn_push[side];
        if (pos->board[to]) continue;
        for (piece_type_t type=KNIGHT; type<=QUEEN; ++type) {
            moves = add_move(pos,
                    create_move_promote(from, to, piece, EMPTY, type),
                    moves);
        }
    }
    *moves = 0;
    return (moves-moves_head);
}

/*
 * Generate all moves that evade check in the given position. This is purely
 * legal move generation; no pseudo-legal moves.
 */
int generate_evasions(const position_t* pos, move_t* moves)
{
    assert(pos->is_check && pos->board[pos->check_square]);
    move_t* moves_head = moves;
    color_t side = pos->side_to_move, other_side = side^1;
    square_t king_sq = pos->pieces[side][0];
    square_t check_sq = pos->check_square;
    piece_t king = create_piece(side, KING);
    piece_t checker = pos->board[check_sq];

    // Generate king moves.
    // Don't let the king mask its possible destination squares in calls
    // to is_square_attacked.
    square_t from = king_sq, to = INVALID_SQUARE;
    ((position_t*)pos)->board[king_sq] = EMPTY;
    for (const direction_t* delta = piece_deltas[king]; *delta; ++delta) {
        to = from + *delta;
        piece_t capture = pos->board[to];
        if (capture != EMPTY && !can_capture(king, capture)) continue;
        if (is_square_attacked((position_t*)pos,to,other_side)) continue;
        ((position_t*)pos)->board[king_sq] = king;
        moves = add_move(pos, create_move(from, to, king, capture), moves);
        ((position_t*)pos)->board[king_sq] = EMPTY;
    }
    ((position_t*)pos)->board[king_sq] = king;
    // If there are multiple checkers, only king moves are possible.
    if (pos->is_check > 1) {
        *moves = 0;
        return moves-moves_head;
    }
    
    // First, the most common case: a check that can be evaded via an
    // en passant capture. Note that if we're in check and an en passant
    // capture is available, the only way the en passant capture would evade
    // the check is if it's the newly moved pawn that's checking us.
    direction_t pin_dir;
    if (pos->ep_square != EMPTY &&
            check_sq+pawn_push[side] == pos->ep_square &&
            pos->board[pos->ep_square] == EMPTY) {
        piece_t our_pawn = create_piece(side, PAWN);
        to = pos->ep_square;
        for (int i=0; i<2; ++i) {
            from = to-piece_deltas[our_pawn][i];
            if (pos->board[from] && pos->board[from] == our_pawn) {
                pin_dir = pin_direction(pos, from, king_sq);
                if (pin_dir) continue;
                moves = add_move(pos,
                        create_move_enpassant(from, to, our_pawn, checker),
                        moves);
            }
        }
    }
    // Generate captures of the checker.
    for (int i = 0; i < pos->num_pawns[side]; ++i) {
        from = pos->pawns[side][i];
        piece_t piece = create_piece(side, PAWN);
        pin_dir = pin_direction(pos, from, king_sq);
        if (pin_dir) continue;
        if (!possible_attack(from, check_sq, piece)) continue;
        if (relative_rank[side][square_rank(from)] == RANK_7) {
            // Capture and promote.
            for (piece_t promoted=QUEEN; promoted > PAWN; --promoted) {
                moves = add_move(pos,
                        create_move_promote(from, check_sq, piece,
                            checker, promoted),
                        moves);
            }
        } else {
            moves = add_move(pos,
                    create_move(from, check_sq, piece, checker),
                    moves);
        }
    }
    for (int i = 1; i < pos->num_pieces[side]; ++i) {
        from = pos->pieces[side][i];
        piece_t piece = pos->board[from];
        pin_dir = pin_direction(pos, from, king_sq);
        if (pin_dir) continue;
        if (!possible_attack(from, check_sq, piece)) continue;
        if (piece_slide_type(piece) == NONE) {
            moves = add_move(pos,
                    create_move(from, check_sq, piece, checker),
                    moves);
        } else {
            // A sliding piece, keep going until we hit something.
            direction_t check_dir = direction(from, check_sq);
            for (to=from+check_dir; pos->board[to] == EMPTY; to+=check_dir) {}
            if (to == check_sq) {
                moves = add_move(pos,
                        create_move(from, to, piece, checker),
                        moves);
                continue;
            }
        }
    }
    if (piece_slide_type(checker) == NONE) {
        *moves = 0;
        return moves-moves_head;
    }
    
    // A slider is doing the checking; generate blocking moves.
    direction_t block_dir = direction(check_sq, king_sq);
    for (int i = 0; i < pos->num_pawns[side]; ++i) {
        from = pos->pawns[side][i];
        piece_t piece = create_piece(side, PAWN);
        direction_t k_dir;
        pin_dir = pin_direction(pos, from, king_sq);
        if (pin_dir) continue;
        to = from + pawn_push[side];
        if (pos->board[to] != EMPTY) continue;
        rank_t rank = relative_rank[side][square_rank(from)];
        k_dir = direction(to, king_sq);
        if (k_dir == block_dir &&
                ((king_sq < to && check_sq > to) ||
                 (king_sq > to && check_sq < to))) {
            if (rank == RANK_7) {
                // Block and promote.
                for (piece_t promoted=QUEEN; promoted > PAWN; --promoted) {
                    moves = add_move(pos,
                        create_move_promote(from, to, piece, EMPTY, promoted),
                        moves);
                }
            } else {
                moves = add_move(pos,
                        create_move(from, to, piece, EMPTY),
                        moves);
            }
        }
        if (rank != RANK_2) continue;
        to += pawn_push[side];
        if (pos->board[to]) continue;
        k_dir = direction(to, king_sq);
        if (k_dir == block_dir &&
                ((king_sq < to && check_sq > to) ||
                 (king_sq > to && check_sq < to))) {
            moves = add_move(pos,
                    create_move(from, to, piece, EMPTY),
                    moves);
        }
    }

    for (int i=1; i<pos->num_pieces[side]; ++i) {
        from = pos->pieces[side][i];
        piece_t piece = pos->board[from];
        direction_t k_dir;
        pin_dir = pin_direction(pos, from, king_sq);
        if (pin_dir) continue;
        if (piece_is_type(piece, KNIGHT)) {
            for (const direction_t* delta=piece_deltas[piece];
                    *delta; ++delta) {
                to = from + *delta;
                if (pos->board[to] != EMPTY) continue;
                k_dir = direction(to, king_sq);
                if (k_dir == block_dir &&
                        ((king_sq < to && check_sq > to) ||
                         (king_sq > to && check_sq < to))) {
                    moves = add_move(pos,
                            create_move(from, to, piece, EMPTY),
                            moves);
                }
            }
        } else {
           for (const direction_t* delta=piece_deltas[piece];
                   *delta; ++delta) {
                for (to = from+*delta; pos->board[to] == EMPTY; to+=*delta) {
                    k_dir = direction(to, king_sq);
                    if (k_dir == block_dir &&
                            ((king_sq < to && check_sq > to) ||
                             (king_sq > to && check_sq < to))) {
                        moves = add_move(pos,
                                create_move(from, to, piece, EMPTY),
                                moves);
                        break;
                    }
                }
            }
        }
    }
    *moves = 0;
    return moves-moves_head;
}

/*
 * Generate all non-capturing, non-promoting, pseudo-legal checks. Used for
 * quiescent move generation.
 */
int generate_pseudo_checks(const position_t* pos, move_t* moves)
{
    move_t* moves_head = moves;
    color_t side = pos->side_to_move, other_side = side^1;
    square_t king_sq = pos->pieces[other_side][0];
    for (int i = 0; i < pos->num_pawns[side]; ++i) {
        square_t from = pos->pawns[side][i];
        square_t to = INVALID_SQUARE;
        piece_t piece = pos->board[from];
        // Figure out whether or not we can discover check by moving.
        direction_t discover_check_dir = 0;
        bool will_discover_check;
        const attack_data_t* king_atk = &get_attack_data(from, king_sq);
        if ((king_atk->possible_attackers & Q_FLAG) == 0) {
            discover_check_dir = 0;
        } else {
            direction_t king_dir = king_atk->relative_direction;
            square_t sq;
            for (sq = from+king_dir; pos->board[sq] == EMPTY; sq+=king_dir) {}
            if (sq == king_sq) {
                // Nothing between us and the king. Is there anything
                // behind us to do the check when we move?
                for (sq = from-king_dir; pos->board[sq] == EMPTY;
                        sq -= king_dir) {}
                if (side == piece_color(pos->board[sq]) &&
                        (king_atk->possible_attackers &
                         get_piece_flag(pos->board[sq]))) {
                    discover_check_dir = king_dir;
                }
            }
        }
        // Generate checking moves.
        will_discover_check = discover_check_dir && abs(discover_check_dir)!=N;
        to = from + pawn_push[side];
        if (pos->board[to] != EMPTY) continue;
        rank_t r_rank =
            relative_rank[side][square_rank(from)];
        if (r_rank == RANK_7) continue; // non-promotes only
        for (const direction_t* delta = piece_deltas[piece]; *delta; ++delta) {
            if (will_discover_check || to + *delta == king_sq) {
                moves = add_move(pos,
                        create_move(from, to, piece, EMPTY),
                        moves);
                break;
            }
        }
        to += pawn_push[side];
        if (r_rank == RANK_2 && pos->board[to] == EMPTY) {
            for (const direction_t* delta = piece_deltas[piece];
                    *delta; ++delta) {
                if (will_discover_check || to + *delta == king_sq) {
                    moves = add_move(pos,
                            create_move(from, to, piece, EMPTY),
                            moves);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < pos->num_pieces[side]; ++i) {
        square_t from = pos->pieces[side][i];
        square_t to = INVALID_SQUARE;
        piece_t piece = pos->board[from];
        // Figure out whether or not we can discover check by moving.
        direction_t discover_check_dir = 0;
        bool will_discover_check;
        const attack_data_t* king_atk = &get_attack_data(from, king_sq);
        if ((king_atk->possible_attackers & Q_FLAG) == 0) {
            discover_check_dir = 0;
        } else {
            direction_t king_dir = king_atk->relative_direction;
            square_t sq;
            for (sq = from+king_dir; pos->board[sq] == EMPTY; sq+=king_dir) {}
            if (sq == king_sq) {
                // Nothing between us and the king. Is there anything
                // behind us to do the check when we move?
                for (sq = from - king_dir; pos->board[sq] == EMPTY;
                        sq -= king_dir) {}
                if (side == piece_color(pos->board[sq]) &&
                        (king_atk->possible_attackers &
                         get_piece_flag(pos->board[sq]))) {
                    discover_check_dir = king_dir;
                }
            }
        }
        // Generate checking moves.
        if (piece_is_type(piece, KNIGHT)) {
            for (const direction_t* delta = piece_deltas[piece];
                    *delta; ++delta) {
                to = from + *delta;
                if (pos->board[to] != EMPTY) continue;
                if (discover_check_dir ||
                        possible_attack(to, king_sq, WN)) {
                    moves = add_move(pos,
                            create_move(from, to, piece, EMPTY),
                            moves);
                }
            }
        } else {
            for (const direction_t* delta = piece_deltas[piece];
                    *delta; ++delta) {
                for (to = from+*delta; pos->board[to] == EMPTY; to+=*delta) {
                    will_discover_check = discover_check_dir &&
                        abs(discover_check_dir) != abs(*delta);
                    if (will_discover_check) {
                        moves = add_move(pos,
                                create_move(from, to, piece, NONE),
                                moves);
                        continue;
                    }
                    if (possible_attack(to, king_sq, piece)) {
                        const direction_t to_king = direction(to, king_sq);
                        square_t x = to + to_king;
                        for (; pos->board[x] == EMPTY; x+=to_king) {}
                        if (x == king_sq) {
                            moves = add_move(pos,
                                    create_move(from, to, piece, NONE),
                                    moves);
                        }
                    }
                }
            }
        }
    }
    *moves = 0;
    return moves-moves_head;
}

/*
 * Add all pseudo-legal captures that the given pawn can make.
 */
static void generate_pawn_captures(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves_head)
{
    color_t side = pos->side_to_move;
    int cap_left = side == WHITE ? 15 : -15;
    int cap_right = side == WHITE ? 17 : -17;
    square_t to;
    rank_t r_rank = relative_rank[side][square_rank(from)];
    move_t* moves = *moves_head;
    if (r_rank < RANK_7) {
        // non-promote captures
        to = from + cap_left;
        if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
            moves = add_move(pos, create_move(from, to, piece,
                    pos->board[to]), moves);
        } else if (to == pos->ep_square && pos->board[to] == EMPTY) {
            moves = add_move(pos, create_move_enpassant(from, to, piece,
                    pos->board[to + pawn_push[side^1]]), moves);
        }
        to = from + cap_right;
        if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
            moves = add_move(pos, create_move(from, to, piece,
                    pos->board[to]), moves);
        } else if (to == pos->ep_square && pos->board[to] == EMPTY) {
            moves = add_move(pos, create_move_enpassant(from, to, piece,
                    pos->board[to + pawn_push[side^1]]), moves);
        }
    } else {
        // capture/promotes
        to = from + cap_left;
        if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
            for (piece_t promoted=QUEEN; promoted > PAWN; --promoted) {
                moves = add_move(pos, create_move_promote(from, to, piece,
                        pos->board[to], promoted), moves);
            }
        }
        to = from + cap_right;
        if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
            for (piece_t promoted=QUEEN; promoted > PAWN; --promoted) {
                moves = add_move(pos, create_move_promote(from, to, piece,
                        pos->board[to], promoted), moves);
            }
        }
    }
    *moves_head = moves;
}

/*
 * Generate all non-capturing, non-promoting pawn moves.
 */
static void generate_pawn_quiet_moves(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves_head)
{
    color_t side = pos->side_to_move;
    square_t to;
    rank_t r_rank = relative_rank[side][square_rank(from)];
    move_t* moves = *moves_head;
    to = from + pawn_push[side];
    if (r_rank == RANK_7 || pos->board[to] != EMPTY) return;
    moves = add_move(pos, create_move(from, to, piece, EMPTY), moves);
    to += pawn_push[side];
    if (r_rank == RANK_2 && pos->board[to] == EMPTY) {
        // initial two-square push
        moves = add_move(pos,
                create_move(from, to, piece, EMPTY),
                moves);
    }
    *moves_head = moves;
}

/*
 * Add all pseudo-legal captures that the given piece (N/B/Q/K) can make.
 */
static void generate_piece_captures(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves_head)
{
    move_t* moves = *moves_head;
    square_t to;
    // Note: I unrolled all these loops to handle each direction explicitly.
    // The idea was to increase performance, but it's only about 1% faster
    // for much more code, so it's possible I'll change this back later.
    switch (piece_type(piece)) {
        case KING:
            to = from - 17;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from - 16;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from - 15;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from - 1;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 1;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 15;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 16;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 17;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            break;
        case KNIGHT:
            to = from - 33;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from - 31;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from - 18;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from - 14;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 14;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 18;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 31;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            to = from + 33;
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            break;
        case BISHOP:
            for (to=from-17; pos->board[to]==EMPTY; to-=17) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from-15; pos->board[to]==EMPTY; to-=15) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+15; pos->board[to]==EMPTY; to+=15) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+17; pos->board[to]==EMPTY; to+=17) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            break;
        case ROOK:
            for (to=from-16; pos->board[to]==EMPTY; to-=16) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from-1; pos->board[to]==EMPTY; to-=1) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+1; pos->board[to]==EMPTY; to+=1) {}
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+16; pos->board[to]==EMPTY; to+=16) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            break;
        case QUEEN:
            for (to=from-17; pos->board[to]==EMPTY; to-=17) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from-16; pos->board[to]==EMPTY; to-=16) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from-15; pos->board[to]==EMPTY; to-=15) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from-1; pos->board[to]==EMPTY; to-=1) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+1; pos->board[to]==EMPTY; to+=1) {}
            if (pos->board[to] != EMPTY && can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+15; pos->board[to]==EMPTY; to+=15) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+16; pos->board[to]==EMPTY; to+=16) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            for (to=from+17; pos->board[to]==EMPTY; to+=17) {}
            if (can_capture(piece, pos->board[to])) {
                moves = add_move(pos, create_move(from, to, piece,
                            pos->board[to]), moves);
            }
            break;
        default: assert(false);
    }
    *moves_head = moves;
}

/*
 * Add all pseudo-legal non-capturing moves that the given piece (N/B/Q/K)
 * can make, with the exception of castles.
 */
static void generate_piece_noncaptures(const position_t* pos,
        square_t from,
        piece_t piece,
        move_t** moves_head)
{
    move_t* moves = *moves_head;
    square_t to;
    if (piece_slide_type(piece) == NONE) {
        // not a sliding piece, just iterate over dest. squares
        for (const direction_t* delta = piece_deltas[piece]; *delta; ++delta) {
            to = from + *delta;
            if (pos->board[to] != EMPTY) continue;
            moves = add_move(pos,
                    create_move(from, to, piece, NONE),
                    moves);
        }
    } else {
        // a sliding piece, keep going until we hit something
        for (const direction_t* delta = piece_deltas[piece]; *delta; ++delta) {
            to = from + *delta;
            while (pos->board[to] == EMPTY) {
                moves = add_move(pos,
                        create_move(from, to, piece, NONE),
                        moves);
                to += *delta;
            }
        }
    }
    *moves_head = moves;
}

