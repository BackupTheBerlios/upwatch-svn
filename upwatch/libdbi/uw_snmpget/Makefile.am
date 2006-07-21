include $(top_srcdir)/Makefile.am.path

program = uw_snmpget
sbin_PROGRAMS = $(program)
man_MANS = uw_snmpget.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_snmpget_SOURCES = run.c uw_snmpget.h uw_snmpget.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_snmpget_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_snmpget_LDADD = uw_snmpget_$(DB_O) uw_snmpget_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBSNMP_LIBS@ @LIBCRYPTO_LIBS@ @LIBGLIB2_LIBS@ 

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

