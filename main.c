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

#ifndef lint
static char Version[] = "@(#)vrfy.c	e07@nikhef.nl (Eric Wassenaar) 980820";
#endif

#include "vrfy.h"

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
File:  vrfy [options] [-v] -f [file] [host]\n\
Ping:  vrfy [options] [-v] -p domain\n\
Etrn:  vrfy [options] [-v] -T domain [name]\n\
Options: [-a] [-d] [-l] [-s] [-c secs] [-t secs]\n\
Special: [-L level] [-R] [-S sender] [-n] [-e] [-h] [-H]\
";


char **optargv = NULL;		/* argument list including default options */
int optargc = 0;		/* number of arguments in new argument list */

char *FromAddr = NULL;		/* -S  explicit envelope sender address */
char *HostSpec = NULL;		/* explicit host to be queried */
char *AddrSpec = NULL;		/* address being processed */
char *FileName = NULL;		/* name of file being processed */
int LineNumber = 0;		/* line number into file */
int ExitStat = EX_SUCCESS;	/* overall result status */
bool SuprErrs = FALSE;		/* suppress parsing errors, if set */

int debug = 0;			/* -d  debugging level */
int verbose = 0;		/* -v  verbosity level */
int recursive = 0;		/* -L  recursive mode maximum level */

bool stripit = FALSE;		/* -s  strip comments, if set */
bool vrfyall = FALSE;		/* -a  query all mx hosts found, if set */
bool localerr = FALSE;		/* -l  handle errors locally, if set */
bool etrnmode = FALSE;		/* -T  etrn mx hosts, if set */
bool pingmode = FALSE;		/* -p  ping mx hosts, if set */
bool filemode = FALSE;		/* -f  verify file or stdin, if set */
bool helomode = FALSE;		/* -h  issue HELO command, if set */
bool ehlomode = FALSE;		/* -H  issue EHLO/HELO command, if set */
bool onexmode = FALSE;		/* -o  issue ONEX command, if set */
bool expnmode = FALSE;		/* -e  use EXPN instead of VRFY, if set */
bool rcptmode = FALSE;		/* -r  use RCPT instead of VRFY, if set */
bool datamode = FALSE;		/* -M  add DATA after MAIL/RCPT, if set */

char *ReplyList[MAXREPLY];	/* saved address expansions */
int ReplyCount = 0;		/* number of valid replies */

char *AddrChain[MAXLOOP];	/* address chain in recursive mode */

char *localhost = LOCALHOST;		/* nearest sendmail daemon */
char *uucprelay = UUCPRELAY;		/* uucp relay host */
char *bitnetrelay = BITNETRELAY;	/* bitnet relay host */
char *singlerelay = SINGLERELAY;	/* unqualified host relay */

extern char *MxHosts[MAXMXHOSTS];	/* names of mx hosts found */

extern int ConnTimeout;		/* -c  timeout in secs for connect() */
extern int ReadTimeout;		/* -t  timeout in secs for sfgets() */

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

	assert(sizeof(u_int) >= 4);	/* probably paranoid */
#ifdef obsolete
	assert(sizeof(u_short) == 2);	/* perhaps less paranoid */
	assert(sizeof(ipaddr_t) == 4);	/* but this is critical */
#endif

/*
 * Synchronize stdout and stderr in case output is redirected.
 */
	linebufmode(stdout);

/*
 * Initialize resolver. Set new defaults.
 * The old defaults are (RES_RECURSE | RES_DEFNAMES | RES_DNSRCH)
 */
	(void) res_init();

	_res.options |=  RES_DEFNAMES;	/* qualify single names */
	_res.options &= ~RES_DNSRCH;	/* dotted names are qualified */

/*
 * Overrule compiled-in defaults.
 */
	option = getenv("VRFY_LOCALHOST");
	if (option != NULL)
		localhost = maxstr(newstr(option), MAXHOST, FALSE);

	option = getenv("VRFY_UUCPRELAY");
	if (option != NULL)
		uucprelay = maxstr(newstr(option), MAXHOST, FALSE);

	option = getenv("VRFY_BITNETRELAY");
	if (option != NULL)
		bitnetrelay = maxstr(newstr(option), MAXHOST, FALSE);

	option = getenv("VRFY_SINGLERELAY");
	if (option != NULL)
		singlerelay = maxstr(newstr(option), MAXHOST, FALSE);

