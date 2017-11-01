/*
    Texel - A UCI chess engine.
    Copyright (C) 2013-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * timeUtil.cpp
 *
 *  Created on: Sep 20, 2013
 *      Author: petero
 */

#include "timeUtil.hpp"

#include <chrono>
#include <iostream>

#ifdef HAS_RT
#include <time.h>
#include <sys/time.h>
#endif

S64 currentTimeMillis() {
#ifdef HAS_RT
    clockid_t c = CLOCK_MONOTONIC;
    timespec sp;
    clock_gettime(c, &sp);
    return (S64)(sp.tv_sec * 1e3 + sp.tv_nsec * 1e-6);
#else
    using namespace std::chrono;
    auto t = high_resolution_clock::now();
    auto t0 = t.time_since_epoch();
    return duration_cast<milliseconds>(t0).count();
#endif
}

double currentTime() {
#ifdef HAS_RT
    clockid_t c = CLOCK_MONOTONIC;
    timespec sp;
    clock_gettime(c, &sp);
    return sp.tv_sec + sp.tv_nsec * 1e-9;
#else
    using namespace std::chrono;
    auto t = high_resolution_clock::now();
    auto t0 = t.time_since_epoch();
    return duration<double>(t0).count();
#endif
}

SampleStatistics&
SampleStatistics::operator+=(const SampleStatistics& other) {
    nSamples += other.nSamples;
    sum += other.sum;
    sqSum += other.sqSum;
    return *this;
}
