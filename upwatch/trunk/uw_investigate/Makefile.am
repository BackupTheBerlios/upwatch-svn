include $(top_srcdir)/Makefile.am.path

program = uw_investigate
sbin_PROGRAMS = $(program)
man_MANS = $(program).1
conf_DATA = $(program).conf
redhat_DATA = $(program).redhat
suse_DATA = $(program).suse

uw_investigate_SOURCES = run.c cmd_options.c cmd_options.h traceroute.c traceroute.h \
  cmd_options.def $(program).spec-generic  $(program).redhat $(program).suse
uw_investigate_CFLAGS = @MYSQL_CFLAGS@ @LIBGTHREAD2_CFLAGS@ @LIBXML2_CFLAGS@ @LIBNET_CFLAGS@ @LIBPCAP_CFLAGS@ # -DUSE_THREADS
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).redhat $(program).suse $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).conf
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGTHREAD2_LIBS@ @LIBPCAP_LIBS@ @LIBNET_LIBS@
include $(top_srcdir)/Makefile.am.common

tp : tp.o traceroute.o
	$(LINK) tp.o traceroute.o $(LIBUPWATCH) @LIBGLIB2_LIBS@ @LIBNET_LIBS@ @LIBPCAP_LIBS@

