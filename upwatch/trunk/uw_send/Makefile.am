include $(top_srcdir)/Makefile.am.path

program = uw_send
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
redhat_DATA = $(program).redhat
suse_DATA = $(program).suse

uw_send_SOURCES = run.c cmd_options.c cmd_options.h cmd_options.def $(program).spec-files \
 $(program).redhat $(program).suse
uw_send_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@
BUILT_SOURCES = cmd_options.h  $(program).spec-files $(program).spec-requires \
 $(program).redhat $(program).suse $(program).spec-install
INCLUDES = -I../upwatch -I$(INCL_ST)
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

