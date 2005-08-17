program = setproctitle

noinst_PROGRAMS = $(program)
noinst_MANS = setproctitle.1

setproctitle_SOURCES = run.c setproctitle.c setproctitle.h
setproctitle_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
setproctitle_LDADD = setproctitle_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@ 

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.c) $(program:%=%.h)
INCLUDES = -I$(top_srcdir)/upwatch 
EXTRA_DIST = setproctitle.def
include $(top_srcdir)/Makefile.am.common

export TOP_SRCDIR = $(top_srcdir)

