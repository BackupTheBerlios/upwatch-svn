include $(top_srcdir)/Makefile.am.path

bin_PROGRAMS = ctime slot uwsaidar
man_MANS = ctime.1 slot.1
INCLUDES = -I../upwatch 
BUILT_SOURCES = 
EXTRA_DIST = ctime.1 slot.1

uwsaidar_SOURCES = uwsaidar.c
uwsaidar_LDADD = $(LIBSTATGRAB) @LIBNCURSES_LIBS@
uwsaidar_CFLAGS = -I$(INCL_STATGRAB)

include $(top_srcdir)/Makefile.am.common

ctime_SOURCES = ctime.c ctime_options.c ctime_options.h ctime_options.def 
ctime_LDADD = $(LIBOPTS) 
BUILT_SOURCES += ctime_options.h ctime.1
EXTRA_DIST += ctime_options.def

ctime_options.h: ctime_options.def ../common/minimal_options.def
	autogen -b ctime_options $(GENINCL) -L $(top_srcdir)/common ctime_options.def
	autogen -b ctime $(GENINCL) -T $(top_srcdir)/common/man1.tpl -L $(top_srcdir)/common ctime_options.def

ctime.1: ctime_options.def $(top_srcdir)/common/man1.tpl
	autogen -b ctime $(AGDEFINES) $(GENINCL) -T $(MAN1TPL) ctime_options.def

slot_SOURCES = slot.c slot_options.c slot_options.h slot_options.def
slot_CFLAGS = @LIBGLIB2_CFLAGS@ @LIBXML2_CFLAGS@ @MYSQL_CFLAGS@
slot_LDADD = $(LIBOPTS) $(LIBUPWATCH) 
BUILT_SOURCES += slot_options.h
EXTRA_DIST += slot_options.def

slot_options.h: slot_options.def ../common/minimal_options.def
	autogen -b slot_options $(GENINCL) -L $(top_srcdir)/common slot_options.def
	autogen -b slot $(GENINCL) -T $(top_srcdir)/common/man1.tpl -L $(top_srcdir)/common slot_options.def

slot.1: slot_options.def $(top_srcdir)/common/man1.tpl
	autogen -b slot $(AGDEFINES) $(GENINCL) -T $(MAN1TPL) slot_options.def

