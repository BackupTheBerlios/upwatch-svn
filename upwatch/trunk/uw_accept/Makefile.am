program = uw_accept
sbin_PROGRAMS = $(program)
uw_accept_SOURCES = run.c cmd_options.c cmd_options.h $(program).spec-generic \
 $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch $(ST_INC)
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBST) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

