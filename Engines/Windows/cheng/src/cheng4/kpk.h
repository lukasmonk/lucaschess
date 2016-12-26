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

namespace cheng4
{

struct KPK
{
	static u8 bits[64*64*24*2/8];
	static void init();
	static void generate();
	// is draw?
	// color (with pawn), color (stm), king (with pawn) position, bare king position, pawn position
	static u8 isDraw( Color c, Color stm, Square kp, Square bkp, Square pp );
private:
	static u8 getBit( uint b );
	static void setBit( uint b, uint v );
	// stm: white = king with pawn
	static uint index( Square kp, Square bkp, Square psq, Color stm );
	// iterate
	// returns 1 if another iteration is needed
	static bool iterate( u8 *status );
};

}
