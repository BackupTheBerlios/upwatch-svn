program = setproctitle
noinst_PROGRAMS = setproctitle
noinst_MANS = setproctitle.1
setproctitle_SOURCES = run.c cmd_options.c cmd_options.h
setproctitle_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@
BUILT_SOURCES = cmd_options.h 
INCLUDES = -I$(top_srcdir)/upwatch 
EXTRA_DIST = cmd_options.def
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@ @MYSQL_LIBS@
include $(top_srcdir)/Makefile.am.common

export TOP_SRCDIR = $(top_srcdir)

