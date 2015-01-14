
#include "daydreamer.h"

/*
 * Implementations of functions that don't exist on all platforms.
 * Right now this is just adding some string handling functions for
 * the Windows build and a standard 32-bit PRNG.
 */

#ifdef _WIN32

void srandom_32(unsigned seed)
{
    srand(seed);
}

int32_t random_32(void)
{
    int r = rand();
    r <<= 16;
    r |= rand();
    return r;
}

int64_t random_64(void)
{
    int64_t r = random_32();
    r <<= 32;
    r |= random_32();
    return r;
}

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <string.h>
/*
 * Find the first occurrence of find in s.
 */
char *
#ifdef __STDC__
strcasestr(register char *s, register char *find)
#else
strcasestr(s,find)
register char *s;
register char *find;
#endif
{
	register char c,
	  sc;
	register size_t len;

	if ((c = *find++) != 0) {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while (sc != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *) s);
}

/*
 * Get next token from string *stringp, where tokens are possibly-empty
 * strings separated by characters from delim.  
 *
 * Writes NULs into the string at *stringp to end tokens.
 * delim need not remain constant from call to call.
 * On return, *stringp points past the last NUL written (if there might
 * be further tokens), or is NULL (if there are definitely no more tokens).
 *
 * If *stringp is NULL, strsep returns NULL.
 */
char *
strsep(char **stringp, const char *delim) {
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}

#else

void srandom_32(unsigned seed)
{
    srandom(seed);
}

int32_t random_32(void)
{
    return random();
}

int64_t random_64(void)
{
    int64_t r = random_32();
    r <<= 32;
    r |= random_32();
    return r;
}

#endif
