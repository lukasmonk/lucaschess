#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include "move.h"

namespace uci {

void loop();

class Info {
public:
    void clear();
    void update(const Position& pos, int depth, int score, int nodes, std::vector<move_t>& pv,
                bool partial = false);
    void print_bestmove(const Position& pos) const;

    Move best_move() const;

    // Do not need synchronization
    Clock clock;  // read-only during search
    std::atomic<int> lastDepth;

private:
    // Require synchronization
    Move bestMove, ponderMove;
    mutable std::mutex mtx;
};

extern Info ui;

std::string format_score(int score);

}    // namespace UCI
