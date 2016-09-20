/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * constants.hpp
 *
 *  Created on: Mar 2, 2012
 *      Author: petero
 */

#ifndef CONSTANTS_HPP_
#define CONSTANTS_HPP_

namespace SearchConst {
    const int MATE0 = 32000;
    const int UNKNOWN_SCORE = -32767; // Represents unknown static eval score
    const int plyScale = 8; // Fractional ply resolution
    const int MIN_SMP_DEPTH = 10; // Minimum depth for SMP work sharing
    const int MAX_SP_PER_THREAD = 32; // Maximum number of SplitPoints per thread

    inline bool isWinScore(int score) { return score > MATE0 / 2; }
    inline bool isLoseScore(int score) { return score < -(MATE0 / 2); }
}

namespace TType {
    const int T_EMPTY = 0;   // Empty hash slot
    const int T_EXACT = 1;   // Exact score
    const int T_GE = 2;      // True score >= this score
    const int T_LE = 3;      // True score <= this score
}

#endif /* CONSTANTS_HPP_ */
