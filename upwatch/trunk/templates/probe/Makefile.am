program = uw_template
uw_template_SOURCES = run.c cmd_options.c cmd_options.h cmd_options.def $(program).spec-generic \
 $(program).init
BUILT_SOURCES = cmd_options.h  $(program).spec-generic $(program).init $(program).spec-install
INCLUDES = -I../upwatch 
EXTRA_DIST = cmd_options.def $(program).mysql
LDADD = $(MAIN_O) $(LIBUPWATCH) @LIBGLIB2_LIBS@
include $(top_srcdir)/Makefile.am.common

