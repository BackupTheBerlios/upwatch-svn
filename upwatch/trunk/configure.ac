# Process this file with autoconf to produce a configure script.
AC_INIT(upwatch, 0.7, bugs@upwatch.com)
AC_CONFIG_SRCDIR(upwatch/spool.c)
VERSION=`cat VERSION`
PACKAGE=upwatch
#AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
AM_CONFIG_HEADER(config.h:config-h.in)
AC_SUBST(VERSION)

ifdef([AC_REVISION],AC_REVISION($Revision: 1.47 $),)dnl

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
# Set some global things based on os type
# ----------------------------------------------------------------------
AC_CANONICAL_HOST

case $host in
  *-*-linux*) 
    linux=true
    if test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr/bin' ; fi
    if test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/sbin' ; fi
    if test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/libexec' ; fi
    if test "$datadir" = '${prefix}/share' ;		then datadir='/usr/local/share' ; fi
    if test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/etc' ; fi
    if test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/lib' ; fi
    if test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/include' ; fi
    if test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/include' ; fi
    if test "$infodir" = '${prefix}/info' ; 		then infodir='/usr/share/info' ; fi
    if test "$mandir" = '${prefix}/man' ;		then mandir='/usr/share/man' ; fi
    ;;
  *-*-solaris*)
    solaris=true
    if test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr/bin' ; fi
    if test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/sbin' ; fi
    if test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/libexec' ; fi
    if test "$datadir" = '${prefix}/share' ;		then datadir='/usr/share' ; fi
    if test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/etc' ; fi
    if test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/local/lib' ; fi
    if test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/include' ; fi
    if test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/include' ; fi
    if test "$infodir" = '${prefix}/info';  		then infodir='/usr/share/info' ; fi
    if test "$mandir" = '${prefix}/man' ;		then mandir='/usr/share/man' ; fi
    LIBS="-lgen $LIBS"
    ;;
  *-*-cygwin*)
    cygwin=true
    if test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr/bin' ; fi
    if test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/sbin' ; fi
    if test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/libexec' ; fi
    if test "$datadir" = '${prefix}/share' ;		then datadir='/usr/share' ; fi
    if test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/etc' ; fi
    if test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/local/lib' ; fi
    if test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/include' ; fi
    if test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/include' ; fi
    if test "$infodir" = '${prefix}/info';  		then infodir='/usr/share/info' ; fi
    if test "$mandir" = '${prefix}/man' ;		then mandir='/usr/share/man' ; fi
    ;;
  *)
    generic=true
    ;;
esac

AM_CONDITIONAL([LINUX], test x$linux = xtrue)
AM_CONDITIONAL([SOLARIS], test x$solaris = xtrue)
AM_CONDITIONAL([CYGWIN], test x$cygwin = xtrue)

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

AC_PATH_PROG([ID], [id], [/usr/bin/id], [/usr/xpg4/bin:$PATH])
if test -z "$ID"
then
  AC_PATH_PROG(ID, echo)
fi

AC_PATH_PROG(JW, jw)
if test -z "$JW"
then
  AC_PATH_PROG(JW, echo)
fi

AC_PATH_PROG(XSLTPROC, xsltproc)
if test -z "$XSLTPROC"
then
  AC_PATH_PROG(XSLTPROC, echo)
fi

AC_PATH_PROG(LYNX, lynx)
if test -z "$LYNX"
then
  AC_PATH_PROG(LYNX, echo)
fi

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

if test x$enable_all = xyes
then
  enable_server=yes
  enable_monitors=yes
  enable_iptraf=yes
fi

AM_CONDITIONAL(ENABLE_SERVER, test x$enable_all = xyes)
AM_CONDITIONAL(ENABLE_MONITORS, test x$enable_all = xyes)
AM_CONDITIONAL(ENABLE_IPTRAF, test x$enable_all = xyes)

AC_ARG_ENABLE(server,
    [  --enable-server         Install server software (default=no)],
    [  ENABLE_SERVER=$enable_server ]
)
AM_CONDITIONAL(ENABLE_SERVER, test x$enable_server = xyes)

AC_ARG_ENABLE(iptraf,
    [  --enable-iptraf         Install iptraf daemon (default=no)],
)
AM_CONDITIONAL(ENABLE_IPTRAF, test x$enable_iptraf = xyes)

AC_ARG_ENABLE(monitors,
    [  --enable-monitors       Install monitor daemons (default=no)],
)
AM_CONDITIONAL(ENABLE_MONITORS, test x$enable_monitors = xyes)

AC_ARG_ENABLE(all,
    [  --enable-all            Install everything (default=no)],
)

# Checks for libraries.

################# First the autogen autoopts library
if test -f /usr/lib/libopts.a
then
  LIBOPTS=/usr/lib/libopts.a
fi

if test -f /usr/local/lib/libopts.a
then
  LIBOPTS=/usr/local/lib/libopts.a
