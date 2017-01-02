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

#include <memory.h>
#include "magic.h"
#include "movegen.h"
#include "history.h"
#include "zobrist.h"
#include "psq.h"
#include "search.h"
#include "engine.h"
#include "protocol.h"
#include <iostream>

int main( int argc, char **argv )
{
	// disable I/O buffering
	std::cin.rdbuf()->pubsetbuf(0, 0);
	std::cout.rdbuf()->pubsetbuf(0, 0);
	setbuf( stdin, 0 );
	setbuf( stdout, 0 );

	// static init
	cheng4::Engine::init( argc-1, const_cast<const char **>(argv)+1 );

	cheng4::Engine *eng = new cheng4::Engine;
	cheng4::Protocol *proto = new cheng4::Protocol( *eng );

	eng->run();

	while ( !proto->shouldQuit() )
	{
		std::string line;
		std::getline( std::cin, line );
		proto->parse( line );
	}

	delete proto;
	delete eng;

	cheng4::Engine::done();
	return 0;
}
