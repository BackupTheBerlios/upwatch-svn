# Process this file with autoconf to produce a configure script.
AC_INIT(upwatch, 0.6, bugs@upwatch.com)
AC_CONFIG_SRCDIR(upwatch/spool.c)
VERSION=`cat VERSION`
PACKAGE=upwatch
AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
AM_CONFIG_HEADER(config.h:config-h.in)
AC_SUBST(VERSION)

ifdef([AC_REVISION],AC_REVISION($Revision: 1.25 $),)dnl

# ----------------------------------------------------------------------
# If `configure' is invoked (in)directly via `make', ensure that it
# encounters no `make' conflicts.  Ignore error if shell does not have
# unset, but at least set these to empty values.
# ----------------------------------------------------------------------
MFLAGS=
MAKEFLAGS=
MAKELEVEL=
unset MFLAGS MAKEFLAGS MAKELEVEL 2>/dev/null

ifdef([AC_LIBTOOL_WIN32_DLL], AC_LIBTOOL_WIN32_DLL)
AM_WITH_DMALLOC

# ----------------------------------------------------------------------
# check for various programs used during the build.
# ----------------------------------------------------------------------
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_EXEEXT
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_AWK

AC_PATH_PROG(RPMBUILD, rpmbuild)
if test -z "$RPMBUILD"
then
  AC_PATH_PROG(RPMBUILD, rpm)
fi
if test -z "$RPMBUILD"
then
  AC_PATH_PROG(RPMBUILD, echo)
fi
 
AC_ARG_ENABLE(debug,
    [  --enable-debug          Turn on compiler debugging information (default=no)],
    [if eval "test x$enable_debug = xyes"; then
        CFLAGS="-g -Wall ${LIBOPTS_CFLAGS}"   # note: discard previous CLAGS value
    else
        CFLAGS="${CFLAGS} -Wall -O2 ${LIBOPTS_CFLAGS}"
    fi]
)

AC_ARG_ENABLE(profile,
    [  --enable-profile        Turn on profiling flags (default=no)],
    [if eval "test x$enable_profile = xyes"; then
        CFLAGS="${CFLAGS} -pg "
    fi]
)

# Checks for libraries.
################## MYSQL
# path to libmysqlclient stuff:.
AC_ARG_WITH(mysql-libs,
    [  --with-mysql-libs=<path>           prefix of MySQL libraries. Default /usr/lib/mysql],
    [MYSQL_LIB_PREFIX=$with_mysql_libs],
    [MYSQL_LIB_PREFIX=/usr/lib/mysql]
)

# path to mysql includes
AC_ARG_WITH(mysql-include,
    [  --with-mysql-include=<path>        prefix of MySQL headers. Default /usr/include/mysql],
    [MYSQL_INC_PREFIX=$with_mysql_include],
    [MYSQL_INC_PREFIX=/usr/include/mysql]
)

AC_SUBST(MYSQL_LIB_PREFIX)
AC_SUBST(MYSQL_INC_PREFIX)
MYSQL_LIBS="-L${MYSQL_LIB_PREFIX} -lmysqlclient"
MYSQL_CFLAGS="-I${MYSQL_INC_PREFIX}"
AC_SUBST(MYSQL_LIBS)
AC_SUBST(MYSQL_CFLAGS)
if test ! -f "$MYSQL_INC_PREFIX/mysql.h"
then
  AC_MSG_ERROR([Cannot find $MYSQL_INC_PREFIX/mysql.h])
fi

################## PCAP
# path to pcap:
AC_ARG_WITH(pcap-libs,
    [  --with-pcap-libs=<path>            prefix of pcap libraries. Default /usr/lib],
    [PCAP_LIB_PREFIX=$with_pcap_libs],
    [PCAP_LIB_PREFIX=/usr/lib]
)

# path to pcap includes
AC_ARG_WITH(pcap-include,
    [  --with-pcap-include=<path>         prefix of pcap headers. Default /usr/include/pcap],
    [PCAP_INC_PREFIX=$with_pcap_include],
    [PCAP_INC_PREFIX=/usr/include/pcap]
)

