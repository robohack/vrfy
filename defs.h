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

int main		PROTO((int, char **));
void set_defaults	PROTO((char *, int, char **));
int getval		PROTO((char *, char *, int, int));
void fatal		PROTO((char *, ...));
void error		PROTO((char *, ...));
void usrerr		PROTO((char *, ...));
void message		PROTO((char *, ...));
void response		PROTO((char *));
void show		PROTO((int, char *));
void loop		PROTO((char *, char *));
void file		PROTO((char *));
void list		PROTO((char *));
void vrfy		PROTO((char *, char *));
void etrn		PROTO((char *, char *));
void ping		PROTO((char *));
int verify		PROTO((char *, char *));
int vrfyhost		PROTO((char *, char *));
int expnhost		PROTO((char *, char *));
int rcpthost		PROTO((char *, char *));
int etrnhost		PROTO((char *, char *));
int pinghost		PROTO((char *));
int getmxhosts		PROTO((char *));
char *setsender		PROTO((char *));

	/* pars.c */

char *parselist		PROTO((char *));
char *parsespec		PROTO((char *, char *, char *));
char *parseaddr		PROTO((char *));
char *parsehost		PROTO((char *));
char *find_delim	PROTO((char *, char));
bool invalidaddr	PROTO((char *));
bool invalidhost	PROTO((char *));
bool invalidloop	PROTO((char *));
char *cataddr		PROTO((char *, char *, char *));

	/* smtp.c */

int smtpinit		PROTO((char *));
int smtphelo		PROTO((char *, bool));
int smtpehlo		PROTO((char *));
int smtponex		PROTO((void));
int smtpverb		PROTO((char *));
int smtpetrn		PROTO((char *));
int smtprset		PROTO((void));
int smtpmail		PROTO((char *));
int smtprcpt		PROTO((char *));
int smtpexpn		PROTO((char *));
int smtpvrfy		PROTO((char *));
int smtpdata		PROTO((void));
int smtpbody		PROTO((void));
int smtpquit		PROTO((void));
void smtpmessage	PROTO((char *, ...));
int smtpreply		PROTO((char *, bool));

	/* conn.c */

sigtype_t timer		PROTO((int));
char *sfgets		PROTO((char *, int, FILE *));
int makeconnection	PROTO((char *, char **, char **));
void setmyhostname	PROTO((void));
int getmyhostname	PROTO((char *));
bool internet		PROTO((char *));
ipaddr_t numeric_addr	PROTO((char *));

	/* stat.c */

char *statstring	PROTO((int));
char *errstring		PROTO((int));
void giveresponse	PROTO((int));

	/* mxrr.c */

int getmxbyname		PROTO((char *));

	/* util.c */

void fixcrlf		PROTO((char *, bool));
char *maxstr		PROTO((char *, int, bool));
char *printable		PROTO((char *));
ptr_t *xalloc		PROTO((ptr_t *, siz_t));
char *itoa		PROTO((int));

/*
** External library functions
** --------------------------
*/
	/* extern */

#if !defined(NO_INET_H)
#include <arpa/inet.h>
#else

ipaddr_t inet_addr	PROTO((CONST char *));
char *inet_ntoa		PROTO((struct in_addr));

#endif

	/* avoid <strings.h> */

#if !defined(index)

char *index		PROTO((const char *, int));
char *rindex		PROTO((const char *, int));

#endif

	/* <string.h> */

#if !defined(NO_STRING_H)
#include <string.h>
#else

char *strcpy		PROTO((char *, const char *));
char *strcat		PROTO((char *, const char *));
char *strncpy		PROTO((char *, const char *, siz_t));

#endif

	/* <stdlib.h> */

#if defined(__STDC__) && !defined(apollo)
#include <stdlib.h>
#else

char *getenv		PROTO((const char *));
ptr_t *malloc		PROTO((siz_t));
ptr_t *realloc		PROTO((ptr_t *, siz_t));
free_t free		PROTO((ptr_t *));
void exit		PROTO((int));

#endif

	/* <unistd.h> */

#if defined(__STDC__) && !defined(apollo)
#include <unistd.h>
#else

unsigned int alarm	PROTO((unsigned int));

#endif
