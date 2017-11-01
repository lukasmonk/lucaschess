/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2013  Peter Österlund, peterosterlund2@gmail.com

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
 * random.cpp
 *
 *  Created on: Mar 3, 2012
 *      Author: petero
 */

#include "random.hpp"
#include "timeUtil.hpp"

Random::Random()
    : gen(currentTimeMillis()) {
}

Random::Random(U64 seed)
    : gen(seed) {
}

void
Random::setSeed(U64 seed) {
    gen.seed(seed);
}

int
Random::nextInt(int modulo) {
    std::uniform_int_distribution<int> dist(0, modulo-1);
    return dist(gen);
}

U64
Random::nextU64() {
    return gen();
}
