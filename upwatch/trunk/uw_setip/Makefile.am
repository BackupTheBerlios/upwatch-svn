include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -I$(INCL_ST)
program = uw_setip
sbin_PROGRAMS = $(program)
man_MANS = uw_setip.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_setip_SOURCES = run.c uw_setip.h uw_setip.c $(SPECFILES) $(INITFILES)
uw_setip_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ $(AM_CFLAGS)
uw_setip_LDADD = uw_setip_$(DB_O) uw_setip_$(MAIN_O) $(LIBOPTS) $(LIBST) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def)

include $(top_srcdir)/Makefile.am.common

