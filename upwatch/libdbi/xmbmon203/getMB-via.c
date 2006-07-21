
/* Direct access to VIA 686 Hardware Monitor Chip */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "methods.h"

/* VIA's HWM Base Address, global */
int  viahwm_base = -1;

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef NETBSD
#include <machine/pio.h>
#endif

#include "io_static.c"


static int ReadByte(int addr)
{
	int ret;
	ret = inb(viahwm_base + addr); WAIT;
	return (ret & 0xFF);
}

static void WriteByte(int addr, int value)
{
	OUTb(viahwm_base + addr, value); WAIT;
}

static int ReadWord(int addr)
{
	int ret;
	ret = inw(viahwm_base + addr); WAIT;
	return (ret & 0xFFFF);
}

static void WriteWord(int addr, int value)
{
	OUTw(viahwm_base + addr, value); WAIT;
}

static int ReadTemp1()
{
	return ReadByte(viahwm_base + 0x21);
}

static int ReadTemp2()
{
	return ReadByte(viahwm_base + 0x1F);
}

struct lm_methods method_via = {
	OpenIO,
	CloseIO,
	ReadByte,
	WriteByte,
	ReadWord,
	WriteWord,
	ReadTemp1,
	ReadTemp2
};
