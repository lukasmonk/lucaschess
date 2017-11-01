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
 * Copyright 2007-2011 Peter Sommerlad, Emanuel Graf
 *
 *********************************************************************************/

#ifndef CUTE_EQUALS_H_
#define CUTE_EQUALS_H_
#if defined(__GXX_EXPERIMENTAL_CXX0X__)
#define USE_STD0X 1
#endif

#include "cute_base.h"
#include "cute_determine_version.h"
#include "cute_demangle.h"
#include <cmath>
#include <limits>
#include <ostream>
#include <sstream>
#include <algorithm>
#if defined(USE_STD0X)
#include <type_traits>
#elif defined(USE_TR1)
#include <tr1/type_traits>
#else
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/make_signed.hpp>
#endif
namespace cute {
#if defined(USE_STD0X)
	namespace impl_place_for_traits = std;
#elif defined(USE_TR1)
	namespace impl_place_for_traits = std::tr1;
#else
	namespace impl_place_for_traits = boost;
#endif
	namespace equals_impl {

		template <typename T>
		std::ostream &to_stream(std::ostream &os,T const &t); // recursion needs forward

		static inline std::string backslashQuoteTabNewline(std::string const &input){
			std::string result;
			result.reserve(input.size());
			for (std::string::size_type i=0; i < input.length() ; ++i){
				switch(input[i]) {
					case '\n': result += "\\n"; break;
					case '\t': result += "\\t"; break;
					case '\\': result += "\\\\"; break;
					case '\r': result += "\\r"; break;
					default: result += input[i];
				}
			}
			return result;

		}

		// the following code was stolen and adapted from Boost Exception library.
		// it avoids compile errors, if a type used with ASSERT_EQUALS doesn't provide an output shift operator
		namespace to_string_detail {
			template <class T,class CharT,class Traits>
			char operator<<( std::basic_ostream<CharT,Traits> &, T const & );
			template <class T,class CharT,class Traits>
			struct is_output_streamable_impl {
				static std::basic_ostream<CharT,Traits> & f();
				static T const & g();
				enum e { value = (sizeof(char) != sizeof(f()<<g())) }; // assumes sizeof(char)!=sizeof(ostream&)
			};
			template <class CONT>
			struct has_begin_end_const_member {
				template <typename T, T, T> struct type_check;
				template <typename C> static typename C::const_iterator test(
						type_check<typename C::const_iterator (C::*)()const,&C::begin, &C::end>*);
				template <typename C> static char test(...);
				enum e { value = (sizeof(char) != sizeof(test<CONT>(0)))
				};
			};
		}
		template <class T, class CharT=char, class Traits=std::char_traits<CharT> >
		struct is_output_streamable {
			enum e { value=to_string_detail::is_output_streamable_impl<T,CharT,Traits>::value };
		};
		// detect standard container conforming begin() end() iterator accessors.
		// might employ begin/end traits from c++0x for loop in the future. --> select_container
		template <typename T>
		struct printItWithDelimiter
		{
			std::ostream &os;
			bool first; // allow use of for_each algorithm
			printItWithDelimiter(std::ostream &os):os(os),first(true){}
			void operator()(T const &t){
				if (!first) os<<',';
				else first=false;
				os << '\n'; // use newlines so that CUTE's plug-in result viewer gives nice diffs
				equals_impl::to_stream<T>(os,t);
			}
		};
		//try print_pair with specialization of template function instead:
		// the generic version prints about missing operator<< that is the last resort
		template <typename T>
		std::ostream &print_pair(std::ostream &os,T const &t){
			return os << "no operator<<(ostream&, " <<cute::demangle(typeid(T).name())<<')';
		}
		//the std::pair overload is useful for std::map etc. however,
		template <typename K, typename V>
		std::ostream &print_pair(std::ostream &os,std::pair<K,V> const &p){
			os << '[' ;
			equals_impl::to_stream(os,p.first);
			os << " -> ";
			equals_impl::to_stream(os,p.second);
			os << ']';
			return os;
		}

