include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DUSE_ST
program = uw_dns
sbin_PROGRAMS = $(program)
man_MANS = uw_dns.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_dns_SOURCES = run.c uw_dns.h uw_dns.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_dns_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS) 
uw_dns_LDADD = uw_dns_$(DB_O) uw_dns_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) $(LIBST) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def
include $(top_srcdir)/Makefile.am.common

