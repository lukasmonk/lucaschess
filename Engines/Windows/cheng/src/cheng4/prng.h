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

#include "types.h"

namespace cheng4
{

struct PRNG
{
	u64 keys[4];							// 4 keys

	static inline u64 rol( u64 r, u8 amt )
	{
		assert( amt != 0 && amt < 64 );
		return (r << amt) | (r >> (64-amt) );
	}
	static inline u64 ror( u64 r, u8 amt )
	{
		assert( amt != 0 && amt < 64 );
		return (r >> amt) | (r << (64-amt) );
	}

	// passes diehard tests!
	// can't test dieharder under windows
	// one round of PRNG
	inline u64 next64()
	{
		u64 tmp = keys[0] + keys[2] - keys[1] - keys[3];
		keys[0] ^= rol( keys[1], 5 );
		keys[1] += ror( keys[2], 27 );
		keys[2] -= rol( keys[3], 41 );
		return keys[3] ^= ror( tmp, 17 ) - 0xfacebabec0ca101aULL;
	}

	PRNG( u64 seed = 0 )
	{
		keys[0] = keys[1] = keys[2] = keys[3] = seed;
		// scramble
		for (int i=0; i<64; i++)
			next64();
	}
};

}
