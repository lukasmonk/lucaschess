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
#include <cstring>
#include "movegen.h"
#include "move.h"

struct SearchInfo {
	move::move_t m, best, killer[2];
	int ply, reduction, eval;
	bool skip_null, null_child;

	void clear(int _ply);
};

/* History heuristic (quite move ordering):
 * - moves are sorted by descending h[color][piece][tsq] (after TT, killers, refutation).
 * - when a move fails high, its history score is incremented by depth*depth.
 * - for all other moves searched before, decrement their history score by depth*depth.
 * */
class History {
public:
	static const int Max = 2000;

	void clear();
	void add(const board::Board& B, move::move_t m, int bonus);
	int get(const board::Board& B, move::move_t m) const;

private:
	int h[NB_COLOR][NB_PIECE][NB_SQUARE];
};

/* Double Move Refutation Hash Table:
 * - used for quiet move ordering, and as an LMR guard.
 * - move pair (m1, m2) -> move m3 that refuted the sequence (m1, m2) last time it was visited by the
 * search.
 * - dm_key is the zobrist key of the last 2 moves (see board::Board::get_dm_key(), in particular floor at
 * root sp0).
 * - always overwrite: fancy ageing schemes, or seveal slots per move pair did not work in testing.
 * */
class Refutation {
public:
	void clear();
	move::move_t get_refutation(Key dm_key) const;
	void set_refutation(Key dm_key, move::move_t m);

private:
	struct Entry {
		uint64_t dm_key: 48;
		move::move_t move;
	};

	static const int count = 0x10000;
	Entry r[count];
};

class MoveSort {
public:
	enum GenType {
		GEN_ALL,				// all legal moves
		GEN_CAPTURES_CHECKS,	// captures and quiet checks
		GEN_CAPTURES			// captures only
	};

	struct Token {
		int score, see;
		move::move_t m;

		bool operator< (const Token& t) const {
			return score < t.score;
		}
	};

	MoveSort(const board::Board* _B, int _depth, const SearchInfo *_ss,
			 const History *_H, const Refutation *_R);

	move::move_t next(int *see);
	move::move_t previous();

	int get_count() const {
		return count;
	}

private:
	const board::Board *B;
	GenType type;
	const SearchInfo *ss;
	const History *H;
	const Refutation *R;
	move::move_t refutation;

	Token list[MAX_MOVES];
	int idx, count, depth;

	move::move_t *generate(move::move_t *mlist);
	void annotate(const move::move_t *mlist);
	void score(MoveSort::Token *t);
};