/*
 * Interpolate default options and parameters.
 */
	if (argc < 1 || argv[0] == NULL)
		fatal(Usage);

	option = getenv("VRFY_DEFAULTS");
	if (option != NULL)
	{
		set_defaults(option, argc, argv);
		argc = optargc; argv = optargv;
	}

/*
 * Fetch command line options.
 */
	while (argc > 1 && argv[1] != NULL && argv[1][0] == '-')
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

	    	    case 'H':		/* issue EHLO/HELO command */
			ehlomode = TRUE;
	    		/*FALLTHROUGH*/

	    	    case 'h':		/* issue HELO command */
			helomode = TRUE;
	    		break;

	    	    case 'o':		/* issue ONEX command */
			onexmode = TRUE;
	    		break;

	    	    case 'e':		/* use EXPN instead of VRFY */
			expnmode = TRUE;
	    		break;

	    	    case 'S':		/* explicit envelope sender address */
			if (argv[2] == NULL || argv[2][0] == '-')
				fatal("Missing sender address");
			FromAddr = setsender(argv[2]);
			if (FromAddr == NULL)
				fatal("Invalid sender address");
			argc--, argv++;
	    		/*FALLTHROUGH*/

	    	    case 'n':		/* use alternative protocol suite */
			ehlomode = TRUE;
			helomode = TRUE;
	    		/*FALLTHROUGH*/

	    	    case 'r':		/* use MAIL/RCPT instead of VRFY */
			rcptmode = TRUE;
			recursive = 0;
	    		break;

	    	    case 'M':		/* add DATA after MAIL/RCPT */
			datamode = TRUE;
	    		break;

	    	    case 'f':		/* verify file */
	    		filemode = TRUE;
			if (etrnmode)
				fatal("-f conflicts with -T");
			if (pingmode)
				fatal("-f conflicts with -p");
	    		break;

	    	    case 'p':		/* ping mx hosts */
	    		pingmode = TRUE;
			if (etrnmode)
				fatal("-p conflicts with -T");
			if (filemode)
				fatal("-p conflicts with -f");
	    		break;

	    	    case 'T':		/* etrn mx hosts */
	    		etrnmode = TRUE;
			if (pingmode)
				fatal("-T conflicts with -p");
			if (filemode)
				fatal("-T conflicts with -f");
	    		break;

		    case 'c':		/* set connect timeout */
			ConnTimeout = getval(argv[2], "timeout value", 1, 0);
			--argc; argv++;
			break;

		    case 't':		/* set read timeout */
			ReadTimeout = getval(argv[2], "timeout value", 1, 0);
			--argc; argv++;
			break;

		    case 'L' :		/* set recursion level */
			recursive = getval(argv[2], "recursion level", 1, MAXLOOP);
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
			exit(EX_SUCCESS);

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
	if (!filemode && (argc < 2 || argv[1] == NULL))
		fatal(Usage);

	/* set optional explicit host to be queried */
	if (argc > 2 && argv[2] != NULL)
		HostSpec = maxstr(argv[2], MAXHOST, TRUE);

	/* rest is undefined */
	if (argc > 3)
		fatal(Usage);

/*
 * Miscellaneous initialization.
 */
	/* reset control variables that may have been used */
	AddrSpec = NULL;
	SuprErrs = FALSE;

	/* get own host name before turning on debugging */
	if (helomode || etrnmode || pingmode)
		setmyhostname();

	/* set proper parameter for etrn mode */
	if (etrnmode)
		option = (HostSpec != NULL) ? HostSpec : MyHostName;

/*
 * Set proper resolver options.
 */
	/* set nameserver debugging on, if requested */
	if (debug == 2)
		_res.options |= RES_DEBUG;

/*
 * All set. Execute the required function.
 */
	if (etrnmode) /* etrn the given domain */
		etrn(argv[1], option);

	else if (pingmode) /* ping the given domain */
		ping(argv[1]);

	else if (filemode) /* verify the given file */
		file(argv[1]);

	else /* verify the address list */
		list(argv[1]);

	return(ExitStat);
	/*NOTREACHED*/
}

/*
** SET_DEFAULTS -- Interpolate default options and parameters in argv
** ------------------------------------------------------------------
**
**	The VRFY_DEFAULTS env variable gives customized options.
**
**	Returns:
**		None.
**
**	Outputs:
**		Creates ``optargv'' vector with ``optargc'' arguments.
*/

