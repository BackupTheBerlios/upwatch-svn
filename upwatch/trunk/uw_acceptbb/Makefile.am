program = uw_acceptbb
uw_acceptbb_SOURCES = run.c cmd_options.c cmd_options.h $(program).spec-generic \
 $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def
LDADD = $(DB_O) $(MAIN_O) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@ @LIBGNET_LIBS@
include $(top_srcdir)/Makefile.am.common

