/*
You can use this program under the terms of either the following zlib-compatible license
or as public domain (where applicable)

  Copyright (C) 2012-2015 Martin Sedlak

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgement in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <cstddef>

// various utility functions that didn't fit elsewhere

namespace cheng4
{

// returns true if input is power of two
bool isPow2( size_t sz );
// round size to nearest power of two
bool roundPow2( size_t &sz );
// align pointer
void *alignPtr( void *ptr, size_t align );
// simple unsafe string copy (doesn't copy null terminator!)
char *scpy( char *dst, const char *src );
// skip leading spaces (exclude EOLs)
void skipSpaces( const char *&ptr );
// skip until EOL (and skip it too)
void skipUntilEOL( const char *&ptr );

}
