/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char Version[] = "@(#)util.c	e07@nikhef.nl (Eric Wassenaar) 921012";
#endif

#include <stdio.h>
#include <ctype.h>
#include <sysexits.h>
#include <sys/types.h>

typedef char	ptr_t;		/* generic pointer type; will become void */
typedef u_int	siz_t;		/* general size type; will become int */

typedef int	bool;
#define TRUE	1
#define FALSE	0

extern void exit();

/* util.c */
char *expandquotes();
char *xalloc();

/*
** EXPANDQUOTES -- Expand quote bits/control chars in a string
** -----------------------------------------------------------
**
**	Returns:
**		Pointer to static buffer containing the expansion.
*/

char *
expandquotes(string, ctrl)
char *string;				/* the string to expand */
bool ctrl;				/* expand control chars, if set */
{
	static char buf[2*BUFSIZ];	/* expanded string buffer */
	register char *p = buf;
	register char *s = string;
	register char c;

	while (*s != '\0')
	{
		c = *s++;
		if (!isascii(c))
		{
			*p++ = '\\';
			c &= 0177;
		}
		if (ctrl && iscntrl(c) && !isspace(c))
		{
			*p++ = '^';
			c ^= 0100;
		}
		*p++ = c;
	}
	*p = '\0';

	return(buf);
}

/*
** XALLOC -- Allocate additional memory
** ------------------------------------
**
**	Returns:
**		Pointer to allocated buffer space.
**		Aborts if the requested memory could not be obtained.
*/

char *
xalloc(size)
int size;				/* number of bytes to allocate */
{
	register char *buf;		/* pointer to new storage */
	extern ptr_t *malloc();

	buf = (char *)malloc((siz_t)size);
	if (buf == NULL)
	{
		(void) fprintf(stderr, "Out of memory\n");
		exit(EX_OSERR);
	}

	return(buf);
}
