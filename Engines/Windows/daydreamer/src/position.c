
#include "daydreamer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/*
 * Set up the basic data structures of a position. Used internally by
 * set_position, but does not result in a legal board and should not be used
 * elsewhere.
 */
static void init_position(position_t* position)
{
    memset(position, 0, sizeof(position_t));
    position->board = position->_board_storage+64;
    for (int square=0; square<256; ++square) {
        position->_board_storage[square] = OUT_OF_BOUNDS;
    }
    for (int i=0; i<64; ++i) {
        int square = index_to_square(i);
        position->piece_index[square] = -1;
        position->board[square] = EMPTY;
    }

    for (color_t color=WHITE; color<=BLACK; ++color) {
        for (int index=0; index<32; ++index) {
            position->pieces[color][index] = INVALID_SQUARE;
        }
        for (int index=0; index<16; ++index) {
            position->pawns[color][index] = INVALID_SQUARE;
        }
    }
}

/*
 * Create a copy of |src| in |dst|.
 */
void copy_position(position_t* dst, const position_t* src)
{
    check_board_validity(src);
    memcpy(dst, src, sizeof(position_t));
    dst->board = dst->_board_storage+64;
    check_board_validity(src);
    check_board_validity(dst);
}

/*
 * Create a copy of |src| with the board flipped, inverting black and white.
 */
void flip_position(position_t* flipped, const position_t* src)
{
    init_position(flipped);
    // Note: we need to iterate over the piece lists like this instead of
    // just reading off the board array to ensure that the relative order
    // of pieces in the piece lists doesn't change in the flipped board. This
    // ensures that things like |static_exchange_eval| have consistent results
    // across the flip.
    for (int i=0; src->pawns[WHITE][i] != INVALID_SQUARE; ++i) {
        place_piece(flipped, BP, flip_square(src->pawns[WHITE][i]));
    }
    for (int i=0; src->pawns[BLACK][i] != INVALID_SQUARE; ++i) {
        place_piece(flipped, WP, flip_square(src->pawns[BLACK][i]));
    }
    for (int i=0; src->pieces[WHITE][i] != INVALID_SQUARE; ++i) {
        square_t sq = src->pieces[WHITE][i];
        piece_t p = src->board[sq];
        place_piece(flipped, flip_piece(p), flip_square(sq));
    }
    for (int i=0; src->pieces[BLACK][i] != INVALID_SQUARE; ++i) {
        square_t sq = src->pieces[BLACK][i];
        piece_t p = src->board[sq];
        place_piece(flipped, flip_piece(p), flip_square(sq));
    }
    flipped->side_to_move = src->side_to_move^1;
    if (src->ep_square) flipped->ep_square = flip_square(src->ep_square);
    if (has_ooo_rights(src, WHITE)) add_ooo_rights(flipped, BLACK);
    if (has_oo_rights(src, WHITE)) add_oo_rights(flipped, BLACK);
    if (has_ooo_rights(src, BLACK)) add_ooo_rights(flipped, WHITE);
    if (has_oo_rights(src, BLACK)) add_oo_rights(flipped, WHITE);
    flipped->fifty_move_counter = src->fifty_move_counter;
    flipped->ply = src->ply;
    set_hash(flipped);
}

/*
 * Given an FEN position description, set the given position to match it.
 * (see wikipedia.org/wiki/Forsyth-Edwards_Notation)
 */
