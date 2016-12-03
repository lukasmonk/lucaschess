/**
 *magicmoves.h
 *
 *Header file for magic move bitboard generation.  Include this in any files
 *need this functionality.
 *
 *Usage:
 *You must first initialize the generator with a call to initmagicmoves().
 *Then you can use the following macros for generating move bitboards by
 *giving them a square and an occupancy.  The macro will then "return"
 *the correct move bitboard for that particular square and occupancy. It
 *has been named Rmagic and Bmagic so that it will not conflict with
 *any functions/macros in your chess program called Rmoves/Bmoves. You
 *can macro Bmagic/Rmagic to Bmoves/Rmoves if you wish.
 Where you typedef your unsigned 64-bit
 *integer declare __64_BIT_INTEGER_DEFINED__.  If USE_INLINING is uncommented,
 *the macros will be expressed as MMINLINEd functions.  

 *Bmagic(square, occupancy)
 *Rmagic(square, occupancy)
 *
 *Square is an integer that is greater than or equal to zero and less than 64.
 *Occupancy is any unsigned 64-bit integer that describes which squares on
 *the board are occupied.
 *
 *
 *Unsigned 64 bit integers are referenced by this generator as U64.
 *Edit the beginning lines of this header for the defenition of a 64 bit
 *integer if necessary.
 *
 *Copyright (C) 2007 Pradyumna Kannan.
 *
 *This code is provided 'as-is', without any expressed or implied warranty.
 *In no event will the authors be held liable for any damages arising from
 *the use of this code. Permission is granted to anyone to use this
 *code for any purpose, including commercial applications, and to alter
 *it and redistribute it freely, subject to the following restrictions:
 *
 *1. The origin of this code must not be misrepresented; you must not
 *claim that you wrote the original code. If you use this code in a
 *product, an acknowledgment in the product documentation would be
 *appreciated but is not required.
 *
 *2. Altered source versions must be plainly marked as such, and must not be
 *misrepresented as being the original code.
 *
 *3. This notice may not be removed or altered from any source distribution.
 */

#pragma once
#ifndef _magicmovesh
#define _magicmovesh

/*********MODIFY THE FOLLOWING IF NECESSARY********/
//the default configuration is the best

#define USE_INLINING /*the MMINLINE keyword is assumed to be available*/

#ifndef __64_BIT_INTEGER_DEFINED__
  #define __64_BIT_INTEGER_DEFINED__
  #if defined(_MSC_VER) && _MSC_VER<1300
    typedef unsigned __int64 U64; //For the old microsoft compilers
  #else
    typedef unsigned long long  U64; //Supported by MSC 13.00+ and C99
  #endif //defined(_MSC_VER) && _MSC_VER<1300
#endif //__64_BIT_INTEGER_DEFINED__
/***********MODIFY THE ABOVE IF NECESSARY**********/

/*Defining the inlining keyword*/
#ifdef USE_INLINING
  #ifdef _MSC_VER
    #define MMINLINE __forceinline
  #elif defined(__GNUC__)
    #define MMINLINE __inline__ __attribute__((always_inline))
  #else
    #define MMINLINE inline
  #endif
#endif

#ifndef C64
  #if (!defined(_MSC_VER) || _MSC_VER>1300)
    #define C64(constantU64) constantU64##ULL
  #else
    #define C64(constantU64) constantU64
  #endif
#endif

extern const U64 magicmoves_r_magics[64];
extern const U64 magicmoves_r_mask[64];
extern const U64 magicmoves_b_magics[64];
extern const U64 magicmoves_b_mask[64];
extern const unsigned int magicmoves_b_shift[64];
extern const unsigned int magicmoves_r_shift[64];

  #define MINIMAL_B_BITS_SHIFT(square) 55
  #define MINIMAL_R_BITS_SHIFT(square) 52

    #ifndef USE_INLINING
      #define Bmagic(square, occupancy) *(magicmoves_b_indices[square]+((((occupancy)&magicmoves_b_mask[square])*magicmoves_b_magics[square])>>magicmoves_b_shift[square]))
      #define Rmagic(square, occupancy) *(magicmoves_r_indices[square]+((((occupancy)&magicmoves_r_mask[square])*magicmoves_r_magics[square])>>magicmoves_r_shift[square]))
   #endif //USE_INLINING

    //extern U64 magicmovesbdb[5248];
    extern const U64* magicmoves_b_indices[64];

    //extern U64 magicmovesrdb[102400];
    extern const U64* magicmoves_r_indices[64];

#ifdef USE_INLINING
  static MMINLINE U64 Bmagic(const unsigned int square,const U64 occupancy)
  {
        return *(magicmoves_b_indices[square]+(((occupancy&magicmoves_b_mask[square])*magicmoves_b_magics[square])>>magicmoves_b_shift[square]));
  }
  static MMINLINE U64 Rmagic(const unsigned int square,const U64 occupancy)
  {
        return *(magicmoves_r_indices[square]+(((occupancy&magicmoves_r_mask[square])*magicmoves_r_magics[square])>>magicmoves_r_shift[square]));
  }

  static MMINLINE U64 Qmagic(const unsigned int square,const U64 occupancy)
  {
    return Bmagic(square,occupancy)|Rmagic(square,occupancy);
  }

#else //!USE_INLINING

#define Qmagic(square, occupancy) (Bmagic(square,occupancy)|Rmagic(square,occupancy))

#endif //USE_INLINING

void initmagicmoves(void);

#endif //_magicmoveshvesh
