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

/*
 * Since this program contains concepts and possibly pieces of code
 * from the sendmail sources, the above copyright notice is included.
 */

/*
 * Written by Eric Wassenaar, Nikhef-H, <e07@nikhef.nl>
 *
 * The officially maintained source of this program is available
 * via anonymous ftp from machine 'ftp.nikhef.nl' [192.16.199.1]
 * in the directory '/pub/network' as 'vrfy.tar.Z'
 *
 * You are kindly requested to report bugs and make suggestions
 * for improvements to the author at the given email address,
 * and to not re-distribute your own modifications to others.
 */

/*
 * Released versions.
 *
 *	910617
 *		Add -t option to set read timeout.
 *		Save errno across smtpquit() calls.
 *	911216
 *		Fetch MX records and verify remotely.
 *		Catch special pseudo-domains.
 *		Add -c option to set connect timeout.
 *		Add -p option to ping mx hosts.
 *	920229
 *		Support parsing of full addresses.
 *		Add -f option to verify address files.
 *		Add -l option to handle errors locally.
 *		Implement recursive mode.
 *		Detect forwarding loops.
 *		Add -s option to strip comments.
 *		Improve recursive loop strategy.
 *		Add undocumented -h -o -m -r options.
 *		Add -n option for alternative suite.
 *	921021
 *		Miscellaneous declaration changes.
 *		Add -e option for expn instead of vrfy.
 *		Various sanity checks.
 *		Fix bug in recursion: save old host.
 *		Add version number to all files.
 *	940525
 *		Adapt for DEC Alpha OSF/1, and BIND 4.9.
 *		General portability changes, port.h conf.h exit.h
 *		Configure relay host for unresolved single hostnames.
 *		Handle 8-bit characters and sendmail V8 meta-chars.
 *		Some error messages slightly modified.
 *		In exit status, temp failures override hard failures.
 *	940929
 *		Various portability changes.
 *		Avoid use of sizeof() for all entities that have a fixed
 *		field width, and use predefined constants instead. This
 *		is necessary for systems without 16 or 32 bit integers.
 *		Fix use of ipaddr_t and struct in_addr appropriately.
 *		All this makes the utility portable to e.g. Cray.
 */

#ifndef lint
static char Version[] = "@(#)vrfy.c	e07@nikhef.nl (Eric Wassenaar) 940929";
#endif

/*
 *			Overview
 *
 * vrfy is a tool to verify electronic mail addresses.
 * It recognizes elementary syntax errors, but can do a lot more,
 * up to complex tasks such as recursively expand mailing lists
 * and detect mail forwarding loops.
 *
 * In its simplest form, vrfy accepts an electronic mail address
 * "user@domain" for which it will figure out the mx hosts for
 * "domain", set up an SMTP session with the primary mx host,
 * and issue the SMTP VRFY command with the given mail address.
 * The reply from the remote host to the VRFY command is printed.
 *
 * If no mx hosts exist, it will try to contact "domain" itself.
 * In case there is no "domain", the address is supposed to
 * represent a local recipient which is verified at "localhost".
 *
 * By default only the primary mx host is contacted, hoping that
 * "user" is local to that machine so that some extra information
 * may be retrieved, such as full name, forwarding, aliasing, or
 * mailing list expansion.
 * With an option one may choose to also query the other mx hosts.
 *
 * For pseudo domains like "uucp" or "bitnet" one can compile in
 * explicit servers to be contacted. They default to "localhost".
 * Not many servers will tell what they are actually going to do
 * with such addresses.
 *
 * Instead of an electronic mail address one can specify the name
 * of a file containing address lists, e.g. mailing list recipient
 * files or .forward files. Verification of all recipients in the
 * file is then attempted.
 *
 * If an explicit additional host name is specified on the command
 * line, verification is carried out at that host, and the input
 * addresses are passed to the host without further parsing.
 *
 * Various levels of verbose output can be selected. Very verbose
 * mode prints all SMTP transactions with the remote host in detail.
 * When even more verbose, an additional SMTP VERB command will be
 * issued, resulting in the display of all actions taken by the
 * remote host when verifying the address. This can be fun.
 *
 * In the special ping mode, the mail exchangers for the specified
 * electronic mail domain will be contacted to check whether they
 * do respond to SMTP requests. No address verification is done.
 *
 * vrfy has built in the basic address parsing rules of sendmail,
 * so it can determine the "domain" part in complicated addresses
 * "comment \"comment\" comment" <user@domain (comment \comment)>
 *
 * Elementary syntax errors are caught locally. If the domain part
 * could not be parsed beyond doubt, the address is passed on to
 * "localhost" hoping to get detailed 'official' error messages.
 * My sendmail.cf has an enormous amount of syntax checking rules.
 * With an option one can reject invalid addresses internally.
 *
 * Another option lets you recursively verify the received replies
 * to the original verified address. This is handy for mailing list
 * expansions, and also to detect possible mail forwarding loops.
 * This works only by the grace of sendmail and other MTAs sending
 * formal address specifications in the VRFY replies.
 *
 * Recursion stops automatically if a local recipient address is
 * received, or if a mail loop is detected. If the received reply
 * is the same as the address that was asked for (modulo comments)
 * the request is retried at its domain itself, unless this was the
 * machine we just queried, or it is not an internet domain host.
 *
 * The default recursion level is set to the MAXHOP value (17) as
 * used by sendmail, but this can be overruled (smaller or larger).
 */