char* set_position(position_t* pos, const char* fen)
{
    const char* orig_fen = fen;
    init_position(pos);
    king_home = E1;
    king_rook_home = H1;
    queen_rook_home = A1;

    // Read piece positions.
    for (square_t square=A8; square!=INVALID_SQUARE; ++fen, ++square) {
        if (isdigit(*fen)) {
            if (*fen == '0' || *fen == '9') {
                warn("Invalid FEN string");
                return (char*)fen;
            } else {
                square += *fen - '1';
            }
            continue;
        }
        switch (*fen) {
            case 'p': place_piece(pos, BP, square); break;
            case 'P': place_piece(pos, WP, square); break;
            case 'n': place_piece(pos, BN, square); break;
            case 'N': place_piece(pos, WN, square); break;
            case 'b': place_piece(pos, BB, square); break;
            case 'B': place_piece(pos, WB, square); break;
            case 'r': place_piece(pos, BR, square); break;
            case 'R': place_piece(pos, WR, square); break;
            case 'q': place_piece(pos, BQ, square); break;
            case 'Q': place_piece(pos, WQ, square); break;
            case 'k': place_piece(pos, BK, square); break;
            case 'K': place_piece(pos, WK, square); break;
            case '/': square -= 17 + square_file(square); break;
            case ' ': square = INVALID_SQUARE-1; break;
            case '\0':
            case '\n':set_hash(pos);
                      pos->is_check = find_checks(pos);
                      check_board_validity(pos);
                      return (char*)fen;
            default: warn("Illegal character in FEN string");
                     warn(orig_fen);
                     return (char*)fen;
        }
    }
    while (isspace(*fen)) ++fen;

    // Read whose turn is next.
    switch (tolower(*fen)) {
        case 'w': pos->side_to_move = WHITE; break;
        case 'b': pos->side_to_move = BLACK; break;
        default:  warn("Illegal side to move in FEN string");
                  return (char*)fen;
    }
    while (*fen && isspace(*(++fen))) {}
    pos->is_check = find_checks(pos);

    // Read castling rights. This is complicated by the need to support all
    // formats for castling flags in Chess960.
    while (*fen && !isspace(*fen)) {
        square_t sq;
        switch (*fen) {
            case 'q': add_ooo_rights(pos, BLACK);
                      for (sq=A8; pos->board[sq]!=BR && sq<=H8; ++sq) {}
                      if (pos->board[sq] == BR) {
                          queen_rook_home = square_file(sq);
                          king_home = square_file(pos->pieces[BLACK][0]);
                      } else warn("inconsistent castling rights");
                      break;
            case 'Q': add_ooo_rights(pos, WHITE);
                      for (sq=A1; pos->board[sq]!=WR && sq<=H1; ++sq) {}
                      if (pos->board[sq] == WR) {
                          queen_rook_home = sq;
                          king_home = pos->pieces[WHITE][0];
                      } else warn("inconsistent castling rights");
                      break;
            case 'k': add_oo_rights(pos, BLACK);
                      for (sq=H8; pos->board[sq]!=BR && sq>=A8; --sq) {}
                      if (pos->board[sq] == BR) {
                          king_rook_home = square_file(sq);
                          king_home = square_file(pos->pieces[BLACK][0]);
                      } else warn("inconsistent castling rights");
                      break;
            case 'K': add_oo_rights(pos, WHITE);
                      for (sq=H1; pos->board[sq]!=WR && sq>=A1; --sq) {}
                      if (pos->board[sq] == WR) {
                          king_rook_home = sq;
                          king_home = pos->pieces[WHITE][0];
                      } else warn("inconsistent castling rights");
                      break;
            case '-': break;
            default:
                // Chess960 castling flags.
                if (*fen >= 'A' && *fen <= 'H') {
                    king_home = pos->pieces[WHITE][0];
                    if (*fen - 'A' < king_home) {
                        add_ooo_rights(pos, WHITE);
                        queen_rook_home = *fen - 'A';
                    } else {
                        add_oo_rights(pos, WHITE);
                        king_rook_home = *fen - 'A';
                    }
                } else if (*fen >= 'a' && *fen <= 'h') {
                    king_home = square_file(pos->pieces[BLACK][0]);
                    if (*fen - 'a' < king_home) {
                        add_ooo_rights(pos, BLACK);
                        queen_rook_home = *fen - 'a';
                    } else {
                        add_oo_rights(pos, BLACK);
                        king_rook_home = *fen - 'a';
                    }
                } else {
                    // The fen string must have ended prematurely.
                    set_hash(pos);
                    check_board_validity(pos);
                    return (char*)fen;
                }
        }
        ++fen;
    }
    while (isspace(*fen)) ++fen;
    if (!*fen) {
        set_hash(pos);
        check_board_validity(pos);
        return (char*)fen;
    }

    // Read en-passant square
    if (*fen != '-') {
        square_t ep_sq = coord_str_to_square(fen++);
        piece_t pawn = create_piece(pos->side_to_move, PAWN);
        if (pos->board[ep_sq - pawn_push[pos->side_to_move] - 1] == pawn ||
                pos->board[ep_sq - pawn_push[pos->side_to_move] + 1] == pawn) {
            pos->ep_square = ep_sq;
        }
        if (*fen) ++fen;
    }
    while (*fen && isspace(*(++fen))) {}
    if (!*fen) {
        set_hash(pos);
        check_board_validity(pos);
        return (char*)fen;
    }

    // Read 50-move rule status and current move number.
    int consumed;
    if (sscanf(fen, "%d %d%n", &pos->fifty_move_counter, &pos->ply,
                &consumed)) fen += consumed;
    pos->ply = 0;
    set_hash(pos);
    check_board_validity(pos);
    return (char*)fen;
}

/*
 * In |pos|, is the side to move in check?
 */
bool is_check(const position_t* pos)
{
    return pos->is_check;
}

/*
 * Test a move's legality. There are no constraints on what kind of move it
 * is; it could be complete nonsense.
 */
