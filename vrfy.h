/*
** Master include file.
**
**	@(#)vrfy.h              e07@nikhef.nl (Eric Wassenaar) 940524
*/

#if defined(apollo) && defined(lint)
#define __attribute(x)
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>

#undef NOERROR			/* in <sys/streams.h> on solaris 2.x */
#include <arpa/nameser.h>
#include <resolv.h>

#include "conf.h"		/* various configuration definitions */
#include "exit.h"		/* exit codes come from <sysexits.h> */
#include "port.h"		/* various portability definitions */

typedef int	bool;		/* boolean type */
#define TRUE	1
#define FALSE	0

#define NOT_DOTTED_QUAD	((ipaddr_t)-1)

/* some string functions */
#define sameword(a,b)	(strcasecmp(a,b) == 0)
#define newstr(a)	strcpy(xalloc(strlen(a) + 1), a)
#define xfree(a)	(void) free((ptr_t *)a)
#define strlength(a)	(int)strlen(a)

/* character checking */
#define is_alnum(c)	(isascii(c) && isalnum(c))
#define is_digit(c)	(isascii(c) && isdigit(c))
#define is_space(c)	(isascii(c) && isspace(c))

/* sendmail V8 meta-characters */
#define is_meta(c)	(((c) & 0340) == 0200)

#ifdef lint
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN int errno;
EXTERN int h_errno;		/* defined in gethostnamadr.c */
EXTERN res_state_t _res;	/* defined in res_init.c */

#include "defs.h"		/* declarations of functions */

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
