include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DWITH_THREADS
program = uw_mysqlstats
sbin_PROGRAMS = $(program)
man_MANS = uw_mysqlstats.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_mysqlstats_SOURCES = run.c uw_mysqlstats.h uw_mysqlstats.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_mysqlstats_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_mysqlstats_LDADD = uw_mysqlstats_$(DB_O) uw_mysqlstats_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCHT) @MYSQL_LIBS@ @LIBGTHREAD2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

