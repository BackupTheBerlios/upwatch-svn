include $(top_srcdir)/Makefile.am.path

probe = mysql
program = uw_mysql
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_mysql_SOURCES = run.c cmd_options.c cmd_options.h $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_mysql_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGTHREAD2_LIBS@
include $(top_srcdir)/Makefile.am.common

