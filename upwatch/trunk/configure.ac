# Process this file with autoconf to produce a configure script.
AC_INIT(upwatch, 0.4, bugs@upwatch.com)
AC_CONFIG_SRCDIR(upwatch/spool.c)
VERSION=`cat VERSION`
PACKAGE=upwatch
AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
AM_CONFIG_HEADER(config.h:config-h.in)
AC_SUBST(VERSION)

ifdef([AC_REVISION],AC_REVISION($Revision: 1.15 $),)dnl

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

 
AC_ARG_ENABLE(debug,
    [  --enable-debug          Turn on compiler debugging information (default=no)],
    [if eval "test x$enable_debug = xyes"; then
        CFLAGS="-g -Wall ${LIBOPTS_CFLAGS}"   # note: discard previous CLAGS value
    else
        CFLAGS="${CFLAGS} -Wall -O2 ${LIBOPTS_CFLAGS}"
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

CFLAGS="$CFLAGS $MYSQL_CFLAGS"

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
PCAP_LIBS="-L${PCAP_LIB_PREFIX} -lpcap"
PCAP_CFLAGS="-I${PCAP_INC_PREFIX}"
AC_SUBST(PCAP_LIBS)
AC_SUBST(PCAP_CFLAGS)

CFLAGS="$CFLAGS $PCAP_CFLAGS"

################## LIBNET
# libnet
LIBNET_CFLAGS="`libnet-config --cflags`"
LIBNET_CFLAGS="$LIBNET_CFLAGS `libnet-config --defines`"
LIBNET_LIBS="`libnet-config --libs`"
if test -z "$LIBNET_LIBS"
then
  AC_MSG_ERROR([You really need libnet to compile upwatch])
fi
AC_SUBST(LIBNET_CFLAGS)
AC_SUBST(LIBNET_LIBS)
CFLAGS="$CFLAGS $LIBNET_CFLAGS"

################## GLIB2
# glib2 - with thread libraries
LIBGTHREAD2_CFLAGS="`pkg-config --cflags gthread-2.0`"
LIBGTHREAD2_LIBS="`pkg-config --libs gthread-2.0`"
if test -z "$LIBGTHREAD2_LIBS"
then
  AC_MSG_ERROR([You really need libglib2-devel to compile upwatch])
fi
AC_SUBST(LIBGTHREAD2_CFLAGS)
AC_SUBST(LIBGTHREAD2_LIBS)
CFLAGS="$CFLAGS $LIBGTHREAD2_CFLAGS"

# glib2 - without thread libraries
LIBGLIB2_CFLAGS="`pkg-config --cflags glib-2.0`"
LIBGLIB2_LIBS="`pkg-config --libs glib-2.0`"
if test -z "$LIBGLIB2_LIBS"
then
  AC_MSG_ERROR([You really need libglib2-devel to compile upwatch])
fi
AC_SUBST(LIBGLIB2_CFLAGS)
AC_SUBST(LIBGLIB2_LIBS)
CFLAGS="$CFLAGS $LIBGLIB2_CFLAGS"

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
CFLAGS="$CFLAGS $LIBXML2_CFLAGS"

AC_CHECK_LIB([pthread], [pthread_create],
  [LIBPTHREAD_LIBS=-lpthread],[AC_MSG_ERROR([Sorry, we need -lpthread to continue])] )
AC_SUBST(LIBPTHREAD_LIBS)

AC_CHECK_LIB([curl], [curl_easy_init],
  [LIBCURL_LIBS=-lcurl], [AC_MSG_ERROR([Sorry, we need -lcurl to continue])])
AC_SUBST(LIBCURL_LIBS)

AC_CHECK_LIB([pcap], [pcap_close],
  [LIBPCAP_LIBS=-lpcap], [AC_MSG_ERROR([Sorry, we need -lpcap to continue])])
AC_SUBST(LIBPCAP_LIBS)

AC_CHECK_LIB([gdbm], [gdbm_close],
  [LIBGDBM_LIBS=-lgdbm], [AC_MSG_ERROR([Sorry, we need -lgdbm to continue])])
AC_SUBST(LIBGDBM_LIBS)

AC_CHECK_LIB([crypto], [SSLeay_version],
  [LIBCRYPTO_LIBS=-lcrypto], [AC_MSG_ERROR([Sorry, we need -lcrypto to continue])])
AC_SUBST(LIBCRYPTO_LIBS)

AC_CHECK_LIB([snmp], [snmp_open],
  [LIBSNMP_LIBS=-lsnmp], [AC_MSG_ERROR([Sorry, we need -lsnmp to continue])], [-lcrypto])
AC_SUBST(LIBSNMP_LIBS)

AC_CHECK_LIB([tds], [tds_alloc_login],
  [LIBTDS_LIBS=-ltds], [AC_MSG_ERROR([Sorry, we need -ltds to continue])])
AC_SUBST(LIBTDS_LIBS)

AC_CHECK_LIB([pq], [PQclear],
  [LIBPQ_LIBS=-lpq], [AC_MSG_ERROR([Sorry, we need -lpq to continue])])
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

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 upwatch/Makefile
                 st-1.4/Makefile
                 util/Makefile
                 uw_accept/Makefile
                 uw_acceptbb/Makefile
                 uw_httpget/Makefile
                 uw_investigate/Makefile
                 uw_iptraf/Makefile
                 uw_notify/Makefile
                 uw_ping/Makefile
                 uw_process/Makefile
                 uw_send/Makefile
                 uw_sysstat/Makefile
                 uw_mysql/Makefile
                 uw_postgresql/Makefile
                 uw_pop3/Makefile
                 uw_snmpget/Makefile
                 uw_mssql/Makefile
                 compat/Makefile
                 compat/bb/Makefile
                 compat/bb/bbhimport/Makefile
                 compat/sometests/Makefile
                 compat/sometests/setproctitle/Makefile])

AC_OUTPUT
