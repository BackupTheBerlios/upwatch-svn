include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -DUSE_ST
program = uw_template
sbin_PROGRAMS = $(program)
man_MANS = uw_template.1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_template_SOURCES = run.c cmd_options.c cmd_options.h cmd_options.def \
 $(SPECFILES) $(INITFILES)
uw_template_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@ $(AM_CFLAGS)
BUILT_SOURCES = cmd_options.h $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch -I$(INCL_ST)
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/$(program) ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/$(program)/new ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/$(program)/tmp ;\
	else \
	  mkdir -p ${DESTDIR}${spooldir}/$(program) ;\
	  mkdir -p ${DESTDIR}${spooldir}/$(program)/new ;\
	  mkdir -p ${DESTDIR}${spooldir}/$(program)/tmp ;\
	fi
