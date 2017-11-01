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
 * Copyright 2007-2009 Peter Sommerlad, Emanuel Graf
 *
 *********************************************************************************/
#ifndef CUTE_TEST_H_
#define CUTE_TEST_H_
#include "cute_determine_version.h"
#include "cute_determine_library.h"
#include "cute_demangle.h"
// make plain functions as tests more 'cute':
namespace cute {

	struct test{
		void operator()()const{ theTest(); }
		std::string name()const{ return name_;}


		// (real) functor types can (almost) spell their name
		// but a name can also be given explicitely, e.g. for CUTE() macro
		// for simple test functions
		template <typename VoidFunctor>
		test(VoidFunctor const &t, std::string sname = demangle(typeid(VoidFunctor).name()))
		:theTest(t),name_(sname){}

	private:
		boost_or_tr1::function<void()> theTest;
		std::string name_;
	};

}
#define CUTE(name) cute::test((&name),(#name))

#endif /*CUTE_TEST_H_*/
