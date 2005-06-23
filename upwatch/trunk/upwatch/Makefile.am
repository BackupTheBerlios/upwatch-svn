include $(top_srcdir)/Makefile.am.path

noinst_LIBRARIES = libupwatch.a libupwatcht.a

if HAVE_LIBPCRE
LOGREGEX = logregex.c
endif

if ENABLE_SERVER
SERVER_SOURCE = pr_process.c pr_bb.c pr_bb_cpu.c pr_httpget.c pr_mysqlstats.c \
 pr_ping.c pr_snmpget.c pr_sysstat.c pr_iptraf.c pr_generic.c pr_hwstat.c 
endif

sources = $(SERVER_SOURCE) slot.c spool.c uwstat.c xml.c util.c setproctitle.c ifaddrlist.c \
 findsaddr-linux.c setsin.c compat.c log.c $(LOGREGEX)  \
 compat.h db.h generic.h slot.h spool.h uwstat.h xml.h setproctitle.h ifaddrlist.h \
 findsaddr.h gnuc.h logregex.h probe.h

if ENABLE_SERVER
pr_process.c: probe.h

probe.h: ../uw_acceptbb/probe.res_h  ../uw_httpget/probe.res_h \
 ../uw_imap/probe.res_h ../uw_iptraf/probe.res_h ../uw_mssql/probe.res_h ../uw_mysql/probe.res_h \
 ../uw_ping/probe.res_h ../uw_pop3/probe.res_h ../uw_postgresql/probe.res_h ../uw_snmpget/probe.res_h \
 ../uw_sysstat/probe.res_h ../uw_smtp/probe.res_h ../uw_tcpconnect/probe.res_h ../uw_mysqlstats/probe.res_h

%/probe.res_h:
	( cd $(@:%/probe.res_h=%) ; $(MAKE) probe.res_h )
endif

libupwatch_a_SOURCES = $(sources)
libupwatch_a_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @MYSQL_CFLAGS@ @LIBPCRE_CFLAGS@ $(AM_CFLAGS)

libupwatcht_a_SOURCES = $(sources)
libupwatcht_a_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @MYSQL_CFLAGS@ @LIBPCRE_CFLAGS@ $(AM_CFLAGS) -DWITH_THREADS

