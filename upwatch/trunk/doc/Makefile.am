docdir = $(prefix)/doc
doc_DATA = upwatch.html upwatch.txt upwatch.pdf
EXTRA_DIST = upwatch.xml

upwatch.html: upwatch.xml
	xsltproc -o upwatch.html /usr/share/sgml/docbook/xsl-stylesheets/html/docbook.xsl upwatch.xml 

upwatch.txt: upwatch.xml
	lynx -nolist -dump upwatch.html > upwatch.txt

upwatch.pdf: upwatch.xml
	db2pdf upwatch.xml

CLEANFILES = upwatch.txt upwatch.html Makefile.in

