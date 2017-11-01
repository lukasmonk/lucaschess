/*********************************************************************************
 * This file is part of CUTE.
 *
 * CUTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CUTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CUTE.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2007-2011 Peter Sommerlad
 *
 *********************************************************************************/

#ifndef CUTE_DETERMINE_LIBRARY_H_
#define CUTE_DETERMINE_LIBRARY_H_
#if defined(USE_TR1)
#include <tr1/functional>
// bind already given by <functional> in cute_test.h from cute_suite.h
namespace boost_or_tr1 = std::tr1;
#elif defined(USE_STD0X)
#include <functional>
namespace boost_or_tr1 = std;
#else
#include <boost/bind.hpp>
#include <boost/function.hpp>
namespace boost_or_tr1 = boost;
#endif

#endif /*CUTE_DETERMINE_LIBRARY_H_*/
