include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -I$(INCL_ST) -DUSE_ST
program = uw_smtp
sbin_PROGRAMS = $(program)
man_MANS = uw_smtp.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_smtp_SOURCES = run.c uw_smtp.h uw_smtp.c  $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_smtp_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ $(AM_CFLAGS) 
uw_smtp_LDADD = uw_smtp_$(DB_O) uw_smtp_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @MYSQL_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

