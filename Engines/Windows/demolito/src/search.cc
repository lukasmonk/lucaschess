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
#include <thread>
#include <vector>
#include <chrono>
#include <cstring>  // std::memset
#include "search.h"
#include "sort.h"
#include "eval.h"
#include "uci.h"
#include "zobrist.h"
#include "tt.h"

namespace {

// Protect thread scheduling decisions
std::mutex mtxSchedule;

}    // namespace

namespace search {

// Set at thread creation, so each thread can know its unique id
thread_local int ThreadId;

// Per thread data
std::vector<zobrist::GameStack> gameStack;
std::vector<uint64_t> nodeCount;

uint64_t nodes()
{
    uint64_t total = 0;

    for (uint64_t n : nodeCount)
        total += n;

    return total;
}

std::atomic<uint64_t> signal;    // signal: bit #i is set if thread #i should stop
enum Abort {
    ABORT_NEXT,    // current thread aborts the current iteration to be scheduled to the next one
    ABORT_STOP    // current thread aborts the current iteration to stop iterating completely
};

const int Tempo = 16;

int Threads = 1;
int Contempt = 10;

int draw_score(int ply)
{
    return (ply & 1 ? Contempt : -Contempt) * EP / 100;
}

template<bool Qsearch = false>
int recurse(const Position& pos, int ply, int depth, int alpha, int beta, std::vector<move_t>& pv)
{
    assert(gameStack[ThreadId].back() == pos.key());
    assert(alpha < beta);

    const bool pvNode = beta > alpha + 1;
    const int oldAlpha = alpha;
    const Color us = pos.turn();
    int bestScore = -INF;
    Move bestMove = 0;
    int score;
    Position nextPos;

    if (!Qsearch) {
        const uint64_t s = signal.load(std::memory_order_relaxed);

        if (s) {
            if (s == STOP)
                throw ABORT_STOP;
            else if (s & (1ULL << ThreadId))
                throw ABORT_NEXT;
        }
    }

    std::vector<move_t> childPv;

    if (pvNode) {
        childPv.reserve(MAX_PLY - ply);
        pv[0] = 0;
    }

    if (ply > 0 && (gameStack[ThreadId].repetition(pos.rule50()) || insufficient_material(pos)))
        return draw_score(ply);

    // TT probe
    tt::Entry tte;
    int staticEval, refinedEval;

    if (tt::read(pos.key(), tte)) {
        tte.score = tt::score_from_tt(tte.score, ply);

        if (tte.depth >= depth && ply > 0) {
            if (tte.score <= alpha && tte.bound >= tt::EXACT)
                return tte.score;
            else if (tte.score >= beta && tte.bound <= tt::EXACT)
                return tte.score;
            else if (alpha < tte.score && tte.score < beta && tte.bound == tt::EXACT)
                return tte.score;
        }

        if (!Qsearch && tte.depth <= 0)
            tte.move = 0;

        refinedEval = staticEval = tte.eval;

        if ((tte.score > refinedEval && tte.bound <= tt::EXACT)
                || (tte.score < refinedEval && tte.bound >= tt::EXACT))
            refinedEval = tte.score;
    } else {
        tte.move = 0;
        refinedEval = staticEval = pos.checkers() ? -INF : evaluate(pos) + Tempo;
    }

    // At Root, ensure that the last best move is searched first. This is not guaranteed,
    // as the TT entry could have got overriden by other search threads.
    if (!Qsearch && ply == 0 && uci::ui.lastDepth > 0)
        tte.move = uci::ui.best_move();

    nodeCount[ThreadId]++;

    if (ply >= MAX_PLY)
        return refinedEval;

    // Null search
    if (!Qsearch && depth >= 2 && !pvNode
            && staticEval >= beta && pos.piece_material(us)) {
        nextPos.toggle(pos);
        gameStack[ThreadId].push(nextPos.key());
        const int nextDepth = depth - (3 + depth/4);
        score = nextDepth <= 0
                ? -recurse<true>(nextPos, ply+1, nextDepth, -beta, -(beta-1), childPv)
                : -recurse(nextPos, ply+1, nextDepth, -beta, -(beta-1), childPv);
        gameStack[ThreadId].pop();

        if (score >= beta)
            return score >= mate_in(MAX_PLY) ? beta : score;
    }

    // QSearch stand pat
    if (Qsearch && !pos.checkers()) {
        bestScore = refinedEval;

        if (bestScore > alpha) {
            alpha = bestScore;

            if (bestScore >= beta)
                return bestScore;
        }
    }

    // Generate and score moves
    Selector S(pos, depth, tte.move);

    size_t moveCount = 0;
    Move currentMove;

    // Move loop
    while (!S.done() && alpha < beta) {
        int see;
        currentMove = S.select(pos, see);

        if (!currentMove.pseudo_is_legal(pos))
            continue;

        moveCount++;

        // Prune losing captures in the qsearch
        if (Qsearch && see < 0 && !pos.checkers())
            continue;

        // SEE proxy tells us we're unlikely to beat alpha
        if (Qsearch && !pos.checkers() && staticEval + P/2 <= alpha && see <= 0)
            continue;

        // Play move
        nextPos.set(pos, currentMove);

        // Prune losing captures in the search, near the leaves
        if (!Qsearch && depth <= 4 && see < 0
                && !pvNode && !pos.checkers() && !nextPos.checkers())
            continue;

        gameStack[ThreadId].push(nextPos.key());

        const int ext = see >= 0 && nextPos.checkers();
        const int nextDepth = depth - 1 + ext;

        // Recursion
        if (Qsearch || nextDepth <= 0) {
            // Qsearch recursion (plain alpha/beta)
            if (depth <= MIN_DEPTH && !pos.checkers()) {
                score = staticEval + see;    // guard against QSearch explosion

                if (pvNode)
                    childPv[0] = 0;
            } else
                score = -recurse<true>(nextPos, ply+1, nextDepth, -beta, -alpha, childPv);
        } else {
            // Search recursion (PVS + Reduction)
            if (moveCount == 1)
                score = -recurse(nextPos, ply+1, nextDepth, -beta, -alpha, childPv);
            else {
                int reduction = see < 0 || (!currentMove.is_capture(pos) && !nextPos.checkers());

                if (!currentMove.is_capture(pos) && !pos.checkers() && !nextPos.checkers())
                    reduction++;

                // Reduced depth, zero window
                score = nextDepth - reduction <= 0
                        ? -recurse<true>(nextPos, ply+1, nextDepth - reduction, -alpha-1, -alpha, childPv)
                        : -recurse(nextPos, ply+1, nextDepth - reduction, -alpha-1, -alpha, childPv);

                // Fail high: re-search zero window at full depth
                if (reduction && score > alpha)
                    score = -recurse(nextPos, ply+1, nextDepth, -alpha-1, -alpha, childPv);

                // Fail high at full depth for pvNode: re-search full window
                if (pvNode && alpha < score && score < beta)
                    score = -recurse(nextPos, ply+1, nextDepth, -beta, -alpha, childPv);
            }
        }

        // Undo move
        gameStack[ThreadId].pop();

        // New best score
        if (score > bestScore) {
            bestScore = score;

            // New alpha
            if (score > alpha) {
                alpha = score;
                bestMove = currentMove;

                if (pvNode) {
                    pv[0] = currentMove;

                    for (int i = 0; i < MAX_PLY - ply; i++)
                        if (!(pv[i + 1] = childPv[i]))
                            break;

                    if (!Qsearch && ply == 0 && uci::ui.lastDepth > 0)
                        uci::ui.update(pos, depth, score, nodes(), pv, true);
                }
            }
        }
    }

    // No legal move: mated or stalemated
    if ((!Qsearch || pos.checkers()) && !moveCount)
        return pos.checkers() ? mated_in(ply) : draw_score(ply);

    // TT write
    tte.key = pos.key();
    tte.bound = bestScore <= oldAlpha ? tt::UBOUND : bestScore >= beta ? tt::LBOUND : tt::EXACT;
    tte.score = tt::score_to_tt(bestScore, ply);
    tte.eval = pos.checkers() ? -INF : staticEval;
    tte.depth = depth;
    tte.move = bestMove;
    tt::write(tte);

    return bestScore;
}

int aspirate(const Position& pos, int depth, std::vector<move_t>& pv, int score)
{
    if (depth <= 1) {
        assert(depth == 1);
        return recurse(pos, 0, depth, -INF, +INF, pv);
    }

    int delta = 32;
    int alpha = score - delta;
    int beta = score + delta;

    for ( ; ; delta += delta) {
        score = recurse(pos, 0, depth, alpha, beta, pv);

        if (score <= alpha) {
            beta = (alpha + beta) / 2;
            alpha -= delta;
        } else if (score >= beta) {
            alpha = (alpha + beta) / 2;
            beta += delta;
        } else
            return score;
    }
}

void iterate(const Position& pos, const Limits& lim, const zobrist::GameStack& initialGameStack,
             std::vector<int>& iteration, int threadId)
{
    ThreadId = threadId;
    std::vector<move_t> pv(MAX_PLY + 1);
    int score;

    std::memset(PawnHash, 0, sizeof(PawnHash));

    for (int depth = 1; depth <= lim.depth; depth++) {
        {
            std::lock_guard<std::mutex> lk(mtxSchedule);

            // If half of the threads are searching >= depth, then move to the next iteration.
            // Special cases where this does not apply:
            // depth == 1: we want all threads to finish depth == 1 asap.
            // depth == lim.depth: there is no next iteration.
            if (Threads >= 2 && depth >= 2 && depth < lim.depth) {
                int cnt = 0;

                for (int i = 0; i < Threads; i++)
                    cnt += i != ThreadId && iteration[i] >= depth;

                if (cnt >= Threads / 2)
                    continue;
            }

            iteration[ThreadId] = depth;

            if (signal == STOP)
                return;

            signal &= ~(1ULL << ThreadId);
        }

        try {
            score = aspirate(pos, depth, pv, score);

            // Iteration was completed normally. Now we need to see who is working on
            // obsolete iterations, and raise the appropriate signal, to make them move
            // on to the next iteration.
            {
                std::lock_guard<std::mutex> lk(mtxSchedule);
                uint64_t s = 0;

                for (int i = 0; i < Threads; i++)
                    if (i != ThreadId && iteration[i] == depth)
                        s |= 1ULL << i;

                signal |= s;
            }
        } catch (const Abort e) {
            assert(signal & (1ULL << ThreadId));
            gameStack[ThreadId] = initialGameStack;    // Restore an orderly state

            if (e == ABORT_STOP)
                break;
            else {
                assert(e == ABORT_NEXT);
                continue;
            }
        }

        uci::ui.update(pos, depth, score, nodes(), pv);
    }

    // Max depth completed by current thread. All threads should stop.
    std::lock_guard<std::mutex> lk(mtxSchedule);
    signal = STOP;
}

void bestmove(const Position& pos, const Limits& lim, const zobrist::GameStack& initialGameStack)
{
    using namespace std::chrono;
    const auto start = high_resolution_clock::now();

    uci::ui.clear();
    signal = 0;
    std::vector<int> iteration(Threads, 0);
    gameStack.resize(Threads);
    nodeCount.resize(Threads);

    std::vector<std::thread> threads;

    for (int i = 0; i < Threads; i++) {
        // Initialize per-thread data
        gameStack[i] = initialGameStack;
        nodeCount[i] = 0;

        // Start searching thread
        threads.emplace_back(iterate, std::cref(pos), std::cref(lim), std::cref(initialGameStack),
                             std::ref(iteration), i);
    }

    do {
        std::this_thread::sleep_for(milliseconds(5));

        // Check for search termination conditions, but only after depth 1 has been
        // completed, to make sure we do not return an illegal move.
        if (uci::ui.lastDepth > 0) {
            if (lim.nodes && nodes() >= lim.nodes) {
                std::lock_guard<std::mutex> lk(mtxSchedule);
                signal = STOP;
            } else if (lim.movetime && duration_cast<milliseconds>
                       (high_resolution_clock::now() - start).count() >= lim.movetime) {
                std::lock_guard<std::mutex> lk(mtxSchedule);
                signal = STOP;
            }
        }
    } while (signal != STOP);

    for (auto& t : threads)
        t.join();

    uci::ui.print_bestmove(pos);
}

}    // namespace search
