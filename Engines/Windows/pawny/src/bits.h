/*--------------------------------------------------------------------------
    Pawny 0.3.1, chess engine (source code).
    Copyright (C) 2009 - 2011 by Mincho Georgiev.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#if(defined(__INTEL_COMPILER) || defined(_MSC_VER))
static __forceinline void bitset(unsigned __int64 *qw, int index) { *qw |= (1ULL << index); }
static __forceinline void bitclear(unsigned __int64 *qw, int index) {*qw &= ~(1ULL << index);}

#if defined(_M_IX86)
static  __forceinline int bitscanf(unsigned __int64 qw)
{ __asm
  { xor eax,eax
    bsf eax,dword ptr[qw]
    jne lret
    bsf eax,dword ptr[qw+4]
    je lret
    add eax,20h
    lret:
  }
}

static  __forceinline int bitscanr(unsigned __int64 qw)
{ __asm
  { xor eax,eax
    bsr eax,dword ptr[qw+4]
    je l1
    add eax,20h
    jmp lret
    l1:
    bsr eax,dword ptr[qw]
    lret:
  }
}

static __forceinline int popcnt(unsigned __int64 qw)
{ unsigned int w1,w2;
  w1 = (unsigned int)(qw & 0xFFFFFFFF);
  w2 = (unsigned int)(qw >> 32);
  w1 -= (w1 >> 1) & 0x55555555;
  w1 = (w1 & 0x33333333) + ((w1 >> 2) & 0x33333333);
  w1 = (w1 + (w1 >> 4)) & 0x0F0F0F0F;
  w2 -= (w2 >> 1) & 0x55555555;
  w2 = (w2 & 0x33333333) + ((w2 >> 2) & 0x33333333);
  w2 = (w2 + (w2 >> 4)) & 0x0F0F0F0F;
  return ((w1 * 0x01010101) >> 24)+((w2 * 0x01010101) >> 24);
}

#else if(defined(_M_IX64))
static  __forceinline int bitscanf(unsigned __int64 qw)
{ __asm
  { xor rax,rax
    bsf rax,qword ptr[qw]
  }
}

static  __forceinline int bitscanr(unsigned __int64 qw)
{ __asm
  { xor rax,rax
    bsr rax,qword ptr[qw]
  }
}

#ifdef POPCNT
#include <smmintrin.h>
static  __inline int popcnt(unsigned __int64 qw)
{ return _mm_popcnt_u64(qw);
}

#else
static __forceinline int popcnt(unsigned __int64 qw)
{ qw -= ((qw >> 1) & 0x5555555555555555ULL);
  qw = (qw & 0x3333333333333333ULL) + ((qw >> 2) & 0x3333333333333333ULL);
  qw = (qw + (qw >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
  return (int)((qw * 0x0101010101010101ULL) >> 56);
}
#endif

#endif

#else //GCC
static __inline__ void bitset(unsigned __int64 *qw, int index) { *qw |= (1ULL << index); }
static __inline__ void bitclear(unsigned __int64 *qw, int index) {*qw &= ~(1ULL << index);}

#if(defined(_M_IX64))
static __inline__ int bitscanr(unsigned __int64 qw)
{ unsigned __int64 r1, r2;
  asm
  ( "xorq %0, %0" "\n\t" 
    "bsrq %1, %0" "\n\t"
    : "=&r"(r1), "=&r"(r2)
    : "1"((unsigned __int64)(qw))
    : "cc"
  );
  return r1;
}
static __inline int bitscanf(unsigned __int64 qw)
{ unsigned __int64 r1, r2;
  asm
  ( "xorq %0, %0" "\n\t"
    "bsfq %1, %0" "\n\t"
    : "=&r"(r1), "=&r"(r2)
    : "1"((unsigned __int64)(qw))
    : "cc"
  );
  return r1;
}
#ifdef POPCNT
static __inline__ int popcnt(unsigned __int64 qw)
{ unsigned __int64 r1, r2;
  asm
  ( "popcnt %1, %0" "\n\t"
    : "=&r"(r1), "=&r" (r2)
    : "1"((unsigned __int64) (qw))
    : "cc"
  );
  return r1;
}
#else
static __inline__ int popcnt(unsigned __int64 qw)
{ unsigned __int64 r1, r2, r3;
  asm
  ( "xorq  %0, %0"    "\n\t"
    "testq %1, %1"    "\n\t"
    "jz 2f"           "\n\t"
    "1: "             "\n\t"
    "leaq -1(%1), %2" "\n\t"
    "incq %0"         "\n\t"
    "andq %2, %1"     "\n\t"
    "jnz  1b"         "\n\t"
    "2: "             "\n\t"
    : "=&r"(r1), "=&r"(r2), "=&r"(r3)
    : "1"((unsigned __int64) (qw))
    : "cc");
  return (r1);
}
#endif

#elif(defined(_M_IX86))
int static __inline__ popcnt(unsigned __int64 qw)
{ unsigned int r1, r2, r3, r4;
  asm
  ( "xorl   %0, %0"   "\n\t"
    "testl  %2, %2"   "\n\t"
    "jz     2f"       "\n\t"
    "1: "             "\n\t"
    "leal -1(%2), %1" "\n\t"
    "incl   %0"       "\n\t"
    "andl   %1, %2"   "\n\t"
    "jnz    1b"       "\n\t"
    "2: "             "\n\t"
    "testl   %3, %3"  "\n\t"
    "jz      4f"      "\n\t"
    "3: "             "\n\t"
    "leal -1(%3), %1" "\n\t"
    "incl    %0"      "\n\t"
    "andl    %1, %3"  "\n\t"
    "jnz     3b"      "\n\t"
    "4:  "            "\n\t"
  : "=&q"(r1), "=&q"(r2), "=&q"(r3), "=&q"(r4)
  :  "2"((unsigned int) (qw >> 32)), "3"((unsigned int) qw)
  :  "cc");
  return (r1);
}

int static __inline__ bitscanf(unsigned __int64 qw)
{ unsigned int r1, r2, r3;
  asm
  ( "xor %0, %0"  "\n\t"
    "bsf %1, %0"  "\n\t"
    "jne 1f"      "\n\t"
    "bsf %2, %0"  "\n\t"
    "je 1f"       "\n\t"
    "add $32, %0" "\n\t"    
    "1: "         "\n\t"
  : "=&q"(r1), "=&q"(r2), "=&q"(r3)
  : "1"((unsigned int) qw), "2"((unsigned int) (qw >> 32))
  : "cc");
  return (r1);
}

int static __inline__ bitscanr(unsigned __int64 qw)
{ unsigned int r1, r2, r3;
  asm
  ( "xor  %0, %0" "\n\t"
    "bsr  %1, %0" "\n\t"
    "je  1f"      "\n\t"
    "add $32, %0" "\n\t"
    "jmp 2f"      "\n\t"
    "1: "         "\n\t"
    "bsr %2, %0"  "\n\t"
    "2: "         "\n\t"
  : "=&q"(r1), "=&q"(r2), "=&q"(r3)
  : "1"((unsigned int) (qw >> 32)), "2"((unsigned int) qw)
  : "cc");
  return (r1);
}
#endif

#endif


