include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DWITH_THREADS
if HAVE_LIBMYSQL
program = uw_mysql
endif
sbin_PROGRAMS = $(program)
man_MANS = uw_mysql.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_mysql_SOURCES = run.c uw_mysql.h uw_mysql.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_mysql_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_mysql_LDADD = uw_mysql_$(DB_O) uw_mysql_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCHT) @MYSQL_LIBS@ @LIBDBI_LIBS@ @LIBGTHREAD2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

