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
#include "bitboard.h"
#include "zobrist.h"
#include "test.h"
#include "pst.h"
#include "uci.h"
#include "tune.h"

int main(int argc, char **argv)
{
    bb::init();
    zobrist::init();
    pst::init();

    if (argc >= 2) {
        const std::string cmd(argv[1]);

        if (cmd == "see")
            std::cout << "\nSEE: " << (test::see(true) ? "ok" : "failed") << std::endl;
        else if ((cmd == "perft" || cmd == "search") && argc >= 4) {
            const int depth = std::stoi(argv[2]), threads = std::stoi(argv[3]);
            const uint64_t nodes = test::bench(cmd == "perft", depth, threads);
            std::cout << "total = " << nodes << std::endl;
        } else if (cmd == "logistic" && argc == 5) {
            tune::load(argv[2]);
            tune::search(0, std::stoi(argv[3]), std::stoi(argv[4]));
            tune::logistic();
        }
    } else
        uci::loop();
}
