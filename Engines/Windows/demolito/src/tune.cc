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
#include <cstring>    // std::memset()
#include <fstream>
#include <iostream>
#include <thread>
#include <cmath>
#include "eval.h"
#include "search.h"
#include "tt.h"
#include "tune.h"
#include "types.h"
#include "uci.h"

namespace {

std::vector<std::string> fens;
std::vector<double> scores;
std::vector<double> qsearches;

void idle_loop(int depth, int threadId)
{
    search::ThreadId = threadId;
    std::memset(PawnHash, 0, sizeof(PawnHash));

    Position pos;
    std::vector<move_t> pv(MAX_PLY + 1);

    for (size_t i = threadId; i < fens.size(); i += search::Threads) {
        pos.set(fens[i]);
        search::gameStack[threadId].clear();
        search::gameStack[threadId].push(pos.key());
        qsearches[i] = depth <= 0
                       ? search::recurse<true>(pos, 0, depth, -INF, INF, pv)
                       : search::recurse(pos, 0, depth, -INF, INF, pv);
        qsearches[i] /= double(EP);    // Rescale to Pawn = 1.0
    }
}

}    // namespace

namespace tune {

void load(const std::string& fileName)
{
    Clock c;
    c.reset();

    fens.clear();
    scores.clear();

    std::ifstream f(fileName);
    std::string s;

    while (std::getline(f, s)) {
        const auto i = s.find(',');

        if (i != std::string::npos) {
            fens.push_back(s.substr(0, i));
            scores.push_back(std::stod(s.substr(i + 1)));
        }
    }

    std::cout << "** loaded " << fens.size() << " positions in " << c.elapsed() / 1000.0 << "s\n";
}

void search(int depth, int threads, int hash)
{
    Clock c;
    c.reset();

    qsearches.resize(fens.size());

    tt::table.resize(hash * 1024 * (1024 / sizeof(tt::Entry)), 0);
    tt::clear();

    search::Threads = threads;
    search::gameStack.resize(threads);
    search::nodeCount.resize(threads);
    std::vector<std::thread> workers;

    uci::ui.clear();
    search::signal = 0;

    for (int i = 0; i < threads; i++) {
        search::nodeCount[i] = 0;
        workers.emplace_back(idle_loop, depth, i);
    }

    for (auto& t : workers)
        t.join();

    std::cout << "** qsearched " << fens.size() << " positions in " << c.elapsed() / 1000.0 << "s\n";
}

double error(double lambda)
{
    double sum = 0;

    for (size_t i = 0; i < fens.size(); i++) {
        const double logistic = 1 / (1.0 + std::exp(-lambda * qsearches[i]));
        sum += std::abs(scores[i] - logistic);
    }

    return sum / fens.size();
}

void logistic()
{
    double lambda = 0.4, e0 = error(lambda);
    double h = 0.01;

    while (true) {
        // Compute function in 3 points
        const double ep = error(lambda + h);
        const double em = error(lambda - h);

        // Deduce first and second order derivative estimates
        const double e1 = (ep - em) / (2 * h);
        const double e2 = (ep - 2 * e0 + em) / (h * h);

        // New Lambda is the minimum of the tangent parabola
        const double newLambda = (e2 * lambda - e1) / e2;

        // Error and difference for the next iteration
        e0 = error(newLambda);
        h = (newLambda - lambda) / 10;

        std::cout << "lambda = " << newLambda << ", error(lambda) = " << e0 << '\n';

        if (std::abs(newLambda - lambda) < 0.00001)
            break;

        lambda = newLambda;
    }
}

}    // namespace tune
