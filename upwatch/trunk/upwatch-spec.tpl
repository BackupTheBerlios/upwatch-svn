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
BuildRequires: gzip glib2-devel gnet-devel mysql-devel curl-devel autogen libxslt kdelibs lynx

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
install -m 644 config/upwatch.conf $RPM_BUILD_ROOT/etc/

mkdir -p $RPM_BUILD_ROOT/etc/rc.d/init.d
[+ FOR program +]
# package specific files for [+program+]
install -m 755 [+program+]/[+program+].init $RPM_BUILD_ROOT/etc/rc.d/init.d/[+program+]
install -m 644 [+program+]/[+program+].conf $RPM_BUILD_ROOT/etc/upwatch.d/[+program+].conf
[+ ENDFOR +]

[+ FOR program +] 
# extra files for [+program+]
[+ include (string-append (get "program") "/" (get "program") ".spec-install") ;+]
[+ ENDFOR +]

%if %{strip_binaries}
{ cd $RPM_BUILD_ROOT
  strip .%{__prefix}/bin/* || /bin/true
}
%endif
%if %{gzip_man}
{ cd $RPM_BUILD_ROOT
  gzip .%{_mandir}/man1/*.1 
}
%endif

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%doc AUTHORS COPYING ChangeLog NEWS README doc/upwatch.html doc/upwatch.txt

%changelog
* Mon Sep 2 2002 Ron Arts <raarts@upwatch.com>
- Rel. 1: First package version

