noinst_LIBRARIES = libst.a
libst_a_SOURCES = sched.c stk.c sync.c key.c io.c st.h common.h  md.h public.h
if LINUX
MYCFLAGS = -DUSE_POLL -DLINUX
endif
if SOLARIS
MYCFLAGS = -DSOLARIS
endif
if CYGWIN
MYCFLAGS = -DCYGWIN
endif
CFLAGS = ${MYCFLAGS}

EXTRA_DIST = osguess.sh

sched.c: st.h

st.h:
	cp public.h st.h
	sed "s/@VERSION@/1.4/g" < st.pc.in > st.pc

