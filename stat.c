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
static char Version[] = "@(#)stat.c	e07@nikhef.nl (Eric Wassenaar) 970926";
#endif

#include "vrfy.h"

#if !defined(ERRLIST_DEFINED)
extern char *sys_errlist[];
extern int sys_nerr;
#endif

extern int SmtpErrno;		/* saved errno from system calls */
extern char *SmtpPhase;		/* connection state message */
extern char *CurHostName;	/* remote host that is being contacted */
extern char SmtpErrorBuffer[];	/* smtp temporary failure message */


/*
 * Status messages.
 */

static char *SysExMsg[] =
{
	/* 64 EX_USAGE       */	"500 Bad usage",
	/* 65 EX_DATAERR     */	"501 Data format error",
	/* 66 EX_NOINPUT     */	"550 Cannot open input",
	/* 67 EX_NOUSER      */	"550 User unknown",
	/* 68 EX_NOHOST      */	"550 Host unknown",
	/* 69 EX_UNAVAILABLE */	"554 Service unavailable",
	/* 70 EX_SOFTWARE    */	"554 Internal error",
	/* 71 EX_OSERR       */	"451 Operating system error",
	/* 72 EX_OSFILE      */	"554 System file missing",
	/* 73 EX_CANTCREAT   */	"550 Can't create output",
	/* 74 EX_IOERR       */	"451 I/O error",
	/* 75 EX_TEMPFAIL    */	"250 Deferred",
	/* 76 EX_PROTOCOL    */	"554 Remote protocol error",
	/* 77 EX_NOPERM      */	"550 Insufficient permission",
	/* 78 EX_CONFIG      */	"554 Local configuration error",
#ifdef notdef
	/* 79 EX_AMBUSER     */	"550 User ambiguous"
#endif
};

static int N_SysEx = sizeof(SysExMsg) / sizeof(SysExMsg[0]);

#define EX__BASE EX_USAGE

/*
** STATSTRING -- Fetch message describing result status
** ----------------------------------------------------
**
**	Returns:
**		Pointer to appropriate status message.
*/

char *
statstring(stat)
int stat;				/* result status */
{
	static char buf[BUFSIZ];

	if (stat == EX_SUCCESS)
	{
		(void) sprintf(buf, "250 Ok");
		return(buf);
	}

	stat -= EX__BASE;
	if (stat < 0 || stat >= N_SysEx)
	{
		(void) sprintf(buf, "554 Unknown status %d", stat + EX__BASE);
		return(buf);
	}

	return(SysExMsg[stat]);
}

/*
** ERRSTRING -- Fetch message describing system call errors
** --------------------------------------------------------
**
**	Returns:
**		Pointer to appropriate error message.
*/

char *
errstring(err)
int err;				/* errno from system calls */
{
	static char buf[BUFSIZ];

	switch (err)
	{
	    case ETIMEDOUT:
	    case ECONNRESET:
	    case EIO:
		if (err == ECONNRESET)
			(void) strcpy(buf, "Connection reset");
		else
			(void) strcpy(buf, sys_errlist[err]);
		if (SmtpPhase != NULL)
		{
			(void) strcat(buf, " during ");
			(void) strcat(buf, SmtpPhase);
		}
		if (CurHostName != NULL)
		{
			(void) strcat(buf, " with ");
			(void) strcat(buf, CurHostName);
		}
		return(buf);

	    case EHOSTUNREACH:
	    case EHOSTDOWN:
	    case ENETUNREACH:
	    case ENETDOWN:
		if (CurHostName == NULL)
			break;
		(void) sprintf(buf, "Host %s is unreachable", CurHostName);
		return(buf);

	    case ECONNREFUSED:
		if (CurHostName == NULL)
			break;
		(void) sprintf(buf, "Connection refused by %s", CurHostName);
		return(buf);
	}

	if (err > 0 && err < sys_nerr)
		return((char *)sys_errlist[err]);

	(void) sprintf(buf, "Error %d", err);
	return(buf);
}

/*
** GIVERESPONSE -- Issue status message about transaction result
** -------------------------------------------------------------
**
**	Returns:
**		None.
*/

void
giveresponse(stat)
int stat;				/* result status */
{
	char buf[BUFSIZ];
	register char *p;

	if (stat == EX_TEMPFAIL)
	{
		if (h_errno == TRY_AGAIN)
			/* temporary nameserver failure */
			p = "Hostname lookup failure";
		else if (SmtpErrno != 0)
			/* non-fatal system call failure */
			p = errstring(SmtpErrno);
		else
			/* temporary smtp failure reply message received */
			p = SmtpErrorBuffer;

		/* add extra information for temporary failures */
		if (p == NULL || p[0] == '\0')
			p = "Transient failure";

		(void) sprintf(buf, "%s: %s", statstring(stat), p);
	}
	else if (stat == EX_NOHOST && h_errno != 0)
	{
		/* add extra information from nameserver */
		if (h_errno == HOST_NOT_FOUND)
			p = "Not registered in DNS";
		else if (h_errno == NO_ADDRESS)
			p = "No address or MX record";
		else
			p = "Nameserver lookup failure";

		(void) sprintf(buf, "%s (%s)", statstring(stat), p);
	}
	else
		(void) sprintf(buf, "%s", statstring(stat));

	/* issue status message */
	message(buf);
}
