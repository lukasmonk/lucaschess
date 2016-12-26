/*
You can use this program under the terms of either the following zlib-compatible license
or as public domain (where applicable)

  Copyright (C) 2014-2015 Martin Sedlak

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

// note that this file contains constants from polyglot so the license may not hold here, I'm not a lawyer

#pragma once

#include "../cheng4/types.h"

using namespace cheng4;

namespace polyglot
{

enum RandomConstants
{
	RandomPiece     =   0, // 12 * 64
	RandomCastle    = 768, // 4
	RandomEnPassant = 772, // 8
	RandomTurn      = 780  // 1
};

extern const u64 Zobrist[];

}
