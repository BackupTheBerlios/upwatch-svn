include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DUSE_ST
program = uw_httpget
sbin_PROGRAMS = $(program) 
man_MANS = uw_httpget.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_httpget_SOURCES = run.c uw_httpget.h uw_httpget.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_httpget_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS) 
uw_httpget_LDADD = uw_httpget_$(DB_O) uw_httpget_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def
include $(top_srcdir)/Makefile.am.common

