probe = snmpget
program = uw_snmpget
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
uw_snmpget_SOURCES = run.c cmd_options.c cmd_options.h probe.def_h probe.res_h \
 $(program).spec-generic $(program).init $(program).init-suse
uw_snmpget_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(program).spec-generic $(program).init $(program).init-suse $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBSNMP_LIBS@ \
 @LIBCRYPTO_LIBS@ @LIBGLIB2_LIBS@ 
include $(top_srcdir)/Makefile.am.common

