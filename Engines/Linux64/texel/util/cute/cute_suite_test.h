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
 * Copyright 2006 Peter Sommerlad
 *
 *********************************************************************************/

#ifndef CUTE_SUITE_TEST_H_
#define CUTE_SUITE_TEST_H_
#include "cute_suite.h"
#include "cute_determine_version.h"
#include "cute_determine_library.h"

#include <algorithm>
namespace cute{
	// make a whole suite a test, failure stops the suite's execution
	struct suite_test {
		suite theSuite;
		suite_test(suite const &s):theSuite(s){}
		void operator()(){
#if defined(USE_STD0X) || defined(USE_TR1)
			using namespace boost_or_tr1::placeholders;
#endif
			std::for_each(theSuite.begin(),theSuite.end(),boost_or_tr1::bind(&test::operator(),_1));
		}
	};
}
#define CUTE_SUITE_TEST(s) cute::test(cute::suite_test((s)),#s)
#define CUTE_SUITE_TEST_NAME(s, name) cute::test(cute::suite_test((s)),name)
#endif /*CUTE_SUITE_TEST_H_*/
