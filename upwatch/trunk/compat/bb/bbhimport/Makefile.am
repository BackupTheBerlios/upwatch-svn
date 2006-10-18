include $(top_srcdir)/Makefile.am.path

program = bbhimport
bin_PROGRAMS = $(program)
man_MANS = bbhimport.1

bbhimport_SOURCES = run.c bbhimport.h bbhimport.c
bbhimport_CFLAGS = @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
bbhimport_LDADD = bbhimport_$(DB_O) bbhimport_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) @LIBGLIB2_LIBS@ @MYSQL_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c)
INCLUDES = -I$(top_srcdir)/upwatch 
EXTRA_DIST = bbhimport.def
include $(top_srcdir)/Makefile.am.common

export TOP_SRCDIR = $(top_srcdir)

