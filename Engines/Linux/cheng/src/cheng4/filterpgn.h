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

#include "board.h"
#include <string>
#include <vector>

namespace cheng4
{

class FilterPgn
{
	struct LexState
	{
		const char *ptr, *top;
	};
	struct Position
	{
		std::string fen;
		// from white's POV
		// 0 = loss, 1 = win, 0.5 = draw
		float outcome;
	};
	struct Game
	{
		Board board;
		std::vector<Position> positions;
		// 0 = white loss (=black win), 0.5 = draw, 1 = white win, -1 = invalid
		float result;

		void clear();
	};
public:
	bool parse(const char *fname);
	bool write(const char *fname);
private:
	LexState ls;
	Game game;
	Game globalGame;

	bool parseTag(std::string &key, std::string &value );
	bool parseString(std::string &str);
	bool parseComment(std::string &str);
	void flushGame();

	inline int getChar()
	{
		return ls.ptr < ls.top ? *ls.ptr++ : -1;
	}
	inline int peekChar() const
	{
		return ls.ptr < ls.top ? *ls.ptr : -1;
	}
};

}
