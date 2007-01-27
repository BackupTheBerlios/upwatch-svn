include $(top_srcdir)/Makefile.am.path

program = uw_syncprobes
sbin_PROGRAMS = $(program)
man_MANS = uw_syncprobes.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_syncprobes_SOURCES = run.c uw_syncprobes.h uw_syncprobes.c  $(SPECFILES) $(INITFILES)
uw_syncprobes_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_syncprobes_LDADD = uw_syncprobes_$(DB_O) uw_syncprobes_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) 

include $(top_srcdir)/Makefile.am.common

