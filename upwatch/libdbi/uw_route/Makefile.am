include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lresolv
endif

program = uw_route
sbin_PROGRAMS = $(program)
man_MANS = uw_route.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_route_SOURCES = run.c uw_route.h uw_route.c $(SPECFILES) $(INITFILES) 
uw_route_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS)
uw_route_LDADD = uw_route_$(DB_O) uw_route_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBDBI_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@ $(SOLARISLIBS)

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


