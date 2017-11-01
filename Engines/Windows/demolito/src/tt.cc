/*
 * Demolito, a UCI chess engine.
 * Copyright 2015 lucasart.
 *
 * Demolito is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Demolito is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <http://www.gnu.org/licenses/>.
*/
#include "tt.h"

namespace tt {

std::vector<Entry> table(1024 * 1024 / sizeof(Entry), 0);    // default=1MB (min Hash)

int score_to_tt(int score, int ply)
{
    return score >= mate_in(MAX_PLY) ? score + ply
           : score <= mated_in(MAX_PLY) ? score - ply
           : score;
}

int score_from_tt(int ttScore, int ply)
{
    return ttScore >= mate_in(MAX_PLY) ? ttScore - ply
           : ttScore <= mated_in(MAX_PLY) ? ttScore + ply
           : ttScore;
}

void clear()
{
    std::fill(table.begin(), table.end(), Entry(0));
}

bool read(uint64_t key, Entry& e)
{
    const size_t idx = key & (table.size() - 1);
    e = table[idx];
    return e.key == key;
}

void write(const Entry& e)
{
    Entry& replace = table[e.key & (table.size() - 1)];

    if (e.key != replace.key || e.depth >= replace.depth)
        replace = e;
}

}    // namespace tt
