docdir = $(prefix)/doc
doc_DATA = upwatch.html upwatch.txt upwatch.pdf

XMLSOURCE = upwatch.xml addingaprobe.xml administration.xml configuration.xml \
  howitallworks.xml installation.xml interfaces.xml iptraf-specs.xml ping-specs.xml \
  probesspecs.xml upwatch.xml

EXTRA_DIST = ${XMLSOURCE}

upwatch.html: ${XMLSOURCE}
	xmllint --valid --noout upwatch.xml
	xsltproc -o upwatch.html upwatch.xsl upwatch.xml

upwatch.txt: ${XMLSOURCE}
	lynx -nolist -dump upwatch.html > upwatch.txt

upwatch.pdf: ${XMLSOURCE}
	jw -c /etc/sgml/catalog -f docbook -b pdf -d style.dsl upwatch.xml

CLEANFILES = upwatch.txt upwatch.html Makefile.in

