include $(top_srcdir)/Makefile.am.path

probe = ping
program = uw_ping
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_ping_SOURCES = run.c cmd_options.h cmd_options.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_ping_CFLAGS = @LIBGLIB2_CFLAGS@ @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h cmd_options.c $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBXML2_LIBS@ @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

