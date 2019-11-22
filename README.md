# Overview

`vrfy` is a tool to verify electronic mail addresses using SMTP.

It recognises elementary syntax errors, but can do a lot more, up to
complex tasks such as recursively expand mailing lists and detect mail
forwarding loops.

In its simplest form, vrfy accepts an electronic mail address like
_"user@domain"_ for which it will figure out the MX hosts for
_"domain"_, set up an SMTP session with the primary MX host, and issue
the SMTP `VRFY` command with the given mail address.  The reply from the
remote host to the `VRFY` command is printed.

If no MX hosts exist, it will try to contact _"domain"_ itself.  In case
there is no _"domain"_, the address is supposed to represent a local
recipient which is verified at _"mail"_, assuming the local system is
configured to look up unqualified hostnames in some default domain that
should support email.

By default only the primary MX host is contacted, assuming that _"user"_
is local to that machine or that it otherwise has authoritative
knowledge of all valid mailboxes.

With an option one may choose to also query all the other MX hosts.

For pseudo domains like "uucp" or "bitnet" one can compile in explicit
servers to be contacted. They default to "localhost".  Not many servers
will tell what they are actually going to do with such addresses.

Instead of an electronic mail address one can specify the name of a file
containing address lists, e.g. mailing list recipient files or
_.forward_ files.  Verification of all recipients in the file is then
attempted.

If an explicit additional host name is specified on the command line,
verification is carried out at that host, and the input addresses are
passed to the host without further parsing.

Various levels of verbose output can be selected.  Very verbose mode
prints the full SMTP protocol trace with the remote host.  Even more
verbose mode causes an additional SMTP _VERB_ command to be issued,
hopefully resulting in the display of all actions taken by the remote
host when verifying the address.  This can be fun.

In the special ping mode, the mail exchangers for the specified
electronic mail domain will be contacted to check whether they do
seem to respond to SMTP requests.  No address verification is done.

_vrfy_ has built in the basic address parsing rules of sendmail, so it
can determine the domain part in complicated addresses such as:
_"comment \"comment\" comment" <user@domain (comment \comment)>_

Elementary syntax errors are caught locally.  If the domain part could
not be parsed beyond doubt, the address is verified on the default
`"mail"` host, hoping to get more detailed error messages from a real
MTA.

Another option lets you recursively verify the received replies to the
original verified address.  This is handy for mailing list expansions,
and also to detect possible mail forwarding loops.  This works only by
the grace of sendmail and other MTAs sending formal address
specifications in the `VRFY` replies.

Recursion stops automatically if a local recipient address is received,
or if a mail loop is detected.  If the received reply is the same as the
address that was asked for (modulo comments) the request is retried at
its domain itself, unless this was the machine we just queried, or it is
not an internet domain host.

The default recursion level is set to the `MAXHOP` value (17) as used by
sendmail, but this can be overruled (smaller or larger).

# Limitations

Many SMTP servers do not support the SMTP `VRFY` and `EXPN` commands, or
may reject those commands when they are issued by a "foreign" sender.

For this reason there is also an option to use the SMTP `RCPT` command
instead of the `VRFY` command.  This does not usually give the same
information, but it is better than nothing.  Usually the `HELO` or
`EHLO`, and `MAIL` commands are required as well.  Recursive mode is not
possible.

Some hosts refuse to `VRFY` the bracketed _"<comment <user@domain>>"_
but may accept the same address without the outermost brackets.

Usually hosts return addresses with an abundant amount of nested
brackets if you present a bracketed address with comments.

An option will strip all comments from addresses to be verified, to
avoid accumulating brackets during recursive verification.  This is now
the default when using the default recursion mode.

Some hosts return an error message, but with the 250 status code.  As
long as there is no '@' in the message, it can do no harm.

Some mailing lists have `CNAME` addresses, but we can now handle these,
and do not get into infinite recursion.

Some hosts return an unqualified address for local recipients.  This is
acceptable if it consists only of the pure "local part" but sometimes it
is of the form _"<user@host>"_ which is difficult to trace further.