bool is_move_legal(position_t* pos, move_t move)
{
    const color_t side = pos->side_to_move;
    const color_t other_side = pos->side_to_move^1;

    // Identify invalid/inconsistent moves.
    const square_t from = get_move_from(move);
    const square_t to = get_move_to(move);
    const piece_t piece = get_move_piece(move);
    const piece_t capture = get_move_capture(move);
    if (piece_type(capture) == KING) return false;
    if (!valid_board_index(from) || !valid_board_index(to)) return false;
    if (piece < WP || piece > BK || (piece > WK && piece < BP)) return false;
    if (pos->board[from] != piece) return false;
    if (capture && !is_move_enpassant(move)) {
        if (pos->board[to] != capture) return false;
    } else if (!(options.chess960 && is_move_castle(move))) {
        if (pos->board[to] != EMPTY) return false;
    }

    // See if any attacks are preventing castling.
    square_t my_king_home = king_home + A8*side;
    if (is_move_castle_long(move)) {
        if (!has_ooo_rights(pos, side)) return false;
        square_t my_d1 = D1 + side*A8;
        square_t my_c1 = C1 + side*A8;
        square_t my_qr = queen_rook_home + side*A8;
        bool castle_ok = true;
        // Check that rook is unimpeded.
        for (square_t sq = MIN(my_qr, my_d1); sq <= MAX(my_qr, my_d1); ++sq) {
            if (sq != my_qr && sq != my_king_home && pos->board[sq] != EMPTY) {
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
                if (is_square_attacked((position_t*)pos, sq, side^1)) {
                    castle_ok = false;
                    break;
                }
            }
        }
        return castle_ok;
    }
    if (is_move_castle_short(move)) {
        if (!has_oo_rights(pos, side)) return false;
        square_t my_f1 = F1 + side*A8;
        square_t my_g1 = G1 + side*A8;
        square_t my_kr = king_rook_home + side*A8;
        bool castle_ok = true;
        // Check that rook is unimpeded.
        for (square_t sq = MIN(my_kr, my_f1); sq <= MAX(my_kr, my_f1); ++sq) {
            if (sq != my_kr  && sq != my_king_home && pos->board[sq] != EMPTY) {
                castle_ok = false;
                break;
            }
        }
        // Check that the king is unimpeded and unattacked
        if (castle_ok) {
            for (square_t sq = MIN(my_king_home, my_g1); sq <= my_g1; ++sq) {
                if (sq != my_king_home && sq != my_kr &&
                        pos->board[sq] != EMPTY) {
                    castle_ok = false;
                    break;
                }
                if (is_square_attacked((position_t*)pos, sq, side^1)) {
                    castle_ok = false;
                    break;
                }
            }
        }
        return castle_ok;
    }

    // Just try the move and see if the king is being attacked afterwards.
    undo_info_t undo;
    do_move(pos, move, &undo);
    bool legal = !is_square_attacked(pos,
            pos->pieces[pos->side_to_move^1][0],
            other_side);
    undo_move(pos, move, &undo);
    return legal;
}

/*
 * A plausible move is one that was once pseudo-legal, but the board has
 * changed since the move was generated, so it may not be pseudo-legal any
 * more. Thus, we don't need to weed out total nonsense moves, but we do
 * need to do stricter legality checking.
 */
