include $(top_srcdir)/Makefile.am.path

dtd_DATA = result.dtd
sysconf_DATA = upwatch.conf
init_DATA = upwatch.redhat upwatch.suse

result.dtd : result.dtd-base ../uw_*/probe.dtd ../uw_*/probe.dtd-inc
	cp result.dtd-base result.dtd
	cat ../uw_*/probe.dtd-inc | sort -u >> result.dtd
	echo  >> result.dtd
	cat ../uw_*/probe.dtd  >> result.dtd

EXTRA_DIST = upwatch.conf upwatch-base.mysql upwatch-full.mysql result.dtd-base logrotate sysconfig \
  cron.daily upwatch.redhat upwatch.suse

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 775 -o upwatch -g upwatch ${DESTDIR}${sysconfdir}/sysconfig ;\
	  ${INSTALL} -m 644 -o upwatch -g upwatch sysconfig ${DESTDIR}${sysconfdir}/sysconfig/upwatch ; \
	else \
	  mkdir ${DESTDIR}${sysconfdir}/sysconfig; \
	  ${INSTALL} -m 644 sysconfig ${DESTDIR}${sysconfdir}/sysconfig/upwatch ; \
	fi

