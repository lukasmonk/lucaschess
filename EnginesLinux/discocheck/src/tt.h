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
#include "board.h"

enum { PV = 0, All = -1, Cut = +1 };

class TTable {
public:
	struct Entry {
		Key key_type;	// bit 0..1 for node_type+1, and 2..63 for key's 62 MSB
		mutable uint8_t generation;
		int8_t depth;
		int16_t score, eval;
		move::move_t move;

		int node_type() const {
			return (key_type & 3) - 1;
		}

		bool key_match(Key k) const {
			return (key_type & ~3ULL) == (k & ~3ULL);
		}

		void save(Key k, uint8_t g, int nt, int8_t d, int16_t s, int16_t e,
				  move::move_t m);
	};

	struct Cluster {
		Entry entry[4];
	};

	TTable(): count(0), cluster(nullptr) {}
	~TTable();

	void alloc(uint64_t size);
	void clear();

	void new_search();
	void refresh(const Entry *e) const {
		e->generation = generation;
	}

	const Entry *probe(Key key) const;
	void prefetch(Key key) const {
		__builtin_prefetch((char *)&cluster[key & (count - 1)]);
	}
	void store(Key key, int node_type, int8_t depth, int16_t score, int16_t eval, move::move_t move);

private:
	size_t count;
	uint8_t generation;
	Cluster *cluster;
};

