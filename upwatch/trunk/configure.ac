# Process this file with autoconf to produce a configure script.
AC_INIT(upwatch, 0.98, bugs@upwatch.com)
AC_CONFIG_SRCDIR(upwatch/spool.c)
VERSION=`cat VERSION`
RELEASE=`cat RELEASE`
PACKAGE=upwatch
AM_INIT_AUTOMAKE($PACKAGE,$VERSION)
AM_CONFIG_HEADER(config.h:config-h.in)
AC_SUBST(VERSION)
AM_PROG_AS

AC_DEFINE_UNQUOTED([RELEASE], ["${RELEASE}"], [Package release string])

ifdef([AC_REVISION],AC_REVISION($Revision: 1.76 $),)dnl

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

# do this early because we totally override CFLAGS here
AC_ARG_ENABLE(debug,
    [  --enable-debug          Turn on compiler debugging information (default=no)],
    [if eval "test x$enable_debug = xyes"; then
        CFLAGS="-g -Wall"  # override
    else
        CFLAGS="${CFLAGS} -Wall -O2"
    fi]
)

# ----------------------------------------------------------------------
# Set some global things based on os type
# ----------------------------------------------------------------------
AC_CANONICAL_HOST
AC_CANONICAL_BUILD

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
    LDFLAGS="${LDFLAGS} -L/usr/lib64"
    ;;
  *-*-freebsd*) 
    freebsd=true
    if ! test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr//local/bin' ; fi
    if ! test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/local/sbin' ; fi
    if ! test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/local/libexec' ; fi
    if ! test "$datadir" = '${prefix}/share' ;		then datadir='/usr/local/share' ; fi
    if ! test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/usr/local/etc' ; fi
    if ! test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if ! test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if ! test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/local/lib' ; fi
    if ! test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/local/include' ; fi
    if ! test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/local/include' ; fi
    if ! test "$infodir" = '${prefix}/info' ; 		then infodir='/usr/local/share/info' ; fi
    if ! test "$mandir" = '${prefix}/man' ;		then mandir='/usr/local/share/man' ; fi
    CFLAGS="${CFLAGS} -I/usr/local/include"
    LDFLAGS="-L/usr/local/lib"
    ;;
  *-*-netbsd*) 
    netbsd=true
    if ! test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr/local/bin' ; fi
    if ! test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/local/sbin' ; fi
    if ! test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/local/libexec' ; fi
    if ! test "$datadir" = '${prefix}/share' ;		then datadir='/usr/local/share' ; fi
    if ! test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/usr/local/etc' ; fi
    if ! test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if ! test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if ! test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/local/lib' ; fi
    if ! test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/local/include' ; fi
    if ! test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/local/include' ; fi
    if ! test "$infodir" = '${prefix}/info' ; 		then infodir='/usr/local/share/info' ; fi
    if ! test "$mandir" = '${prefix}/man' ;		then mandir='/usr/local/share/man' ; fi
    CFLAGS="${CFLAGS} -I/usr/local/include -I/usr/pkg/include"
    LDFLAGS="-L/usr/local/lib -L/usr/pkg/lib"
    ;;
  *-*-openbsd*)
    openbsd=true
    if ! test "$bindir" = '${exec_prefix}/bin' ;          then bindir='/usr/local/bin' ; fi
    if ! test "$sbindir" = '${exec_prefix}/sbin' ;        then sbindir='/usr/local/sbin' ; fi
    if ! test "$libexecdir" = '${exec_prefix}/libexec';   then libexecdir='/usr/local/libexec' ; fi
    if ! test "$datadir" = '${prefix}/share' ;            then datadir='/usr/local/share' ; fi
    if ! test "$sysconfdir" = '${prefix}/etc' ;           then sysconfdir='/etc' ; fi
    if ! test "$sharedstatedir" = '${prefix}/com' ;       then sharedstatedir='/usr/local/com' ; fi
    if ! test "$localstatedir" = '${prefix}/var' ;        then localstatedir='/var' ; fi
    if ! test "$libdir" = '${exec_prefix}/lib' ;          then libdir='/usr/local/lib' ; fi
    if ! test "$includedir" = '${prefix}/include' ;       then includedir='/usr/local/include' ; fi
    if ! test "$oldincludedir" = '/usr/include' ;         then oldincludedir='/usr/local/include' ; fi
    if ! test "$infodir" = '${prefix}/info' ;             then infodir='/usr/local/share/info' ; fi
    if ! test "$mandir" = '${prefix}/man' ;               then mandir='/usr/local/share/man' ; fi
    CFLAGS="${CFLAGS} -I/usr/local/include"
    LDFLAGS="-L/usr/local/lib"
    ;;
  *-*-solaris*)
    solaris=true
    if ! test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr/local/bin' ; fi
    if ! test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/local/sbin' ; fi
    if ! test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/local/libexec' ; fi
    if ! test "$datadir" = '${prefix}/share' ;		then datadir='/usr/local/share' ; fi
    if ! test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/etc' ; fi
    if ! test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if ! test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if ! test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/local/lib' ; fi
    if ! test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/local/include' ; fi
    if ! test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/local/include' ; fi
    if ! test "$infodir" = '${prefix}/info';  		then infodir='/usr/local/share/info' ; fi
    if ! test "$mandir" = '${prefix}/man' ;		then mandir='/usr/local/share/man' ; fi
    LIBS="-lgen ${LIBS}"
    ;;
  *-*-cygwin*)
    cygwin=true
    if ! test "$bindir" = '${exec_prefix}/bin' ; 		then bindir='/usr/bin' ; fi
    if ! test "$sbindir" = '${exec_prefix}/sbin' ; 	then sbindir='/usr/sbin' ; fi
    if ! test "$libexecdir" = '${exec_prefix}/libexec';	then libexecdir='/usr/libexec' ; fi
    if ! test "$datadir" = '${prefix}/share' ;		then datadir='/usr/share' ; fi
    if ! test "$sysconfdir" = '${prefix}/etc' ;		then sysconfdir='/etc' ; fi
    if ! test "$sharedstatedir" = '${prefix}/com' ;	then sharedstatedir='/usr/local/com' ; fi
    if ! test "$localstatedir" = '${prefix}/var' ; 	then localstatedir='/var' ; fi
    if ! test "$libdir" = '${exec_prefix}/lib' ; 		then libdir='/usr/local/lib' ; fi
    if ! test "$includedir" = '${prefix}/include' ; 	then includedir='/usr/include' ; fi
    if ! test "$oldincludedir" = '/usr/include' ; 	then oldincludedir='/usr/include' ; fi
    if ! test "$infodir" = '${prefix}/info';  		then infodir='/usr/share/info' ; fi
    if ! test "$mandir" = '${prefix}/man' ;		then mandir='/usr/share/man' ; fi
    ;;
  *)
    generic=true
    ;;
