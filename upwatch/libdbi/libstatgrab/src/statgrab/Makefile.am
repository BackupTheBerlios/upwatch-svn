# Makefile for libstatgrab/src/statgrab
# http://www.i-scream.org/libstatgrab/
# $Id: Makefile.am,v 1.2 2004/05/30 19:56:28 raarts Exp $

if STATGRAB
bin_PROGRAMS = statgrab
bin_SCRIPTS = statgrab-make-mrtg-config statgrab-make-mrtg-index
endif

statgrab_SOURCES = statgrab.c

INCLUDES = -I$(top_srcdir) -I$(top_builddir) \
           -I$(top_srcdir)/src -I$(top_builddir)/src\
           -I$(top_srcdir)/src/libstatgrab -I$(top_builddir)/src/libstatgrab

LDADD = $(top_builddir)/src/libstatgrab/libstatgrab.la @LINKFLAGS@

install-exec-local:
if STATGRAB
if SETGIDBINS
# Some operating systems need the binary to be setgid kmem
	chgrp kmem @bindir@/statgrab
	chmod g+s @bindir@/statgrab
endif
if SETUIDBINS
# Some operating systems need the binary to be setuid root
	chown root @bindir@/statgrab
	chmod u+s @bindir@/statgrab
endif
endif
