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

#include "chtypes.h"
#include <cassert>
#include <memory.h>

namespace cheng4
{

typedef u8 repIndex;

// FIXME: rethink completely
// repetition stack
struct RepHash
{
	Signature rep[ repHashMax ];		// repetition signatures
	u8 res[ repHashMax ];				// fifty counter reset flag (1=just captured/moved a pawn)
	repIndex start[ repHashMax ];		// starting pointers
	repIndex ptr;						// stack ptr
	repIndex sptr;						// starting stack ptr

	RepHash()
	{
		memset( rep, 0, sizeof(rep) );
		memset( res, 0, sizeof(res) );
		memset( start, 0, sizeof(start) );
		clear();
	}

	void copyFrom( const RepHash &o )
	{
		ptr = o.ptr;
		sptr = o.sptr;
		memcpy( start, o.start, sptr*sizeof(repIndex) );
		memcpy( res, o.res, ptr*sizeof(u8) );
		memcpy( rep, o.rep, ptr*sizeof(Signature) );
	}

	// clear rep hash
	inline void clear()
	{
		assert( this );
		sptr = ptr = 0;
	}

	// is repetition?
	inline bool isRep( Signature h ) const
	{
		// the position below first resulted from a pawn move/capture
		int i;
		repIndex first = sptr > 0 ? start[ sptr - 1 ] : 0;
		for (i = (int)ptr-3; i >= first; i -= 2 )
		{
			if ( rep[i] == h )
				break;
		}
		return i >= first;
	}
	inline void push( Signature h, bool reset )
	{
		if ( reset )
		{
			assert( sptr < repHashMax-1 );
			start[ sptr++ ] = ptr;
		}
		res[ ptr ] = reset ? 1 : 0;
		assert( ptr < repHashMax-1 );
		rep[ ptr++ ] = h;
	}
	inline void pop()
	{
		assert( ptr > 0 );
		if ( res[ --ptr ] )
		{
			assert( sptr > 0 );
			sptr--;
		}
	}
};

}
