program = bbhimport
bin_PROGRAMS = bbhimport
man_MANS = $(program).1
bbhimport_SOURCES = run.c cmd_options.c cmd_options.h bbhimport.h
bbhimport_CFLAGS = @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@
BUILT_SOURCES = cmd_options.h 
INCLUDES = -I$(top_srcdir)/upwatch 
EXTRA_DIST = cmd_options.def
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@ @MYSQL_LIBS@
include $(top_srcdir)/Makefile.am.common

export TOP_SRCDIR = $(top_srcdir)

