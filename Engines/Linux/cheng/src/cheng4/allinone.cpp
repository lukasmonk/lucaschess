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

// everything glued together into one cpp file aka unity build

#ifdef _MSC_VER
	#undef _CRT_SECURE_NO_WARNINGS
	#undef _HAS_EXCEPTIONS
	#undef _SECURE_SCL
	#define _CRT_SECURE_NO_WARNINGS
	#define _HAS_EXCEPTIONS		0
	#define _SECURE_SCL			0
#endif

#include "board.cpp"
#include "book.cpp"
#include "bookzobrist.cpp"
#include "engine.cpp"
#include "eval.cpp"
#include "filterpgn.cpp"
#include "history.cpp"
#include "kpk.cpp"
#include "magic.cpp"
#include "main.cpp"
#include "move.cpp"
#include "movegen.cpp"
#include "protocol.cpp"
#include "psq.cpp"
#include "search.cpp"
#include "see.cpp"
#include "tables.cpp"
#include "thread.cpp"
#include "trans.cpp"
#include "tune.cpp"
#include "utils.cpp"
#include "version.cpp"
#include "zobrist.cpp"
#include "epd.cpp"
