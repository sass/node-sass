/*
 * This source file is part of the bstring string library.  This code was
 * written by Paul Hsieh in 2002-2010, and is covered by either the 3-clause 
 * BSD open source license or GPL v2.0. Refer to the accompanying documentation 
 * for details on usage and license.
 */

/*
 * bsafe.c
 *
 * This is an optional module that can be used to help enforce a safety
 * standard based on pervasive usage of bstrlib.  This file is not necessarily
 * portable, however, it has been tested to work correctly with Intel's C/C++
 * compiler, WATCOM C/C++ v11.x and Microsoft Visual C++.
 */

#include <stdio.h>
#include <stdlib.h>
#include "bsafe.h"

static int bsafeShouldExit = 1;

char * strcpy (char *dst, const char *src);
char * strcat (char *dst, const char *src);

char * strcpy (char *dst, const char *src) {
	dst = dst;
	src = src;
	fprintf (stderr, "bsafe error: strcpy() is not safe, use bstrcpy instead.\n");
	if (bsafeShouldExit) exit (-1);
	return NULL;
}

char * strcat (char *dst, const char *src) {
	dst = dst;
	src = src;
	fprintf (stderr, "bsafe error: strcat() is not safe, use bstrcat instead.\n");
	if (bsafeShouldExit) exit (-1);
	return NULL;
}

#if !defined (__GNUC__) && (!defined(_MSC_VER) || (_MSC_VER <= 1310))
char * (gets) (char * buf) {
	buf = buf;
	fprintf (stderr, "bsafe error: gets() is not safe, use bgets.\n");
	if (bsafeShouldExit) exit (-1);
	return NULL;
}
#endif

// char * (strncpy) (char *dst, const char *src, size_t n) {
// 	dst = dst;
// 	src = src;
// 	n = n;
// 	fprintf (stderr, "bsafe error: strncpy() is not safe, use bmidstr instead.\n");
// 	if (bsafeShouldExit) exit (-1);
// 	return NULL;
// }

char * (strncat) (char *dst, const char *src, size_t n) {
	dst = dst;
	src = src;
	n = n;
	fprintf (stderr, "bsafe error: strncat() is not safe, use bstrcat then btrunc\n\tor cstr2tbstr, btrunc then bstrcat instead.\n");
	if (bsafeShouldExit) exit (-1);
	return NULL;
}

char * (strtok) (char *s1, const char *s2) {
	s1 = s1;
	s2 = s2;
	fprintf (stderr, "bsafe error: strtok() is not safe, use bsplit or bsplits instead.\n");
	if (bsafeShouldExit) exit (-1);
	return NULL;
}

// char * (strdup) (const char *s) {
// 	s = s;
// 	fprintf (stderr, "bsafe error: strdup() is not safe, use bstrcpy.\n");
// 	if (bsafeShouldExit) exit (-1);
// 	return NULL;
// }
