#if HAVE_LIBNET
#USE_LIBNET = uw_investigate
#endif

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
# base (includes docs, utils, uw_sysstat and uw_send).
# monitor (for monitoring station. All remote probes.)
# iptraf (uw_iptraf)
# server (uw_process, uw_accept, uw_acceptbb, uw_setip)
CLIENTPROG = uw_sysstat uw_send
# order is important here: uw_process should be last
SERVERPROG = uw_acceptbb uw_accept uw_setip uw_notify uw_process
MONITORPROG = uw_httpget uw_ping uw_mysql uw_pop3 uw_imap \
 ${UW_IPTRAF} ${UW_POSTGRESQL} ${UW_SNMPGET} ${UW_MSSQL}
EXTRAPROG = uw_iptraf
# order is important: SERVERPROG should be last
PROGNAMES = ${CLIENTPROG} ${MONITORPROG} ${EXTRAPROG} ${SERVERPROG}

export PROGNAMES MONITORPROG CLIENTPROG SERVERPROG EXTRAPROG
export TOP_SRCDIR = $(top_srcdir)

SUBDIRS = upwatch st-1.4 libstatgrab-0.6 ${PROGNAMES} util scripts compat doc
DIST_SUBDIRS = ${SUBDIRS}

EXTRA_DIST = Makefile.am.common Makefile.am.libs VERSION libtool ChangeLog \
  upwatch-spec.def upwatch-spec.tpl upwatch.spec templates upwatch.mysql probes.enum \
  config/upwatch.conf config/upwatch-base.mysql config/result.dtd config/logrotate \
  config/cron.daily config/upwatch.init config/upwatch.init-suse \
  patches/libpcap-linux-timeout.patch \
  common/db.c common/generic_options.def common/dbase_options.def common/init.tpl \
  common/main.c common/man1.tpl common/spec.tpl common/minimal_options.def \
  common/probe.tpl common/probe_generic.def common/common.h \
  doc/upwatch.xml doc/Makefile doc/upwatch.xsl doc/jadetex.cfg doc/style.dsl \
  doc/style.xsl doc/style.css \
  cfg/install-sh cfg/config.sub cfg/mkinstalldirs cfg/depcomp cfg/cvs2cl.pl

upwatch.spec: upwatch-spec.def upwatch-spec.tpl common/spec.tpl \
  uw_*/*.spec-files uw_*/*.spec-requires uw_*/*.spec-install
	autogen -b upwatch upwatch-spec.def

upwatch.mysql: config/upwatch-base.mysql uw_*/probe.mysql
	cat $+ > upwatch.mysql

force-changelog:
	touch force-changelog

ChangeLog: force-changelog
	cfg/cvs2cl.pl -U AUTHORS -b -T --hide-filenames
	rm -f force-changelog

CLEANFILES = upwatch.mysql upwatch.spec upwatch-*.tar.gz

rpm: dist
	$(RPMBUILD) -ta upwatch-${VERSION}.tar.gz

