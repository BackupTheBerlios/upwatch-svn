include $(top_srcdir)/Makefile.am.path

nodist_doc_DATA = upwatch.html upwatch.txt upwatch.pdf \
 admin-guide.html admin-guide.txt admin-guide.pdf \
 program-guide.html program-guide.txt program-guide.pdf

if ENABLE_SERVER
DIST_DOC = upwatch.html upwatch.txt upwatch.pdf \
 admin-guide.html admin-guide.txt admin-guide.pdf \
 program-guide.html program-guide.txt program-guide.pdf
else
DIST_DOC = upwatch.html upwatch.txt upwatch.pdf 
endif

UPWATCH_SOURCE = upwatch.xml client-about.xml client-install.xml \
  client-configuration.xml utilities.xml

ADMIN_SOURCE = admin-guide.xml administration.xml configuration.xml \
  installation.xml about.xml

PROGRAM_SOURCE = program-guide.xml addingaprobe.xml \
  howitallworks.xml interfaces.xml probesspecs.xml about.xml ../uw_*/*.xml

XMLSOURCE = ${UPWATCH_SOURCE} ${ADMIN_SOURCE} ${PROGRAM_SOURCE}

if ENABLE_SERVER
EXTRA_DIST = ${XMLSOURCE} ${DIST_DOC} upwatch.xsl jadetex.cfg style.dsl style.xsl style.css
else
EXTRA_DIST = ${DIST_DOC} 
endif

upwatch.html: ${UPWATCH_SOURCE}
	-xmllint --valid --noout upwatch.xml
	-${XSLTPROC} --nonet -o upwatch.html upwatch.xsl upwatch.xml
	touch upwatch.html

upwatch.txt: ${UPWATCH_SOURCE}
	-${LYNX} -nolist -dump upwatch.html > upwatch.txt
	touch upwatch.txt

# jw wants internet access to www.oasis-open.org. I don't understand catalogs
# enough to disable this
# So I'll just ignore any errors from jw
upwatch.pdf: ${UPWATCH_SOURCE}
	-${JW} -c /etc/sgml/catalog -f docbook -b pdf -d style.dsl upwatch.xml
	touch upwatch.pdf

admin-guide.html: ${ADMIN_SOURCE}
	-xmllint --valid --noout admin-guide.xml
	-${XSLTPROC} --nonet -o admin-guide.html upwatch.xsl admin-guide.xml
	touch admin-guide.html

admin-guide.txt: ${ADMIN_SOURCE}
	-${LYNX} -nolist -dump admin-guide.html > admin-guide.txt
	touch admin-guide.txt

admin-guide.pdf: ${ADMIN_SOURCE}
	-${JW} -c /etc/sgml/catalog -f docbook -b pdf -d style.dsl admin-guide.xml
	touch admin-guide.pdf

program-guide.html: ${PROGRAM_SOURCE}
	-xmllint --valid --noout program-guide.xml
	-${XSLTPROC} --nonet -o program-guide.html upwatch.xsl program-guide.xml
	touch program-guide.html

program-guide.txt: ${PROGRAM_SOURCE}
	-${LYNX} -nolist -dump program-guide.html > program-guide.txt
	touch program-guide.txt

program-guide.pdf: ${PROGRAM_SOURCE}
	-${JW} -c /etc/sgml/catalog -f docbook -b pdf -d style.dsl program-guide.xml
	touch program-guide.pdf

CLEANFILES = upwatch.pdf upwatch.txt upwatch.html \
 admin-guide.html admin-guide.txt admin-guide.pdf \
 program-guide.html program-guide.txt program-guide.pdf


