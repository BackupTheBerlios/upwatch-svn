include $(top_srcdir)/Makefile.am.path

program = uw_process

if SOLARIS
SOLARISLIBS = -lresolv
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

sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_process_SOURCES = run.c process.c generic_ct.c cmd_options.h cmd_options.c \
 insertcache.c uw_process.h modules.inc $(SPECFILES) $(INITFILES) \
 ping.c httpget.c iptraf.c sysstat.c mysql.c bb_cpu.c bb.c pop3.c $(USE_LIBPQ) \
 $(USE_LIBSNMP) $(USE_LIBTDS) imap.c errlog.c diskfree.c
uw_process_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ \
  $(DEF_LIBPQ) $(DEF_LIBSNMP) $(DEF_LIBTDS) # -DUSE_THREADS
BUILT_SOURCES = cmd_options.h cmd_options.c $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@ $(SOLARISLIBS)
include $(top_srcdir)/Makefile.am.common

run.c: ../probes.enum

../probes.enum: ../uw_*/*.enum
	cat ../uw_*/*.enum > ../probes.enum

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

CLEANFILES += ../probes.enum
