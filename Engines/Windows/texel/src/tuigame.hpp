/*
    Texel - A UCI chess engine.
    Copyright (C) 2012,2014-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * tuigame.hpp
 *
 *  Created on: Mar 4, 2012
 *      Author: petero
 */

#ifndef TUIGAME_HPP_
#define TUIGAME_HPP_

#include "game.hpp"

#include <memory>

/**
 * Handles a game played using a text interface.
 */
class TUIGame : public Game {
public:
    TUIGame(std::unique_ptr<Player>&& whitePlayer,
            std::unique_ptr<Player>&& blackPlayer);

    /**
     * Administrate a game between two players, human or computer.
     */
    void play();

protected:
    bool handleCommand(const std::string& moveStr) override;

private:
    void showHelp();

    void handleTestSuite(const std::string& cmd);
};


#endif /* TUIGAME_HPP_ */
