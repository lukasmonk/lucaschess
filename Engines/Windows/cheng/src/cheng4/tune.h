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

#ifndef USE_TUNING
#	define TUNE_STATIC static
#	define TUNE_CONST const
#	define TUNE_EXPORT(x, y, z)
#else
#	include <vector>
#	include <string>
#	include <sstream>

#	define TUNE_STATIC
#	define TUNE_CONST
#	define TUNE_EXPORT(x, y, z) static const Tunable<x> tunable_##y(&z, #y)

namespace cheng4
{

class TunableBase
{
public:
	virtual const std::string &name() const = 0;
	virtual void set( const char *str ) = 0;
	virtual std::string get() const = 0;
};

class TunableParams
{
	static TunableParams *inst;
public:
	static TunableParams *get();
	static std::vector< TunableBase * > params;
	static void addParam( TunableBase *param );
	static bool setParam( const char *name, const char *value );
	static size_t paramCount();
	static const TunableBase *getParam( size_t index );
	static TunableBase *findParam( const char *name );
	// dump params
	static void dump();
};

// this represents a tunable parameter
template< typename T > class Tunable : public TunableBase
{
	T *value;
	std::string parName;
public:
	Tunable( T *tvalue, const char *tname ) : value(tvalue), parName(tname)
	{
		TunableParams::get()->addParam(this);
	}
	const std::string &name() const
	{
		return parName;
	}
	std::string get() const
	{
		std::stringstream stream;
		stream << *value;
		return stream.str();
	}
	void set( const char *str )
	{
		std::stringstream stream(str);
		stream >> *value;
	}
};

}

#endif
