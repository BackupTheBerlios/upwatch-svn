include $(top_srcdir)/Makefile.am.path

probe = imap
program = uw_imap
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_imap_SOURCES = run.c cmd_options.h cmd_options.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_imap_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h cmd_options.c $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch  -I$(INCL_ST)
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @MYSQL_LIBS@ @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