esac

AM_CONDITIONAL(LINUX, test x$linux = xtrue)
AM_CONDITIONAL(FREEBSD, test x$freebsd = xtrue)
AM_CONDITIONAL(NETBSD, test x$netbsd = xtrue)
AM_CONDITIONAL(OPENBSD, test x$openbsd = xtrue)
AM_CONDITIONAL(SOLARIS, test x$solaris = xtrue)
AM_CONDITIONAL(CYGWIN, test x$cygwin = xtrue)

case $host in
  i?86-*-linux* | i?86-*-freebsd*) 
  enable_xmbmon=yes
  ;;
esac
AM_CONDITIONAL(XMBMON, test x$enable_xmbmon = xyes)

# ----------------------------------------------------------------------
# check for various programs used during the build.
# ----------------------------------------------------------------------
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_EXEEXT
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_SYS_LARGEFILE
export SED=sed

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
  case $host in
    *-*-linux*)
    enable_iptraf=yes
    ;;
  esac
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
AC_CHECK_PROG([HAVE_LIBOPTS], [libopts-config], ["yes"], ["no"])
if test "$HAVE_LIBOPTS" = "yes"
then
  LIBOPTS_CFLAGS="`libopts-config --cflags`"
  LIBOPTS_LIBS="`libopts-config --libs`"
fi

if test -z "$LIBOPTS_LIBS"
then
  for libpath in /usr/lib /usr/local/lib /usr/lib/lib64 /usr/local/lib64 /usr/pkg/lib
  do
    if test -f $libpath/libopts.a
    then
      LIBOPTS_LIBS="-L$libpath -lopts"
    fi
  done
fi

