
#include "daydreamer.h"

/*
 * Modify |pos| by adding |piece| to |square|. If |square| is occupied, its
 * occupant is properly removed.
 */
void place_piece(position_t* pos, piece_t piece, square_t square)
{
    if (pos->board[square]) {
        remove_piece(pos, square);
    }
    color_t color = piece_color(piece);
    piece_type_t type = piece_type(piece);
    (void)type;
    assert(color == WHITE || color == BLACK);
    assert(type >= PAWN && type <= KING);
    assert(square != INVALID_SQUARE);

    pos->board[square] = piece;
    if (piece_is_type(piece, PAWN)) {
        int index = pos->num_pawns[color]++;
        pos->pawns[color][index] = square;
        pos->piece_index[square] = index;
        pos->pawn_hash ^= piece_hash(piece, square);
    } else {
        int index = pos->num_pieces[color]++;
        pos->pieces[color][index+1] = INVALID_SQUARE;
        for (; index>0 && pos->board[pos->pieces[color][index-1]] < piece;
                --index) {
            square_t sq = pos->pieces[color][index-1];
            pos->pieces[color][index] = sq;
            pos->piece_index[sq] = index;
        }
        pos->pieces[color][index] = square;
        pos->piece_index[square] = index;
    }
    pos->hash ^= piece_hash(piece, square);
    pos->material_hash ^= material_hash(piece, pos->piece_count[piece]);
    pos->material_eval[color] += material_value(piece);
    pos->piece_square_eval[color].midgame += piece_square_value(piece, square);
    pos->piece_square_eval[color].endgame +=
        endgame_piece_square_value(piece, square);
    pos->piece_count[piece]++;
}

/*
 * Modify |pos| by removing any occupant from |square|.
 */
void remove_piece(position_t* pos, square_t square)
{
    assert(pos->board[square]);
    piece_t piece = pos->board[square];
    color_t color = piece_color(piece);
    
    if (piece_is_type(piece, PAWN)) {
        int index = --pos->num_pawns[color];
        int position = pos->piece_index[square];
        square_t sq = pos->pawns[color][index];
        pos->pawns[color][position] = sq;
        pos->pawns[color][index] = INVALID_SQUARE;
        pos->piece_index[sq] = position;
        pos->pawn_hash ^= piece_hash(piece, square);
    } else {
        int index = --pos->num_pieces[color];
        for (; index>0 && pos->pieces[color][index] != square; --index) {}
        for (; index <= pos->num_pieces[color]; ++index) {
            square_t sq = pos->pieces[color][index+1];
            pos->pieces[color][index] = sq;
            pos->piece_index[sq] = index;
        }
    }
    pos->board[square] = EMPTY;
    pos->piece_index[square] = -1;
    pos->piece_count[piece]--;
    pos->hash ^= piece_hash(piece, square);
    pos->material_hash ^= material_hash(piece, pos->piece_count[piece]);
    pos->material_eval[color] -= material_value(piece);
    pos->piece_square_eval[color].midgame -= piece_square_value(piece, square);
    pos->piece_square_eval[color].endgame -=
        endgame_piece_square_value(piece, square);
}

/*
 * Modify |pos| by moving the occupant of |from| to |to|. If there is already
 * a piece at |to|, it is removed.
 */
void transfer_piece(position_t* pos, square_t from, square_t to)
{
    assert(pos->board[from]);
    if (from == to) return;
    if (pos->board[to] != EMPTY) {
        remove_piece(pos, to);
    }

    piece_t p = pos->board[from];
    pos->board[to] = pos->board[from];
    int index = pos->piece_index[to] = pos->piece_index[from];
    color_t color = piece_color(p);
    pos->board[from] = EMPTY;
    if (piece_is_type(p, PAWN)) {
        pos->pawns[color][index] = to;
        pos->piece_index[to] = index;
        pos->pawn_hash ^= piece_hash(p, from);
        pos->pawn_hash ^= piece_hash(p, to);
    } else {
        pos->pieces[color][index] = to;
        pos->piece_index[to] = index;
    }
    pos->piece_square_eval[color].midgame -= piece_square_value(p, from);
    pos->piece_square_eval[color].midgame += piece_square_value(p, to);
    pos->piece_square_eval[color].endgame -=
        endgame_piece_square_value(p, from);
    pos->piece_square_eval[color].endgame +=
        endgame_piece_square_value(p, to);
    pos->hash ^= piece_hash(p, from);
    pos->hash ^= piece_hash(p, to);
}

