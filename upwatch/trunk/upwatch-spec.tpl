[+ AutoGen5 template spec +]
Summary: UpWatch - A High performance monitoring framwork
Vendor: http://www.upwatch.com
Name: upwatch
Version: [+ version +]
Release: 0
Source: http://www.upwatch.com/%{name}-%{version}.tar.gz
Packager: Ron Arts <raarts@upwatch.com>
Copyright: Proprietary - Redistribution Prohibited
Group: Application/Monitoring
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Requires: libxml2 >= 2.4.19
BuildRequires: gzip glib2-devel mysql-devel autogen libxslt lynx 

%define strip_binaries 1
%define gzip_man 1
%define __prefix /usr

Prefix: %{__prefix} 

%description
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It is built for high-volume sites.
It boasts support for various services, long-time
history, graphs, and notification. 

This package contains all upwatch documentation, plus supporting files
like the database schema.

%package utils
Summary: UpWatch - utilities and supporting files
Group: Application/Monitoring
Requires: upwatch libxml2 >= 2.4.19 mysql-client glib2 

%description utils
This package contains utilities, maintenance scripts and other
files.

%files utils
%attr(0755,root,root) /usr/bin/bbhimport
%attr(0755,root,root) /usr/bin/ctime
%attr(0755,root,root) /usr/bin/slot
%attr(0755,root,root) /usr/bin/fill_probe_description.pl
%attr(0755,root,root) /usr/bin/uw_maint.pl
/usr/share/man/man1/ctime.1.gz
/usr/share/man/man1/slot.1.gz
/usr/share/man/man1/bbhimport.1.gz

[+ FOR program +]
[+ include (string-append (get "program") "/" (get "program") ".spec-generic") ;+]
[+ ENDFOR +]

%prep
%setup  

%build 
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

if [ -f /usr/lib/libopts.so ]
then
  cp /usr/lib/libopts.so $RPM_BUILD_ROOT/usr/lib
  cp /usr/lib/libopts.so.9 $RPM_BUILD_ROOT/usr/lib
fi

if [ -f /usr/local/lib/libopts.so ]
then
  cp /usr/local/lib/libopts.so $RPM_BUILD_ROOT/usr/lib
  cp /usr/local/lib/libopts.so.9 $RPM_BUILD_ROOT/usr/lib
fi

[+ FOR program +]
# package specific files for [+program+]
install -m 770 [+program+]/[+program+].init $RPM_BUILD_ROOT/usr/lib/upwatch/redhat/[+program+]
install -m 770 [+program+]/[+program+].init-suse $RPM_BUILD_ROOT/usr/lib/upwatch/suse/[+program+]
install -m 660 [+program+]/[+program+].conf $RPM_BUILD_ROOT/etc/upwatch.d/[+program+].conf
[+ ENDFOR +]

[+ FOR program +] 
# extra files for [+program+]
[+ include (string-append (get "program") "/" (get "program") ".spec-install") ;+]
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
fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
  ln -sf /etc/init.d/upwatch /usr/lib/upwatch/$DISTR/upwatch
fi

%postun
if [ "$1" -eq "0" ]; then
  rm -f /etc/init.d/upwatch
  rm -f /usr/sbin/rcupwatch
fi

%files
%defattr(0660,root,upwatch,0770)
%attr(0644,root,root) %doc AUTHORS COPYING ChangeLog NEWS README upwatch.mysql doc/upwatch.html doc/upwatch.txt doc/upwatch.pdf
%attr(0755,root,root) /usr/lib/libopts.so
%attr(0755,root,root) /usr/lib/libopts.so.9
%attr(0770,upwatch,upwatch) /usr/lib/upwatch
/etc/logrotate.d/upwatch
/etc/cron.daily/upwatch
%attr(2770,upwatch,upwatch) %dir /var/log/upwatch
%attr(2770,upwatch,upwatch) %dir /var/run/upwatch
%attr(2770,upwatch,upwatch) %dir /var/spool/upwatch

%changelog
* Fri Dec 27 2002 Ron Arts <raarts@upwatch.com>
- Added ChangeLog, removed xml files from doc dir

* Mon Sep 2 2002 Ron Arts <raarts@upwatch.com>
- Rel. 1: First package version

