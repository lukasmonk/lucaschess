
#include "daydreamer.h"
#include <string.h>

/*
 * Do some basic consistency checking on |pos| to identify bugs.
 */
void _check_board_validity(const position_t* pos)
{
    assert(pos->board[pos->pieces[WHITE][0]] == WK);
    assert(pos->board[pos->pieces[BLACK][0]] == BK);
    assert(pos->num_pawns[WHITE] <= 8);
    assert(pos->num_pawns[BLACK] <= 8);
    int my_piece_count[16];
    memset(my_piece_count, 0, 16 * sizeof(int));
    for (square_t sq=A1; sq<=H8; ++sq) {
        if (!valid_board_index(sq) || !pos->board[sq]) continue;
        piece_t piece = pos->board[sq];
        color_t side = piece_color(piece);
        (void)side;
        my_piece_count[piece]++;
        if (piece_is_type(piece, PAWN)) {
            assert(pos->pawns[side][pos->piece_index[sq]] == sq);
        } else {
            assert(pos->pieces[side][pos->piece_index[sq]] == sq);
        }
    }
    assert(my_piece_count[WK] == 1);
    assert(my_piece_count[BK] == 1);
    for (int i=0; i<16; ++i) {
        assert(my_piece_count[i] == pos->piece_count[i]);
    }
    for (int i=0; i<pos->num_pieces[0]; ++i) {
        assert(pos->piece_index[pos->pieces[0][i]] == i);
    }
    for (int i=0; i<pos->num_pawns[0]; ++i) {
        assert(pos->piece_index[pos->pawns[0][i]] == i);
    }
    for (int i=0; i<pos->num_pieces[1]; ++i) {
        assert(pos->piece_index[pos->pieces[1][i]] == i);
    }
    for (int i=0; i<pos->num_pawns[1]; ++i) {
        assert(pos->piece_index[pos->pawns[1][i]] == i);
    }
    assert(hash_position(pos) == pos->hash);
    assert(hash_pawns(pos) == pos->pawn_hash);
    assert(hash_material(pos) == pos->material_hash);
}

/*
 * Perform some sanity checks on |move| to flag obviously invalid moves.
 */
void _check_move_validity(const position_t* pos, move_t move)
{
    const square_t from = get_move_from(move);
    const square_t to = get_move_to(move);
    const piece_t piece = get_move_piece(move);
    const piece_t capture = get_move_capture(move);
    (void)pos,(void)move;                           // Avoid warning when
    (void)from,(void)to,(void)piece,(void)capture;  // NDEBUG is defined.
    assert(valid_board_index(from) && valid_board_index(to));
    assert(pos->board[from] == piece);
    if (capture && !is_move_enpassant(move)) {
        assert(pos->board[to] == capture);
    } else if (!(options.chess960 && is_move_castle(move))) {
        assert(pos->board[to] == EMPTY);
    }
}

/*
 * Make sure that legality testing for pseudo-moves gives the same result
 * as the more expensive testing.
 */
void _check_pseudo_move_legality(position_t* pos, move_t move)
{
    (void)pos; (void)move;
    bool legal = is_move_legal(pos, move);
    bool pseudo_legal = is_pseudo_move_legal(pos, move);
    (void)legal; (void)pseudo_legal;
    assert(legal == pseudo_legal);
}

/*
 * Check to see if |pos|'s incremental hash is correct.
 */
void _check_position_hash(const position_t* pos)
{
    (void)pos; // Avoid warning when NDEBUG is defined.
    assert(hash_position(pos) == pos->hash);
}

/*
 * Verify that a given list of moves in valid in the given position.
 */
void _check_line(position_t* pos, move_t* moves)
{
    if (moves[0] == NO_MOVE) return;
    undo_info_t undo;
    if (moves[0] == NULL_MOVE) do_nullmove(pos, &undo);
    else do_move(pos, moves[0], &undo);
    _check_line(pos, moves+1);
    if (moves[0] == NULL_MOVE) undo_nullmove(pos, &undo);
    else undo_move(pos, moves[0], &undo);
}

/*
 * Verify that flipping the board doesn't change the evaluation.
 */
void _check_eval_symmetry(const position_t* pos, int normal_eval)
{
    eval_data_t ed;
    position_t flipped_pos;
    flip_position(&flipped_pos, pos);
    int flipped_eval = full_eval(&flipped_pos, &ed);
    if (normal_eval != flipped_eval) {
        printf("Asymmetric eval. Original:\n");
        print_board(pos, false);
        printf("Asymmetric eval. Flipped:\n");
        print_board(&flipped_pos, false);
        assert(false);
    }
}
