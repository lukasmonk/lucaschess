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

#include "tune.h"

#ifdef USE_TUNING

#include <iostream>

namespace cheng4
{

// TunableParams

TunableParams *TunableParams::inst = 0;

TunableParams *TunableParams::get()
{
	// note: not thread-safe
	if ( !inst )
		inst = new TunableParams;
	return inst;
}

std::vector< TunableBase * > TunableParams::params;

void TunableParams::addParam( TunableBase *param )
{
	params.push_back( param );
}

bool TunableParams::setParam( const char *name, const char *value )
{
	for ( size_t i=0; i<params.size(); i++ )
	{
		if ( params[i]->name() == name )
		{
			params[i]->set( value );
			return 1;
		}
	}
	return 0;
}

// dump params
void TunableParams::dump()
{
	for ( size_t i=0; i<params.size(); i++ )
	{
		std::cout << params[i]->name() << " = " << params[i]->get() << std::endl;
	}
}

size_t TunableParams::paramCount()
{
	return params.size();
}

const TunableBase *TunableParams::getParam( size_t index )
{
	return params[index];
}

TunableBase *TunableParams::findParam( const char *name )
{
	for (size_t i=0; i<params.size(); i++)
	{
		if (params[i]->name() == name) {
			return params[i];
		}
	}
	return 0;
}

}

#endif
