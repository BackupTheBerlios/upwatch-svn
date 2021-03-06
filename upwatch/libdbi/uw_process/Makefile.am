include $(top_srcdir)/Makefile.am.path

if SOLARIS
SOLARISLIBS = -lresolv
endif

if HAVE_LIBESMTP
DEF_LIBESMTP = -DHAVE_LIBESMTP
endif

if HAVE_LIBGNOKII
DEF_LIBGNOKII = -DHAVE_LIBGNOKII
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

program = uw_process 
sbin_PROGRAMS = uw_process 
man_MANS = uw_process.1 
conf_DATA = $(program:%=%.conf)
init_DATA = $(INITFILES)

uw_process_SOURCES = run.c process.c notify.c generic_ct.c uw_process.c insertcache.c \
 ping.c httpget.c iptraf.c sysstat.c mysql.c mysqlstats.c bb_cpu.c bb.c pop3.c smtp.c local.c tcpconnect.c \
$(USE_LIBPQ) $(USE_LIBSNMP) $(USE_LIBTDS) imap.c errlog.c diskfree.c hwstat.c \
 uw_process.h uw_process_glob.h modules.inc $(SPECFILES) $(INITFILES)
uw_process_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBOPTS_CFLAGS@ $(AM_CFLAGS) \
  $(DEF_LIBPQ) $(DEF_LIBSNMP) $(DEF_LIBTDS) $(DEF_LIBESMTP) $(DEF_LIBGNOKII)
uw_process_LDADD = uw_process_$(DB_O) uw_process_$(MAIN_O) @LIBOPTS_LIBS@ $(LIBUPWATCH) @LIBDBI_LIBS@ \
  @LIBESMTP_LIBS@ @LIBGNOKII_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@ $(SOLARISLIBS) $(LIBGTHREAD2_LIBS)

BUILT_SOURCES = $(program:%=%.1) $(program:%=%.h) $(program:%=%.c) $(SPECFILES) $(INITFILES) ../probes.enum
INCLUDES = -I../upwatch
EXTRA_DIST = $(program:%=%.conf) $(program:%=%.def)

include $(top_srcdir)/Makefile.am.common

run.c: ../probes.enum

../probes.enum: ../uw_*/*.enum
	cat ../uw_*/*.enum > ../probes.enum

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/uw_process ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/uw_process/new ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/uw_process/tmp ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_failed ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_failed/new ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_failed/tmp ;\
	  for i in `cat ../probes.enum | sed -e 's:PROBE_::g' | cut -f1 -d=`; do \
	    ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_$$i ;\
	    ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_$$i/new ;\
	    ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir}/q_$$i/tmp ;\
	  done ; \
	else \
	  mkdir -p ${DESTDIR}${spooldir}/uw_process ;\
	  mkdir -p ${DESTDIR}${spooldir}/uw_process/new ;\
	  mkdir -p ${DESTDIR}${spooldir}/uw_process/tmp ;\
	  mkdir -p ${DESTDIR}${spooldir}/q_failed ;\
	  mkdir -p ${DESTDIR}${spooldir}/q_failed/new ;\
	  mkdir -p ${DESTDIR}${spooldir}/q_failed/tmp ;\
	  for i in `cat ../probes.enum | sed -e 's:PROBE_::g' | cut -f1 -d=`; do \
	    mkdir -p ${DESTDIR}${spooldir}/q_$$i ;\
	    mkdir -p ${DESTDIR}${spooldir}/q_$$i/new ;\
	    mkdir -p ${DESTDIR}${spooldir}/q_$$i/tmp ;\
	  done ; \
	fi

CLEANFILES += ../probes.enum

