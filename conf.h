/*
** Various configuration definitions.
**
**	@(#)conf.h              e07@nikhef.nl (Eric Wassenaar) 940525
*/

/*
 * This is your nearest host running the sendmail daemon.
 * It is contacted in case local addresses without a domain are given.
 * Also when domain parsing errors were encountered, assuming that
 * this host can give a more appropriate error message.
 */

#ifndef LOCALHOST
#define LOCALHOST "localhost"	/* redefine if not running sendmail */
#endif

/*
 * This host is contacted when a .uucp address is specified.
 * You probably won't get much useful information.
 */

#ifndef UUCPRELAY
#define UUCPRELAY LOCALHOST	/* where to send pure uucp addresses */
#endif

/*
 * This host is contacted when a .bitnet or .earn address is specified.
 * You probably won't get much useful information.
 */

#ifndef BITNETRELAY
#define BITNETRELAY LOCALHOST	/* where to send earn/bitnet addresses */
#endif

/*
 * This host is contacted when a single unqualified host name
 * could not be resolved to a fully qualified MX domain host.
 * It is assumed that single hosts in your own domain can be
 * resolved, i.e. they have an MX record.
 * It depends on your local strategy for unqualified hosts what they
 * mean: a .uucp host, a .bitnet host, or just a local host without MX.
 */

#ifndef SINGLERELAY
#define SINGLERELAY LOCALHOST	/* where to send single host addresses */
#endif


/*
 * Various constants.
 */

#define MAXSPEC		256	/* maximum size of single address spec */
#define MAXREPLY	1200	/* maximum number of replies per query */
#define MAXHOP		17	/* default maximum recursion level */
#define MAXLOOP		50	/* maximum useable recursion level */
#define MAXMXHOSTS	10	/* maximum number of mx hosts */
#define MAXADDRS	35	/* max address count from gethostnamadr.c */
#define MAXHOSTNAME	256	/* maximum size of an hostname */
