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
static char Version[] = "@(#)util.c	e07@nikhef.nl (Eric Wassenaar) 971113";
#endif

#include "vrfy.h"

/*
** FIXCRLF -- Fix trailing <CR><LF> combination in line
** ----------------------------------------------------
**
**	Unconditionally strip a trailing <LF> from an input line,
**	and optionally strip a preceding <CR>.
**	Both characters must be removed during SMTP input, since
**	they are part of the protocol.
**	But when reading local input, we must just remove the
**	trailing <LF>, being the UNIX canonical <NL> character,
**	and preserve a possible <CR>. This guarantees transparent
**	transmission of files ending in <CR><LF>.
**
**	Returns:
**		None.
**
**	Side effects:
**		The line is changed in place.
*/

void
fixcrlf(line, stripcr)
char *line;				/* the input line to fix */
bool stripcr;				/* also strip CR, if set */
{
	register char *p;

	p = index(line, '\n');
	if (p != NULL)
	{
		/* move back to preceding CR if necessary */
		if (stripcr && ((p > line) && (p[-1] == '\r')))
			p--;

		/* properly terminate the line */
		*p = '\0';
	}
}

/*
** MAXSTR -- Ensure string does not exceed maximum size
** ----------------------------------------------------
**
**	Returns:
**		Pointer to the (possibly truncated) string.
**
**	If necessary, a new string is allocated, and is then
**	truncated, and the original string is left intact.
**	Otherwise the original string is truncated in place.
**
*/

char *
maxstr(string, n, save)
char *string;				/* the string to check */
int n;					/* the maximum allowed size */
bool save;				/* allocate new string, if set */
{
	if (strlength(string) > n)
	{
		if (save)
			string = newstr(string);
		string[n] = '\0';
	}
	return(string);
}

/*
** PRINTABLE -- Expand quote bits/control chars in a string
** --------------------------------------------------------
**
**	Returns:
**		Pointer to static buffer containing the expansion.
**
**	The expanded string is silently truncated if it gets too long.
*/

char *
printable(string)
char *string;				/* the string to expand */
{
	static char buf[BUFSIZ];	/* expanded string buffer */
	register char *p = buf;
	register char *s = string;
	register char c;

	while ((c = *s++) != '\0')
	{
		if (p >= buf + sizeof(buf) - 4)
			break;

		if (!isascii(c))
		{
			*p++ = '\\';
			c &= 0177;
		}
		if (iscntrl(c) && !isspace(c))
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
** XALLOC -- Allocate or reallocate additional memory
** --------------------------------------------------
**
**	Returns:
**		Pointer to (re)allocated buffer space.
**		Aborts if the requested memory could not be obtained.
*/

ptr_t *
xalloc(buf, size)
register ptr_t *buf;			/* current start of buffer space */
siz_t size;				/* number of bytes to allocate */
{
	if (buf == NULL)
		buf = malloc(size);
	else
		buf = realloc(buf, size);

	if (buf == NULL)
	{
		error("Out of memory");
		exit(EX_OSERR);
	}

	return(buf);
}

/*
** ITOA -- Convert integer value to ascii string
** ---------------------------------------------
**
**	Returns:
**		Pointer to static storage containing string.
*/

char *
itoa(n)
int n;					/* value to convert */
{
	static char buf[30];		/* sufficient for 64-bit values */

	(void) sprintf(buf, "%d", n);
	return(buf);
}
