include $(top_srcdir)/Makefile.am.path

dtd_DATA = result.dtd
sysconf_DATA = upwatch.conf
init_DATA = upwatch.redhat upwatch.suse

result.dtd : result.dtd-base ../uw_*/probe.dtd ../uw_*/probe.dtd-inc
	cp result.dtd-base result.dtd
	cat ../uw_*/probe.dtd-inc | sort -u >> result.dtd
	echo  >> result.dtd
	cat ../uw_*/probe.dtd  >> result.dtd

EXTRA_DIST = upwatch.conf upwatch-common.mysql upwatch-realm.mysql upwatch-full.mysql \
  upwatch-common.pgsql upwatch-realm.pgsql upwatch-full.pgsql \
  result.dtd-base logrotate sysconfig cron.daily upwatch.redhat upwatch.suse

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 775 -o upwatch -g upwatch ${DESTDIR}${sysconfdir}/sysconfig ;\
	  ${INSTALL} -m 644 -o upwatch -g upwatch sysconfig ${DESTDIR}${sysconfdir}/sysconfig/upwatch ; \
	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${upwatchdir}/uw_sysstat.d ;\
	  cp -r uw_sysstat.d/* ${DESTDIR}${confdir}/uw_sysstat.d ; \
	  find ${DESTDIR}${confdir}/uw_sysstat.d -type d -name .svn -o -name CVS | xargs rm -rf ; \
	  chown -R upwatch:upwatch ${DESTDIR}${confdir}/uw_sysstat.d ; \
	else \
	  mkdir -p ${DESTDIR}${sysconfdir}/sysconfig; \
	  ${INSTALL} -m 644 sysconfig ${DESTDIR}${sysconfdir}/sysconfig/upwatch ; \
	  mkdir -p ${DESTDIR}${confdir}/uw_sysstat.d; \
	  cp -r uw_sysstat.d/* ${DESTDIR}${confdir}/uw_sysstat.d ;\
	  find ${DESTDIR}${confdir}/uw_sysstat.d -type d -name .svn -o -name CVS | xargs rm -rf ; \
	fi

