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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
  
#ifndef VOID_PTR_SZ
#define VOID_PTR_SZ sizeof(void *)
#endif

#ifndef _UINTPTR_T_DEFINED
#ifdef  __GNUC__
typedef unsigned long uintptr_t;
#else
typedef unsigned __int64 uintptr_t;
#endif
#define _UINTPTR_T_DEFINED
#endif

static void is_aligned(void *addr, size_t size)
{ assert((size & 0x0F) == 0);
  assert(((uintptr_t)addr & 0x0F) == 0);
}

void aligned_wipe_out(void *aligned_memblock, size_t size, size_t bound)
{ uintptr_t p1,p2;
  if(!aligned_memblock) return;
  assert((bound & (bound - 1)) == 0);
  p2 = (uintptr_t)aligned_memblock;
  p1 = ((uintptr_t *)(p2))[-1];
  memset((void *)p1, 0, (size_t)(size + (bound - 1) + VOID_PTR_SZ));
  ((uintptr_t *)(p2))[-1] = p1;
}

void aligned_free(void *aligned_memblock)
{ uintptr_t p;
  if(!aligned_memblock) return;
  p = (uintptr_t)aligned_memblock;	
  p = ((uintptr_t *)(p))[-1];
  free((void *)p);
}

void *aligned_malloc(size_t size, size_t bound)
{ void *p1 = 0,*p2 = 0;
  uintptr_t mask = ~(uintptr_t)(bound - 1);
  //is the alignment request a power of 2 ?:
  assert((bound & (bound - 1)) == 0);
  p1 = malloc(size + (bound - 1) + VOID_PTR_SZ);
  p2 = (void *)(((uintptr_t)p1 + (bound - 1) + VOID_PTR_SZ) & mask);
  is_aligned(p2, size);
  ((void **)p2)[-1] = p1;
  return p2;
}
