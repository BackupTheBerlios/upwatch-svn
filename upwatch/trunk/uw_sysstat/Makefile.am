probe = sysstat
program = uw_sysstat
sbin_PROGRAMS = $(program)
uw_sysstat_SOURCES = run.c cmd_options.c sysinfo.c sysinfo.h version.c version.h \
 cmd_options.h probe.def_h probe.res_h $(program).spec-generic $(program).init $(program).init-suse
uw_sysstat_CFLAGS = @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(program).spec-generic $(program).init $(program).init-suse $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBXML2_LIBS@

include $(top_srcdir)/Makefile.am.common

