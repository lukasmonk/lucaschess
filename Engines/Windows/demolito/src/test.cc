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
#include <iostream>
#include "test.h"
#include "search.h"
#include "gen.h"
#include "uci.h"

namespace test {

uint64_t bench(bool perft, int depth, int threads)
{
    const std::string fens[] = {
        "r1bqkbnr/pp1ppppp/2n5/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
        "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
        "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
        "1rbqk1nr/p3ppbp/2np2p1/2p5/1p2PP2/3PB1P1/PPPQ2BP/R2NK1NR b KQk - 0 1",
        "r1bqk2r/pp1p1ppp/2n1pn2/2p5/1bPP4/2NBP3/PP2NPPP/R1BQK2R b KQkq - 0 1",
        "rnb1kb1r/ppp2ppp/1q2p3/4P3/2P1Q3/5N2/PP1P1PPP/R1B1KB1R b KQkq - 0 1",
        "r1b2rk1/pp2nppp/1b2p3/3p4/3N1P2/2P2NP1/PP3PBP/R3R1K1 b - - 0 1",
        "n1q1r1k1/3b3n/p2p1bp1/P1pPp2p/2P1P3/2NBB2P/3Q1PK1/1R4N1 b - - 0 1",
        "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
        "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
        "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
        "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
        "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
        "2r5/8/1n6/1P1p1pkp/p2P4/R1P1PKP1/8/1R6 w - - 0 1",
        "r2q1rk1/1b1nbppp/4p3/3pP3/p1pP4/PpP2N1P/1P3PP1/R1BQRNK1 b - - 0 1",
        "6k1/5pp1/7p/p1p2n1P/P4N2/6P1/1P3P1K/8 w - - 0 35",
        "r4rk1/1pp1q1pp/p2p4/3Pn3/1PP1Pp2/P7/3QB1PP/2R2RK1 b - - 0 1"
    };

    uint64_t result = 0, nodes;
    search::Limits lim;
    lim.depth = depth;
    search::Threads = threads;
    Position pos;
    zobrist::GameStack gameStack;

    Clock clock;
    clock.reset();

    for (const std::string& fen : fens) {
        pos.set(fen);
        gameStack.clear();
        gameStack.push(pos.key());
        print(pos);

        if (perft) {
            nodes = gen::perft(pos, depth);
            std::cout << "perft(" << depth << ") = " << nodes << std::endl;
        } else {
            search::bestmove(pos, lim, gameStack);
            nodes = search::nodes();
        }

        std::cout << std::endl;
        result += nodes;
    }

    if (dbgCnt[1])
        std::cout << "dbgCnt[0] = " << dbgCnt[0] << ", dbgCnt[1] = " << dbgCnt[1] << '\n';

    std::cout << "kn/s: " << result / clock.elapsed() << std::endl;

    return result;
}

bool see(bool verbose)
{
    struct TestSEE {
        std::string fen, move;
        int value;
    };

    TestSEE test[] = {
        {"k6K/8/4b3/8/3N4/8/8/8 w - -", "d4e6", B},
        {"k6K/3p4/4b3/8/3N4/8/8/8 w - -", "d4e6", 0},
        {"k6K/3p4/4b3/8/3N4/8/8/4R3 w - -", "d4e6", B - N + P},
        {"k3r2K/3p4/4b3/8/3N4/8/4R3/4R3 w - -", "d4e6", B - N + P},
        {"k6K/3P4/8/8/8/8/8/8 w - -", "d7d8q", Q - P},
        {"k6K/3P4/2n5/8/8/8/8/8 w - -", "d7d8q", -P},
        {"k6K/3P4/2n1N3/8/8/8/8/8 w - -", "d7d8q", -P + N},
        {"k6K/3PP3/2n5/b7/7B/8/8/3R4 w - -", "d7d8q", N + B - 2*P},
        {"3R3K/k3P3/8/b7/8/8/8/8 b - -", "a5d8", R - B + P - Q},
        {"8/4k3/8/8/RrR1N2r/8/5K2/8 b - - 11 1 ", "h4e4", N - R}
    };

    Position pos;

    for (auto& t : test) {
        pos.set(t.fen);
        const Move m(pos, t.move);
        const int value = m.see(pos);

        if (verbose) {
            std::cout << '\n';
            print(pos);
        } else
            std::cout << t.fen << '\t';

        std::cout << t.move << "\tSEE = " << value << std::endl;

        if (value != t.value)
            return false;
    }

    return true;
}

}    // namespace test
