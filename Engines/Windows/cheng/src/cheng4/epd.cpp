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

#include "epd.h"
#include "utils.h"
#include "board.h"
#include <fstream>

using namespace std;

namespace cheng4
{

// EPD

// compares token, if it fits, consumes token and returns ptr to next
static bool isToken( const char *&ptr, const char *str )
{
	const char *c = ptr;
	while ( *c && *str && *c == *str )
	{
		c++;
		str++;
	}
	if ( *str )
		return 0;	// not matched
	if ( *c && *c > 32 )
		return 0;	// not a token => no match
	ptr = c;
	return 1;
}

// parse EPD from buffer
// note that this is not a real EPD parser, just a simplified one!
bool EPDFile::parse( const char *ptr )
{
	cheng4::Board b;
	while (ptr && *ptr)
	{
		skipSpaces( ptr );
		const char *res = b.fromFEN( ptr );
		if ( !res )
		{
			skipUntilEOL(ptr);
			continue;
		}
		EPDPosition pos;
		pos.fen = b.toFEN();
		ptr = res;
		for (;;)
		{
			skipSpaces( ptr );
			if ( isToken( ptr, "bm" ) )
			{
				// parse best moves
				for (;;)
				{
					skipSpaces(ptr);
					Move m = b.fromSAN(ptr);
					if ( m == mcNone )
						break;
					pos.best.push_back(m);
				}
				continue;
			}
			if ( isToken( ptr, "am" ) )
			{
				// parse avoid moves
				for (;;)
				{
					skipSpaces(ptr);
					Move m = b.fromSAN(ptr);
					if ( m == mcNone )
						break;
					pos.avoid.push_back(m);
				}
				continue;
			}
			if ( *ptr == 13 || *ptr == 10 || *ptr == ';')
			{
				skipUntilEOL( ptr );
				break;
			}
		}
		positions.push_back( pos );
	}
	return 1;
}

// load EPD file
bool EPDFile::load( const char *fnm )
{
	skipSpaces(fnm);
	positions.clear();
	streampos fsz;
	ifstream ifs( fnm, ios::binary );
	if ( !ifs )
		return 0;
	ifs.seekg( 0, ios::end );
	fsz = ifs.tellg();
	ifs.seekg( 0, ios::beg );
	char *buf = new (std::nothrow) char[ (size_t)(fsz+streampos(1)) ];
	if ( !buf )
		return 0;
	ifs.read( buf, fsz );
	if ( !ifs )
	{
		delete[] buf;
		return 0;
	}
	buf[ (size_t)fsz ] = 0;
	bool res = parse( buf );
	delete[] buf;
	return res;
}

}