fi
AC_SUBST(LIBOPTS)

################## MYSQL
# path to libmysqlclient stuff:.
AC_ARG_WITH(mysql-libs,
    [  --with-mysql-libs=<path>           prefix of MySQL libraries. Default /usr/lib/mysql],
    [MYSQL_LIB_PREFIX=$with_mysql_libs],
    [MYSQL_LIB_PREFIX=/usr/lib/mysql]
)
AC_SUBST(MYSQL_LIB_PREFIX)

for libpath in /usr/lib/mysql $MYSQL_LIB_PREFIX /usr/local/lib/mysql /usr/lib /cygdrive/c/mysql/lib/opt
do
  if test -f $libpath/libmysqlclient.so -o -f $libpath/mysqlclient.lib
  then
    MYSQL_LIBS="-L${libpath} -lmysqlclient"
  fi
done
if test -z "$MYSQL_LIBS"
then
  AC_MSG_ERROR([Cannot find MySQL libraries, tell me where they are with --with-mysql-libs=])
fi

# path to mysql includes
AC_ARG_WITH(mysql-include,
    [  --with-mysql-include=<path>        prefix of MySQL headers. Default /usr/include/mysql],
    [MYSQL_INC_PREFIX=$with_mysql_include],
    [MYSQL_INC_PREFIX=/usr/include/mysql]
)
AC_SUBST(MYSQL_INC_PREFIX)

for incpath in /usr/include/mysql $MYSQL_LIB_PREFIX /usr/local/include/mysql /cygdrive/c/mysql/include
do
  if test -f $incpath/mysql.h
  then
    MYSQL_CFLAGS="-I${incpath}"
  fi
done
if test -z "$MYSQL_CFLAGS"
then
  AC_MSG_ERROR([Cannot find MySQL include files, tell me where they are with --with-mysql-include=])
fi

AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)

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

# CURSES/NCURSES
AC_CHECK_LIB([curses], [initscr], [HAVE_LIBCURSES=1], [HAVE_LIBCURSES=0])
if test $HAVE_LIBCURSES -eq 1
then
        LIBCURSES_LIBS=-lcurses
fi
AC_CHECK_LIB([ncurses], [initscr], [HAVE_LIBNCURSES=1], [HAVE_LIBNCURSES=0])
if test $HAVE_LIBNCURSES -eq 1
then
        LIBCURSES_LIBS=-lncurses
fi
AM_CONDITIONAL(HAVE_CURSES, test ! -z $LIBCURSES_LIBS)
AC_SUBST(LIBCURSES_LIBS)

# NCURSES
AM_CONDITIONAL(HAVE_LIBNCURSES, test $HAVE_LIBNCURSES -eq 1)
AC_SUBST(LIBNCURSES_LIBS)

# PCAP
AC_CHECK_LIB([pcap], [pcap_close], [HAVE_LIBPCAP=1], [HAVE_LIBPCAP=0])
if test $HAVE_LIBPCAP -eq 1
then
	LIBPCAP_LIBS=-lpcap
fi
AM_CONDITIONAL(HAVE_LIBPCAP, test $HAVE_LIBPCAP -eq 1)
AC_SUBST(LIBPCAP_LIBS)

# GDBM
AC_CHECK_LIB([gdbm], [gdbm_close], [HAVE_LIBGDBM=1], [HAVE_LIBGDBM=0])
if test $HAVE_LIBGDBM -eq 1
then
	LIBGDBM_LIBS=-lgdbm
fi
AM_CONDITIONAL(HAVE_LIBGDBM, test $HAVE_LIBGDBM -eq 1)
AC_SUBST(LIBGDBM_LIBS)

# crypto (openSSL)
AC_CHECK_LIB([crypto], [SSLeay_version], [HAVE_LIBCRYPTO=1], [HAVE_LIBCRYPTO=0])
if test $HAVE_LIBCRYPTO -eq 1
then
	LIBCRYPTO_LIBS=-lcrypto
fi
AM_CONDITIONAL(HAVE_LIBCRYPTO, test $HAVE_LIBCRYPTO -eq 1)
AC_SUBST(LIBCRYPTO_LIBS)

# libesmtp
AC_CHECK_LIB([esmtp], [smtp_create_session], [HAVE_LIBESMTP=1], [HAVE_LIBESMTP=0])
if test $HAVE_LIBESMTP -eq 1
then
        LIBESMTP_LIBS=-lesmtp
fi
AM_CONDITIONAL(HAVE_LIBESMTP, test $HAVE_LIBESMTP -eq 1)
AC_SUBST(LIBESMTP_LIBS)

# snmp
AC_CHECK_LIB([snmp], [snprint_value], [HAVE_LIBSNMP=1], [HAVE_LIBSNMP=0], -lcrypto)
if test $HAVE_LIBSNMP -eq 1
then
	LIBSNMP_LIBS=-lsnmp
