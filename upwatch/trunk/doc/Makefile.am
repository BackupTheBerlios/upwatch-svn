docdir = $(prefix)/doc
doc_DATA = upwatch.html upwatch.txt 
EXTRA_DIST = upwatch.xml

upwatch.html: upwatch.xml
	xsltproc -o upwatch.html /usr/share/sgml/docbook/xsl-stylesheets/html/docbook.xsl upwatch.xml 

upwatch.txt: upwatch.xml
	lynx -nolist -dump upwatch.html > upwatch.txt

upwatch.fo: upwatch.xml
	xsltproc -o upwatch.fo /usr/share/sgml/docbook/xsl-stylesheets/fo/docbook.xsl upwatch.xml 

CLEANFILES = upwatch.txt upwatch.html Makefile.in

