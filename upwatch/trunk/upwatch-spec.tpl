[+ AutoGen5 template spec +]
Summary: UpWatch - A High performance monitoring framwork
Vendor: http://www.upwatch.com
Name: upwatch
Version: [+ version +]
Release: 11
Source: http://www.upwatch.com/%{name}-%{version}.tar.gz
Packager: Ron Arts <raarts@upwatch.com>
Copyright: Proprietary - Redistribution Prohibited
Group: Application/Monitoring
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: gzip glib2-devel mysql-devel autogen libxslt lynx readline-devel
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

%prep

%setup  

%build 
%configure [+ (getenv "confargs") +]
make 
make check

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

# linux only 
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
install -m 660 config/logrotate $RPM_BUILD_ROOT/etc/logrotate.d/upwatch
mkdir -p $RPM_BUILD_ROOT/etc/cron.daily
install -m 770 config/cron.daily $RPM_BUILD_ROOT/etc/cron.daily/upwatch

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
for unpackaged in /usr/bin/tnot /usr/bin/saidar /usr/bin/statgrab /usr/bin/statgrab-make-mrtg-config /usr/bin/statgrab-make-mrtg-index /usr/include/statgrab.h /usr/lib/libstatgrab.a /usr/lib/libstatgrab.la /usr/lib/libstatgrab.so.1.1.0 /usr/lib/pkgconfig/libstatgrab.pc
do
  rm -f $RPM_BUILD_ROOT$unpackaged
done

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%pre
/usr/sbin/groupadd -g 78 upwatch
/usr/sbin/useradd -M -o -r -d /usr/share/upwatch -s /bin/bash \
        -c "Upwatch" -u 78 -g 78 upwatch > /dev/null 2>&1 || :

%post
# install initscripts
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
  ln -sf /usr/share/upwatch/init/upwatch.$DISTR /etc/init.d/upwatch
[+ FOR clientprog +]  ln -sf /usr/share/upwatch/init/[+clientprog+].$DISTR /etc/init.d/[+clientprog+]
[+ ENDFOR +]fi

if [ -f /etc/SuSE-release ]
then
  DISTR=suse
  ln -sf /usr/share/upwatch/init/upwatch.$DISTR /etc/init.d/upwatch
[+ FOR clientprog +]  ln -sf /usr/share/upwatch/init/[+clientprog+].$DISTR /etc/init.d/[+clientprog+]
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
  rm -f /etc/init.d/upwatch
[+ FOR clientprog +]  rm -f /etc/init.d/[+clientprog+]
  rm -f /usr/sbin/rc[+clientprog+]
[+ ENDFOR +]fi

%files
%defattr(0660,root,upwatch,0770)
/usr/share/doc/%{name}-%{version}
#%doc doc/upwatch.txt doc/upwatch.html doc/upwatch.pdf
%attr(0770,upwatch,upwatch) %dir /etc/upwatch.d
%attr(0770,upwatch,upwatch) %dir /usr/share/upwatch
%attr(0770,upwatch,upwatch) %dir /usr/share/upwatch/init
%attr(0770,upwatch,upwatch) %dir /usr/share/upwatch/dtd
/usr/share/upwatch/dtd/result.dtd
%attr(0755,root,root) /usr/share/upwatch/init/upwatch.redhat
%attr(0755,root,root) /usr/share/upwatch/init/upwatch.suse
%config(noreplace) /etc/upwatch.conf
/etc/logrotate.d/upwatch
%attr(700,root,root) /etc/cron.daily/upwatch
%attr(2770,upwatch,upwatch) %dir /var/log/upwatch
%attr(2770,upwatch,upwatch) %dir /var/spool/upwatch
%attr(2770,upwatch,upwatch) %dir /var/run/upwatch
%attr(0755,root,root) /usr/bin/uwsaidar
%attr(0755,root,root) /usr/bin/ctime
%attr(0755,root,root) /usr/bin/slot
%attr(0755,root,root) /usr/bin/uwregexp
%attr(0755,root,root) /usr/bin/uwq
/usr/share/man/man1/ctime.1.gz
/usr/share/man/man1/slot.1.gz
/usr/share/man/man1/uwq.1.gz
[+ FOR clientprog +][+ include (string-append (get "clientprog") "/" (get "clientprog") ".spec-files") ;+]
[+ ENDFOR +]