AC_SUBST(PCAP_LIB_PREFIX)
AC_SUBST(PCAP_INC_PREFIX)
LIBPCAP_LIBS="-L${PCAP_LIB_PREFIX} -lpcap"
LIBPCAP_CFLAGS="-I${PCAP_INC_PREFIX}"
AC_SUBST(LIBPCAP_LIBS)
AC_SUBST(LIBPCAP_CFLAGS)
AM_CONDITIONAL(HAVE_PCAP, test -d $PCAP_INC_PREFIX/pcap.h)
if test -d $PCAP_INC_PREFIX/pcap.h
then
HAVE_PCAP=1
else
HAVE_PCAP=0
fi

################## LIBNET
# libnet
AC_CHECK_PROG([HAVE_LIBNET], [libnet-config], ["yes"], ["no"])
if test "$HAVE_LIBNET" = "yes"
then
  LIBNET_CFLAGS="`libnet-config --cflags`"
  LIBNET_CFLAGS="$LIBNET_CFLAGS `libnet-config --defines`"
  LIBNET_LIBS="`libnet-config --libs`"
fi
if test -z "$LIBNET_LIBS"
then
  HAVE_LIBNET="0"
else
  HAVE_LIBNET="1"
fi
AM_CONDITIONAL(HAVE_LIBNET, test $HAVE_LIBNET -eq 1)
AC_SUBST(LIBNET_CFLAGS)
AC_SUBST(LIBNET_LIBS)

################## GLIB2
# glib2 - with thread libraries
LIBGTHREAD2_CFLAGS="`pkg-config --cflags gthread-2.0`"
LIBGTHREAD2_LIBS="`pkg-config --libs gthread-2.0`"
if test -z "$LIBGTHREAD2_LIBS"
then
  HAVE_LIBGTHREAD2="0"
else
  HAVE_LIBGTHREAD2="1"
fi
AM_CONDITIONAL(HAVE_LIBGTHREAD2, test $HAVE_LIBGTHREAD2 -eq 1)
AC_SUBST(LIBGTHREAD2_CFLAGS)
AC_SUBST(LIBGTHREAD2_LIBS)

# glib2 - without thread libraries
LIBGLIB2_CFLAGS="`pkg-config --cflags glib-2.0`"
LIBGLIB2_LIBS="`pkg-config --libs glib-2.0`"
if test -z "$LIBGLIB2_LIBS"
then
  HAVE_LIBGLIB2="0"
else
  HAVE_LIBGLIB2="1"
fi
AM_CONDITIONAL(HAVE_LIBGLIB2, test $HAVE_LIBGLIB2 -eq 1)
AC_SUBST(LIBGLIB2_CFLAGS)
AC_SUBST(LIBGLIB2_CFLAGS)
AC_SUBST(LIBGLIB2_LIBS)

################## XML2
# libxml2 - always need XML libraries
LIBXML2_CFLAGS="`pkg-config --cflags libxml-2.0`"
LIBXML2_LIBS="`pkg-config --libs libxml-2.0`"
if test -z "$LIBXML2_LIBS"
then
  AC_MSG_ERROR([You really need libxml-2.0 to compile upwatch])
fi
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)

AC_CHECK_LIB([pthread], [pthread_create],
  [LIBPTHREAD_LIBS=-lpthread],[AC_MSG_ERROR([Sorry, we need -lpthread to continue])] )
AC_SUBST(LIBPTHREAD_LIBS)

AC_CHECK_LIB([pcap], [pcap_close], [HAVE_LIBPCAP=1], [HAVE_LIBPCAP=0])
if test $HAVE_LIBPCAP -eq 1
then
	LIBPCAP_LIBS=-lpcap
fi
AM_CONDITIONAL(HAVE_LIBPCAP, test $HAVE_LIBPCAP -eq 1)
AC_SUBST(LIBPCAP_LIBS)

AC_CHECK_LIB([gdbm], [gdbm_close], [HAVE_LIBGDBM=1], [HAVE_LIBGDBM=0])
if test $HAVE_LIBGDBM -eq 1
then
	LIBGDBM_LIBS=-lgdbm
fi
AM_CONDITIONAL(HAVE_LIBGDBM, test $HAVE_LIBGDBM -eq 1)
AC_SUBST(LIBGDBM_LIBS)