bool is_plausible_move_legal(position_t* pos, move_t move)
{
    assert(!is_check(pos));

    piece_t piece = get_move_piece(move);
    piece_t capture = get_move_capture(move);
    square_t to = get_move_to(move);
    square_t from = get_move_from(move);
    color_t side = piece_color(piece);
    // Make sure source and destination squares are legal.
    if (pos->board[from] != piece) return false;
    if (pos->board[to] != capture && !is_move_enpassant(move)) return false;

    // Make sure nothing's in the way of sliding pieces.
    if (piece_slide_type(piece) != NONE) {
        direction_t delta = direction(from, to);
        for (square_t sq = from + delta; sq != to; sq += delta) {
            if (pos->board[sq] != EMPTY) return false;
        }
    }

    square_t my_king_home = king_home + side*A8;
    if (!options.chess960) {
        if (is_move_castle_short(move) && !(has_oo_rights(pos, side) &&
                pos->board[my_king_home+1] == EMPTY &&
                pos->board[my_king_home+2] == EMPTY &&
                !is_square_attacked((position_t*)pos,my_king_home+1,side^1))) {
            return false;
        }
        if (is_move_castle_long(move) && !(has_ooo_rights(pos, side) &&
                pos->board[my_king_home-1] == EMPTY &&
                pos->board[my_king_home-2] == EMPTY &&
                pos->board[my_king_home-3] == EMPTY &&
                !is_square_attacked((position_t*)pos,my_king_home-1,side^1))) {
            return false;
        }
    } else {
        if (is_move_castle_short(move)) {
            if (!has_oo_rights(pos, side)) return false;
            square_t my_f1 = F1 + side*A8;
            square_t my_g1 = G1 + side*A8;
            square_t my_kr = king_rook_home + side*A8;
            // Check that rook is unimpeded.
            for (square_t sq = MIN(my_kr, my_f1);
                    sq <= MAX(my_kr, my_f1); ++sq) {
                if (sq != my_kr  && sq != my_king_home &&
                        pos->board[sq] != EMPTY) {
                    return false;
                }
            }
            // Check that the king is unimpeded and unattacked
            for (square_t sq = MIN(my_king_home, my_g1);
                    sq <= my_g1; ++sq) {
                if (sq != my_king_home && sq != my_kr &&
                        pos->board[sq] != EMPTY) {
                    return false;
                }
                if (sq != my_g1 &&
                        is_square_attacked((position_t*)pos, sq, side^1)) {
                    return false;
                }
            }
        }
        if (is_move_castle_long(move)) {
            if (!has_ooo_rights(pos, side)) return false;
            square_t my_d1 = D1 + side*A8;
            square_t my_c1 = C1 + side*A8;
            square_t my_qr = queen_rook_home + side*A8;
            // Check that rook is unimpeded.
            for (square_t sq = MIN(my_qr, my_d1);
                    sq <= MAX(my_qr, my_d1); ++sq) {
                if (sq != my_qr && sq != my_king_home &&
                        pos->board[sq] != EMPTY) {
                    return false;
                }
            }
            // Check that the king is unimpeded and unattacked
            for (square_t sq = MIN(my_king_home, my_c1);
                    sq <= MAX(my_king_home, my_c1); ++sq) {
                if (sq != my_king_home && sq != my_qr &&
                        pos->board[sq] != EMPTY) {
                    return false;
                }
                if (sq != my_c1 &&
                        is_square_attacked((position_t*)pos, sq, side^1)) {
                    return false;
                }
            }
        }
    }
    return is_pseudo_move_legal(pos, move);
}

/*
 * Test a pseudo-legal move's legality. For this, we only have to check that
 * it doesn't leave the king in check.
 */
bool is_pseudo_move_legal(position_t* pos, move_t move)
{
    if (!move) return false;
    piece_t piece = get_move_piece(move);
    square_t to = get_move_to(move);
    square_t from = get_move_from(move);
    color_t side = piece_color(piece);
    // Note: any pseudo-legal castle is legal if the king isn't attacked
    // afterwards, by design.
    // Also note: This is more complicated for Chess960,
    // since the rook may be shielding the king from check.
    if (options.chess960 && is_move_castle(move)) {
        piece_t my_r = create_piece(pos->side_to_move, ROOK);
        if (is_move_castle_long(move)) {
            square_t my_qr = queen_rook_home + A8*pos->side_to_move;
            assert(pos->board[my_qr] == my_r);
            pos->board[my_qr] = EMPTY;
            bool castle_ok = !is_square_attacked(pos, to, side^1);
            pos->board[my_qr] = my_r;
            return castle_ok;
        }
        assert(is_move_castle_short(move));
        square_t my_kr = king_rook_home + A8*pos->side_to_move;
        assert(pos->board[my_kr] == my_r);
        pos->board[my_kr] = EMPTY;
        bool castle_ok = !is_square_attacked(pos, to, side^1);
        pos->board[my_kr] = my_r;
        return castle_ok;
    }
    if (piece_is_type(piece, KING)) return !is_square_attacked(pos, to, side^1);

    // Avoid moving pinned pieces.
    direction_t pin_dir = pin_direction(pos, from, pos->pieces[side][0]);
    if (pin_dir) return abs(pin_dir) == abs(direction(from, to));

    // Resolving pins for en passant moves is a pain, and they're very rare.
    // I just do them the expensive way and don't worry about it.
    if (is_move_enpassant(move)) {
        undo_info_t undo;
        do_move(pos, move, &undo);
        bool legal = !is_square_attacked(pos, pos->pieces[side][0], side^1);
        undo_move(pos, move, &undo);
        return legal;
    }
    return true;
}

/*
 * Detect if the current position is a repetition of earlier game
 * positions.
 */
bool is_repetition(const position_t* pos)
{
    int max_age = MIN(pos->fifty_move_counter, pos->ply);
    for (int age = 2; age < max_age; age += 2) {
        assert(pos->ply >= age);
        if (pos->hash_history[pos->ply - age] == pos->hash) return true;
    }
    return false;
}

