include $(top_srcdir)/Makefile.am.path

if HAVE_CURSES
UWSAIDAR = uwsaidar
endif

if SOLARIS
SOLARISLIBS = -lkstat -ldevinfo
endif

if CYGWIN
MYCFLAGS = -DCYGWIN
endif

bin_PROGRAMS = ctime slot $(UWSAIDAR) uwq
man_MANS = ctime.1 slot.1 uwq.1
INCLUDES = -I../upwatch 
BUILT_SOURCES = 
EXTRA_DIST = ctime.1 slot.1 uwq.1

if HAVE_CURSES
uwsaidar_SOURCES = uwsaidar.c
uwsaidar_LDADD = $(LIBSTATGRAB) $(SOLARISLIBS) @LIBCURSES_LIBS@ 
uwsaidar_CFLAGS = -I$(INCL_STATGRAB) $(MYCFLAGS)
endif

include $(top_srcdir)/Makefile.am.common

uwq_SOURCES = uwq.c uwq_options.h uwq_options.c uwq_options.def 
uwq_LDADD = $(LIBOPTS) @LIBGLIB2_LIBS@
uwq_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@
BUILT_SOURCES += uwq_options.h uwq.1
EXTRA_DIST += uwq_options.def
CLEAN_FILES = uwq_options.h uwq_options.c

uwq_options.h: uwq_options.def ../common/minimal_options.def
	autogen -b uwq_options $(AGDEFINES) $(GENINCL) -L $(top_srcdir)/common uwq_options.def
	autogen -b uwq $(AGDEFINES) $(GENINCL) -T $(top_srcdir)/common/man1.tpl -L $(top_srcdir)/common uwq_options.def

uwq.1: uwq_options.def $(top_srcdir)/common/man1.tpl
	autogen -b uwq $(AGDEFINES) $(GENINCL) -T $(MAN1TPL) uwq_options.def

ctime_SOURCES = ctime.c ctime_options.c ctime_options.h ctime_options.def 
ctime_LDADD = $(LIBOPTS) 
BUILT_SOURCES += ctime_options.h ctime.1
EXTRA_DIST += ctime_options.def
CLEAN_FILES += ctime_options.c ctime_options.h

ctime_options.h: ctime_options.def ../common/minimal_options.def
	autogen -b ctime_options $(AGDEFINES) $(GENINCL) -L $(top_srcdir)/common ctime_options.def
	autogen -b ctime $(AGDEFINES) $(GENINCL) -T $(top_srcdir)/common/man1.tpl -L $(top_srcdir)/common ctime_options.def

ctime.1: ctime_options.def $(top_srcdir)/common/man1.tpl
	autogen -b ctime $(AGDEFINES) $(GENINCL) -T $(MAN1TPL) ctime_options.def

slot_SOURCES = slot.c slot_options.c slot_options.h slot_options.def
slot_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@
slot_LDADD = $(LIBOPTS) $(LIBUPWATCH) 
BUILT_SOURCES += slot_options.h
EXTRA_DIST += slot_options.def
CLEAN_FILES +=  slot_options.c slot_options.h

slot_options.h: slot_options.def ../common/minimal_options.def
	autogen -b slot_options $(AGDEFINES) $(GENINCL) -L $(top_srcdir)/common slot_options.def
	autogen -b slot $(AGDEFINES) $(GENINCL) -T $(top_srcdir)/common/man1.tpl -L $(top_srcdir)/common slot_options.def

slot.1: slot_options.def $(top_srcdir)/common/man1.tpl
	autogen -b slot $(AGDEFINES) $(GENINCL) -T $(MAN1TPL) slot_options.def

