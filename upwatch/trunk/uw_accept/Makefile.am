program = uw_accept
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
uw_accept_SOURCES = run.c cmd_options.c cmd_options.h $(program).spec-files \
 $(program).init $(program).init-suse
uw_accept_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h  $(program).spec-files $(program).spec-requires \
 $(program).init $(program).init-suse $(program).spec-install
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBST) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

