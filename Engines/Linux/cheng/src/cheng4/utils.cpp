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

#include "utils.h"
#include <assert.h>
#ifndef _MSC_VER
#include <stdint.h>
#endif

namespace cheng4
{

// returns true if input is power of two
bool isPow2( size_t sz )
{
	size_t tmp = sz;
	while ( tmp < sz )
	{
		size_t otmp = tmp;
		tmp <<= 1;
		if ( tmp < otmp )
			return 0;			// overflow
	}
	return tmp == sz;
}

// round size to nearest power of two
bool roundPow2( size_t &sz )
{
	size_t tmp = 1;
	while ( tmp < sz )
	{
		size_t otmp = tmp;
		tmp <<= 1;
		if ( tmp < otmp )
			return 0;			// overflow
	}
	sz = tmp;
	return 1;
}

// align pointer
void *alignPtr( void *ptr, size_t align )
{
	assert( isPow2(align) );
	return reinterpret_cast<void *>( ((uintptr_t)ptr + align-1) & ~(uintptr_t)(align-1) );
}

// simple unsafe string copy (doesn't copy null terminator!)
char *scpy( char *dst, const char *src )
{
	while (*src)
		*dst++ = *src++;
	return dst;
}

// skip leading spaces (exclude EOLs)
void skipSpaces( const char *&ptr )
{
  while (*ptr && *ptr > 0 && *ptr <= 32 && *ptr != 13 && *ptr != 10 )
    ptr++;
}

// skip until EOL (and skip it too)
void skipUntilEOL( const char *&ptr )
{
  while ( *ptr && *ptr != 13 && *ptr != 10 )
    ptr++;
  if ( !*ptr )
    return;
  if ( *ptr == 13 && ptr[1] == 10 )
    ptr += 2;
  else
    ptr++;
}

}
