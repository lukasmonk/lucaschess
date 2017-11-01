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
 * utilSuiteBase.hpp
 *
 *  Created on: May 29, 2015
 *      Author: petero
 */

#ifndef UTILSUITEBASE_HPP_
#define UTILSUITEBASE_HPP_

#include "cute_suite.h"

#include <string>

class UtilSuiteBase {
public:

    virtual ~UtilSuiteBase() {}

    /** Get the test suite name. */
    virtual std::string getName() const = 0;

    /** Get the test suite. */
    virtual cute::suite getSuite() const = 0;
};

#endif /* UTILSUITEBASE_HPP_ */
