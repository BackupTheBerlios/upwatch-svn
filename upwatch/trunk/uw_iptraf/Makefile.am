include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DWITH_THREADS
program = uw_iptraf
sbin_PROGRAMS = $(program)
man_MANS = uw_iptraf.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_iptraf_SOURCES = run.c uw_iptraf.h uw_iptraf.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_iptraf_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBPCAP_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_iptraf_LDADD = uw_iptraf_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCHT) @LIBGTHREAD2_LIBS@ @LIBPCAP_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def 

include $(top_srcdir)/Makefile.am.common

