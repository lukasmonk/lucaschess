//
// Cyrano Chess engine
//
// Copyright (C) 2007,2008  Harald JOHNSEN
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
//

#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

// Define forced inline
#ifndef INLINE
   #ifdef _MSC_VER
      #define INLINE __forceinline
        #if (_MSC_VER >= 1400)
            #define RESTRICT __restrict
        #elif defined(__INTEL_COMPILER)
            #define RESTRICT __restrict
        #else
            #define RESTRICT
        #endif
   #elif defined(__GNUC__)
      #define INLINE __inline__ __attribute__((always_inline))
      #define RESTRICT __restrict__
   #else
      #define INLINE inline
   #endif
#endif

#ifndef __64_BIT_INTEGER_DEFINED__
	#define __64_BIT_INTEGER_DEFINED__
	#if defined(_MSC_VER) && _MSC_VER<1300
		typedef unsigned __int64 U64; //For the old microsoft compilers
	#else
		typedef unsigned long long  U64; //Supported by MSC 13.00+ and C99
	#endif //defined(_MSC_VER) && _MSC_VER<1300
#endif //__64_BIT_INTEGER_DEFINED__

//typedef	unsigned __int64	U64;	// This is an MSVC data type.  If you need
									//  to change it, your compiler may have a
									//  64-bit unsigned data type.  If not,
									//  you have some work to do, since you
									//  will have to make this an array or
									//  struct, and deal with the places where
									//  math is done on these.

typedef unsigned int		U32;
typedef unsigned char       U8;
typedef U64                 bitboard;

// Define LSB function
#ifdef _MSC_VER
   #if defined(_WIN64) || defined(__LP64__)
   #define TARGET64
	  #include <intrin.h>
      #pragma intrinsic(_BitScanForward64)
	  #define LSB_DEFINED
      
	  INLINE unsigned int LSB(const U64 val)
	  {
		  unsigned int lsb;
		  _BitScanForward64(&lsb, val);
		  return lsb;
	  }
   #endif
#elif defined(__GNUC__) && defined(__LP64__)
   #define TARGET64
   INLINE unsigned int LSB(const U64 val)
   {
      U64 Ret;
      __asm__
      (
         "bsfq %[val], %[Ret]"
         :[Ret] "=r" (Ret)
         :[val] "mr" (val)
      );
      return (unsigned int)Ret;
   }
   #define LSB_DEFINED
#endif

#ifndef C64
    #if (!defined(_MSC_VER) || _MSC_VER>1300)
        #define C64(constantU64) constantU64##ULL
    #else
        #define C64(constantU64) constantU64
    #endif
#endif

#ifndef TARGET64
    #define USE_32_BIT_MULTIPLICATIONS
#endif

#ifdef _WIN32
    #define TARGET_WIN
#endif
#ifdef _WIN64
    #define TARGET_WIN
#endif

#ifndef TARGET_WIN
    #define USE_PTHREADS
	#define U64_FORMAT "%llu"
#else
	#define U64_FORMAT "%I64u"
#endif

#endif // _CONFIG_HPP_
