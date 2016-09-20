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
 *
 * Credits:
 * - TT entry replacement scheme replicates what Stockfish does. Thanks to Tord Romstad and Marco
 * Costalba.
*/
#include <cstdlib>
#include <cstring>
#include "tt.h"
#include "move.h"

namespace {

void *aligned_malloc(size_t size, size_t align)
{
	void *mem = malloc(size + (align - 1) + sizeof(void*));
	if (!mem) throw std::bad_alloc();

	char *amem = ((char*)mem) + sizeof(void*);
	amem += align - ((std::uintptr_t)amem & (align - 1));

	((void**)amem)[-1] = mem;
	return amem;
}

void aligned_free(void *mem)
{
	free(((void**)mem)[-1]);
}

}	// namespace

TTable::~TTable()
{
	if (count)
		aligned_free(cluster);

	cluster = nullptr;
	generation = 0;
	count = 0;
}

void TTable::alloc(uint64_t size)
{
	// calculate the number of clusters allocate (count must be a power of two)
	size_t new_count = 1ULL << bb::msb(size / sizeof(Cluster));

	// nothing to do if already allocated to the given size
	if (new_count == count)
		return;

	if (cluster)
		aligned_free(cluster);

	// Allocate the cluster array. On failure, std::bad_alloc is thrown and not caught, which
	// terminates the program. It's not a bug, it's a "feature".
	cluster = (Cluster *)aligned_malloc(new_count * sizeof(Cluster), 64);

	count = new_count;
	clear();
}

void TTable::clear()
{
	std::memset(cluster, 0, count * sizeof(Cluster));
	generation = 0;
}

void TTable::new_search()
{
	++generation;
}

const TTable::Entry *TTable::probe(Key key) const
{
	const Entry *e = &cluster[key & (count - 1)].entry[0];

	for (size_t i = 0; i < 4; ++i, ++e)
		if (e->key_match(key))
			return e;

	return nullptr;
}

void TTable::Entry::save(Key k, uint8_t g, int nt, int8_t d, int16_t s, int16_t e,
						 move::move_t m)
{
	key_type = (k & ~3ULL) ^ (nt + 1);
	generation = g;
	depth = d;
	score = s;
	eval = e;
	move = m;
}

void TTable::store(Key key, int node_type, int8_t depth, int16_t score, int16_t eval, move::move_t move)
{
	Entry *e = cluster[key & (count - 1)].entry, *replace = e;

	if (node_type == All)
		move = move::move_t(0);

	for (size_t i = 0; i < 4; ++i, ++e) {
		// overwrite empty or old
		if (!e->key_type || e->key_match(key)) {
			replace = e;
			if (!move)
				move = e->move;
			break;
		}

		// Stockfish replacement strategy
		int c1 = generation == replace->generation ? 2 : 0;
		int c2 = e->generation == generation || e->node_type() == PV ? -2 : 0;
		int c3 = e->depth < replace->depth ? 1 : 0;
		if (c1 + c2 + c3 > 0)
			replace = e;
	}

	replace->save(key, generation, node_type, depth, score, eval, move);
}
