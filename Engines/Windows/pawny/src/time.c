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

#include "data.h"
#include <time.h>

#ifdef _WIN32
#include <windows.h>
int get_time()
{//returns the time in milliseconds
  SYSTEMTIME st;
  struct timeval t;
  struct tm tmrec;
  time_t current_time;

  current_time = time(0);
  tmrec = *localtime(&current_time);
  t.tv_sec = (int)mktime(&tmrec);
  GetLocalTime(&st);
  t.tv_usec = st.wMilliseconds * 1000;
  return t.tv_sec * 1000 + t.tv_usec / 1000;
}
#else
#include <sys/time.h>
int get_time()
{//returns the time in milliseconds
  struct timeval tval;
  struct timezone tzone;
  int  ms;

  gettimeofday(&tval, &tzone);
  ms = (tval.tv_sec * 1000) + tval.tv_usec/1000;
  return ms;
}
#endif

void time_check()
{ if(get_time() - opt->startup_time >= opt->time_high)
    si->stop_search = true;
}
  
#ifdef _WIN32
//measurement functions:
LARGE_INTEGER start_ticks,stop_ticks;
void timer_start()
{
  QueryPerformanceCounter(&start_ticks);
}

void timer_stop()
{
  unsigned long elapsed;
  LARGE_INTEGER freq;
  
  QueryPerformanceCounter(&stop_ticks);
  QueryPerformanceFrequency(&freq);
  elapsed = (unsigned long)(1000000*(stop_ticks.QuadPart - start_ticks.QuadPart)/freq.QuadPart);
  printf("time elapsed: \n");
  printf("-------------------\n");
  printf("%lu mS\n",elapsed);
  printf("%lu  S\n",(elapsed/1000000));
  printf("-------------------\n");
}
#else
static time_t t1,t2;

void timer_start()
{ 
  time(&t1);
}

void timer_stop()
{
  int s;
  time(&t2);
  s = (int)(t2 - t1);
  printf("time elapsed: \n");
  printf("-------------------\n");
  printf("%d s\n",s);
  printf("-------------------\n");
}
#endif



