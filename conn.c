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
static char Version[] = "@(#)conn.c	e07@nikhef.nl (Eric Wassenaar) 970826";
#endif

#include "vrfy.h"

extern int verbose;
extern int debug;

#define incopy(a)	*((struct in_addr *)(a))
#define setalarm(n)	(void) alarm((unsigned int)(n))

char *CurHostName = NULL;	/* remote host we are connecting to */
char *MyHostName = NULL;	/* my own fully qualified host name */

int ConnTimeout = CONNTIMEOUT;	/* timeout in secs for connect */
int ReadTimeout = READTIMEOUT;	/* timeout in secs for read reply */

/*
** SFGETS -- Read an input line, using timeout
** -------------------------------------------
**
**	Returns:
**		Pointer to start of input line.
**		NULL on error (including timeout).
*/

static jmp_buf Timeout;

sigtype_t
/*ARGSUSED*/
timer(sig)
int sig;
{
	longjmp(Timeout, 1);
	/*NOTREACHED*/
}


char *
sfgets(buf, size, fp)
char *buf;				/* input buffer */
int size;				/* size of input buffer */
FILE *fp;				/* input channel */
{
	register char *p = NULL;

	if (setjmp(Timeout) != 0)
	{
		errno = ETIMEDOUT;
		return(NULL);
	}

	(void) signal(SIGALRM, timer);
	setalarm(ReadTimeout);
	while ((p == NULL) && !feof(fp) && !ferror(fp))
	{
		errno = 0;
		p = fgets(buf, size, fp);
		if (errno == EINTR)
			clearerr(fp);
	}
	setalarm(0);

	if ((p == NULL) && feof(fp) && (errno == 0))
		errno = ECONNRESET;
	if ((p == NULL) && ferror(fp) && (errno == 0))
		errno = EIO;
	return(p);
}

/*
** MAKECONNECTION -- Establish SMTP connection to remote host
** ----------------------------------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	Outputs:
**		Sets outfile and infile to the output and input channel.
**		Sets CurHostName to the name of the remote host.
*/

int
makeconnection(host, outfile, infile)
char *host;				/* host name to connect to */
FILE **outfile;				/* smtp output channel */
FILE **infile;				/* smtp input channel */
{
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	static char hostname[MAXHOST+1];
	struct in_addr inaddr[MAXADDRS];
	int naddrs;
	ipaddr_t addr;
	int sock;
	register int i;

/*
 * Reset state.
 */
	bzero((char *)&sin, sizeof(sin));
	CurHostName = NULL;

	errno = 0;
	h_errno = 0;

	if (host == NULL || host[0] == '\0')
		host = "localhost";

/*
 * Check for dotted quad, potentially within brackets.
 */
	addr = numeric_addr(host);
	if (addr == NOT_DOTTED_QUAD)
		addr = inet_addr(host);

/*
 * Fetch the ip addresses of the given host.
 */
	if (addr != NOT_DOTTED_QUAD)
	{
		inaddr[0].s_addr = addr;
		naddrs = 1;

		hp = gethostbyaddr((char *)&inaddr[0], INADDRSZ, AF_INET);
		if (hp != NULL)
			host = (char *)hp->h_name;
	}
	else
	{
		hp = gethostbyname(host);
		if (hp == NULL)
		{
			/* cannot contact nameserver, force retry */
			if (errno == ETIMEDOUT || errno == ECONNREFUSED)
				h_errno = TRY_AGAIN;

			/* nameserver could not resolve name properly */
			if (h_errno == TRY_AGAIN)
				return(EX_TEMPFAIL);

			/* no address found by nameserver */
			return(EX_NOHOST);
		}
		host = (char *)hp->h_name;

		for (i = 0; i < MAXADDRS && hp->h_addr_list[i]; i++)
			inaddr[i] = incopy(hp->h_addr_list[i]);
		naddrs = i;
	}

	(void) strncpy(hostname, host, MAXHOST);
	hostname[MAXHOST] = '\0';
	CurHostName = hostname;

/*
 * Try to make connection to each of the addresses in turn.
 */
	sp = getservbyname("smtp", "tcp");
	if (sp == NULL)
		return(EX_OSERR);

	for (i = 0; i < naddrs; i++)
	{
		sin.sin_family = AF_INET;
		sin.sin_port = sp->s_port;
		sin.sin_addr = inaddr[i];

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
			return(EX_TEMPFAIL);

		if (verbose >= 2 || debug)
		{
			printf("connecting to %s (%s) port %d\n", CurHostName,
				inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
		}

		if (setjmp(Timeout) != 0)
		{
			(void) close(sock);
			errno = ETIMEDOUT;
			continue;
		}

		(void) signal(SIGALRM, timer);
		setalarm(ConnTimeout);
		if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
			int err = errno;
			setalarm(0);
			(void) close(sock);
			errno = err;
			if (errno == EINTR)
				errno = ETIMEDOUT;
			if (errno == ECONNREFUSED)
				return(EX_TEMPFAIL);
			continue;
		}
		setalarm(0);

		*outfile = fdopen(sock, "w");
		*infile  = fdopen(dup(sock), "r");

		if (*infile == NULL || *outfile == NULL)
		{
			int err = errno;
			if (*infile != NULL)
				(void) fclose(*infile);
			if (*outfile != NULL)
				(void) fclose(*outfile);
			errno = err;
			return(EX_TEMPFAIL);
		}

		if (debug)
			printf("connected to %s\n", CurHostName);

		errno = 0;
		h_errno = 0;
		return(EX_SUCCESS);
	}

	return(EX_TEMPFAIL);
}

