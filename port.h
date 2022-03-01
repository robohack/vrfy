/*
** Various portability definitions.
**
**	@(#)port.h              e07@nikhef.nl (Eric Wassenaar) 990511
*/

#if (defined(__SVR4) || defined(__svr4) || defined(SVR4) || defined(svr4)) && !defined(__svr4__)
# define __svr4__	1
#endif
#if defined(__SVR4) || defined(__svr4__)
#define SVR4
#endif

#if defined(__APPLE__) && defined(__MACH__) && !defined(__darwin__)
# define __darwin__	1
#endif

#if defined(SYSV) || defined(SVR4)
#define SYSV_MALLOC
#define SYSV_MEMSET
#define SYSV_STRCHR
#define SYSV_SETVBUF
#endif

#if defined(__hpux) || defined(hpux)
#define SYSV_MALLOC
#define SYSV_SETVBUF
#endif

#if defined(sgi)
#define SYSV_MALLOC
#endif

#if defined(linux)
#define SYSV_MALLOC
#endif

#if defined(bsdi) || defined(__bsdi__)
#define SYSV_MALLOC
#endif

#if defined(NeXT)
#define SYSV_MALLOC
#endif

/*
** Special definitions for certain platforms.
*/


/*
** Distinguish between various BIND releases.
*/

#if defined(RES_PRF_STATS)
#define BIND_49
#else
#define BIND_48
#endif

#if defined(BIND_49) && defined(__BIND)
#define BIND_493
#endif

/*
** Define constants for fixed sizes.
*/

#ifndef INT16SZ
#define	INT16SZ		2	/* for systems without 16-bit ints */
#endif

#ifndef INT32SZ
#define	INT32SZ		4	/* for systems without 32-bit ints */
#endif

#ifndef INADDRSZ
#define	INADDRSZ	4	/* for sizeof(struct inaddr) != 4 */
#endif

/*
** The following should depend on existing definitions.
*/

typedef int	bool_t;		/* boolean type */

#undef TRUE			/* SunOS-5 defines this in <rpc/types.h> */
#define TRUE		1
#undef FALSE			/* SunOS-5 defines this in <rpc/types.h> */
#define FALSE		0

#if defined(BIND_48) || defined(OLD_RES_STATE)
typedef struct state		res_state_t;
#else
typedef struct __res_state	res_state_t;
#endif

#if defined(BIND_493)
typedef u_char	qbuf_t;
#else
typedef char	qbuf_t;
#endif

#if defined(BIND_493)
typedef char	nbuf_t;
#else
typedef u_char	nbuf_t;
#endif

#ifndef _IPADDR_T
#if defined(__alpha) || defined(BIND_49)
typedef u_int	ipaddr_t;
#else
typedef u_long	ipaddr_t;
#endif
#endif

#if defined(apollo) || defined(_BSD_SIGNALS)
typedef int	sigtype_t;
#else
typedef void	sigtype_t;
#endif

#ifdef SYSV_MALLOC
typedef void	ptr_t;		/* generic pointer type */
typedef u_int	siz_t;		/* general size type */
typedef void	free_t;
#else
typedef char	ptr_t;		/* generic pointer type */
typedef u_int	siz_t;		/* general size type */
typedef int	free_t;
#endif

#ifdef SYSV_MEMSET
#define bzero(a,n)	(void) memset(a,'\0',n)
#define bcopy(a,b,n)	(void) memcpy(b,a,n)
#endif

#ifdef SYSV_STRCHR
#define index		strchr
#define rindex		strrchr
#endif

#ifdef SYSV_SETVBUF
#define linebufmode(a)	(void) setvbuf(a, (char *)NULL, _IOLBF, BUFSIZ);
#else
#define linebufmode(a)	(void) setlinebuf(a);
#endif

#if defined(sun) && defined(NO_YP_LOOKUP)
#define gethostbyname	(struct hostent *)res_gethostbyname
#define gethostbyaddr	(struct hostent *)res_gethostbyaddr
#endif

#if defined(SVR4)
#define jmp_buf		sigjmp_buf
#define setjmp(e)	sigsetjmp(e,1)
#define longjmp(e,n)	siglongjmp(e,n)
#endif

#ifdef NEED_SYS_ERR
extern char *sys_errlist[];
extern int sys_nerr;
#endif

#if defined(apollo) && defined(lint)
# define __attribute(x)
#endif

#ifndef __P		/* in *BSD's <sys/cdefs.h>, included by everything! */
# if ((__STDC__ - 0) > 0) || defined(__cplusplus)
#  define __P(protos)	protos		/* full-blown ANSI C */
# else
#  define __P(protos)	()		/* traditional C */
# endif
#endif

#ifndef const		/* in *BSD's <sys/cdefs.h>, included by everything! */
# if ((__STDC__ - 0) <= 0) || defined(apollo)
#  define const		/* NOTHING */
# endif
#endif

#ifdef __STDC__
# define VA_START(args, lastarg)       va_start(args, lastarg)
#else
# define VA_START(args, lastarg)       va_start(args)
#endif
