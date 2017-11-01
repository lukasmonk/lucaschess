/*
    Texel - A UCI chess engine.
    Copyright (C) 2012,2014  Peter Österlund, peterosterlund2@gmail.com

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
 * moveGenTest.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef MOVEGENTEST_HPP_
#define MOVEGENTEST_HPP_

#include "suiteBase.hpp"

class MoveGenTest : public SuiteBase {
public:
    std::string getName() const override { return "MoveGenTest"; }

    cute::suite getSuite() const override;
};

#endif /* MOVEGENTEST_HPP_ */
