[+ AutoGen5 template spec-generic spec-install +]
[+ CASE (suffix) +][+
   == spec-generic 
+][= AutoGen5 template spec =]
[+(dne "# ")+]
%package [+prog-name+]
Summary: UpWatch - [+prog-title+]
Group: Application/Monitoring
Requires: upwatch libxml2 >= 2.4.19 [+spec-requires+]

%description [+prog-name+]
[+detail+]

%files [+prog-name+]
%defattr(0660,upwatch,upwatch,0770)
%attr(0755,upwatch,upwatch) /usr/lib/upwatch/redhat/[+prog-name+]
%attr(0755,upwatch,upwatch) /usr/lib/upwatch/suse/[+prog-name+]
%config(noreplace) /etc/upwatch.conf
%config(noreplace) /etc/upwatch.d/[+prog-name+].conf
%attr(0444,upwatch,upwatch) /usr/share/man/man1/[+prog-name+].1.gz
[+ FOR spec-files +][+ 
spec-files +]
[+ ENDFOR +]

%post [+prog-name+]
if [ -f /etc/redhat-release ]
then
  DISTR=redhat
fi
if [ -f /etc/SuSE-release ]
then
  DISTR=suse
fi
# install initscript
ln -sf /usr/lib/upwatch/$DISTR/[+prog-name+] /etc/init.d/[+prog-name+]
pushd /usr/sbin
ln -sf ../../etc/init.d/[+prog-name+] rc[+prog-name+]
popd
if [ -x /sbin/chkconfig ]; then
  /sbin/chkconfig --add [+prog-name+] 2>/dev/null || true
fi

%postun [+prog-name+]
if [ "$1" -eq "0" ]; then
  rm -f /etc/init.d/[+prog-name+]
  rm -f /usr/sbin/rc[+prog-name+]
fi

%preun [+prog-name+]
if [ "$1" = 0 ] ; then
  if [ -x /sbin/chkconfig ]; then
    /sbin/chkconfig --del [+prog-name+] 2>/dev/null || true
  fi
fi

[+ == spec-install 
+][= AutoGen5 template spec =]
[+(dne "# ")+]
[+ FOR spec-install +][+ 
spec-install +]
[+ ENDFOR +][+ 
ESAC +]
