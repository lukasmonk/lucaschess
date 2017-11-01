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
 * humanPlayer.cpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#include "humanPlayer.hpp"
#include "position.hpp"

#include <iostream>


std::string
HumanPlayer::getCommand(const Position& pos, bool drawOffer, const std::vector<Position>& history) {
    const char* color = pos.isWhiteMove() ? "white" : "black";
    std::cout << "Enter move (" << color << "):" << std::flush;
    std::string moveStr;
    getline(std::cin, moveStr);
    if (!std::cin || std::cin.eof())
        return "quit";
    if (moveStr.length() == 0) {
        return lastCmd;
    } else {
        lastCmd = moveStr;
    }
    return moveStr;
}
