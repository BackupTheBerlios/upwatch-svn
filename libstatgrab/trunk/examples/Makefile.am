# Makefile for libstatgrab/examples
# http://www.i-scream.org/libstatgrab/
# $Id: Makefile.am,v 1.12 2004/04/03 16:55:38 tdb Exp $

if EXAMPLES
noinst_PROGRAMS = network_traffic disk_traffic cpu_usage load_stats process_stats os_info vm_stats user_list page_stats network_iface_stats process_snapshot
endif

INCLUDES = -I$(top_srcdir) -I$(top_builddir)\
           -I$(top_srcdir)/src -I$(top_builddir)/src\
           -I$(top_srcdir)/src/libstatgrab -I$(top_builddir)/src/libstatgrab

LDADD = $(top_builddir)/src/libstatgrab/libstatgrab.la @LINKFLAGS@
