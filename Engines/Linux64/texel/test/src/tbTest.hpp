/*
    Texel - A UCI chess engine.
    Copyright (C) 2014-2015  Peter Österlund, peterosterlund2@gmail.com

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
 * tbTest.hpp
 *
 *  Created on: Jun 2, 2014
 *      Author: petero
 */

#ifndef TBTEST_HPP_
#define TBTEST_HPP_

#include "suiteBase.hpp"

const std::string gtbDefaultPath("/home/petero/chess/gtb");
const int gtbDefaultCacheMB = 16;
const std::string rtbDefaultPath("/home/petero/chess/rtb/wdl:"
                                 "/home/petero/chess/rtb/dtz:"
                                 "/home/petero/chess/rtb/6wdl:"
                                 "/home/petero/chess/rtb/6dtz");

class TBTest : public SuiteBase {
public:
    std::string getName() const override { return "tbTest"; }

    cute::suite getSuite() const override;

    static void initTB(const std::string& gtbPath, int cacheMB,
                       const std::string& rtbPath);

private:
    static void dtmTest();
    static void kpkTest();
    static void rtbTest();
    static void tbTest();
    static void testMissingTables();
    static void testMaxSubMate();
};

#endif /* TBTEST_HPP_ */
