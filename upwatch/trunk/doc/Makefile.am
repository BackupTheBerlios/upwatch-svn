docdir = $(prefix)/doc
doc_DATA = upwatch.html upwatch.txt upwatch.pdf

XMLSOURCE = upwatch.xml addingaprobe.xml administration.xml configuration.xml \
  howitallworks.xml installation.xml interfaces.xml iptraf-specs.xml ping-specs.xml \
  probesspecs.xml upwatch.xml

upwatch.html: ${XMLSOURCE}
	xmllint --valid --noout upwatch.xml
	xsltproc --nonet -o upwatch.html upwatch.xsl upwatch.xml

upwatch.txt: ${XMLSOURCE}
	lynx -nolist -dump upwatch.html > upwatch.txt

# jw wants internet access to www.oasis-open.org. I don't understand catalogs
# enough to disable this
# So I'll just ignore any ererors from jw
upwatch.pdf: ${XMLSOURCE}
	-jw -c /etc/sgml/catalog -f docbook -b pdf -d style.dsl upwatch.xml

CLEANFILES = upwatch.pdf upwatch.txt upwatch.html Makefile.in

