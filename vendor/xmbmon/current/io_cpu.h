#if !defined(__io_cpu_h__)
#define	__io_cpu_h__

#ifdef LINUX				/* LINUX */
#include <sys/io.h>
#define   SET_IOPL()	iopl(3)
#define RESET_IOPL()	iopl(0)

#elif defined(__svr4__)		/* Solaris for x86 */
#include <sys/sysi86.h>
#include <sys/psw.h>
#ifndef SI86IOPL
#define   SET_IOPL()	sysi86(SI86V86,V86SC_IOPL,PS_IOPL)
#define RESET_IOPL()	sysi86(SI86V86,V86SC_IOPL,0)
#else
#define   SET_IOPL()	sysi86(SI86IOPL,3)
#define RESET_IOPL()	sysi86(SI86IOPL,0)
#endif

#elif NETBSD			/* NetBSD, OpenBSD */
#include "machine/sysarch.h"
#define   SET_IOPL()	i386_iopl(1)
#define RESET_IOPL()	i386_iopl(0)

#else					/* FreeBSD */
extern int iofl;
#define   SET_IOPL()	(iofl = open("/dev/io",000))
#define RESET_IOPL()	close(iofl)
#endif

extern int iopl_counter;

/*
 * These assume GCC is being used with GAS.
 */

static __inline__ unsigned char
my_inb(unsigned short port)
{
	unsigned char ret;
	__asm__ __volatile__("inb %1,%0" : "=a" (ret) : "d" (port));
	return ret;
}

static __inline__ unsigned short
my_inw(unsigned short port)
{
	unsigned short ret;
	__asm__ __volatile__("inw %1,%0" : "=a" (ret) : "d" (port));
	return ret;
}

static __inline__ unsigned int
my_inl(unsigned short port)
{
	unsigned int ret;
	__asm__ __volatile__("inl %1,%0" : "=a" (ret) : "d" (port));
	return ret;
}

static __inline__ void
my_outb(unsigned short port, unsigned char val)
{
	__asm__ __volatile__("outb %0,%1" : : "a" (val), "d" (port));
}

static __inline__ void
my_outw(unsigned short port, unsigned short val)
{
	__asm__ __volatile__("outw %0,%1" : : "a" (val), "d" (port));
}

static __inline__ void
my_outl(unsigned short port, unsigned int val)
{
	__asm__ __volatile__("outl %0,%1" : : "a" (val), "d" (port));
}

#define INb(x)	my_inb((x))
#define INw(x)	my_inw((x))
#define INl(x)	my_inl((x))
#define OUTb(x,y)	my_outb((x),(y))
#define OUTw(x,y)	my_outw((x),(y))
#define OUTl(x,y)	my_outl((x),(y))
#define WAIT	my_outb(0xEB,0x00)

#endif	/*__io_cpu_h__*/
