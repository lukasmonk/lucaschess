/*
    Texel - A UCI chess engine.
    Copyright (C) 2013  Peter Österlund, peterosterlund2@gmail.com

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
 * logger.cpp
 *
 *  Created on: Aug 6, 2013
 *      Author: petero
 */

#include "logger.hpp"

std::mutex& Logger::getLogMutex() {
    static std::mutex m;
    return m;
}


std::ostream& Logger::getOutStream() {
    if (false) {
        static std::ofstream out("/home/petero/texel.log");
        return out;
    } else {
        return std::cout;
    }
}
