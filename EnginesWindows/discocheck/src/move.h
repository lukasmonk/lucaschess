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

enum {
	NORMAL,
	EN_PASSANT,
	PROMOTION,
	CASTLING
};

struct move_t {
	move_t(): b(0) {}	// silence compiler warnings
	explicit move_t(uint16_t _b): b(_b) {}

	operator bool() const;
	bool operator== (move_t m) const;
	bool operator!= (move_t m) const;

	int fsq() const;
	int tsq() const;
	int flag() const;
	int prom() const;

	void fsq(int new_fsq);
	void tsq(int new_tsq);
	void flag(int new_flag);
	void prom(int piece);

private:
	/* a move is incoded in 16 bits, as follows:
	 * 0..5: fsq (from square)
	 * 6..11: tsq (to square)
	 * 12,13: prom (promotion). Uses unusual numbering for optimal compactness: Knight=0 ... Queen=3
	 * 14,15: flag. Flags are: NORMAL=0, EN_PASSANT=1, PROMOTION=2, CASTLING=3 */
	uint16_t b;
};

enum { NO_CHECK, NORMAL_CHECK, DISCO_CHECK };

extern int is_check(const board::Board& B, move_t m);
extern bool is_cop(const board::Board& B, move_t m);	// capture or promotion
extern bool is_pawn_threat(const board::Board& B, move_t m);

extern move_t string_to_move(const board::Board& B, const std::string& s);
extern std::string move_to_string(move_t m);

extern int see(const board::Board& B, move_t m);
extern int mvv_lva(const board::Board& B, move_t m);

}	// namespace move