/*
** SETMYHOSTNAME -- Determine own fqdn hostname
** --------------------------------------------
**
**	Returns:
**		None.
**
**	Outputs:
**		Sets MyHostName to the local hostname.
*/

void
setmyhostname()
{
	static char hostname[MAXHOST+1];
	int status;

	if (MyHostName == NULL)
	{
		status = getmyhostname(hostname);
		if (status != EX_SUCCESS)
		{
			giveresponse(status);
			exit(status);
		}

		MyHostName = hostname;
	}
}

/*
** GETMYHOSTNAME -- Determine own fqdn hostname
** --------------------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	Outputs:
**		Stores hostname in given buffer.
*/

int
getmyhostname(hostname)
char *hostname;				/* buffer to store host name */
{
	struct hostent *hp;

	errno = 0;
	h_errno = 0;

	if (gethostname(hostname, MAXHOST) < 0)
	{
		perror("gethostname");
		return(EX_OSERR);
	}
	hostname[MAXHOST] = '\0';

	hp = gethostbyname(hostname);
	if (hp == NULL)
	{
		/* cannot contact nameserver, force retry */
		if (errno == ETIMEDOUT || errno == ECONNREFUSED)
			h_errno = TRY_AGAIN;

		/* nameserver could not resolve name properly */
		if (h_errno == TRY_AGAIN)
			return(EX_TEMPFAIL);

		/* no address found by nameserver */
		return(EX_NOHOST);
	}

	(void) strncpy(hostname, hp->h_name, MAXHOST);
	hostname[MAXHOST] = '\0';
	return(EX_SUCCESS);
}

/*
** INTERNET -- Check whether given name has an internet address
** ------------------------------------------------------------
**
**	Returns:
**		TRUE if an internet address exists.
**		FALSE otherwise.
**
**	The given name can be a dotted quad, perhaps between
**	square brackets. If not, an A resource record must exist.
**
**	Note that we do not check the status after a negative return
**	from gethostbyname. Failure can be due to nameserver timeout,
**	in which case the result is still undecided.
**	Currently we consider this an error, so that we won't retry
**	such host during recursive lookups.
*/

bool
internet(host)
char *host;				/* host name to check */
{
	ipaddr_t addr;
	struct hostent *hp;

	/* check dotted quad between brackets */
	addr = numeric_addr(host);
	if (addr != NOT_DOTTED_QUAD)
		return(TRUE);

	/* check plain dotted quad */
	addr = inet_addr(host);
	if (addr != NOT_DOTTED_QUAD)
		return(TRUE);

	/* check if nameserver can resolve it */
	hp = gethostbyname(host);
	if (hp != NULL)
		return(TRUE);

	/* probably not, but could be nameserver timeout */
	return(FALSE);
}

/*
** NUMERIC_ADDR -- Check if we have a dotted quad between brackets
** ---------------------------------------------------------------
**
**	Returns:
**		The numeric address if yes.
**		NOT_DOTTED_QUAD if not.
*/

ipaddr_t
numeric_addr(host)
char *host;				/* host name to check */
{
	ipaddr_t addr;
	register char *p;

	if (host[0] != '[')
		return(NOT_DOTTED_QUAD);

	p = index(host+1, ']');
	if (p == NULL || p[1] != '\0')
		return(NOT_DOTTED_QUAD);

	*p = '\0';
	addr = inet_addr(host+1);
	*p = ']';

	return(addr);
}