/*
 *			Limitations
 *
 * Many non-sendmail SMTP servers have a lousy VRFY handling.
 * Sometimes this command is not implemented at all. Other servers
 * are only willing to VRFY local recipient names without a domain
 * part (PMDF, VM, MVS).
 * Furthermore, the sendmail V8 server can be configured to refuse
 * the VRFY or EXPN command for privacy reasons.
 *
 * For those hosts there is an option to use the RCPT instead of
 * the VRFY command. This does not give the same information, but
 * it is better than nothing. Usually the HELO and MAIL commands
 * are required as well. Recursive mode is not possible.
 *
 * Some hosts refuse to VRFY the bracketed <comment <user@domain>>
 * but accept the same address without the outermost brackets.
 *
 * Usually hosts return addresses with an abundant amount of nested
 * brackets if you present a bracketed address with comments.
 * Note that sendmail returns "q_paddr" within a new set of brackets.
 * It would be more illustrative if "q_user" were returned instead.
 *
 * An option will strip all comments from addresses to be verified,
 * to avoid accumulating brackets during recursive verification.
 * This is now the default when using the default recursion mode.
 *
 * Some hosts return an error message, but with the 250 status code.
 * As long as there is no '@' in the message, it can do no harm.
 *
 * Some mailing lists have CNAME addresses, but we can now handle
 * these, and do not get into infinite recursion.
 *
 * Some hosts return an unqualified address for local recipients.
 * This is acceptable if it consists only of the pure "local part"
 * but sometimes it is of the form <user@host> which is difficult
 * to trace further.
 *
 * MX records may direct mail to a central mail host. Tracing down
 * may yield an address at a host which is not reachable from the
 * outside world.
 */

/*
 *			Compilation options
 *
 * This program usually compiles without special compilation options,
 * but for some platforms you may have to define special settings.
 * See the Makefile and the header file port.h for details.
 */

/*
 *		Usage: vrfy [options] address [host]
 *
 * This section is still to be supplied.
 * For now, refer to the manual page.
 */

static char Usage[] =
"\
Usage: vrfy [options] [-v] address [host]\n\
File:  vrfy [options] [-v] -f file [host]\n\
Ping:  vrfy [options] [-v] -p domain\n\
Options: [-a] [-d] [-l] [-c secs] [-t secs]\n\
Special: [-L level] [-R] [-s] [-n] [-e]\
";


#include "vrfy.h"

char *HostSpec = NULL;		/* explicit host to be queried */
char *AddrSpec = NULL;		/* address being processed */
char *FileName = NULL;		/* name of file being processed */
int LineNumber = 0;		/* line number into file */
int ExitStat = EX_OK;		/* overall result status */
bool SuprErrs = FALSE;		/* suppress parsing errors, if set */