if test -z "$LIBOPTS_CFLAGS"
then
  # path to autoopts includes
  AC_ARG_WITH(autoopts-include,
      [  --with-autoopts-include=<path>        prefix of autoopts headers. Default /usr/include/],
      [LIBOPTS_INC_PREFIX=$with_autoopts_include],
      [LIBOPTS_INC_PREFIX=/usr/include/]
  )
  AC_SUBST(LIBOPTS_INC_PREFIX)

  for incpath in /usr/include $LIBOPTS_INC_PREFIX /usr/local/include /cygdrive/c/include /usr/pkg/include
  do
    if test -f $incpath/autoopts/options.h
    then
      LIBOPTS_CFLAGS="-I${incpath}"
    fi
    if test -f $incpath/autogen/autoopts/options.h
    then
      LIBOPTS_CFLAGS="-I${incpath}/autogen"
    fi
  done
fi
if test -z "$LIBOPTS_LIBS" -o -z "$LIBOPTS_CFLAGS"
then
  AC_MSG_ERROR([You really need autogen or libopts to compile upwatch])
fi
AC_SUBST(LIBOPTS_LIBS)
AC_SUBST(LIBOPTS_CFLAGS)

################## MYSQL
# path to libmysqlclient stuff:.
AC_ARG_WITH(mysql-libs,
    [  --with-mysql-libs=<path>           prefix of MySQL libraries. Default /usr/lib/mysql],
    [MYSQL_LIB_PREFIX=$with_mysql_libs],
    [MYSQL_LIB_PREFIX=/usr/lib/mysql]
)
AC_SUBST(MYSQL_LIB_PREFIX)

for libpath in /usr/lib/mysql $MYSQL_LIB_PREFIX /usr/local/lib/mysql /usr/lib /cygdrive/c/mysql/lib/opt /usr/pkg/lib/mysql /usr/lib64/mysql
do
  for file in $libpath/libmysqlclient.so* $libpath/mysqlclient.lib
  do
    if test -f $file
    then
      MYSQL_LIBS="-L${libpath} -lmysqlclient"
      break
    fi
  done
done

# path to mysql includes
AC_ARG_WITH(mysql-include,
    [  --with-mysql-include=<path>        prefix of MySQL headers. Default /usr/include/mysql],
    [MYSQL_INC_PREFIX=$with_mysql_include],
    [MYSQL_INC_PREFIX=/usr/include/mysql]
)
AC_SUBST(MYSQL_INC_PREFIX)

for incpath in /usr/include/mysql $MYSQL_LIB_PREFIX /usr/local/include/mysql /cygdrive/c/mysql/include /usr/pkg/include/mysql
do
  if test -f $incpath/mysql.h
  then
    MYSQL_CFLAGS="-I${incpath}"
  fi
done

if test "$enable_monitors" = "yes" -o "$enable_server" = "yes"
then
  if test -z "$MYSQL_LIBS"
  then
    AC_MSG_ERROR([Cannot find MySQL libraries, tell me where they are with --with-mysql-libs=])
  fi
  if test -z "$MYSQL_CFLAGS"
  then
    AC_MSG_ERROR([Cannot find MySQL include files, tell me where they are with --with-mysql-include=])
  fi
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

################## LIBPCRE
# libpcre
AC_CHECK_PROG([HAVE_LIBPCRE], [pcre-config], ["yes"], ["no"])
if test "$HAVE_LIBPCRE" = "yes"
then
  LIBPCRE_CFLAGS="`pcre-config --cflags`"
  LIBPCRE_LIBS="`pcre-config --libs-posix`"
fi
if test -z "$LIBPCRE_LIBS"
then
  HAVE_LIBPCRE="0"
else
  HAVE_LIBPCRE="1"
  AC_DEFINE(HAVE_LIBPCRE,1,[Do we have the perl REGEX library])
fi
AM_CONDITIONAL(HAVE_LIBPCRE, test $HAVE_LIBPCRE -eq 1)
AC_SUBST(LIBPCRE_CFLAGS)
AC_SUBST(LIBPCRE_LIBS)

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

# STATE THREADS
#AC_CHECK_LIB(st, st_init, HAS_ST=yes, HAS_ST=no)
#if test ${HAS_ST} = yes ; then
#  LIBSTLIBS=-lst
#else
#  AC_MSG_ERROR([Sorry, we really need libst to continue])
#fi
#AC_SUBST(LIBST_CFLAGS)
#AC_SUBST(LIBST_LIBS)

