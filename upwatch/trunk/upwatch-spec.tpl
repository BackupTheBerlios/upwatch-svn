[+ AutoGen5 template spec +]
Summary: UpWatch - A High performance monitoring framwork
Vendor: http://www.upwatch.com
Name: upwatch
Version: [+ version +]
Release: 1
Source: http://www.upwatch.com/%{name}-%{version}.tar.gz
Packager: Ron Arts <raarts@upwatch.com>
Copyright: Proprietary - Redistribution Prohibited
Group: Application/Monitoring
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: gzip glib2-devel mysql-devel autogen libxslt lynx 
Requires: libxml2 >= 2.4.19 glib2
[+ FOR clientprog +]# [+clientprog+] requirements:
[+ include (string-append (get "clientprog") "/" (get "clientprog") ".spec-requires") ;+]
[+ ENDFOR +]
%define strip_binaries 1
%define gzip_man 1
%define __prefix /usr

Prefix: %{__prefix} 

%description
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package contains all upwatch documentation, the client monitoring
programs, utilities and supporting files like the database schema.

%package server
Summary: UpWatch - server programs
Group: Application/Monitoring
Requires: upwatch 
[+ FOR serverprog +]# [+serverprog+] requirements:
[+ include (string-append (get "serverprog") "/" (get "serverprog") ".spec-requires") ;+]
[+ ENDFOR +]
%description server
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package contains the needed programs for the upwatch
processing server.

%package monitor
Summary: UpWatch - programs for a monitoring station
Group: Application/Monitoring
Requires: upwatch
[+ FOR monitorprog +]# [+monitorprog+] requirements:
[+ include (string-append (get "monitorprog") "/" (get "monitorprog") ".spec-requires") ;+]
[+ ENDFOR +]
%description monitor
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package should be installed on a monitoring station
that remotely checks services. It contains the following
programs:
[+ FOR monitorprog +] [+monitorprog+]
[+ ENDFOR +]
%package iptraf
Summary: UpWatch - monitor IP traffic
Group: Application/Monitoring
Requires: upwatch libxml2 >= 2.4.19 libpcap glib2 >= 2.0.4

%description iptraf
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package contains the IP traffic monitor for
edge/border routers.

%prep

%setup  

%build 
# so libstatgrab configures with our commandline parameters
cp patches/libstatgrab-configure.gnu libstatgrab-0.6/configure.gnu 

%configure
make 
make check

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

mkdir -p $RPM_BUILD_ROOT/etc/upwatch.d
mkdir -p $RPM_BUILD_ROOT/var/lib/upwatch
mkdir -p $RPM_BUILD_ROOT/var/log/upwatch
mkdir -p $RPM_BUILD_ROOT/var/spool/upwatch
mkdir -p $RPM_BUILD_ROOT/var/run/upwatch
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/lib
install -m 660 config/upwatch.conf $RPM_BUILD_ROOT/etc/
mkdir -p $RPM_BUILD_ROOT/usr/lib/upwatch/dtd
install -m 660 config/result.dtd $RPM_BUILD_ROOT/usr/lib/upwatch/dtd
mkdir -p $RPM_BUILD_ROOT/usr/lib/upwatch/redhat
mkdir -p $RPM_BUILD_ROOT/usr/lib/upwatch/suse
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
install -m 660 config/logrotate $RPM_BUILD_ROOT/etc/logrotate.d/upwatch
mkdir -p $RPM_BUILD_ROOT/etc/cron.daily
install -m 700 config/cron.daily $RPM_BUILD_ROOT/etc/cron.daily/upwatch

mkdir -p $RPM_BUILD_ROOT/etc/init.d
install -m 770 config/upwatch.init $RPM_BUILD_ROOT/usr/lib/upwatch/redhat/upwatch
install -m 770 config/upwatch.init-suse $RPM_BUILD_ROOT/usr/lib/upwatch/suse/upwatch