int debug = 0;			/* debugging level */
int verbose = 0;		/* verbosity level */
int recursive = 0;		/* recursive mode maximum level */
bool stripit = FALSE;		/* strip comments, if set */
bool vrfyall = FALSE;		/* query all mx hosts found, if set */
bool localerr = FALSE;		/* handle errors locally, if set */
bool pingmode = FALSE;		/* ping mx hosts, if set */
bool filemode = FALSE;		/* verify file, if set */
bool helomode = FALSE;		/* issue HELO command, if set */
bool onexmode = FALSE;		/* issue ONEX command, if set */
bool mailmode = FALSE;		/* issue MAIL command, if set */
bool expnmode = FALSE;		/* use EXPN instead of VRFY, if set */
bool rcptmode = FALSE;		/* use RCPT instead of VRFY, if set */

char *ReplyList[MAXREPLY];	/* saved address expansions */
int ReplyCount = 0;		/* number of valid replies */

char *AddrChain[MAXLOOP];	/* address chain in recursive mode */

extern char *MxHosts[MAXMXHOSTS];	/* names of mx hosts found */

extern int ConnTimeout;		/* timeout in secs for connect() */
extern int ReadTimeout;		/* timeout in secs for sfgets() */
extern char *MyHostName;	/* my own fully qualified host name */
extern char *version;		/* program version number */

/*
** MAIN -- Start of program vrfy
** -----------------------------
**
**	Exits:
**		Various possibilities from sysexits.h
*/

int
main(argc, argv)
int argc;
char *argv[];
{
	register char *option;

#ifdef obsolete
	assert(sizeof(u_int) == 4);	/* probably paranoid */
	assert(sizeof(u_short) == 2);	/* perhaps less paranoid */
	assert(sizeof(ipaddr_t) == 4);	/* but this is critical */
#endif

/*
 * Synchronize stdout and stderr in case output is redirected.
 */
	linebufmode(stdout);

/*
 * Fetch command line options.
 */
	while (argc > 1 && argv[1][0] == '-')
	{
	    for (option = &argv[1][1]; *option; option++)
	    {
	    	switch (*option)
	    	{
	    	    case 'd':		/* increment debugging level */
	    		debug++;
	    		break;

	    	    case 'v':		/* increment verbosity level */
	    		verbose++;
	    		break;

	    	    case 'a':		/* query all mx hosts */
	    		vrfyall = TRUE;
	    		break;

	    	    case 'l':		/* handle errors locally */
			localerr = TRUE;
	    		break;

	    	    case 's':		/* strip comments from address */
			stripit = TRUE;
	    		break;

	    	    case 'h':		/* issue HELO command */
			helomode = TRUE;
	    		break;

	    	    case 'o':		/* issue ONEX command */
			onexmode = TRUE;
	    		break;

	    	    case 'm':		/* issue MAIL command */
			mailmode = TRUE;
	    		break;

	    	    case 'e':		/* use EXPN instead of VRFY */
			expnmode = TRUE;
	    		break;

	    	    case 'r':		/* use RCPT instead of VRFY */
			rcptmode = TRUE;
			recursive = 0;
	    		break;

	    	    case 'n':		/* use alternative protocol suite */
			helomode = TRUE;
			onexmode = FALSE;
			mailmode = TRUE;
			rcptmode = TRUE;
			recursive = 0;
	    		break;

	    	    case 'f':		/* verify file */
	    		filemode = TRUE;
			if (pingmode)
				fatal("-f conflicts with -p");
	    		break;

	    	    case 'p':		/* ping mx hosts */
	    		pingmode = TRUE;
			if (filemode)
				fatal("-p conflicts with -f");
	    		break;

		    case 'c':		/* set connect timeout */
			if (argv[2] == NULL || argv[2][0] == '-')
				fatal("Missing timeout value");
			ConnTimeout = atoi(argv[2]);
			if (ConnTimeout <= 0)
				fatal("Illegal timeout value %s", argv[2]);
			--argc; argv++;
			break;

		    case 't':		/* set read timeout */
			if (argv[2] == NULL || argv[2][0] == '-')
				fatal("Missing timeout value");
			ReadTimeout = atoi(argv[2]);
			if (ReadTimeout <= 0)
				fatal("Illegal timeout value %s", argv[2]);
			--argc; argv++;
			break;

		    case 'L' :		/* set recursion level */
			if (argv[2] == NULL || argv[2][0] == '-')
				fatal("Missing recursion level");
			recursive = atoi(argv[2]);
			if (recursive <= 0)
				fatal("Invalid recursion level %s", argv[2]);
			if (recursive > MAXLOOP)
				recursive = MAXLOOP;
			rcptmode = FALSE;
			--argc; argv++;
			break;

		    case 'R' :		/* set default recursion level */
			if (recursive == 0)
				recursive = MAXHOP;
			stripit = TRUE;
			rcptmode = FALSE;
			break;

		    case 'V' :
			printf("Version %s\n", version);
			exit(EX_OK);

	    	    default:
	    		fatal(Usage);
	    	}
	    }

	    --argc; argv++;
	}

/*
 * Scan remaining command line arguments.
 */
	/* must have at least one argument */
	if (argc < 2)
		fatal(Usage);

	/* set optional explicit host to be queried */
	if (argc > 2)
		HostSpec = argv[2];

	/* rest is undefined */
	if (argc > 3)
		fatal(Usage);

/*
 * Miscellaneous initialization.
 */
	/* get own host name before turning on debugging */
	if (helomode || pingmode)
		setmyhostname();

/*
 * Set proper resolver options.
 */
	/* only do RES_DEFNAMES for single host names without dot */
	_res.options |=  RES_DEFNAMES;
	_res.options &= ~RES_DNSRCH;

	/* set nameserver debugging on, if requested */
	if (debug == 2)
		_res.options |= RES_DEBUG;

/*
 * All set. Execute the required function.
 */
	if (pingmode)
		ping(argv[1]);		/* ping the given domain */

	else if (filemode)
		file(argv[1]);		/* verify the given file */

	else
		list(argv[1]);		/* verify the address list */

	exit(ExitStat);
	/*NOTREACHED*/
}

