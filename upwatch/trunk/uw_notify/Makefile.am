include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lresolv
endif

if HAVE_LIBPQ
USE_LIBPQ = ../uw_process/postgresql.c
DEF_LIBPQ = -DHAVE_LIBPQ
endif

if HAVE_LIBSNMP
USE_LIBSNMP = ../uw_process/snmpget.c
DEF_LIBSNMP = -DHAVE_LIBSNMP
endif

if HAVE_LIBTDS
USE_LIBTDS = ../uw_process/mssql.c
DEF_LIBTDS = -DHAVE_LIBTDS
endif

program = uw_notify
sbin_PROGRAMS = uw_notify
man_MANS = uw_notify.1
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_notify_SOURCES = ../uw_process/run.c ../uw_process/ping.c ../uw_process/generic_ct.c \
 uw_notify.h uw_notify.c uw_notify_glob.h \
 ../uw_process/modules.inc ../uw_process/httpget.c ../uw_process/iptraf.c ../uw_process/sysstat.c \
 ../uw_process/mysql.c ../uw_process/bb_cpu.c ../uw_process/bb.c ../uw_process/pop3.c \
 ../uw_process/imap.c ../uw_process/errlog.c ../uw_process/diskfree.c ../uw_process/process.c \
 $(USE_LIBPQ) $(USE_LIBSNMP) $(USE_LIBTDS) $(SPECFILES) $(INITFILES)
uw_notify_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ $(AM_CFLAGS) \
  $(DEF_LIBPQ) $(DEF_LIBSNMP) $(DEF_LIBTDS) $(DEF_LIBESMTP) -DUW_NOTIFY
uw_notify_LDADD = uw_notify_$(DB_O) uw_notify_$(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ \
  @LIBGLIB2_LIBS@ @LIBXML2_LIBS@ $(SOLARISLIBS)

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES)
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
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_failed ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_failed/new ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_failed/tmp ;\
	else \
	  mkdir -p ${DESTDIR}${spooldir}/uw_notify ;\
	  mkdir -p ${DESTDIR}${spooldir}/uw_notify/new ;\
	  mkdir -p ${DESTDIR}${spooldir}/uw_notify/tmp ;\
	  mkdir -p ${DESTDIR}${spooldir}/q_failed ;\
	  mkdir -p ${DESTDIR}${spooldir}/q_failed/new ;\
	  mkdir -p ${DESTDIR}${spooldir}/q_failed/tmp ;\
	fi

CLEANFILES += ../probes.enum