		template <typename T, bool select>
		struct select_container {
			std::ostream &os;
			select_container(std::ostream &os):os(os){}
			std::ostream& operator()(T const &t){
				printItWithDelimiter<typename T::value_type> printer(os);
				os << cute::demangle(typeid(T).name()) << '{';
				std::for_each(t.begin(),t.end(),printer);
				return os << '}';
			}
		};

		template <typename T>
		struct select_container<T,false> {
			std::ostream &os;
			select_container(std::ostream &os):os(os){}
			std::ostream & operator()(T const &t){
				//  look for std::pair. a future with tuple might be useful as well, but not now.
				return print_pair(os,t); // here a simple template function overload works.
			}
		};

		template <typename T, bool select>
		struct select_built_in_shift_if {
			std::ostream &os;
			select_built_in_shift_if(std::ostream &ros):os(ros){}
			std::ostream& operator()(T const &t){
				return os << t ; // default uses operator<<(std::ostream&,T const&) if available
			}
		};

		template <typename T>
		struct select_built_in_shift_if<T,false> {
			std::ostream &os;
			select_built_in_shift_if(std::ostream &ros):os(ros){}
			std::ostream & operator()(T const &t){
				// if no operator<< is found, try if it is a container or std::pair
				return select_container<T,to_string_detail::has_begin_end_const_member<T>::value >(os)(t);
			}
		};
		template <typename T>
		std::ostream &to_stream(std::ostream &os,T const &t){
			select_built_in_shift_if<T,equals_impl::is_output_streamable<T>::value > out(os);
			return out(t);
		}

		template <typename T>
		std::string to_string(T const &t) {
			std::ostringstream os;
			to_stream(os,t);
			return os.str();
		}
	}
	// you could provide your own overload for diff_values for your app-specific types
	// be sure to use tabs as given below, then the CUTE eclipse plug-in will parse correctly
	template <typename ExpectedValue, typename ActualValue>
	std::string diff_values(ExpectedValue const &expected
						,ActualValue const & actual){
		// construct a simple message...to be parsed by IDE support
		std::ostringstream os;
		os << " expected:\t" << equals_impl::backslashQuoteTabNewline(equals_impl::to_string(expected))
		   <<"\tbut was:\t"<<equals_impl::backslashQuoteTabNewline(equals_impl::to_string(actual))<<"\t";
		return os.str();
	}
	namespace equals_impl {
		// provide some template meta programming tricks to select "correct" comparison for floating point and integer types
		template <typename ExpectedValue, typename ActualValue, typename DeltaValue>
		bool do_equals_floating_with_delta(ExpectedValue const &expected
				,ActualValue const &actual
				,DeltaValue const &delta) {
			return std::abs(delta)  >= std::abs(expected-actual);
		}
		template <typename ExpectedValue, typename ActualValue, bool select_non_floating_point_type>
		bool do_equals_floating(ExpectedValue const &expected
					,ActualValue const &actual,const impl_place_for_traits::integral_constant<bool, select_non_floating_point_type>&){
			return expected==actual; // normal case for most types uses operator==!
		}
		template <typename ExpectedValue, typename ActualValue>
		bool do_equals_floating(ExpectedValue const &expected
					,ActualValue const &actual,const impl_place_for_traits::true_type&){
			const ExpectedValue automatic_delta_masking_last_significant_digit=(10*std::numeric_limits<ExpectedValue>::epsilon())*expected;
			return do_equals_floating_with_delta(expected,actual,automatic_delta_masking_last_significant_digit);
		}
		// TMP-overload dispatch for floating points 2 bool params --> 4 overloads
		template <typename ExpectedValue, typename ActualValue, bool select_non_integral_type>
		bool do_equals(ExpectedValue const &expected
					,ActualValue const &actual
					,const impl_place_for_traits::integral_constant<bool, select_non_integral_type>&exp_is_integral
					,const impl_place_for_traits::integral_constant<bool, select_non_integral_type>&act_is_integral){
			return do_equals_floating(expected,actual,impl_place_for_traits::is_floating_point<ExpectedValue>());
		}
		template <typename ExpectedValue, typename ActualValue, bool select_non_integral_type>
		bool do_equals(ExpectedValue const &expected
					,ActualValue const &actual
					,const impl_place_for_traits::false_type&,const impl_place_for_traits::false_type&){
			return do_equals_floating(expected,actual,impl_place_for_traits::is_floating_point<ActualValue>());
		}
		template <typename ExpectedValue, typename ActualValue, bool select_non_integral_type>
		bool do_equals(ExpectedValue const &expected
					,ActualValue const &actual
					,const impl_place_for_traits::integral_constant<bool, select_non_integral_type>&exp_is_integral
					,const impl_place_for_traits::true_type&){
			return do_equals_floating(expected,actual,impl_place_for_traits::is_floating_point<ExpectedValue>());
		}
		template <typename ExpectedValue, typename ActualValue, bool select_non_integral_type>
		bool do_equals(ExpectedValue const &expected
					,ActualValue const &actual
					,const impl_place_for_traits::true_type&,const impl_place_for_traits::integral_constant<bool, select_non_integral_type>&act_is_integral){
			return do_equals_floating(expected,actual,impl_place_for_traits::is_floating_point<ActualValue>());
		}
		// can I get rid of the following complexity by doing a do_equals_integral
		// parameterized by is_signed<ExpectedValue>==is_signed<ActualValue> or nofBits<A> < nofBits<B>


