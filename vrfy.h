/*
** Master include file of the vrfy utility.
**
**	@(#)vrfy.h              e07@nikhef.nl (Eric Wassenaar) 990522
*/

#if defined(apollo) && defined(lint)
#define __attribute(x)
#endif

#undef  obsolete		/* old code left as a reminder */
#undef  notyet			/* new code for possible future use */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>

#include <sys/types.h>		/* not always automatically included */
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#undef NOERROR			/* in <sys/streams.h> on solaris 2.x */
#include <arpa/nameser.h>
#include <resolv.h>

#include "port.h"		/* various portability definitions */
#include "conf.h"		/* various configuration definitions */
#include "exit.h"		/* exit codes come from <sysexits.h> */

#define NOT_DOTTED_QUAD	((ipaddr_t)-1)

#ifdef lint
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int errno;
EXTERN int h_errno;		/* defined in the resolver library */
EXTERN res_state_t _res;	/* defined in res_init.c */

#include "defs.h"		/* declarations of functions */

/* memory allocation */
#define newlist(a,n,t)	(t *)xalloc((ptr_t *)a, (siz_t)((n)*sizeof(t)))
#define newstruct(t)	(t *)xalloc((ptr_t *)NULL, (siz_t)(sizeof(t)))
#define newstring(s)	(char *)xalloc((ptr_t *)NULL, (siz_t)(strlen(s)+1))
#define newstr(s)	strcpy(newstring(s), s)
#define xfree(a)	(void) free((ptr_t *)a)

/* some string functions */
#define sameword(a,b)	(strcasecmp(a,b) == 0)
#define strlength(a)	(int)strlen(a)

/* character checking */
#define is_alnum(c)	(isascii(c) && isalnum(c))
#define is_digit(c)	(isascii(c) && isdigit(c))
#define is_space(c)	(isascii(c) && isspace(c))

/* check for linear white space */
#define is_lwsp(c)	(((c) == ' ') || ((c) == '\t'))

/* sendmail V8 meta-characters */
#define is_meta(c)	(((c) & 0340) == 0200)

/* special address syntax */
#define file_addr(a)	((a)[0] == '/')
#define prog_addr(a)	((a)[0] == '|' || ((a)[0] == '"' && (a)[1] == '|'))
#define incl_addr(a)	(strncmp(a, ":include:", 9) == 0)

#ifdef DEBUG
#define assert(condition)\
{\
	if (!(condition))\
	{\
		(void) fprintf(stderr, "assertion botch: ");\
		(void) fprintf(stderr, "%s(%d): ", __FILE__, __LINE__);\
		(void) fprintf(stderr, "%s\n", "condition");\
		exit(EX_SOFTWARE);\
	}\
}
#else
#define assert(condition)
#endif