/*
** FATAL -- Abort program when illegal option encountered
** ------------------------------------------------------
**
**	Returns:
**		Aborts after issuing error message.
*/

void
/*VARARGS1*/
fatal(fmt, a, b, c)
char *fmt;				/* format of message */
char *a, *b, *c;			/* optional arguments */
{
	(void) fprintf(stderr, fmt, a, b, c);
	(void) fprintf(stderr, "\n");
	exit(EX_USAGE);
}

/*
** USRERR -- Issue error message during address parsing
** ----------------------------------------------------
**
**	Returns:
**		None.
**
**	Called from various parsing routines when an error is found.
**	Printing of the message is suppressed if necessary.
*/

void
/*VARARGS1*/
usrerr(fmt, a, b, c)
char *fmt;				/* format of message */
char *a, *b, *c;			/* optional arguments */
{
	char msg[BUFSIZ];		/* status message buffer */

	/* suppress message if requested */
	if (SuprErrs)
		return;

	/* issue message with fatal error status */
	(void) sprintf(msg, "554 %s", fmt);
	message(msg, a, b, c);
}

/*
** MESSAGE -- Issue a status message in special format
** ---------------------------------------------------
**
**	Returns:
**		None.
**
**	The status message should begin with 3-digit status code.
*/

void
/*VARARGS1*/
message(msg, a, b, c)
char *msg;				/* status message */
char *a, *b, *c;			/* optional arguments */
{
	char *fmt = &msg[4];		/* format of actual message */

	/* do not print informational messages if not verbose */
	if ((msg[0] == '0' || msg[0] == '1') && !verbose)
		return;

	/* prepend with filename and line number if appropriate */
	if (FileName != NULL)
		printf("%s: line %d: ", FileName, LineNumber);

	/* print the address being processed */
	if (AddrSpec != NULL && *AddrSpec != '\0')
		printf("%s ... ", printable(AddrSpec));

	/* print message itself */
	printf(fmt, a, b, c);
	printf("\n");
}

/*
** RESPONSE -- Process reply message from smtp vrfy request
** --------------------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Valid replies may be saved for later recursion.
**
**	Called from smtpreply() for each reply line received in the
**	vrfy wait phase. Status code 2xx indicate a message with a
**	valid address expansion. More than one such line may arrive.
*/

