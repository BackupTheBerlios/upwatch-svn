include $(top_srcdir)/Makefile.am.path

if HAVE_LIBPCAP
UW_IPTRAF = uw_iptraf
endif

if HAVE_LIBPQ
UW_POSTGRESQL = uw_postgresql
endif

if HAVE_LIBSNMP
UW_SNMPGET = uw_snmpget
endif

if HAVE_LIBTDS
UW_MSSQL = uw_mssql
endif

# variables to build installable packages
# packages are:
# base (includes docs, utils, uw_sysstat, uw_null and uw_send).
# monitor (for monitoring station. All remote probes.)
# iptraf (uw_iptraf)
# server (uw_process, uw_accept, uw_acceptbb, uw_setip)
CLIENTLIST = uw_send uw_sysstat uw_null
# order is important here: uw_process should be last
SERVERLIST = uw_acceptbb uw_accept uw_purple uw_route uw_syncprobes uw_setip uw_process
MONITORLIST = uw_httpget uw_dns uw_ping uw_mysql uw_mysqlstats uw_pop3 uw_smtp uw_tcpconnect uw_imap \
 ${UW_POSTGRESQL} ${UW_SNMPGET} ${UW_MSSQL}
IPTRAFLIST = uw_iptraf
EXTRALIST = ${IPTRAFLIST}

CLIENTPROG = ${CLIENTLIST}

if ENABLE_SERVER
SERVERPROG = ${SERVERLIST}
ENABLE_SERVER = --enable-server
endif

if ENABLE_MONITORS
MONITORPROG = ${MONITORLIST}
ENABLE_MONITOR = --enable-monitors
endif

if ENABLE_IPTRAF
ENABLE_IPTRAF = --enable-iptraf
IPTRAFPROG = ${IPTRAFLIST}
endif

EXTRAPROG = ${IPTRAFPROG}

if XMBMON
XMBMON = xmbmon203
endif

CONFARGS = --enable-client $(ENABLE_SERVER) $(ENABLE_MONITOR) $(ENABLE_IPTRAF)

# order is important: SERVERPROG should be last
PROGNAMES = ${CLIENTPROG} ${MONITORPROG} ${EXTRAPROG} ${SERVERPROG}

export PROGNAMES MONITORPROG CLIENTPROG SERVERPROG EXTRAPROG
export TOP_SRCDIR = $(top_srcdir)

if ENABLE_SERVER
SUBDIRS = upwatch st-1.7 ${XMBMON} libstatgrab-0.13 ${PROGNAMES} compat util scripts config common
else
SUBDIRS = upwatch ${XMBMON} libstatgrab-0.13 ${PROGNAMES} util scripts config common 
endif

DIST_SUBDIRS = ${SUBDIRS} doc

dist_doc_DATA = AUTHORS COPYING NEWS README upwatch-base.mysql upwatch-full.mysql upwatch-base.pgsql upwatch-full.pgsql

if ENABLE_SERVER
SERVER_DIST = probes.enum 
endif

EXTRA_DIST = Makefile.am.common Makefile.am.libs Makefile.am.path VERSION RELEASE libtool ChangeLog \
  upwatch-spec.def upwatch-spec.tpl upwatch.spec templates \
  patches/libpcap-linux-timeout.patch mswindows \
  ${SERVER_DIST} \
  install-sh config.sub mkinstalldirs depcomp


upwatch.spec: force upwatch-spec.def upwatch-spec.tpl common/spec.tpl \
  uw_*/*.spec-files uw_*/*.spec-requires  uw_*/*.spec-buildrequires
	autogen -b upwatch -Dconfargs="$(CONFARGS)" upwatch-spec.def
	rm -f force

upwatch-base.mysql: config/upwatch-realm.mysql config/upwatch-common.mysql uw_*/probe.def_mysql
	cat $+ > upwatch-base.mysql

upwatch-full.mysql: config/upwatch-full.mysql config/upwatch-common.mysql uw_*/probe.def_mysql uw_*/probe.raw_mysql
	cat $+ > upwatch-full.mysql

upwatch-base.pgsql: config/upwatch-realm.pgsql config/upwatch-common.pgsql uw_*/probe.def_pgsql
	echo "CREATE DOMAIN uinteger AS INTEGER CHECK( VALUE >= '0');" > upwatch-base.pgsql
	echo "CREATE DOMAIN usmallint AS SMALLINT CHECK( VALUE >= '0');" >> upwatch-base.pgsql
	echo "CREATE DOMAIN ubigint AS BIGINT CHECK( VALUE >= '0');" >> upwatch-base.pgsql
	echo "BEGIN;" >> upwatch-base.pgsql
	cat $+ >> upwatch-base.pgsql
	echo "ROLLBACK;" >> upwatch-base.pgsql

upwatch-full.pgsql: config/upwatch-full.pgsql config/upwatch-common.pgsql uw_*/probe.def_pgsql uw_*/probe.raw_pgsql
	echo "BEGIN;" > upwatch-full.pgsql
	cat $+ >> upwatch-full.pgsql
	echo "ROLLBACK;" >> upwatch-full.pgsql

force:
	touch force

RELEASE: force
	svnversion . > RELEASE
	rm -f force

CLEANFILES = upwatch.mysql upwatch.pgsql upwatch.spec upwatch-*.tar.gz

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${confdir} ;\
	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${upwatchdir} ;\
	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${dtddir} ;\
	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${initdir} ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${logdir} ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}${spooldir} ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTDIR}/var/run/upwatch ;\
	else \
	  mkdir -p ${DESTDIR}${confdir} ;\
	  mkdir -p ${DESTDIR}${upwatchdir} ;\
	  mkdir -p ${DESTDIR}${dtddir} ;\
	  mkdir -p ${DESTDIR}${initdir} ;\
	  mkdir -p ${DESTDIR}${logdir} ;\
	  mkdir -p ${DESTDIR}${spooldir} ;\
	  mkdir -p ${DESTDIR}/var/run/upwatch ;\
	fi

rpm: all dist upwatch.spec RELEASE
	SED=sed $(RPMBUILD) -ta upwatch-${VERSION}.tar.gz

dist-hook:
	rm -rf `find $(distdir) -name .svn`
if !ENABLE_SERVER
	for i in ${SERVERLIST} ; do \
	  rm -rf "$(distdir)/$$i" ; \
	done
endif
if !ENABLE_MONITORS
	for i in ${MONITORLIST} ; do \
	  rm -rf "$(distdir)/$$i" ; \
	done
endif
if !ENABLE_IPTRAF
	for i in ${EXTRALIST} ; do \
	  rm -rf "$(distdir)/$$i" ; \
	done
endif


#	rm -f $(distdir)/doc/program-guide.*
#	rm -f $(distdir)/doc/admin-guide.*

