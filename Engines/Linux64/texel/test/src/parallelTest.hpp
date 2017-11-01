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
 * parallelTest.hpp
 *
 *  Created on: Jul 17, 2013
 *      Author: petero
 */

#ifndef PARALLELTEST_HPP_
#define PARALLELTEST_HPP_

#include "suiteBase.hpp"

class ParallelTest : public SuiteBase {
public:
    std::string getName() const override { return "ParallelTest"; }

    cute::suite getSuite() const override;

private:
    static void testFailHighInfo();
    static void testNpsInfo();
    static void testWorkQueue();
    static void testWorkQueueParentChild();
    static void testSplitPointHolder();
    static void testWorkerThread();
};

#endif /* PARALLELTEST_HPP_ */
