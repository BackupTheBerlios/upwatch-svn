program = uw_process
uw_process_SOURCES = process_httpget.c process_ping.c run.c cmd_options.c cmd_options.h $(program).spec-generic \
 $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def
uw_process_LDADD = db.o main.o ../upwatch/libupwatch.a
include $(top_srcdir)/Makefile.am.common

