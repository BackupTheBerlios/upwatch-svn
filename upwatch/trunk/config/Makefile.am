include $(top_srcdir)/Makefile.am.path

dtd_DATA = result.dtd
sysconf_DATA = upwatch.conf
init_DATA = upwatch.redhat upwatch.suse

result.dtd : result.dtd-base ../uw_*/probe.dtd ../uw_*/probe.dtd-inc
	cp result.dtd-base result.dtd
	cat ../uw_*/probe.dtd-inc | sort -u >> result.dtd
	echo  >> result.dtd
	cat ../uw_*/probe.dtd  >> result.dtd

EXTRA_DIST = upwatch.conf upwatch-base.mysql result.dtd logrotate \
  cron.daily upwatch.redhat upwatch.suse