# THREADS
AC_CHECK_LIB(pthread, pthread_create, HAS_PTHREADS=yes, HAS_PTHREADS=no)
if test ${HAS_PTHREADS} = yes ; then
  LIBPTHREAD_LIBS=-lpthread
else
  # FreeBSD checks
  AC_CHECK_LIB(c_r, pthread_create, HAS_PTHREADS=yes, HAS_PTHREADS=no)
  if test ${HAS_PTHREADS} = yes ; then
    LIBPTHREAD_LIBS=-pthread
    LIBPTHREAD_CFLAGS="-D_REENTRANT -pthread"
  fi
fi

if test ${HAS_PTHREADS} = no ; then
  AC_MSG_ERROR([Sorry, we need a thread library to continue])
fi

AC_SUBST(LIBPTHREAD_LIBS)
AC_SUBST(LIBPTHREAD_CFLAGS)

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

# snmp
AC_CHECK_LIB([snmp], [snmp_errno], [HAVE_LIBSNMP=1], [HAVE_LIBSNMP=0], -lcrypto)
if test $HAVE_LIBSNMP -eq 1
then
	LIBSNMP_LIBS=-lsnmp
else
	AC_CHECK_LIB([netsnmp], [snmp_errno], [HAVE_LIBSNMP=1], [HAVE_LIBSNMP=0], -lcrypto)
	if test $HAVE_LIBSNMP -eq 1
	then
		LIBSNMP_LIBS=-lnetsnmp
	fi
fi
AM_CONDITIONAL(HAVE_LIBSNMP, test $HAVE_LIBSNMP -eq 1)
AC_SUBST(LIBSNMP_LIBS)

if test "$enable_server" = "yes"
then
	# libesmtp
	AC_CHECK_LIB([esmtp], [smtp_create_session], [HAVE_LIBESMTP=1], [HAVE_LIBESMTP=0], $LIBPTHREAD_LIBS)
	if test $HAVE_LIBESMTP -eq 1
	then
	        LIBESMTP_LIBS=-lesmtp
	else
		AC_MSG_ERROR([You really need libesmtp to compile upwatch server])
	fi
	AC_SUBST(LIBESMTP_LIBS)

	# libgnokii
	AC_CHECK_LIB([gnokii], [gn_cfg_get], [HAVE_LIBGNOKII=1], [HAVE_LIBGNOKII=0])
	if test $HAVE_LIBGNOKII -eq 1
	then
	        LIBGNOKII_LIBS=-lgnokii
	fi
	AC_SUBST(LIBGNOKII_LIBS)
fi
AM_CONDITIONAL(HAVE_LIBESMTP, test "$HAVE_LIBESMTP" = "1")
AM_CONDITIONAL(HAVE_LIBGNOKII, test "$HAVE_LIBGNOKII" = "1")

# readline
AC_CHECK_LIB([readline], [readline], [HAVE_LIBREADLINE=1], [HAVE_LIBREADLINE=0], -lncurses)
if test $HAVE_LIBREADLINE -eq 1
then
	LIBREADLINE_LIBS=-lreadline
        AC_DEFINE(HAVE_LIBREADLINE,1,[Do we have the readline library])
else 
	AC_CHECK_LIB([readline], [readline], [HAVE_LIBREADLINE=1], [HAVE_LIBREADLINE=0], -lcurses)
	if test $HAVE_LIBREADLINE -eq 1
	then
		LIBREADLINE_LIBS=-lreadline
                AC_DEFINE(HAVE_LIBREADLINE,1,[Do we have the readline library])
	fi
fi
AM_CONDITIONAL(HAVE_LIBREADLINE, test $HAVE_LIBREADLINE -eq 1)
AC_SUBST(LIBREADLINE_LIBS)

