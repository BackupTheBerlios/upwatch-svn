program = uw_investigate
uw_investigate_SOURCES = run.c cmd_options.c cmd_options.h traceroute.c traceroute.h \
  cmd_options.def $(program).spec-generic  $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def 
LDADD = $(MAIN_O) $(LIBOPTS) $(LIBUPWATCH) @LIBGLIB2_LIBS@ @LIBNET_LIBS@ @LIBPCAP_LIBS@
include $(top_srcdir)/Makefile.am.common

tp : tp.o traceroute.o
	$(LINK) tp.o traceroute.o $(LIBUPWATCH) @LIBGLIB2_LIBS@ @LIBNET_LIBS@ @LIBPCAP_LIBS@

