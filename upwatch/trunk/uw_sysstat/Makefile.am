include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lkstat -ldevinfo
endif

AM_CFLAGS = -I$(INCL_STATGRAB)
program = uw_sysstat
sbin_PROGRAMS = uw_sysstat
man_MANS = uw_sysstat.1
conf_DATA = $(program:%=%.conf) $(program:%=%.logexp)
init_DATA = $(INITFILES)

uw_sysstat_SOURCES = run.c uw_sysstat.c uw_sysstat.h $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_sysstat_CFLAGS = @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ $(AM_CFLAGS)
uw_sysstat_LDADD = uw_sysstat_$(MAIN_O) $(LIBOPTS) $(SOLARISLIBS) $(LIBSTATGRAB) $(LIBUPWATCH) @LIBXML2_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def $(program:%=%.logexp) $(program:%=%.logtest)

include $(top_srcdir)/Makefile.am.common

