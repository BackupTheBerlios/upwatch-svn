program = uw_iptraf
uw_iptraf_SOURCES = run.c cmd_options.c cmd_options.h iptraf.c iptraf.h \
  cmd_options.def $(program).spec-generic  $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def 
LDADD = $(MAIN_O) $(LIBUPWATCH) @LIBGLIB2_LIBS@ @LIBNET_LIBS@ @LIBPCAP_LIBS@
include $(top_srcdir)/Makefile.am.common

tp : tp.o iptraf.o
	$(LINK) tp.o iptraf.o $(LIBUPWATCH) @LIBGLIB2_LIBS@ @LIBNET_LIBS@ @LIBPCAP_LIBS@
