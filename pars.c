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
static char Version[] = "@(#)pars.c	e07@nikhef.nl (Eric Wassenaar) 961013";
#endif

#include "vrfy.h"

/*
** PARSELIST -- Isolate a single address in an address list
** --------------------------------------------------------
**
**	Returns:
**		Beginning of first address in the list.
**		NULL in case of elementary syntax errors.
**
**	Side effects:
**		The address is terminated with a null byte,
**		clobbering the original address list.
**		An error message is generated if invalid.
**
**	DelimAddr will point to the beginning of the next address
**	in the list, or is set to NULL if this was the last address.
*/

char *DelimAddr = NULL;		/* position of next address in list */

char *
parselist(addrspec)
char *addrspec;
{
	char delimiter;			/* delimiter between addresses */
	register char *p, *q;

/*
 * Do a rudimentary attempt to recognize list (group) syntax.
 * Not many SMTP servers are capable to handle this properly.
 * The general syntax is "comment: address1, address2;"
 * An empty address list may be specified as "comment:;"
 */
	p = rindex(addrspec, ';');
	if (p != NULL && p[1] == '\0')
	{
		q = find_delim(addrspec, ':');
		if (q != NULL && *q == ':')
		{
			*p = '\0';
			addrspec = q+1;
		}
	}

/*
 * Always search for comma as address delimiter.
 * Old style address lists with space delimiters are not applicable here.
 */
#ifdef notdef
	delimiter = ' ';
	if (index(addrspec, ',') != NULL || index(addrspec, '<') != NULL ||
	    index(addrspec, '(') != NULL || index(addrspec, ';') != NULL)
#endif
		delimiter = ',';

/*
 * Move to the beginning of the first address in the list.
 */
	while (is_space(*addrspec) || *addrspec == ',')
		addrspec++;

/*
 * Scan address list for delimiter. Abort on syntax errors.
 */
	DelimAddr = find_delim(addrspec, delimiter);
	if (DelimAddr == NULL)
		return(NULL);

/*
 * Move to the beginning of the next following address.
 */
	while (is_space(*DelimAddr) || *DelimAddr == ',')
		*DelimAddr++ = '\0';

/*
 * Return the first address and the position of the next address.
 */
	if (*DelimAddr == '\0')
		DelimAddr = NULL;

	return(addrspec);
}

/*
** PARSESPEC --  Extract and validate plain address and domain part
** ----------------------------------------------------------------
**
**	Returns:
**		Pointer to the domain part, if present, or
**		"localhost" in case there is no domain part.
**		NULL in case of elementary syntax errors.
**		NULL in case of probably invalid domain.
**
**	Side effects:
**		An error message is generated if invalid.
**
**	Outputs:
**		A copy of the parsed components is saved, if requested.
**		The parsed components must fit into the provided storage.
*/

char *
parsespec(addrspec, copya, copyd)
char *addrspec;				/* full address specification */
char *copya;				/* buffer to store plain address */
char *copyd;				/* buffer to store domain part */
{
	char *address;			/* plain address without comment */
	char *domain;			/* domain part of address */

/*
 * Make some elementary syntax checks.
 * Abort if this is an invalid address.
 */
	if (invalidaddr(addrspec))
		return(NULL);

/*
 * Extract plain address from full address specification.
 * Abort if address could not be parsed properly.
 */
	address = parseaddr(addrspec);
	if (address == NULL)
		return(NULL);

	/* save plain address if requested */
	if (copya != NULL)
		(void) strcpy(copya, address);

/*
 * Extract the domain part from the plain address.
 * Abort if domain could not be parsed properly.
 */
	domain = parsehost(address);
	if (domain == NULL)
		return(NULL);

/*
 * Validate the domain part. Make some basic checks.
 * Abort if this is an invalid domain name.
 */
	if (invalidhost(domain))
		return(NULL);

	/* save domain part if requested */
	if (copyd != NULL)
		(void) strcpy(copyd, domain);

/*
 * Looks like a valid address specification.
 */
	return(domain);
}

/*
** PARSEADDR -- Construct a plain address without comments
** -------------------------------------------------------
**
**	Returns:
**		The plain address as saved in static storage.
**		NULL in case of elementary syntax errors.
**
**	Side Effects:
**		An error message is generated if invalid.
**
**	We assume the parsed address will fit into the local storage.
**
**	Comments outside brackets, and the brackets, are eliminated.
**	Comments within parentheses, and the parens, are eliminated.
**	The remaining address parts are concatenated without blanks.
**
**	Note for the insiders: this is not 100% conforming to sendmail
**	since we disregard the type of individual tokens, and do not
**	insert SpaceSub characters between atoms. We don't care here.
*/

