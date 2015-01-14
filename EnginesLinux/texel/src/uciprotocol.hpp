/*
    Texel - A UCI chess engine.
    Copyright (C) 2012  Peter Österlund, peterosterlund2@gmail.com

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

#include <vector>
#include <string>
#include <iosfwd>

/**
 * Handle the UCI protocol mode.
 */
class UCIProtocol {
private:
    // Data set by the "position" command.
    Position pos;
    std::vector<Move> moves;

    // Engine data
    std::shared_ptr<EngineControl> engine;

    // Set to true to break out of main loop
    bool quit;

public:
    static void main(bool autoStart);

    UCIProtocol();

    void mainLoop(std::istream& is, std::ostream& os, bool autoStart);

private:
    void handleCommand(const std::string& cmdLine, std::ostream& os);

    void initEngine(std::ostream& os);

    /** Convert a string to tokens by splitting at whitespace characters. */
    void tokenize(const std::string& cmdLine, std::vector<std::string>& tokens);
};


#endif /* UCIPROTOCOL_HPP_ */
