
#include "daydreamer.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

extern const char glyphs[];
#define piece_type_char(x) glyphs[x]

#define AMBIG_NONE  0x00
#define AMBIG_RANK  0x01
#define AMBIG_FILE  0x02
typedef int ambiguity_t;

/*
 * Convert a square into ascii coordinates.
 */
int square_to_coord_str(square_t sq, char* str)
{
    *str++ = (char)square_file(sq) + 'a';
    *str++ = (char)square_rank(sq) + '1';
    *str = '\0';
    return 2;
}

/*
 * Convert an algebraic string representation of a square (e.g. A1, c6) to
 * a square_t.
 */
square_t coord_str_to_square(const char* alg_square)
{
    if (tolower(alg_square[0]) < 'a' || tolower(alg_square[0]) > 'h' ||
        alg_square[1] < '0' || alg_square[1] > '9') return EMPTY;
    return create_square(tolower(alg_square[0])-'a', alg_square[1]-'1');
}

/*
 * Convert a move to its coordinate string form.
 */
void move_to_coord_str(move_t move, char* str)
{
    if (move == NO_MOVE) {
        strcpy(str, "(none)");
        return;
    }
    if (move == NULL_MOVE) {
        strcpy(str, "0000");
        return;
    }
    square_t from = get_move_from(move);
    square_t to = get_move_to(move);
    if (options.chess960) {
        if (is_move_castle_long(move)) {
            if (options.arena_castle) {
                if (queen_rook_home != A1 || king_home != E1) {
                    strcpy(str, "O-O-O");
                    return;
                }
            } else to = queen_rook_home + get_move_piece_color(move)*A8;
        } else if (is_move_castle_short(move)) {
            if (options.arena_castle) {
                if (king_rook_home != H1 || king_home != E1) {
                    strcpy(str, "O-O");
                    return;
                }
            } else to = king_rook_home + get_move_piece_color(move)*A8;
        }
    }
    str += snprintf(str, 5, "%c%c%c%c",
           (char)square_file(from) + 'a', (char)square_rank(from) + '1',
           (char)square_file(to) + 'a', (char)square_rank(to) + '1');
    if (get_move_promote(move)) {
        snprintf(str, 2, "%c", tolower(glyphs[get_move_promote(move)]));
    }
}

/*
 * Convert a long algebraic move string (e.g. E2E4, c7c8q) to a move_t.
 * Only legal moves are generated--if the given move is impossible, NO_MOVE
 * is returned instead.
 */
move_t coord_str_to_move(position_t* pos, const char* coord_move)
{
    bool san_castle = false;
    square_t to, from;
    piece_type_t promote_type = NONE;
    if (options.arena_castle) {
        if (!strncmp(coord_move, "O-O-O", 5)) {
            san_castle = true;
            from = king_home + A8*pos->side_to_move;
            to = queen_rook_home + A8*pos->side_to_move;
        } else if (!strncmp(coord_move, "O-O", 3)) {
            san_castle = true;
            from = king_home + A8*pos->side_to_move;
            to = king_rook_home + A8*pos->side_to_move;
        }
    }
    if (!san_castle) {
        from = coord_str_to_square(coord_move);
        to = coord_str_to_square(coord_move + 2);
        if (from == INVALID_SQUARE || to == INVALID_SQUARE) return NO_MOVE;
        switch (*(coord_move+4)) {
            case 'n': case 'N': promote_type = KNIGHT; break;
            case 'b': case 'B': promote_type = BISHOP; break;
            case 'r': case 'R': promote_type = ROOK; break;
            case 'q': case 'Q': promote_type = QUEEN; break;
        }
    }

    move_t possible_moves[256];
    int num_moves = generate_legal_moves(pos, possible_moves);
    move_t move;
    for (int i=0; i<num_moves; ++i) {
        move = possible_moves[i];
        if (options.chess960 && is_move_castle(move)) {
            if (is_move_castle_long(move) &&
                    from == get_move_from(move) &&
                    to == (square_t)(queen_rook_home + A8*pos->side_to_move)) {
                return move;
            } else if (is_move_castle_short(move) &&
                    from == get_move_from(move) &&
                    to == (square_t)(king_rook_home + A8*pos->side_to_move)) {
                return move;
            }
            continue;
        }
        if ((piece_type_t)get_move_promote(move) == promote_type &&
                from == get_move_from(move) &&
                to == get_move_to(move)) return move;
    }
    return NO_MOVE;
}

