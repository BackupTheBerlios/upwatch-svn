noinst_LIBRARIES = libupwatch.a libupwatcht.a

sources = slot.c spool.c uwstat.c xml.c util.c setproctitle.c ifaddrlist.c \
 findsaddr-linux.c setsin.c compat.c pr_process.c pr_bb.c pr_bb_cpu.c pr_httpget.c \
 pr_ping.c pr_snmpget.c pr_sysstat.c pr_iptraf.c pr_generic.c \
 compat.h db.h generic.h slot.h spool.h uwstat.h xml.h setproctitle.h ifaddrlist.h \
 findsaddr.h gnuc.h probe.h

probe.h: ../uw_acceptbb/probe.res_h
	(cd ../uw_acceptbb ; make probe.res_h)
	(cd ../uw_httpget ; make probe.res_h)
	(cd ../uw_imap ; make probe.res_h)
	(cd ../uw_iptraf ; make probe.res_h)
	(cd ../uw_mssql ; make probe.res_h)
	(cd ../uw_mysql ; make probe.res_h)
	(cd ../uw_ping ; make probe.res_h)
	(cd ../uw_pop3 ; make probe.res_h)
	(cd ../uw_postgresql ; make probe.res_h)
	(cd ../uw_snmpget ; make probe.res_h)
	(cd ../uw_sysstat ; make probe.res_h)

libupwatch_a_SOURCES = $(sources)
libupwatch_a_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @MYSQL_CFLAGS@ 

libupwatcht_a_SOURCES = $(sources)
libupwatcht_a_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @MYSQL_CFLAGS@ -DWITH_THREADS
