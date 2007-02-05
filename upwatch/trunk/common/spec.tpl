[+ AutoGen5 template spec-files spec-requires spec-buildrequires +]
[+ CASE (suffix) +][+
   == spec-files
+][= AutoGen5 template spec =]
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].redhat
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].suse
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].solaris
%attr(0755,upwatch,upwatch) /usr/share/upwatch/init/[+prog-name+].debian
%config(noreplace) /etc/upwatch.d/[+prog-name+].conf
%attr(0444,upwatch,upwatch) /usr/share/man/man1/[+prog-name+].1.gz
[+ FOR spec-files +][+ 
spec-files +]
[+ ENDFOR +]
[+ == spec-requires
+][= AutoGen5 template spec =]
[+ IF spec-buildrequires +]Requires: [+ FOR spec-requires +][+ spec-requires +][+ ENDFOR +][+ ENDIF +]
[+ == spec-buildrequires
+][= AutoGen5 template spec =]
[+ IF spec-buildrequires +]BuildRequires: [+ FOR spec-buildrequires +][+ spec-buildrequires +][+ ENDFOR +][+ ENDIF +]
[+ ESAC +]
