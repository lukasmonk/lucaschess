/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

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
 * largePageAlloc.hpp
 *
 *  Created on: Oct 19, 2016
 *      Author: petero
 */

#ifndef LARGEPAGEALLOC_HPP_
#define LARGEPAGEALLOC_HPP_

#include <memory>


/** A utility class for allocating memory using large pages
 *  if supported by the operating system. */
class LargePageAlloc {
public:
    template <typename T>
    static std::shared_ptr<T> allocate(size_t numEntries);

private:
    static std::shared_ptr<void> allocBytes(size_t numBytes);
};


template <typename T>
inline
std::shared_ptr<T> LargePageAlloc::allocate(size_t numEntries) {
    size_t numBytes = numEntries * sizeof(T);
    return std::static_pointer_cast<T, void>(allocBytes(numBytes));
}

#endif /* LARGEPAGEALLOC_HPP_ */