[+ IF monitorprog +]
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
%post monitor
# install initscripts
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
[+ FOR monitorprog +]  ln -sf /usr/share/upwatch/init/[+monitorprog+].$DISTR /etc/init.d/[+monitorprog+]
[+ ENDFOR +]fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
[+ FOR monitorprog +]  ln -sf /usr/share/upwatch/init/[+monitorprog+].$DISTR /etc/init.d/[+monitorprog+]
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

%files monitor
%defattr(0660,root,upwatch,0770)
[+ FOR monitorprog +][+ include (string-append (get "monitorprog") "/" (get "monitorprog") ".spec-files") ;+]
[+ ENDFOR +]
[+ ENDIF monitor +]

[+ IF serverprog +]
%package server
Summary: UpWatch - server programs
Group: Application/Monitoring
Requires: upwatch 
[+ FOR serverprog +]# [+serverprog+] requirements:
[+ include (string-append (get "serverprog") "/" (get "serverprog") ".spec-requires") ;+]
[+ include (string-append (get "serverprog") "/" (get "serverprog") ".spec-buildrequires") ;+]
[+ ENDFOR +]
%description server
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package contains the needed programs for the upwatch
processing server.

%post server
# install initscripts
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
[+ FOR serverprog +]  ln -sf /usr/share/upwatch/init/[+serverprog+].$DISTR /etc/init.d/[+serverprog+]
[+ ENDFOR +]fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
[+ FOR serverprog +]  ln -sf /usr/share/upwatch/init/[+serverprog+].$DISTR /etc/init.d/[+serverprog+]
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

%files server
%defattr(0660,root,upwatch,0770)
%attr(0755,root,root) /usr/bin/bbhimport
%attr(0755,root,root) /usr/bin/uw_maint.pl
/usr/share/man/man1/bbhimport.1.gz
%attr(0755,root,root) /usr/bin/fill_probe_description.pl
[+ FOR serverprog +][+ include (string-append (get "serverprog") "/" (get "serverprog") ".spec-files") ;+]
[+ ENDFOR +]
[+ ENDIF serverprog +]

[+ IF extraprog +]
[+ FOR extraprog +]
%package [+ extraprog +]
Summary: UpWatch - monitor IP traffic
Group: Application/Monitoring
Requires: upwatch libxml2 >= 2.4.19 libpcap glib2 >= 2.0.4

%description [+ extraprog +]
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package contains [+ extraprog +]

%post [+ extraprog +]
# install initscript
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
  ln -sf /usr/share/upwatch/init/[+ extraprog +].$DISTR /etc/init.d/[+ extraprog +]
fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
  ln -sf /usr/share/upwatch/init/[+ extraprog +].$DISTR /etc/init.d/[+ extraprog +]
  pushd /usr/sbin
  ln -sf ../../etc/init.d/[+ extraprog +] rc[+ extraprog +]
  popd
fi
if [ -x /sbin/chkconfig ]; then
  /sbin/chkconfig --add [+ extraprog +] 2>/dev/null || true
fi

%postun [+ extraprog +]
if [ "$1" -eq "0" ]; then
  rm -f /etc/init.d/[+ extraprog +]
  rm -f /usr/sbin/rc[+ extraprog +]
fi

%preun [+ extraprog +]
if [ "$1" = 0 ] ; then
  if [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --del [+ extraprog +] 2>/dev/null || true
  fi
fi

%files [+extraprog+]
%defattr(0660,root,upwatch,0770)
[+ include (string-append (get "extraprog") "/" (get "extraprog") ".spec-files") ;+]

[+ ENDFOR extraprog +]
[+ ENDIF extraprog +]

%changelog
* Fri Dec 27 2002 Ron Arts <raarts@upwatch.com>
- Added ChangeLog, removed xml files from doc dir

* Mon Sep 2 2002 Ron Arts <raarts@upwatch.com>
- Rel. 1: First package version

