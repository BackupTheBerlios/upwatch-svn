include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lresolv
endif

if HAVE_LIBESMTP
DEF_LIBESMTP = -DHAVE_LIBESMTP
endif

if HAVE_LIBPQ
USE_LIBPQ = postgresql.c
DEF_LIBPQ = -DHAVE_LIBPQ
endif

if HAVE_LIBSNMP
USE_LIBSNMP = snmpget.c
DEF_LIBSNMP = -DHAVE_LIBSNMP
endif

if HAVE_LIBTDS
USE_LIBTDS = mssql.c
DEF_LIBTDS = -DHAVE_LIBTDS
endif

program = uw_notify 
sbin_PROGRAMS = uw_notify 
man_MANS = uw_notify.1 
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_notify_SOURCES = run.c process.c uw_notify.c notify.c \
 ping.c httpget.c iptraf.c sysstat.c mysql.c bb_cpu.c bb.c pop3.c $(USE_LIBPQ) \
 $(USE_LIBSNMP) $(USE_LIBTDS) imap.c errlog.c diskfree.c \
 uw_notify.h uw_notify_glob.h modules.inc $(SPECFILES) $(INITFILES)
uw_notify_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS) \
  $(DEF_LIBPQ) $(DEF_LIBSNMP) $(DEF_LIBTDS) $(DEF_LIBESMTP) 
uw_notify_LDADD = uw_notify_$(DB_O) uw_notify_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ \
  @LIBESMTP_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@ $(SOLARISLIBS)

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES) ../probes.enum
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def)

include $(top_srcdir)/Makefile.am.common

run.c: ../probes.enum

../probes.enum: ../uw_*/*.enum
	cat ../uw_*/*.enum > ../probes.enum

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/uw_notify ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/uw_notify/new ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/uw_notify/tmp ;\
	else \
	  mkdir -p ${DESTDIR}${spooldir}/uw_notify ;\
	  mkdir -p ${DESTDIR}${spooldir}/uw_notify/new ;\
	  mkdir -p ${DESTDIR}${spooldir}/uw_notify/tmp ;\
	fi

CLEANFILES += ../probes.enum

