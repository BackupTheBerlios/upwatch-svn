include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DWITH_THREADS
program = uw_postgresql
sbin_PROGRAMS = $(program)
man_MANS = uw_postgresql.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_postgresql_SOURCES = run.c uw_postgresql.h uw_postgresql.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_postgresql_CFLAGS = @LIBDBI_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBPQ_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_postgresql_LDADD = uw_postgresql_$(DB_O) uw_postgresql_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCHT) @LIBDBI_LIBS@ @LIBGTHREAD2_LIBS@ @LIBPQ_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

