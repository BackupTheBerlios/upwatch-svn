[+ AutoGen5 template spec-files spec-requires +]
[+ CASE (suffix) +][+
   == spec-files
+][= AutoGen5 template spec =]
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].redhat
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].suse
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].solaris
%config(noreplace) /etc/upwatch.d/[+prog-name+].conf
%attr(0444,upwatch,upwatch) /usr/share/man/man1/[+prog-name+].1.gz
[+ FOR spec-files +][+ 
spec-files +]
[+ ENDFOR +]
[+ == spec-requires
+][= AutoGen5 template spec =]
Requires: [+ FOR spec-requires +][+ spec-requires +][+ ENDFOR +]
[+ ESAC +]
