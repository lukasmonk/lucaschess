
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

#ifdef _MSC_VER
#  define S64_FORMAT "%I64d"
#  define U64_FORMAT "%016I64X"
#else
#  define S64_FORMAT "%lld"
#  define U64_FORMAT "%016llX"
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
   double start_cpu;
   double elapsed_real;
   double elapsed_cpu;
   bool running;
};



/*
BitBases
*/
#define USE_EGBB _WIN32

#if defined _WIN32

#define _WKING     1
#define _WQUEEN    2
#define _WROOK     3
#define _WBISHOP   4
#define _WKNIGHT   5
#define _WPAWN     6
#define _BKING     7
#define _BQUEEN    8
#define _BROOK     9
#define _BBISHOP  10
#define _BKNIGHT  11
#define _BPAWN    12
#define _NOTFOUND 99999

typedef int (*PPROBE_EGBB) (int player, int w_king, int b_king,
			   int piece1, int square1,int piece2,int square2);
extern PPROBE_EGBB probe_egbb;
extern int egbb_is_loaded;

#endif
/*
end BitBases
*/

// functions

extern void   util_init             ();

extern void   my_random_init        ();
extern int    my_random             (int n);

extern sint64 my_atoll              (const char string[]);

extern int    my_round              (double x);

extern void * my_malloc             (int size);
extern void   my_free               (void * address);

extern void   my_fatal              (const char format[], ...);

extern bool   my_file_read_line     (FILE * file, char string[], int size);

extern bool   my_string_empty       (const char string[]);
extern bool   my_string_equal       (const char string_1[], const char string_2[]);
extern char * my_strdup             (const char string[]);

extern void   my_string_clear       (const char * * variable);
extern void   my_string_set         (const char * * variable, const char string[]);

extern void   my_timer_reset        (my_timer_t * timer);
extern void   my_timer_start        (my_timer_t * timer);
extern void   my_timer_stop         (my_timer_t * timer);

extern double my_timer_elapsed_real (const my_timer_t * timer);
extern double my_timer_elapsed_cpu  (const my_timer_t * timer);
extern double my_timer_cpu_usage    (const my_timer_t * timer);

#endif // !defined UTIL_H

// end of util.h

