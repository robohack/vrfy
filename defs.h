/*
** Declaration of functions.
**
**	@(#)defs.h              e07@nikhef.nl (Eric Wassenaar) 940524
*/

/* main.c */
int main();
void fatal();
void usrerr();
void message();
void response();
void show();
void loop();
void file();
void list();
void vrfy();
void ping();
int verify();
int vrfyhost();
int expnhost();
int rcpthost();
int pinghost();
int getmxhosts();

/* pars.c */
bool invalidaddr();
char *find_delim();
char *parselist();
char *cataddr();
char *parseaddr();
bool invalidhost();
char *parsehost();
char *parsespec();
bool invalidloop();

/* smtp.c */
int smtpinit();
int smtphelo();
int smtponex();
int smtpverb();
int smtpmail();
int smtprcpt();
int smtpexpn();
int smtpvrfy();
int smtpquit();
void smtpmessage();
int smtpreply();

/* conn.c */
sigtype_t timer();
char *sfgets();
int makeconnection();
void setmyhostname();
int getmyhostname();
bool internet();
ipaddr_t numeric_addr();

/* stat.c */
char *statstring();
char *errstring();
void giveresponse();

/* mxrr.c */
int getmxbyname();

/* util.c */
char *printable();
char *xalloc();

/* extern */
char *fgets();
char *index();
char *rindex();
char *strcat();
char *strcpy();
char *strncpy();
char *inet_ntoa();
ipaddr_t inet_addr();
void exit();
