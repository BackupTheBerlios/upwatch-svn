program = uw_process
sbin_PROGRAMS = $(program)
uw_process_SOURCES = run.c process.c cmd_options.c cmd_options.h uw_process.h modules.inc $(program).spec-generic $(program).init \
 ping.c httpget.c iptraf.c sysstat.c mysql.c snmpget.c bb_cpu.c bb_generic.c pop3.c
BUILT_SOURCES = cmd_options.h $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@
include $(top_srcdir)/Makefile.am.common

run.c: ../probes.enum

../probes.enum: ../uw_*/*.enum
	cat ../uw_*/*.enum > ../probes.enum

CLEANFILES += ../probes.enum