/*
 * Modify |pos| by performing the given |move|. The information needed to undo
 * this move is preserved in |undo|.
 */
void do_move(position_t* pos, move_t move, undo_info_t* undo)
{
    check_move_validity(pos, move);
    check_board_validity(pos);
    // Set undo info, so we can roll back later.
    undo->is_check = pos->is_check;
    undo->check_square = pos->check_square;
    undo->prev_move = pos->prev_move;
    undo->ep_square = pos->ep_square;
    undo->fifty_move_counter = pos->fifty_move_counter;
    undo->castle_rights = pos->castle_rights;
    undo->hash = pos->hash;

    // xor old data out of the hash key
    pos->hash ^= ep_hash(pos);
    pos->hash ^= castle_hash(pos);
    pos->hash ^= side_hash(pos);

    const color_t side = pos->side_to_move;
    const color_t other_side = side^1;
    const square_t from = get_move_from(move);
    const square_t to = get_move_to(move);
    assert(valid_board_index(from) && valid_board_index(to));
    pos->ep_square = EMPTY;
    ++pos->fifty_move_counter;
    if (piece_type(get_move_piece(move)) == PAWN) {
        if (relative_rank[side][square_rank(to)] -
                relative_rank[side][square_rank(from)] != 1) {
            piece_t opp_pawn = create_piece(other_side, PAWN);
            if (pos->board[to-1] == opp_pawn || pos->board[to+1] == opp_pawn) {
                pos->ep_square = from + pawn_push[side];
            }
        }
        pos->fifty_move_counter = 0;
    } else if (get_move_capture(move) != EMPTY) {
        pos->fifty_move_counter = 0;
    }

    // Remove castling rights as necessary.
    if (from == (square_t)(queen_rook_home + side*A8)) {
        remove_ooo_rights(pos, side);
    } else if (from == (square_t)(king_rook_home + side*A8)) {
        remove_oo_rights(pos, side);
    } else if (from == (square_t)(king_home + side*A8))  {
        remove_oo_rights(pos, side);
        remove_ooo_rights(pos, side);
    } 
    if (to == (square_t)(queen_rook_home + other_side*A8)) {
        remove_ooo_rights(pos, other_side);
    } else if (to == (square_t)(king_rook_home + other_side*A8)) {
        remove_oo_rights(pos, other_side);
    }

    if (!is_move_castle(move)) transfer_piece(pos, from, to);

    const piece_type_t promote_type = get_move_promote(move);
    if (is_move_castle_short(move)) {
        assert(pos->board[king_home + A8*side] == create_piece(side, KING));
        assert(pos->board[king_rook_home + A8*side] ==
                create_piece(side, ROOK));
        remove_piece(pos, king_home + A8*side);
        transfer_piece(pos, king_rook_home + A8*side, F1 + A8*side);
        place_piece(pos, create_piece(side, KING), G1 + A8*side);
    } else if (is_move_castle_long(move)) {
        assert(pos->board[king_home + A8*side] == create_piece(side, KING));
        assert(pos->board[queen_rook_home + A8*side] ==
                create_piece(side, ROOK));
        remove_piece(pos, king_home + A8*side);
        transfer_piece(pos, queen_rook_home + A8*side, D1 + A8*side);
        place_piece(pos, create_piece(side, KING), C1 + A8*side);
    } else if (is_move_enpassant(move)) {
        remove_piece(pos, to-pawn_push[side]);
    } else if (promote_type) {
        place_piece(pos, create_piece(side, promote_type), to);
    }

    pos->hash_history[pos->ply++] = undo->hash;
    assert(pos->ply <= HASH_HISTORY_LENGTH);
    pos->side_to_move ^= 1;
    pos->is_check = find_checks(pos);
    pos->prev_move = move;
    pos->hash ^= ep_hash(pos);
    pos->hash ^= castle_hash(pos);
    pos->hash ^= side_hash(pos);
    check_board_validity(pos);
}