		// this is an optimization for avoiding if and sign-extend overhead if both int types are the same as below
		template <typename IntType>
		bool do_equals(IntType const &expected
					,IntType const &actual
					,const impl_place_for_traits::true_type&,const impl_place_for_traits::true_type&){
			return expected==actual;
		}
		// bool cannot be made signed, therefore we need the following three overloads, also to avoid ambiguity
		template <typename IntType>
		bool do_equals(bool const &expected
					,IntType const &actual
					,const impl_place_for_traits::true_type&,const impl_place_for_traits::true_type&){
			return expected==actual;
		}
		template <typename IntType>
		bool do_equals(IntType const &expected
					,bool const &actual
					,const impl_place_for_traits::true_type&,const impl_place_for_traits::true_type&){
			return expected==actual;
		}
		// do not forget the inline on a non-template overload!
		// this overload is needed to actually avoid ambiguity for comparing bool==bool as a best match
		inline bool do_equals(bool const &expected
				      ,bool const &actual
				      , const impl_place_for_traits::true_type&,const impl_place_for_traits::true_type&){
			return expected==actual;
		}
		template <typename IntegralType>
		size_t nof_bits(IntegralType const &){
			return std::numeric_limits<IntegralType>::digits;
		}
#if defined(USE_TR1)
		template <typename ExpectedValue, typename ActualValue>
		bool do_equals_integral(ExpectedValue const &expected
				,ActualValue const &actual
				,const impl_place_for_traits::true_type&,const impl_place_for_traits::true_type&){
			if (nof_bits(expected) < nof_bits(actual))
						return static_cast<ActualValue>(expected) == actual;
			else
						return expected == static_cast<ExpectedValue>(actual);
			return false;
		}
		template <typename ExpectedValue, typename ActualValue>
		bool do_equals_integral(ExpectedValue const &expected
				,ActualValue const &actual
				,const impl_place_for_traits::false_type&,const impl_place_for_traits::true_type&){
//TODO complicated case, one signed one unsigned type. since it is about equality casting to the longer should work?
			if (sizeof(ExpectedValue) >	sizeof(ActualValue))
				return expected==static_cast<ExpectedValue>(actual);
			else
				return static_cast<ActualValue>(expected) == actual;
		}
		template <typename ExpectedValue, typename ActualValue>
		bool do_equals_integral(ExpectedValue const &expected
				,ActualValue const &actual
				,const impl_place_for_traits::true_type&,const impl_place_for_traits::false_type&){
//TODO
			if (sizeof(ExpectedValue) < sizeof(ActualValue))
				return static_cast<ActualValue>(expected)==	actual;
			else
				return expected == static_cast<ExpectedValue>(actual);
		}
		template <typename ExpectedValue, typename ActualValue>
		bool do_equals_integral(ExpectedValue const &expected
				,ActualValue const &actual
				,const impl_place_for_traits::false_type&,const impl_place_for_traits::false_type&){
			if (nof_bits(expected) < nof_bits(actual))
						return static_cast<ActualValue>(expected) == actual;
			else
						return expected == static_cast<ExpectedValue>(actual);
			return false;
		}
#endif
		// will not work if either type is bool, therefore the overloads above.
		template <typename ExpectedValue, typename ActualValue>
		bool do_equals(ExpectedValue const &expected
					,ActualValue const &actual
					,const impl_place_for_traits::true_type&,const impl_place_for_traits::true_type&){
#if defined(USE_TR1)
			return do_equals_integral(expected,actual,
					impl_place_for_traits::is_signed<ExpectedValue>()
					,impl_place_for_traits::is_signed<ActualValue>());
#else
// TODO: replace the following code with a dispatcher on signed/unsigned
			typedef typename impl_place_for_traits::make_signed<ExpectedValue>::type ex_s;
			typedef typename impl_place_for_traits::make_signed<ActualValue>::type ac_s;
				// need to sign extend with the longer type, should work...
			    // might be done through more template meta prog tricks....but...
				if (nof_bits(expected) < nof_bits(actual))
					return static_cast<ac_s>(expected) == static_cast<ac_s>(actual);
				else
					return static_cast<ex_s>(expected) == static_cast<ex_s>(actual);
#endif
		}
	} // namespace equals_impl
	template <typename ExpectedValue, typename ActualValue>
	void assert_equal(ExpectedValue const &expected
				,ActualValue const &actual
				,char const *msg
				,char const *file
				,int line) {
		// unfortunately there is no is_integral_but_not_bool_or_enum
		typedef typename impl_place_for_traits::is_integral<ExpectedValue> exp_integral;
		typedef typename impl_place_for_traits::is_integral<ActualValue> act_integral;
		if (equals_impl::do_equals(expected,actual,exp_integral(),act_integral()))
			return;
		throw test_failure(equals_impl::backslashQuoteTabNewline(msg) + diff_values(expected,actual),file,line);
	}

	template <typename ExpectedValue, typename ActualValue, typename DeltaValue>
	void assert_equal_delta(ExpectedValue const &expected
				,ActualValue const &actual
				,DeltaValue const &delta
				,char const *msg
				,char const *file
				,int line) {
		if (equals_impl::do_equals_floating_with_delta(expected,actual,delta)) return;
		throw test_failure(equals_impl::backslashQuoteTabNewline(msg) + diff_values(expected,actual),file,line);
	}

}

#define ASSERT_EQUALM(msg,expected,actual) cute::assert_equal((expected),(actual),msg,__FILE__,__LINE__)
#define ASSERT_EQUAL(expected,actual) ASSERT_EQUALM(#expected " == " #actual, (expected),(actual))
#define ASSERT_EQUAL_DELTAM(msg,expected,actual,delta) cute::assert_equal_delta((expected),(actual),(delta),msg,__FILE__,__LINE__)
#define ASSERT_EQUAL_DELTA(expected,actual,delta) ASSERT_EQUAL_DELTAM(#expected " == " #actual " with error " #delta  ,(expected),(actual),(delta))
#endif /*CUTE_EQUALS_H_*/