void
set_defaults(option, argc, argv)
char *option;				/* option string */
int argc;				/* original command line arg count */
char *argv[];				/* original command line arguments */
{
	register char *p, *q;
	register int i;

/*
 * Allocate new argument vector.
 */
	optargv = newlist(NULL, 2, char *);
	optargv[0] = argv[0];
	optargc = 1;

/*
 * Construct argument list from option string.
 */
	for (q = newstr(option), p = q; *p != '\0'; p = q)
	{
		while (is_space(*p))
			p++;

		if (*p == '\0')
			break;

		for (q = p; *q != '\0' && !is_space(*q); q++)
			continue;

		if (*q != '\0')
			*q++ = '\0';

		optargv = newlist(optargv, optargc+2, char *);
		optargv[optargc] = p;
		optargc++;
	}

/*
 * Append command line arguments.
 */
	for (i = 1; i < argc && argv[i] != NULL; i++)
	{
		optargv = newlist(optargv, optargc+2, char *);
		optargv[optargc] = argv[i];
		optargc++;
	}

	/* and terminate */
	optargv[optargc] = NULL;
}

/*
** GETVAL -- Decode parameter value and perform range check
** --------------------------------------------------------
**
**	Returns:
**		Parameter value if successfully decoded.
**		Aborts in case of syntax or range errors.
*/

