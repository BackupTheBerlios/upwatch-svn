include $(top_srcdir)/Makefile.am.path

probe = mysql
program = uw_mysql
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
redhat_DATA = $(program).redhat
suse_DATA = $(program).suse

uw_mysql_SOURCES = run.c cmd_options.c cmd_options.h probe.def_h probe.res_h \
 $(program).spec-files $(program).redhat $(program).suse
uw_mysql_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@
BUILT_SOURCES = cmd_options.h probe.mysql probe.enum probe.def_h probe.res_h \
 $(program).spec-files $(program).spec-requires $(program).redhat $(program).suse $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def probe.def probe.enum probe.mysql $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGTHREAD2_LIBS@
include $(top_srcdir)/Makefile.am.common

