bin_PROGRAMS = ctime slot
man_MANS = ctime.1 slot.1
INCLUDES = -I../upwatch 
BUILT_SOURCES = 
EXTRA_DIST = ctime.1 slot.1

include $(top_srcdir)/Makefile.am.libs

ctime_SOURCES = ctime_options.c ctime_options.h ctime_options.def 
ctime_LDADD = ctime.o $(LIBOPTS) 
BUILT_SOURCES += ctime_options.h
EXTRA_DIST += ctime_options.def

ctime_options.h: ctime_options.def ../common/minimal_options.def
	autogen -b ctime_options -L /usr/share/autogen ctime_options.def
	autogen -b ctime -L /usr/share/autogen -T ../common/man1.tpl ctime_options.def

slot_SOURCES = slot_options.c slot_options.h slot_options.def
slot_LDADD = slot.o $(LIBOPTS) $(LIBUPWATCH) 
BUILT_SOURCES += slot_options.h
EXTRA_DIST += slot_options.def

slot_options.h: slot_options.def ../common/minimal_options.def
	autogen -b slot_options -L /usr/share/autogen slot_options.def
	autogen -b slot -L /usr/share/autogen -T ../common/man1.tpl slot_options.def

CLEANFILES = $(BUILT_SOURCES) $(bin_PROGRAMS) Makefile.in

