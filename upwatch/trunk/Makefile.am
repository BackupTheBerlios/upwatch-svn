PROBES = uw_httpget uw_ping uw_iptraf uw_sysstat uw_acceptbb uw_mysql uw_snmpget uw_pop3

# NOTE: order is important, first the probes, then uw_process, then the rest
PROGNAMES = ${PROBES} uw_process uw_send uw_accept uw_notify uw_investigate

export PROGNAMES
export TOP_SRCDIR = $(top_srcdir)

SUBDIRS = upwatch st-1.4 ${PROGNAMES} util compat doc

EXTRA_DIST = Makefile.am.common upwatch-spec.def upwatch-spec.tpl upwatch.spec templates \
  config/upwatch.conf config/upwatch-base.mysql config/result.dtd config/logrotate config/cron.daily \
  config/upwatch.init patches/libpcap-linux-timeout.patch \
  common/db.c common/generic_options.def common/dbase_options.def common/init.tpl \
  common/main.c common/man1.tpl common/spec.tpl \
  common/probe.tpl common/probe_generic.def common/common.h \
  doc/upwatch.xml doc/Makefile doc/upwatch.xsl doc/jadetex.cfg doc/style.dsl doc/style.xsl doc/style.css \
  upwatch.mysql probes.enum Makefile.am.libs VERSION common/minimal_options.def \
  cfg/install-sh cfg/config.sub libtool cfg/mkinstalldirs cfg/depcomp ChangeLog

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
	rpmbuild -ta upwatch-${VERSION}.tar.gz