void
response(msg)
char *msg;				/* status message from reply */
{
	char *address = &msg[4];	/* address expansion in reply */

	/* print the text of the reply */
	printf("%s\n", address);

	/* skip if this is not a valid address expansion */
	if (msg[0] != '2')
		return;

	/* save the reply for later recursion processing */
	if (recursive && AddrSpec != NULL && ReplyCount < MAXREPLY)
	{
		ReplyList[ReplyCount] = newstr(address);
		ReplyCount++;
	}
}

/*
** SHOW -- Print a detailed result status of a transaction
** -------------------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Updates the overall result status.
**
**	In recursive mode, all received replies are verified in turn.
*/

#define tempfail(x) (x == EX_TEMPFAIL || x == EX_OSERR || x == EX_IOERR)

void
show(status, host)
int status;				/* result status of operation */
char *host;				/* remote host that was queried */
{
	/* save result status, keeping previous failures */
	if (status != EX_OK && !tempfail(ExitStat))
		ExitStat = status;

	/* display the appropriate error message */
	if (status != EX_OK)
		giveresponse(status);

	/* special message in ping mode */
	else if (pingmode)
		printf("%s is alive\n", host);

	/* recursively verify the received replies */
	else if (recursive && AddrSpec != NULL && ReplyCount > 0)
		loop(AddrSpec, host);
}

/*
** LOOP -- Recursively verify received address expansions
** ------------------------------------------------------
**
**	Returns:
**		None.
**
**	Called from show() to reprocess the saved valid replies
**	after the smtp session is terminated.
*/

int recursion_level = 0;	/* current recursion level */

void
loop(address, host)
char *address;				/* address we have just verified */
char *host;				/* remote host that was queried */
{
	char *replylist[MAXREPLY];	/* local copy of replies */
	int replycount = ReplyCount;	/* number of replies received */
	char *oldhost = host;		/* host queried for old address */
	char oldaddr[BUFSIZ];		/* parsed original address */
	char newaddr[BUFSIZ];		/* parsed reply to address */
	char hostbuf[BUFSIZ];		/* local copy of domain part */
	char *domain;			/* domain part of address */
	char *SaveFile;
	int SaveLine;
	register int i;

/*
 * Save state.
 */
	SaveFile = FileName;
	SaveLine = LineNumber;

/*
 * Save local copy of replies.
 */
	for (i = 0; i < replycount; i++)
		replylist[i] = ReplyList[i];

/*
 * Parse original address and save local copy. Do not report errors.
 * Loop over the replies if not yet at the maximum recursion level.
 */
	AddrSpec = NULL;
	SuprErrs = TRUE;

	domain = parsespec(address, oldaddr, (char *)NULL);
	if (domain != NULL && recursive && recursion_level < recursive)
	{
		/* put the current address on the chain */
		AddrChain[recursion_level] = oldaddr;

		FileName = oldaddr;
		LineNumber = 0;
		for (i = 0; i < replycount; i++)
		{
			LineNumber++;
			address = replylist[i];

			/* always process address parsing errors */
			AddrSpec = address;
			SuprErrs = FALSE;

			/* make sure it is a single address */
			address = parselist(address);
			if (address == NULL)
				continue;

			/* skip if address cannot be parsed */
			domain = parsespec(address, newaddr, hostbuf);
			if (domain == NULL)
				continue;

			/* skip if this is a local recipient address */
			if (sameword(domain, "localhost"))
				continue;

			/* retry if this is not the same address */
			if (!sameword(newaddr, oldaddr))
				host = NULL;

			/* skip if this host was just queried */
			else if (sameword(domain, oldhost))
				continue;

			/* skip if this is not an internet host */
			else if (!internet(domain))
				continue;

			/* give explicit host a try */
			else
				host = hostbuf;

			/* skip if forwarding loop is detected */
			if (host == NULL && invalidloop(newaddr))
				continue;

			/* reset for message */
			AddrSpec = NULL;

			/* display address */
			message("250 %s", address);

			/* recursively verify the reply */
			recursion_level++;
			vrfy(address, host);
			recursion_level--;
		}
	}

/*
 * Replies were allocated dynamically.
 */
	for (i = 0; i < replycount; i++)
		xfree(replylist[i]);

/*
 * Restore state.
 */
	FileName = SaveFile;
	LineNumber = SaveLine;
}

/*
** FILE -- Process a file containing address lists
** -----------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Shows the status for each transaction done.
*/

