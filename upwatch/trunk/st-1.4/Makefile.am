noinst_LIBRARIES = libst.a
libst_a_SOURCES = sched.c stk.c sync.c key.c io.c st.h common.h  md.h public.h
libst_a_CFLAGS = -DUSE_POLL -DLINUX

EXTRA_DIST = osguess.sh

st.h: public.h
	cp public.h st.h
	sed "s/@VERSION@/1.4/g" < st.pc.in > st.pc

