
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
BuildRequires: gzip glib2-devel gnet-devel mysql-devel curl-devel autogen 

%define strip_binaries 1
%define gzip_man 1
%define  __prefix /usr

Prefix: %{__prefix} 

%description
Upwatch is a full-fledged monitoring and report engine for
internet hosts. It boasts support for various services, long-time
history, graphs, and notification

This package contains all upwatch documentation, plus supporting files
like the database schema.



%package uw_accept
Summary: UpWatch - Upwatch daemon for accepting reports
Group: Application/Monitoring
Requires: libpcap mysql glib2 gnet >= 1.1.2 curl >= 7.9.3 autogen >= 5.3.6 libnet >= 1.0.2

%description uw_accept
uw_accept listen on port 1985 for incoming upwatch reports.
Using a POP3-like protocol it asks for a username and password
and copies incoming files to a queue

%files uw_accept
%defattr(-,root,root)
/usr/bin/uw_accept
%config(noreplace) /etc/uw_accept.conf
%config(noreplace) /etc/upwatch.d/uw_accept.conf
/usr/share/man/man1/uw_accept.1.gz


%package uw_httpget
Summary: UpWatch - Upwatch parallel probe daemon
Group: Application/Monitoring
Requires: libpcap mysql glib2 gnet >= 1.1.2 curl >= 7.9.3 autogen >= 5.3.6 libnet >= 1.0.2

%description uw_httpget
uw_httpget reads a list of hosts from the database, and 
sends http GET requests to each host. This happens in parallel, 
so uw_httpget can process thousands of hosts in a very short period.

%files uw_httpget
%defattr(-,root,root)
/usr/bin/uw_httpget
%config(noreplace) /etc/uw_httpget.conf
%config(noreplace) /etc/upwatch.d/uw_httpget.conf
/usr/share/man/man1/uw_httpget.1.gz


%package uw_ping
Summary: UpWatch - Upwatch parallel ping daemon
Group: Application/Monitoring
Requires: libpcap mysql glib2 gnet >= 1.1.2 curl >= 7.9.3 autogen >= 5.3.6 libnet >= 1.0.2

%description uw_ping
uw_ping reads a list of hosts from the database, and sends ping 
packets to each host.  This happens in parallel, so uw_ping can 
process thousands of hosts in a very short period.

%files uw_ping
%defattr(-,root,root)
/usr/bin/uw_ping
%config(noreplace) /etc/uw_ping.conf
%config(noreplace) /etc/upwatch.d/uw_ping.conf
/usr/share/man/man1/uw_ping.1.gz


%package uw_process
Summary: UpWatch - Upwatch parallel probe daemon
Group: Application/Monitoring
Requires: libpcap mysql glib2 gnet >= 1.1.2 curl >= 7.9.3 autogen >= 5.3.6 libnet >= 1.0.2

%description uw_process
uw_process sends all files in the queue to a central server using 
the uw_process protocol. This is a very simple protocol and looks
something like POP3.

%files uw_process
%defattr(-,root,root)
/usr/bin/uw_process
%config(noreplace) /etc/uw_process.conf
%config(noreplace) /etc/upwatch.d/uw_process.conf
/usr/share/man/man1/uw_process.1.gz


%package uw_send
Summary: UpWatch - Upwatch parallel probe daemon
Group: Application/Monitoring
Requires: libpcap mysql glib2 gnet >= 1.1.2 curl >= 7.9.3 autogen >= 5.3.6 libnet >= 1.0.2

%description uw_send
uw_send sends all files in the queue to a central server using 
the uw_send protocol. This is a very simple protocol and looks
something like POP3.

%files uw_send
%defattr(-,root,root)
/usr/bin/uw_send
%config(noreplace) /etc/uw_send.conf
%config(noreplace) /etc/upwatch.d/uw_send.conf
/usr/share/man/man1/uw_send.1.gz


%package uw_traceroute
Summary: UpWatch - Upwatch problem investigator daemon
Group: Application/Monitoring
Requires: libpcap mysql glib2 gnet >= 1.1.2 curl >= 7.9.3 autogen >= 5.3.6 libnet >= 1.0.2

%description uw_traceroute
When some probe cannot get to its destination, it hands the
problem over to uw_traceroute, which does a traceroute (either icmp, 
tcp or udp) to the destination, and reports on its findings.

%files uw_traceroute
%defattr(-,root,root)
/usr/bin/uw_traceroute
%config(noreplace) /etc/uw_traceroute.conf
%config(noreplace) /etc/upwatch.d/uw_traceroute.conf
/usr/share/man/man1/uw_traceroute.1.gz


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
install -m 644 config/upwatch.conf $RPM_BUILD_ROOT/etc/
install -m 644 uw_accept/uw_accept.conf $RPM_BUILD_ROOT/etc/upwatch.d
install -m 644 uw_process/uw_process.conf $RPM_BUILD_ROOT/etc/upwatch.d

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
#[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%doc AUTHORS COPYING ChangeLog NEWS README 

%changelog
* Mon Sep 2 2002 Ron Arts <raarts@upwatch.com>
- Rel. 1: First package version

