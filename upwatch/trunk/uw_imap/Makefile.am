include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DUSE_ST
program = uw_imap
sbin_PROGRAMS = $(program)
man_MANS = uw_imap.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_imap_SOURCES = run.c uw_imap.h uw_imap.c $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_imap_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_imap_LDADD = uw_imap_$(DB_O) uw_imap_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) $(LIBST) @MYSQL_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def

include $(top_srcdir)/Makefile.am.common