AC_CHECK_LIB([crypto], [SSLeay_version], [HAVE_LIBCRYPTO=1], [HAVE_LIBCRYPTO=0])
if test $HAVE_LIBCRYPTO -eq 1
then
	LIBCRYPTO_LIBS=-lcrypto
fi
AM_CONDITIONAL(HAVE_LIBCRYPTO, test $HAVE_LIBCRYPTO -eq 1)
AC_SUBST(LIBCRYPTO_LIBS)

AC_CHECK_LIB([snmp], [snmp_open], [HAVE_LIBSNMP=1], [HAVE_LIBSNMP=0], -lcrypto)
if test $HAVE_LIBSNMP -eq 1
then
	LIBSNMP_LIBS=-lsnmp
fi
AM_CONDITIONAL(HAVE_LIBSNMP, test $HAVE_LIBSNMP -eq 1)
AC_SUBST(LIBSNMP_LIBS)

AC_CHECK_LIB([tds], [tds_alloc_login], [HAVE_LIBTDS=1], [HAVE_LIBTDS=0])
if test $HAVE_LIBTDS -eq 1
then
	LIBTDS_LIBS=-ltds
fi
AM_CONDITIONAL(HAVE_LIBTDS, test $HAVE_LIBTDS -eq 1)
AC_SUBST(LIBTDS_LIBS)

AC_CHECK_LIB([pq], [PQclear], [HAVE_LIBPQ=1], [HAVE_LIBPQ=0])
if test $HAVE_LIBPQ -eq 1
then
	LIBPQ_LIBS=-lpq
fi
AM_CONDITIONAL(HAVE_LIBPQ, test $HAVE_LIBPQ -eq 1)
AC_SUBST(LIBPQ_LIBS)

#LIBOPTS_LDADD=-lopts
#AC_SUBST(LIBOPTS_LDADD)

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/fs/s5param.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
#LIBOPTS_CHECK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([alarm atexit dup2 gethostbyname gethostname gettimeofday inet_ntoa memset putenv regcomp select socket strchr strdup strerror strncasecmp strrchr strstr strtol strtoul tzset uname])


echo 
echo "*********************************************************"
echo "* CONFIGURATION:                                        *"
echo "*                                                       *"
echo "* using RPM program: $RPMBUILD                        *"

#if test $HAVE_LIBNET -eq 0
#then
#echo "* libnet not found, uw_investigate will not be built    *"
#else
#AC_CONFIG_FILES([uw_investigate/Makefile])
#fi
if test $HAVE_LIBPCAP -eq 0
then
echo "* libpcap not found, uw_iptraf will not be built        *"
else
AC_CONFIG_FILES([uw_iptraf/Makefile])
fi
if test $HAVE_LIBCRYPTO -eq 0
then
echo "* -lcrypto not found, some things will not be built     *"
fi
if test $HAVE_LIBSNMP -eq 0
then
echo "* snmp libs not found, no SNMP support                  *"
else
AC_CONFIG_FILES([uw_snmpget/Makefile])
fi
if test $HAVE_LIBTDS -eq 0
then
echo "* TDS lib not found, no MS-SQL support                  *"
else
AC_CONFIG_FILES([uw_mssql/Makefile])
fi
if test $HAVE_LIBPQ -eq 0
then
echo "* PostgreSQL not found, no PostgreSQL support           *"
else
AC_CONFIG_FILES([uw_postgresql/Makefile])
fi

echo "*********************************************************"
echo 

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 upwatch/Makefile
                 st-1.4/Makefile
                 util/Makefile
                 scripts/Makefile
                 uw_setip/Makefile
                 uw_accept/Makefile
                 uw_acceptbb/Makefile
                 uw_httpget/Makefile
                 uw_notify/Makefile
                 uw_ping/Makefile
                 uw_process/Makefile
                 uw_send/Makefile
                 uw_sysstat/Makefile
                 uw_mysql/Makefile
                 uw_pop3/Makefile
                 uw_imap/Makefile
                 compat/Makefile
                 compat/bb/Makefile
                 compat/bb/bbhimport/Makefile
                 compat/sometests/Makefile
                 compat/sometests/setproctitle/Makefile])

AC_OUTPUT