char *
parseaddr(addrspec)
char *addrspec;				/* full address specification */
{
	register char *address;		/* plain address without comment */
	static char buf[MAXSPEC+1];	/* saved plain address */
	register char *p, *q;

/*
 * Check if we have anything in angle brackets. If so, reprocess
 * the part between the brackets. Abort in case of syntax errors.
 */
	p = find_delim(addrspec, '<');
	if (p != NULL && *p == '<')
	{
		q = find_delim(p+1, '>');
		if (q != NULL && *q == '>')
		{
			*q = '\0';
			address = parseaddr(p+1);
			*q = '>';
			return(address);
		}
	}

	if (p == NULL || *p != '\0')
		return(NULL);

/*
 * Strip out comments between parentheses.
 * Concatenate the real address parts. Abort on syntax errors.
 */
	address = buf;
rescan:
	p = find_delim(addrspec, '(');
	if (p != NULL && *p == '(')
	{
		q = find_delim(p+1, ')');
		if (q != NULL && *q == ')')
		{
			*p = '\0';
			address = cataddr(buf, address, addrspec);
			*p = '(';
			addrspec = q+1;
			goto rescan;
		}
	}

	if (p == NULL || *p != '\0')
		return(NULL);

	address = cataddr(buf, address, addrspec);
	*address = '\0';
	return(buf);
}

/*
** PARSEHOST --  Extract the domain part from a plain address
** ----------------------------------------------------------
**
**	Returns:
**		Pointer to the domain part, if present, or
**		"localhost" in case there is no domain part.
**		NULL in case of elementary syntax errors.
**
**	Side effects:
**		Domain part may be terminated with a null
**		byte, thereby clobbering the address.
**		An error message is generated if invalid.
**
**	In addresses with both '!' and '@' precedence is
**	given to the '@' part: 'foo!user@bar' goes to bar.
**	Source routes have the highest priority.
*/

char *
parsehost(address)
char *address;				/* plain address without comment */
{
	register char *delim;

/*
 * RFC822 source route.
 * Note that it should have been specified between brackets.
 */
	if (*address == '@')
	{
		delim = find_delim(address, ',');
		if (delim == NULL || *delim == '\0')
			delim = find_delim(address, ':');
		if (delim == NULL || *delim == '\0')
		{
			usrerr("Invalid source route");
			return(NULL);
		}
		*delim = '\0';
		return(address+1);
	}

/*
 * Ordinary RFC822 internet address.
 * Note that we scan for the first '@' and not for the last.
 */
	delim = find_delim(address, '@');
	if (delim != NULL && *delim != '\0')
		return(delim+1);

/*
 * Old fashioned uucp path.
 */
	delim = find_delim(address, '!');
	if (delim != NULL && *delim != '\0')
	{
		*delim = '\0';
		return(address);
	}

/*
 * Everything else is local.
 */
	return("localhost");
}

/*
** FIND_DELIM -- Find the position of a delimiter in an address
** ------------------------------------------------------------
**
**	Returns:
**		Position of delimiter in address if found.
**		Position of null byte after address if not found.
**		NULL in case of elementary syntax errors.
**
**	Side Effects:
**		An error message is generated if invalid.
**
**	Used by parselist() to locate address delimiters in address lists.
**	Used by parseaddr() to locate brackets and parens in addresses.
**	Used by parsehost() to locate domain separators in domain names.
**
**	It will search for an unquoted delimiter outside comments
**	in complicated full address specifications like
**	"comment \"comment\" comment" <address (comment \comment)>
*/

char *
find_delim(addrspec, delimiter)
char *addrspec;				/* full address specification */
char delimiter;				/* delimiter char to search for */
{
	bool backslash = FALSE;		/* set if need to escape next char */
	bool quoting = FALSE;		/* set if within quoted string */
	int comment = 0;		/* level of parenthesized comments */
	int bracket = 0;		/* level of bracketed addresses */
	register char *p;
	register char c;

/*
 * Scan address list, and break when delimiter found.
 */
	for (p = addrspec; (c = *p) != '\0'; p++)
	{
		if (backslash)
			backslash = FALSE;
		else if (c == '\\')
			backslash = TRUE;
		else if (c == '"')
			quoting = !quoting;
		else if (quoting)
			continue;
		else if (c == delimiter && bracket == 0 && comment == 0)
			break;
		else if (c == '(')
			comment++;
		else if (c == ')')
			comment--;
		else if (comment > 0)
			continue;
		else if (c == '<')
			bracket++;
		else if (c == '>')
			bracket--;
		if (bracket < 0 || comment < 0)
			break;
	}

/*
 * Check for elementary syntax errors.
 * If ok, p points to the delimiter, or to a null byte.
 */
	if (quoting)
		usrerr("Unbalanced '\"'");
	else if (comment > 0)
		usrerr("Unbalanced '('");
	else if (comment < 0)
		usrerr("Unbalanced ')'");
	else if (bracket > 0)
		usrerr("Unbalanced '<'");
	else if (bracket < 0)
		usrerr("Unbalanced '>'");
	else
		return(p);

	return(NULL);
}

