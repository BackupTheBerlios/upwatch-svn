PROGNAMES = uw_send uw_accept uw_httpget uw_ping uw_process uw_traceroute uw_notify
SUBDIRS = doc upwatch ${PROGNAMES}

EXTRA_DIST = Makefile.am.common upwatch-spec.def upwatch-spec.tpl upwatch.spec upwatch.mysql templates \
  config/upwatch.conf config/upwatch-base.mysql config/result.dtd \
  common/db.c common/generic_options.def common/init.tpl common/main.c common/man1.tpl common/spec.tpl \
  doc/upwatch.xml doc/Makefile doc/upwatch.xsl doc/jadetex.cfg doc/style.dsl doc/style.xsl doc/style.css

BUILT_SOURCES = upwatch.mysql 

upwatch.spec: upwatch-spec.def upwatch-spec.tpl
	autogen -b upwatch upwatch-spec.def

upwatch.mysql: config/upwatch-base.mysql uw_*/uw_*.mysql
	cat $+ > upwatch.mysql

rpm: dist
	rpm -ta upwatch-0.1.tar.gz

