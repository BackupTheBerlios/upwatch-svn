/* CPU out IO different in FreeBSD and Linux */

#if !defined(__io_cpu_h__)
#define	__io_cpu_h__

#ifdef LINUX	/* LINUX */
#include <sys/io.h>
#define OUTb(x,y)	outb((y),(x))
#define OUTw(x,y)	outw((y),(x))
#define OUTl(x,y)	outl((y),(x))
#define WAIT	outb(0x00,0xEB)
#else			/* FreeBSD, NetBSD */
#include <machine/cpufunc.h>
#define OUTb(x,y)	outb((x),(y))
#define OUTw(x,y)	outw((x),(y))
#define OUTl(x,y)	outl((x),(y))
#define WAIT	outb(0xEB,0x00)
#endif

#ifdef LINUX	/* LINUX */
extern int iopl_counter;
#elif NETBSD
#else			/* FreeBSD */
extern int iofl;
#endif

#endif	/*__io_cpu_h__*/
