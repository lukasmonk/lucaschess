
// util.h

#ifndef UTIL_H
#define UTIL_H

// includes

#include <cstdio>

// constants

#undef FALSE
#define FALSE 0

#undef TRUE
#define TRUE 1

#ifdef DEBUG
#  undef DEBUG
#  define DEBUG TRUE
#else
#  define DEBUG FALSE
#endif

// disable affinity for non-windows builds
#if !defined(_WIN32) && !defined(_MSC_VER)
#define NO_AFFINITY
#endif
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#  define    S64_FORMAT  "%I64d"
#  define    U64_FORMAT  "%016I64X"
#  define  S64_FORMAT_9  "%9I64d"
#  define  S64_FORMAT_10 "%10I64d" 
#else
#  define    S64_FORMAT  "%lld"
#  define  S64_FORMAT_9  "%9lld"
#  define  S64_FORMAT_10 "%10lld" 
#  define    U64_FORMAT  "%016llX"
#endif

// macros

#ifdef _MSC_VER
#  define S64(u) (u##i64)
#  define U64(u) (u##ui64)
#else
#  define S64(u) (u##LL)
#  define U64(u) (u##ULL)
#endif

#undef ASSERT
#if DEBUG
#  define ASSERT(a) { if (!(a)) my_fatal("file \"%s\", line %d, assertion \"" #a "\" failed\n",__FILE__,__LINE__); }
#else
#  define ASSERT(a)
#endif

// types

typedef signed char sint8;
typedef unsigned char uint8;

typedef signed short sint16;
typedef unsigned short uint16;

typedef signed int sint32;
typedef unsigned int uint32;

#ifdef _MSC_VER
  typedef signed __int64 sint64;
  typedef unsigned __int64 uint64;
#else
  typedef signed long long int sint64;
  typedef unsigned long long int uint64;
#endif

struct my_timer_t {
   double start_real;
   double elapsed_real;
   bool running;
};

// functions

#ifdef _WIN32
  #include <windows.h>
  inline void Idle(void) {
    Sleep(1);
  }
  inline void Idle500msecs(void){
	  Sleep(500);
  }
#else
  #include <unistd.h>
  inline void Idle(void) {
    usleep(1000);
  }
#endif
extern void   util_init             ();

extern void   my_random_init        ();
extern int    my_random_int         (int n);
extern double my_random_double      ();

extern sint64 my_atoll              (const char string[]);

extern int    my_round              (double x);

extern void * my_malloc             (size_t size);
extern void * my_realloc            (void * address, size_t size);
extern void   my_free               (void * address);

extern void   my_log_open           (const char file_name[]);
extern void   my_log_close          ();

extern void   my_log                (const char format[], ...);
extern void   my_fatal              (const char format[], ...);

extern bool   my_file_read_line     (FILE * file, char string[], int size);

extern bool   my_string_empty       (const char string[]);
extern bool   my_string_equal       (const char string_1[], const char string_2[]);
extern bool   my_string_case_equal  (const char string_1[], const char string_2[]);
extern char * my_strdup             (const char string[]);

extern void   my_string_clear       (const char * * variable);
extern void   my_string_set         (const char * * variable, const char string[]);

extern void   my_timer_reset        (my_timer_t * timer);
extern void   my_timer_start        (my_timer_t * timer);
extern void   my_timer_stop         (my_timer_t * timer);

extern double my_timer_elapsed_real (const my_timer_t * timer);


#endif // !defined UTIL_H

// end of util.h
