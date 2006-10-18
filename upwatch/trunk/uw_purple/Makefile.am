include $(top_srcdir)/Makefile.am.path

program = uw_purple
sbin_PROGRAMS = $(program)
man_MANS = uw_purple.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_purple_SOURCES = run.c uw_purple.h uw_purple.c  $(SPECFILES) $(INITFILES)
uw_purple_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_purple_LDADD = uw_purple_$(DB_O) uw_purple_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) 

include $(top_srcdir)/Makefile.am.common