#if [ -f /usr/lib/libopts.so ]
#then
#  cp /usr/lib/libopts.so $RPM_BUILD_ROOT/usr/lib
#  cp /usr/lib/libopts.so.9 $RPM_BUILD_ROOT/usr/lib
#fi

#if [ -f /usr/local/lib/libopts.so ]
#then
#  cp /usr/local/lib/libopts.so $RPM_BUILD_ROOT/usr/lib
#  cp /usr/local/lib/libopts.so.9 $RPM_BUILD_ROOT/usr/lib
#fi

[+ FOR monitorprog +]# package specific files for [+monitorprog+]
install -m 770 [+monitorprog+]/[+monitorprog+].init $RPM_BUILD_ROOT/usr/lib/upwatch/redhat/[+monitorprog+]
install -m 770 [+monitorprog+]/[+monitorprog+].init-suse $RPM_BUILD_ROOT/usr/lib/upwatch/suse/[+monitorprog+]
install -m 660 [+monitorprog+]/[+monitorprog+].conf $RPM_BUILD_ROOT/etc/upwatch.d/[+monitorprog+].conf
[+ include (string-append (get "monitorprog") "/" (get "monitorprog") ".spec-install") ;+]
[+ ENDFOR +]
[+ FOR clientprog +]# package specific files for [+clientprog+]
install -m 770 [+clientprog+]/[+clientprog+].init $RPM_BUILD_ROOT/usr/lib/upwatch/redhat/[+clientprog+]
install -m 770 [+clientprog+]/[+clientprog+].init-suse $RPM_BUILD_ROOT/usr/lib/upwatch/suse/[+clientprog+]
install -m 660 [+clientprog+]/[+clientprog+].conf $RPM_BUILD_ROOT/etc/upwatch.d/[+clientprog+].conf
[+ include (string-append (get "clientprog") "/" (get "clientprog") ".spec-install") ;+]
[+ ENDFOR +]
[+ FOR serverprog +]# package specific files for [+serverprog+]
install -m 770 [+serverprog+]/[+serverprog+].init $RPM_BUILD_ROOT/usr/lib/upwatch/redhat/[+serverprog+]
install -m 770 [+serverprog+]/[+serverprog+].init-suse $RPM_BUILD_ROOT/usr/lib/upwatch/suse/[+serverprog+]
install -m 660 [+serverprog+]/[+serverprog+].conf $RPM_BUILD_ROOT/etc/upwatch.d/[+serverprog+].conf
[+ include (string-append (get "serverprog") "/" (get "serverprog") ".spec-install") ;+]
[+ ENDFOR +]
[+ FOR extraprog +]# package specific files for [+extraprog+]
install -m 770 [+extraprog+]/[+extraprog+].init $RPM_BUILD_ROOT/usr/lib/upwatch/redhat/[+extraprog+]
install -m 770 [+extraprog+]/[+extraprog+].init-suse $RPM_BUILD_ROOT/usr/lib/upwatch/suse/[+extraprog+]
install -m 660 [+extraprog+]/[+extraprog+].conf $RPM_BUILD_ROOT/etc/upwatch.d/[+extraprog+].conf
[+ include (string-append (get "extraprog") "/" (get "extraprog") ".spec-install") ;+]
[+ ENDFOR +]
%if %{strip_binaries}
{ cd $RPM_BUILD_ROOT
  strip .%{__prefix}/bin/* || /bin/true
  strip .%{__prefix}/sbin/* || /bin/true
}
%endif
%if %{gzip_man}
{ cd $RPM_BUILD_ROOT
  gzip .%{_mandir}/man1/*.1 
}
%endif

# remove files we don't want to package
for unpackaged in /usr/bin/saidar /usr/bin/statgrab /usr/bin/statgrab-make-mrtg-config /usr/bin/statgrab-make-mrtg-index /usr/include/statgrab.h /usr/lib/libstatgrab.a /usr/lib/libstatgrab.la /usr/lib/libstatgrab.so.1.0.7 /usr/lib/pkgconfig/libstatgrab.pc
do
  rm -f $RPM_BUILD_ROOT$unpackaged
done

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%pre
/usr/sbin/groupadd -g 78 upwatch
/usr/sbin/useradd -M -o -r -d /usr/lib/upwatch -s /bin/bash \
        -c "Upwatch" -u 78 -g 78 upwatch > /dev/null 2>&1 || :

%post
# install initscripts
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
  ln -sf /etc/init.d/upwatch /usr/lib/upwatch/$DISTR/upwatch
  ln -sf /etc/init.d/upwatch /usr/sbin/rcupwatch
[+ FOR clientprog +]  ln -sf /usr/lib/upwatch/$DISTR/[+clientprog+] /etc/init.d/[+clientprog+]
[+ ENDFOR +]fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
  ln -sf /etc/init.d/upwatch /usr/lib/upwatch/$DISTR/upwatch
[+ FOR clientprog +]  ln -sf /usr/lib/upwatch/$DISTR/[+clientprog+] /etc/init.d/[+clientprog+]
[+ ENDFOR +]  pushd /usr/sbin
[+ FOR clientprog +]  ln -sf ../../etc/init.d/[+clientprog+] rc[+clientprog+]
[+ ENDFOR +]  popd
fi
if [ -x /sbin/chkconfig ]; then
[+ FOR clientprog +]  /sbin/chkconfig --add [+clientprog+] 2>/dev/null || true
[+ ENDFOR +]fi

%preun
if [ "$1" = 0 ] ; then
  if [ -x /sbin/chkconfig ]; then
[+ FOR clientprog +]    /sbin/chkconfig --del [+clientprog+] 2>/dev/null || true
[+ ENDFOR +]  fi
fi

%postun
if [ "$1" -eq "0" ]; then
[+ FOR clientprog +]  rm -f /etc/init.d/[+clientprog+]
  rm -f /usr/sbin/rc[+clientprog+]
[+ ENDFOR +]fi

%post monitor
# install initscripts
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
[+ FOR monitorprog +]  ln -sf /usr/lib/upwatch/$DISTR/[+monitorprog+] /etc/init.d/[+monitorprog+]
[+ ENDFOR +]fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
[+ FOR monitorprog +]  ln -sf /usr/lib/upwatch/$DISTR/[+monitorprog+] /etc/init.d/[+monitorprog+]
[+ ENDFOR +]  pushd /usr/sbin
[+ FOR monitorprog +]  ln -sf ../../etc/init.d/[+monitorprog+] rc[+monitorprog+]
[+ ENDFOR +]  popd
fi
if [ -x /sbin/chkconfig ]; then
[+ FOR monitorprog +]  /sbin/chkconfig --add [+monitorprog+] 2>/dev/null || true
[+ ENDFOR +]fi

%preun monitor
if [ "$1" = 0 ] ; then
  if [ -x /sbin/chkconfig ]; then
[+ FOR monitorprog +]    /sbin/chkconfig --del [+monitorprog+] 2>/dev/null || true
[+ ENDFOR +]  fi
fi

%postun monitor
if [ "$1" -eq "0" ]; then
[+ FOR monitorprog +]  rm -f /etc/init.d/[+monitorprog+]
  rm -f /usr/sbin/rc[+monitorprog+]
[+ ENDFOR +]fi

%post server
# install initscripts
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
[+ FOR serverprog +]  ln -sf /usr/lib/upwatch/$DISTR/[+serverprog+] /etc/init.d/[+serverprog+]
[+ ENDFOR +]fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
[+ FOR serverprog +]  ln -sf /usr/lib/upwatch/$DISTR/[+serverprog+] /etc/init.d/[+serverprog+]
[+ ENDFOR +]  pushd /usr/sbin
[+ FOR serverprog +]  ln -sf ../../etc/init.d/[+serverprog+] rc[+serverprog+]
[+ ENDFOR +]  popd
fi
if [ -x /sbin/chkconfig ]; then
[+ FOR serverprog +]  /sbin/chkconfig --add [+serverprog+] 2>/dev/null || true
[+ ENDFOR +]fi

%preun server
if [ "$1" = 0 ] ; then
  if [ -x /sbin/chkconfig ]; then
[+ FOR serverprog +]    /sbin/chkconfig --del [+serverprog+] 2>/dev/null || true
[+ ENDFOR +]  fi
fi

%postun server
if [ "$1" -eq "0" ]; then
[+ FOR serverprog +]  rm -f /etc/init.d/[+serverprog+]
  rm -f /usr/sbin/rc[+serverprog+]
[+ ENDFOR +]fi

%post iptraf
# install initscript
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
  ln -sf /usr/lib/upwatch/$DISTR/uw_iptraf /etc/init.d/uw_iptraf
fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
  ln -sf /usr/lib/upwatch/$DISTR/uw_iptraf /etc/init.d/uw_iptraf
  pushd /usr/sbin
  ln -sf ../../etc/init.d/uw_iptraf rcuw_iptraf
  popd
fi
if [ -x /sbin/chkconfig ]; then
  /sbin/chkconfig --add uw_iptraf 2>/dev/null || true
fi

%postun iptraf
if [ "$1" -eq "0" ]; then
  rm -f /etc/init.d/uw_iptraf
  rm -f /usr/sbin/rcuw_iptraf
fi

%preun iptraf
if [ "$1" = 0 ] ; then
  if [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --del uw_iptraf 2>/dev/null || true
  fi
fi

%files
%defattr(0660,root,upwatch,0770)
%attr(0644,root,root) %doc AUTHORS COPYING ChangeLog NEWS README upwatch.mysql doc/upwatch.html doc/upwatch.txt doc/upwatch.pdf
#%attr(0755,root,root) /usr/lib/libopts.so
#%attr(0755,root,root) /usr/lib/libopts.so.9
%attr(0770,upwatch,upwatch) %dir /usr/lib/upwatch
%attr(0770,upwatch,upwatch) %dir /usr/lib/upwatch/redhat
%attr(0770,upwatch,upwatch) %dir /usr/lib/upwatch/suse
%attr(0770,upwatch,upwatch) %dir /usr/lib/upwatch/dtd
/usr/lib/upwatch/dtd/result.dtd
/usr/lib/upwatch/redhat/upwatch
/usr/lib/upwatch/suse/upwatch
%config(noreplace) /etc/upwatch.conf
/etc/logrotate.d/upwatch
/etc/cron.daily/upwatch
%attr(2770,upwatch,upwatch) %dir /var/log/upwatch
%attr(2770,upwatch,upwatch) %dir /var/run/upwatch
%attr(2770,upwatch,upwatch) %dir /var/spool/upwatch
%attr(0755,root,root) /usr/bin/ctime
%attr(0755,root,root) /usr/bin/slot
/usr/share/man/man1/ctime.1.gz
/usr/share/man/man1/slot.1.gz
[+ FOR clientprog +][+ include (string-append (get "clientprog") "/" (get "clientprog") ".spec-files") ;+]
[+ ENDFOR +]
%files monitor
%defattr(0660,root,upwatch,0770)
[+ FOR monitorprog +][+ include (string-append (get "monitorprog") "/" (get "monitorprog") ".spec-files") ;+]
[+ ENDFOR +]
%files iptraf
%defattr(0660,root,upwatch,0770)
[+ include "uw_iptraf/uw_iptraf.spec-files" ;+]

%files server
%defattr(0660,root,upwatch,0770)
%attr(0755,root,root) /usr/bin/bbhimport
%attr(0755,root,root) /usr/bin/uw_maint.pl
/usr/share/man/man1/bbhimport.1.gz
%attr(0755,root,root) /usr/bin/fill_probe_description.pl
[+ FOR serverprog +][+ include (string-append (get "serverprog") "/" (get "serverprog") ".spec-files") ;+]
[+ ENDFOR +]
%changelog
* Fri Dec 27 2002 Ron Arts <raarts@upwatch.com>
- Added ChangeLog, removed xml files from doc dir

* Mon Sep 2 2002 Ron Arts <raarts@upwatch.com>
- Rel. 1: First package version

