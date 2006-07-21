include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DWITH_THREADS
if HAVE_LIBTDS
program = uw_mssql
endif
sbin_PROGRAMS = $(program)
man_MANS = uw_mssql.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_mssql_SOURCES = run.c uw_mssql.h uw_mssql.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_mssql_CFLAGS = @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_mssql_LDADD = uw_mssql_$(DB_O) uw_mssql_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCHT) @LIBDBI_LIBS@ @LIBGTHREAD2_LIBS@ @LIBTDS_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

