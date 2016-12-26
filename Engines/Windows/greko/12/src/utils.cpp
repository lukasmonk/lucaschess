//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  utils.cpp: some utilities
//  modified: 19-Dec-2014

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

void RandSeed32(U32 seed) { g_rand = seed; }

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

bool CaseInsensitiveEquals(const std::string& s1, const std::string& s2)
{
	if (s1.length() != s2.length())
		return false;
	for (size_t i = 0; i < s1.length(); ++i)
	{
		if (toupper(s1[i]) != toupper(s2[i]))
			return false;
	}
	return true;
}

