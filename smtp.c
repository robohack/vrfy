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
static char Version[] = "@(#)smtp.c	e07@nikhef.nl (Eric Wassenaar) 940525";
#endif

#include "vrfy.h"

extern int verbose;
extern int debug;

#define REPLYTYPE(r) ((r)/100)	/* first digit of smtp reply code */
#define SMTPCLOSING	421	/* "Service Shutting Down" temp failure */

#define SMTP_CLOSED	0	/* connection is closed */
#define SMTP_OPEN	1	/* connection is open for business */
#define SMTP_SSD	2	/* service shutting down */

char SmtpMsgBuffer[BUFSIZ];	/* buffer for outgoing commands */
char SmtpReplyBuffer[BUFSIZ];	/* buffer for incoming replies */
char SmtpErrorBuffer[BUFSIZ];	/* saved temporary failure error messages */
FILE *SmtpOut = NULL;		/* smtp output channel */
FILE *SmtpIn  = NULL;		/* smtp input channel */
char *SmtpPhase = NULL;		/* connection state message */
char *SmtpPrint = NULL;		/* phase to print and process replies */
int SmtpState = SMTP_CLOSED;	/* current connection state */
int SmtpErrno = 0;		/* saved errno from system calls */

/*
** SMTPINIT -- Initiate SMTP connection with remote host
** -----------------------------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	Outputs:
**		Sets SmtpIn and SmtpOut to input and output channel.
**		Sets SmtpErrno appropriately.
*/

int
smtpinit(host)
char *host;				/* remote host to be contacted */
{
	register int r;

	SmtpIn = SmtpOut = NULL;
	SmtpState = SMTP_CLOSED;
	SmtpErrorBuffer[0] = '\0';
	SmtpErrno = 0;

	SmtpPhase = "user open";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = makeconnection(host, &SmtpOut, &SmtpIn);
	SmtpErrno = errno;
	if (r != EX_OK)
		return(r);

	SmtpState = SMTP_OPEN;

	SmtpPhase = "greeting wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = smtpreply();

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	else if (REPLYTYPE(r) == 2)
		return (EX_OK);

	return(EX_TEMPFAIL);
}

/*
** SMTPHELO -- Issue the HELO command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtphelo(name)
char *name;				/* my own fully qualified hostname */
{
	register int r;

	smtpmessage("HELO %s", name);

	SmtpPhase = "HELO wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = smtpreply();

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	else if (REPLYTYPE(r) == 2)
		return (EX_OK);

	else if (REPLYTYPE(r) == 5)
		return(EX_UNAVAILABLE);

	return(EX_TEMPFAIL);
}

/*
** SMTPONEX -- Issue the ONEX command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	It is not an error if the remote host does not support this.
*/

int
smtponex()
{
	register int r;

	smtpmessage("ONEX");

	SmtpPhase = "ONEX wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = smtpreply();

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	return(EX_OK);
}

/*
** SMTPVERB -- Issue the VERB command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	It is not an error if the remote host does not support this.
**
**	Note. Some systems require an 'on' or 'off' parameter.
**	Systems that do not require a parameter won't object.
*/

int
smtpverb(onoff)
char *onoff;				/* some hosts require parameter */
{
	register int r;

	smtpmessage("VERB %s", onoff);

	SmtpPhase = "VERB wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = smtpreply();

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	return(EX_OK);
}

