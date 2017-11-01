/*--------------------------------------------------------------------------
    Pawny 1.2, chess engine (source code).
    Copyright (C) 2009 - 2016 by Mincho Georgiev.
    
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
#if(defined(_M_IX86) || defined(__i386__))
static  __forceinline int bitscanf(uint64 qw)
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
static  __forceinline int bitscanr(uint64 qw)
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
#else if(defined(_M_X64) || defined(__x86_64__))
static  __forceinline int bitscanf(uint64 qw)
{ __asm
  { xor rax,rax
    bsf rax,qword ptr[qw]
  }
}
static  __forceinline int bitscanr(uint64 qw)
{ __asm
  { xor rax,rax
    bsr rax,qword ptr[qw]
  }
}
#endif

#if((defined(_M_X64) || defined(__x86_64__)) && defined(POPCNT))
#include <smmintrin.h>
static  __inline int popcnt(uint64 qw)
{ return _mm_popcnt_u64(qw);
}
#else
static __forceinline int popcnt(uint64 qw)
{ return 
   (bittable[(qw & 0xFFFF)] + 
    bittable[(qw >> 16) & 0xFFFF] + 
    bittable[(qw >> 32) & 0xFFFF] + 
    bittable[(qw >> 48) & 0xFFFF]);
}
#endif

#else //GCC
#if(defined(__x86_64__))
#include <inttypes.h>
static __inline__ int bitscanr(uint64_t qw)
{ uint64_t r1, r2;
  asm
  ( "xorq %0, %0" "\n\t" 
    "bsrq %1, %0" "\n\t"
    : "=&r"(r1), "=&r"(r2)
    : "1"((uint64_t)(qw))
    : "cc"
  );
  return r1;
}
static __inline__ int bitscanf(uint64_t qw)
{ uint64_t r1, r2;
  asm
  ( "xorq %0, %0" "\n\t"
    "bsfq %1, %0" "\n\t"
    : "=&r"(r1), "=&r"(r2)
    : "1"((uint64_t)(qw))
    : "cc"
  );
  return r1;
}

#elif(defined(__i386__))
int static __inline__ bitscanf(uint64 qw)
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

int static __inline__ bitscanr(uint64 qw)
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
#if(defined(__x86_64__) && defined(POPCNT))
static __inline__ int popcnt(uint64_t qw)
{ uint64_t r1, r2;
  asm
  ( "popcnt %1, %0" "\n\t"
    : "=&r"(r1), "=&r" (r2)
    : "1"((uint64_t) (qw))
    : "cc"
  );
  return r1;
}
#else
static __inline__ int popcnt(uint64_t qw)
{ return 
   (bittable[(qw & 0xFFFF)] + 
    bittable[(qw >> 16) & 0xFFFF] + 
    bittable[(qw >> 32) & 0xFFFF] + 
    bittable[(qw >> 48) & 0xFFFF]);
}
#endif
#endif
