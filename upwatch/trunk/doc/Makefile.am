docdir = $(prefix)/doc
doc_DATA = upwatch.html upwatch.txt 
EXTRA_DIST = upwatch.xml

upwatch.html: upwatch.xml
	xsltproc -o upwatch.html /usr/share/apps/ksgmltools2/docbook/xsl/html/docbook.xsl upwatch.xml 

upwatch.txt: upwatch.xml
	lynx -nolist -dump upwatch.html > upwatch.txt

CLEANFILES = upwatch.txt upwatch.html Makefile.in

