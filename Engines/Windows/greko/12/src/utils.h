//  GREKO Chess Engine
//  (c) 2002-2015 Vladimir Medvedev <vrm@bk.ru>
//  http://greko.su

//  utils.h: some utilities
//  modified: 19-Dec-2014

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

extern FILE* g_log;

U32   GetTime();
void  InitInput();
int   InputAvailable();
void  out(const char* s);
char* ReadInput(char* buf, int sz);
void  RandSeed32(U32 seed);
U32   Rand32();
U64   Rand64();
void  Highlight(bool on);
void  SleepMilliseconds(int ms);

inline void out(const char* s)
{
	printf("%s", s);
	if (g_log)
	{
		fprintf(g_log, "%s", s);
		fflush(g_log);
	}
}

template <typename T>
inline void out(const char* format, T arg)
{
	printf(format, arg);
	if (g_log)
	{
		fprintf(g_log, format, arg);
		fflush(g_log);
	}
}

template <typename T1, typename T2>
inline void out(const char* format, T1 arg1, T2 arg2)
{
	printf(format, arg1, arg2);
	if (g_log)
	{
		fprintf(g_log, format, arg1, arg2);
		fflush(g_log);
	}
}

inline void out(const char* format, const std::string& str)
{
	printf(format, str.c_str());
	if (g_log)
	{
		fprintf(g_log, format, str.c_str());
		fflush(g_log);
	}
}

inline bool Is(const std::string& cmd, const std::string& pattern, size_t minLen)
{
	return pattern.find(cmd) == 0 && cmd.length() >= minLen;
}

class TokenString
{
public:
	TokenString() : m_curr(std::string::npos) {}
	TokenString(std::string s): m_line(s), m_curr(0) {}

	std::string GetToken()
	{
		std::string res;
		if (m_curr == std::string::npos)
			res = "";
		else
		{
			size_t next = m_line.find(" ", m_curr);
			if (next == std::string::npos)
			{
				res = m_line.substr(m_curr, m_line.length() - m_curr);
				m_curr = std::string::npos;
			}
			else
			{
				res = m_line.substr(m_curr, next - m_curr);
				m_curr = next + 1;
			}
		}
		return res;
	}
	const std::string& Str() const { return m_line; }
private:
	std::string m_line;
	size_t m_curr;
};

bool CaseInsensitiveEquals(const std::string& s1, const std::string& s2);

#endif
