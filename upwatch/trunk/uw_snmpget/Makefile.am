include $(top_srcdir)/Makefile.am.path

probe = snmpget
program = uw_snmpget
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_snmpget_SOURCES = run.c cmd_options.h cmd_options.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_snmpget_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h cmd_options.c $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBSNMP_LIBS@ \
 @LIBCRYPTO_LIBS@ @LIBGLIB2_LIBS@ 
include $(top_srcdir)/Makefile.am.common

