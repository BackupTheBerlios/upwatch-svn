PROGNAMES = uw_send uw_accept uw_process uw_notify uw_acceptbb uw_investigate \
 uw_httpget uw_ping uw_iptraf uw_sysstat
SUBDIRS = doc upwatch ${PROGNAMES}

EXTRA_DIST = Makefile.am.common upwatch-spec.def upwatch-spec.tpl upwatch.spec upwatch.mysql templates \
  config/upwatch.conf config/upwatch-base.mysql config/result.dtd config/logrotate config/cron.daily \
  config/upwatch.init patches/libpcap-linux-timeout.patch \
  common/db.c common/generic_options.def common/init.tpl common/main.c common/man1.tpl common/spec.tpl \
  doc/upwatch.xml doc/Makefile doc/upwatch.xsl doc/jadetex.cfg doc/style.dsl doc/style.xsl doc/style.css

BUILT_SOURCES = upwatch.mysql 

upwatch.spec: upwatch-spec.def upwatch-spec.tpl uw_*/*.spec-generic uw_*/*.spec-install
	autogen -b upwatch upwatch-spec.def

upwatch.mysql: config/upwatch-base.mysql uw_*/uw_*.mysql
	cat $+ > upwatch.mysql

rpm: dist
	rpmbuild -ta upwatch-0.2.tar.gz


ACLOCAL_AMFLAGS = -I m4
