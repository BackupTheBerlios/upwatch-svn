/* Specific treatment for Tyan TigerMP motherboard */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#undef TyanTigerMP_SMBUS

#include <fcntl.h>
#include <unistd.h>

#include "io_static.c"

#define TTPortCTR	0x2E
#define TTPortDAT	0x2F

#define SENS2_IO_PORT	0xC00

/* external (global) data */
extern int isa_port_base;

#define INDEX_REG_PORT (isa_port_base + 0x05)
#define DATA_PORT      (isa_port_base + 0x06)

/* Winbond Registors */
#define	WINBD_CONFIG	0x40
#define	WINBD_SMBADDR	0x48
#define	WINBD_VENDEX	0x4E
#define	WINBD_VENDID	0x4F
#define	WINBD_TEMPADDR	0x4A

static int readbyte(int addr)
{
	int ret;
	OUTb(INDEX_REG_PORT, addr); WAIT;
	ret = INb(DATA_PORT); WAIT;
	return (ret & 0xFF);
}

static void writebyte(int addr, int value)
{
	OUTb(INDEX_REG_PORT, addr); WAIT;
	OUTb(DATA_PORT, value); WAIT;
}

static int vendercheck(void)
{
	int nv, save, ret;

	ret = 0;
	save = isa_port_base;
	isa_port_base = SENS2_IO_PORT;
	nv = readbyte(WINBD_VENDID) & 0xFF;
	if (nv == 0xA3) {
		writebyte(WINBD_VENDEX, 0x80);
		nv = readbyte(WINBD_VENDID) & 0xFF;
		if (nv == 0x5C)
			ret = 1;
	}
	isa_port_base = save;
	return ret;
}

/* attach 2nd winbond 627HF */
void TyanTigerMPinit()
{
	unsigned char c;

	OpenIO();
	if (vendercheck()) {
		CloseIO();
		/* change ISA IO-port to 627HF */
		isa_port_base = SENS2_IO_PORT;
		return;
	}

	OUTb(TTPortCTR, 0x87);	/* get access to 627HF on TTPortCTR */
	OUTb(TTPortCTR, 0x87);

	OUTb(TTPortCTR, 0x07);
	OUTb(TTPortDAT, 0x0B);	/* want device as 0x0B access */

	OUTb(TTPortCTR, 0x60);	/* want device 0x0B on ISA port base 0x0C00 */
	c = SENS2_IO_PORT >> 8;
	OUTb(TTPortDAT, c);
	OUTb(TTPortCTR, 0x61);
	c = SENS2_IO_PORT & 0xFF;
	OUTb(TTPortDAT, c);

	/* change ISA IO-port to 627HF */
	isa_port_base = SENS2_IO_PORT;
	
	OUTb(TTPortCTR, 0x30);	/* now enabled */
	OUTb(TTPortDAT, 0x01);

#ifdef TyanTigerMP_SMBUS
	OUTb(TTPortCTR, 0x2B);	/* SMBus access enabled */
	c = INb(TTPortDAT) & 0x3F;
	OUTb(TTPortCTR, 0x2B);
	OUTb(TTPortDAT, c);

	writebyte(WINBD_SMBADDR, 0x2E); /* set 627HF on SMBus slave 0x5C(0x2E) */
	writebyte(WINBD_TEMPADDR, 0x32); /* set temp2,3 on SMBus slave 0x94,0x96 */
#endif

	writebyte(WINBD_CONFIG, 0x80);	/* kickstart it */
	writebyte(WINBD_CONFIG, 0x01);

	CloseIO();

	return;
}
