include $(top_srcdir)/Makefile.am.path

doc_DATA = upwatch.html upwatch.txt upwatch.pdf

XMLSOURCE = upwatch.xml addingaprobe.xml administration.xml configuration.xml \
  howitallworks.xml installation.xml interfaces.xml iptraf-specs.xml ping-specs.xml \
  probesspecs.xml upwatch.xml

EXTRA_DIST = ${XMLSOURCE} upwatch.xsl jadetex.cfg style.dsl style.xsl style.css

upwatch.html: ${XMLSOURCE}
	xmllint --valid --noout upwatch.xml
	-${XSLTPROC} --nonet -o upwatch.html upwatch.xsl upwatch.xml
	touch upwatch.html

upwatch.txt: ${XMLSOURCE}
	-${LYNX} -nolist -dump upwatch.html > upwatch.txt
	touch upwatch.txt

# jw wants internet access to www.oasis-open.org. I don't understand catalogs
# enough to disable this
# So I'll just ignore any ererors from jw
upwatch.pdf: ${XMLSOURCE}
	-${JW} -c /etc/sgml/catalog -f docbook -b pdf -d style.dsl upwatch.xml
	touch upwatch.pdf

CLEANFILES = upwatch.pdf upwatch.txt upwatch.html Makefile.in


