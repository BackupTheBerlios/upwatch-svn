if LINUX
SPECIFIC = findsaddr-linux.c
else
SPECIFIC = findsaddr-generic.c
endif

noinst_LIBRARIES = libupwatch.a
libupwatch_a_SOURCES = slot.c spool.c uwstat.c xml.c util.c setproctitle.c ifaddrlist.c \
 findsaddr-linux.c setsin.c \
 db.h generic.h slot.h spool.h uwstat.h xml.h setproctitle.h ifaddrlist.h \
 findsaddr.h gnuc.h
libupwatch_a_CFLAGS = @LIBXML2_CFLAGS@ @LIBGLIB2_CFLAGS@ @MYSQL_CFLAGS@