void
file(filename)
char *filename;				/* name of file to be verified */
{
	char *addrlist;			/* address list to be verified */
	int status;			/* result status */
	FILE *fp;
	char buf[BUFSIZ];
	register char *p;

/*
 * Terminate if the file could not be opened.
 */
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		perror(filename);
		status = EX_NOINPUT;
		show(status, (char *)NULL);
		return;
	}

	/* truncate to basename to shorten message */
	p = rindex(filename, '/');
	if (p++ != NULL)
		filename = p;

/*
 * Process each line in the file, skipping comment lines.
 */
	FileName = filename;
	LineNumber = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		LineNumber++;
		addrlist = buf;

		/* skip line terminator */
		p = index(addrlist, '\n');
		if (p != NULL)
			*p = '\0';

		/* skip leading whitespace */
		while (is_space(*addrlist))
			addrlist++;

		/* skip comment lines */
		if (*addrlist == '\0' || *addrlist == '#')
			continue;

		/* reset for message */
		AddrSpec = NULL;

		/* display address list */
		if (recursive)
			message("250 %s", addrlist);
		else
			message("050 %s", addrlist);

		/* verify list of addresses */
		list(addrlist);
	}

	(void) fclose(fp);
	FileName = NULL;
}

/*
** LIST -- Verify an address list at appropriate smtp hosts
** --------------------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Shows the status for each transaction done.
*/

void
list(addrlist)
char *addrlist;				/* address list to be verified */
{
	char *address;			/* single address to be verified */
	char *host;			/* remote host to be queried */
	int status;			/* result status */
	extern char *DelimAddr;		/* position of next address in list */

/*
 * Query explicit host if specified.
 * No parsing of the address list will be done in this case.
 */
	if (HostSpec != NULL)
	{
		host = HostSpec;
		status = verify(addrlist, host);
		show(status, host);
		return;
	}

/*
 * Extract individual addresses from address list.
 */
	while (addrlist != NULL)
	{
		/* always process list parsing errors */
		AddrSpec = addrlist;
		SuprErrs = FALSE;

		/* scan next address; abort on syntax error */
		address = parselist(addrlist);
		if (address == NULL)
		{
			status = EX_UNAVAILABLE;
			show(status, (char *)NULL);
			break;
		}

		/* move to the following address */
		addrlist = DelimAddr;

		/* verify single address */
		vrfy(address, (char *)NULL);
	}
}

/*
** VRFY -- Verify a single address at appropriate smtp hosts
** ---------------------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Shows the status for each transaction done.
*/

void
vrfy(address, host)
char *address;				/* address to be verified */
char *host;				/* remote host to be queried */
{
	char addrbuf[BUFSIZ];		/* plain address without comment */
	char hostbuf[BUFSIZ];		/* local copy of domain part */
	char *domain;			/* domain part of address */
	char *mxhosts[MAXMXHOSTS];	/* local copy of mx hosts */
	int nmx;			/* number of mx hosts found */
	int status;			/* result status */
	register int n;

/*
 * Reset count of valid replies received.
 */
	ReplyCount = 0;

/*
 * Check for invalid control characters in address.
 * Always handle errors locally, to avoid fooling sendmail.
 */
	AddrSpec = address;
	SuprErrs = FALSE;

	if (invalidaddr(address))
	{
		status = EX_UNAVAILABLE;
		show(status, (char *)NULL);
		return;
	}

/*
 * Perform sanity check to skip nonsense addresses.
 */
	if (strlength(address) > MAXSPEC)
	{
		status = EX_USAGE;
		show(status, (char *)NULL);
		return;
	}

/*
 * Query local host if the domain could not be parsed properly,
 * but only if parsing errors are not handled locally (default).
 */
	AddrSpec = address;
	SuprErrs = !localerr;

	domain = parsespec(address, addrbuf, hostbuf);
	if (domain == NULL && !SuprErrs)
	{
		status = EX_UNAVAILABLE;
		show(status, (char *)NULL);
		return;
	}
	else if (domain == NULL)
	{
		host = LOCALHOST;
		status = verify(address, host);
		show(status, host);
		return;
	}

/*
 * Verify plain address without comment if requested.
 */
	if (stripit)
		address = addrbuf;

/*
 * Query explicit host if requested.
 */
	if (host != NULL)
	{
		status = verify(address, host);
		show(status, host);
		return;
	}

/*
 * Query local host for local addresses without domain part.
 */
	if (sameword(domain, "localhost"))
	{
		host = LOCALHOST;
		status = verify(address, host);
		show(status, host);
		return;
	}

/*
 * Query address host itself if no mx hosts found.
 */
	nmx = getmxhosts(domain);
	if (nmx < 1)
	{
		host = hostbuf;
		status = verify(address, host);
		show(status, host);
		return;
	}

/*
 * Query all mx hosts found. Use local copy of mx host names.
 * Query primary mx host only, if not verifying all.
 */
	for (n = 0; n < nmx; n++)
		mxhosts[n] = newstr(MxHosts[n]);

	for (n = 0; n < nmx; n++)
	{
		host = mxhosts[n];
		status = verify(address, host);
		show(status, host);
	}

	for (n = 0; n < nmx; n++)
		xfree(mxhosts[n]);
}

