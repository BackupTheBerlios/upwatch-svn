[+ AutoGen5 template spec-generic spec-install +]

[+ CASE (suffix) +][+
   == spec-generic +]
[= AutoGen5 template spec =]
%package [+prog-name+]
Summary: UpWatch - [+prog-title+]
Group: Application/Monitoring
Requires: [+spec-requires+]

%description [+prog-name+]
[+detail+]

%files [+prog-name+]
%defattr(-,root,root)
/usr/bin/[+prog-name+]
%dir /var/log/upwatch
%dir /var/run/upwatch
%dir /var/spool/upwatch
/etc/rc.d/init.d/[+prog-name+]
%config(noreplace) /etc/upwatch.conf
%config(noreplace) /etc/upwatch.d/[+prog-name+].conf
/usr/share/man/man1/[+prog-name+].1.gz
[+ FOR spec-files +][+ 
spec-files +]
[+ ENDFOR +]

%post [+prog-name+]
/sbin/chkconfig --del [+prog-name+] 2>/dev/null || true # Make sure old versions aren't there anymore
/sbin/chkconfig --add [+prog-name+] || true

%postun [+prog-name+]
/sbin/chkconfig --del [+prog-name+] 2>/dev/null || true

[+ == spec-install +] 
[= AutoGen5 template spec =]
[+ FOR spec-install +][+ 
spec-install +]
[+ ENDFOR +][+ 
ESAC +]
