
#ifndef COMPATIBILITY_H
#define COMPATIBILITY_H
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform-specific settings, included before we get to anything else
 * in daydreamer.h.
 */
#ifdef __APPLE__
#define DIR_SEP     "/"
#endif

#ifdef __linux__
// This is needed to give access to some of the string handling functions
// that we use.
#define _GNU_SOURCE
#define DIR_SEP     "/"
#endif

#ifdef _WIN32
// Rename windows functions to the posix equivalent where possible,
// provide an implementation where there is no equivalent.
#include <windows.h>
#define strcasecmp      _stricmp
#define strncasecmp     _strnicmp
int _strnicmp(const char *string1, const char *string2, size_t count);
int _stricmp(const char *string1, const char *string2);
char* strcasestr(register char *s, register char *find);
char* strsep(char **stringp, const char *delim);
#define DIR_SEP     "\\"
#define WINDOWS_THREADS
#else
#endif

#ifdef _MSC_VER
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64;
typedef unsigned __int64 uint64_t;
typedef __int16 int16_t;
#else
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#endif

// Cache line size
#define CACHE_LINE_BYTES    64
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#   define  CACHE_ALIGN __declspec(align(CACHE_LINE_BYTES))
#else
#   define  CACHE_ALIGN __attribute__ ((aligned(CACHE_LINE_BYTES)))
#endif

// Threading support
#define	_REENTRANT
#define _PTHREADS
#define _POSIX_PTHREAD_SEMANTICS

// 32 or 64 bit?
#if defined(__x86_64) || \
    defined(_WIN64) || \
    (__SIZEOF_INT__ > 4) || \
    defined(_M_X64)
#   define ARCH_64_BIT
#else
#   define ARCH_32_BIT
#endif

// Endian-ness fixers. I provide my own versions to give support for 64-bit
// types and to avoid the need to link in the winsock library on Windows.
extern bool big_endian;
#define my_ntohll(x) \
    (!big_endian ? \
    ((((uint64_t)(x) & 0xff00000000000000ULL) >> 56) | \
     (((uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
     (((uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
     (((uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
     (((uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
     (((uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
     (((uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
     (((uint64_t)(x) & 0x00000000000000ffULL) << 56)) : \
     (x))
#define my_htonll(x) my_ntohll(x)

#define my_ntohl(x) \
    (!big_endian ? \
    ((((uint32_t)(x) & 0xff000000) >> 24) | \
     (((uint32_t)(x) & 0x00ff0000) >>  8) | \
     (((uint32_t)(x) & 0x0000ff00) <<  8) | \
     (((uint32_t)(x) & 0x000000ff) << 24)) : \
     (x))
#define my_htonl(x) my_ntohl(x)

#define my_ntohs(x) \
    (!big_endian ? \
    ((((uint16_t)(x) & 0xff00) >>  8) | \
     (((uint16_t)(x) & 0x00ff) <<  8)) : \
     (x))
#define my_htons(x) my_ntohs(x)

#ifdef __cplusplus
}
#endif
#endif
