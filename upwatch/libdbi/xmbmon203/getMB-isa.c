
/* ISA IO port method for accessing Hardware Monitor Chip */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "methods.h"

/* ioport_base, global */
int isa_port_base  = IOP_ADDR;

#include <fcntl.h>
#include <unistd.h>
#ifdef NETBSD
#include <machine/pio.h>
#endif

#include "io_static.c"


#define INDEX_REG_PORT (isa_port_base + 0x05)
#define DATA_PORT      (isa_port_base + 0x06)

static int ReadByte(int addr)
{
	int ret;
	OUTb(INDEX_REG_PORT, addr); WAIT;
	ret = inb(DATA_PORT); WAIT;
	return (ret & 0xFF);
}

static void WriteByte(int addr, int value)
{
	OUTb(INDEX_REG_PORT, addr); WAIT;
	OUTb(DATA_PORT, value); WAIT;
}

static int ReadWord(int addr)
{
	int ret;
	OUTb(INDEX_REG_PORT, addr); WAIT;
	ret = inw(DATA_PORT); WAIT;
	return (ret & 0xFFFF);
}

static void WriteWord(int addr, int value)
{
	OUTb(INDEX_REG_PORT, addr); WAIT;
	OUTw(DATA_PORT, value); WAIT;
}

static int ReadTemp1()
{
	int ret;
	WriteByte(0x4E, 0x01);	/* changing to bank 1 */
	ret = ReadWord(0x50);
	WriteByte(0x4E, 0x00);	/* returning to bank 0 */
	return ret;
}

static int ReadTemp2()
{
	int ret;
	WriteByte(0x4E, 0x02);	/* changing to bank 2 */
	ret = ReadWord(0x50);
	WriteByte(0x4E, 0x00);	/* returning to bank 0 */
	return ret;
}

struct lm_methods method_isa = {
	OpenIO,
	CloseIO,
	ReadByte,
	WriteByte,
	ReadWord,
	WriteWord,
	ReadTemp1,
	ReadTemp2
};
