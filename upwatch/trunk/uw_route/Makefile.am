include $(top_srcdir)/Makefile.am.path

program = uw_route

if SOLARIS
SOLARISLIBS = -lresolv
endif

sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_route_SOURCES = run.c cmd_options.h cmd_options.c \
 $(SPECFILES) $(INITFILES) 
uw_route_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ 
BUILT_SOURCES = cmd_options.h cmd_options.c $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ \
  @LIBGLIB2_LIBS@ @LIBXML2_LIBS@ $(SOLARISLIBS)
include $(top_srcdir)/Makefile.am.common


