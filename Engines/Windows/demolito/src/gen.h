#pragma once
#include "position.h"
#include "move.h"

#define MAX_MOVES 192

namespace gen {

move_t *pawn_moves(const Position& pos, move_t *emList, bitboard_t targets,
                   bool subPromotions = true);
move_t *piece_moves(const Position& pos, move_t *emList, bitboard_t targets, bool kingMoves = true);
move_t *castling_moves(const Position& pos, move_t *emList);
move_t *check_escapes(const Position& pos, move_t *emList, bool subPromotions = true);

move_t *all_moves(const Position& pos, move_t *emList);

template <bool Root=true> uint64_t perft(const Position& pos, int depth);

}    // namespace gen
