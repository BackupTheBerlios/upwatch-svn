include $(top_srcdir)/Makefile.am.path

program = uw_accept
sbin_PROGRAMS = $(program)
man_MANS = $(program:%=%.1)
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_accept_SOURCES = run.c uw_accept_options.h uw_accept_options.c $(SPECFILES) $(INITFILES)
uw_accept_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ $(AM_CFLAGS)
uw_accept_LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBST) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = uw_accept_options.h uw_accept_options.c $(SPECFILES) $(INITFILES) 
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = uw_accept_options.def $(program:%=%.conf)
include $(top_srcdir)/Makefile.am.common

