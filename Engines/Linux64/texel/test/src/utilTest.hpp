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
 * utilTest.hpp
 *
 *  Created on: Sep 21, 2013
 *      Author: petero
 */

#ifndef UTILTEST_HPP_
#define UTILTEST_HPP_

#include "suiteBase.hpp"

class UtilTest : public SuiteBase {
public:
    std::string getName() const override { return "UtilTest"; }

    cute::suite getSuite() const override;

private:
    static void testUtil();
    static void testSampleStat();
    static void testTime();
    static void testRangeSumArray();
    static void testHeap();
    static void testHistogram();
    static void testFloorLog2();
};


#endif /* UTILTEST_HPP_ */
