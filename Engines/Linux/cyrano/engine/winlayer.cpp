
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Gerbil
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Gerbil chess program project.
//
//	Gerbil is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Gerbil is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Gerbil; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include "engine.hpp"
#ifdef TARGET_WIN
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Returns system time in milliseconds.  If you are porting this away from
//	windows, this might cause you minor trouble, but you should be able to
//	find something.  High clock resolution is a plus, but if you don't have
//	it it's not absolutely fatal.

#ifdef TARGET_WIN

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
 
struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  return 0;
}
#endif

static int newEpoch = 0;

unsigned TmNow(void)
{
#ifdef TARGET_WIN
    return GetTickCount();
#else
    struct timeval tv[1];
    gettimeofday(tv,NULL);

    return (tv->tv_sec - newEpoch ) * 1000 + tv->tv_usec / 1000;
#endif
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VReseed(void)
{
#ifdef TARGET_WIN
	SYSTEMTIME	st;

	GetSystemTime(&st);
	srand((st.wHour ^ st.wMinute ^ st.wSecond ^ st.wMilliseconds) & 0x7FFF);
#else
    struct timeval tv[1];
    gettimeofday(tv,NULL);

    // perhaps useless, but tv_sec * 1000 can not fit 32bits
    newEpoch = tv->tv_sec;

#endif
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static void VGetIniFile(char * szOut, int cbOut)
{
#ifdef TARGET_WIN
    int i;
	GetModuleFileName(GetModuleHandle(NULL), szOut, cbOut);
	for (i = (int)strlen(szOut) - 1; i >= 0; i--)
		if (szOut[i] == '\\') {
			strcpy(szOut + i + 1, "cyrano.ini");
			break;
		}
	Assert(i >= 0);
#else
    strcpy(szOut, "cyrano.ini");
#endif
}

static char const s_aszApp[] = "settings";

void VGetProfileSz(const char * szKey, const char * szDefault,
	char * szOut, int cbOut)
{
	char	aszPath[512];
#ifdef TARGET_WIN
	VGetIniFile(aszPath, sizeof(aszPath));
	GetPrivateProfileString(s_aszApp, szKey, szDefault,
		szOut, cbOut, aszPath);
#else
    strcpy(szOut, szDefault);
#endif
}

int IGetProfileInt(const char * szKey, int iDefault)
{
#ifdef TARGET_WIN
	char	aszPath[512];
	VGetIniFile(aszPath, sizeof(aszPath));
	return GetPrivateProfileInt(s_aszApp, szKey, iDefault, aszPath);
#else
    return iDefault;
#endif
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VLowPriority(void)
{
#ifdef TARGET_WIN
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
