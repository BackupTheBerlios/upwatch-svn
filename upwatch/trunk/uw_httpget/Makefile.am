probe = httpget
program = uw_httpget
sbin_PROGRAMS = $(program)
uw_httpget_SOURCES = run.c cmd_options.c cmd_options.h probe.def_h probe.res_h \
 $(program).spec-generic $(program).init
uw_httpget_CFLAGS = @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch -I$(INCL_ST)
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @MYSQL_LIBS@ @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

