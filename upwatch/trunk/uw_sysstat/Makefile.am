include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lkstat -ldevinfo
endif

if FREEBSD
FREEBSDLIBS = -lkvm -ldevstat
endif

if XMBMON
XMBMON_LIB = ../xmbmon203/xmbmon.a
XMBMON_CFLAGS = -DUSE_XMBMON -I../xmbmon203
endif

AM_CFLAGS += -I$(INCL_STATGRAB) $(XMBMON_CFLAGS)
program = uw_sysstat
sbin_PROGRAMS = uw_sysstat
man_MANS = uw_sysstat.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_sysstat_SOURCES = run.c uw_sysstat.c uw_sysstat.h $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_sysstat_CFLAGS = @MYSQL_CFLAGS@ @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ $(AM_CFLAGS) @LIBPCRE_CFLAGS@
uw_sysstat_LDADD = uw_sysstat_$(MAIN_O) $(XMBMON_LIB) $(LIBOPTS) \
  $(FREEBSDLIBS) $(SOLARISLIBS) $(LIBSTATGRAB) \
  $(LIBUPWATCH) @LIBXML2_LIBS@ @LIBGLIB2_LIBS@ @LIBPCRE_LIBS@

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def) probe.def uw_sysstat.d/*/*

include $(top_srcdir)/Makefile.am.common

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${confdir}/uw_sysstat.d ;\
	  cp -r uw_sysstat.d/* ${DESTDIR}${confdir}/uw_sysstat.d ; \
	  find ${DESTDIR}${confdir}/uw_sysstat.d -type d -name CVS | xargs rm -rf ; \
	  chown -R upwatch:upwatch ${DESTDIR}${confdir}/uw_sysstat.d ; \
	else \
	  mkdir -p ${DESTDIR}${confdir}/uw_sysstat.d ;\
	  cp -r uw_sysstat.d/* ${DESTDIR}${confdir}/uw_sysstat.d ;\
	  find ${DESTDIR}${confdir}/uw_sysstat.d -type d -name CVS | xargs rm -rf ; \
	fi

