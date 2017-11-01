/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * alignedAlloc.hpp
 *
 *  Created on: May 22, 2012
 *      Author: petero
 */

#ifndef ALIGNEDALLOC_HPP_
#define ALIGNEDALLOC_HPP_

#include <cstdint>

/** STL allocator that makes sure all allocated memory
 *  blocks are aligned to a 64-byte boundary. */
template <typename T>
class AlignedAllocator {
private:
    enum { ALIGN = 64 };
    using U64 = uint64_t;
public:
    using value_type = T;

    T* allocate(size_t n) {
        size_t needed_size = n*sizeof(T) + sizeof(U64) + ALIGN;
        void* mem = malloc(needed_size);
        if (!mem)
            throw std::bad_alloc();
        U64 ret = ((U64)mem) + sizeof(U64);
        ret = (ret + ALIGN - 1) & ~(ALIGN - 1);
        *(U64*)(ret - sizeof(U64)) = (U64)mem;
        return (T*)ret;
    }

    void deallocate(T* p, size_t n) {
        U64 mem = *(U64*)(((U64)p) - sizeof(U64));
        free((void*)mem);
    }

    template <typename U>
    bool operator==(const AlignedAllocator<U>& other) const {
        return true;
    }

    template <typename U>
    bool operator!=(const AlignedAllocator<U>& other) const {
        return false;
    }
};

#endif /* ALIGNEDALLOC_HPP_ */
