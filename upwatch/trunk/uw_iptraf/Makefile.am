include $(top_srcdir)/Makefile.am.path

probe = iptraf
program = uw_iptraf
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_iptraf_SOURCES = run.c cmd_options.c cmd_options.h probe.def_h probe.res_h \
 $(SPECFILES) $(INITFILES)
uw_iptraf_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBPCAP_CFLAGS@ @LIBXML2_CFLAGS@ -DUSE_THREADS
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGTHREAD2_LIBS@ @LIBPCAP_LIBS@
include $(top_srcdir)/Makefile.am.common