# tds
AC_CHECK_LIB([tds], [tds_alloc_login], [HAVE_LIBTDS=1], [HAVE_LIBTDS=0])
if test $HAVE_LIBTDS -eq 1
then
	LIBTDS_LIBS=-ltds
	if [ -f /usr/include/tdsver.h ]
	then
		LIBTDS_INCLUDE=/usr/include
	fi
	if [ -f /usr/include/freetds/tdsver.h ]
	then
		LIBTDS_INCLUDE=/usr/include/freetds
	fi
	case `grep TDS_VERSION_NO $LIBTDS_INCLUDE/tdsver.h` in
	*0.64*)
		LIBTDS_CFLAGS="-DFREETDS_0_64"
	;;
	*0.63*)
		LIBTDS_CFLAGS="-DFREETDS_0_63"
	;;
	*0.62*)
		LIBTDS_CFLAGS="-DFREETDS_0_62"
	;;
	*)
		LIBTDS_CFLAGS="-DFREETDS_PRE_0_62"
	;;
	esac
        LIBTDS_CFLAGS="$LIBTDS_CFLAGS -I$LIBTDS_INCLUDE"
fi
AM_CONDITIONAL(HAVE_LIBTDS, test $HAVE_LIBTDS -eq 1)
AC_SUBST(LIBTDS_LIBS)
AC_SUBST(LIBTDS_CFLAGS)

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
	if test -f /usr/include/postgresql/libpq-fe.h
	then
		LIBPQ_CFLAGS=-I/usr/include/postgresql
	fi
	if test -f /usr/local/include/postgresql/libpq-fe.h
	then
		LIBPQ_CFLAGS=-I/usr/local/include/postgresql
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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h memory.h netdb.h netinet/in.h stdlib.h string.h strings.h sys/fs/s5param.h sys/ioctl.h sys/sockio.h sys/param.h sys/socket.h sys/time.h netinet/ip_var.h syslog.h unistd.h ncurses.h])

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
    echo "* snmp libs (or openssl libs) not found, no SNMP support"
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
sleep 1

# base
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([config/Makefile])
AC_CONFIG_FILES([common/Makefile])
AC_CONFIG_FILES([upwatch/Makefile])
AC_CONFIG_FILES([util/Makefile])
AC_CONFIG_FILES([uw_send/Makefile])
AC_CONFIG_FILES([uw_sysstat/Makefile])
AC_CONFIG_FILES([uw_sysstat/os/Makefile])
AC_CONFIG_FILES([uw_null/Makefile])
AC_CONFIG_FILES([scripts/Makefile])

if test "$enable_server" = "yes" -o "$enable_monitors" = "yes"
then
AC_CONFIG_FILES([st-1.7/Makefile])
AC_CONFIG_FILES([uw_httpget/Makefile])
AC_CONFIG_FILES([uw_dns/Makefile])
AC_CONFIG_FILES([uw_ping/Makefile])
AC_CONFIG_FILES([uw_mysql/Makefile])
AC_CONFIG_FILES([uw_mysqlstats/Makefile])
AC_CONFIG_FILES([uw_pop3/Makefile])
AC_CONFIG_FILES([uw_smtp/Makefile])
AC_CONFIG_FILES([uw_tcpconnect/Makefile])
AC_CONFIG_FILES([uw_imap/Makefile])
AC_CONFIG_FILES([uw_snmpget/Makefile])
AC_CONFIG_FILES([uw_mssql/Makefile])
AC_CONFIG_FILES([uw_postgresql/Makefile])
fi

if test "$enable_server" = "yes" -o "$enable_iptraf" = "yes"
then
AC_CONFIG_FILES([uw_iptraf/Makefile])
fi

# server 
if test "$enable_server" = "yes"
then
AC_CONFIG_FILES([uw_setip/Makefile])
AC_CONFIG_FILES([uw_accept/Makefile])
AC_CONFIG_FILES([uw_acceptbb/Makefile])
AC_CONFIG_FILES([uw_purple/Makefile])
AC_CONFIG_FILES([uw_route/Makefile])
AC_CONFIG_FILES([uw_process/Makefile])
AC_CONFIG_FILES([uw_syncprobes/Makefile])
AC_CONFIG_FILES([compat/Makefile])
AC_CONFIG_FILES([compat/bb/Makefile])
AC_CONFIG_FILES([compat/bb/bbhimport/Makefile])
fi

( cd libstatgrab-0.13 ; ./configure --disable-manpages --disable-examples \
  --disable-statgrab --disable-saidar --disable-shared ; cd .. )

if test "$enable_xmbmon" = "yes"
then
  ( cd xmbmon203 ; ./configure ; cd .. )
fi

AC_OUTPUT