fi
AM_CONDITIONAL(HAVE_LIBSNMP, test $HAVE_LIBSNMP -eq 1)
AC_SUBST(LIBSNMP_LIBS)

# tds
AC_CHECK_LIB([tds], [tds_alloc_login], [HAVE_LIBTDS=1], [HAVE_LIBTDS=0])
if test $HAVE_LIBTDS -eq 1
then
	LIBTDS_LIBS=-ltds
fi
AM_CONDITIONAL(HAVE_LIBTDS, test $HAVE_LIBTDS -eq 1)
AC_SUBST(LIBTDS_LIBS)

# PostgreSQL
AC_CHECK_LIB([pq], [PQclear], [HAVE_LIBPQ=1], [HAVE_LIBPQ=0])
if test $HAVE_LIBPQ -eq 1
then
	LIBPQ_LIBS=-lpq
	LIBPQ_CFLAGS=""
	if test -f /usr/include/pgsql/libpq-fe.h
	then
		LIBPQ_CFLAGS=-I/usr/include/pgsql
	fi
fi
AM_CONDITIONAL(HAVE_LIBPQ, test $HAVE_LIBPQ -eq 1)
AC_SUBST(LIBPQ_LIBS)
AC_SUBST(LIBPQ_CFLAGS)

#LIBOPTS_LDADD=-lopts
#AC_SUBST(LIBOPTS_LDADD)

# Check for various functions
AC_CHECK_FUNCS([daemon, setsid])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/fs/s5param.h sys/ioctl.h sys/sockio.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h ncurses.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([u_int32_t, uint32_t])

# Checks for library functions.
#LIBOPTS_CHECK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MKTIME
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([alarm atexit dup2 gethostbyname gethostname gettimeofday inet_ntoa memset putenv regcomp select socket strchr strdup strerror strncasecmp strrchr strstr strtol strtoul tzset uname])
AC_CHECK_LIB(mysqlclient, mysql_real_escape_string, AC_DEFINE(HAVE_MYSQL_REAL_ESCAPE,1,[Does mysql have real_escape]),,[$MYSQL_LIBS])

echo 
echo "*********************************************************"
echo "* CONFIGURATION:"
echo "*"
echo "* using RPM program: $RPMBUILD"

if test "$enable_iptraf" = "yes"
then
  if test $HAVE_LIBPCAP -eq 0
  then
    echo "* libpcap not found, uw_iptraf will not be built"
  fi
fi

if test "$enable_monitors" = "yes"
then
  if test $HAVE_LIBSNMP -eq 0
  then
    echo "* snmp libs not found, no SNMP support"
  fi

  if test $HAVE_LIBTDS -eq 0
  then
    echo "* TDS lib not found, no MS-SQL support"
  fi

  if test $HAVE_LIBPQ -eq 0
  then
    echo "* PostgreSQL not found, no PostgreSQL support"
  fi
fi

echo "*********************************************************"
echo 

# base
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([config/Makefile])
AC_CONFIG_FILES([common/Makefile])
AC_CONFIG_FILES([st-1.4/Makefile])
AC_CONFIG_FILES([upwatch/Makefile])
AC_CONFIG_FILES([util/Makefile])
AC_CONFIG_FILES([scripts/Makefile])
AC_CONFIG_FILES([uw_send/Makefile])
AC_CONFIG_FILES([uw_sysstat/Makefile])
AC_CONFIG_FILES([uw_null/Makefile])
AC_CONFIG_FILES([compat/Makefile])
AC_CONFIG_FILES([compat/sometests/Makefile])
AC_CONFIG_FILES([compat/sometests/setproctitle/Makefile])

# server 
AC_CONFIG_FILES([uw_setip/Makefile])
AC_CONFIG_FILES([uw_accept/Makefile])
AC_CONFIG_FILES([uw_acceptbb/Makefile])
AC_CONFIG_FILES([uw_route/Makefile])
AC_CONFIG_FILES([uw_process/Makefile])
AC_CONFIG_FILES([compat/bb/Makefile])
AC_CONFIG_FILES([compat/bb/bbhimport/Makefile])

# monitors
AC_CONFIG_FILES([uw_httpget/Makefile])
AC_CONFIG_FILES([uw_ping/Makefile])
AC_CONFIG_FILES([uw_mysql/Makefile])
AC_CONFIG_FILES([uw_pop3/Makefile])
AC_CONFIG_FILES([uw_imap/Makefile])
AC_CONFIG_FILES([uw_snmpget/Makefile])
AC_CONFIG_FILES([uw_mssql/Makefile])
AC_CONFIG_FILES([uw_postgresql/Makefile])

# extra
AC_CONFIG_FILES([uw_iptraf/Makefile])

cp patches/libstatgrab-configure.gnu libstatgrab/configure.gnu
AC_CONFIG_SUBDIRS(libstatgrab)

AC_OUTPUT
