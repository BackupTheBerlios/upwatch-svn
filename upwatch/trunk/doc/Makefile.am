docdir = $(prefix)/doc
doc_DATA = upwatch.html upwatch.txt upwatch.pdf

XMLSOURCE = upwatch.xml addingaprobe.xml administration.xml configuration.xml \
  howitallworks.xml installation.xml interfaces.xml iptraf-specs.xml ping-specs.xml \
  probesspecs.xml upwatch.xml

EXTRA_DIST = ${XMLSOURCE}

upwatch.html: ${XMLSOURCE}
	xsltproc -o upwatch.html upwatch.xsl upwatch.xml

upwatch.txt: ${XMLSOURCE}
	lynx -nolist -dump upwatch.html > upwatch.txt

upwatch.pdf: ${XMLSOURCE}
	jw -f docbook -b pdf -d style.dsl upwatch.xml

CLEANFILES = upwatch.txt upwatch.html Makefile.in

