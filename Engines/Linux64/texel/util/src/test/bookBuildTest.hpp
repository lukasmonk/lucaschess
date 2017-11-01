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
 * bookBuildTest.hpp
 *
 *  Created on: May 29, 2015
 *      Author: petero
 */

#ifndef BOOKBUILDTEST_HPP_
#define BOOKBUILDTEST_HPP_

#include "utilSuiteBase.hpp"

class BookBuildTest : public UtilSuiteBase {
    std::string getName() const override { return "BookBuildTest"; }

    cute::suite getSuite() const override;
private:
    static void testBookNode();
    static void testShortestDepth();
    static void testBookNodeDAG();
    static void testAddPosToBook();
    static void testAddPosToBookConnectToChild();
    static void testSelector();
};

#endif /* BOOKBUILDTEST_HPP_ */
