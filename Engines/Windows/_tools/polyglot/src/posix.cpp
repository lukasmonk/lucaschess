
// posix.cpp

// includes

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifdef _WIN32

#include <windows.h>

#else

#include <sys/time.h> // Mac OS X needs this one first
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#endif

#include "posix.h"
#include "util.h"

#ifndef _WIN32
// prototypes

static double duration (const struct timeval *t);

// functions


// input_available()

bool input_available() {

   int val;
   fd_set set[1];
   struct timeval time_val[1];

   FD_ZERO(set);
   FD_SET(STDIN_FILENO,set);

   time_val->tv_sec = 0;
   time_val->tv_usec = 0;

   val = select(STDIN_FILENO+1,set,NULL,NULL,time_val);
   if (val == -1) my_fatal("input_available(): select(): %s\n",strerror(errno));

   return val != 0;
}


// now_real()

double now_real() {

   struct timeval tv[1];
   struct timezone tz[1];

   tz->tz_minuteswest = 0;
   tz->tz_dsttime = 0; // DST_NONE not declared in linux

   if (gettimeofday(tv,tz) == -1) {
      my_fatal("now_real(): gettimeofday(): %s\n",strerror(errno));
   }

   return duration(tv);
}

// now_cpu()

double now_cpu() {

   struct rusage ru[1];

   if (getrusage(RUSAGE_SELF,ru) == -1) {
      my_fatal("now_cpu(): getrusage(): %s\n",strerror(errno));
   }

   return duration(&ru->ru_utime);
}

// duration()

static double duration(const struct timeval *tv) {

   return tv->tv_sec + tv->tv_usec * 1E-6;
}

#else

double now_real(void) {
  return (double) GetTickCount() / 1000.0;
}

#endif

// end of posix.cpp