/*
** PING -- Ping the mx hosts for a given domain
** --------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Shows the status for each transaction done.
*/

void
ping(domain)
char *domain;				/* remote domain to be pinged */
{
	char *host;			/* remote host to be queried */
	int nmx;			/* number of mx hosts found */
	int status;			/* result status */
	register int n;

/*
 * Ping address host itself if no mx hosts found.
 */
	nmx = getmxhosts(domain);
	if (nmx < 1)
	{
		host = domain;
		status = pinghost(host);
		show(status, host);
		return;
	}

/*
 * Ping all mx hosts found. No need for local copy.
 * Ping primary mx host only, if not verifying all.
 */
	for (n = 0; n < nmx; n++)
	{
		host = MxHosts[n];
		status = pinghost(host);
		show(status, host);
	}
}

/*
** VERIFY -- Verify an address at a given remote smtp host
** -------------------------------------------------------
**
**	Returns:
**		Status code of smtp transaction.
*/

int
verify(address, host)
char *address;				/* address to be verified */
char *host;				/* remote host to be queried */
{
	int status;			/* result status */

/*
 * Kludge, most hosts cannot handle empty address lists. However,
 * if we pass the full address specification, we cannot detect an
 * address list consisting only of comments such as '(comment)'.
 */
	while (is_space(*address) || *address == ',')
		address++;
	if (*address == '\0')
		return(EX_OK);

/*
 * Perform sanity check to skip nonsense addresses.
 */
	if (strlength(address) > MAXSPEC)
		return(EX_USAGE);

/*
 * Carry out the RCPT or EXPN or VRFY protocol as requested.
 */
	if (rcptmode)
		status = rcpthost(address, host);
	else if (expnmode)
		status = expnhost(address, host);
	else
		status = vrfyhost(address, host);
	return(status);
}

/*
** VRFYHOST -- Verify an address at a given remote smtp host
** ---------------------------------------------------------
**
**	Returns:
**		Status code of smtp transaction.
*/

int
vrfyhost(address, host)
char *address;				/* address to be verified */
char *host;				/* remote host to be queried */
{
	register int reply;

/*
 * Show which address we are going to verify at which host.
 */
	if (verbose || debug)
		printf("vrfy '%s' at '%s'\n", address, host);

	if (debug >= 3)
		return(EX_OK);

/*
 * Carry out the smtp protocol suite using VRFY.
 * Note that smtonex returns ok if ONEX is not supported remotely.
 * Note that smtverb returns ok if VERB is not supported remotely.
 * Some hosts require a parameter for the VERB command.
 */
	reply = smtpinit(host);

	if (reply == EX_OK && helomode)
		reply = smtphelo(MyHostName);

	if (reply == EX_OK && onexmode)
		reply = smtponex();

	if (reply == EX_OK && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_OK)
		reply = smtpvrfy(address);

	(void) smtpquit();
	return(reply);
}

/*
** EXPNHOST -- Verify an address at a given remote smtp host
** ---------------------------------------------------------
**
**	Returns:
**		Status code of smtp transaction.
*/

