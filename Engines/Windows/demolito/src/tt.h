#pragma once
#include <vector>
#include "move.h"

namespace tt {

enum {LBOUND, EXACT, UBOUND};

struct Entry {
    uint64_t key;
    int16_t score, eval, move;
    int8_t depth, bound;

    Entry() = default;
    Entry(uint64_t v) { key = v; }
};

// Adjust mate scores to plies from current position, instead of plies from root
int score_to_tt(int score, int ply);
int score_from_tt(int ttScore, int ply);

void clear();
bool read(uint64_t key, Entry& e);
void write(const Entry& e);

extern std::vector<Entry> table;

}    // namespace tt
