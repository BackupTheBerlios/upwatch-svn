program = uw_ping
uw_ping_SOURCES = run.c cmd_options.c cmd_options.h $(program).spec-generic \
 $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).mysql
LDADD = $(DB_O) $(MAIN_O) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBXML2_LIBS@
uw_ping_CFLAGS = -g 
include $(top_srcdir)/Makefile.am.common
