include $(top_srcdir)/Makefile.am.path

AM_CFLAGS += -I$(INCL_ST) -DUSE_ST
program = uw_send
sbin_PROGRAMS = $(program)
man_MANS = uw_send.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_send_SOURCES = run.c uw_send.c uw_send.h uw_send.def $(SPECFILES) $(INITFILES)
uw_send_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@ $(AM_CFLAGS)
uw_send_LDADD = uw_send_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) $(LIBST) @LIBGLIB2_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) 

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
