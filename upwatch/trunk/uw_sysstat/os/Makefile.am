include $(top_srcdir)/Makefile.am.path

syslogdir = $(confdir)/uw_sysstat.d/syslog

if LINUX
KERNEL = linux
endif

if SOLARIS
KERNEL = solaris
endif

if FREEBSD
KERNEL = freebsd
endif

syslog_DATA = $(KERNEL)

EXTRA_DIST = linux solaris 

#install-data-local:
#	if test `${ID} -u` -eq 0; then \
#	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${confdir}/uw_sysstat.d ;\
#	  ${INSTALL} -d -m 770  -o upwatch -g upwatch ${DESTDIR}${confdir}/uw_sysstat.d/syslog ;\
#	  cp -r ${KERNEL} ${DESTDIR}${confdir}/uw_sysstat.d/syslog/kernel ; \
#	  chown upwatch:upwatch ${DESTDIR}${confdir}/uw_sysstat.d/syslog/kernel ; \
#	else \
#	  mkdir -p ${DESTDIR}${confdir}/uw_sysstat.d/syslog ;\
#	  cp -r ${KERNEL} ${DESTDIR}${confdir}/uw_sysstat.d/syslog ;\
#	fi
