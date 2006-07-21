include $(top_srcdir)/Makefile.am.path

syslogdir = $(confdir)/uw_sysstat.d/syslog

if LINUX
KERNEL = linux.kernel
endif

if SOLARIS
KERNEL = solaris.kernel
endif

if FREEBSD
KERNEL = freebsd.kernel
endif

syslog_DATA = $(KERNEL)

EXTRA_DIST = linux.kernel solaris.kernel

