include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -I$(INCL_ST) -DUSE_ST
program = uw_tcpconnect
sbin_PROGRAMS = $(program)
man_MANS = uw_tcpconnect.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_tcpconnect_SOURCES = run.c uw_tcpconnect.h uw_tcpconnect.c  $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_tcpconnect_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_tcpconnect_LDADD = uw_tcpconnect_$(DB_O) uw_tcpconnect_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) $(LIBST) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

