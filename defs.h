/*
** Declaration of functions.
**
**	@(#)defs.h              e07@nikhef.nl (Eric Wassenaar) 971114
*/

/*
** Internal declarations of the vrfy utility
** -----------------------------------------
*/
	/* main.c */

int main		__P((int, char **));
void set_defaults	__P((char *, int, char **));
int getval		__P((char *, char *, int, int));
void fatal		__P((const char *, ...));
void error		__P((const char *, ...));
void usrerr		__P((const char *, ...));
void message		__P((const char *, ...));
void response		__P((char *));
void show		__P((int, char *));
void loop		__P((char *, char *));
void file		__P((char *));
void list		__P((char *));
void vrfy		__P((char *, char *));
void etrn		__P((char *, char *));
void ping		__P((char *));
void relay		__P((char *, char *));
int verify		__P((char *, char *));
int vrfyhost		__P((char *, char *));
int expnhost		__P((char *, char *));
int rcpthost		__P((char *, char *));
int etrnhost		__P((char *, char *));
int pinghost		__P((char *));
int relayhost		__P((char *, char *));
int getmxhosts		__P((char *));
char *setsender		__P((char *));

	/* pars.c */

char *parselist		__P((char *));
char *parsespec		__P((char *, char *, char *));
char *parseaddr		__P((char *));
char *parsehost		__P((char *));
char *find_delim	__P((char *, int));
bool_t invalidaddr	__P((char *));
bool_t invalidhost	__P((char *));
bool_t invalidloop	__P((char *));
char *cataddr		__P((char *, char *, char *));

	/* smtp.c */

int smtpinit		__P((char *));
int smtphelo		__P((char *, bool_t));
int smtpehlo		__P((char *));
int smtponex		__P((void));
int smtpverb		__P((char *));
int smtpetrn		__P((char *));
int smtprset		__P((void));
int smtpmail		__P((char *));
int smtprcpt		__P((char *));
int smtpexpn		__P((char *));
int smtpvrfy		__P((char *));
int smtpdata		__P((void));
int smtpbody		__P((void));
int smtpquit		__P((void));
void smtpmessage	__P((const char *, ...));
int smtpreply		__P((char *, bool_t));

	/* conn.c */

sigtype_t timer		__P((int));
char *sfgets		__P((char *, int, FILE *));
int makeconnection	__P((char *, FILE **, FILE **));
void setmyhostname	__P((void));
int getmyhostname	__P((char *));
bool_t internet		__P((char *));
ipaddr_t numeric_addr	__P((char *));

	/* stat.c */

char *statstring	__P((int));
char *errstring		__P((int));
void giveresponse	__P((int));

	/* mxrr.c */

int getmxbyname		__P((char *));

	/* util.c */

void fixcrlf		__P((char *, bool_t));
char *maxstr		__P((char *, int, bool_t));
char *printable		__P((char *));
ptr_t *xalloc		__P((ptr_t *, siz_t));
char *itoa		__P((int));

/*
** External library functions
** --------------------------
*/
	/* extern */

#if !defined(NO_INET_H)
#include <arpa/inet.h>
#else

ipaddr_t inet_addr	__P((const char *));
char *inet_ntoa		__P((struct in_addr));

#endif

	/* avoid <strings.h> */

#if !defined(index)

char *index		__P((const char *, int));
char *rindex		__P((const char *, int));

#endif

	/* <string.h> */

#if !defined(NO_STRING_H)
#include <string.h>
#else

char *strcpy		__P((char *, const char *));
char *strcat		__P((char *, const char *));
char *strncpy		__P((char *, const char *, siz_t));

#endif

	/* <stdlib.h> */

#if defined(__STDC__) && !defined(apollo)
#include <stdlib.h>
#else

char *getenv		__P((const char *));
ptr_t *malloc		__P((siz_t));
ptr_t *realloc		__P((ptr_t *, siz_t));
free_t free		__P((ptr_t *));
void exit		__P((int));

#endif

	/* <unistd.h> */

#if defined(__STDC__) && !defined(apollo)
#include <unistd.h>
#else

unsigned int alarm	__P((unsigned int));

#endif