/*
 * For a given move in a position, determine any ambiguities to be resolved in
 * the move's SAN representation. These are other pieces of the same type
 * on the same rank or file that can move to the destination square.
 */
static ambiguity_t determine_move_ambiguity(position_t* pos, move_t move)
{
    square_t dest = get_move_to(move);
    square_t from = get_move_from(move);
    rank_t from_rank = square_rank(from);
    file_t from_file = square_file(from);
    piece_type_t type = get_move_piece_type(move);
    move_t moves[256];
    generate_legal_moves(pos, moves);
    ambiguity_t ambiguity = AMBIG_NONE;
    for (move_t* other_move = moves; *other_move; ++other_move) {
        if (*other_move == move) continue;
        if (get_move_to(*other_move) != dest) continue;
        if (get_move_piece_type(*other_move) != type) continue;
        square_t other_from = get_move_from(*other_move);
        if (from == other_from) continue;
        if (square_rank(other_from) == from_rank) ambiguity |= AMBIG_RANK;
        if (square_file(other_from) == from_file) ambiguity |= AMBIG_FILE;
    }
    return ambiguity;
}

/*
 * Given a move in a position, produce a move string in standard algebraic
 * notation. Uses correct disambiguation rules, at least I think it does.
 */
int move_to_san_str(position_t* pos, move_t move, char* san)
{
    char* orig_san = san;
    if (move == NO_MOVE) {
        strcpy(san, "(none)");
        return 6;
    } else if (move == NULL_MOVE) {
        strcpy(san, "0000");
        return 6;
    }else if (is_move_castle_short(move)) {
        strcpy(san, "O-O");
        san += 3;
    } else if (is_move_castle_long(move)) {
        strcpy(san, "O-O-O");
        san += 5;
    } else {
        // type
        piece_type_t type = get_move_piece_type(move);
        ambiguity_t ambiguity = determine_move_ambiguity(pos, move);
        if (type != PAWN) {
            *san++ = piece_type_char(type);
        } else if (get_move_capture(move) && !(ambiguity & AMBIG_RANK)) {
            *san++ = square_file(get_move_from(move)) + 'a';
        }
        // source
        square_t from = get_move_from(move);
        if (ambiguity & AMBIG_RANK) *san++ = square_file(from) + 'a';
        if (ambiguity & AMBIG_FILE) *san++ = square_rank(from) + '1';
        // destination
        if (get_move_capture(move)) *san++ = 'x';
        san += square_to_coord_str(get_move_to(move), san);
        // promotion
        piece_type_t promote_type = get_move_promote(move);
        if (promote_type) {
            *san++ = '=';
            *san++ = piece_type_char(promote_type);
        }
    }
    // check or checkmate
    undo_info_t undo;
    do_move(pos, move, &undo);
    if (is_check(pos)) {
        move_t moves[256];
        int legal_moves = generate_legal_moves(pos, moves);
        *san++ = legal_moves ? '+' : '#';
    }
    undo_move(pos, move, &undo);
    *san = '\0';
    return san-orig_san;
}

/*
 * Converts the given move string in standard algebraic notation into a move
 * in engine format.
 */
