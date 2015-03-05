/*
    Texel - A UCI chess engine.
    Copyright (C) 2012,2014  Peter Österlund, peterosterlund2@gmail.com

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
 * texel.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "computerPlayer.hpp"
#include "humanPlayer.hpp"
#include "tuigame.hpp"
#include "treeLogger.hpp"
#include "uciprotocol.hpp"
#include "numa.hpp"

#include <memory>

/**
 * Texel chess engine main function.
 */
int main(int argc, char* argv[]) {
    ComputerPlayer::initEngine();
    if ((argc == 2) && (std::string(argv[1]) == "txt")) {
        std::shared_ptr<Player> whitePlayer = std::make_shared<HumanPlayer>();
        std::shared_ptr<ComputerPlayer> blackPlayer = std::make_shared<ComputerPlayer>();
        blackPlayer->setTTLogSize(21);
        TUIGame game(whitePlayer, blackPlayer);
        game.play();
    } else if ((argc == 3) && (std::string(argv[1]) == "tree")) {
        TreeLoggerReader::main(argv[2]);
    } else {
        if ((argc == 2) && (std::string(argv[1]) == "-nonuma"))
            Numa::instance().disable();
        UCIProtocol::main(false);
    }
}
