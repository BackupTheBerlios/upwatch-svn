PROGNAMES = uw_send uw_accept uw_httpget uw_ping uw_process uw_traceroute uw_notify
SUBDIRS = doc upwatch ${PROGNAMES}

EXTRA_DIST = Makefile.am.common upwatch-spec.def upwatch-spec.tpl upwatch.spec config/upwatch.conf \
  common/db.c common/generic_options.def common/init.tpl common/main.c common/man1.tpl common/spec.tpl \
  doc/upwatch.xml doc/Makefile

upwatch.spec: upwatch-spec.def upwatch-spec.tpl touch.tch
	autogen -b upwatch upwatch-spec.def
	rm -f touch.tch

touch.tch:
	touch touch.tch
