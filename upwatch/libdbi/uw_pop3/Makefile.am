include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -I$(INCL_ST) -DUSE_ST
program = uw_pop3
sbin_PROGRAMS = $(program)
man_MANS = uw_pop3.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_pop3_SOURCES = run.c uw_pop3.h uw_pop3.c  $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_pop3_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_pop3_LDADD = uw_pop3_$(DB_O) uw_pop3_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) $(LIBST) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

