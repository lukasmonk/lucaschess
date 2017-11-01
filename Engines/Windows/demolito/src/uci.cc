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
#include <sstream>
#include <thread>
#include "uci.h"
#include "eval.h"
#include "search.h"
#include "tt.h"
#include "gen.h"

zobrist::GameStack gameStack;

namespace {

Position pos;
search::Limits lim;
std::thread Timer;

size_t Hash = 1;
int TimeBuffer = 30;

void intro()
{
    std::cout << "id name Demolito\nid author lucasart\n" << std::boolalpha
              << "option name UCI_Chess960 type check default " << Chess960 << '\n'
              << "option name Hash type spin default " << Hash << " min 1 max 1048576\n"
              << "option name Threads type spin default " << search::Threads << " min 1 max 64\n"
              << "option name Contempt type spin default " << search::Contempt << " min -100 max 100\n"
              << "option name Time Buffer type spin default " << TimeBuffer << " min 0 max 1000\n"
              << "uciok" << std::endl;
}

void setoption(std::istringstream& is)
{
    std::string token, name;

    is >> token;

    if (token != "name")
        return;

    while ((is >> token) && token != "value")
        name += token;

    if (name == "UCI_Chess960")
        is >> std::boolalpha >> Chess960;
    else if (name == "Hash") {
        is >> Hash;
        Hash = 1ULL << bb::msb(Hash);    // must be a power of two
        tt::table.resize(Hash * 1024 * (1024 / sizeof(tt::Entry)), 0);
    } else if (name == "Threads")
        is >> search::Threads;
    else if (name == "Contempt")
        is >> search::Contempt;
    else if (name == "TimeBuffer")
        is >> TimeBuffer;
}

void position(std::istringstream& is)
{
    Position p[NB_COLOR];
    int idx = 0;

    std::string token, fen;
    is >> token;

    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        is >> token;    // consume "moves" token (if present)
    } else if (token == "fen") {
        while (is >> token && token != "moves")
            fen += token + " ";
    } else
        return;

    p[idx].set(fen);
    gameStack.clear();
    gameStack.push(p[idx].key());

    // Parse moves (if any)
    while (is >> token) {
        const Move m(p[idx], token);
        p[idx ^ 1].set(p[idx], m);
        idx ^= 1;
        gameStack.push(p[idx].key());
    }

    pos = p[idx];
}

void go(std::istringstream& is)
{
    std::string token;
    lim.movestogo = 30;

    while (is >> token) {
        if (token == "depth")
            is >> lim.depth;
        else if (token == "nodes")
            is >> lim.nodes;
        else if (token == "movetime") {
            is >> lim.movetime;
            lim.movetime -= TimeBuffer;
        } else if (token == "movestogo")
            is >> lim.movestogo;
        else if ((pos.turn() == WHITE && token == "wtime") || (pos.turn() == BLACK && token == "btime"))
            is >> lim.time;
        else if ((pos.turn() == WHITE && token == "winc") || (pos.turn() == BLACK && token == "binc"))
            is >> lim.inc;
    }

    if (lim.time || lim.inc) {
        lim.movetime = ((lim.movestogo - 1) * lim.inc + lim.time) / lim.movestogo;
        lim.movetime = std::min(lim.movetime, lim.time - TimeBuffer);
    }

    if (Timer.joinable())
        Timer.join();

    Timer = std::thread(search::bestmove, std::cref(pos), std::cref(lim), std::cref(gameStack));
}

void eval()
{
    print(pos);
    std::cout << "score " << uci::format_score(evaluate(pos)) << std::endl;
}

void perft(std::istringstream& is)
{
    int depth;
    is >> depth;

    print(pos);
    std::cout << "score " << gen::perft(pos, depth) << std::endl;
}

}    // namespace

namespace uci {

Info ui;

void loop()
{
    std::string command, token;

    while (std::getline(std::cin, command)) {
        std::istringstream is(command);
        is >> token;

        if (token == "uci")
            intro();
        else if (token == "setoption")
            setoption(is);
        else if (token == "isready")
            std::cout << "readyok" << std::endl;
        else if (token == "ucinewgame")
            tt::clear();
        else if (token == "position")
            position(is);
        else if (token == "go")
            go(is);
        else if (token == "stop")
            search::signal = STOP;
        else if (token == "eval")
            eval();
        else if (token == "perft")
            perft(is);
        else if (token == "quit")
            break;
        else
            std::cout << "unknown command: " << command << std::endl;
    }

    if (Timer.joinable())
        Timer.join();
}

void Info::clear()
{
    lastDepth = 0;
    bestMove = ponderMove = 0;
    clock.reset();
}

void Info::update(const Position& pos, int depth, int score, int nodes, std::vector<move_t>& pv,
                  bool partial)
{
    std::lock_guard<std::mutex> lk(mtx);

    if (depth > lastDepth) {
        bestMove = pv[0];
        ponderMove = pv[1];

        if (partial)
            return;

        lastDepth = depth;

        std::ostringstream os;
        const auto elapsed = clock.elapsed() + 1;  // Prevent division by zero

        os << "info depth " << depth << " score " << format_score(score)
           << " time " << elapsed << " nodes " << nodes
           << " nps " << (1000 * nodes / elapsed) << " pv";

        Position p[2];
        int idx = 0;
        p[idx] = pos;

        for (int i = 0; pv[i]; i++) {
            Move m(pv[i]);
            os << ' ' << m.to_string(p[idx]);
            p[idx ^ 1].set(p[idx], m);
            idx ^= 1;
        }

        std::cout << os.str() << std::endl;
    }
}

void Info::print_bestmove(const Position& pos) const
{
    std::lock_guard<std::mutex> lk(mtx);
    std::cout << "bestmove " << bestMove.to_string(pos)
              << " ponder " << ponderMove.to_string(pos) << std::endl;
}

Move Info::best_move() const
{
    std::lock_guard<std::mutex> lk(mtx);
    return bestMove;
}

std::string format_score(int score)
{
    std::ostringstream os;

    if (is_mate_score(score)) {
        const int dtm = score > 0
                        ? (MATE - score + 1) / 2
                        : (score - MATE + 1) / 2;
        os << "mate " << dtm;
    } else
        os << "cp " << score * 100 / EP;

    return os.str();
}

}    // namespace uci