int
expnhost(address, host)
char *address;				/* address to be verified */
char *host;				/* remote host to be queried */
{
	register int reply;

/*
 * Show which address we are going to verify at which host.
 */
	if (verbose || debug)
		printf("expn '%s' at '%s'\n", address, host);

	if (debug >= 3)
		return(EX_OK);

/*
 * Carry out the smtp protocol suite using EXPN.
 * Note that smtonex returns ok if ONEX is not supported remotely.
 * Note that smtverb returns ok if VERB is not supported remotely.
 * Some hosts require a parameter for the VERB command.
 */
	reply = smtpinit(host);

	if (reply == EX_OK && helomode)
		reply = smtphelo(MyHostName);

	if (reply == EX_OK && onexmode)
		reply = smtponex();

	if (reply == EX_OK && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_OK)
		reply = smtpexpn(address);

	(void) smtpquit();
	return(reply);
}

/*
** RCPTHOST -- Verify an address at a given remote smtp host
** ---------------------------------------------------------
**
**	Returns:
**		Status code of smtp transaction.
*/

int
rcpthost(address, host)
char *address;				/* address to be verified */
char *host;				/* remote host to be queried */
{
	register int reply;

/*
 * Show which address we are going to verify at which host.
 */
	if (verbose || debug)
		printf("rcpt '%s' at '%s'\n", address, host);

	if (debug >= 3)
		return(EX_OK);

/*
 * Carry out the smtp protocol suite using RCPT.
 * Note that smtonex returns ok if ONEX is not supported remotely.
 * Note that smtverb returns ok if VERB is not supported remotely.
 * Some hosts require a parameter for the VERB command.
 * Only malconfigured hosts do not accept an empty sender address.
 */
	reply = smtpinit(host);

	if (reply == EX_OK && helomode)
		reply = smtphelo(MyHostName);

	if (reply == EX_OK && onexmode)
		reply = smtponex();

	if (reply == EX_OK && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_OK && mailmode)
		reply = smtpmail("");

	if (reply == EX_OK)
		reply = smtprcpt(address);

	(void) smtpquit();
	return(reply);
}

/*
** PINGHOST -- Ping a given remote host to check smtp connectivity
** ---------------------------------------------------------------
**
**	Returns:
**		Status code of smtp transaction.
*/

int
pinghost(host)
char *host;				/* remote host to be queried */
{
	register int reply;

/*
 * Show which host we are going to ping.
 */
	if (verbose || debug)
		printf("ping '%s'\n", host);

	if (debug >= 3)
		return(EX_OK);

/*
 * Carry out the smtp protocol suite.
 */
	reply = smtpinit(host);

	if (reply == EX_OK && (helomode || verbose >= 3))
		reply = smtphelo(MyHostName);

	(void) smtpquit();
	return(reply);
}

/*
** GETMXHOSTS -- Fetch mx hosts for a given domain
** -----------------------------------------------
**
**	Returns:
**		Number of mx hosts successfully found.
**
**	Outputs:
**		Global table MxHosts[] contains list of host names.
*/

int
getmxhosts(domain)
char *domain;				/* domain to get mx hosts for */
{
	int nmx;			/* number of mx hosts found */
	register char *dot;

/*
 * Catch special pseudo-domains that cannot be mapped.
 */
	dot = rindex(domain, '.');
	if (dot != NULL)
	{
		if (sameword(dot, ".uucp"))
		{
			MxHosts[0] = UUCPRELAY;
			nmx = 1;
			return(nmx);
		}

		if (sameword(dot, ".bitnet") || sameword(dot, ".earn"))
		{
			MxHosts[0] = BITNETRELAY;
			nmx = 1;
			return(nmx);
		}
	}

/*
 * Try to find any mx hosts.
 * This is supposed to catch undomained local hosts.
 * Return only primary mx host if not verifying all.
 */
	nmx = getmxbyname(domain);
	if (nmx > 0)
		return(vrfyall ? nmx : 1);

/*
 * Unresolved single undomained hosts go to the single relay.
 * How these should be interpreted depends on your local strategy:
 * they could default to uucp addresses, bitnet addresses, or just
 * local hosts without an mx record.
 */
	dot = rindex(domain, '.');
	if (dot == NULL)
	{
		MxHosts[0] = SINGLERELAY;
		nmx = 1;
		return(nmx);
	}

/*
 * No mx hosts found, and not a special case.
 */
	return(nmx);
}
