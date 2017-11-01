/*
    Texel - A UCI chess engine.
    Copyright (C) 2012,2015  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * uciprotocol.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef UCIPROTOCOL_HPP_
#define UCIPROTOCOL_HPP_

#include "position.hpp"
#include "enginecontrol.hpp"
#include "search.hpp"

#include <vector>
#include <string>
#include <iosfwd>

/**
 * This class is responsible for sending "info" strings during search.
 */
class SearchListener : public Search::Listener {
public:
    explicit SearchListener(std::ostream& os);

    void notifyDepth(int depth) override;

    void notifyCurrMove(const Move& m, int moveNr) override;

    void notifyPV(int depth, int score, S64 time, S64 nodes, S64 nps, bool isMate,
                  bool upperBound, bool lowerBound, const std::vector<Move>& pv,
                  int multiPVIndex, S64 tbHits) override;

    void notifyStats(S64 nodes, S64 nps, int hashFull, S64 tbHits, S64 time) override;

    void notifyPlayedMove(const Move& bestMove, const Move& ponderMove);

private:
    static std::string moveToString(const Move& m);

    std::ostream& os;
};

/**
 * Handle the UCI protocol mode.
 */
class UCIProtocol {
public:
    static void main(bool autoStart);

    UCIProtocol(std::istream& is, std::ostream& os);

    void mainLoop(bool autoStart);

private:
    void handleCommand(const std::string& cmdLine, std::ostream& os);

    void initEngine(std::ostream& os);

    /** Convert a string to tokens by splitting at whitespace characters. */
    void tokenize(const std::string& cmdLine, std::vector<std::string>& tokens);

    // Input/output streams
    std::istream& is;
    std::ostream& os;

    // Data set by the "position" command.
    Position pos;
    std::vector<Move> moves;

    // Engine data
    std::unique_ptr<EngineControl> engine;
    SearchListener searchListener;
    EngineMainThread engineThread;

    // Set to true to break out of main loop
    bool quit;
};


#endif /* UCIPROTOCOL_HPP_ */