/*
** SMTPMAIL -- Issue the MAIL command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpmail(address)
char *address;				/* sender address specification */
{
	register int r;

	smtpmessage("MAIL From:<%s>", address);

	SmtpPhase = "MAIL wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = smtpreply();

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	else if (r == 250)
		return (EX_OK);

	else if (r == 552 || r == 554)
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPRCPT -- Issue the RCPT command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	Side effects:
**		Sets SmtpPrint flag to process responses.
*/

int
smtprcpt(address)
char *address;				/* recipient address specification */
{
	register int r;

	smtpmessage("RCPT To:<%s>", address);

	SmtpPhase = "RCPT wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	SmtpPrint = SmtpPhase;
	r = smtpreply();
	SmtpPrint = NULL;

	if (r < 0 || REPLYTYPE(r) == 4)
		return (EX_TEMPFAIL);

	else if (REPLYTYPE(r) == 2)
		return (EX_OK);

	else if (r == 550 || r == 551 || r == 553)
		return (EX_NOUSER);

	else if (r == 552 || r == 554)
		return (EX_UNAVAILABLE);

	return (EX_PROTOCOL);
}

/*
** SMTPEXPN -- Issue the EXPN command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	Side effects:
**		Sets SmtpPrint flag to process responses.
*/

int
smtpexpn(address)
char *address;				/* address to be verified */
{
	register int r;

	smtpmessage("EXPN %s", address);

	SmtpPhase = "EXPN wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	SmtpPrint = SmtpPhase;
	r = smtpreply();
	SmtpPrint = NULL;

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	else if (REPLYTYPE(r) == 2)
		/* address was verified ok */
		return(EX_OK);

	else if (r == 550 || r == 551 || r == 553)
		/* local address but unknown or ambiguous user */
		return(EX_NOUSER);

	else if (r == 552 || r == 554)
		/* address could not be verified */
		return(EX_UNAVAILABLE);

	else if (r == 500 || r == 501 || r == 502 || r == 504)
		/* command not implemented */
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPVRFY -- Issue the VRFY command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	Side effects:
**		Sets SmtpPrint flag to process responses.
*/

int
smtpvrfy(address)
char *address;				/* address to be verified */
{
	register int r;

	smtpmessage("VRFY %s", address);

	SmtpPhase = "VRFY wait";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	SmtpPrint = SmtpPhase;
	r = smtpreply();
	SmtpPrint = NULL;

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	else if (REPLYTYPE(r) == 2)
		/* address was verified ok */
		return(EX_OK);

	else if (r == 550 || r == 551 || r == 553)
		/* local address but unknown or ambiguous user */
		return(EX_NOUSER);

	else if (r == 552 || r == 554)
		/* address could not be verified */
		return(EX_UNAVAILABLE);

	else if (r == 500 || r == 501 || r == 502 || r == 504)
		/* command not implemented */
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPQUIT -- Issue the QUIT command, and reset connection
** --------------------------------------------------------
**
**	Returns:
**		Status code indicating success or failure.
**
**	This routine may be called recursively.
**	Save the previous state across calls.
*/

int
smtpquit()
{
	int SaveErrno = SmtpErrno;	/* save across recursive calls */
	char *SavePhase = SmtpPhase;	/* save across recursive calls */

	if (SmtpIn == NULL && SmtpOut == NULL)
		return(EX_OK);

	if (SmtpState == SMTP_OPEN || SmtpState == SMTP_SSD)
	{
		smtpmessage("QUIT");

		SmtpPhase = "QUIT wait";
		if (debug)
			printf("smtp phase %s\n", SmtpPhase);

		(void) smtpreply();

		if (SmtpState == SMTP_CLOSED)
		{
			SmtpErrno = SaveErrno;
			SmtpPhase = SavePhase;
			return(EX_OK);
		}
	}

	if (SmtpIn != NULL)
		(void) fclose(SmtpIn);
	if (SmtpOut != NULL)
		(void) fclose(SmtpOut);

	SmtpIn = SmtpOut = NULL;
	SmtpState = SMTP_CLOSED;

	SmtpErrno = SaveErrno;
	SmtpPhase = SavePhase;
	return(EX_OK);
}

/*
** SMTPMESSAGE -- Output an SMTP command
** -------------------------------------
**
**	Returns:
**		None.
**
**	Outputs:
**		Saves the command in SmtpMsgBuffer.
**
**	The command is always followed by a CR/LF combination.
*/

void
/*VARARGS1*/
smtpmessage(fmt, a, b, c)
char *fmt;				/* format of message */
char *a, *b, *c;			/* optional arguments */
{
	if (SmtpOut != NULL)
	{
		/* construct the output message */
		(void) sprintf(SmtpMsgBuffer, fmt, a, b, c);

		/* display the output in verbose mode */
		if (verbose >= 2 || debug)
			printf(">>> %s\n", SmtpMsgBuffer);

		/* send the message over the channel */
		(void) fprintf(SmtpOut, "%s\r\n", SmtpMsgBuffer);
	}
}

/*
** SMTPREPLY -- Read an SMTP reply to a command
** --------------------------------------------
**
**	Returns:
**		The SMTP reply code if the reply has been received.
**		-1 on read errors or timeout.
**
**	Outputs:
**		Saves temporary failures in SmtpErrorBuffer.
**		Sets SmtpErrno appropriately.
**
**	Side effects:
**		Calls response() to process response if requested.
*/

int
smtpreply()
{
	register int r;
	register char *p;

	if (SmtpOut != NULL)
		(void) fflush(SmtpOut);

	for (;;)
	{
		/* if we are in the process of closing just give the code */
		if (SmtpState == SMTP_CLOSED || SmtpIn == NULL)
			return(SMTPCLOSING);

		/* get the line from the other side */
		p = sfgets(SmtpReplyBuffer, sizeof(SmtpReplyBuffer), SmtpIn);
		if (p == NULL)
		{
			/* if the remote end closed early, fake an error */
			if (errno == 0)
				errno = ECONNRESET;
			SmtpErrno = errno;

			SmtpState = SMTP_CLOSED;
			(void) smtpquit();
			return(-1);
		}

		/* remove cr/lf combination */
		p = index(SmtpReplyBuffer, '\n');
		if (p != NULL)
		{
			if (p > SmtpReplyBuffer && p[-1] == '\r')
				p--;
			*p = '\0';
		}

		/* display the input in verbose mode */
		if (verbose >= 2 || debug)
			printf("<<< %s\n", SmtpReplyBuffer);

		/* if continuation is required, we can go on */
		if (!is_digit(SmtpReplyBuffer[0]))
			continue;

		/* decode the reply code */
		r = atoi(SmtpReplyBuffer);

		/* extra semantics: 0xx codes are "informational" */
		if (r < 100)
			continue;

		/* process response if requested */
		if (SmtpPrint != NULL && strcmp(SmtpPhase, SmtpPrint) == 0)
			response(SmtpReplyBuffer);

		/* if continuation is required, we can go on */
		if (SmtpReplyBuffer[3] == '-')
			continue;

		/* save temporary failure messages for posterity */
		if (SmtpReplyBuffer[0] == '4' && SmtpErrorBuffer[0] == '\0')
			(void) strcpy(SmtpErrorBuffer, &SmtpReplyBuffer[4]);

		/* reply code 421 is "Service Shutting Down" */
		if (r == SMTPCLOSING && SmtpState != SMTP_SSD)
		{
			/* send the quit protocol */
			SmtpState = SMTP_SSD;
			(void) smtpquit();
		}

		/* valid reply code received */
		SmtpErrno = 0;
		return(r);
	}
	/*NOTREACHED*/
}
