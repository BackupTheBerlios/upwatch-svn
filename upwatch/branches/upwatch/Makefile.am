SUBDIRS = upwatch uw_send uw_accept uw_httpget uw_ping uw_process uw_traceroute
PROGNAMES = uw_send uw_accept uw_httpget uw_ping uw_process uw_traceroute

EXTRA_DIST = Makefile.am.common common upwatch-spec.def upwatch-spec.tpl \
  upwatch.spec config/upwatch.conf

upwatch.spec: upwatch-spec.def upwatch-spec.tpl touch.tch
	autogen -b upwatch upwatch-spec.def
	rm -f touch.tch

touch.tch:
	touch touch.tch
