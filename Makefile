# ----------------------------------------------------------------------
# Adapt the installation directories to your local standards.
# ----------------------------------------------------------------------

BINDIR = /local/bin
MANDIR = /local/share/man/man1

# ----------------------------------------------------------------------
# Special compilation options are needed only on a few platforms.
# ----------------------------------------------------------------------

#if defined(_AIX)
#	SYSDEFS = -D_BSD -D_BSD_INCLUDES -U__STR__ -DBIT_ZERO_ON_LEFT
#endif

SYSDEFS = -I/local/include
SYSLDF = -L/local/lib

# ----------------------------------------------------------------------
# Compilation definitions.
# ----------------------------------------------------------------------

# Define LOCALHOST if "localhost" is not running the sendmail daemon.
#	DEFS = -DLOCALHOST=\"nikhapo\"

# Define UUCPRELAY if you have a better place to send uucp addresses.
#	DEFS = -DUUCPRELAY=LOCALHOST

# Define EARNRELAY if you know where to send earn/bitnet addresses.
#	DEFS = -DEARNRELAY=LOCALHOST

#DEFS = -DUUCPRELAY=\"nirv.web.net\"

OPTIM = -O
SDB =#-g
CFLAGS = $(SDB) $(OPTIM) $(DEFS) $(SYSDEFS)
LDFLAGS = $(SDB) $(SYSLDF)

CC = /usr/5bin/cc

# ----------------------------------------------------------------------
# Files.
# ----------------------------------------------------------------------

HDRS =
SRCS = main.c pars.c smtp.c conn.c stat.c mxrr.c util.c
OBJS = main.o pars.o smtp.o conn.o stat.o mxrr.o util.o
PROG = vrfy
MANS = vrfy.1

FILES = Makefile $(HDRS) $(SRCS) $(MANS)

# ----------------------------------------------------------------------
# libresolv.a should contain the resolver library of BIND 4.8.2 or later.
# Link it in only if your default library is different.
# libnet.a contains the getnet...() getserv...() getproto...() calls.
# It is safe to leave it out and use your default library.
# ----------------------------------------------------------------------

#LIBS = ../resolver/libresolv.a
LIBS = -lresolv
#LIBS = -lresolv -lnet

# ----------------------------------------------------------------------
# Rules for installation.
# ----------------------------------------------------------------------

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBS)

install: $(PROG)
	install -c -m 755 -s $(PROG) $(BINDIR)
	install -c -m 444 vrfy.1 $(MANDIR)

clean:
	rm -f $(PROG) $(OBJS) *.o a.out core vrfy.tar vrfy.tar.Z

# ----------------------------------------------------------------------
# Rules for maintenance.
# ----------------------------------------------------------------------

print:
	lpr -J vrfy.c -p $(HDRS) $(SRCS)

lint:
	lint $(SRCS)

llint:
	lint $(SRCS) -lresolv

dist:
	tar cf vrfy.tar $(FILES)
	compress vrfy.tar
