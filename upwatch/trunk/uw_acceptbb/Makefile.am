include $(top_srcdir)/Makefile.am.path

program = uw_acceptbb
sbin_PROGRAMS = $(program)
man_MANS = uw_acceptbb.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_acceptbb_SOURCES = run.c uw_acceptbb.h uw_acceptbb.c \
 $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_acceptbb_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ $(AM_CFLAGS)
uw_acceptbb_LDADD = uw_acceptbb_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @MYSQL_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = $(program:%=%.def) $(program:%=%.conf) probe.def probe.enum probe.mysql
include $(top_srcdir)/Makefile.am.common

