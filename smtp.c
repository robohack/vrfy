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
static char Version[] = "@(#)smtp.c	e07@nikhef.nl (Eric Wassenaar) 980626";
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
char SmtpReplyBuffer[BUFSIZ];	/* buffer for incoming replies (first line) */
char SmtpContBuffer[BUFSIZ];	/* buffer for incoming replies (continuation) */
char SmtpErrorBuffer[BUFSIZ];	/* saved temporary failure error messages */

FILE *SmtpOut = NULL;		/* smtp output channel */
FILE *SmtpIn  = NULL;		/* smtp input channel */

char *SmtpPhase = NULL;		/* connection state message */
int SmtpState = SMTP_CLOSED;	/* current connection state */
int SmtpErrno = 0;		/* saved errno from system calls */

char *SmtpCrLf = "\r\n";	/* smtp end-of-line terminator */

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

	SmtpPhase = "connect";
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

	r = makeconnection(host, &SmtpOut, &SmtpIn);
	SmtpErrno = errno;
	if (r != EX_SUCCESS)
		return(r);

	SmtpState = SMTP_OPEN;

	r = smtpreply("greeting wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		return(EX_SUCCESS);

	if (REPLYTYPE(r) == 5)
		return(EX_UNAVAILABLE);

	return(EX_TEMPFAIL);
}

