program = bbhimport
bbhimport_SOURCES = run.c cmd_options.c cmd_options.h cmd_options.def
BUILT_SOURCES = cmd_options.h 
INCLUDES = -I$(top_srcdir)/upwatch 
EXTRA_DIST = cmd_options.def
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

export TOP_SRCDIR = $(top_srcdir)

