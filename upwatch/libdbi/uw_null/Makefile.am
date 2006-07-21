include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lresolv
endif

program = uw_null
sbin_PROGRAMS = $(program)
man_MANS = uw_null.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_null_SOURCES = run.c uw_null.h uw_null.c $(SPECFILES) $(INITFILES) 
uw_null_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_null_LDADD = uw_null_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@ $(SOLARISLIBS)

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


