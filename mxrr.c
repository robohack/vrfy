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
static char Version[] = "@(#)mxrr.c	e07@nikhef.nl (Eric Wassenaar) 990522";
#endif

#include "vrfy.h"

extern int verbose;
extern int debug;

#if PACKETSZ > 8192
#define MAXPACKET PACKETSZ	/* PACKETSZ should be the max udp size (512) */
#else
#define MAXPACKET 8192		/* but tcp packets can be considerably larger */
#endif

typedef union {
	HEADER hdr;
	u_char buf[MAXPACKET];
} querybuf;

#ifndef HFIXEDSZ
#define HFIXEDSZ 12		/* actually sizeof(HEADER) */
#endif

#define MAXMXBUFSIZ (MAXMXHOSTS * (MAXHOST+1))

static char hostbuf[MAXMXBUFSIZ];

char *MxHosts[MAXMXHOSTS];	/* list of names of mx hosts found */

char *dbprefix = DBPREFIX;	/* prefix for debug messages to stdout */

/*
** GETMXBYNAME -- Fetch mx hosts for a domain
** ------------------------------------------
**
**	Returns:
**		Number of mx hosts found.
**
**	Outputs:
**		The global array MxHosts contains the mx names.
*/

int
getmxbyname(domain)
char *domain;				/* domain to get mx hosts for */
{
	querybuf answer;		/* answer buffer from nameserver */
	HEADER *hp;			/* answer buffer header */
	int ancount, qdcount;		/* answer count and query count */
	u_char *msg, *eom, *cp;		/* answer buffer positions */
	int type, class, dlen;		/* record type, class and length */
	u_short pref;			/* mx preference value */
	u_short prefer[MAXMXHOSTS];	/* saved preferences of mx records */
	char *bp;			/* hostbuf pointer */
	int nmx;			/* number of mx hosts found */
	register int i;
	register int j;
	register int n;

/*
 * Query the nameserver to retrieve mx records for the given domain.
 */
	errno = 0;			/* reset before querying nameserver */
	h_errno = 0;

	n = res_search(domain, C_IN, T_MX, (u_char *)&answer, sizeof(answer));
	if (n < 0)
	{
		if (_res.options & RES_DEBUG)
			printf("%sres_search failed\n", dbprefix);
		return(0);
	}

	errno = 0;			/* reset after we got an answer */

	if (n < HFIXEDSZ)
	{
		h_errno = NO_RECOVERY;
		return(0);
	}

	/* avoid problems after truncation in tcp packets */
	if (n > sizeof(answer))
		n = sizeof(answer);

/*
 * Valid answer received. Skip the query record.
 */
	hp = (HEADER *)&answer;
	qdcount = ntohs((u_short)hp->qdcount);
	ancount = ntohs((u_short)hp->ancount);

	msg = (u_char *)&answer;
	eom = (u_char *)&answer + n;
	cp  = (u_char *)&answer + HFIXEDSZ;

	while (qdcount-- > 0 && cp < eom)
	{
		n = dn_skipname(cp, eom);
		if (n < 0)
			return(0);
		cp += n;
		cp += QFIXEDSZ;
	}

/*
 * Loop through the answer buffer and extract mx records.
 */
	nmx = 0;
	bp = hostbuf;

	while (ancount-- > 0 && cp < eom && nmx < MAXMXHOSTS)
	{
#ifdef obsolete
		if (verbose >= 4 || debug)
			(void) p_rr((qbuf_t *)cp, (qbuf_t *)msg, stdout);
#endif /*obsolete*/

		n = dn_expand(msg, eom, cp, (nbuf_t *)bp, MAXHOST);
		if (n < 0)
			break;
		cp += n;

		type = _getshort(cp);
 		cp += INT16SZ;

		class = _getshort(cp);
 		cp += INT16SZ;

		/* ttl = _getlong(cp); */
 		cp += INT32SZ;

		dlen = _getshort(cp);
		cp += INT16SZ;

		if (type != T_MX || class != C_IN) 
		{
			cp += dlen;
			continue;
		}

		pref = _getshort(cp);
		cp += INT16SZ;

		n = dn_expand(msg, eom, cp, (nbuf_t *)bp, MAXHOST);
		if (n < 0)
			break;
		cp += n;

		prefer[nmx] = pref;
		MxHosts[nmx] = bp;
		nmx++;

		n = strlength(bp) + 1;
		bp += n;
	}

/*
 * Sort all records by preference.
 */
	for (i = 0; i < nmx; i++)
	{
		for (j = i + 1; j < nmx; j++)
		{
			if (prefer[i] > prefer[j])
			{
				register u_short tmppref;
				register char *tmphost;

				tmppref = prefer[i];
				prefer[i] = prefer[j];
				prefer[j] = tmppref;

				tmphost = MxHosts[i];
				MxHosts[i] = MxHosts[j];
				MxHosts[j] = tmphost;
			}
		}
	}

	return(nmx);
}
