program = uw_process
uw_process_SOURCES = run.c cmd_options.c cmd_options.h uw_process.h modules.inc $(program).spec-generic $(program).init \
 ping.c httpget.c iptraf.c sysstat.c bb_cpu.c
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def
LDADD = $(DB_O) $(MAIN_O) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@
include $(top_srcdir)/Makefile.am.common

