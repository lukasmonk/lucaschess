/*
 * DiscoCheck, an UCI chess engine. Copyright (C) 2011-2013 Lucas Braesch.
 *
 * DiscoCheck is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * DiscoCheck is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include "types.h"

namespace board {
class Board;
}
namespace move {
struct move_t;
}

#define MAX_MOVES	0x80	// max number of legal moves

namespace movegen {

extern move::move_t *gen_piece_moves(const board::Board& B, Bitboard targets, move::move_t *mlist, bool king_moves);
extern move::move_t *gen_castling(const board::Board& B, move::move_t *mlist);
extern move::move_t *gen_pawn_moves(const board::Board& B, Bitboard targets, move::move_t *mlist, bool sub_promotions);
extern move::move_t *gen_evasion(const board::Board& B, move::move_t *mlist);
extern move::move_t *gen_quiet_checks(const board::Board& B, move::move_t *mlist);
extern move::move_t *gen_moves(const board::Board& B, move::move_t *mlist);

}	// namespace movegen