/*
** SMTPHELO -- Issue the HELO command
** ----------------------------------
**
**	In ESMTP mode, first try the EHLO command.
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtphelo(name, esmtp)
char *name;				/* my own fully qualified hostname */
bool esmtp;				/* try EHLO first, if set */
{
	register int r;

	if (esmtp)
	{
		r = smtpehlo(name);
		if (r != EX_UNAVAILABLE)
			return(r);
	}

	smtpmessage("HELO %s", name);

	r = smtpreply("HELO wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		return(EX_SUCCESS);

	if (REPLYTYPE(r) == 5)
		return(EX_UNAVAILABLE);

	return(EX_TEMPFAIL);
}

/*
** SMTPEHLO -- Issue the EHLO command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpehlo(name)
char *name;				/* my own fully qualified hostname */
{
	register int r;

	smtpmessage("EHLO %s", name);

	r = smtpreply("EHLO wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		return(EX_SUCCESS);

	if (REPLYTYPE(r) == 5)
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

	r = smtpreply("ONEX wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	return(EX_SUCCESS);
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

	r = smtpreply("VERB wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	return(EX_SUCCESS);
}

/*
** SMTPETRN -- Issue the ETRN command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpetrn(name)
char *name;				/* domain name for the ETRN command */
{
	register int r;

	smtpmessage("ETRN %s", name);

	r = smtpreply("ETRN wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		return(EX_SUCCESS);

	if (REPLYTYPE(r) == 5)
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPRSET -- Issue the RSET command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtprset()
{
	register int r;

	smtpmessage("RSET");

	r = smtpreply("RSET wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		return(EX_SUCCESS);

	if (REPLYTYPE(r) == 5)
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
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

	r = smtpreply("MAIL wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (r == 250)
		return(EX_SUCCESS);

	if (r == 552 || r == 554)
		return(EX_UNAVAILABLE);

	if (r == 550 || r == 551 || r == 553)
		return(EX_UNAVAILABLE);

	if (r == 500 || r == 501 || r == 503)
		return(EX_UNAVAILABLE);

	if (r == 521)
		return(EX_UNAVAILABLE);

	if (r == 571)
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPRCPT -- Issue the RCPT command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtprcpt(address)
char *address;				/* recipient address specification */
{
	register int r;

	smtpmessage("RCPT To:<%s>", address);

	r = smtpreply("RCPT wait", TRUE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (r == 250 || r == 251)
		return(EX_SUCCESS);

	if (r == 550 || r == 551 || r == 553)
		return(EX_NOUSER);

	if (r == 552 || r == 554)
		return(EX_UNAVAILABLE);

	if (r == 500 || r == 501 || r == 503)
		return(EX_UNAVAILABLE);

	if (r == 521)
		return(EX_UNAVAILABLE);

	if (r == 571)
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPEXPN -- Issue the EXPN command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpexpn(address)
char *address;				/* address to be verified */
{
	register int r;

	smtpmessage("EXPN %s", address);

	r = smtpreply("EXPN wait", TRUE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		/* address was verified ok */
		return(EX_SUCCESS);

	if (r == 550 || r == 551 || r == 553)
		/* local address but unknown or ambiguous user */
		return(EX_NOUSER);

	if (r == 552 || r == 554)
		/* address could not be verified */
		return(EX_UNAVAILABLE);

	if (r == 500 || r == 501 || r == 502 || r == 504)
		/* command not implemented */
		return(EX_UNAVAILABLE);

	if (r == 521)
		/* not a real mail server */
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPVRFY -- Issue the VRFY command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpvrfy(address)
char *address;				/* address to be verified */
{
	register int r;

	smtpmessage("VRFY %s", address);

	r = smtpreply("VRFY wait", TRUE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (REPLYTYPE(r) == 2)
		/* address was verified ok */
		return(EX_SUCCESS);

	if (r == 550 || r == 551 || r == 553)
		/* local address but unknown or ambiguous user */
		return(EX_NOUSER);

	if (r == 552 || r == 554)
		/* address could not be verified */
		return(EX_UNAVAILABLE);

	if (r == 500 || r == 501 || r == 502 || r == 504)
		/* command not implemented */
		return(EX_UNAVAILABLE);

	if (r == 521)
		/* not a real mail server */
		return(EX_UNAVAILABLE);

	return(EX_PROTOCOL);
}

/*
** SMTPDATA -- Issue the DATA command
** ----------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpdata()
{
	register int r;

/*
 * Issue the DATA command, and wait for the go-ahead.
 */
	smtpmessage("DATA");

	r = smtpreply("DATA wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (r == 552 || r == 554)
		return(EX_UNAVAILABLE);

	if (r == 500 || r == 501 || r == 503)
		return(EX_UNAVAILABLE);

	if (r == 521)
		return(EX_UNAVAILABLE);

	if (r != 354)
		return(EX_PROTOCOL);

/*
 * Transmit the message body. This fails only on I/O errors.
 */
	if (smtpbody() != EX_SUCCESS)
		return(EX_TEMPFAIL);

/*
 * Terminate the message, and wait for acceptance.
 */
	smtpmessage(".");

	r = smtpreply("accept wait", FALSE);

	if (r < 0 || REPLYTYPE(r) == 4)
		return(EX_TEMPFAIL);

	if (r == 552 || r == 554)
		return(EX_UNAVAILABLE);

	if (r == 500 || r == 501 || r == 503)
		return(EX_UNAVAILABLE);

	if (r == 521)
		return(EX_UNAVAILABLE);

	if (r != 250)
		return(EX_PROTOCOL);

	return(EX_SUCCESS);
}

/*
** SMTPBODY -- Transmit the message itself
** ---------------------------------------
**
**	Returns:
**		Status code indicating success or failure.
*/

int
smtpbody()
{
	return(EX_SUCCESS);
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
		return(EX_SUCCESS);

	if (SmtpState == SMTP_OPEN || SmtpState == SMTP_SSD)
	{
		smtpmessage("QUIT");

		(void) smtpreply("QUIT wait", FALSE);

		if (SmtpState == SMTP_CLOSED)
		{
			SmtpErrno = SaveErrno;
			SmtpPhase = SavePhase;
			return(EX_SUCCESS);
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
	return(EX_SUCCESS);
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
smtpmessage(fmt, a, b, c, d)
char *fmt;				/* format of message */
char *a, *b, *c, *d;			/* optional arguments */
{
	if (SmtpOut != NULL)
	{
		/* construct the output message */
		(void) sprintf(SmtpMsgBuffer, fmt, a, b, c, d);

		/* display the output in verbose mode */
		if (verbose >= 2 || debug)
			printf(">>> %s\n", SmtpMsgBuffer);

		/* send the message over the channel */
		(void) fprintf(SmtpOut, "%s%s", SmtpMsgBuffer, SmtpCrLf);
	}
}

/*
** SMTPREPLY -- Read an SMTP reply to a command
** --------------------------------------------
**
**	Returns:
**		The SMTP reply code if the reply has been received.
**		-1 on I/O errors or timeout.
**
**	Outputs:
**		Saves temporary failures in SmtpErrorBuffer.
**		Sets SmtpErrno appropriately.
**
**	Side effects:
**		Calls response() to process response if requested.
*/

int
smtpreply(phase, check)
char *phase;				/* new connection state message */
bool check;				/* process response, if set */
{
	register int r;
	register char *p;
	char *buf;			/* current smtp reply buffer */

/*
 * Define the new SMTP connection state message.
 */
	SmtpPhase = phase;
	if (debug)
		printf("smtp phase %s\n", SmtpPhase);

/*
 * Force the previous SMTP message to be written out.
 * Make sure the connection is still open, and check for errors.
 */
	if (SmtpOut != NULL)
	{
		if (fflush(SmtpOut) || ferror(SmtpOut))
		{
			if (errno == 0)
				errno = EIO;
			SmtpErrno = errno;

			SmtpState = SMTP_CLOSED;
			(void) smtpquit();
			return(-1);
		}
	}

/*
 * Read the response to the SMTP message.
 * Only the first line is saved in the main reply buffer.
 */
	for (buf = SmtpReplyBuffer;; buf = SmtpContBuffer)
	{
		/* if we are in the process of closing just give the code */
		if (SmtpState == SMTP_CLOSED || SmtpIn == NULL)
		{
			/* make sure we have a meaningful error message */
			if (SmtpErrorBuffer[0] == '\0')
				(void) strcpy(SmtpErrorBuffer, "Connection closed");

			/* return a valid reply code */
			SmtpErrno = 0;
			return(SMTPCLOSING);
		}

		/* get the line from the other side */
		p = sfgets(buf, BUFSIZ, SmtpIn);
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
		fixcrlf(buf, TRUE);

		/* display the input in verbose mode */
		if (verbose >= 2 || debug)
			printf("<<< %s\n", buf);

		/* if continuation is required, we can go on */
		if (!is_digit(buf[0]))
			continue;

		/* decode the reply code */
		r = atoi(buf);

		/* extra semantics: 0xx codes are "informational" */
		if (r < 100)
			continue;

		/* process response if requested */
		if (check)
			response(buf);

		/* if continuation is required, we can go on */
		if (buf[3] == '-')
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
