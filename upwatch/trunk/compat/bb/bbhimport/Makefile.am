program = bbhimport
bin_PROGRAMS = bbhimport
bbhimport_SOURCES = run.c cmd_options.c cmd_options.h
BUILT_SOURCES = cmd_options.h 
INCLUDES = -I$(top_srcdir)/upwatch 
EXTRA_DIST = cmd_options.def
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@ @MYSQL_LIBS@
include $(top_srcdir)/Makefile.am.common

export TOP_SRCDIR = $(top_srcdir)

