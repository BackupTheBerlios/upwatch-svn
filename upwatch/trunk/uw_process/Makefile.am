program = uw_process

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
uw_process_SOURCES = run.c process.c generic_ct.c cmd_options.c cmd_options.h \
 uw_process.h modules.inc $(program).spec-files $(program).init $(program).init-suse \
 ping.c httpget.c iptraf.c sysstat.c mysql.c bb_cpu.c bb.c pop3.c $(USE_LIBPQ) \
 $(USE_LIBSNMP) $(USE_LIBTDS) imap.c
uw_process_CFLAGS = @MYSQL_CFLAGS@ @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ \
  $(DEF_LIBPQ) $(DEF_LIBSNMP) $(DEF_LIBTDS) # -DUSE_THREADS
BUILT_SOURCES = cmd_options.h $(program).spec-files $(program).spec-requires \
 $(program).init $(program).init-suse $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(DB_O) $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @MYSQL_LIBS@ @LIBGLIB2_LIBS@ @LIBXML2_LIBS@
include $(top_srcdir)/Makefile.am.common

run.c: ../probes.enum

../probes.enum: ../uw_*/*.enum
	cat ../uw_*/*.enum > ../probes.enum

CLEANFILES += ../probes.enum
