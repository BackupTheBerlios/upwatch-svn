include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DUSE_ST
program = uw_accept
sbin_PROGRAMS = $(program)
man_MANS = uw_accept.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_accept_SOURCES = run.c uw_accept.h uw_accept.c $(SPECFILES) $(INITFILES)
uw_accept_CFLAGS = @LIBDBI_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS) 
uw_accept_LDADD = uw_accept_$(DB_O) uw_accept_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBST) $(LIBUPWATCH) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES) 
INCLUDES = -I../upwatch -I $(INCL_ST)
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def)
CLEAN_FILES = uw_accept.h uw_accept.c

include $(top_srcdir)/Makefile.am.common

