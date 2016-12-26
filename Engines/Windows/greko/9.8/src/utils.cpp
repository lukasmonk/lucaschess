//  GREKO Chess Engine
//  (c) 2002-2012 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.110mb.com

//  utils.cpp: some utilities
//  modified: 31-Dec-2012

#include "utils.h"

extern FILE *g_log;
static U32 g_rand = 42;

char* ReadInput(char *buf, int sz)
{
  buf = fgets(buf, sz, stdin);
  if (g_log)
  {
    fprintf(g_log, "%s", buf);
    fflush(g_log);
  }
  buf[strlen(buf) - 1] = 0;
  return buf;
}


//
// Pseudorandom generator - D.Knuth, H.W.Lewis
//

void RandSeed32(U32 seed)
{
  g_rand = seed;
}


U32 Rand32()
{
  g_rand = 1664525L * g_rand + 1013904223L;
  return g_rand;
}


U64 Rand64()
{
  U64 r = Rand32();
  r <<= 16;
  r ^= Rand32();
  r <<= 16;
  r ^= Rand32();
  r <<= 16;
  r ^= Rand32();
  r <<= 16;
  r ^= Rand32();

  return r;
}


