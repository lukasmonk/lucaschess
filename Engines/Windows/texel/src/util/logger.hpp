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
 * logger.hpp
 *
 *  Created on: Aug 6, 2013
 *      Author: petero
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include "util.hpp"
#include "timeUtil.hpp"

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <mutex>


namespace Logger {
    /** Get mutex for log synchronization. */
    std::mutex& getLogMutex();

    /** Get stream to log to. */
    std::ostream& getOutStream();

    /** Thread-safe logging to cout. */
    template <typename Func> void log(Func func) {
        std::stringstream ss;
        {
            std::stringstream t;
            t.precision(6);
            t << std::fixed << currentTime() << ' ';
            ss << t.str();
        }
        func(ss);
        std::lock_guard<std::mutex> L(getLogMutex());
        getOutStream() << ss.str() << std::endl;
    }
};

#define LOG(x) Logger::log([&](std::ostream& os) { os << x; })

#endif /* LOGGER_HPP_ */
