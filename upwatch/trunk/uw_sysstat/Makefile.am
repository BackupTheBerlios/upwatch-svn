include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lkstat
endif

probe = sysstat
program = uw_sysstat
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_sysstat_SOURCES = run.c cmd_options.c cmd_options.h probe.def_h probe.res_h \
 $(SPECFILES) $(INITFILES)
uw_sysstat_CFLAGS = @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ -I$(INCL_STATGRAB)
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(SOLARISLIBS) $(LIBSTATGRAB) $(LIBUPWATCH) @LIBXML2_LIBS@ @LIBGLIB2_LIBS@

include $(top_srcdir)/Makefile.am.common

