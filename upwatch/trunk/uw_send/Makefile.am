program = uw_send
sbin_PROGRAMS = $(program)
uw_send_SOURCES = run.c cmd_options.c cmd_options.h cmd_options.def $(program).spec-generic \
 $(program).init
uw_send_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch -I$(INCL_ST)
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

