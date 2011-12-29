include $(top_srcdir)/Makefile.am.path

program = uw_ping
sbin_PROGRAMS = $(program)
man_MANS = uw_ping.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_ping_SOURCES = run.c uw_ping.h uw_ping.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_ping_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBDBI_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_ping_LDADD = uw_ping_$(DB_O) uw_ping_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) @LIBDBI_LIBS@ @LIBXML2_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

