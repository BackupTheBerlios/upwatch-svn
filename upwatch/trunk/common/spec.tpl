[+ AutoGen5 template spec-files spec-requires spec-install +]
[+ CASE (suffix) +][+
   == spec-files
+][= AutoGen5 template spec =]
%attr(0755,upwatch,upwatch) /usr/share/upwatch/redhat/[+prog-name+].redhat
%attr(0755,upwatch,upwatch) /usr/share/upwatch/suse/[+prog-name+].suse
%config(noreplace) /etc/upwatch.d/[+prog-name+].conf
%attr(0444,upwatch,upwatch) /usr/share/man/man1/[+prog-name+].1.gz
[+ FOR spec-files +][+ 
spec-files +]
[+ ENDFOR +]
[+ == spec-requires
+][= AutoGen5 template spec =]
Requires: [+ FOR spec-requires +][+ spec-requires +][+ ENDFOR +]
[+ == spec-install 
+][= AutoGen5 template spec =]
[+ FOR spec-install +][+ 
spec-install +]
[+ ENDFOR +][+ 
ESAC +]
