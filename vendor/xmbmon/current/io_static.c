/* static open/close IO routines used some places */

#include "io_cpu.h"

static int OpenIO()
{
	if (iopl_counter == 0) {
		if (SET_IOPL() < 0)
			return -1;
	}
	++iopl_counter;
	return 0;
}

static void CloseIO()
{
	if (iopl_counter == 1)
		RESET_IOPL();
	if (iopl_counter > 0)
		--iopl_counter;
}
