probe = mysql
program = uw_mysql
sbin_PROGRAMS = $(program)
uw_mysql_SOURCES = run.c cmd_options.c cmd_options.h probe.def_h probe.res_h \
 $(program).spec-generic $(program).init
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGTHREAD2_LIBS@
include $(top_srcdir)/Makefile.am.common

