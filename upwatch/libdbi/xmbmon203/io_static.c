/* static open/close IO routines used some places */

#include "io_cpu.h"
#ifdef NETBSD
#include "machine/sysarch.h"
#endif

static int OpenIO()
{
#ifdef LINUX	/* LINUX */
	if (iopl_counter == 0) {
		if (iopl(3) < 0)
			return -1;
	}
	++iopl_counter;
#elif NETBSD
	return i386_iopl(1);
#else			/* FreeBSD */
	if ((iofl = open("/dev/io",000)) < 0)
		return -1;
#endif
	return 0;
}
static void CloseIO()
{
#ifdef LINUX	/* LINUX */
	if (iopl_counter == 1)
		iopl(0);
	if (iopl_counter > 0)
		--iopl_counter;
#elif NETBSD
	i386_iopl(0);
#else			/* FreeBSD */
	close(iofl);
#endif
}