/*
** INVALIDADDR -- check an address for invalid control characters
** --------------------------------------------------------------
**
**	Returns:
**		TRUE if address string could cause problems.
**		FALSE otherwise.
**
**	Side Effects:
**		An error message is generated if invalid.
**
**	Called before any parsing is attempted.
*/

bool
invalidaddr(addrspec)
char *addrspec;				/* address specification */
{
	bool backslash = FALSE;		/* set if need to escape next char */
	bool quoting = FALSE;		/* set if within quoted string */
	register char *p;
	register char c;

	for (p = addrspec; (c = *p) != '\0'; p++)
	{
		/* always reject special metacharacters */
		if (is_meta(c))
			break;

		/* reject embedded newlines without lwsp */
		if (c == '\n' && !is_lwsp(p[1]))
			break;

		/* check for unquoted control characters */
		if (backslash)
			backslash = FALSE;
		else if (c == '\\')
			backslash = TRUE;
		else if (c == '"')
			quoting = !quoting;
		else if (quoting)
			continue;
		else
		{
			/* non-ascii for ordinary 8-bit characters */
			if (!isascii(c))
				continue;

			/* reject non-harmless control characters */
			if (iscntrl(c) && !isspace(c))
				break;
		}
	}

	/* abort if embedded control character found */
	if (*p != '\0')
	{
		usrerr("Invalid control character in address");
		return(TRUE);
	}

	/* avoid possible future buffer overflow */
	if (strlength(addrspec) > MAXSPEC)
	{
		usrerr("Address too long");
		return(TRUE);
	}

	/* so far so good */
	return(FALSE);
}

/*
** INVALIDHOST -- check for invalid domain name specification
** ----------------------------------------------------------
**
**	Returns:
**		TRUE if the domain name is (probably) invalid.
**		FALSE otherwise.
**
**	Side Effects:
**		An error message is generated if invalid.
*/

bool
invalidhost(domain)
char *domain;				/* domain name to be checked */
{
	register char *p;
	register int n;

	/* pickup domain name length */
	n = strlength(domain);

	/* must not be of zero length */
	if (n < 1)
	{
		usrerr("Invalid null domain");
		return(TRUE);
	}

	/* must not be too long */
	if (n > MAXHOST)
	{
		usrerr("Domain name too long");
		return(TRUE);
	}

	/* must not end with a dot */
	if (domain[n-1] == '.')
	{
		usrerr("Illegal trailing dot");
		return(TRUE);
	}

	/* must not be a plain dotted quad */
	if (inet_addr(domain) != NOT_DOTTED_QUAD)
	{
		usrerr("Illegal dotted quad");
		return(TRUE);
	}

	/* but may be a dotted quad between brackets */
	if (numeric_addr(domain) != NOT_DOTTED_QUAD)
		return(FALSE);

	/* check for invalid embedded characters */
	for (p = domain; *p != '\0'; p++)
	{
		/* only alphanumeric plus dot and dash allowed */
		if (!is_alnum(*p) && *p != '.' && *p != '-')
		{
			usrerr("Invalid domain name");
			return(TRUE);
		}
	}

	/* looks like a valid domain name */
	return(FALSE);
}

/*
** INVALIDLOOP -- Check whether an address is present in chain
** -----------------------------------------------------------
**
**	Returns:
**		TRUE if address is found in the chain.
**		FALSE otherwise.
**
**	Side effects:
**		An error message is generated if invalid.
*/

bool
invalidloop(address)
char *address;
{
	extern char *AddrChain[];	/* addresses in chain */
	extern int recursion_level;	/* current limit */
	register int j;

	for (j = 0; j < recursion_level; j++)
	{
		if (sameword(address, AddrChain[j]))
		{
			usrerr("Mail forwarding loop");
			return(TRUE);
		}
	}

	return(FALSE);
}

/*
** CATADDR -- Append an address part to an address buffer
** ------------------------------------------------------
**
**	Returns:
**		Next free position in address buffer.
**
**	Used by parseaddr() to construct an address without comments.
**	Address parts are concatenated without embedded blanks.
**	Trailing ';' characters used in group syntax are skipped.
*/

char *
cataddr(buf, address, addrspec)
char *buf;				/* start of address buffer */
register char *address;			/* buf position to append to */
register char *addrspec;		/* address spec to fetch from */
{
	/* skip leading whitespace */
	while (is_space(*addrspec))
		addrspec++;

	/* copy address part */
	while (*addrspec != '\0')
		*address++ = *addrspec++;

	/* remove trailing whitespace and trailing ';' */
	while (address > buf && (is_space(address[-1]) || address[-1] == ';'))
		address--;

	/* return next free position */
	return(address);
}