/*
 * Undo the effects of |move| on |pos|, using the undo info generated by
 * do_move().
 */
void undo_move(position_t* pos, move_t move, undo_info_t* undo)
{
    check_board_validity(pos);
    const color_t side = pos->side_to_move^1;
    const square_t from = get_move_from(move);
    const square_t to = get_move_to(move);

    // Move the piece back, and fix en passant captures.
    if (!is_move_castle(move)) transfer_piece(pos, to, from);
    piece_type_t captured = get_move_capture(move);
    if (captured != EMPTY) {
        if (is_move_enpassant(move)) {
            place_piece(pos, create_piece(side^1, PAWN), to-pawn_push[side]);
        } else {
            place_piece(pos, create_piece(pos->side_to_move, captured), to);
        }
    }

    // Un-promote/castle, if necessary.
    const piece_type_t promote_type = get_move_promote(move);
    if (is_move_castle_short(move)) {
        remove_piece(pos, G1 + A8*side);
        transfer_piece(pos, F1 + A8*side, king_rook_home + A8*side);
        place_piece(pos, create_piece(side, KING), king_home + A8*side);
    } else if (is_move_castle_long(move)) {
        remove_piece(pos, C1 + A8*side);
        transfer_piece(pos, D1 + A8*side, queen_rook_home + A8*side);
        place_piece(pos, create_piece(side, KING), king_home + A8*side);
    } else if (promote_type) {
        place_piece(pos, create_piece(side, PAWN), from);
    }

    // Reset non-board state information.
    pos->side_to_move ^= 1;
    pos->ply--;
    pos->is_check = undo->is_check;
    pos->check_square = undo->check_square;
    pos->ep_square = undo->ep_square;
    pos->fifty_move_counter = undo->fifty_move_counter;
    pos->castle_rights = undo->castle_rights;
    pos->prev_move = undo->prev_move;
    pos->hash = undo->hash;
    check_board_validity(pos);
}

/*
 * Perform a nullmove, aka pass. Not a legal move, but useful for search
 * heuristics.
 */
void do_nullmove(position_t* pos, undo_info_t* undo)
{
    assert(!pos->is_check);
    check_board_validity(pos);
    undo->ep_square = pos->ep_square;
    undo->castle_rights = pos->castle_rights;
    undo->prev_move = pos->prev_move;
    undo->fifty_move_counter = pos->fifty_move_counter;
    undo->hash = pos->hash;
    undo->is_check = pos->is_check;
    undo->check_square = pos->check_square;
    pos->hash ^= ep_hash(pos);
    pos->hash ^= side_hash(pos);
    pos->side_to_move ^= 1;
    pos->hash ^= side_hash(pos);
    pos->ep_square = EMPTY;
    pos->hash ^= ep_hash(pos);
    pos->fifty_move_counter++;
    pos->hash_history[pos->ply++] = undo->hash;
    pos->prev_move = NULL_MOVE;
    check_board_validity(pos);
}

/*
 * Undo the effects of a nullmove.
 */
void undo_nullmove(position_t* pos, undo_info_t* undo)
{
    assert(!pos->is_check);
    check_board_validity(pos);
    pos->ep_square = undo->ep_square;
    pos->castle_rights = undo->castle_rights;
    pos->prev_move = undo->prev_move;
    pos->side_to_move ^= 1;
    pos->fifty_move_counter = undo->fifty_move_counter;
    pos->hash = undo->hash;
    pos->is_check = undo->is_check;
    pos->check_square = undo->check_square;
    pos->ply--;
    check_board_validity(pos);
}

