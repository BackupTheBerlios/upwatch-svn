[+ AutoGen5 template spec +]

Summary: UpWatch - The Best monitoring framework
Vendor: http://www.upwatch.com
Name: upwatch
Version: 0.1
Release: 1
Source: http://www.upwatch.com/%{name}-%{version}.tar.gz
Packager: Ron Arts <raarts@upwatch.com>
Copyright: Proprietary
Group: Application/Monitoring
BuildRoot: %{_tmppath}/%{name}-%{version}-root
BuildRequires: gzip glib2-devel gnet-devel mysql-devel curl-devel autogen libxslt docbook-style-xsl lynx libnet

%define strip_binaries 1
%define gzip_man 1
%define __prefix /usr

Prefix: %{__prefix} 

%description
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It boasts support for various services, long-time
history, graphs, and notification

This package contains all upwatch documentation, plus supporting files
like the database schema.

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
install -m 660 config/upwatch.conf $RPM_BUILD_ROOT/etc/
mkdir -p $RPM_BUILD_ROOT/usr/lib/upwatch/dtd
install -m 660 config/result.dtd $RPM_BUILD_ROOT/usr/lib/upwatch/dtd
mkdir -p $RPM_BUILD_ROOT/etc/logrotate.d
install -m 660 config/logrotate $RPM_BUILD_ROOT/etc/logrotate.d/upwatch

mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
[+ FOR program +]
# package specific files for [+program+]
install -m 770 [+program+]/[+program+].init $RPM_BUILD_ROOT/etc/rc.d/init.d/[+program+]
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

%files
%defattr(0660,root,upwatch,0770)
%attr(0644,root,root) %doc AUTHORS COPYING ChangeLog NEWS README doc/upwatch.html doc/upwatch.txt doc/upwatch.pdf doc/upwatch.xml
/usr/lib/upwatch
/etc/logrotate.d/upwatch
%dir /var/log/upwatch
%dir /var/run/upwatch
%dir /var/spool/upwatch

%changelog
* Mon Sep 2 2002 Ron Arts <raarts@upwatch.com>
- Rel. 1: First package version