move_t san_str_to_move(position_t* pos, char* san)
{
    move_t moves[256];
    generate_legal_moves(pos, moves);
    // Go ahead and take care of castles; they're easy.
    if (strcasestr(san, "O-O-O") ||
            strstr(san, "0-0-0") ||
            strcasestr(san, "OOO")) {
        for (move_t* move=moves; *move; ++move) {
            if (is_move_castle_long(*move)) return *move;
        }
        return NO_MOVE;
    } else if (strcasestr(san, "O-O") ||
            strstr(san, "0-0") ||
            strcasestr(san, "OO")) {
        for (move_t* move=moves; *move; ++move) {
            if (is_move_castle_short(*move)) return *move;
        }
    }
    char* end = san + strlen(san) - 1;
    if (end <= san) {
        warn("Unable to parse SAN input");
        return NO_MOVE;
    }
    piece_type_t promote_type = NONE;
    char* is_promote = strchr(san, '=');
    if (is_promote) {
        char* promote_pos = strchr(glyphs, toupper(*(is_promote+1)));
        promote_type = promote_pos ? promote_pos - glyphs : NONE;
        assert(promote_type <= QUEEN);
        end = is_promote - 1;
    } else if (*end == '+' || *end == '#') --end;
    file_t from_file=FILE_NONE, to_file=FILE_NONE;
    rank_t from_rank=RANK_NONE, to_rank=RANK_NONE;
    if (!(*end <= '8' && *end >= '1')) {
        warn("Unable to parse SAN input");
        return NO_MOVE;
    }
    to_rank = *end - '1';
    end--;
    if (!(*end <= 'h' && *end >= 'a')) {
        warn("Unable to parse SAN input");
        return NO_MOVE;
    }
    to_file = *end - 'a';
    end--;
    square_t to_sq = create_square(to_file, to_rank);

    char* piece_pos = strchr(glyphs, toupper(san[0]));
    piece_type_t piece_type = piece_pos ? piece_pos - glyphs : PAWN;
    san++;
    if (san <= end && *san <= 'h' && *san >= 'a') {
        from_file = *san - 'a';
        san++;
    }
    if (san <= end && *san <= '8' && *san >= '1') {
        from_rank = *san - '1';
    }
    for (move_t* move = moves; *move; ++move) {
        if (get_move_to(*move) != to_sq) continue;
        if (get_move_piece_type(*move) != piece_type) continue;
        if ((piece_type_t)get_move_promote(*move) != promote_type) continue;
        square_t from = get_move_from(*move);
        if (from_file != FILE_NONE && square_file(from) != from_file) continue;
        if (from_rank != RANK_NONE && square_rank(from) != from_rank) continue;
        return *move;
    }
    warn("Unable to parse SAN input");
    return NO_MOVE;
}

/*
 * Convert a list of moves in engine format to it's equivalent standard
 * algebraic notation string, with each move separated by a space.
 */
int line_to_san_str(position_t* pos, move_t* line, char* san)
{
    if (!*line) {
        *san = '\0';
        return 0;
    }
    assert2(is_move_legal(pos, *line));

    int len = move_to_san_str(pos, *line, san);
    *(san+len) = ' ';
    undo_info_t undo;
    do_move(pos, *line, &undo);
    len += line_to_san_str(pos, line+1, san+len+1);
    undo_move(pos, *line, &undo);
    return len;
}

/*
 * Convert a position to its FEN form.
 * (see wikipedia.org/wiki/Forsyth-Edwards_Notation)
 */
void position_to_fen_str(const position_t* pos, char* fen)
{
    int empty_run=0;
    for (square_t square=A8;; ++square) {
        if (empty_run && (pos->board[square] || !valid_board_index(square))) {
            *fen++ = empty_run + '0';
            empty_run = 0;
        }
        if (!valid_board_index(square)) {
            *fen++ = '/';
            square -= 0x19; // drop down to next rank
        } else if (pos->board[square]) {
            *fen++ = glyphs[pos->board[square]];
        } else empty_run++;
        if (square == H1) {
            if (empty_run) *fen++ = empty_run + '0';
            break;
        }
    }
    *fen++ = ' ';
    *fen++ = pos->side_to_move == WHITE ? 'w' : 'b';
    *fen++ = ' ';
    if (pos->castle_rights == CASTLE_NONE) *fen++ = '-';
    else if (options.chess960) {
        if (has_oo_rights(pos, WHITE)) *fen++ = king_rook_home + 'A';
        if (has_ooo_rights(pos, WHITE)) *fen++ = queen_rook_home + 'A';
        if (has_oo_rights(pos, BLACK)) *fen++ = king_rook_home + 'a';
        if (has_ooo_rights(pos, BLACK)) *fen++ = queen_rook_home + 'a';
    } else {
        if (has_oo_rights(pos, WHITE)) *fen++ = 'K';
        if (has_ooo_rights(pos, WHITE)) *fen++ = 'Q';
        if (has_oo_rights(pos, BLACK)) *fen++ = 'k';
        if (has_ooo_rights(pos, BLACK)) *fen++ = 'q';
    }
    *fen++ = ' ';
    if (pos->ep_square != EMPTY && valid_board_index(pos->ep_square)) {
        *fen++ = square_file(pos->ep_square) + 'a';
        *fen++ = square_rank(pos->ep_square) + '1';
    } else *fen++ = '-';
    *fen++ = ' ';
    fen += sprintf(fen, "%d", pos->fifty_move_counter);
    *fen++ = ' ';
    fen += sprintf(fen, "%d", (pos->ply+1)/2);
    *fen = '\0';
}