int
getval(optstring, optname, minvalue, maxvalue)
char *optstring;			/* parameter from command line */
char *optname;				/* descriptive name of option */
int minvalue;				/* minimum value for option */
int maxvalue;				/* maximum value for option */
{
	register int optvalue;

	if (optstring == NULL || optstring[0] == '-')
		fatal("Missing %s", optname);

	optvalue = atoi(optstring);

	if (optvalue == 0 && optstring[0] != '0')
		fatal("Invalid %s %s", optname, optstring);

	if (optvalue < minvalue)
		fatal("Minimum %s %s", optname, itoa(minvalue));

	if (maxvalue > 0 && optvalue > maxvalue)
		fatal("Maximum %s %s", optname, itoa(maxvalue));

	return(optvalue);
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
fatal(fmt, a, b, c, d)
char *fmt;				/* format of message */
char *a, *b, *c, *d;			/* optional arguments */
{
	(void) fprintf(stderr, fmt, a, b, c, d);
	(void) fprintf(stderr, "\n");
	exit(EX_USAGE);
}


/*
** ERROR -- Issue error message to error output
** --------------------------------------------
**
**	Returns:
**		None.
*/

void /*VARARGS1*/
error(fmt, a, b, c, d)
char *fmt;				/* format of message */
char *a, *b, *c, *d;			/* optional arguments */
{
	(void) fprintf(stderr, fmt, a, b, c, d);
	(void) fprintf(stderr, "\n");
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
usrerr(fmt, a, b, c, d)
char *fmt;				/* format of message */
char *a, *b, *c, *d;			/* optional arguments */
{
	char msg[BUFSIZ];		/* status message buffer */

	/* suppress message if requested */
	if (SuprErrs)
		return;

	/* issue message with fatal error status */
	(void) sprintf(msg, "554 %s", fmt);
	message(msg, a, b, c, d);
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
message(msg, a, b, c, d)
char *msg;				/* status message */
char *a, *b, *c, *d;			/* optional arguments */
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
	printf(fmt, a, b, c, d);
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
**	vrfy wait phase. Status code 2xx indicates a message with a
**	valid address expansion. More than one such line may arrive.
**
**	Strictly speaking only 250 and 251 are valid for VRFY, EXPN,
**	and RCPT. Code 252 is added per RFC 1123 to reject VRFY.
**
**	Note that we must have an AddrSpec in order to do recursion.
*/

void
response(msg)
char *msg;				/* status message from reply */
{
	char *address = &msg[4];	/* address expansion in reply */

	/* print the text of the reply */
	printf("%s\n", address);

	/* skip if this is not a standard reply line */
	if (!is_digit(msg[0]) || (msg[3] != ' ' && msg[3] != '-'))
		return;

	/* skip if this is not a valid address expansion */
	if (msg[0] != '2')
		return;

	/* only allow 250 and 251 -- explicitly skip 252 */
	if (msg[1] != '5' || (msg[2] != '0' && msg[2] != '1'))
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
**	Note that we must have an AddrSpec in order to do recursion.
*/

#define tempfail(x) (x == EX_TEMPFAIL || x == EX_OSERR || x == EX_IOERR)

void
show(status, host)
int status;				/* result status of operation */
char *host;				/* remote host that was queried */
{
	/* save result status, keeping previous failures */
	if (status != EX_SUCCESS && !tempfail(ExitStat))
		ExitStat = status;

	/* this must be an internal programming error */
	if (status == EX_SUCCESS && host == NULL)
		status = EX_SOFTWARE;

	/* display the appropriate error message */
	if (status != EX_SUCCESS)
		giveresponse(status);

	/* special message in etrn mode */
	else if (etrnmode)
		printf("%s responded\n", host);

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
	char oldaddr[MAXSPEC+1];	/* parsed original address */
	char newaddr[MAXSPEC+1];	/* parsed reply to address */
	char hostbuf[MAXHOST+1];	/* local copy of domain part */
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
	char *SaveFile;
	int SaveLine;
	register char *p;

/*
 * Save state across recursive calls.
 */
	SaveFile = FileName;
	SaveLine = LineNumber;

/*
 * Allow the use of vrfy -f as a filter.
 */
	if (filename == NULL || *filename == '\0')
		filename = "stdin";
	if (sameword(filename, "stdin"))
		fp = stdin;
	else
		fp = fopen(filename, "r");

/*
 * Terminate if the file could not be opened.
 */
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

	if (fp != stdin)
		(void) fclose(fp);

/*
 * Restore state.
 */
	FileName = SaveFile;
	LineNumber = SaveLine;
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
**
**	If an explicit HostSpec was given, we don't set AddrSpec since
**	it may be an entire list. This effectively disables recursion.
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
 * This requires some extra sanity checks to be done in verify.
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
**
**	This routine is not used if an explicit HostSpec was given.
*/

void
vrfy(address, host)
char *address;				/* address to be verified */
char *host;				/* remote host to be queried */
{
	char addrbuf[MAXSPEC+1];	/* plain address without comment */
	char hostbuf[MAXHOST+1];	/* local copy of domain part */
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
 * Perform also sanity length check to skip nonsense addresses.
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
 * Query local host if the domain could not be parsed properly,
 * but only if parsing errors are not handled locally (default).
 */
	AddrSpec = address;
	SuprErrs = !localerr;

	domain = parsespec(address, addrbuf, hostbuf);
	if (domain == NULL)
	{
		if (!SuprErrs)
		{
			status = EX_UNAVAILABLE;
			show(status, (char *)NULL);
			return;
		}

		host = localhost;
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
 * Handle special /filename, "|program", and :include: syntax locally.
 */
	if (sameword(domain, "localhost"))
	{
		if (file_addr(addrbuf) || prog_addr(addrbuf))
		{
			printf("%s\n", addrbuf);
			return;
		}

		if (incl_addr(addrbuf))
		{
			file(&addrbuf[9]);
			return;
		}

		host = localhost;
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
 * Query primary mx host only, if not doing all.
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
** ETRN -- Etrn the mx hosts for a given domain
** --------------------------------------------
**
**	Returns:
**		None.
**
**	Side effects:
**		Shows the status for each transaction done.
*/

void
etrn(domain, name)
char *domain;				/* remote domain to be etrned */
char *name;				/* domain name for the ETRN command */
{
	char *host;			/* remote host to be queried */
	int nmx;			/* number of mx hosts found */
	int status;			/* result status */
	register int n;

/*
 * Validate domain syntax.
 */
	AddrSpec = domain;
	SuprErrs = FALSE;

	if (invalidhost(domain))
	{
		status = EX_UNAVAILABLE;
		show(status, (char *)NULL);
		return;
	}

/*
 * Etrn address host itself if no mx hosts found.
 */
	nmx = getmxhosts(domain);
	if (nmx < 1)
	{
		host = domain;
		status = etrnhost(name, host);
		show(status, host);
		return;
	}

/*
 * Etrn all mx hosts found. No need for local copy.
 * Etrn primary mx host only, if not doing all.
 */
	for (n = 0; n < nmx; n++)
	{
		host = MxHosts[n];
		status = etrnhost(name, host);
		show(status, host);
	}
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
 * Validate domain syntax.
 */
	AddrSpec = domain;
	SuprErrs = FALSE;

	if (invalidhost(domain))
	{
		status = EX_UNAVAILABLE;
		show(status, (char *)NULL);
		return;
	}

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
 * Ping primary mx host only, if not doing all.
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
**
**	The address may be an entire address list in case an explicit
**	HostSpec was given. Parsing of the list is skipped in that case.
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
		return(EX_SUCCESS);

/*
 * Perform extra sanity check to skip nonsense addresses.
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
		return(EX_SUCCESS);

/*
 * Carry out the smtp protocol suite using VRFY.
 * Note that smtponex returns ok if ONEX is not supported remotely.
 * Note that smtpverb returns ok if VERB is not supported remotely.
 * Some hosts require a parameter for the VERB command.
 */
	reply = smtpinit(host);

	if (reply == EX_SUCCESS && helomode)
		reply = smtphelo(MyHostName, ehlomode);

	if (reply == EX_SUCCESS && onexmode)
		reply = smtponex();

	if (reply == EX_SUCCESS && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_SUCCESS)
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
		return(EX_SUCCESS);

/*
 * Carry out the smtp protocol suite using EXPN.
 * Note that smtponex returns ok if ONEX is not supported remotely.
 * Note that smtpverb returns ok if VERB is not supported remotely.
 * Some hosts require a parameter for the VERB command.
 */
	reply = smtpinit(host);

	if (reply == EX_SUCCESS && helomode)
		reply = smtphelo(MyHostName, ehlomode);

	if (reply == EX_SUCCESS && onexmode)
		reply = smtponex();

	if (reply == EX_SUCCESS && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_SUCCESS)
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
		return(EX_SUCCESS);

/*
 * Carry out the smtp protocol suite using MAIL/RCPT.
 * Only malconfigured hosts do not accept an empty sender address.
 */
	reply = smtpinit(host);

	if (reply == EX_SUCCESS && helomode)
		reply = smtphelo(MyHostName, ehlomode);

	if (reply == EX_SUCCESS && onexmode)
		reply = smtponex();

	if (reply == EX_SUCCESS && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_SUCCESS)
		reply = smtpmail((FromAddr == NULL) ? "" : FromAddr);

	if (reply == EX_SUCCESS)
		reply = smtprcpt(address);

	if (reply == EX_SUCCESS && datamode)
		(void) smtpdata();
	else
		(void) smtprset();

	(void) smtpquit();
	return(reply);
}

/*
** ETRNHOST -- Issue an ETRN command at a given remote smtp host
** -------------------------------------------------------------
**
**	Returns:
**		Status code of smtp transaction.
*/

int
etrnhost(name, host)
char *name;				/* domain name for the ETRN command */
char *host;				/* remote host to be queried */
{
	register int reply;

/*
 * Show which name we are going to etrn at which host.
 */
	if (verbose || debug)
		printf("etrn '%s' at '%s'\n", name, host);
	if (debug >= 3)
		return(EX_SUCCESS);

/*
 * Carry out the smtp protocol suite.
 */
	reply = smtpinit(host);

	if (reply == EX_SUCCESS && helomode)
		reply = smtphelo(MyHostName, ehlomode);

	if (reply == EX_SUCCESS && verbose >= 3)
		reply = smtpverb("on");

	if (reply == EX_SUCCESS)
		reply = smtpetrn(name);

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
		return(EX_SUCCESS);

/*
 * Carry out the smtp protocol suite.
 */
	reply = smtpinit(host);

	if (reply == EX_SUCCESS && (helomode || verbose >= 3))
		reply = smtphelo(MyHostName, ehlomode);

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
			MxHosts[0] = uucprelay;
			nmx = 1;
			return(nmx);
		}

		if (sameword(dot, ".bitnet") || sameword(dot, ".earn"))
		{
			MxHosts[0] = bitnetrelay;
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
		MxHosts[0] = singlerelay;
		nmx = 1;
		return(nmx);
	}

/*
 * No mx hosts found, and not a special case.
 */
	return(nmx);
}

/*
** SETSENDER -- Define an explicit envelope sender address
** -------------------------------------------------------
**
**	Returns:
**		The parsed plain address.
**		NULL in case of elementary syntax errors.
*/

char *
setsender(address)
char *address;				/* potential envelope sender */
{
	static char addrbuf[MAXSPEC+1];	/* parsed plain address */
	char *domain;			/* domain part of address */

	/* always process address parsing errors */
	AddrSpec = address;
	SuprErrs = FALSE;

	/* make sure it is a single address */
	address = parselist(address);
	if (address == NULL)
		return(NULL);

	/* skip if address cannot be parsed */
	domain = parsespec(address, addrbuf, (char *)NULL);
	if (domain == NULL)
		return(NULL);

	return(addrbuf);
}
