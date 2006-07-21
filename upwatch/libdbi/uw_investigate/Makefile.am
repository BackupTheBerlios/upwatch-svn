include $(top_srcdir)/Makefile.am.path

program = uw_investigate
AM_CFLAGS = -DWITH_THREADS
sbin_PROGRAMS = $(program)
man_MANS = uw_investigate.1
conf_DATA = $(program).conf
init_DATA = $(INITFILES)

uw_investigate_SOURCES = run.c cmd_options.c cmd_options.h traceroute.c traceroute.h \
  cmd_options.def $(PROBFILES) $(SPECFILES) $(INITFILES)
uw_investigate_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBNET_CFLAGS@ \
@LIBOPTS_CFLAGS@  @LIBPCAP_CFLAGS@ $(AM_CFLAGS)
BUILT_SOURCES = cmd_options.h $(PROBFILES) $(SPECFILES) $(INITFILES)
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCHT) @LIBGTHREAD2_LIBS@ @LIBPCAP_LIBS@ @LIBNET_LIBS@
include $(top_srcdir)/Makefile.am.common

tp : tp.o traceroute.o
	$(LINK) tp.o traceroute.o $(LIBUPWATCH) @LIBGLIB2_LIBS@ @LIBNET_LIBS@ @LIBPCAP_LIBS@

install-data-local:
	if test `${ID} -u` -eq 0; then \
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTRIR}${localstatedir}/spool/upwatch/$(program) ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTRIR}${localstatedir}/spool/upwatch/$(program)/new ;\
	  ${INSTALL} -d -m 2770 -o upwatch -g upwatch ${DESTRIR}${localstatedir}/spool/upwatch/$(program)/tmp ;\
	else \
	  mkdir -p ${DESTRIR}${localstatedir}/spool/upwatch/$(program) ;\
	  mkdir -p ${DESTRIR}${localstatedir}/spool/upwatch/$(program)/new ;\
	  mkdir -p ${DESTRIR}${localstatedir}/spool/upwatch/$(program)/tmp ;\
	fi
