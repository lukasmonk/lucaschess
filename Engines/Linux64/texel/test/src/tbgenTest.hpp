/*
    Texel - A UCI chess engine.
    Copyright (C) 2015  Peter Österlund, peterosterlund2@gmail.com

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
 * tbgenTest.hpp
 *
 *  Created on: Apr 4, 2015
 *      Author: petero
 */

#ifndef TBGENTEST_HPP_
#define TBGENTEST_HPP_

#include "suiteBase.hpp"

class PieceCount;


class TBGenTest : public SuiteBase {
public:
    std::string getName() const override { return "tbgenTest"; }

    cute::suite getSuite() const override;

private:
    static void testPositionValue();
    static void testTBIndex();
    static void testTBPosition();
    static void testMoveGen();
    static void testGenerate();
    static void testGenerateInternal(const PieceCount& pc);
};

#endif /* TBGENTEST_HPP_ */
