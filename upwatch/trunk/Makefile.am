PROBES = uw_httpget uw_ping uw_sysstat uw_acceptbb uw_mysql \
  uw_pop3 uw_imap

if HAVE_LIBNET
USE_LIBNET = uw_investigate
endif

if HAVE_LIBPCAP
USE_LIBPCAP = uw_iptraf
endif

if HAVE_LIBPQ
USE_LIBPQ = uw_postgresql
endif

if HAVE_LIBSNMP
USE_LIBSNMP = uw_snmpget
endif

if HAVE_LIBTDS
USE_LIBTDS = uw_mssql
endif

PROBES += $(USE_LIBPCAP) $(USE_LIBNET) $(USE_LIBPQ) $(USE_LIBSNMP) \
 $(USE_LIBTDS) 

# NOTE: order is important, first the probes, then uw_process, then the rest
PROGNAMES = ${PROBES} uw_process uw_send uw_accept uw_setip uw_notify 

export PROGNAMES
export TOP_SRCDIR = $(top_srcdir)

SUBDIRS = upwatch st-1.4 ${PROGNAMES} util scripts compat doc
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

upwatch.spec: upwatch-spec.def upwatch-spec.tpl uw_*/*.spec-generic uw_*/*.spec-install
	autogen -b upwatch upwatch-spec.def

upwatch.mysql: config/upwatch-base.mysql uw_*/probe.mysql
	cat $+ > upwatch.mysql

force-changelog:
	touch force-changelog

ChangeLog: force-changelog
	cfg/cvs2cl.pl -U AUTHORS -b -T --hide-filenames
	rm -f force-changelog

CLEANFILES = upwatch.mysql upwatch.spec

rpm: dist
	$(RPMBUILD) -ta upwatch-${VERSION}.tar.gz

