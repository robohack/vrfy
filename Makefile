#	@(#)Makefile            e07@nikhef.nl (Eric Wassenaar) 950410

# ----------------------------------------------------------------------
# Adapt the installation directories to your local standards.
# ----------------------------------------------------------------------

# This is where the vrfy executable will go.
DESTBIN = /local/bin

# This is where the vrfy manual page will go.
DESTMAN = /local/share/man

BINDIR = $(DESTBIN)
MANDIR = $(DESTMAN)/man1

# ----------------------------------------------------------------------
# Special compilation options may be needed only on a few platforms.
# See also the header file port.h for portability issues.
# ----------------------------------------------------------------------

#if defined(_AIX)
SYSDEFS = -D_BSD -D_BSD_INCLUDES -U__STR__ -DBIT_ZERO_ON_LEFT
#endif
 
#if defined(solaris) && You do not want to use BSD compatibility mode
SYSDEFS = -DSYSV
#endif
 
#if defined(solaris) && You are using its default broken resolver library
SYSDEFS = -DNO_YP_LOOKUP
#endif

SYSDEFS =

# ----------------------------------------------------------------------
# Configuration definitions.
# See also the header file conf.h for further details.
# ----------------------------------------------------------------------

# Define LOCALHOST if "localhost" is not running the sendmail daemon.
CONFIGDEFS = -DLOCALHOST=\"mailhost\"
CONFIGDEFS = -DLOCALHOST=\"nikhefh\"
CONFIGDEFS = -DLOCALHOST=\"nikhapo\"
CONFIGDEFS = -DLOCALHOST=\"asgard\"
CONFIGDEFS = -DLOCALHOST=\"rurik\"
CONFIGDEFS = -DLOCALHOST=\"paramount\"

# Define UUCPRELAY if you have a better place to send uucp addresses.
CONFIGDEFS = -DUUCPRELAY=LOCALHOST

# Define BITNETRELAY if you know where to send earn/bitnet addresses.
CONFIGDEFS = -DBITNETRELAY=LOCALHOST

# Define SINGLERELAY as the host where to send unqualified host names.
CONFIGDEFS = -DSINGLERELAY=LOCALHOST

CONFIGDEFS =

# ----------------------------------------------------------------------
# Compilation definitions.
# ----------------------------------------------------------------------

DEFS = $(CONFIGDEFS) $(SYSDEFS)

COPTS =
COPTS = -O

CFLAGS = $(COPTS) $(DEFS)

# Select your favorite compiler.
CC = cc
CC = /usr/ucb/cc			#if defined(solaris) && BSD
CC = cc

# ----------------------------------------------------------------------
# Linking definitions.
# libresolv.a should contain the resolver library of BIND 4.8.2 or later.
# Link it in only if your default library is different.
# lib44bsd.a contains various utility routines, and comes with BIND 4.9.*
# You may need it if you link with the 4.9.* resolver library.
# libnet.a contains the getnet...() getserv...() getproto...() calls.
# It is safe to leave it out and use your default library.
# ----------------------------------------------------------------------

RES = ../res/libresolv.a
RES = -lresolv

COMPLIB = ../compat/lib/lib44bsd.a
COMPLIB = -lnet
COMPLIB =

LIBS = -lsocket -lnsl			#if defined(solaris) && not BSD
LIBS =

LIBRARIES = $(RES) $(COMPLIB) $(LIBS)

LDFLAGS =

# ----------------------------------------------------------------------
# Miscellaneous definitions.
# ----------------------------------------------------------------------

MAKE = make $(MFLAGS)

# This assumes the BSD install.
INSTALL = install -c

# Grrr
SHELL = /bin/sh

# ----------------------------------------------------------------------
# Files.
# ----------------------------------------------------------------------

HDRS = conf.h defs.h exit.h port.h vrfy.h
SRCS = main.c pars.c smtp.c conn.c stat.c mxrr.c util.c vers.c
OBJS = main.o pars.o smtp.o conn.o stat.o mxrr.o util.o vers.o
PROG = vrfy
MANS = vrfy.1
DOCS = RELEASE_NOTES

FILES = Makefile $(DOCS) $(HDRS) $(SRCS) $(MANS)

# ----------------------------------------------------------------------
# Rules for installation.
# ----------------------------------------------------------------------

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIBRARIES)

install: $(PROG)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)

man: $(MANS)
	$(INSTALL) -m 444 vrfy.1 $(MANDIR)

clean:
	rm -f $(PROG) $(OBJS) *.o a.out core vrfy.tar vrfy.tar.Z

# ----------------------------------------------------------------------
# Rules for maintenance.
# ----------------------------------------------------------------------

lint:
	lint $(DEFS) $(SRCS)

llint:
	lint $(DEFS) $(SRCS) -lresolv

print:
	lpr -J $(PROG) -p Makefile $(DOCS) $(HDRS) $(SRCS)

dist:
	tar cf vrfy.tar $(FILES)
	compress vrfy.tar

depend:
	mkdep $(DEFS) $(SRCS)

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
